/*
  ==============================================================================

    Oscillator.h
    Created: 25 Nov 2018 8:29:30pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class Oscillator    : public ElementComponent, public TextButton::Listener
{
public:
    Oscillator()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
     //   setBounds(getBoundsInParent());
        
        addAndMakeVisible(groupOP1);
        addAndMakeVisible(groupOP2);
        addAndMakeVisible(groupOP3);
        addAndMakeVisible(groupOP4);
        addAndMakeVisible(groupOP5);
        addAndMakeVisible(groupOP6);
        
        
        setOscSliderStyle(sliderOsc1);
                setOscSliderStyle(sliderOsc2);
                setOscSliderStyle(sliderOsc3);
                setOscSliderStyle(sliderOsc4);
                setOscSliderStyle(sliderOsc5);
                setOscSliderStyle(sliderOsc6);
       // addAndMakeVisible(labelOsc1);
       // labelOsc1.attachToComponent(&sliderOsc1, false);
        
    }

    ~Oscillator()
    {

    }
    void setOscSliderStyle (Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setPopupDisplayEnabled(true, true, this);
        slider.setRange(0, 15);
        slider.setNumDecimalPlacesToDisplay(0);
        slider.setLookAndFeel(&OscLook);
        slider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
       // slider.setTextBoxStyle(Slider::TextBoxBelow, false, 40, 18);
    }
    
    void setElementNumber ( int element, UndoManager& um) override
    {
 
        int sysexdata2[9] = { 0x43, 0X10, 0x34, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00 };
        int sysexdata[9] = { 0x43, 0X10, 0x34, 0x46, 0x00, 0x00, 0x17, 0x00, 0x00 };
        
        if(element == 1)
        {
            sliderOsc1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC1, &um));
            sliderOsc2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC2, &um));
            sliderOsc3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC3, &um));
            sliderOsc4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC4, &um));
            sliderOsc5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC5, &um));
            sliderOsc6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC6, &um));
        }
        else if (element ==2)
        {
            sysexdata[4] = 0x20;
            sysexdata2[4] = 0x20;
            sliderOsc1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSC1, &um));
            sliderOsc2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSC2, &um));
            sliderOsc3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSC3, &um));
            sliderOsc4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSC4, &um));
            sliderOsc5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSC5, &um));
            sliderOsc6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSC6, &um));
        
        }
    
        else if (element == 3)
        {
            sysexdata[4] = 0x40;
            sysexdata2[4] = 0x40;
            sliderOsc1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSC1, &um));
            sliderOsc2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSC2, &um));
            sliderOsc3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSC3, &um));
            sliderOsc4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSC4, &um));
            sliderOsc5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSC5, &um));
            sliderOsc6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSC6, &um));
        }
        else if (element == 4)
        {
            sysexdata[4] = 0x60;
            sysexdata2[4] = 0x60;
            sliderOsc1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSC1, &um));
            sliderOsc2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSC2, &um));
            sliderOsc3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSC3, &um));
            sliderOsc4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSC4, &um));
            sliderOsc5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSC5, &um));
            sliderOsc6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSC6, &um));
            
        }
        sysexdata[3] = 0x56;
        sliderOsc1.setMidiSysex(sysexdata);
             sysexdata[3] = 0x46;
        sliderOsc2.setMidiSysex(sysexdata);
             sysexdata[3] = 0x36;
        sliderOsc3.setMidiSysex(sysexdata);
             sysexdata[3] = 0x26;
        sliderOsc4.setMidiSysex(sysexdata);
             sysexdata[3] = 0x16;
        sliderOsc5.setMidiSysex(sysexdata);
             sysexdata[3] = 0x06;
        sliderOsc6.setMidiSysex(sysexdata);
  
    }
    
    void buttonClicked (Button* button) override
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

        //g.setColour (Colours::darkorange);
 
       // g.drawText ("AFM a implementer", getLocalBounds(),
       //             Justification::centred, true);   // draw some placeholder text
    }

    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..

        groupOP1.setBoundsRelative(0.0f, 0.0f, 0.33f, 0.5f);
        sliderOsc1.setBoundsRelative(0.2f, 0.04f, 0.12f, 1.0f);
        roundSize(sliderOsc1);
        
        groupOP2.setBoundsRelative(0.33f, 0, 0.33f, 0.5f);
        sliderOsc2.setBoundsRelative(0.53f, 0.04f, 0.12f, 1.0f);
        roundSize(sliderOsc2);
        
        groupOP3.setBoundsRelative(0.66f, 0, 0.33f, 0.5f);
        sliderOsc3.setBoundsRelative(0.86f, 0.04f, 0.12f, 1.0f);
        roundSize(sliderOsc3);
        
        groupOP4.setBoundsRelative(0, 0.5f, 0.33f, 0.5f);
        sliderOsc4.setBoundsRelative(0.2f, 0.54f, 0.12f, 1.0f);
        roundSize(sliderOsc4);
        
        groupOP5.setBoundsRelative(0.33f, 0.5f, 0.33f, 0.5f);
        sliderOsc5.setBoundsRelative(0.53f, 0.54f, 0.12f, 1.0f);
        roundSize(sliderOsc5);
        
        groupOP6.setBoundsRelative(0.66f, 0.5f, 0.33f, 0.5f);
        sliderOsc6.setBoundsRelative(0.86f, 0.54f, 0.12f, 1.0f);
        roundSize(sliderOsc6);

    }
    void roundSize (Slider& slider)
    {
        int w;
        w = jmin(slider.getWidth(),slider.getHeight());
        slider.setBounds(slider.getX(), slider.getY(), w, w);
    }
    
private:
    GroupComponent   groupOP1 {"Op1", "OP1"};
    GroupComponent   groupOP2 {"Op1", "OP2"};
    GroupComponent   groupOP3 {"Op1", "OP3"};
    GroupComponent   groupOP4 {"Op1", "OP4"};
    GroupComponent   groupOP5 {"Op1", "OP5"};
    GroupComponent   groupOP6 {"Op1", "OP6"};
    MidiSlider  sliderOsc1;
    MidiSlider  sliderOsc2;
    MidiSlider  sliderOsc3;
    MidiSlider  sliderOsc4;
    MidiSlider  sliderOsc5;
    MidiSlider  sliderOsc6;
    Label labelOsc1 {"Op1", "Afm Osc"};
    AfmOscLookAndFeel OscLook;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oscillator)
};
