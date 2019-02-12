/*
  ==============================================================================

    FilterVue.h
    Created: 29 Nov 2018 12:59:25am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ADSR.h"

//==============================================================================
/*
*/
class FilterVue    : public Component, public TextButton::Listener, Slider::Listener, private Timer, private Value::Listener, private ComboBox::Listener
{
public:
    FilterVue()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        addAndMakeVisible(btClose);
        btClose.addListener(this);
        addAndMakeVisible(keyDraw);
        addAndMakeVisible(samplePathLeft);
        addAndMakeVisible(samplePathRight);
        
        addAndMakeVisible(btEasyDraw);
        btEasyDraw.setColour(TextButton::ColourIds::buttonOnColourId, Colours::darkorange);
        btEasyDraw.setClickingTogglesState(true);
        btEasyDraw.addListener(this);
        
        EgFilter.setName("Filter EG ");
       // EgFilter.setRelease(1);

        addAndMakeVisible(EgFilter);
        
        
        addAndMakeVisible(labelFilter1);
        addAndMakeVisible(labelFq1);
        labelFilter1.setJustificationType(Justification::centred);
        labelFq1.setJustificationType(Justification::centred);
        labelFq1.attachToComponent(&sliderFq1, false);
        addAndMakeVisible(labelFilter2);
        addAndMakeVisible(labelFq2);
        labelFilter2.setJustificationType(Justification::centred);
        labelFq2.setJustificationType(Justification::centred);
        labelFq2.attachToComponent(&sliderFq2, false);
        
        addAndMakeVisible(labelLfoSens);
        labelLfoSens.setJustificationType(Justification::centred);
        labelLfoSens.attachToComponent(&sliderLfoSens, false);
        
        addAndMakeVisible(labelVelocity);
        labelVelocity.setJustificationType(Justification::centred);
        labelVelocity.attachToComponent(&sliderVelocity, false);
        
        addAndMakeVisible(labelResonnance);
        labelResonnance.setJustificationType(Justification::centred);
        labelResonnance.attachToComponent(&sliderResonnance, false);
        
        addAndMakeVisible(labelSlope);
        labelSlope.setJustificationType(Justification::centred);
        labelSlope.attachToComponent(&sliderSlope, true);
   
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
        
