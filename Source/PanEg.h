/*
  ==============================================================================

    PanEg.h
    Éditeur COMPLET du pan d'un élément (vue EG) : sélection de la table de pan
    (PANNM), nom de la table (PNNAM), et enveloppe de pan — tout au même endroit
    (choix utilisateur : « édition du pan entièrement dans la vue EG »).

    Modèle SY77 : un élément stocke un INDEX de table (PANNM, group 0x03/0x07) ;
    le CONTENU de la table (source + profondeur + EG + slope + nom) vit dans le
    group 0x0A « Pan Data », adressé par T2 = numéro de table. Calqué sur Filter1
    pour l'EG (7 niveaux L0-L4/RL1-2 + 6 rates R1-4/RR1-2), mais group 0x0A,
    niveaux o/b OFFSET 32 (centre octet 32, plage -32..+31), addrHi = T2 = table.

    🟡 FIABILITÉ : adresses lues au spec (docs/sy77midi_ocr.txt l.290, l.543-584),
    NON vérifiées hardware. Envoi actif (choix « tout, envoi compris »).

  ==============================================================================
*/

#pragma once

#include "SysexUtils.h"
#include "EnvelopeDraw.h"

class PanEg    : public ElementComponent, private Slider::Listener
{
public:
    PanEg()
    {
        for (auto* s : { &sliderL0, &sliderL1, &sliderL2, &sliderL3, &sliderL4, &sliderRL1, &sliderRL2,
                         &sliderR1, &sliderR2, &sliderR3, &sliderR4, &sliderRR1, &sliderRR2,
                         &sliderHold, &sliderDepth, &sliderSlope })
        {
            addAndMakeVisible (*s);
            s->addListener (this);
        }
        // Hold / Depth / Repeat : HORIZONTAUX (le défaut MidiSlider est vertical -> illisible
        // dans une boîte large et basse). Les niveaux/rates L*/R* restent verticaux (hauts).
        for (auto* s : { &sliderHold, &sliderDepth, &sliderSlope })
            s->setSliderStyle (Slider::SliderStyle::LinearHorizontal);

        for (auto* l : { &labelL0, &labelL1, &labelL2, &labelL3, &labelL4, &labelRL1, &labelRL2,
                         &labelR1, &labelR2, &labelR3, &labelR4, &labelRR1, &labelRR2,
                         &labelHold, &labelDepth, &labelSrc, &labelSlope, &labelTableSel, &labelName })
            addAndMakeVisible (*l);
        labelL0.attachToComponent (&sliderL0, false);   labelL1.attachToComponent (&sliderL1, false);
        labelL2.attachToComponent (&sliderL2, false);   labelL3.attachToComponent (&sliderL3, false);
        labelL4.attachToComponent (&sliderL4, false);   labelRL1.attachToComponent (&sliderRL1, false);
        labelRL2.attachToComponent (&sliderRL2, false); labelR1.attachToComponent (&sliderR1, false);
        labelR2.attachToComponent (&sliderR2, false);   labelR3.attachToComponent (&sliderR3, false);
        labelR4.attachToComponent (&sliderR4, false);   labelRR1.attachToComponent (&sliderRR1, false);
        labelRR2.attachToComponent (&sliderRR2, false);
        labelHold.attachToComponent (&sliderHold, true);
        labelDepth.attachToComponent (&sliderDepth, true);
        labelSlope.attachToComponent (&sliderSlope, true);
        labelSrc.attachToComponent (&comboSrc, true);
        labelTableSel.attachToComponent (&btTable, true);
        labelName.attachToComponent (&nameEditor, true);

        addAndMakeVisible (comboSrc);
        comboSrc.addItem ("Velocity", 1);
        comboSrc.addItem ("Note #",  2);
        comboSrc.addItem ("LFO",     3);

        // Barre du haut : sélecteur de table (PANNM) + nom de la table (PNNAM).
        addAndMakeVisible (btTable);
        btTable.onClick = [this] { showTableMenu(); };
        addAndMakeVisible (nameEditor);
        nameEditor.setInputRestrictions (10);
        nameEditor.setJustification (Justification::centredLeft);
        nameEditor.onReturnKey  = [this] { commitName(); };
        nameEditor.onFocusLost  = [this] { commitName(); };

        // Édition à la souris du grand graphe d'EG (niveau Y + rate X), comme Filter1.
        addAndMakeVisible (egGraph);
        egGraph.maxLevel = 63.0f;   // octet filaire o/b 0..63 (centre 32)
        egGraph.getData = [this] (juce::Array<float>& levels, juce::Array<float>& weights)
        {
            levels = { (float) sliderL0.getValue(), (float) sliderL1.getValue(), (float) sliderL2.getValue(),
                       (float) sliderL3.getValue(), (float) sliderL4.getValue(),
                       (float) sliderRL1.getValue(), (float) sliderRL2.getValue() };
            auto rw = [] (double r) { return (float) (64.0 - r); };
            weights = { rw (sliderR1.getValue()), rw (sliderR2.getValue()), rw (sliderR3.getValue()),
                        rw (sliderR4.getValue()), rw (sliderRR1.getValue()), rw (sliderRR2.getValue()) };
        };
        egGraph.onEditNode = [this] (int node, float levelF, float segWeightF)
        {
            MidiSlider* L[7] = { &sliderL0, &sliderL1, &sliderL2, &sliderL3, &sliderL4, &sliderRL1, &sliderRL2 };
            MidiSlider* R[6] = { &sliderR1, &sliderR2, &sliderR3, &sliderR4, &sliderRR1, &sliderRR2 };
            if (node < 0 || node > 6) return;
            L[node]->setValue (jlimit (0, 63, roundToInt (levelF)));
            if (node >= 1 && segWeightF >= 0.0f)
                R[node - 1]->setValue (jlimit (0, 63, roundToInt (64.0f - segWeightF)));
        };
    }

