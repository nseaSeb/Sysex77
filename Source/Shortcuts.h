/*
  ==============================================================================

    Shortcuts.h
    Raccourcis clavier configurables. Chaque action a une touche par defaut ; la touche
    choisie est stockee dans les reglages ("Shortcut_<id>", KeyPress::getTextDescription).
    Dispatch : MainWindow (KeyListener, recoit tout ce qu'aucun enfant n'a consomme)
    -> MidiDemo::handleShortcut (id). Edition : ShortcutsEditor (Reglages > General).

  ==============================================================================
*/

#pragma once

namespace Shortcuts
{
    struct Action { const char* id; const char* label; const char* defaultKey; };

    // Onglets dans l'ordre de la barre (index = position). "close" = ferme l'editeur
    // ouvert par-dessus l'ecran Voice.
    inline const std::vector<Action>& actions()
    {
        static const std::vector<Action> a {
            { "tab0",  "Setting tab",       "command + 1" },
            { "tab1",  "Library tab",       "command + 2" },
            { "tab2",  "Voice tab",         "command + 3" },
            { "tab3",  "Effects tab",       "command + 4" },
            { "tab4",  "Controllers tab",   "command + 5" },
            { "tab5",  "Midi Setting tab",  "command + 6" },
            { "close", "Close editor",      "escape" },
            { "seqplay", "Sequencer play/stop", "command + p" },
            { "undo",    "Undo",                "command + Z" },
            { "redo",    "Redo",                "command + shift + Z" },
            { "voicenext", "Next voice (audition)",     "command + cursor down" },
            { "voiceprev", "Previous voice (audition)", "command + cursor up" },
            { "banknext",  "Next bank",                 "command + cursor right" },
            { "bankprev",  "Previous bank",             "command + cursor left" },
            { "ab",        "Compare A/B",               "command + B" },
            { "newvoice",  "New voice...",              "command + N" },
            { "presethex", "Preset hex view (developer)", "command + shift + H" },
            { "help",      "Show keyboard shortcuts",   "command + /" } };
        return a;
    }

    inline juce::String settingKey (const juce::String& id) { return "Shortcut_" + id; }