        startTimer(500);
 //       addKeyListener(this);
  
   

    }

    ~FilterVue()
    {
    //    removeKeyListener(this);
        stopTimer();
        btClose.removeListener(this);
        btEasyDraw.removeListener(this);
        sliderSlope.removeListener(this);
    }

    void valueChanged(Value & value) override
    {
        Logger::writeToLog("Filter ValueChanged");
    }
 /*
    bool keyPressed (const KeyPress& key,
                     Component* originatingComponent) override
    {
        if(key.getKeyCode() == 90)
        {
            if(key.getModifiers() == ModifierKeys::commandModifier)
                undoManager.undo();
            
            if(key.getModifiers() == ModifierKeys::shiftModifier + ModifierKeys::commandModifier)
                undoManager.redo();
        }
        repaint();
        return 0;
    }
  */
    void 	sliderValueChanged (Slider *slider) override
    {
        Logger::writeToLog("Filter::Slider value changed");
        
        if(slider == &sliderSlope)
        resized();
    }
    void timerCallback() override
    {
 //       undoManager.beginNewTransaction();
    }

    void setElementNumber ( int number, UndoManager& um)
    {
        btClose.setButtonText("Close Filter Vue OP: " + String(number));
        radioFilter1Mode.addRadio("Thru",112);
        radioFilter1Mode.addRadio("LPF",112);
        radioFilter1Mode.addRadio("HPF",112);
        addAndMakeVisible(radioFilter1Mode);
        
        radioControlFiltre1.addRadio("EG", 114);
        radioControlFiltre1.addRadio("LFO", 114);
        radioControlFiltre1.addRadio("EG-VA", 114);
        addAndMakeVisible(radioControlFiltre1);
        
        radioControlFiltre2.addRadio("EG", 114);
        radioControlFiltre2.addRadio("LFO", 114);
        radioControlFiltre2.addRadio("EG-VA", 114);
        addAndMakeVisible(radioControlFiltre2);
        
 //       Value valueCommonVolume = valueTreeVoice.getPropertyAsValue(IDs::COMMONVOLUME.toString(), &undoManager);
 //       sliderMaster.getValueObject().referTo(valueCommonVolume);

        
        intElement = number;
        // Need to initialise here if we wanna have the element number
        int sysexdata2[9] = { 0x43, 0X10, 0x34, 0x09, 0x03, 0x00, 0x09, 0x00, 0x00 };
        int sysexdata[9] = { 0x43, 0X10, 0x34, 0x09, 0x03, 0x00, 0x00, 0x00, 0x00 };
        if(intElement == 1)
        {
        pathFilter1.addPath(myPath);
        btFilter2.getToggleStateValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1MODEFILTRE2, &um));

            
            radioFilter1Mode.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1MODEFILTRE1, &um));
            
        radioControlFiltre1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1CONTROLEURFILTRE1, &um));
        radioControlFiltre2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1CONTROLEURFILTRE2, &um));
        sliderFq1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1FQ1, &um));
        sliderFq2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1FQ2, &um));
        sliderVelocity.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1VELOCITYFILTRE, &um));
        sliderLfoSens.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1LFOFILTRE, &um));
        sliderResonnance.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1RESONNANCEFILTRE, &um));
        sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1SLOPEFILTRE, &um));
        
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRELEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRELEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRELEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRELEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRELEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRERL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRERL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRER1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRER2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRER3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRER4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRERR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRERR2 , &um));
            
        }
        if(intElement == 2)
        {
         
            pathFilter2.addPath(myPath);

            comboCtrl1.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2CONTROLEURFILTRE1, &um));
            comboCtrl2.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2CONTROLEURFILTRE2, &um));
            sliderFq1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2FQ1, &um));
            sliderFq2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2FQ2, &um));
            sliderVelocity.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2VELOCITYFILTRE, &um));
            sliderLfoSens.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2LFOFILTRE, &um));
            sliderResonnance.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2RESONNANCEFILTRE, &um));
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2SLOPEFILTRE, &um));
            
            sysexdata[4] += 0x20;
              sysexdata2[4] += 0x20;
        }
        if(intElement == 3)
        {
            pathFilter3.addPath(myPath);
            
            comboCtrl1.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3CONTROLEURFILTRE1, &um));
            comboCtrl2.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3CONTROLEURFILTRE2, &um));
            sliderFq1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3FQ1, &um));
            sliderFq2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3FQ2, &um));
            sliderVelocity.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3VELOCITYFILTRE, &um));
            sliderLfoSens.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3LFOFILTRE, &um));
            sliderResonnance.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3RESONNANCEFILTRE, &um));
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3SLOPEFILTRE, &um));
            
            sysexdata[4] += 0x40;
             sysexdata2[4] += 0x40;
        }
        if(intElement == 4)
        {
               pathFilter4.addPath(myPath);

            comboCtrl1.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4CONTROLEURFILTRE1, &um));
            comboCtrl2.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4CONTROLEURFILTRE2, &um));
            sliderFq1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4FQ1, &um));
            sliderFq2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4FQ2, &um));
            sliderVelocity.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4VELOCITYFILTRE, &um));
            sliderLfoSens.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4LFOFILTRE, &um));
            sliderResonnance.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4RESONNANCEFILTRE, &um));
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4SLOPEFILTRE, &um));
            
            sysexdata[4] += 0x60;
             sysexdata2[4] += 0x60;
        }
 
        sliderFilter1Mode.setRangeAndRound(0, 2, 0);
        sliderFilter1Mode.setMidiSysex(sysexdata);
        sliderFilter1Mode.setSliderStyle(Slider::SliderStyle::Rotary);
        addAndMakeVisible(sliderFilter1Mode);
        
      

        
  
   
        sysexdata[6] = 0x01;
        sliderFq1.setMidiSysex(sysexdata);
        sliderFq1.setRangeAndRound(0, 127,0);
        sliderFq1.setPopupDisplayEnabled(true, true, nullptr);
        sliderFq1.setTooltip("Adjust frequency of the first filter");
  
        
        addAndMakeVisible(sliderFq1);
        
        sysexdata[6] = 0x02;
        comboCtrl1.setMidiSysex(sysexdata);
        comboCtrl1.addItem("Ctrl EG", 1);
        comboCtrl1.addItem("Ctrl LFO", 2);
        comboCtrl1.addItem("Ctrl EG-VA", 3);
        comboCtrl1.addListener(this);
        comboCtrl1.setSelectedId(1);
        addAndMakeVisible(comboCtrl1);
        
        sliderFilter1Control.setMidiSysex(sysexdata);
        sliderFilter1Control.setRangeAndRound(0, 2, 0);
        addAndMakeVisible(sliderFilter1Control);
    
        
        sysexdata[6] = 0x10;
        sliderSlope.setMidiSysex(sysexdata);
        sliderSlope.setRangeAndRound(-7,7,0);
        sliderSlope.setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(sliderSlope);
        sliderSlope.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
     
        sliderSlope.addListener(this);
        
        sysexdata[4] = 0x04;
        sysexdata[6] = 0x00;
        btFilter2.setMidiSysex(sysexdata);
        btFilter2.setTextOnOff("Filter2 LPF", "Filter2 TRUE");
     
        addAndMakeVisible(btFilter2);
      
        sysexdata[6] = 0x01;
        sliderFq2.setMidiSysex(sysexdata);
        sliderFq2.setRangeAndRound(0, 127,0);
        sliderFq2.setPopupDisplayEnabled(true, true, this);
        sliderFq2.setTooltip("Adjust frequency of the second filter");
     
        addAndMakeVisible(sliderFq2);
        
        sysexdata[6] = 0x02;
        comboCtrl2.setMidiSysex(sysexdata);
        comboCtrl2.addItem("Ctrl EG", 1);
        comboCtrl2.addItem("Ctrl LFO", 2);
        comboCtrl2.addItem("Ctrl EG-VA", 3);
        comboCtrl2.setSelectedId(1);
        addAndMakeVisible(comboCtrl2);
        
        sysexdata[4] = 0x05;
        sysexdata[6] = 0x32;
        sliderResonnance.setMidiSysex(sysexdata);
        sliderResonnance.setRangeAndRound(0, 127,0);
        sliderResonnance.setPopupDisplayEnabled(true, true, this);
        sliderResonnance.setTooltip("Adjust Resonnance of filters");
  
        addAndMakeVisible(sliderResonnance);
        
        sysexdata[6] = 0x33;
        sliderVelocity.setMidiSysex(sysexdata);
        sliderVelocity.setRangeAndRound(-7, 7,0);
        sliderVelocity.setPopupDisplayEnabled(true, true, this);
        sliderVelocity.setTooltip("Adjust velocity sens filters");
    
        addAndMakeVisible(sliderVelocity);
        
        sysexdata[6] = 0x34;
        sliderLfoSens.setMidiSysex(sysexdata);
        sliderLfoSens.setRangeAndRound(-7,7,0);
        sliderLfoSens.setPopupDisplayEnabled(true, true, this);
        sliderLfoSens.setTooltip("Adjust Lfo depth filters");
    
        addAndMakeVisible(sliderLfoSens);
        
        sysexdata[4] = 0x03;
        sysexdata[6] = 0x03;
        EgFilter.addSegment(10, 10, "L0R1", 64, sysexdata, 64, sysexdata2);
        sliderL0.setMidiSysex(sysexdata);
        sliderL0.setRangeAndRound(0, 64, 0);
        sliderR1.setMidiSysex(sysexdata2);
        sliderR1.setRangeAndRound(0, 64, 0);
        
        sysexdata[6] = 0x04;
        sysexdata2[6] = 0x0a;
        EgFilter.addSegment(10, 10, "L1R2", 64, sysexdata, 64, sysexdata2);
        sliderL1.setMidiSysex(sysexdata);
        sliderL1.setRangeAndRound(0, 64, 0);
        sliderR2.setMidiSysex(sysexdata2);
        sliderR2.setRangeAndRound(0, 64, 0);
        
        sysexdata[6] = 0x05;
        sysexdata2[6] = 0x0b;
        EgFilter.addSegment(10, 10, "L2R3", 64, sysexdata, 64, sysexdata2);
        sliderL2.setMidiSysex(sysexdata);
        sliderL2.setRangeAndRound(0, 64, 0);
        sliderR3.setMidiSysex(sysexdata2);
        sliderR3.setRangeAndRound(0, 64, 0);
        
        sysexdata[6] = 0x06;
        sysexdata2[6] = 0x0c;
        EgFilter.addSegment(10, 10, "L3R4", 64, sysexdata, 64, sysexdata2);
        sliderL3.setMidiSysex(sysexdata);
        sliderL3.setRangeAndRound(0, 64, 0);
        sliderR4.setMidiSysex(sysexdata2);
        sliderR4.setRangeAndRound(0, 64, 0);
        
        sysexdata[6] = 0x07;
        sysexdata2[6] = 0x0d;
        EgFilter.addSegment(10, 10, "L4RR1", 64, sysexdata, 64, sysexdata2);
        sliderL4.setMidiSysex(sysexdata);
        sliderL4.setRangeAndRound(0, 64, 0);
        sliderRR1.setMidiSysex(sysexdata2);
        sliderRR2.setRangeAndRound(0, 64, 0);
    
    }
    void comboBoxChanged	(	ComboBox * 	comboBoxThatHasChanged	) override
    {
        Logger::writeToLog("Filter comboBox Changed");
    }
    void buttonClicked (Button* button) override
    {
        if(button == &btEasyDraw)
            EgFilter.setEasyDraw(btEasyDraw.getToggleState());
        if(button==&btClose)
        setVisible(false);
    }
    Path getPath() // fonction to get the graff filter path
    {
        return myPath;
    }
    void resized() override
    {
        
        //  const float fGrid = 1.0f/4;
        btClose.setBounds(10, 4, getWidth()-70, 20);
        btEasyDraw.setBounds(getWidth()-68, 4, 58, 20);
        labelFilter1.setBoundsRelative(0.01f, 0.05f, 0.2f, 0.06f);
        labelFilter2.setBoundsRelative(0.22f,0.05f,0.2f,0.06f);
  
        radioFilter1Mode.setBoundsRelative(0.01f, 0.1f, 0.2f, 0.06f);
        radioControlFiltre1.setBoundsRelative(0.01f, 0.17f, 0.2f, 0.06f);
        radioControlFiltre2.setBoundsRelative(0.22f, 0.17f, 0.2f, 0.06f);
      //sliderFilter1Control.setBoundsRelative(0.11f, 0.08f, 0.1f, 0.1f);
        btFilter2.setBoundsRelative(0.22f,0.1f,0.2f,0.06f);
        
        //    comboCtrl1.setBoundsRelative(0.01f,0.15f,0.23f,0.06f);
    //    comboCtrl2.setBoundsRelative(0.25f,0.15f,0.23f,0.06f);
        
        EgFilter.setBoundsRelative(0.46f, 0.08f, 0.49f, 0.4f);
  
        
        sliderFq1.setBoundsRelative(0.01f,0.30f,0.1f,0.4f);
        sliderFq2.setBoundsRelative(0.11f,0.30f,0.1f,0.4f);
        sliderResonnance.setBoundsRelative(0.21f,0.3f,0.1f,0.4f);
        
        sliderVelocity.setBoundsRelative(0.32f,0.3f,0.11f,0.11f);
        sliderLfoSens.setBoundsRelative(0.32f,0.52f,0.11f,0.11f);
        
        sliderL0.setBoundsRelative(0.46f, 0.56, 0.03f, 0.18f);
        sliderL1.setBoundsRelative(0.50f, 0.56, 0.03f, 0.18f);
        sliderL2.setBoundsRelative(0.54f, 0.56, 0.03f, 0.18f);
        sliderL3.setBoundsRelative(0.58f, 0.56, 0.03f, 0.18f);
        sliderL4.setBoundsRelative(0.62f, 0.56, 0.03f, 0.18f);
        sliderRL1.setBoundsRelative(0.66f, 0.56, 0.03f, 0.18f);
        sliderRL2.setBoundsRelative(0.7f, 0.56, 0.03f, 0.18f);
        sliderR1.setBoundsRelative(0.74f, 0.56, 0.03f, 0.18f);
          sliderR2.setBoundsRelative(0.78f, 0.56, 0.03f, 0.18f);
          sliderR3.setBoundsRelative(0.82f, 0.56, 0.03f, 0.18f);
          sliderR4.setBoundsRelative(0.86f, 0.56, 0.03f, 0.18f);
          sliderRR1.setBoundsRelative(0.9f, 0.56, 0.03f, 0.18f);
          sliderRR2.setBoundsRelative(0.94f, 0.56, 0.03f, 0.18f);
        keyDraw.setBoundsRelative(0.0f, 0.85f, 1.0f, 0.09f);
        float sampleWidth = 0.2f + (sliderSlope.getValue()/100);
        samplePathLeft.setBoundsRelative(0.0f, 0.93f, sampleWidth, 0.09f);
        sampleWidth = 0.2f - (sliderSlope.getValue()/100);
        samplePathRight.setBoundsRelative(1.0f - sampleWidth, 0.93f, sampleWidth, 0.09f);
        
        sliderSlope.setBoundsRelative(0.4f, 0.92f, 0.2f, 0.1f);
        // This method is where you should set the bounds of any child
        // components that your component contains..
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background



    }

