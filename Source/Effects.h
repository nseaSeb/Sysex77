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
#include "SyParam.h"     // dictionnaire déclaratif : adresse (syTranslate) + plage (table)

class EffectsPage : public juce::Component, private juce::Value::Listener
{
public:
    EffectsPage()
    {
        // Effect Mode : radios 0..3 (comme la carte ROUTAGE de Voice) + topologie en clair.
        addAndMakeVisible (radioMode);
        for (auto* t : { "0", "1", "2", "3" }) radioMode.addRadio (t, 7778);
        {
            const auto a = SyVoice::syTranslate (7400, 0, 0);   // group 0x08 / N2 0x00
            int sx[9] = { 0x43, 0x10, 0x34, a.group, a.t2, 0x00, a.n2, 0x00, 0x00 };
            radioMode.setMidiSysex (sx);
        }
        radioMode.getValueObject().referTo (valueTreeVoice.getPropertyAsValue (IDs::EFFECTMODE, nullptr));
        modeValue.referTo (valueTreeVoice.getPropertyAsValue (IDs::EFFECTMODE, nullptr));
        modeValue.addListener (this);
        labelModeTitle.setText ("Effect Mode", juce::dontSendNotification);
        addAndMakeVisible (labelModeTitle);
        labelTopo.setFont (juce::FontOptions (13.0f));
        addAndMakeVisible (labelTopo);

        // Chorus 1 (N2 0x01-0x07)
        setupParam (chr1Type, IDs::CHR1TYPE, 7401, "Type");
        setupParam (chr1Bal,  IDs::CHR1BAL,  7402, "Balance");
        setupParam (chr1Lvl,  IDs::CHR1LVL,  7403, "Out Lvl");
        setupParam (chr1P1,   IDs::CHR1PRM1, 7404, "Prm 1");
        setupParam (chr1P2,   IDs::CHR1PRM2, 7405, "Prm 2");
        setupParam (chr1P3,   IDs::CHR1PRM3, 7406, "Prm 3");
        setupParam (chr1P4,   IDs::CHR1PRM4, 7407, "Prm 4");

        // Chorus 2 (N2 0x08-0x0E)
        setupParam (chr2Type, IDs::CHR2TYPE, 7408, "Type");
        setupParam (chr2Bal,  IDs::CHR2BAL,  7409, "Balance");
        setupParam (chr2Lvl,  IDs::CHR2LVL,  7410, "Out Lvl");
        setupParam (chr2P1,   IDs::CHR2PRM1, 7411, "Prm 1");
        setupParam (chr2P2,   IDs::CHR2PRM2, 7412, "Prm 2");
        setupParam (chr2P3,   IDs::CHR2PRM3, 7413, "Prm 3");
        setupParam (chr2P4,   IDs::CHR2PRM4, 7414, "Prm 4");

        // Reverb 1 (N2 0x0F-0x14)
        setupParam (rev1Type, IDs::REV1TYPE, 7415, "Type");
        setupParam (rev1Bal,  IDs::REV1BAL,  7416, "Balance");
        setupParam (rev1Lvl,  IDs::REV1LVL,  7417, "Out Lvl");
        setupParam (rev1P1,   IDs::REV1PRM1, 7418, "Prm 1");
        setupParam (rev1P2,   IDs::REV1PRM2, 7419, "Prm 2");
        setupParam (rev1P3,   IDs::REV1PRM3, 7420, "Prm 3");

        // Reverb 2 (N2 0x15-0x1A)
        setupParam (rev2Type, IDs::REV2TYPE, 7421, "Type");
        setupParam (rev2Bal,  IDs::REV2BAL,  7422, "Balance");
        setupParam (rev2Lvl,  IDs::REV2LVL,  7423, "Out Lvl");
        setupParam (rev2P1,   IDs::REV2PRM1, 7424, "Prm 1");
        setupParam (rev2P2,   IDs::REV2PRM2, 7425, "Prm 2");
        setupParam (rev2P3,   IDs::REV2PRM3, 7426, "Prm 3");

        // Stereo Mix 1 / 2 (ui 7427 / 7428 -> N2 0x1B / 0x1C, on/off)
        setupToggle (btMix1, IDs::STMIX1, 7427, "St Mix 1");
        setupToggle (btMix2, IDs::STMIX2, 7428, "St Mix 2");

        grpChr1.setText ("Chorus 1");  addAndMakeVisible (grpChr1);
        grpChr2.setText ("Chorus 2");  addAndMakeVisible (grpChr2);
        grpRev1.setText ("Reverb 1");  addAndMakeVisible (grpRev1);
        grpRev2.setText ("Reverb 2");  addAndMakeVisible (grpRev2);
        updateMode();
    }
    ~EffectsPage() override { modeValue.removeListener (this); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (12);

        auto top = r.removeFromTop (50);
        top.removeFromTop (14); // place pour le libellé attaché du Mode
        labelModeTitle.setBounds (top.getX(), top.getY() - 14, 150, 14);
        radioMode.setBounds (top.removeFromLeft (150).reduced (4, 4));
        btMix1.setBounds (top.removeFromLeft (130).reduced (8, 4));
        btMix2.setBounds (top.removeFromLeft (130).reduced (8, 4));
        labelTopo.setBounds (top.reduced (8, 0));

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
    // PILOTE migration dico (Phase 4) : adresse (group/T2/N2) ET plage proviennent désormais du
    // dictionnaire déclaratif via le `ui` (SyParamTable). Plus de N2/maxVal recopiés à la main.
    void setupParam (MidiSlider& s, const juce::Identifier& id, int ui, const juce::String& name)
    {
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 46, 16);
        const auto* e = SyParam::entry (ui);
        const auto  a = SyVoice::syTranslate (ui, 0, 0);   // effets : group 0x08, T2 0x00
        jassert (e != nullptr && a.ok);
        s.setRangeAndRound (e->dispMin, e->dispMax, 0);
        s.getValueObject().referTo (valueTreeVoice.getPropertyAsValue (id, nullptr));
        int sx[9] = { 0x43, 0x10, 0x34, a.group, a.t2, 0x00, a.n2, 0x00, 0x00 };
        s.setMidiSysex (sx);

        auto* lab = labels.add (new juce::Label());
        addAndMakeVisible (*lab);
        lab->setText (name, juce::dontSendNotification);
        lab->setJustificationType (juce::Justification::centred);
        lab->setFont (juce::FontOptions (12.0f));
        lab->attachToComponent (&s, false);
    }

