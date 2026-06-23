/*
  ==============================================================================

    CommonFilter.h
    Created: 10 Feb 2019 2:54:18pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "EnvelopeDraw.h"

class CommonFilter    : public ElementComponent, public TextButton::Listener,
                        public Slider::Listener, public Value::Listener
{
public:
    CommonFilter()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
 
        
        addAndMakeVisible(labelFilter1);
        addAndMakeVisible(labelFilter2);
        addAndMakeVisible(groupSens);
        
        // add radio for the first filter
        radioFilter1Mode.addRadio("Thru",112);
        radioFilter1Mode.addRadio("LPF",112);
        radioFilter1Mode.addRadio("HPF",112);
        addAndMakeVisible(radioFilter1Mode);
        
        radioControlFiltre1.addRadio("EG", 113);
        radioControlFiltre1.addRadio("LFO", 113);
        radioControlFiltre1.addRadio("EG-VA", 113);
        addAndMakeVisible(radioControlFiltre1);
        
        radioControlFiltre2.addRadio("EG", 114);
        radioControlFiltre2.addRadio("LFO", 114);
        radioControlFiltre2.addRadio("EG-VA", 114);
        addAndMakeVisible(radioControlFiltre2);
        
        btFilter2.setTextOnOff("Filter2 LPF", "Filter2 TRUE");
        addAndMakeVisible(btFilter2);
        
        sliderFq1.setRangeAndRound(0, 127,0);
        sliderFq1.setSliderStyle(Slider::SliderStyle::Rotary);
        sliderFq1.setPopupDisplayEnabled(true, true, nullptr);
        sliderFq1.setTooltip("Adjust frequency of the first filter");
        addAndMakeVisible(sliderFq1);
        
        sliderFq2.setRangeAndRound(0, 127,0);
        sliderFq2.setSliderStyle(Slider::SliderStyle::Rotary);
        sliderFq2.setPopupDisplayEnabled(true, true, this);
        sliderFq2.setTooltip("Adjust frequency of the second filter");
        addAndMakeVisible(sliderFq2);
        
        sliderLfoSens.setRangeAndRound(-7,7,0);
        sliderLfoSens.setPopupDisplayEnabled(true, true, this);
        sliderLfoSens.setTooltip("Adjust Lfo depth filters");
        addAndMakeVisible(sliderLfoSens);
        
        sliderVelocity.setRangeAndRound(-7, 7,0);
        sliderVelocity.setPopupDisplayEnabled(true, true, this);
        sliderVelocity.setTooltip("Adjust velocity sens filters");
        addAndMakeVisible(sliderVelocity);
        
        sliderResonnance.setRangeAndRound(0, 127,0);
        sliderResonnance.setSliderStyle(Slider::SliderStyle::Rotary);
        sliderResonnance.setPopupDisplayEnabled(true, true, this);
        sliderResonnance.setTooltip("Adjust Resonnance of filters");
        addAndMakeVisible(sliderResonnance);
        
        
        addAndMakeVisible(labelLfoSens);
        labelLfoSens.setJustificationType(Justification::centred);
        labelLfoSens.attachToComponent(&sliderLfoSens, false);
        
        addAndMakeVisible(labelVelocity);
        labelVelocity.setJustificationType(Justification::centred);
        labelVelocity.attachToComponent(&sliderVelocity, false);
        
        addAndMakeVisible(labelResonnance);
        labelResonnance.setJustificationType(Justification::centred);
        labelResonnance.attachToComponent(&sliderResonnance, false);
        
        addAndMakeVisible(labelFq1);
        labelFq1.setJustificationType(Justification::centred);
        labelFq1.attachToComponent(&sliderFq1, false);
  
        addAndMakeVisible(labelFq2);
        labelFq2.setJustificationType(Justification::centred);
        labelFq2.attachToComponent(&sliderFq2, false);

        // mise à jour live du tracé du filtre
        sliderFq1.addListener (this);
        sliderFq2.addListener (this);
        sliderResonnance.addListener (this);

        // Édition à la souris directement sur le grand graphe de réponse (Filtre 1) :
        // glisser = cutoff (X) / résonance (Y). Les contrôles sont sous le graphe.
        addAndMakeVisible (filterGraph);
        filterGraph.onEdit = [this] (int cut, int res)
        {
            sliderFq1.setValue (cut);
            sliderResonnance.setValue (res);
            repaint();
        };
        radioFilter1Mode.getValueObject().addListener (this);
    }

    ~CommonFilter()
    {

    }
    void sliderValueChanged (Slider*) override { repaint(); }
    void valueChanged (Value&) override        { repaint(); }
    void setElementNumber ( int element, UndoManager& um) override
    {
        Logger::writeToLog("Common Filter set element number");
        int sysexdata2[9] = { 0x43, 0X10, 0x34, 0x09, 0x03, 0x00, 0x09, 0x00, 0x00 };
        int sysexdata[9] = { 0x43, 0X10, 0x34, 0x09, 0x03, 0x00, 0x00, 0x00, 0x00 };
        
        if(element==1)
        {
        Logger::writeToLog("Common Filter set element 1");
        btFilter2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1MODEFILTRE2, &um));
            
        radioFilter1Mode.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1MODEFILTRE1, &um));
        radioControlFiltre1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1CONTROLEURFILTRE1, &um));
        radioControlFiltre2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1CONTROLEURFILTRE2, &um));
        sliderFq1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1FQ1, &um));
        sliderFq2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1FQ2, &um));
        sliderVelocity.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1VELOCITYFILTRE, &um));
        sliderLfoSens.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1LFOFILTRE, &um));
        sliderResonnance.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1RESONNANCEFILTRE, &um));
   
        }
        else if(element==2)
        {
            sysexdata[4] = 0x20;
            sysexdata2[4] = 0x20;
            btFilter2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2MODEFILTRE2, &um));
            radioFilter1Mode.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2MODEFILTRE1, &um));
            radioControlFiltre1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2CONTROLEURFILTRE1, &um));
            radioControlFiltre2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2CONTROLEURFILTRE2, &um));
            sliderFq1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2FQ1, &um));
            sliderFq2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2FQ2, &um));
            sliderVelocity.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2VELOCITYFILTRE, &um));
            sliderLfoSens.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2LFOFILTRE, &um));
            sliderResonnance.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2RESONNANCEFILTRE, &um));
        }
        else if(element==3)
        {
            sysexdata[4] = 0x40;
            sysexdata2[4] = 0x40;
            
            btFilter2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3MODEFILTRE2, &um));
            radioFilter1Mode.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3MODEFILTRE1, &um));
            radioControlFiltre1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3CONTROLEURFILTRE1, &um));
            radioControlFiltre2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3CONTROLEURFILTRE2, &um));
            sliderFq1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3FQ1, &um));
            sliderFq2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3FQ2, &um));
            sliderVelocity.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3VELOCITYFILTRE, &um));
            sliderLfoSens.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3LFOFILTRE, &um));
            sliderResonnance.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3RESONNANCEFILTRE, &um));
        }
        else if(element==4)
        {
            sysexdata[4] = 0x60;
            sysexdata2[4] = 0x60;
            
            btFilter2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4MODEFILTRE2, &um));
            radioFilter1Mode.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4MODEFILTRE1, &um));
            radioControlFiltre1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4CONTROLEURFILTRE1, &um));
            radioControlFiltre2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4CONTROLEURFILTRE2, &um));
            sliderFq1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4FQ1, &um));
            sliderFq2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4FQ2, &um));
            sliderVelocity.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4VELOCITYFILTRE, &um));
            sliderLfoSens.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4LFOFILTRE, &um));
            sliderResonnance.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4RESONNANCEFILTRE, &um));
        }

        // Adresse filtre SY77 (CONFIRMÉE sur le synthé) : groupe 0x09,
        // addrHi = (élément-1)<<5 | (n°filtre-1) ; param 0x00 = mode, 0x01 = cutoff.
        const int elemBase = (element - 1) << 5;   // 0x00 / 0x20 / 0x40 / 0x60

        sysexdata[4] = elemBase;            // Filtre 1
        sysexdata[6] = 0x01;                // cutoff
        sliderFq1.setMidiSysex(sysexdata);
        // (Mode filtre 1 = AH elemBase, param 0x00 confirmé synthé ; MidiRadio n'a pas
        //  setMidiSysex -> à câbler autrement plus tard.)

        sysexdata[4] = elemBase | 0x01;     // Filtre 2
        sysexdata[6] = 0x00;                // mode (Thru/LPF/HPF)
        btFilter2.setMidiSysex(sysexdata);
        sysexdata[6] = 0x01;                // cutoff
        sliderFq2.setMidiSysex(sysexdata);
        
        // Sous-bloc "common" de l'élément : addrHi = elemBase | 0x02 (confirmé synthé via résonance).
        sysexdata[4] = elemBase | 0x02;
        sysexdata[6] = 0x32;                // résonance (CONFIRMÉ)
        sliderResonnance.setMidiSysex(sysexdata);
        sysexdata[6] = 0x33;                // vélocité (param supposé, addrHi confirmé)
        sliderVelocity.setMidiSysex(sysexdata);
        sysexdata[6] = 0x34;                // LFO (param supposé, addrHi confirmé)
        sliderLfoSens.setMidiSysex(sysexdata);

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
        juce::Rectangle<float> filterArea (bounds.getWidth() * 0.03f, bounds.getHeight() * 0.05f,
                                           bounds.getWidth() * 0.94f, bounds.getHeight() * 0.62f);

        int   mode   = radioFilter1Mode.getValue();              // 0 Thru, 1 LPF, 2 HPF
        float cutoff = (float) sliderFq1.getValue() / 127.0f;
        float reso   = (float) sliderResonnance.getValue() / 127.0f;

        auto modeName = mode == SyDraw::LPF ? "LPF" : (mode == SyDraw::HPF ? "HPF" : "Thru");
        String caption = String ("Filtre 1  ") + modeName
                       + "   FQ "  + String (roundToInt (sliderFq1.getValue()))
                       + "   Res " + String (roundToInt (sliderResonnance.getValue()));

        SyDraw::drawFilterResponse (g, filterArea, mode, cutoff, reso,
                                    SYColSelected, caption);
        
    }
    void buttonClicked (Button* button) override
    {
        setVisible(false);
    }
    
    void resized() override
    {
        // Overlay d'édition = même rectangle que la réponse dessinée dans paint().
        auto b = getLocalBounds().toFloat();
        filterGraph.setBounds (juce::Rectangle<float> (b.getWidth() * 0.03f, b.getHeight() * 0.05f,
                                                       b.getWidth() * 0.94f, b.getHeight() * 0.62f).toNearestInt());

        labelFilter1.setBoundsRelative(0.001f, 0.74f, 0.31f, 0.26f);
        radioFilter1Mode.setBoundsRelative(0.01f, 0.84f, 0.2f, 0.06f);
        radioControlFiltre1.setBoundsRelative(0.01f, 0.92f, 0.2f, 0.06f);
        sliderFq1.setBoundsRelative(0.19f,0.84f,0.12f,0.12f);
        
        labelFilter2.setBoundsRelative(0.33f, 0.74f, 0.31f, 0.26f);
        btFilter2.setBoundsRelative(0.34f,0.84f,0.2f,0.06f);
        radioControlFiltre2.setBoundsRelative(0.34f, 0.92f, 0.2f, 0.06f);
        sliderFq2.setBoundsRelative(0.52f,0.84f,0.12f,0.12f);
  
        groupSens.setBoundsRelative(0.66f,0.74f,0.33f,0.26f);
        sliderResonnance.setBoundsRelative(0.67f,0.84f,0.12f,0.12f);
        sliderVelocity.setBoundsRelative(0.77f,0.84f,0.12f,0.12f);
        sliderLfoSens.setBoundsRelative(0.87f,0.84f,0.12f,0.12f);
        
    }
    
private:
    

    
    GroupComponent   labelFilter1 {"test", "Filter1"};
    GroupComponent   labelFilter2 {"test", "Filter2"};
    GroupComponent   groupSens {"test", "Sensitivity"};
    
    Label   labelVelocity {"test", "Velocity"};
    Label   labelLfoSens {"test", "LFO"};
    Label   labelResonnance {"test", "Resonnance"};
    Label   labelFq1 {"test", "FQ Filter1"};
    Label   labelFq2 {"test", "FQ Filter2"};
    
    MidiRadio   radioFilter1Mode;
    MidiRadio   radioControlFiltre1;
    MidiRadio   radioControlFiltre2;
    MidiButton btFilter2;
    
    MidiSlider  sliderFq1;
    MidiSlider  sliderFq2;
    MidiSlider  sliderVelocity;
    MidiSlider  sliderLfoSens;
    MidiSlider sliderResonnance;

    FilterGraphView filterGraph; // édition souris du grand graphe (cutoff/résonance)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CommonFilter)
};