private:
    int intElement;
  
    Label   labelFilter1 {"test", "Filter1"};
    Label   labelFilter2 {"test", "Filter2"};
    Label   labelFq1 {"test", "FQ Filter1"};
    Label   labelFq2 {"test", "FQ Filter2"};
    Label   labelVelocity {"test", "Velocity Sens"};
    Label   labelLfoSens {"test", "LFO Sens"};
    Label   labelResonnance {"test", "Resonnance"};
    Label   labelSlope {"test", "Slope"};
    
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
    
    

    MidiSlider  sliderFilter1Mode;
    MidiSlider  sliderFilter1Control;
    MidiRadio   radioFilter1Mode;
    MidiRadio   radioControlFiltre1;
    MidiRadio   radioControlFiltre2;
    
    
    MidiCombo   comboCtrl1;
    MidiCombo   comboCtrl2;
    MidiSlider  sliderFq1;
    MidiSlider  sliderFq2;
    MidiSlider  sliderVelocity;
    MidiSlider  sliderLfoSens;
    MidiSlider sliderResonnance;
    MidiKeyDraw keyDraw;
    
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
    
    
    MidiSlider  sliderSlope;
    MidiPath    samplePathLeft;
    MidiPath    samplePathRight;
    SADSR EgFilter;
    MidiButton btFilter2;
    TextButton btClose {"Close"};
    TextButton btEasyDraw{"Easy Draw","Easy Draw"};
    int intFilter1;
    int intFilter2;
    int intResonnance;

    
 //   UndoManager undoManager;
    Path myPath;    // create the path filter
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterVue)
};
