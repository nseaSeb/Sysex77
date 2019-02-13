/*
  ==============================================================================

    Element.h
    Created: 24 Nov 2018 1:05:52pm
    Author:  Sébastien Portrait

  ==============================================================================
*/
#include "Pitch.h"
#pragma once
#include "Oscillator.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "LookAndFeel.h"

#include "WaveVue.h"
#include "FilterVue.h"
#include "AfmVue.h"



//==============================================================================
/*
*/
class Element    : public Component, TextButton::Listener, public ChangeBroadcaster, public Slider::Listener, public Value::Listener,public ChangeListener
{
public:
    Element()
    {
     
        addAndMakeSlider(sliderVolume);
        sliderVolume.setColour(Slider::ColourIds::thumbColourId, Colours::darkorange);
        sliderVolume.setRange(0, 100);
        sliderVolume.setPopupDisplayEnabled(true, true, this);
        sliderVolume.setNumDecimalPlacesToDisplay(0);
        sliderVolume.addListener(this);
        addAndMakeSlider(sliderPan);
        sliderPan.setLookAndFeel(&myLookAndFeel);
        sliderPan.setPopupDisplayEnabled(true, true,this);
       
        addAndMakeVisible(pitch);
        sliderPan.setRange(-64, 64);
         sliderPan.setNumDecimalPlacesToDisplay(0);
        sliderPan.addMouseListener(this, false);
        
        addAndMakeVisible(btGroup1);
        addAndMakeVisible(btGroup2);

        addAndMakeVisible(groupWave);
        addAndMakeVisible(groupFilter);
        addAndMakeVisible(groupVolume);
        addAndMakeVisible(groupPan);
        addAndMakeVisible(btWave);
        /*
        Path shape;
        shape.lineTo(0.0f, 1.0f);
        shape.lineTo(0.2f,0.1f);
        shape.lineTo(0.6f,0.1f);
        shape.lineTo(1.0f,1.0f);
        shape.lineTo(0.0f,1.0f);
        pathFilter1.addPath(shape);
*/
        addAndMakeVisible(btFilter);
        
        addAndMakeVisible(btVCA);

        btWave.addListener(this);
        btVCA.addListener(this);
        btFilter.addListener(this);
    
        
  

        imgAudio =  ImageFileFormat::loadFrom(BinaryData::Audio_png, (size_t) BinaryData::Audio_pngSize);
        imgAFM = ImageFileFormat::loadFrom(BinaryData::AFM_png,(size_t) BinaryData::AFM_pngSize);
        imgFilter = ImageFileFormat::loadFrom(BinaryData::Filter_png,(size_t) BinaryData::Filter_pngSize);
        imgVCA = ImageFileFormat::loadFrom(BinaryData::VCA_png,(size_t) BinaryData::VCA_pngSize);
        btFilter.setImages (false, true, true,
                          imgFilter, 0.7f, Colours::transparentBlack,
                          imgFilter, 1.0f, Colours::transparentBlack,
                          imgFilter, 0.6f, Colours::transparentBlack,
                          0.0f);
   
    
  
   btVCA.setImages (false, true, true,
                          imgVCA, 0.7f, Colours::transparentBlack,
                          imgVCA, 1.0f, Colours::transparentBlack,
                          imgVCA, 0.6f, Colours::transparentBlack,
                          0.0f);
        
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.

    }

    ~Element()
    {
        btWave.removeListener(this);
        btVCA.removeListener(this);
        btFilter.removeListener(this);
        sliderPan.setLookAndFeel(nullptr);
        sliderPan.removeMouseListener(this);
        sliderVolume.removeListener(this);

    }
    void addAndMakeSlider (Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setRange(0, 127);
        
    }
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        
        