    ~PanEg() override
    {
        for (auto* s : { &sliderL0, &sliderL1, &sliderL2, &sliderL3, &sliderL4, &sliderRL1, &sliderRL2,
                         &sliderR1, &sliderR2, &sliderR3, &sliderR4, &sliderRR1, &sliderRR2,
                         &sliderHold, &sliderDepth, &sliderSlope })
            s->removeListener (this);
    }

    void sliderValueChanged (Slider*) override { repaint(); }

    void setElementNumber (int element, UndoManager& um) override
    {
        storedElement = jlimit (1, 4, element);
        const String e (storedElement);
        auto id = [&e] (const char* s) { return Identifier (String ("ELEMENT") + e + s); };
        auto bind = [&] (MidiSlider& s, const char* suffix)
        { s.getValueObject().referTo (valueTreeVoice.getPropertyAsValue (id (suffix), &um)); };

        bind (sliderL0, "PANL0");  bind (sliderL1, "PANL1");  bind (sliderL2, "PANL2");
        bind (sliderL3, "PANL3");  bind (sliderL4, "PANL4");
        bind (sliderRL1, "PANRL1"); bind (sliderRL2, "PANRL2");
        bind (sliderR1, "PANR1");  bind (sliderR2, "PANR2");  bind (sliderR3, "PANR3");  bind (sliderR4, "PANR4");
        bind (sliderRR1, "PANRR1"); bind (sliderRR2, "PANRR2");
        bind (sliderHold, "PANHOLD");  bind (sliderDepth, "PANDEPTH");  bind (sliderSlope, "PANSLP");
        comboSrc.getSelectedIdAsValue().referTo (valueTreeVoice.getPropertyAsValue (id ("PANSRC"), &um));

        for (auto* s : { &sliderL0, &sliderL1, &sliderL2, &sliderL3, &sliderL4, &sliderRL1, &sliderRL2 })
            SyVoice::applyPanLevelDisplay (*s);

        applyAddresses (currentPanTable());
        loadName();
    }

    // À l'ouverture de l'overlay : relit la table + le nom et réapplique les adresses.
    void refresh()
    {
        applyAddresses (currentPanTable());
        loadName();
        repaint();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        auto egArea = egBounds();

        juce::Array<float> levels { (float) sliderL0.getValue(), (float) sliderL1.getValue(),
                                    (float) sliderL2.getValue(), (float) sliderL3.getValue(),
                                    (float) sliderL4.getValue(), (float) sliderRL1.getValue(),
                                    (float) sliderRL2.getValue() };
        auto rw = [] (double r) { return (float) (64.0 - r); };
        juce::Array<float> weights { rw (sliderR1.getValue()), rw (sliderR2.getValue()),
                                     rw (sliderR3.getValue()), rw (sliderR4.getValue()),
                                     rw (sliderRR1.getValue()), rw (sliderRR2.getValue()) };
        SyDraw::drawEnvelope (g, egArea, levels, weights, 63.0f, SYColSelected);

        // Repère du centre (pan = milieu) : ligne horizontale au niveau o/b 32, + repères L/R.
        g.setColour (SYColLabel.withAlpha (0.4f));
        const float cy = egArea.getBottom() - (32.0f / 63.0f) * egArea.getHeight();
        g.drawHorizontalLine ((int) cy, egArea.getX(), egArea.getRight());
        // Niveau o/b : octet 63 (haut) = +31, octet 0 (bas) = -32. Convention - = gauche,
        // + = droite -> haut = R, bas = L (🟡 sens L/R à confirmer hardware).
        g.setColour (SYColLabel.withAlpha (0.75f));
        g.setFont (11.0f);
        g.drawText ("R (right)", (int) egArea.getX() + 4, (int) egArea.getY() + 3, 60, 13, Justification::left);
        g.drawText ("L (left)",  (int) egArea.getX() + 4, (int) egArea.getBottom() - 16, 60, 13, Justification::left);
        g.drawText ("center",    (int) egArea.getRight() - 56, (int) cy - 14, 52, 12, Justification::right);
    }

