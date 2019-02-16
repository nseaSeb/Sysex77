/*
  ==============================================================================

    Volume.h
    Created: 25 Nov 2018 8:30:16pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ADSR.h"

//==============================================================================
/*
 Normalement opsolete a supprimer
*/
class Volume    : public Component, public Button::Listener,public Slider::Listener,public Value::Listener,public ChangeListener
{
public:
    Volume()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        addAndMakeVisible(btClose);
        btClose.addListener(this);
        addAndMakeVisible(sliderRelease);
        sliderRelease.setSkewFactor(1);
        sliderRelease.setRange(0, 64);
        sliderRelease.setDoubleClickReturnValue(true, 32);
        sliderRelease.setNumDecimalPlacesToDisplay(0);
        sliderRelease.setPopupDisplayEnabled(true, true, this);
        sliderRelease.setLookAndFeel(&myLookAndFeel);
        sliderRelease.addListener(this);
        
        addAndMakeVisible(sliderR4);
        sliderR4.setRange(0, 64);
        sliderR4.setDoubleClickReturnValue(true, 32);
        sliderR4.setNumDecimalPlacesToDisplay(0);
        sliderR4.setPopupDisplayEnabled(true, true, this);
        sliderR4.setLookAndFeel(&myLookAndFeel);
        sliderR4.addListener(this);

        addAndMakeVisible(sliderSlope);
        sliderSlope.setSkewFactor(1);
        sliderSlope.setRange(-7, 7);
        sliderSlope.setDoubleClickReturnValue(true, 0);
        sliderSlope.setLookAndFeel(&panLookAndFeel);
        sliderSlope.setNumDecimalPlacesToDisplay(0);
        sliderSlope.setPopupDisplayEnabled(true,true, this);
        sliderSlope.addListener(this);
        
        addAndMakeVisible(labelRelease);
        addAndMakeVisible(labelSlope);
        addAndMakeVisible(labelR4);

 
        
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
    
    }

    ~Volume()
    {
        btClose.removeListener(this);
        sliderRelease.removeListener(this);
        sliderRelease.setLookAndFeel(nullptr);
        sliderR4.removeListener(this);
        sliderR4.setLookAndFeel(nullptr);
        sliderSlope.removeListener(this);
        sliderSlope.setLookAndFeel(nullptr);
        
  
     
    }
    void valueChanged(Value & value) override
    {
       
    }
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        
        
        Logger::writeToLog("Volume: changeListener");
        //    Logger::writeToLog(source);
        //    test.setVisible(true);
// When Graf change We update the value of L1 ( element volume )

 
    }
    void setElementNumber ( int number)
    {
        btClose.setButtonText("Close Volume Vue OP: " + String(number));
        intElement = number;
// Need to initialise here if we wanna have the element number
        int sysexdata[9] = { 0x43, 0X10, 0x34, 0x07, 0x00, 0x00, 0x50, 0x00, 0x00 };
        if(intElement == 2)
            sysexdata[4] = 0x20;
        if(intElement == 3)
            sysexdata[4] = 0x40;
        if(intElement == 4)
            sysexdata[4] = 0x60;
        
        sliderR1.setMidiSysex(sysexdata);
        sliderR1.setRangeAndRound(0, 63,0);
        addAndMakeVisible(sliderR1);
        
        sysexdata[6] = 0x51;
        sliderR2.setMidiSysex(sysexdata);
        sliderR2.setRangeAndRound(0, 63,0);
        
        addAndMakeVisible(sliderR2);
        
        sysexdata[6] = 0x52;
        sliderR3.setMidiSysex(sysexdata);
        sliderR3.setRangeAndRound(0, 63,0);
        addAndMakeVisible(sliderR3);
        
        sysexdata[6] = 0x55;
        sliderL2.setMidiSysex(sysexdata);
        sliderL2.setRangeAndRound(0, 63,0);
        addAndMakeVisible(sliderL2);
        
        sysexdata[6] = 0x56;
        sliderL3.setMidiSysex(sysexdata);
        sliderL3.setRangeAndRound(0, 63,0);
        addAndMakeVisible(sliderL3);
        
        sysexdata[6] = 0x4f;
        btHold.setMidiSysex(sysexdata);
        btHold.setTextOnOff("Hold", "Attack");
        addAndMakeVisible(btHold);
    }
    void sliderValueChanged (Slider* slider) override
    {
        //R4 replace the release graf to be more visual on the graf
        //Release doesn't drawed on the graf for the moment
        
        Logger::writeToLog ("Volume slider value change");


    }

    
    void setEgName(String name)
    {
       
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
           draws some placeholder text to get you started.

           You should replace everything in this method with your own
           drawing code..
        */

        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background

//        g.setColour (Colours::grey);
//        g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    }
    void buttonClicked (Button* button) override
    {
        if(button==&btClose)
        setVisible(false);
        
       
    }
    void resized() override
    {
        int wgrid = getWidth()/8;
        btHold.setBounds(10,getHeight()-30,wgrid-10,24);
        sliderSlope.setBounds(wgrid * 4, getHeight()-50, wgrid -10,48);
        labelSlope.setBounds(wgrid * 5, getHeight()-30,wgrid-10,24);
        sliderRelease.setBounds(wgrid * 8, getHeight()-50, wgrid -10,48);
        labelRelease.setBounds(wgrid*9, getHeight()-30,wgrid-10,24);
        sliderR4.setBounds(wgrid * 6, getHeight()-50, wgrid -10,48);
        labelR4.setBounds(wgrid*7, getHeight()-30,wgrid-10,24);
        sliderR1.setBounds(10, 50, wgrid - 20, getHeight()-100);
        sliderR2.setBounds(10 + wgrid, 50, wgrid - 20, getHeight()-100);
        sliderR3.setBounds(10 + wgrid *2, 50, wgrid - 20, getHeight()-100);
        sliderL2.setBounds(10 + wgrid*3, 50, wgrid - 20, getHeight()-100);
        sliderL3.setBounds(10 + wgrid*4, 50, wgrid - 20, getHeight()-100);
        // This method is where you should set the bounds of any child
        // components that your component contains..
        btClose.setBounds(10, 4, getWidth()-20, 20);
    }

private:
        TextButton btClose {TRANS("Close volume editor")};
 
    int intElement =1;
 
    CustomLookAndFeelV2 myLookAndFeel;
    CustomLookAndFeel panLookAndFeel;
    Slider  sliderRelease {Slider::SliderStyle::Rotary , Slider::NoTextBox};
    Slider  sliderR4 {Slider::SliderStyle::Rotary , Slider::NoTextBox};
    Slider  sliderSlope {Slider::SliderStyle::Rotary , Slider::NoTextBox};
      Label labelRelease {"","Release"};
      Label labelSlope {"","Slope"};
    Label labelR4 {"","R4"};
    MidiSlider  sliderR1;
    MidiSlider  sliderR2;
    MidiSlider sliderR3;
    MidiSlider sliderL2;
    MidiSlider sliderL3;
    MidiButton btHold;
    OSCSender sender;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Volume)
};