        Logger::writeToLog("Element: changeListener");
        //    Logger::writeToLog(source);
        //    test.setVisible(true);
        
    }
    
    void mouseUp (const MouseEvent& e) override
    {
        
        if(e.getNumberOfClicks()>1) // double clic
        {
            if(e.eventComponent == &sliderPan)
                sliderPan.setValue(0);
        }
            
    }
    void valueChanged(Value & value) override
    {
        Logger::writeToLog("Element value change");
/*        if(operatorID == 1)
            sliderVolume.setValue(intVolumeOP1);
  
        if(operatorID == 2)
                  sliderVolume.setValue(intVolumeOP2);
        if(operatorID == 3)
                 sliderVolume.setValue(intVolumeOP3);
        if(operatorID == 4)
             sliderVolume.setValue(intVolumeOP4);
  */
    }
    void sliderValueChanged (Slider* slider) override
    {
 
    }
    void buttonClicked (Button* button) override
    {
    
        if(button == &btWave)
        {
            Logger::writeToLog("Bouton wave clicked");
           elementValue.setValue(0);
            if(operatorMode == mode::AWM)
            elementValue.setValue(commande::WaveEdit) ;
            
            else
            {
                elementValue.setValue(commande::AfmEdit);
            }
        }
        if(button == &btFilter)
            elementValue.setValue(commande::FilterEdit);
        
        if (button == &btVCA)
        {
            if(operatorMode == mode::AWM)
            {
            elementValue.setValue(commande::VolumeEdit);
            }
            else
            {
            elementValue.setValue(commande::VolumeAFM);
            }
        }
    
  //      sendChangeMessage();
  
    }
    void setOpNumber (int operatorNumber, UndoManager& undoManager)
    {
        Logger::writeToLog(String(operatorNumber));
        operatorID = operatorNumber;
        pitch.setOperator(operatorNumber, undoManager);
        if(operatorID == 1)
        {
//            btFilter.setShape(pathFilter1, false, false, true);
            Value valueElement1Volume = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1VOLUME.toString(), &undoManager);
            sliderVolume.getValueObject().referTo(valueElement1Volume);
            int data[9] = { 0x43, 0X10, 0x34, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 };
            sliderVolume.setMidiSysex(data);
        
        }
        if(operatorID == 2)
        {
//            btFilter.setShape(pathFilter2, false, false, true);
            Value valueElement2Volume = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2VOLUME.toString(), &undoManager);
            sliderVolume.getValueObject().referTo(valueElement2Volume);
            int data[9] = { 0x43, 0X10, 0x34, 0x03, 0x20, 0x00, 0x00, 0x00, 0x00 };
            sliderVolume.setMidiSysex(data);
        }
        if(operatorID == 3)
        {
//            btFilter.setShape(pathFilter3, true, false, true);
            Value valueElement3Volume = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3VOLUME.toString(), &undoManager);
            sliderVolume.getValueObject().referTo(valueElement3Volume);
            int data[9] = { 0x43, 0X10, 0x34, 0x03, 0x40, 0x00, 0x00, 0x00, 0x00 };
            sliderVolume.setMidiSysex(data);
        }
        if(operatorID == 4)
        {
 //           btFilter.setShape(pathFilter4, false, false, true);
            Value valueElement4Volume = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4VOLUME.toString(), &undoManager);
            sliderVolume.getValueObject().referTo(valueElement4Volume);
            int data[9] = { 0x43, 0X10, 0x34, 0x03, 0x60, 0x00, 0x00, 0x00, 0x00 };
            sliderVolume.setMidiSysex(data);
        }
    }

   int getOpNumber ()
    {
        return operatorID;
    }
    void setOpMode (int mode)
    {
        operatorMode = mode;
    }
    int getOpMode ()
    {
        return operatorMode;
    }
    void setWaveMode()
    {
                btWave.setButtonText("Wave");
 
        btWave.setImages (false, true, true,
                         imgAudio, 0.7f, Colours::transparentBlack,
                         imgAudio, 1.0f, Colours::transparentBlack,
                          imgAudio, 0.6f, Colours::transparentBlack,
                         0.0f);
        repaint();
    }
    void setAfmMode()
    {
        btWave.setButtonText("AFM");
        btWave.setButtonText("AFM");
        
        btWave.setImages (false, true, true,
                          imgAFM, 0.7f, Colours::transparentBlack,
                          imgAFM, 1.0f, Colours::transparentBlack,
                          imgAFM, 0.6f, Colours::transparentBlack,
                          0.0f);
        repaint();
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
           draws some placeholder text to get you started.

           You should replace everything in this method with your own
           drawing code..
        */

//        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background

        g.setColour (Colour(0xffffffff));
        g.addTransform(AffineTransform::rotation(fAngle));
        String str = "OP" + String(operatorID) + " ";
        if(operatorMode == 1)
        {
            str = str + "AFM Mono";
            setAfmMode();
        }
        if(operatorMode == 2)
        {
             str = str + "AFM Poly";
            setAfmMode();
        }
        if(operatorMode == 3)
        {
            str = str + "AWM";
            setWaveMode();
        }
        if(operatorMode == 4)
        {
            str = str + "DRUM Set";
        }
        g.drawText( str, -getHeight(), 0, getHeight(), 24, Justification::centred);
        
//        Rect localRect = getLocalBounds();   // draw an outline around the component
  //      g.fillRoundedRectangle (0, 0, getWidth(),getHeight(), 20);
     
    }

    void resized() override
    {
        int wGrid = getWidth()/4;
        int wwGrid = wGrid/2;
//        int hGrid = getHeight()/5;
   
        // This method is where you should set the bounds of any child
        // components that your component contains..

        pitch.setBounds(30,0,wGrid - 30,getHeight());
   //     sliderPitch.setBounds(10, 20, wGrid -20, hGrid);
    //    sliderFine.setBounds(10, hGrid + 30, wGrid - 20, hGrid);
        
        groupWave.setBounds(wGrid, 0, wwGrid, getHeight());
        btWave.setBounds(groupWave.getBounds().reduced(4, 16));
        
        groupFilter.setBounds(wGrid + wwGrid, 0, wGrid, getHeight());
        
        btFilter.setBounds(groupFilter.getBounds().reduced(4, 16));
        
        groupVolume.setBounds(wwGrid * 5, 0, wGrid, getHeight());
        btVCA.setBounds(wwGrid * 5 + 4, 4, wGrid - 34, getHeight() -8);
        sliderVolume.setBounds(wwGrid * 7 - 30, 16, 24, getHeight() - 24);
        
        groupPan.setBounds(wwGrid * 7 , 0, wwGrid, getHeight());
        sliderPan.setBounds(4 + wwGrid * 7, (getHeight()/2)-(wwGrid/2),wwGrid - 8, wwGrid);
        

        
    }
