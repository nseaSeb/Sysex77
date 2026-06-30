/*
  ==============================================================================

    ThemeBuilder.h
    Constructeur de thème interactif : choisir les styles (boutons / panneaux /
    sliders / potards) et les couleurs par rôle, voir l'aperçu en direct (toute
    l'app se met à jour via la palette globale), puis sauver en theme.xml.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <functional>
#include "LookAndFeel.h"
#include "Themes.h"

class ThemeBuilder : public juce::Component,
                     private juce::ChangeListener
{
public:
    ThemeBuilder()
    {
        work = SYPal;                       // on part du thème courant

        addStyleCombo (cbButtons, "Boutons",  { "square","round","flat","fun","outline" },     work.buttonStyle);
        addStyleCombo (cbPanels,  "Panneaux", { "square","round","flat","fun" },               work.panelStyle);
        addStyleCombo (cbSliders, "Sliders",  { "bar","led","rail","groove","notch","dots" },  work.sliderStyle);
        addStyleCombo (cbKnobs,   "Potards",  { "arc","dot","vintage","tick","neon","arctick" }, work.knobStyle);

        cbButtons.onChange = [this] { work.buttonStyle = cbButtons.getText(); apply(); };
        cbPanels .onChange = [this] { work.panelStyle  = cbPanels .getText(); apply(); };
        cbSliders.onChange = [this] { work.sliderStyle = cbSliders.getText(); apply(); };
        cbKnobs  .onChange = [this] { work.knobStyle   = cbKnobs  .getText(); apply(); };

        addAndMakeVisible (tgDark);
        tgDark.setButtonText ("Theme sombre");
        tgDark.setToggleState (work.dark, juce::dontSendNotification);
        tgDark.onClick = [this] { work.dark = tgDark.getToggleState(); apply(); };

        addColour ("Fond",         &work.background);
        addColour ("Surface",      &work.surface);
        addColour ("Surface alt",  &work.surfaceAlt);
        addColour ("Bordure",      &work.panelBorder);
        addColour ("Texte",        &work.textPrimary);
        addColour ("Texte att.",   &work.textMuted);
        addColour ("Accent",       &work.accent);
        addColour ("Accent 2",     &work.accentSecondary);
        addColour ("Piste potard", &work.knobTrack);
        addColour ("Remplissage",  &work.knobFill);
        addColour ("Corps potard", &work.knobBody);

        addAndMakeVisible (labName);
        labName.setText ("Nom :", juce::dontSendNotification);
        addAndMakeVisible (edName);
        edName.setText (work.name == "Custom" || work.name.isEmpty() ? "Mon theme" : work.name, false);
        addAndMakeVisible (btSave);
        btSave.setButtonText ("Sauver le theme");
        btSave.onClick = [this] { save(); };
        addAndMakeVisible (btClose);
        btClose.setButtonText ("Fermer");
        btClose.onClick = [this] { if (auto* dw = findParentComponentOfClass<juce::DialogWindow>()) dw->exitModalState (0); };
        addAndMakeVisible (labStatus);
        labStatus.setJustificationType (juce::Justification::centredLeft);

        setupPreview();
        apply();
        setSize (760, 560);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (SYPal.background);
        g.setColour (SYPal.panelBorder);
        g.drawVerticalLine (controlsW, 8.0f, (float) getHeight() - 8.0f);
        g.setColour (SYPal.textMuted);
        g.setFont (juce::Font (juce::FontOptions (13.0f)).boldened());
        g.drawText ("APERCU", controlsW + 14, 8, getWidth() - controlsW - 20, 20, juce::Justification::left, false);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12);
        auto left = area.removeFromLeft (controlsW - 24);

        auto rowH = 26;
        auto comboRow = [&] (juce::Label& l, juce::ComboBox& c)
        {
            auto r = left.removeFromTop (rowH);
            l.setBounds (r.removeFromLeft (90));
            c.setBounds (r);
            left.removeFromTop (4);
        };
        comboRow (lbButtons, cbButtons);
        comboRow (lbPanels,  cbPanels);
        comboRow (lbSliders, cbSliders);
        comboRow (lbKnobs,   cbKnobs);
        tgDark.setBounds (left.removeFromTop (rowH));
        left.removeFromTop (8);

        // Pastilles de couleur en 2 colonnes.
        const int n = (int) colours.size();
        const int rows = (n + 1) / 2;
        auto grid = left.removeFromTop (rows * (rowH + 2));
        for (int i = 0; i < n; ++i)
        {
            auto& cc = colours[(size_t) i];
            const int col = i % 2, row = i / 2;
            const int cw = (grid.getWidth() - 8) / 2;
            juce::Rectangle<int> cell (grid.getX() + col * (cw + 8), grid.getY() + row * (rowH + 2), cw, rowH);
            cc.label->setBounds (cell.removeFromLeft (cw - 34));
            cc.swatch->setBounds (cell);
        }
        left.removeFromTop (10);

        auto nameRow = left.removeFromTop (rowH);
        labName.setBounds (nameRow.removeFromLeft (52));
        edName.setBounds (nameRow);
        left.removeFromTop (6);
        auto btnRow = left.removeFromTop (rowH + 4);
        btSave.setBounds (btnRow.removeFromLeft (controlsW / 2 - 20).reduced (2));
        btClose.setBounds (btnRow.reduced (2));
        labStatus.setBounds (left.removeFromTop (rowH));

        // ---- Aperçu (colonne droite) ----
        auto prev = area.withTrimmedLeft (14).withTrimmedTop (30);
        previewPanel.setBounds (prev);
        auto inner = prev.reduced (16);
        auto r1 = inner.removeFromTop (40);
        btP1.setBounds (r1.removeFromLeft (110).reduced (3));
        btP2.setBounds (r1.removeFromLeft (110).reduced (3));
        tgP.setBounds  (r1.reduced (3));
        inner.removeFromTop (12);
        // Barres (légende à gauche, barre à droite).
        auto layBar = [&] (juce::Slider& s, juce::Label& c)
        {
            auto row = inner.removeFromTop (28);
            c.setBounds (row.removeFromLeft (70));
            s.setBounds (row.reduced (2));
            inner.removeFromTop (8);
        };
        layBar (slP1, capSl1);
        layBar (slP2, capSl2);
        inner.removeFromTop (8);
        // Potards : normal, normal, pan bipolaire — légende sous chacun.
        auto r4 = inner.removeFromTop (96);
        juce::Slider* kn[] = { &knP1, &knP2, &knPan };
        juce::Label*  kc[] = { &capKn1, &capKn2, &capPan };
        const int kw = juce::jmin (90, r4.getWidth() / 3);
        for (int i = 0; i < 3; ++i)
        {
            auto cell = r4.removeFromLeft (kw);
            kc[i]->setBounds (cell.removeFromBottom (16));
            kn[i]->setBounds (cell);
        }
    }

    // Appelé après une sauvegarde réussie (ex. ConfigPage -> reloadThemes pour lister le nouveau).
    std::function<void()> onThemeSaved;

private:
    static constexpr int controlsW = 360;

    // Pastille de couleur : peint DIRECTEMENT *target (indépendant du style de bouton du thème,
    // qui rendait la couleur invisible en « outline »/« flat »). Clic -> ouvre le picker.
    struct Swatch : public juce::Component
    {
        juce::Colour* target = nullptr;
        std::function<void()> onClick;
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (target != nullptr ? *target : juce::Colours::grey);
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.drawRoundedRectangle (r, 4.0f, 1.0f);
        }
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    };

    struct ColourRow { std::unique_ptr<juce::Label> label; std::unique_ptr<Swatch> swatch; juce::Colour* target; };

    void addStyleCombo (juce::ComboBox& cb, const juce::String& name,
                        juce::StringArray opts, const juce::String& current)
    {
        auto* lb = (&cb == &cbButtons) ? &lbButtons : (&cb == &cbPanels) ? &lbPanels
                 : (&cb == &cbSliders) ? &lbSliders : &lbKnobs;
        addAndMakeVisible (*lb);
        lb->setText (name, juce::dontSendNotification);
        addAndMakeVisible (cb);
        for (int i = 0; i < opts.size(); ++i) cb.addItem (opts[i], i + 1);
        const int sel = opts.indexOf (current.toLowerCase());
        cb.setSelectedItemIndex (sel >= 0 ? sel : 0, juce::dontSendNotification);
    }

    void addColour (const juce::String& name, juce::Colour* target)
    {
        ColourRow row;
        row.label = std::make_unique<juce::Label> (juce::String(), name);
        row.swatch = std::make_unique<Swatch> ();
        row.target = target;
        row.swatch->target = target;
        addAndMakeVisible (*row.label);
        addAndMakeVisible (*row.swatch);
        auto* sw = row.swatch.get();
        sw->onClick = [this, target, sw] { openColourPicker (target, sw); };
        colours.push_back (std::move (row));
    }

    void openColourPicker (juce::Colour* target, juce::Component* anchor)
    {
        activeTarget = target;
        auto sel = std::make_unique<juce::ColourSelector> (
            juce::ColourSelector::showColourAtTop | juce::ColourSelector::showSliders
          | juce::ColourSelector::showColourspace);
        sel->setCurrentColour (*target);
        sel->setSize (280, 300);
        sel->addChangeListener (this);
        juce::CallOutBox::launchAsynchronously (std::move (sel), anchor->getScreenBounds(), nullptr);
    }

    void changeListenerCallback (juce::ChangeBroadcaster* src) override
    {
        if (auto* sel = dynamic_cast<juce::ColourSelector*> (src))
            if (activeTarget != nullptr) { *activeTarget = sel->getCurrentColour(); apply(); }
    }

    void setupPreview()
    {
        addAndMakeVisible (previewPanel);
        previewPanel.setText ("Panneau");
        addAndMakeVisible (btP1); btP1.setButtonText ("Bouton");
        addAndMakeVisible (btP2); btP2.setButtonText ("Actif"); btP2.setClickingTogglesState (true); btP2.setToggleState (true, juce::dontSendNotification);
        addAndMakeVisible (tgP); tgP.setButtonText ("Switch"); tgP.setToggleState (true, juce::dontSendNotification);

        auto bar = [&] (juce::Slider& s, double mn, double mx, double v, bool bip = false)
        { addAndMakeVisible (s); s.setSliderStyle (juce::Slider::LinearBar); s.setRange (mn, mx, 1); s.setValue (v, juce::dontSendNotification);
          if (bip) s.getProperties().set ("bipolar", true); };
        bar (slP1, 0, 127, 88);
        bar (slP2,  0,  63, 48, true);           // barre bipolaire (pan) : centre 32, calque pan EG

        auto knob = [&] (juce::Slider& s, double mn, double mx, double v, bool bip = false)
        { addAndMakeVisible (s); s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); s.setRange (mn, mx, 1); s.setValue (v, juce::dontSendNotification);
          s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0); if (bip) s.getProperties().set ("bipolar", true); };
        knob (knP1,  0, 127, 80);
        knob (knP2,  0, 127, 30);
        knob (knPan, 0,  63, 48, true);          // potard bipolaire (pan) : arc/LED partent du centre

        // Légendes : on identifie chaque surface de l'aperçu (sinon « pan » indiscernable d'un potard).
        auto cap = [&] (juce::Label& l, const juce::String& t, juce::Justification j)
        { addAndMakeVisible (l); l.setText (t, juce::dontSendNotification); l.setJustificationType (j);
          l.setFont (juce::Font (juce::FontOptions (11.0f))); l.setColour (juce::Label::textColourId, SYPal.textMuted); };
        cap (capSl1, "Barre",            juce::Justification::centredLeft);
        cap (capSl2, "Barre pan",        juce::Justification::centredLeft);
        cap (capKn1, "Potard",           juce::Justification::centred);
        cap (capKn2, "Potard",           juce::Justification::centred);
        cap (capPan, "Pan",              juce::Justification::centred);
    }

    void apply()
    {
        work.glow = work.accent.withAlpha (work.dark ? 0.5f : 0.33f);   // halo suit l'accent
        for (auto& c : colours)
            c.swatch->repaint();                 // la pastille relit *target
        for (auto* l : { &capSl1, &capSl2, &capKn1, &capKn2, &capPan })
            l->setColour (juce::Label::textColourId, work.textMuted);   // légendes suivent le thème
        applySyPalette (work);
        repaintEverything();
    }

    void save()
    {
        work.name = edName.getText().trim();
        if (work.name.isEmpty()) work.name = "Mon theme";
        auto dir = syThemesDir();
        dir.createDirectory();
        auto sub = dir.getChildFile (work.name.toLowerCase().replaceCharacter (' ', '-'));
        sub.createDirectory();
        sub.getChildFile ("theme.xml").replaceWithText (syPaletteToXml (work));
        applySyPalette (work);
        repaintEverything();
        labStatus.setText ("Sauve -> " + sub.getFileName(), juce::dontSendNotification);
        if (onThemeSaved) onThemeSaved();   // ex. rafraichit le selecteur de la page Setting
    }

    static void repaintEverything()
    {
        auto& d = juce::Desktop::getInstance();
        for (int i = 0; i < d.getNumComponents(); ++i)
            if (auto* c = d.getComponent (i)) c->repaint();
    }

    SyPalette work;
    juce::Colour* activeTarget = nullptr;

    juce::Label lbButtons, lbPanels, lbSliders, lbKnobs;
    juce::ComboBox cbButtons, cbPanels, cbSliders, cbKnobs;
    juce::ToggleButton tgDark;
    std::vector<ColourRow> colours;

    juce::Label labName, labStatus;
    juce::TextEditor edName;
    juce::TextButton btSave, btClose;

    // Aperçu live
    juce::GroupComponent previewPanel;
    juce::TextButton btP1, btP2;
    juce::ToggleButton tgP;
    juce::Slider slP1, slP2, knP1, knP2, knPan;
    juce::Label  capSl1, capSl2, capKn1, capKn2, capPan;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThemeBuilder)
};
