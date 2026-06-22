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
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "EnvelopeDraw.h"

#include "WaveVue.h"
#include "FilterVue.h"
#include "AfmVue.h"
#include "AlgoDraw.h"



//==============================================================================
/*
*/
class Element    : public Component, TextButton::Listener, public ChangeBroadcaster, public Slider::Listener, public Value::Listener,public ChangeListener
{
public:
    Element()
    {
     
        addAndMakeSlider(sliderVolume);
        sliderVolume.setColour(Slider::ColourIds::thumbColourId, SYColSelected);
        sliderVolume.setRange(0, 100);
        sliderVolume.setPopupDisplayEnabled(true, true, this);
        sliderVolume.setNumDecimalPlacesToDisplay(0);
        sliderVolume.addListener(this);
        addAndMakeSlider(sliderPan);
        sliderPan.setLookAndFeel(nullptr);   // suit le LnF du thème (plat en Light)
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
        // Mini-vue de l'algo FM par-dessus le bouton WAVE (n'intercepte pas la souris :
        // le bouton dessous reste cliquable pour ouvrir l'éditeur d'opérateurs).
        addAndMakeVisible(elementAlgo);
        elementAlgo.setInterceptsMouseClicks(false, false);
        elementAlgo.setVisible(false);
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
        // L'emplacement Filter affiche désormais une vignette (réponse du filtre)
        // dessinée dans paint(). Le bouton reste cliquable (seuil hit-test = 0).
        btFilter.setImages (false, true, true,
                          Image(), 0.0f, Colours::transparentBlack,
                          Image(), 0.0f, Colours::transparentBlack,
                          Image(), 0.0f, Colours::transparentBlack,
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
        if (value.refersToSameSourceAs (algoValue))
            elementAlgo.setAlgo (jmax (1, (int) algoValue.getValue()));
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

        // Lien vers l'algo FM de cet élément -> mini-vue (mise à jour live).
        algoValue = valueTreeVoice.getPropertyAsValue (Identifier ("AFMALGOELEMENT" + String (operatorID)), &undoManager);
        algoValue.addListener (this);
        elementAlgo.setAlgo (jmax (1, (int) algoValue.getValue()));
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
        elementAlgo.setVisible (false);   // AWM : pas d'algo FM
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
        elementAlgo.setVisible (true);    // AFM : montre l'algo choisi
        elementAlgo.setAlgo (jmax (1, (int) algoValue.getValue()));
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

        // Vignette : réponse du filtre de cet élément (lue depuis valueTreeVoice)
        {
            auto idFor = [this] (const char* suffix)
            { return Identifier (String ("ELEMENT") + String (operatorID) + suffix); };

            int   fMode = (int) valueTreeVoice.getProperty (idFor ("MODEFILTRE1"), 0);
            float fCut  = (float) (int) valueTreeVoice.getProperty (idFor ("FQ1"), 0) / 127.0f;
            float fRes  = (float) (int) valueTreeVoice.getProperty (idFor ("RESONNANCEFILTRE"), 0) / 127.0f;

            auto fb = groupFilter.getBounds().toFloat().reduced (5.0f, 14.0f);
            if (fb.getWidth() > 12.0f && fb.getHeight() > 12.0f)
                SyDraw::drawFilterResponse (g, fb, fMode, fCut, fRes, SYColSelected);
        }

        // Deux sorties du pan (L / R) sur le bord droit de l'élément : point de départ
        // du routage vers la matrice (à connecter ensuite).
        {
            auto pb = groupPan.getBounds().toFloat();
            const float ox = (float) getWidth() - 4.0f;
            const float lY = pb.getY() + pb.getHeight() * 0.32f;
            const float rY = pb.getY() + pb.getHeight() * 0.68f;
            g.setColour (SYColSelected);
            g.fillEllipse (ox - 3.0f, lY - 3.0f, 6.0f, 6.0f);
            g.fillEllipse (ox - 3.0f, rY - 3.0f, 6.0f, 6.0f);
            g.drawLine (sliderPan.getRight(), lY, ox - 3.0f, lY, 1.0f);
            g.drawLine (sliderPan.getRight(), rY, ox - 3.0f, rY, 1.0f);
            g.setColour (SYColLabel);
            g.setFont (10.0f);
            g.drawText ("L", (int) ox - 30, (int) lY - 7, 14, 12, Justification::right);
            g.drawText ("R", (int) ox - 30, (int) rY - 7, 14, 12, Justification::right);
        }

        g.setColour (SYColBackground.contrasting());
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
        // Colonnes homogènes (mêmes fractions que les en-têtes de VoicePage) :
        // PITCH .18 | WAVE .18 | FILTER .26 | VOLUME .26 | PAN .12
        const float W = (float) getWidth();
        const int   H = getHeight();
        auto col = [W] (float f) { return (int) (W * f); };
        const int xPitch = col (0.0f),  xWave = col (0.18f), xFilter = col (0.36f),
                  xVol   = col (0.62f), xPan  = col (0.88f), xEnd   = col (1.0f);

        const int labelMargin = 22; // place pour le libellé vertical "OPx ..." à gauche
        pitch.setBounds (xPitch + labelMargin, 0, (xWave - xPitch) - labelMargin - 2, H);

        groupWave.setBounds (xWave, 0, xFilter - xWave, H);
        btWave.setBounds (groupWave.getBounds().reduced (4, 16));
        elementAlgo.setBounds (btWave.getBounds().reduced (2)); // mini-algo par-dessus le bouton

        groupFilter.setBounds (xFilter, 0, xVol - xFilter, H);
        btFilter.setBounds (groupFilter.getBounds().reduced (4, 16));

        const int volW = xPan - xVol;
        groupVolume.setBounds (xVol, 0, volW, H);
        btVCA.setBounds (xVol + 4, 4, volW - 34, H - 8);
        sliderVolume.setBounds (xPan - 30, 16, 24, H - 24);

        const int panW = xEnd - xPan;
        groupPan.setBounds (xPan, 0, panW, H);
        // Pan vertical (façon SynthWorks) ; les 2 sorties L/R sont dessinées à droite (paint).
        sliderPan.setBounds (xPan + 6, 16, 16, H - 28);
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
    ImageButton btFilter; //{"Filter",SYColSelected,Colours::white,Colours::orange };
    ImageButton btVCA;
//    Slider sliderPitch {Slider::SliderStyle::Rotary , Slider::NoTextBox};
 //   Slider sliderFine {Slider::SliderStyle::LinearBar,Slider::NoTextBox};
    MidiSlider sliderVolume;
    AlgoDraw elementAlgo;   // mini-vue de l'algo FM (colonne WAVE, mode AFM)
    Value    algoValue;     // -> AFMALGOELEMENTx
    Slider sliderPan {Slider::SliderStyle::LinearVertical , Slider::NoTextBox};
    TextButton  btGroup1 {"1"};
    TextButton btGroup2 {"2"};

    GroupComponent groupWave{"","Wave"};
    GroupComponent groupFilter{"",TRANS("Filter")};
    GroupComponent groupVolume{"","Volume"};
    GroupComponent groupPan{"","Pan"};
    float fAngle = -90 * (juce::MathConstants<float>::pi  / 180.0);
         CustomLookAndFeel myLookAndFeel;
   
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Element)
};
