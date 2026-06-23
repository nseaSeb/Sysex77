/*
  ==============================================================================

    Effects.h
    Éditeur de la section effets du SY77/TG77 — câblé sur les ADRESSES RÉELLES
    (Parameter Change groupe 0x08, Table 1-9 ; recoupé avec la map Electra One
    TG77 vérifiée). Structure : Effect Mode + 2 unités Chorus + 2 unités Reverb,
    chacune Type / Balance / Output Level + paramètres génériques, plus 2 Stereo Mix.

    Les widgets sont des MidiSlider / MidiButton : ils persistent dans valueTreeVoice
    ET envoient le sysex au synthé (et se resynchronisent sur le sysex entrant), comme
    les autres éditeurs. T2 = 0x00 (les effets sont globaux à la voix).

    NOTE : la spec ne nomme pas les types d'effet (chorus 0-4, reverb 0-40) ni le sens
    des paramètres « Prm » (ils dépendent du type choisi) -> affichage numérique, pas
    d'invention de noms (cf. [[feedback-hardware-safety]]). Adresses + plages, elles,
    sont vérifiées.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Values.h"
#include "LookAndFeel.h"
#include "MidiObjects.h"

class EffectsPage : public juce::Component
{
public:
    EffectsPage()
    {
        setupParam (sliderMode, IDs::EFFECTMODE, 0x00, 3, "Effect Mode");

        // Chorus 1 (N2 0x01-0x07)
        setupParam (chr1Type, IDs::CHR1TYPE, 0x01,   4, "Type");
        setupParam (chr1Bal,  IDs::CHR1BAL,  0x02, 100, "Balance");
        setupParam (chr1Lvl,  IDs::CHR1LVL,  0x03, 100, "Out Lvl");
        setupParam (chr1P1,   IDs::CHR1PRM1, 0x04, 127, "Prm 1");
        setupParam (chr1P2,   IDs::CHR1PRM2, 0x05, 127, "Prm 2");
        setupParam (chr1P3,   IDs::CHR1PRM3, 0x06, 127, "Prm 3");
        setupParam (chr1P4,   IDs::CHR1PRM4, 0x07, 127, "Prm 4");

        // Chorus 2 (N2 0x08-0x0E)
        setupParam (chr2Type, IDs::CHR2TYPE, 0x08,   4, "Type");
        setupParam (chr2Bal,  IDs::CHR2BAL,  0x09, 100, "Balance");
        setupParam (chr2Lvl,  IDs::CHR2LVL,  0x0A, 100, "Out Lvl");
        setupParam (chr2P1,   IDs::CHR2PRM1, 0x0B, 127, "Prm 1");
        setupParam (chr2P2,   IDs::CHR2PRM2, 0x0C, 127, "Prm 2");
        setupParam (chr2P3,   IDs::CHR2PRM3, 0x0D, 127, "Prm 3");
        setupParam (chr2P4,   IDs::CHR2PRM4, 0x0E, 127, "Prm 4");

        // Reverb 1 (N2 0x0F-0x14)
        setupParam (rev1Type, IDs::REV1TYPE, 0x0F,  40, "Type");
        setupParam (rev1Bal,  IDs::REV1BAL,  0x10, 100, "Balance");
        setupParam (rev1Lvl,  IDs::REV1LVL,  0x11, 100, "Out Lvl");
        setupParam (rev1P1,   IDs::REV1PRM1, 0x12, 127, "Prm 1");
        setupParam (rev1P2,   IDs::REV1PRM2, 0x13, 127, "Prm 2");
        setupParam (rev1P3,   IDs::REV1PRM3, 0x14, 127, "Prm 3");

        // Reverb 2 (N2 0x15-0x1A)
        setupParam (rev2Type, IDs::REV2TYPE, 0x15,  40, "Type");
        setupParam (rev2Bal,  IDs::REV2BAL,  0x16, 100, "Balance");
        setupParam (rev2Lvl,  IDs::REV2LVL,  0x17, 100, "Out Lvl");
        setupParam (rev2P1,   IDs::REV2PRM1, 0x18, 127, "Prm 1");
        setupParam (rev2P2,   IDs::REV2PRM2, 0x19, 127, "Prm 2");
        setupParam (rev2P3,   IDs::REV2PRM3, 0x1A, 127, "Prm 3");

        // Stereo Mix 1 / 2 (N2 0x1B / 0x1C, on/off)
        setupToggle (btMix1, IDs::STMIX1, 0x1B, "St Mix 1");
        setupToggle (btMix2, IDs::STMIX2, 0x1C, "St Mix 2");

        grpChr1.setText ("Chorus 1");  addAndMakeVisible (grpChr1);
        grpChr2.setText ("Chorus 2");  addAndMakeVisible (grpChr2);
        grpRev1.setText ("Reverb 1");  addAndMakeVisible (grpRev1);
        grpRev2.setText ("Reverb 2");  addAndMakeVisible (grpRev2);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (12);

        auto top = r.removeFromTop (50);
        top.removeFromTop (14); // place pour le libellé attaché du Mode
        sliderMode.setBounds (top.removeFromLeft (150).reduced (4, 0));
        btMix1.setBounds (top.removeFromLeft (130).reduced (8, 4));
        btMix2.setBounds (top.removeFromLeft (130).reduced (8, 4));

        r.removeFromTop (6);
        const int rowH = r.getHeight() / 2;
        auto row1 = r.removeFromTop (rowH);
        auto row2 = r;
        const int colW = row1.getWidth() / 2;

        grpChr1.setBounds (row1.removeFromLeft (colW).reduced (4));
        grpChr2.setBounds (row1.reduced (4));
        grpRev1.setBounds (row2.removeFromLeft (colW).reduced (4));
        grpRev2.setBounds (row2.reduced (4));

        layoutKnobs (grpChr1, { &chr1Type, &chr1Bal, &chr1Lvl, &chr1P1, &chr1P2, &chr1P3, &chr1P4 });
        layoutKnobs (grpChr2, { &chr2Type, &chr2Bal, &chr2Lvl, &chr2P1, &chr2P2, &chr2P3, &chr2P4 });
        layoutKnobs (grpRev1, { &rev1Type, &rev1Bal, &rev1Lvl, &rev1P1, &rev1P2, &rev1P3 });
        layoutKnobs (grpRev2, { &rev2Type, &rev2Bal, &rev2Lvl, &rev2P1, &rev2P2, &rev2P3 });
    }

private:
    void setupParam (MidiSlider& s, const juce::Identifier& id, int n2, int maxVal, const juce::String& name)
    {
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 46, 16);
        s.setColour (juce::Slider::thumbColourId, SYColSelected);
        s.setRangeAndRound (0, maxVal, 0);
        s.getValueObject().referTo (valueTreeVoice.getPropertyAsValue (id, nullptr));
        int sx[9] = { 0x43, 0x10, 0x34, 0x08, 0x00, 0x00, n2, 0x00, 0x00 };
        s.setMidiSysex (sx);

        auto* lab = labels.add (new juce::Label());
        addAndMakeVisible (*lab);
        lab->setText (name, juce::dontSendNotification);
        lab->setJustificationType (juce::Justification::centred);
        lab->setColour (juce::Label::textColourId, SYColLabel);
        lab->setFont (juce::FontOptions (12.0f));
        lab->attachToComponent (&s, false);
    }

    void setupToggle (MidiButton& b, const juce::Identifier& id, int n2, const juce::String& name)
    {
        addAndMakeVisible (b);
        b.setButtonText (name);
        b.setClickingTogglesState (true);
        b.getToggleStateValue().referTo (valueTreeVoice.getPropertyAsValue (id, nullptr));
        int sx[9] = { 0x43, 0x10, 0x34, 0x08, 0x00, 0x00, n2, 0x00, 0x00 };
        b.setMidiSysex (sx);
    }

    void layoutKnobs (juce::GroupComponent& grp, std::initializer_list<juce::Component*> knobs)
    {
        auto b = grp.getBounds().reduced (10, 24);
        b.removeFromTop (14); // place pour les libellés au-dessus des knobs
        const int n = (int) knobs.size();
        if (n == 0) return;
        const int kw = b.getWidth() / n;
        int i = 0;
        for (auto* k : knobs)
            k->setBounds (b.getX() + i++ * kw, b.getY(), kw, b.getHeight());
    }

    MidiSlider sliderMode;

    juce::GroupComponent grpChr1, grpChr2, grpRev1, grpRev2;
    MidiSlider chr1Type, chr1Bal, chr1Lvl, chr1P1, chr1P2, chr1P3, chr1P4;
    MidiSlider chr2Type, chr2Bal, chr2Lvl, chr2P1, chr2P2, chr2P3, chr2P4;
    MidiSlider rev1Type, rev1Bal, rev1Lvl, rev1P1, rev1P2, rev1P3;
    MidiSlider rev2Type, rev2Bal, rev2Lvl, rev2P1, rev2P2, rev2P3;
    MidiButton btMix1, btMix2;

    juce::OwnedArray<juce::Label> labels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsPage)
};
