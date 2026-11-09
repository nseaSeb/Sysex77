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
#include <array>
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
        addStyleCombo (cbPanels,  "Panneaux", { "square","round","flat","studio","fun" },      work.panelStyle);
        addStyleCombo (cbSliders, "Sliders",  { "bar","led","rail","groove","notch","dots","field" },  work.sliderStyle);
        addStyleCombo (cbKnobs,   "Potards",  { "arc","dot","vintage","tick","neon","arctick","thin" }, work.knobStyle);

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

        // Couleurs d'operateurs : jeu predefini / monochrome (= accent) / personnalise.
        addAndMakeVisible (lbOps);
        lbOps.setText ("Operateurs", juce::dontSendNotification);
        addAndMakeVisible (cbOps);
        cbOps.addItem ("Monochrome", 1);
        for (int k = 0; k < kNumOpSets; ++k) cbOps.addItem (opSets()[k].name, k + 2);
        cbOps.addItem ("Personnalise", kCustomId);
        cbOps.setSelectedId (detectOpSet(), juce::dontSendNotification);
        cbOps.onChange = [this] { opSetChosen(); };
        for (int i = 0; i < 6; ++i)
        {
            auto& r = opRows[(size_t) i];
            r.label = std::make_unique<juce::Label> (juce::String(), "OP" + juce::String (i + 1));
            r.swatch = std::make_unique<Swatch>();
            r.target = &work.opColour[i];
            r.swatch->target = r.target;
            addAndMakeVisible (*r.label);
            addAndMakeVisible (*r.swatch);
            auto* sw = r.swatch.get();
            auto* tg = r.target;
            sw->onClick = [this, tg, sw] { openColourPicker (tg, sw); };
        }

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
        setSize (760, 640);
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
        left.removeFromTop (8);

        // Operateurs : combo + 6 pastilles (3 par ligne).
        comboRow (lbOps, cbOps);
        {
            auto og = left.removeFromTop (2 * (rowH + 2));
            const int cw = (og.getWidth() - 16) / 3;
            for (int i = 0; i < 6; ++i)
            {
                auto& r = opRows[(size_t) i];
                juce::Rectangle<int> cell (og.getX() + (i % 3) * (cw + 8), og.getY() + (i / 3) * (rowH + 2), cw, rowH);
                r.label->setBounds (cell.removeFromLeft (cw - 40));
                r.swatch->setBounds (cell);
            }
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
        inner.removeFromTop (14);
        capOps.setBounds (inner.removeFromTop (16));
        opTiles.setBounds (inner.removeFromTop (40).withWidth (juce::jmin (inner.getWidth(), 6 * 40)));
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
            g.setColour ((target != nullptr ? *target : juce::Colours::grey).withMultipliedAlpha (isEnabled() ? 1.0f : 0.25f));
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.drawRoundedRectangle (r, 4.0f, 1.0f);
        }
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    };

    struct ColourRow { std::unique_ptr<juce::Label> label; std::unique_ptr<Swatch> swatch; juce::Colour* target; };

    // Apercu : 6 tuiles d'operateur, meme recette que AlgoDraw (tuile pleine couleur de l'op si
    // le theme a des couleurs d'op, sinon tuile neutre cerclee d'accent).
    struct OpTiles : public juce::Component
    {
        void paint (juce::Graphics& g) override
        {
            const float s = juce::jmin ((float) getHeight(), getWidth() / 6.0f) - 6.0f;
            g.setFont (juce::Font (juce::FontOptions (s * 0.48f)).boldened());
            for (int o = 0; o < 6; ++o)
            {
                juce::Rectangle<float> b (o * (s + 6.0f) + 3.0f, (getHeight() - s) * 0.5f, s, s);
                if (SYPal.hasOpColours)
                {
                    const auto oc = syOpColour (o);
                    g.setColour (oc);
                    g.fillRoundedRectangle (b, 3.0f);
                    g.setColour (oc.getPerceivedBrightness() > 0.62f ? juce::Colour (0xff141416) : juce::Colours::white);
                }
                else
                {
                    g.setColour (SYPal.surfaceAlt);
                    g.fillRoundedRectangle (b, 3.0f);
                    g.setColour (SYPal.accent);
                    g.drawRoundedRectangle (b.reduced (0.5f), 3.0f, 1.4f);
                }
                g.drawText (juce::String (o + 1), b, juce::Justification::centred);
            }
        }
    };

    // Jeux de couleurs d'operateurs predefinis (noms generiques, op1 -> op6).
    struct OpSet { const char* name; juce::uint32 c[6]; };
    static constexpr int kNumOpSets = 4, kCustomId = 99;
    static const OpSet* opSets()
    {
        static const OpSet sets[kNumOpSets] = {
            { "Spectre", { 0xffe8333f, 0xffd6337a, 0xffb03ad0, 0xff5a3ae0, 0xff2e7be0, 0xff2fd6e0 } },
            { "Pastel",  { 0xfff4a6a6, 0xfff4c59a, 0xffeee39b, 0xffa8e0b0, 0xff9fc8f0, 0xffc7aef0 } },
            { "Neon",    { 0xffff2d95, 0xffff9f1c, 0xffe6ff2e, 0xff39ff88, 0xff2ee6ff, 0xffa64dff } },
            { "Chaud",   { 0xffffc94d, 0xffffa632, 0xffff7a2b, 0xfff0532e, 0xffd23a3a, 0xffa82848 } },
        };
        return sets;
    }

    int detectOpSet() const
    {
        if (! work.hasOpColours) return 1;
        for (int k = 0; k < kNumOpSets; ++k)
        {
            bool same = true;
            for (int i = 0; i < 6; ++i) same = same && work.opColour[i].getARGB() == opSets()[k].c[i];
            if (same) return k + 2;
        }
        return kCustomId;
    }

    void opSetChosen()
    {
        const int id = cbOps.getSelectedId();
        work.hasOpColours = id != 1;
        if (id >= 2 && id < 2 + kNumOpSets)
            for (int i = 0; i < 6; ++i) work.opColour[i] = juce::Colour (opSets()[id - 2].c[i]);
        apply();
    }

    bool isOpTarget (const juce::Colour* t) const { return t >= &work.opColour[0] && t <= &work.opColour[5]; }

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
            if (activeTarget != nullptr)
            {
                *activeTarget = sel->getCurrentColour();
                if (isOpTarget (activeTarget))   // pastille OP retouchee -> jeu « Personnalise »
                    cbOps.setSelectedId (detectOpSet(), juce::dontSendNotification);
                apply();
            }
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
          s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);   // valeur deja ecrite par le LnF (sinon 2x)
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
        cap (capOps, "Operateurs",       juce::Justification::centredLeft);
        addAndMakeVisible (opTiles);
    }

    void apply()
    {
        work.glow = work.accent.withAlpha (work.dark ? 0.5f : 0.33f);   // halo suit l'accent
        for (auto& c : colours)
            c.swatch->repaint();                 // la pastille relit *target
        for (auto& r : opRows)
        {
            r.swatch->setEnabled (work.hasOpColours);   // Monochrome : pastilles grisees
            r.label->setEnabled (work.hasOpColours);
            r.swatch->repaint();
        }
        for (auto* l : { &capSl1, &capSl2, &capKn1, &capKn2, &capPan, &capOps })
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
    juce::Label lbOps;
    juce::ComboBox cbOps;
    std::array<ColourRow, 6> opRows;

    juce::Label labName, labStatus;
    juce::TextEditor edName;
    juce::TextButton btSave, btClose;

    // Aperçu live
    juce::GroupComponent previewPanel;
    juce::TextButton btP1, btP2;
    juce::ToggleButton tgP;
    juce::Slider slP1, slP2, knP1, knP2, knPan;
    juce::Label  capSl1, capSl2, capKn1, capKn2, capPan, capOps;
    OpTiles      opTiles;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThemeBuilder)
};
