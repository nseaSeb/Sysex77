/*
  ==============================================================================

    AfmLfo.h
    Created: 2026-06-25

    Panneau LFO de l'élément AFM (Main LFO + Sub LFO), calqué sur l'écran
    "AFM ELEMENT 1" de SynthWorks (colonne de droite, sous l'ALGORITHM).

    Adresses : élément commun AFM, groupe 0x05, addrHi = (élément-1)<<5
    (0x00/0x20/0x40/0x60). Table 1-6 (sy77midi_ocr.txt l.357-371), recoupée
    avec la carte project-sy77-addresses.md :
      Main LFO : Speed 0x0D (0-99), Delay 0x0E (0-99), PMD 0x0F (0-127),
                 AMD 0x10 (0-127), FMD 0x11 (0-127), Wave 0x12 (0-5),
                 InitPhase 0x13 (0-99).
      Sub  LFO : Wave 0x15 (0-3), Speed 0x16 (0-127), Mode 0x17 (delay/decay),
                 Time 0x18 (0-99), PMD 0x19 (0-127).

    Toutes ces adresses sont DOCUMENTÉES (OCR + carte mémoire) -> on câble
    l'envoi sysex. Statut MAP : 🟡 (câblé, non vérifié hardware). Le chargement
    depuis dump (.syx) n'est PAS branché ici (offsets bulk non vérifiés).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "MidiObjects.h"   // MidiSlider / MidiButton / MidiCombo + ElementComponent

//==============================================================================
class AfmLfo : public ElementComponent
{
public:
    AfmLfo()
    {
        // -- Main LFO --
        addAndMakeVisible (mlfoWave);
        for (auto* n : { "Triangle", "Saw Down", "Saw Up", "Square", "Sine", "S/Hold" })
            mlfoWave.addItem (n, mlfoWave.getNumItems() + 1);   // 6 formes (0-5)

        for (MidiSlider* s : { &mlfoSpeed, &mlfoDelay, &mlfoPhase, &mlfoPmd, &mlfoAmd, &mlfoFmd })
            setupBar (*s);

        // -- Sub LFO --
        addAndMakeVisible (slfoWave);
        // Formes du Sub LFO (relevées sur le SY77) : Triangle, Saw Down, Square, Sample&Hold.
        // L'ORDRE = la valeur sysex (id-1) : 0=Triangle, 1=Saw Down, 2=Square, 3=S/Hold.
        for (auto* n : { "Triangle", "Saw Down", "Square", "S/Hold" })
            slfoWave.addItem (n, slfoWave.getNumItems() + 1);   // 4 formes (0-3)

        for (MidiSlider* s : { &slfoSpeed, &slfoTime, &slfoPmd })
            setupBar (*s);

        addAndMakeVisible (slfoMode);
        slfoMode.setTextOnOff ("Decay", "Delay");   // 0 = delay, 1 = decay

        for (Label* l : { &lblMain, &lblSub,
                          &lblMWave, &lblMSpd, &lblMDly, &lblMPhs, &lblMPmd, &lblMAmd, &lblMFmd,
                          &lblSWave, &lblSSpd, &lblSTime, &lblSPmd, &lblSMode })
        {
            addAndMakeVisible (*l);
            l->setJustificationType (Justification::centredLeft);
            setRoleLabelColour (*l, [] { return SYPal.textMuted; });
        }
        setRoleLabelColour (lblMain, [] { return SYPal.accent; });
        setRoleLabelColour (lblSub,  [] { return SYPal.accent; });
        lblMain.setFont (Font (FontOptions (13.0f)).boldened());
        lblSub .setFont (Font (FontOptions (13.0f)).boldened());
    }

    void setupBar (MidiSlider& s)
    {
        addAndMakeVisible (s);
        s.setSliderStyle (Slider::LinearBar);
        s.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
        s.setPopupDisplayEnabled (true, true, this);
    }

    // Câblage : group 0x05, addrHi = élément<<5 (4 éléments), param = N2 de la Table 1-6.
    void setElementNumber (int element, UndoManager& um) override
    {
        const int aH = (element - 1) << 5;   // 0x00 / 0x20 / 0x40 / 0x60

        // Identifiers par élément (affichage/persistance), via Identifier construit.
        auto P = [&] (const String& suffix) -> Value
        { return valueTreeVoice.getPropertyAsValue (Identifier ("ELEMENT" + String (element) + suffix), &um); };

        // Slider : referTo + adresse sysex (group 0x05) + plage.
        auto wireS = [&] (MidiSlider& s, int param, int maxv, const String& suffix)
        {
            int sx[9] = { 0x43, 0X10, 0x34, 0x05, aH, 0x00, param, 0x00, 0x00 };
            s.setMidiSysex (sx);
            s.setRangeAndRound (0, maxv, 0);
            s.getValueObject().referTo (P (suffix));
        };

        // Main LFO
        wireS (mlfoSpeed, 0x0D, 99,  "MLFOSPEED");
        wireS (mlfoDelay, 0x0E, 99,  "MLFODELAY");
        wireS (mlfoPmd,   0x0F, 127, "MLFOPMD");
        wireS (mlfoAmd,   0x10, 127, "MLFOAMD");
        wireS (mlfoFmd,   0x11, 127, "MLFOFMD");
        wireS (mlfoPhase, 0x13, 99,  "MLFOPHASE");

        // Sub LFO
        wireS (slfoSpeed, 0x16, 127, "SLFOSPEED");
        wireS (slfoTime,  0x18, 99,  "SLFOTIME");
        wireS (slfoPmd,   0x19, 127, "SLFOPMD");

        // Combos WAVE : MidiCombo émet (id-1) et reçoit via valueSysexIn.
        // À l'OUVERTURE de la vue/voix, le combo doit AFFICHER sa valeur liée. Le referTo
        // synchronise selectedId <- Value, MAIS si la propriété est VIDE (void) — c'est le cas
        // au démarrage, car voiceBlobToParams OMET le LFO (cf. SysexUtils.h) — selectedId reste
        // 0 -> combo VIDE. Même garde-fou que comboFoot/comboMod (cf. MidiDemo.h) : si pas de
        // sélection valide après le referTo, on pose le wave 0 (id 1) -> jamais vide.
        // (Reliability-first : on ne touche PAS à l'encodage d'envoi, juste l'init d'affichage.)
        {
            int sxM[9] = { 0x43, 0X10, 0x34, 0x05, aH, 0x00, 0x12, 0x00, 0x00 };   // Main wave
            mlfoWave.setMidiSysex (sxM);
            mlfoWave.getSelectedIdAsValue().referTo (P ("MLFOWAVE"));
            if (mlfoWave.getSelectedItemIndex() < 0)
                mlfoWave.setSelectedId (1, dontSendNotification);   // wave 0 par défaut

            int sxS[9] = { 0x43, 0X10, 0x34, 0x05, aH, 0x00, 0x15, 0x00, 0x00 };   // Sub wave
            slfoWave.setMidiSysex (sxS);
            slfoWave.getSelectedIdAsValue().referTo (P ("SLFOWAVE"));
            if (slfoWave.getSelectedItemIndex() < 0)
                slfoWave.setSelectedId (1, dontSendNotification);   // wave 0 par défaut
        }

        // Sub LFO Mode : bouton 2 états (delay=0 / decay=1).
        {
            int sx[9] = { 0x43, 0X10, 0x34, 0x05, aH, 0x00, 0x17, 0x00, 0x00 };
            slfoMode.setMidiSysex (sx);
            slfoMode.getToggleStateValue().referTo (P ("SLFOMODE"));
        }
    }

    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        // Séparateur entre Main et Sub (repère visuel comme l'original).
        g.setColour (SYPal.panelBorder);
        g.drawHorizontalLine (subTop - 6, 0.0f, (float) getWidth());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (6);

        // Le panneau contient un nombre FIXE de lignes :
        //   1 entête Main + 7 champs Main + 1 espaceur + 1 entête Sub + 5 champs Sub
        // soit 14 lignes-texte (kRows) avec un inter-ligne (gap) entre chacune, plus un
        // espaceur supplémentaire (kSpacer) entre Main et Sub.
        // Plutôt que de figer rowH (l'ancien jlimit clampait à 14px et débordait la dernière
        // ligne — le Sub LFO Mode — hors zone), on CALCULE rowH+gap pour que l'ensemble
        // remplisse EXACTEMENT la hauteur disponible : la dernière ligne reste donc toujours
        // visible, quelle que soit la hauteur que nous donne la colonne de droite.
        const int kRows   = 14;
        const int kSpacer = 8;                     // espace dédié sous le bloc Main
        int gap = 3;                               // inter-ligne nominal

        int avail = area.getHeight() - kSpacer;
        int rowH  = (avail - gap * (kRows - 1)) / kRows;
        if (rowH > 24) { rowH = 24; gap = 4; }     // pas de lignes géantes si large
        if (rowH < 13)                              // très court : on resserre l'inter-ligne
        {
            gap  = 1;
            rowH = jmax (11, (avail - gap * (kRows - 1)) / kRows);
        }

        auto row = [&] (Rectangle<int>& a) { return a.removeFromTop (rowH); };

        // ---- Main LFO ----
        lblMain.setBounds (row (area));
        area.removeFromTop (gap);
        layoutField (area, rowH, gap, lblMWave, mlfoWave);
        layoutField (area, rowH, gap, lblMSpd,  mlfoSpeed);
        layoutField (area, rowH, gap, lblMDly,  mlfoDelay);
        layoutField (area, rowH, gap, lblMPhs,  mlfoPhase);
        layoutField (area, rowH, gap, lblMPmd,  mlfoPmd);
        layoutField (area, rowH, gap, lblMAmd,  mlfoAmd);
        layoutField (area, rowH, gap, lblMFmd,  mlfoFmd);

        area.removeFromTop (kSpacer);
        subTop = area.getY();

        // ---- Sub LFO ----
        lblSub.setBounds (row (area));
        area.removeFromTop (gap);
        layoutField (area, rowH, gap, lblSWave, slfoWave);
        layoutField (area, rowH, gap, lblSSpd,  slfoSpeed);
        layoutField (area, rowH, gap, lblSTime, slfoTime);
        layoutField (area, rowH, gap, lblSPmd,  slfoPmd);
        layoutField (area, rowH, gap, lblSMode, slfoMode);
    }

private:
    // Une ligne « label (gauche) + contrôle (droite) », suivie d'un inter-ligne (gap).
    void layoutField (Rectangle<int>& area, int rowH, int gap, Label& lab, Component& ctl)
    {
        auto r = area.removeFromTop (rowH);
        area.removeFromTop (gap);
        const int labW = jmin (84, r.getWidth() * 42 / 100);
        lab.setBounds (r.removeFromLeft (labW));
        ctl.setBounds (r.reduced (2, 1));
    }

    Label lblMain { "", "MAIN LFO" }, lblSub { "", "SUB LFO" };

    // Main LFO
    MidiCombo  mlfoWave;
    MidiSlider mlfoSpeed, mlfoDelay, mlfoPhase, mlfoPmd, mlfoAmd, mlfoFmd;
    Label lblMWave { "", "Wave" }, lblMSpd { "", "Speed" }, lblMDly { "", "Delay" },
          lblMPhs  { "", "Phase" }, lblMPmd { "", "PMD" }, lblMAmd { "", "AMD" }, lblMFmd { "", "FMD" };

    // Sub LFO
    MidiCombo  slfoWave;
    MidiSlider slfoSpeed, slfoTime, slfoPmd;
    MidiButton slfoMode;
    Label lblSWave { "", "Wave" }, lblSSpd { "", "Speed" }, lblSTime { "", "Time" },
          lblSPmd  { "", "PMD" }, lblSMode { "", "Mode" };

    int subTop = 0;   // y du bloc Sub (séparateur dessiné dans paint)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AfmLfo)
};
