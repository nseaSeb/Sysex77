/*
  ==============================================================================

    Filter2.h
    Created: 10 Feb 2019 11:26:20am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
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
    }
    void setElementNumber ( int element, UndoManager& um) override
    {
        Logger::writeToLog( "Filter1 setElement");
        int sysexdata2[9] = { 0x43, 0X10, 0x34, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00 };
        int sysexdata[9] = { 0x43, 0X10, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        
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
        sysexdata[6] = 0x10;
        sliderSlope.setMidiSysex(sysexdata);
        
        sysexdata[4] +=0x01;  // filtre 2
        
        sysexdata[6] = 0x09;
        sysexdata2[6] = 0x03;
        //       EgFilter.addSegment(10, 10, "L0R1", 64, sysexdata, 64, sysexdata2);
        sliderL0.setMidiSysex(sysexdata);
        sliderL0.setRangeAndRound(0, 64, 0);
        sliderR1.setMidiSysex(sysexdata2);
        sliderR1.setRangeAndRound(0, 64, 0);
        
        sysexdata[6] = 0x0a;
        sysexdata2[6] = 0x04;
        //        EgFilter.addSegment(10, 10, "L1R2", 64, sysexdata, 64, sysexdata2);
        sliderL1.setMidiSysex(sysexdata);
        sliderL1.setRangeAndRound(0, 64, 0);
        sliderR2.setMidiSysex(sysexdata2);
        sliderR2.setRangeAndRound(0, 64, 0);
        
        sysexdata[6] = 0x0b;
        sysexdata2[6] = 0x05;
        //        EgFilter.addSegment(10, 10, "L2R3", 64, sysexdata, 64, sysexdata2);
        sliderL2.setMidiSysex(sysexdata);
        sliderL2.setRangeAndRound(0, 64, 0);
        sliderR3.setMidiSysex(sysexdata2);
        sliderR3.setRangeAndRound(0, 64, 0);
        
        sysexdata[6] = 0x0c;
        sysexdata2[6] = 0x06;
        //       EgFilter.addSegment(10, 10, "L3R4", 64, sysexdata, 64, sysexdata2);
        sliderL3.setMidiSysex(sysexdata);
        sliderL3.setRangeAndRound(0, 64, 0);
        sliderR4.setMidiSysex(sysexdata2);
        sliderR4.setRangeAndRound(0, 64, 0);
        
        sysexdata[6] = 0x0d;
        sysexdata2[6] = 0x07;
        //       EgFilter.addSegment(10, 10, "L4RR1", 64, sysexdata, 64, sysexdata2);
        sliderL4.setMidiSysex(sysexdata);
        sliderL4.setRangeAndRound(0, 64, 0);
        sliderRR1.setMidiSysex(sysexdata2);
        sliderRR2.setRangeAndRound(0, 64, 0);
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
        g.setColour(Colours::darkorange);
        g.drawText("A faire ajouter ADSR graphique et cutoff Scaling", 0, getHeight()/2, getWidth()/2, 20, Justification::centred);
        
    }
    void buttonClicked (Button* button) override
    {
        setVisible(false);
    }
    
    void resized() override
    {
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
    
    UndoManager um;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Filter2)
};
