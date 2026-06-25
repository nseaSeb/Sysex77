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
        for (auto* n : { "Saw Down", "Saw Up", "Triangle", "Square" })
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
            l->setColour (Label::textColourId, SYPal.textMuted);
        }
        lblMain.setColour (Label::textColourId, SYPal.accent);
        lblSub .setColour (Label::textColourId, SYPal.accent);
        lblMain.setFont (Font (FontOptions (13.0f)).boldened());
        lblSub .setFont (Font (FontOptions (13.0f)).boldened());
    }

    void setupBar (MidiSlider& s)
    {
        addAndMakeVisible (s);
        s.setSliderStyle (Slider::LinearBar);
        s.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
        s.setPopupDisplayEnabled (true, true, this);
        s.setColour (Slider::trackColourId, SYColSelected);
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
        {
            int sxM[9] = { 0x43, 0X10, 0x34, 0x05, aH, 0x00, 0x12, 0x00, 0x00 };   // Main wave
            mlfoWave.setMidiSysex (sxM);
            mlfoWave.getSelectedIdAsValue().referTo (P ("MLFOWAVE"));

            int sxS[9] = { 0x43, 0X10, 0x34, 0x05, aH, 0x00, 0x15, 0x00, 0x00 };   // Sub wave
            slfoWave.setMidiSysex (sxS);
            slfoWave.getSelectedIdAsValue().referTo (P ("SLFOWAVE"));
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
        const int rowH = jlimit (16, 24, area.getHeight() / 14);
        auto row = [&] (Rectangle<int>& a) { return a.removeFromTop (rowH); };

        // ---- Main LFO ----
        lblMain.setBounds (row (area));
        layoutField (area, rowH, lblMWave, mlfoWave);
        layoutField (area, rowH, lblMSpd,  mlfoSpeed);
        layoutField (area, rowH, lblMDly,  mlfoDelay);
        layoutField (area, rowH, lblMPhs,  mlfoPhase);
        layoutField (area, rowH, lblMPmd,  mlfoPmd);
        layoutField (area, rowH, lblMAmd,  mlfoAmd);
        layoutField (area, rowH, lblMFmd,  mlfoFmd);

        area.removeFromTop (10);
        subTop = area.getY();

        // ---- Sub LFO ----
        lblSub.setBounds (row (area));
        layoutField (area, rowH, lblSWave, slfoWave);
        layoutField (area, rowH, lblSSpd,  slfoSpeed);
        layoutField (area, rowH, lblSTime, slfoTime);
        layoutField (area, rowH, lblSPmd,  slfoPmd);
        layoutField (area, rowH, lblSMode, slfoMode);
    }

private:
    // Une ligne « label (gauche) + contrôle (droite) ».
    void layoutField (Rectangle<int>& area, int rowH, Label& lab, Component& ctl)
    {
        auto r = area.removeFromTop (rowH);
        area.removeFromTop (3);
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
