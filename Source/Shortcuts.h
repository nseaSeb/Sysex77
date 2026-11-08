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
            { "close", "Close editor",      "escape" } };
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
        return k;
    }
    /** Id de l'action liee a cette touche, ou chaine vide. */
    inline juce::String find (const juce::KeyPress& key)
    {
        const auto k = normalise (key);
        for (auto& a : actions())
            if (get (a).isValid() && normalise (get (a)) == k) return a.id;
        return {};
    }

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
            btDefaults.onClick = [this] { resetAll(); for (auto* x : buttons) x->refresh(); };
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