enum mode
    {
        AFMmono = 1,
        AFMPoly = 2,
        AWM = 3,
        DrumSet = 4
    };
enum commande
    {
        AfmEdit = 1,
        WaveEdit = 3,
        DrumEdit = 4,
        FilterEdit = 5,
        VolumeEdit = 6,
        PanEdit = 7,
        VolumeAFM =8
    };
public:
        Value elementValue;
private:
    int operatorID;
    int operatorMode;
    
    Image imgAudio;
    Image imgAFM;
    Image imgFilter;
    Image imgVCA;
    Pitch pitch;
    ImageButton btWave;
    ImageButton btFilter; //{"Filter",Colours::darkorange,Colours::white,Colours::orange };
    ImageButton btVCA;
//    Slider sliderPitch {Slider::SliderStyle::Rotary , Slider::NoTextBox};
 //   Slider sliderFine {Slider::SliderStyle::LinearBar,Slider::NoTextBox};
    MidiSlider sliderVolume;
    Slider sliderPan {Slider::SliderStyle::Rotary , Slider::NoTextBox};
    TextButton  btGroup1 {"1"};
    TextButton btGroup2 {"2"};

    GroupComponent groupWave{"","Wave"};
    GroupComponent groupFilter{"",TRANS("Filter")};
    GroupComponent groupVolume{"","Volume"};
    GroupComponent groupPan{"","Pan"};
    float fAngle = -90 * (float_Pi / 180.0);
         CustomLookAndFeel myLookAndFeel;
   
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Element)
};