    inline juce::KeyPress get (const Action& a)
    {
        const auto s = getAppSettings()->getValue (settingKey (a.id), a.defaultKey);
        return juce::KeyPress::createFromDescription (s);
    }
    inline void set (const Action& a, const juce::KeyPress& k)
    {
        getAppSettings()->setValue (settingKey (a.id), k.isValid() ? k.getTextDescription() : juce::String());
    }
    inline void resetAll()
    {
        for (auto& a : actions()) getAppSettings()->removeValue (settingKey (a.id));
    }
    /** Chiffre du pave numerique -> chiffre de la rangee (Cmd+5 marche avec les deux). */
    inline juce::KeyPress normalise (const juce::KeyPress& k)
    {
        const int c = k.getKeyCode();
        for (int d = 0; d <= 9; ++d)
            if (c == juce::KeyPress::numberPad0 + d)
                return juce::KeyPress ('0' + d, k.getModifiers(), k.getTextCharacter());
        // Rangee des chiffres d'un Mac AZERTY (sans Maj) : & e-aigu " ' ( paragraphe e-grave
        // ! c-cedille a-grave = 1 2 3 4 5 6 7 8 9 0. Codes Unicode (pas de litteral UTF-8 ici).
        static const int azerty[10] = { 0xE0, '&', 0xE9, '"', '\'', '(', 0xA7, 0xE8, '!', 0xE7 };
        const int lc = (int) juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) c);
        for (int d = 0; d <= 9; ++d)
            if (lc == azerty[d])
                return juce::KeyPress ('0' + d, k.getModifiers(), k.getTextCharacter());
        return k;
    }
    /** Id de l'action liee a cette touche, ou chaine vide. */
    inline juce::String find (const juce::KeyPress& key)
    {
        const auto k = normalise (key);
        for (auto& a : actions())
            if (get (a).isValid() && normalise (get (a)) == k) return a.id;
        // Symbol typed with Shift on this layout (e.g. "/" = Shift+: on a French Mac):
        // retry with the typed character and without Shift.
        const auto ch = key.getTextCharacter();
        if (ch > 32 && ch < 128 && ! juce::CharacterFunctions::isLetterOrDigit (ch)
            && key.getModifiers().isShiftDown())
        {
            const juce::KeyPress alt (ch, key.getModifiers().withoutFlags (juce::ModifierKeys::shiftModifier), ch);
            for (auto& a : actions())
                if (get (a).isValid() && normalise (get (a)) == alt) return a.id;
        }
        return {};
    }

    /** Global dispatch (installed by MainWindow). */
    inline std::function<bool (const juce::KeyPress&)>& dispatcher()
    {
        static std::function<bool (const juce::KeyPress&)> d;
        return d;
    }

    /** KeyListener for components that would swallow a shortcut (ListBox eats the arrows,
        even with Cmd): key listeners run BEFORE the component's own keyPressed. */
    struct Forwarder : juce::KeyListener
    {
        bool keyPressed (const juce::KeyPress& k, juce::Component*) override
        {
            if (! k.getModifiers().isCommandDown() && ! k.getModifiers().isAltDown()) return false;   // plain keys stay with the list
            return find (k).isNotEmpty() && dispatcher() && dispatcher() (k);
        }
        static Forwarder& get() { static Forwarder f; return f; }
    };

    /** " (Cmd+Z)"-style suffix for tooltips, from the CURRENT binding ("" when none). */
    inline juce::String hint (const char* id)
    {
        for (auto& a : actions())
            if (juce::String (a.id) == id)
            {
                const auto k = get (a);
                return k.isValid() ? "  (" + k.getTextDescriptionWithIcons() + ")" : juce::String();
            }
        return {};
    }

    /** Broadcast when the bindings change, so tooltips can refresh. */
    inline juce::ChangeBroadcaster& changed() { static juce::ChangeBroadcaster b; return b; }

    //==========================================================================
    /** Read-only cheat sheet (Cmd+/): every action with its current key. */
    class CheatSheet : public juce::Component
    {
    public:
        CheatSheet()
        {
            for (auto& a : actions())
            {
                const auto k = get (a);
                rows.add ({ TRANS (a.label), k.isValid() ? k.getTextDescriptionWithIcons() : TRANS ("(none)") });
            }
            setSize (420, 56 + rows.size() * 24 + 36);
        }
        void paint (juce::Graphics& g) override
        {
            g.fillAll (SYPal.background);
            auto r = getLocalBounds().reduced (16, 12);
            g.setColour (SYColSelected);
            g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
            g.drawText (TRANS ("Keyboard shortcuts"), r.removeFromTop (28), juce::Justification::centredLeft);
            r.removeFromTop (6);
            g.setFont (juce::FontOptions (13.5f));
            for (auto& row : rows)
            {
                auto line = r.removeFromTop (24);
                g.setColour (SYColLabel);
                g.drawText (row.first, line.removeFromLeft (line.getWidth() * 3 / 5), juce::Justification::centredLeft);
                g.setColour (SYColSelected);
                g.drawText (row.second, line, juce::Justification::centredRight);
            }
            g.setColour (SYColLabel.withAlpha (0.6f));
            g.setFont (juce::FontOptions (12.0f));
            g.drawText (TRANS ("Change them in Setting > General > Keyboard shortcuts"),
                        r.removeFromBottom (24), juce::Justification::centredLeft);
        }
    private:
        juce::Array<std::pair<juce::String, juce::String>> rows;
    };

    //==========================================================================
    /** Bouton de capture : clic -> "Appuyez sur une touche..." -> la touche suivante est
        enregistree (une touche deja prise par une autre action lui est retiree). */
    class KeyCaptureButton : public juce::TextButton
    {
    public:
        explicit KeyCaptureButton (const Action& a) : action (a)
        {
            setWantsKeyboardFocus (true);
            refresh();
            onClick = [this] { capturing = true; setButtonText (TRANS ("Press a key...")); grabKeyboardFocus(); };
        }
        std::function<void()> onChanged;
        void refresh() { const auto k = get (action); setButtonText (k.isValid() ? k.getTextDescriptionWithIcons() : TRANS ("(none)")); }

        bool keyPressed (const juce::KeyPress& k) override
        {
            if (! capturing) return false;
            capturing = false;
            if (k.getKeyCode() == juce::KeyPress::backspaceKey && k.getModifiers().isAnyModifierKeyDown() == false)
                set (action, {});                                     // Retour arriere = aucun raccourci
            else
            {
                const auto other = find (k);
                for (auto& a : actions()) if (other == a.id && other != action.id) set (a, {});
                set (action, normalise (k));
            }
            if (onChanged) onChanged();
            changed().sendChangeMessage();
            return true;                                              // consomme : pas de dispatch
        }
        void focusLost (FocusChangeType) override { if (capturing) { capturing = false; refresh(); } }
    private:
        const Action& action;
        bool capturing = false;
    };

    //==========================================================================
    class ShortcutsEditor : public juce::Component
    {
    public:
        ShortcutsEditor()
        {
            for (auto& a : actions())
            {
                auto* l = labels.add (new juce::Label ({}, TRANS (a.label)));
                addAndMakeVisible (l);
                auto* b = buttons.add (new KeyCaptureButton (a));
                b->onChanged = [this] { for (auto* x : buttons) x->refresh(); };
                addAndMakeVisible (b);
            }
            hint.setText (TRANS ("Click a shortcut, then press the new key. Backspace = none."), juce::dontSendNotification);
            hint.setFont (juce::FontOptions (12.0f));
            addAndMakeVisible (hint);
            addAndMakeVisible (btDefaults);
            btDefaults.onClick = [this] { resetAll(); for (auto* x : buttons) x->refresh(); changed().sendChangeMessage(); };
            setSize (440, 60 + (int) actions().size() * 32 + 50);
        }
        void paint (juce::Graphics& g) override { g.fillAll (SYPal.background); }
        void resized() override
        {
            auto r = getLocalBounds().reduced (14);
            hint.setBounds (r.removeFromTop (24));
            r.removeFromTop (6);
            for (int i = 0; i < labels.size(); ++i)
            {
                auto row = r.removeFromTop (32).reduced (0, 3);
                labels[i]->setBounds (row.removeFromLeft (row.getWidth() / 2));
                buttons[i]->setBounds (row);
            }
            r.removeFromTop (8);
            btDefaults.setBounds (r.removeFromTop (28).removeFromRight (180));
        }
    private:
        juce::OwnedArray<juce::Label> labels;
        juce::OwnedArray<KeyCaptureButton> buttons;
        juce::Label hint;
        juce::TextButton btDefaults { TRANS ("Default shortcuts") };
    };
}