    void resized() override
    {
        auto b = getLocalBounds().toFloat();
        egGraph.setBounds (egBounds().toNearestInt());

        // Barre du haut : sélecteur de table + nom (les labels « Table »/« Name » sont attachés à gauche).
        btTable.setBoundsRelative    (0.10f, 0.025f, 0.20f, 0.055f);
        nameEditor.setBoundsRelative (0.42f, 0.025f, 0.20f, 0.055f);

        // Niveaux (rangée haute) + rates (rangée basse), à droite.
        const float lx[7] = { 0.65f, 0.70f, 0.75f, 0.80f, 0.85f, 0.90f, 0.95f };
        MidiSlider* L[7] = { &sliderL0, &sliderL1, &sliderL2, &sliderL3, &sliderL4, &sliderRL1, &sliderRL2 };
        MidiSlider* R[6] = { &sliderR1, &sliderR2, &sliderR3, &sliderR4, &sliderRR1, &sliderRR2 };
        for (int i = 0; i < 7; ++i) L[i]->setBoundsRelative (lx[i], 0.16f, 0.04f, 0.26f);
        for (int i = 0; i < 6; ++i) R[i]->setBoundsRelative (lx[i + 1], 0.50f, 0.04f, 0.26f);

        // Contrôles du bas, AÉRÉS (deux colonnes, labels attachés à gauche ; sliders horizontaux).
        comboSrc.setBoundsRelative    (0.15f, 0.82f, 0.22f, 0.06f);
        sliderDepth.setBoundsRelative (0.15f, 0.91f, 0.22f, 0.05f);
        sliderHold.setBoundsRelative  (0.55f, 0.82f, 0.22f, 0.05f);
        sliderSlope.setBoundsRelative (0.55f, 0.91f, 0.22f, 0.05f);
    }

private:
    juce::Rectangle<float> egBounds() const
    {
        auto b = getLocalBounds().toFloat();
        return { b.getWidth() * 0.03f, b.getHeight() * 0.13f, b.getWidth() * 0.58f, b.getHeight() * 0.62f };
    }

    int currentPanTable() const
    {
        return jlimit (0, 95, (int) valueTreeVoice.getProperty (
            Identifier (String ("ELEMENT") + String (storedElement) + "PANNM"), 0));
    }

