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

        addStyleCombo (cbButtons, "Boutons",  { "square","round","flat","fun" },  work.buttonStyle);
        addStyleCombo (cbPanels,  "Panneaux", { "square","round","flat","fun" },  work.panelStyle);
        addStyleCombo (cbSliders, "Sliders",  { "bar","led","rail","groove" },    work.sliderStyle);
        addStyleCombo (cbKnobs,   "Potards",  { "arc","dot","vintage" },          work.knobStyle);

        cbButtons.onChange = [this] { work.buttonStyle = cbButtons.getText(); apply(); };
        cbPanels .onChange = [this] { work.panelStyle  = cbPanels .getText(); apply(); };
        cbSliders.onChange = [this] { work.sliderStyle = cbSliders.getText(); apply(); };
        cbKnobs  .onChange = [this] { work.knobStyle   = cbKnobs  .getText(); apply(); };

        addAndMakeVisible (tgDark);
        tgDark.setButtonText ("Thème sombre");
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
        auto r2 = inner.removeFromTop (30);
        slP1.setBounds (r2.reduced (2));
        inner.removeFromTop (8);
        auto r3 = inner.removeFromTop (30);
        slP2.setBounds (r3.reduced (2));
        inner.removeFromTop (16);
        auto r4 = inner.removeFromTop (90);
        knP1.setBounds (r4.removeFromLeft (90));
        knP2.setBounds (r4.removeFromLeft (90));
    }

    // Appelé après une sauvegarde réussie (ex. ConfigPage -> reloadThemes pour lister le nouveau).
    std::function<void()> onThemeSaved;

private:
    static constexpr int controlsW = 360;

    struct ColourRow { std::unique_ptr<juce::Label> label; std::unique_ptr<juce::TextButton> swatch; juce::Colour* target; };

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
        row.swatch = std::make_unique<juce::TextButton> ();
        row.target = target;
        addAndMakeVisible (*row.label);
        addAndMakeVisible (*row.swatch);
        auto* btn = row.swatch.get();
        btn->onClick = [this, target, btn] { openColourPicker (target, btn); };
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
        auto bar = [&] (juce::Slider& s, double mn, double mx, double v)
        { addAndMakeVisible (s); s.setSliderStyle (juce::Slider::LinearBar); s.setRange (mn, mx, 1); s.setValue (v, juce::dontSendNotification); };
        bar (slP1, 0, 127, 88);
        bar (slP2, -15, 15, -6);                 // bipolaire
        auto knob = [&] (juce::Slider& s, double v)
        { addAndMakeVisible (s); s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); s.setRange (0, 127, 1); s.setValue (v, juce::dontSendNotification); s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0); };
        knob (knP1, 80);
        knob (knP2, 30);
    }

    void apply()
    {
        work.glow = work.accent.withAlpha (work.dark ? 0.5f : 0.33f);   // halo suit l'accent
        for (auto& c : colours)
            c.swatch->setColour (juce::TextButton::buttonColourId, *c.target);
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
    juce::Slider slP1, slP2, knP1, knP2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThemeBuilder)
};
