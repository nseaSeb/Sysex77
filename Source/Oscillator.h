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
        
        setSliderStyle(sliderFine1);
        labelFine1.attachToComponent(&sliderFine1, false);
        setSliderDetune(sliderDetune1);
        labelDetune1.attachToComponent(&sliderDetune1, false);
        labelDetune1.setJustificationType(Justification::centred);
        setSliderStyle(sliderPhase1);
        labelPhase1.attachToComponent(&sliderPhase1, false);
        
        setSliderStyle(sliderFine2);
        labelFine2.attachToComponent(&sliderFine1, false);
        setSliderDetune(sliderDetune2);
        labelDetune2.attachToComponent(&sliderDetune2, false);
        labelDetune2.setJustificationType(Justification::centred);
        setSliderStyle(sliderPhase2);
        labelPhase2.attachToComponent(&sliderPhase1, false);
        
        setSliderStyle(sliderFine3);
        labelFine3.attachToComponent(&sliderFine3, false);
        setSliderDetune(sliderDetune3);
        labelDetune3.attachToComponent(&sliderDetune3, false);
        labelDetune3.setJustificationType(Justification::centred);
        setSliderStyle(sliderPhase3);
        labelPhase3.attachToComponent(&sliderPhase3, false);
        
        setSliderStyle(sliderFine4);
        labelFine4.attachToComponent(&sliderFine4, false);
        setSliderDetune(sliderDetune4);
        labelDetune4.attachToComponent(&sliderDetune4, false);
        labelDetune4.setJustificationType(Justification::centred);
        setSliderStyle(sliderPhase4);
        labelPhase4.attachToComponent(&sliderPhase4, false);
        
        setSliderStyle(sliderFine5);
        labelFine5.attachToComponent(&sliderFine5, false);
        setSliderDetune(sliderDetune5);
        labelDetune5.attachToComponent(&sliderDetune5, false);
        labelDetune5.setJustificationType(Justification::centred);
        setSliderStyle(sliderPhase5);
        labelPhase5.attachToComponent(&sliderPhase5, false);
        
        setSliderStyle(sliderFine6);
        labelFine6.attachToComponent(&sliderFine6, false);
        setSliderDetune(sliderDetune6);
        labelDetune6.attachToComponent(&sliderDetune6, false);
        labelDetune6.setJustificationType(Justification::centred);
        setSliderStyle(sliderPhase6);
        labelPhase6.attachToComponent(&sliderPhase6, false);
        
       // addAndMakeVisible(labelOsc1);
       // labelOsc1.attachToComponent(&sliderOsc1, false);
        addAndMakeVisible(btFix1);
                addAndMakeVisible(btFix2);
                addAndMakeVisible(btFix3);
                addAndMakeVisible(btFix4);
                addAndMakeVisible(btFix5);
                addAndMakeVisible(btFix6);
        btFix1.setTextOnOff("Fixed", "Ratio");
                btFix2.setTextOnOff("Fixed", "Ratio");
                btFix3.setTextOnOff("Fixed", "Ratio");
                btFix4.setTextOnOff("Fixed", "Ratio");
                btFix5.setTextOnOff("Fixed", "Ratio");
                btFix6.setTextOnOff("Fixed", "Ratio");
        
        addAndMakeVisible(btPhase1);
        addAndMakeVisible(btPhase2);
        addAndMakeVisible(btPhase3);
        addAndMakeVisible(btPhase4);
        addAndMakeVisible(btPhase5);
        addAndMakeVisible(btPhase6);
        btPhase1.setTextOnOff("Sync Off", "Sync On");
        btPhase2.setTextOnOff("Sync Off", "Sync On");
        btPhase3.setTextOnOff("Sync Off", "Sync On");
        btPhase4.setTextOnOff("Sync Off", "Sync On");
        btPhase5.setTextOnOff("Sync Off", "Sync On");
        btPhase6.setTextOnOff("Sync Off", "Sync On");
        
    }

    ~Oscillator()
    {

    }
    void setSliderDetune (Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setSliderStyle(Slider::SliderStyle::Rotary);
        slider.setRange(-15, 15);
        slider.setNumDecimalPlacesToDisplay(0);
        slider.setLookAndFeel(&myLookAndFeel);
        slider.setPopupDisplayEnabled(true, true,this);

        
 
    }
    void setSliderStyle (Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setPopupDisplayEnabled(true, true, this);
        slider.setRange(0, 127);
        slider.setNumDecimalPlacesToDisplay(0);
      //  slider.setLookAndFeel(&OscLook);
        slider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
        // slider.setTextBoxStyle(Slider::TextBoxBelow, false, 40, 18);
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
 
        int sysexdata2[9] = { 0x43, 0X10, 0x34, 0x56, 0x00, 0x00, 0x26, 0x00, 0x00 };
        int sysexdata[9] = { 0x43, 0X10, 0x34, 0x46, 0x00, 0x00, 0x17, 0x00, 0x00 };
        
        if(element == 1)
        {
            sliderOsc1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC1, &um));
            sliderOsc2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC2, &um));
            sliderOsc3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC3, &um));
            sliderOsc4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC4, &um));
            sliderOsc5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC5, &um));
            sliderOsc6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC6, &um));

            btFix1.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFIX1, &um));
            btFix2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFIX2, &um));
            btFix3.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFIX3, &um));
            btFix4.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFIX4, &um));
            btFix5.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFIX5, &um));
            btFix6.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFIX6, &um));
            
            btPhase1.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCSYNC1, &um));
            btPhase2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCSYNC2, &um));
            btPhase3.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCSYNC3, &um));
            btPhase4.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCSYNC4, &um));
            btPhase5.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCSYNC5, &um));
            btPhase6.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCSYNC6, &um));
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
            
            btFix1.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFIX1, &um));
            btFix2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFIX2, &um));
            btFix3.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFIX3, &um));
            btFix4.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFIX4, &um));
            btFix5.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFIX5, &um));
            btFix6.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFIX6, &um));
            
            btPhase1.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCSYNC1, &um));
            btPhase2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCSYNC2, &um));
            btPhase3.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCSYNC3, &um));
            btPhase4.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCSYNC4, &um));
            btPhase5.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCSYNC5, &um));
            btPhase6.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCSYNC6, &um));
        
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
            
            btFix1.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFIX1, &um));
            btFix2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFIX2, &um));
            btFix3.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFIX3, &um));
            btFix4.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFIX4, &um));
            btFix5.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFIX5, &um));
            btFix6.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFIX6, &um));
            
            btPhase1.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCSYNC1, &um));
            btPhase2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCSYNC2, &um));
            btPhase3.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCSYNC3, &um));
            btPhase4.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCSYNC4, &um));
            btPhase5.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCSYNC5, &um));
            btPhase6.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCSYNC6, &um));
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
            
            btFix1.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFIX1, &um));
            btFix2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFIX2, &um));
            btFix3.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFIX3, &um));
            btFix4.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFIX4, &um));
            btFix5.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFIX5, &um));
            btFix6.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFIX6, &um));
            
            btPhase1.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCSYNC1, &um));
            btPhase2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCSYNC2, &um));
            btPhase3.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCSYNC3, &um));
            btPhase4.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCSYNC4, &um));
            btPhase5.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCSYNC5, &um));
            btPhase6.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCSYNC6, &um));
            
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
        
        sysexdata[6] = 0x18;
        sysexdata[3] = 0x56;
        btFix1.setMidiSysex(sysexdata);
            sysexdata[3] = 0x46;
        btFix2.setMidiSysex(sysexdata);
            sysexdata[3] = 0x36;
        btFix3.setMidiSysex(sysexdata);
            sysexdata[3] = 0x26;
        btFix4.setMidiSysex(sysexdata);
            sysexdata[3] = 0x16;
        btFix5.setMidiSysex(sysexdata);
            sysexdata[3] = 0x06;
        btFix6.setMidiSysex(sysexdata);
        
        sysexdata[6] = 0x19;
        sysexdata[3] = 0x56;
        btPhase1.setMidiSysex(sysexdata);
        sysexdata[3] = 0x46;
        btPhase2.setMidiSysex(sysexdata);
        sysexdata[3] = 0x36;
        btPhase3.setMidiSysex(sysexdata);
        sysexdata[3] = 0x26;
        btPhase4.setMidiSysex(sysexdata);
        sysexdata[3] = 0x16;
        btPhase5.setMidiSysex(sysexdata);
        sysexdata[3] = 0x06;
        btPhase6.setMidiSysex(sysexdata);
  
        sysexdata[7] = 0x01;
        sysexdata[3] = 0x56;
        sliderPhase1.setMidiSysex(sysexdata);
        sysexdata[3] = 0x46;
        sliderPhase2.setMidiSysex(sysexdata);
        sysexdata[3] = 0x36;
        sliderPhase3.setMidiSysex(sysexdata);
        sysexdata[3] = 0x26;
        sliderFine4.setMidiSysex(sysexdata);
        sysexdata[3] = 0x16;
        sliderPhase5.setMidiSysex(sysexdata);
        sysexdata[3] = 0x06;
        sliderPhase6.setMidiSysex(sysexdata);
        sysexdata[7] = 0x00;
        
        sysexdata[6] = 0x26;
        sysexdata[3] = 0x56;
        sliderFine1.setMidiSysex(sysexdata);
        sysexdata[3] = 0x46;
        sliderFine2.setMidiSysex(sysexdata);
        sysexdata[3] = 0x36;
        sliderFine3.setMidiSysex(sysexdata);
        sysexdata[3] = 0x26;
        sliderFine4.setMidiSysex(sysexdata);
        sysexdata[3] = 0x16;
        sliderFine5.setMidiSysex(sysexdata);
        sysexdata[3] = 0x06;
        sliderFine6.setMidiSysex(sysexdata);
        
        sysexdata[6] = 0x1a;
        sysexdata[3] = 0x56;
        sliderDetune1.setMidiSysex(sysexdata);
        sysexdata[3] = 0x46;
        sliderDetune2.setMidiSysex(sysexdata);
        sysexdata[3] = 0x36;
        sliderDetune3.setMidiSysex(sysexdata);
        sysexdata[3] = 0x26;
        sliderDetune4.setMidiSysex(sysexdata);
        sysexdata[3] = 0x16;
        sliderDetune5.setMidiSysex(sysexdata);
        sysexdata[3] = 0x06;
        sliderDetune6.setMidiSysex(sysexdata);
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
        sliderFine1.setBoundsRelative(0.01f, 0.1f, 0.18f, 0.06f);
        sliderDetune1.setBoundsRelative(0.01f, 0.2f, 0.18f, 0.16f);
        sliderPhase1.setBoundsRelative(0.01f, 0.4f, 0.18f, 0.06f);
        
        roundSize(sliderOsc1);
        btFix1.setBoundsRelative(0.2f, 0.3f, 0.12f, 0.06f);
        btPhase1.setBoundsRelative(0.2f, 0.4f, 0.12f, 0.06f);
        
        groupOP2.setBoundsRelative(0.33f, 0, 0.33f, 0.5f);
        sliderOsc2.setBoundsRelative(0.53f, 0.04f, 0.12f, 1.0f);
        sliderFine2.setBoundsRelative(0.34f, 0.1f, 0.18f, 0.06f);
        sliderDetune2.setBoundsRelative(0.34f, 0.2f, 0.18f, 0.16f);
        sliderPhase2.setBoundsRelative(0.34f, 0.4f, 0.18f, 0.06f);
        
        roundSize(sliderOsc2);
        btFix2.setBoundsRelative(0.53f, 0.3f, 0.12f, 0.06f);
        btPhase2.setBoundsRelative(0.53f, 0.4f, 0.12f, 0.06f);
        
        groupOP3.setBoundsRelative(0.66f, 0, 0.33f, 0.5f);
        sliderOsc3.setBoundsRelative(0.86f, 0.04f, 0.12f, 1.0f);
        sliderFine3.setBoundsRelative(0.67f, 0.1f, 0.18f, 0.06f);
        sliderDetune3.setBoundsRelative(0.67f, 0.2f, 0.18f, 0.16f);
        sliderPhase3.setBoundsRelative(0.67f, 0.4f, 0.18f, 0.06f);
        
        roundSize(sliderOsc3);
        btFix3.setBoundsRelative(0.86f, 0.3f, 0.12f, 0.06f);
        btPhase3.setBoundsRelative(0.86f, 0.4f, 0.12f, 0.06f);
        
        groupOP4.setBoundsRelative(0, 0.5f, 0.33f, 0.5f);
        sliderOsc4.setBoundsRelative(0.2f, 0.54f, 0.12f, 1.0f);
        sliderFine4.setBoundsRelative(0.01f, 0.6f, 0.18f, 0.06f);
        sliderDetune4.setBoundsRelative(0.01f, 0.7f, 0.18f, 0.16f);
        sliderPhase4.setBoundsRelative(0.01f, 0.9f, 0.18f, 0.06f);
        
        roundSize(sliderOsc4);
        btFix4.setBoundsRelative(0.2f, 0.8f, 0.12f, 0.06f);
        btPhase4.setBoundsRelative(0.2f, 0.9f, 0.12f, 0.06f);
        
        groupOP5.setBoundsRelative(0.33f, 0.5f, 0.33f, 0.5f);
        sliderOsc5.setBoundsRelative(0.53f, 0.54f, 0.12f, 1.0f);
        sliderFine5.setBoundsRelative(0.34f, 0.6f, 0.18f, 0.06f);
        sliderDetune5.setBoundsRelative(0.34f, 0.7f, 0.18f, 0.16f);
        sliderPhase5.setBoundsRelative(0.34f, 0.9f, 0.18f, 0.06f);
        
        roundSize(sliderOsc5);
        btFix5.setBoundsRelative(0.53f, 0.8f, 0.12f, 0.06f);
        btPhase5.setBoundsRelative(0.53f, 0.9f, 0.12f, 0.06f);
        
        groupOP6.setBoundsRelative(0.66f, 0.5f, 0.33f, 0.5f);
        sliderOsc6.setBoundsRelative(0.86f, 0.54f, 0.12f, 1.0f);
        sliderFine6.setBoundsRelative(0.67f, 0.6f, 0.18f, 0.06f);
        sliderDetune6.setBoundsRelative(0.67f, 0.7f, 0.18f, 0.16f);
        sliderPhase6.setBoundsRelative(0.67f, 0.9f, 0.18f, 0.06f);
        roundSize(sliderOsc6);
        btFix6.setBoundsRelative(0.86f, 0.8f, 0.12f, 0.06f);
        btPhase6.setBoundsRelative(0.86f, 0.9f, 0.12f, 0.06f);
        
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
    
    MidiSlider sliderFine1;
    MidiSlider sliderDetune1;
    MidiSlider sliderPhase1;
    Label labelFine1 {"f1", "Coarse Fine"};
    Label labelDetune1 {"f1", "Detune"};
    Label labelPhase1 {"f1", "Phase"};
    
    MidiSlider sliderFine2;
    MidiSlider sliderDetune2;
    MidiSlider sliderPhase2;
    Label labelFine2 {"f1", "Coarse Fine"};
    Label labelDetune2 {"f1", "Detune"};
    Label labelPhase2 {"f1", "Phase"};
    
    MidiSlider sliderFine3;
    MidiSlider sliderDetune3;
    MidiSlider sliderPhase3;
    Label labelFine3 {"f1", "Coarse Fine"};
    Label labelDetune3 {"f1", "Detune"};
    Label labelPhase3 {"f1", "Phase"};
    
    MidiSlider sliderFine4;
    MidiSlider sliderDetune4;
    MidiSlider sliderPhase4;
    Label labelFine4 {"f1", "Coarse Fine"};
    Label labelDetune4 {"f1", "Detune"};
    Label labelPhase4 {"f1", "Phase"};
    
    MidiSlider sliderFine5;
    MidiSlider sliderDetune5;
    MidiSlider sliderPhase5;
    Label labelFine5 {"f1", "Coarse Fine"};
    Label labelDetune5 {"f1", "Detune"};
    Label labelPhase5 {"f1", "Phase"};
    
    MidiSlider sliderFine6;
    MidiSlider sliderDetune6;
    MidiSlider sliderPhase6;
    Label labelFine6 {"f1", "Coarse Fine"};
    Label labelDetune6 {"f1", "Detune"};
    Label labelPhase6 {"f1", "Phase"};
    
    MidiButton  btFix1;
    MidiButton  btFix2;
    MidiButton  btFix3;
    MidiButton  btFix4;
    MidiButton  btFix5;
    MidiButton  btFix6;
    
    MidiButton  btPhase1;
    MidiButton  btPhase2;
    MidiButton  btPhase3;
    MidiButton  btPhase4;
    MidiButton  btPhase5;
    MidiButton  btPhase6;
    
    Label labelOsc1 {"Op1", "Afm Osc"};
    
    CustomLookAndFeel myLookAndFeel;
    AfmOscLookAndFeel OscLook;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oscillator)
};
