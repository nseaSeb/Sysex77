/*
  ==============================================================================

    Filter2.h
    Created: 10 Feb 2019 11:26:20am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "SysexUtils.h"
#include "EnvelopeDraw.h"

class Filter2    : public ElementComponent, public TextButton::Listener, Slider::Listener
{
public:
    Filter2()
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
        egGraph.maxLevel = 127.0f;   // niveaux EG filtre 2 en octet filaire o/b 0..127 (centre 64)
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

    ~Filter2()
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
    // AFM (fN 1) / AWM (fN 4) pour le filtre 2 (EG). Ré-applique l'adressage courant.
    void setAwmMode (bool awm) override
    {
        fnBase = awm ? 3 : 0;
        if (lastUm != nullptr) setElementNumber (storedElement, *lastUm);
    }
    int fnBase = 0;
    int storedElement = 1;
    UndoManager* lastUm = nullptr;

    void setElementNumber ( int element, UndoManager& um) override
    {
        Logger::writeToLog( "Filter2 setElement");
        storedElement = element; lastUm = &um;
        // Groupe 0x09 = Filtre (SY77/TG77 Table 1-10). T2 = ((elem-1)<<5)|fN ; fN=1 (filtre AFM 2,
        // ajouté plus bas sur les DEUX tableaux). Offsets EG conformes spec (FR1-4=03-06,FRR1=07,FL0-4=09-0D,FRS=10).
        // (Avant : groupe 0x00 jamais valide -> les EG n'atteignaient pas le synthé.)
        const int filterGroup = (int) SyVoice::egGroupFor (SyVoice::EgKind::filter);   // 0x09
        int sysexdata2[9] = { 0x43, 0X10, 0x34, filterGroup, 0x00, 0x00, 0x09, 0x00, 0x00 };
        int sysexdata[9] = { 0x43, 0X10, 0x34, filterGroup, 0x00, 0x00, 0x00, 0x00, 0x00 };

        if(element == 1)
        {
            
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1SLOPEFILTRE2, &um));
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2LEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2LEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2LEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2LEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2LEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2RL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2RL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2R1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2R2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2R3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2R4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2RR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2RR2 , &um));
        }
        else if (element ==2)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2SLOPEFILTRE2, &um));
            sysexdata[4] = 0x20;
            sysexdata2[4] = 0x20;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2LEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2LEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2LEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2LEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2LEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2RL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2RL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2R1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2R2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2R3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2R4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2RR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2RR2 , &um));
        }
        else if (element == 3)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3SLOPEFILTRE2, &um));
            sysexdata[4] = 0x40;
            sysexdata2[4] = 0x40;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2LEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2LEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2LEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2LEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2LEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2RL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2RL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2R1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2R2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2R3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2R4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2RR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2RR2 , &um));
        }
        else if (element == 4)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4SLOPEFILTRE2, &um));
            sysexdata[4] = 0x60;
            sysexdata2[4] = 0x60;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2LEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2LEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2LEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2LEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2LEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2RL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2RL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2R1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2R2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2R3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2R4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2RR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2RR2 , &um));
        }
        // fN du filtre 2 : 1 (AFM) / 4 (AWM) sur les DEUX tableaux, AVANT tout envoi
        // (slope/FRS inclus) : sinon le slope et les rates partaient vers le filtre 1.
        sysexdata[4]  += 0x01 + fnBase;
        sysexdata2[4] += 0x01 + fnBase;

        sysexdata[6] = 0x10;
        sliderSlope.setMidiSysex(sysexdata);

        // Niveaux d'EG filtre 2 FL0-4 / FRL1-2 : offset-binary (o/b). Octet filaire 0..127
        // (centre 64) ; le slider stocke l'octet filaire et affiche le signé -64..+63 via
        // egLevelToDisplay. Provenance : main.lua l.22 + TG77_Voice.json pNum 7309-7315.
        for (auto* s : { &sliderL0, &sliderL1, &sliderL2, &sliderL3, &sliderL4, &sliderRL1, &sliderRL2 })
            SyVoice::applyEgLevelDisplay (*s);

        sysexdata[6] = 0x09;
        sysexdata2[6] = 0x03;
        sliderL0.setMidiSysex(sysexdata);
        sliderL0.setRangeAndRound(0, 127, 64);
        sliderR1.setMidiSysex(sysexdata2);
        sliderR1.setRangeAndRound(0, 64, 0);

        sysexdata[6] = 0x0a;
        sysexdata2[6] = 0x04;
        sliderL1.setMidiSysex(sysexdata);
        sliderL1.setRangeAndRound(0, 127, 64);
        sliderR2.setMidiSysex(sysexdata2);
        sliderR2.setRangeAndRound(0, 64, 0);

        sysexdata[6] = 0x0b;
        sysexdata2[6] = 0x05;
        sliderL2.setMidiSysex(sysexdata);
        sliderL2.setRangeAndRound(0, 127, 64);
        sliderR3.setMidiSysex(sysexdata2);
        sliderR3.setRangeAndRound(0, 64, 0);

        sysexdata[6] = 0x0c;
        sysexdata2[6] = 0x06;
        sliderL3.setMidiSysex(sysexdata);
        sliderL3.setRangeAndRound(0, 127, 64);
        sliderR4.setMidiSysex(sysexdata2);
        sliderR4.setRangeAndRound(0, 64, 0);

        sysexdata[6] = 0x0d;
        sysexdata2[6] = 0x07;
        sliderL4.setMidiSysex(sysexdata);
        sliderL4.setRangeAndRound(0, 127, 64);
        sliderRR1.setMidiSysex(sysexdata2);
        sliderRR1.setRangeAndRound(0, 64, 0);

        // Compléments EG filtre 2 (le bit fN=1 est déjà appliqué à [4] plus haut) :
        // FRR2 (rate, 0x08), FRL1 (level, 0x0E, o/b), FRL2 (level, 0x0F, o/b).
        sysexdata2[6] = 0x08;               // FRR2 (key_off Rate 2)
        sliderRR2.setMidiSysex(sysexdata2);
        sliderRR2.setRangeAndRound(0, 64, 0);
        sysexdata[6] = 0x0e;                // FRL1 (key_off cut_off Level 1, o/b)
        sliderRL1.setMidiSysex(sysexdata);
        sliderRL1.setRangeAndRound(0, 127, 64);
        sysexdata[6] = 0x0f;                // FRL2 (key_off cut_off Level 2, o/b)
        sliderRL2.setMidiSysex(sysexdata);
        sliderRL2.setRangeAndRound(0, 127, 64);
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

        SyDraw::drawEnvelope (g, egArea, egLevels, egWeights, 127.0f, SYColSelected, "Filter 2 EG");
        
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
        
        sliderSlope.setBoundsRelative(0.4f, 0.92f, 0.2f, 0.1f);
        
        sliderL0.setBoundsRelative(0.65f, 0.06, 0.04f, 0.3f);
        sliderL1.setBoundsRelative(0.7f, 0.06, 0.04f, 0.3f);
        sliderL2.setBoundsRelative(0.75f, 0.06, 0.04f, 0.3f);
        sliderL3.setBoundsRelative(0.8f, 0.06, 0.04f, 0.3f);
        sliderL4.setBoundsRelative(0.85f, 0.06, 0.04f, 0.3f);
        sliderRL1.setBoundsRelative(0.9f, 0.06, 0.04f, 0.3f);
        sliderRL2.setBoundsRelative(0.95f, 0.06, 0.04f, 0.3f);
        
        sliderR1.setBoundsRelative(0.7f, 0.4, 0.04f, 0.3f);
        sliderR2.setBoundsRelative(0.75f, 0.4, 0.04f, 0.3f);
        sliderR3.setBoundsRelative(0.8f, 0.4, 0.04f, 0.3f);
        sliderR4.setBoundsRelative(0.85f, 0.4, 0.04f, 0.3f);
        sliderRR1.setBoundsRelative(0.9f, 0.4, 0.04f, 0.3f);
        sliderRR2.setBoundsRelative(0.95f, 0.4, 0.04f, 0.3f);
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

    EgGraphView egGraph; // édition souris des nœuds de l'EG de filtre (niveau + rate)

    UndoManager um;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Filter2)
};
