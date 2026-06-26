/*
  ==============================================================================

    PitchEG.h
    Created: 12 Feb 2019 12:11:34am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "SysexUtils.h"
#include "EnvelopeDraw.h"

class PitchEg    : public ElementComponent, public TextButton::Listener, Slider::Listener
{
public:
    PitchEg()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        
        addAndMakeVisible(keyDraw);
        addAndMakeVisible(samplePathLeft);
        addAndMakeVisible(samplePathRight);
        addAndMakeVisible(labelSlope);
        labelSlope.setJustificationType(Justification::centred);
        labelSlope.attachToComponent(&sliderSlope, true);
        
        sliderSlope.setRangeAndRound(-7,7,0);
        sliderSlope.setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(sliderSlope);
        sliderSlope.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
        sliderSlope.addListener(this);
        for (auto* s : { &sliderL0, &sliderL1, &sliderL2, &sliderL3, &sliderL4,
                         &sliderR1, &sliderR2, &sliderR3, &sliderR4,
                         &sliderRL1, &sliderRL2, &sliderRR1, &sliderRR2 })
            s->addListener (this);

        addAndMakeVisible(sliderL0);
        addAndMakeVisible(sliderL1);
        addAndMakeVisible(sliderL2);
        addAndMakeVisible(sliderL3);
        addAndMakeVisible(sliderL4);
        addAndMakeVisible(sliderR0);
        addAndMakeVisible(sliderR1);
        addAndMakeVisible(sliderR2);
        addAndMakeVisible(sliderR3);
        addAndMakeVisible(sliderR4);
        addAndMakeVisible(sliderRL1);
        addAndMakeVisible(sliderRL2);
        addAndMakeVisible(sliderRR1);
        addAndMakeVisible(sliderRR2);
        
        addAndMakeVisible(labelL0);
        addAndMakeVisible(labelL1);
        addAndMakeVisible(labelL2);
        addAndMakeVisible(labelL3);
        addAndMakeVisible(labelL4);
        addAndMakeVisible(labelRL1);
        addAndMakeVisible(labelRL2);
        addAndMakeVisible(labelR1);
        addAndMakeVisible(labelR2);
        addAndMakeVisible(labelR3);
        addAndMakeVisible(labelR4);
        addAndMakeVisible(labelRR1);
        addAndMakeVisible(labelRR2);
        labelL0.attachToComponent(&sliderL0, false);
        labelL1.attachToComponent(&sliderL1, false);
        labelL2.attachToComponent(&sliderL2, false);
        labelL3.attachToComponent(&sliderL3, false);
        labelL4.attachToComponent(&sliderL4, false);
        labelRL1.attachToComponent(&sliderRL1, false);
        labelRL2.attachToComponent(&sliderRL2, false);
        labelR1.attachToComponent(&sliderR1, false);
        labelR2.attachToComponent(&sliderR2, false);
        labelR3.attachToComponent(&sliderR3, false);
        labelR4.attachToComponent(&sliderR4, false);
        labelRR1.attachToComponent(&sliderRR1, false);
        labelRR2.attachToComponent(&sliderRR2, false);

        // Édition à la souris sur le grand graphe d'EG (gauche) : niveau (Y) + rate (X).
        addAndMakeVisible (egGraph);
        egGraph.maxLevel = 127.0f;   // niveaux EG en octet filaire o/b 0..127 (centre 64)
        egGraph.getData = [this] (juce::Array<float>& levels, juce::Array<float>& weights)
        {
            levels = { (float) sliderL0.getValue(), (float) sliderL1.getValue(), (float) sliderL2.getValue(),
                       (float) sliderL3.getValue(), (float) sliderL4.getValue(),
                       (float) sliderRL1.getValue(), (float) sliderRL2.getValue() };
            auto rw = [] (double r) { return (float) (66.0 - r); };
            weights = { rw (sliderR1.getValue()), rw (sliderR2.getValue()), rw (sliderR3.getValue()),
                        rw (sliderR4.getValue()), rw (sliderRR1.getValue()), rw (sliderRR2.getValue()) };
        };
        egGraph.onEditNode = [this] (int node, float levelF, float segWeightF)
        {
            MidiSlider* L[7] = { &sliderL0, &sliderL1, &sliderL2, &sliderL3, &sliderL4, &sliderRL1, &sliderRL2 };
            MidiSlider* R[6] = { &sliderR1, &sliderR2, &sliderR3, &sliderR4, &sliderRR1, &sliderRR2 };
            if (node < 0 || node > 6) return;
            L[node]->setValue (jlimit (0, 127, roundToInt (levelF)));
            if (node >= 1 && segWeightF >= 0.0f)
                R[node - 1]->setValue (jlimit (0, 64, roundToInt (66.0f - segWeightF)));
        };
    }

    ~PitchEg()
    {
        sliderSlope.removeListener(this);
    }
    void 	sliderValueChanged (Slider *slider) override
    {
        Logger::writeToLog("Filter::Slider value changed");
        
        if(slider == &sliderSlope)
            resized();
        repaint();
    }
    void setElementNumber ( int element, UndoManager& um) override
    {
        Logger::writeToLog( "PitchEg setElement");
        // Pitch EG = AFM Element Common (groupe 0x05). T2 = (element-1)<<5 (réglé par bloc ci-dessous).
        // Offsets spec Table 1-6 : FPR1-3=01-03, FPRR1=04, FPL0-3=05-08, FPRL1=09, FPRS(slope)=0B.
        const int pitchGroup = (int) SyVoice::egGroupFor (SyVoice::EgKind::pitch);   // 0x05
        int sysexdata2[9] = { 0x43, 0X10, 0x34, pitchGroup, 0x00, 0x00, 0x01, 0x00, 0x00 }; // rates
        int sysexdata[9]  = { 0x43, 0X10, 0x34, pitchGroup, 0x00, 0x00, 0x05, 0x00, 0x00 }; // levels
        
        if(element == 1)
        {
            
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1SLOPEPITCH, &um));
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHLEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHLEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHLEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHLEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHLEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHRL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHRL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHR1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHR2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHR3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHR4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHRR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGPITCHRR2 , &um));
            
        }
        else if (element ==2)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2SLOPEPITCH, &um));
            sysexdata[4] = 0x20;
            sysexdata2[4] = 0x20;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHLEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHLEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHLEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHLEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHLEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHRL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHRL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHR1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHR2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHR3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHR4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHRR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGPITCHRR2 , &um));
            
        }
        else if (element == 3)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3SLOPEPITCH, &um));
            sysexdata[4] = 0x40;
            sysexdata2[4] = 0x40;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHLEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHLEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHLEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHLEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHLEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHRL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHRL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHR1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHR2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHR3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHR4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHRR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGPITCHRR2 , &um));
            
        }
        else if (element == 4)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4SLOPEPITCH, &um));
            sysexdata[4] = 0x60;
            sysexdata2[4] = 0x60;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHLEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHLEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHLEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHLEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHLEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHRL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHRL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHR1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHR2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHR3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHR4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHRR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGPITCHRR2 , &um));
            
        }
        // --- Sysex Pitch EG (groupe 0x05). On n'émet QUE les paramètres documentés. ---
        sysexdata[6] = 0x0B;                        // FPRS (rate scaling) -> slider "slope"
        sliderSlope.setMidiSysex(sysexdata);

        // Levels FPL0-3 (05-08) + release level FPRL1 (09). Encodage offset-binary (o/b) :
        // l'octet filaire est 0..127 (centre 64) ; la valeur STOCKÉE par le slider EST cet octet
        // filaire (midiTxOffset 0 -> passthrough TX/RX), et l'AFFICHAGE montre le signé -64..+63
        // via egLevelToDisplay (provenance main.lua l.22 + TG77_Voice.json pNum 7005-7008).
        // Centre par défaut = 64. Plage 0..127 -> le slider atteint enfin TOUTE la plage du synthé.
        for (auto* s : { &sliderL0, &sliderL1, &sliderL2, &sliderL3, &sliderL4, &sliderRL1, &sliderRL2 })
            SyVoice::applyEgLevelDisplay (*s);
        sysexdata[6] = 0x05; sliderL0.setMidiSysex(sysexdata);  sliderL0.setRangeAndRound(0, 127, 64);
        sysexdata[6] = 0x06; sliderL1.setMidiSysex(sysexdata);  sliderL1.setRangeAndRound(0, 127, 64);
        sysexdata[6] = 0x07; sliderL2.setMidiSysex(sysexdata);  sliderL2.setRangeAndRound(0, 127, 64);
        sysexdata[6] = 0x08; sliderL3.setMidiSysex(sysexdata);  sliderL3.setRangeAndRound(0, 127, 64);
        sysexdata[6] = 0x09; sliderRL1.setMidiSysex(sysexdata); sliderRL1.setRangeAndRound(0, 127, 64);

        // Rates FPR1-3 (01-03) + release rate FPRR1 (04)
        sysexdata2[6] = 0x01; sliderR1.setMidiSysex(sysexdata2);  sliderR1.setRangeAndRound(0, 63, 0);
        sysexdata2[6] = 0x02; sliderR2.setMidiSysex(sysexdata2);  sliderR2.setRangeAndRound(0, 63, 0);
        sysexdata2[6] = 0x03; sliderR3.setMidiSysex(sysexdata2);  sliderR3.setRangeAndRound(0, 63, 0);
        sysexdata2[6] = 0x04; sliderRR1.setMidiSysex(sysexdata2); sliderRR1.setRangeAndRound(0, 63, 0);

        // Sliders sans équivalent dans le Pitch EG SY77 (pas de L4 ; pas de 4e on-rate ;
        // pas de 2e release) : affichés et persistés mais NON émis vers le synthé.
        sliderL4.setRangeAndRound(0, 127, 64);   // niveau o/b (centre 64)
        sliderR4.setRangeAndRound(0, 63, 0);
        sliderRL2.setRangeAndRound(0, 127, 64);  // niveau o/b (centre 64)
        sliderRR2.setRangeAndRound(0, 63, 0);
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        auto bounds = getLocalBounds().toFloat();
        juce::Rectangle<float> egArea (bounds.getWidth() * 0.03f, bounds.getHeight() * 0.05f,
                                       bounds.getWidth() * 0.58f, bounds.getHeight() * 0.74f);

        juce::Array<float> egLevels { (float) sliderL0.getValue(), (float) sliderL1.getValue(),
                                      (float) sliderL2.getValue(), (float) sliderL3.getValue(),
                                      (float) sliderL4.getValue(), (float) sliderRL1.getValue(),
                                      (float) sliderRL2.getValue() };

        auto rateWeight = [] (double rate) { return (float) (66.0 - rate); };
        juce::Array<float> egWeights { rateWeight (sliderR1.getValue()), rateWeight (sliderR2.getValue()),
                                       rateWeight (sliderR3.getValue()), rateWeight (sliderR4.getValue()),
                                       rateWeight (sliderRR1.getValue()), rateWeight (sliderRR2.getValue()) };

        SyDraw::drawEnvelope (g, egArea, egLevels, egWeights, 127.0f, SYColSelected, "Pitch EG");
        
    }
    void buttonClicked (Button* button) override
    {
        setVisible(false);
    }
    
    void resized() override
    {
        auto b = getLocalBounds().toFloat();
        egGraph.setBounds (juce::Rectangle<float> (b.getWidth() * 0.03f, b.getHeight() * 0.05f,
                                                   b.getWidth() * 0.58f, b.getHeight() * 0.74f).toNearestInt());

        keyDraw.setBoundsRelative(0.0f, 0.85f, 1.0f, 0.09f);
        float sampleWidth = 0.2f + (sliderSlope.getValue()/100);
        samplePathLeft.setBoundsRelative(0.0f, 0.93f, sampleWidth, 0.09f);
        sampleWidth = 0.2f - (sliderSlope.getValue()/100);
        samplePathRight.setBoundsRelative(1.0f - sampleWidth, 0.93f, sampleWidth, 0.09f);
        
        // Tailles FIXES (cf. WaveEg) : sliders qui n'enflent plus, Slope au-dessus du clavier.
        const int W = getWidth(), H = getHeight();
        const int kbTop = (int) (H * 0.85f);
        sliderSlope.setBounds (jmax (90, (int) (W * 0.42f)), kbTop - 30, 160, 22);

        const int sw   = 26;
        const int sh   = jlimit (90, 200, (int) (H * 0.28f));
        const int x0   = (int) (W * 0.63f);
        const int step = jmax (sw + 8, jmin (sw + 34, (W - x0 - 12) / 7));
        const int y1   = (int) (H * 0.11f);
        const int y2   = y1 + sh + 26;

        MidiSlider* top[7] = { &sliderL0,&sliderL1,&sliderL2,&sliderL3,&sliderL4,&sliderRL1,&sliderRL2 };
        for (int i = 0; i < 7; ++i) top[i]->setBounds (x0 + i * step, y1, sw, sh);
        MidiSlider* bot[6] = { &sliderR1,&sliderR2,&sliderR3,&sliderR4,&sliderRR1,&sliderRR2 };
        for (int i = 0; i < 6; ++i) bot[i]->setBounds (x0 + (i + 1) * step, y2, sw, sh);
    }
    
private:
    
    Label   labelSlope {"test", "Slope"};
    MidiKeyDraw keyDraw;
    MidiSlider  sliderSlope;
    MidiPath    samplePathLeft;
    MidiPath    samplePathRight;
    
    Label labelL0 {"","L0"};
    Label labelL1 {"","L1"};
    Label labelL2 {"","L2"};
    Label labelL3 {"","L3"};
    Label labelL4 {"","L4"};
    Label labelRL1 {"","RL1"};
    Label labelRL2 {"","RL2"};
    Label labelR1 {"","R1"};
    Label labelR2 {"","R2"};
    Label labelR3 {"","R3"};
    Label labelR4 {"","R4"};
    Label labelRR1 {"","RR1"};
    Label labelRR2 {"","RR2"};
    
    MidiSlider sliderL0;
    MidiSlider sliderL1;
    MidiSlider sliderL2;
    MidiSlider sliderL3;
    MidiSlider  sliderL4;
    MidiSlider  sliderRL1;
    MidiSlider  sliderRL2;
    
    MidiSlider  sliderR0;
    MidiSlider  sliderR1;
    MidiSlider  sliderR2;
    MidiSlider  sliderR3;
    MidiSlider  sliderR4;
    MidiSlider  sliderRR1;
    MidiSlider  sliderRR2;

    EgGraphView egGraph; // édition souris des nœuds de l'EG (niveau + rate) sur le grand graphe

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchEg)
};