    // Liste « PAN JOBS » (façon SynthWorks) : choisit la table de pan (PANNM 0..95).
    // Persiste ELEMENT<n>PANNM, envoie PANNM (group 0x03/0x07), puis recharge l'EG.
    void showTableMenu()
    {
        PopupMenu m;
        const int current = currentPanTable();
        for (int i = 0; i < 96; ++i)
            m.addItem (i + 1, panTableName (i), true, i == current);
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (btTable),
                         [this] (int res)
                         {
                             if (res <= 0) return;
                             const int idx = res - 1;
                             valueTreeVoice.setProperty (Identifier (String ("ELEMENT") + String (storedElement) + "PANNM"), idx, nullptr);
                             sendPanNm (idx);
                             refresh();
                         });
    }

    // PANNM : group 0x03 / param 0x07 / T2 = élément<<5 (l'élément, pas la table).
    void sendPanNm (int idx)
    {
        juce::uint8 b[9] = { 0x43, 0x10, 0x34, 0x03, (juce::uint8) ((storedElement - 1) << 5),
                             0x00, 0x07, 0x00, (juce::uint8) (jlimit (0, 95, idx) & 0x7F) };
        panNmSender.sendParam9 ("/SYSEX", b);
    }

    void loadName()
    {
        nameEditor.setText (valueTreeVoice.getProperty (
            Identifier (String ("ELEMENT") + String (storedElement) + "PANNAME")).toString(),
            dontSendNotification);
    }

    // Nom de table (PNNAM0..9) : 10 ASCII, group 0x0A / param 0x11+i / T2 = table. Complété
    // à droite par des espaces (convention Yamaha), un message par caractère.
    void commitName()
    {
        const String name = nameEditor.getText();
        valueTreeVoice.setProperty (Identifier (String ("ELEMENT") + String (storedElement) + "PANNAME"), name, nullptr);
        const int T2 = currentPanTable() & 0x7F;
        for (int i = 0; i < 10; ++i)
        {
            const juce::uint8 ch = (juce::uint8) (i < name.length() ? (int) (juce::juce_wchar) name[i] : 0x20);
            juce::uint8 b[9] = { 0x43, 0x10, 0x34, 0x0A, (juce::uint8) T2, 0x00,
                                 (juce::uint8) (0x11 + i), 0x00, ch };
            panNameSender.sendParam9 ("/SYSEX", b);
        }
        applyAddresses (currentPanTable());   // rafraîchit le libellé du bouton « Table »
    }

    // (Ré)applique group 0x0A + T2 = table à tous les widgets EG/source. Plages : rates/hold
    // 0..63 (depth 0..99) ; niveaux o/b 0..63 (centre 32) ; slope 0..3 ; source 0..2.
    void applyAddresses (int panTable)
    {
        const int T2 = panTable & 0x7F;   // bits 6,5 ignorés en réception (spec) -> envoi brut
        int sx[9] = { 0x43, 0x10, 0x34, 0x0A, T2, 0x00, 0x00, 0x00, 0x00 };
        auto wire = [&] (MidiSlider& s, int n2, int min, int max, int def)
        { sx[6] = n2; s.setMidiSysex (sx); s.setRangeAndRound (min, max, def); };

        wire (sliderDepth, 0x01, 0, 99, 0);   // PNSCDPT
        wire (sliderHold,  0x02, 0, 63, 0);   // PNDT (hold)
        wire (sliderR1,    0x03, 0, 63, 0);   wire (sliderR2, 0x04, 0, 63, 0);
        wire (sliderR3,    0x05, 0, 63, 0);   wire (sliderR4, 0x06, 0, 63, 0);
        wire (sliderRR1,   0x07, 0, 63, 0);   wire (sliderRR2, 0x08, 0, 63, 0);
        wire (sliderL0,    0x09, 0, 63, 32);  wire (sliderL1, 0x0A, 0, 63, 32);
        wire (sliderL2,    0x0B, 0, 63, 32);  wire (sliderL3, 0x0C, 0, 63, 32);
        wire (sliderL4,    0x0D, 0, 63, 32);
        wire (sliderRL1,   0x0E, 0, 63, 32);  wire (sliderRL2, 0x0F, 0, 63, 32);
        wire (sliderSlope, 0x10, 0,  3, 0);   // PNSLP (repeat segment)

        sx[6] = 0x00;                          // PNSCSEL (source)
        comboSrc.setMidiSysex (sx);

        // Le bouton « Table » affiche le NOM personnalisé (PNNAM) si défini, sinon « Pan NN ».
        const String custom = valueTreeVoice.getProperty (
            Identifier (String ("ELEMENT") + String (storedElement) + "PANNAME")).toString();
        btTable.setButtonText (custom.isNotEmpty() ? custom : panTableName (panTable));
    }

    int storedElement = 1;

    MidiSlider sliderL0, sliderL1, sliderL2, sliderL3, sliderL4, sliderRL1, sliderRL2;
    MidiSlider sliderR1, sliderR2, sliderR3, sliderR4, sliderRR1, sliderRR2;
    MidiSlider sliderHold, sliderDepth, sliderSlope;
    MidiCombo  comboSrc;
    TextButton btTable;            // sélecteur de table de pan (PANNM)
    TextEditor nameEditor;         // nom de la table (PNNAM0..9)
    SysexBusSender panNmSender, panNameSender;

    Label labelL0 {"","L0"}, labelL1 {"","L1"}, labelL2 {"","L2"}, labelL3 {"","L3"}, labelL4 {"","L4"};
    Label labelRL1 {"","RL1"}, labelRL2 {"","RL2"};
    Label labelR1 {"","R1"}, labelR2 {"","R2"}, labelR3 {"","R3"}, labelR4 {"","R4"};
    Label labelRR1 {"","RR1"}, labelRR2 {"","RR2"};
    Label labelHold {"","Hold"}, labelDepth {"","Depth"}, labelSrc {"","Source"}, labelSlope {"","Repeat"};
    Label labelTableSel {"","Table"}, labelName {"","Name"};

    EgGraphView egGraph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanEg)
};
