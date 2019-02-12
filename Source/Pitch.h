/*
  ==============================================================================

    Pitch.h
    Created: 25 Nov 2018 12:49:27pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "LookAndFeel.h"
#include "MidiObjects.h"

//==============================================================================
/*
*/
class Pitch    : public Component, public Slider::Listener, private Value::Listener
{
public:
    Pitch()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        addAndMakeVisible(sliderPitch);
        sliderPitch.setRangeAndRound(0, 127, 60);
        sliderPitch.setSliderStyle(Slider::SliderStyle::IncDecButtons);
        sliderPitch.addListener(this);
        
        addAndMakeVisible(groupTitre);
        addAndMakeVisible(btFixed);
        btFixed.setClickingTogglesState(true);
        btFixed.setColour(TextButton::ColourIds::buttonOnColourId, Colours::darkorange);
        addAndMakeVisible(sliderFine);
        valuePitch.addListener(this);
        
     
    }

    ~Pitch()
    {
  
        valuePitch.removeListener(this);
        sliderPitch.removeListener(this);
 
    }
    void setOperator (int opNumber, UndoManager& undoManager)
    {
        intOperator = opNumber;
        
        
        int sysexdata[9] = {0x43, 0x10, 0x34, 0x03, 0x00, 0x00, 0x01, 0x00, 0x00};
        if(opNumber==1)
        {
            valuePitch= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1PITCH.toString(), &undoManager);
            Value valueElement1Fine= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1FINE.toString(), &undoManager);
            Value valueElement1Fixed= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1FIXED.toString(), &undoManager);
            sliderFine.getValueObject().referTo(valueElement1Fine);
            btFixed.getToggleStateValue().referTo(valueElement1Fixed);
       
            
            
            // sliderVolume.getValueObject().referTo(valueElement2Volume);
        }
        if(opNumber == 2)
        {
            valuePitch= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2PITCH.toString(), &undoManager);
            Value valueElement2Fine= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2FINE.toString(), &undoManager);
            Value valueElement2Fixed= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2FIXED.toString(), &undoManager);
            sliderFine.getValueObject().referTo(valueElement2Fine);
            btFixed.getToggleStateValue().referTo(valueElement2Fixed);
            sysexdata[4] = 0x20;
        }
        
        if(opNumber == 3)
        {
            valuePitch= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3PITCH.toString(), &undoManager);
            Value valueElement3Fine= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3FINE.toString(), &undoManager);
            Value valueElement3Fixed= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3FIXED.toString(), &undoManager);
            sliderFine.getValueObject().referTo(valueElement3Fine);
            btFixed.getToggleStateValue().referTo(valueElement3Fixed);
            sysexdata[4] = 0x40;
        }
        
        if(opNumber == 4)
        {
            valuePitch= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4PITCH.toString(), &undoManager);
            Value valueElement4Fine= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4FINE.toString(), &undoManager);
            Value valueElement4Fixed= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4FIXED.toString(), &undoManager);
            sliderFine.getValueObject().referTo(valueElement4Fine);
            btFixed.getToggleStateValue().referTo(valueElement4Fixed);
            sysexdata[4] = 0x60;
        }
        
        
        sliderFine.setMidiSysex(sysexdata);
        sliderFine.setRangeAndRound(-7, 7,0);
        sliderFine.setPopupDisplayEnabled(true, true, this);
        sysexdata[6] = 0x02;
        sliderPitch.setMidiSysex(sysexdata);
        valuePitch.setValue(60);
        sliderPitch.getValueObject().referTo(valuePitch);
      
     
 
        addAndMakeVisible(sliderFine);
    }
    void valueChanged	(	Value & 	value	) override
    {
        Logger::writeToLog("pitch value change");
  //      if(value == valuePitch)
            sliderValueChanged(&sliderPitch);
    }

    void sliderValueChanged (Slider* slider) override
    {
        if(slider == &sliderPitch)
        {
            String str = "Pitch " + MidiMessage::getMidiNoteName(sliderPitch.getValue(), true, true, 3);
            groupTitre.setText(str);
        }
 
        
    }

    
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
           draws some placeholder text to get you started.

           You should replace everything in this method with your own
           drawing code..
        */

       // g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background

        
    }
    
    void resized() override
    {
        int hGrid = getHeight()/6;

        groupTitre.setBounds( 0,0,getWidth(),getHeight());
   
        sliderPitch.setBoundsRelative(0.04f, 0.1f, 0.9f, 0.2f);
 
        btFixed.setBounds(10,getHeight()- (hGrid + 10) ,getWidth()-20, hGrid);
        sliderFine.setBounds(10, hGrid + hGrid + 8, getWidth() - 24, hGrid + hGrid);

    }

private:
    int intOperator = 1;
    int intFine;
    
    GroupComponent groupTitre {"Pitch","Pitch C3"};
    TextButton btFixed {"Fixed", "Fix"};

    MidiSlider sliderFine;
        CustomLookAndFeel myLookAndFeel;
    Value valuePitch;
    MidiSlider  sliderPitch; // strange way but it's like that for the moment
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pitch)
};