    void setupToggle (MidiButton& b, const juce::Identifier& id, int ui, const juce::String& name)
    {
        addAndMakeVisible (b);
        b.setButtonText (name);
        b.setClickingTogglesState (true);
        b.getToggleStateValue().referTo (valueTreeVoice.getPropertyAsValue (id, nullptr));
        const auto a = SyVoice::syTranslate (ui, 0, 0);
        jassert (a.ok);
        int sx[9] = { 0x43, 0x10, 0x34, a.group, a.t2, 0x00, a.n2, 0x00, 0x00 };
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

    // Topologies RELEVEES SUR LE SY77 (MAP.md, EFMODE ; meme source que Voice.h).
    void valueChanged (juce::Value&) override { updateMode(); }
    void updateMode()
    {
        const int mode = (int) modeValue.getValue();
        static const char* topo[4] = {
            "Bypass : G1 et G2 en direct",
            "G1 : Chorus 1 > Reverb 1     G2 : Chorus 2 > Reverb 2",
            "G1 : Chorus 1 > Reverb 1 > Reverb 2     G2 : Chorus 2",
            "G1 : (Chorus 1 | Chorus 2) > Reverb 1 > Reverb 2     G2 : direct" };
        labelTopo.setText (TRANS (topo[juce::jlimit (0, 3, mode)]), juce::dontSendNotification);
        const float a = (mode == 0) ? 0.45f : 1.0f;     // bypass : effets sans action
        for (juce::Component* c : { (juce::Component*) &grpChr1, (juce::Component*) &grpChr2,
                                    (juce::Component*) &grpRev1, (juce::Component*) &grpRev2 })
            c->setAlpha (a);
        for (auto* k : { &chr1Type, &chr1Bal, &chr1Lvl, &chr1P1, &chr1P2, &chr1P3, &chr1P4,
                         &chr2Type, &chr2Bal, &chr2Lvl, &chr2P1, &chr2P2, &chr2P3, &chr2P4,
                         &rev1Type, &rev1Bal, &rev1Lvl, &rev1P1, &rev1P2, &rev1P3,
                         &rev2Type, &rev2Bal, &rev2Lvl, &rev2P1, &rev2P2, &rev2P3 })
            k->setAlpha (a);
    }

    MidiRadio  radioMode;
    juce::Value modeValue;
    juce::Label labelModeTitle, labelTopo;

    juce::GroupComponent grpChr1, grpChr2, grpRev1, grpRev2;
    MidiSlider chr1Type, chr1Bal, chr1Lvl, chr1P1, chr1P2, chr1P3, chr1P4;
    MidiSlider chr2Type, chr2Bal, chr2Lvl, chr2P1, chr2P2, chr2P3, chr2P4;
    MidiSlider rev1Type, rev1Bal, rev1Lvl, rev1P1, rev1P2, rev1P3;
    MidiSlider rev2Type, rev2Bal, rev2Lvl, rev2P1, rev2P2, rev2P3;
    MidiButton btMix1, btMix2;

    juce::OwnedArray<juce::Label> labels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsPage)
};
