/*
  ==============================================================================

    Operator.h
    Created: 12 Feb 2019 11:48:10pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "AlgoDraw.h"

//==============================================================================
/*
 */
class Operator    : public ElementComponent, public TextButton::Listener, public Value::Listener
{
public:
    Operator()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        //   setBounds(getBoundsInParent());
        
        addAndMakeVisible(algoFm);
        addAndMakeVisible(sliderAlgo);
        sliderAlgo.setSliderStyle(Slider::LinearHorizontal);
//        sliderAlgo.setRange(1, 45);
        sliderAlgo.setRangeAndRound(1, 45, 1);
  //      sliderAlgo.setNumDecimalPlacesToDisplay(0);
        sliderAlgo.setPopupDisplayEnabled(true, true, this);
        sliderAlgo.setTextBoxStyle(Slider::NoTextBox, true, 10, 10);
        sliderAlgo.setColour(Slider::ColourIds::thumbColourId, SYColSelected);
        sliderAlgo.onValueChange = [this] {setAlgorythm();};
        // Redessine la topologie AUSSI quand la valeur change sans notification (chargement
        // d'une voix : MidiSlider fait setValue(dontSendNotification) -> onValueChange ne se
        // déclenche pas, mais le Value sous-jacent change -> ce listener régénère le dessin).
        sliderAlgo.getValueObject().addListener(this);
        addAndMakeVisible(labelAlgo);
        labelAlgo.attachToComponent(&sliderAlgo, false);

        setAlgorythm();
        
    }
    
    ~Operator()
    {
        
    }
    void setElementNumber ( int element, UndoManager& um) override
    {
        
        int sysexdata[9] = { 0x43, 0X10, 0x34, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00 };
        
        if(element == 1)
        {
        sliderAlgo.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMALGOELEMENT1, &um));
        }
        if(element == 2)
        {
            
        sliderAlgo.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMALGOELEMENT2, &um));
        
        sysexdata[4] = 0x20;
        }
        if(element == 3)
        {
        sliderAlgo.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMALGOELEMENT3, &um));
        sysexdata[4] = 0x40;
        }
        if(element == 4)
        {
        sliderAlgo.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMALGOELEMENT4, &um));
        sysexdata[4] = 0x60;
        }
        sliderAlgo.setMidiSysex(sysexdata);
        // ALGNUM (groupe 0x05 N2 0x00) est 0-indexé sur le synthé (0..44) alors que le
        // slider affiche 1..45 (cohérent avec le rendu FM kAlgo[N-1]) -> on retire 1 à l'envoi
        // et on rajoute 1 à la réception. (Spec OCR + map TG77 vérifiée.)
        sliderAlgo.setMidiValueOffset(-1);

    }
    
    void setAlgorythm()
    {
        algoFm.setAlgo(sliderAlgo.getValue());
        repaint();
    }
    void valueChanged (Value&) override   // valeur algo changée (y compris au chargement) -> redraw
    {
        setAlgorythm();
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
        
        g.setColour (SYColSelected);
        
     //   g.drawText ("AFM operator a implementer", getLocalBounds(),
      //              Justification::centred, true);   // draw some placeholder text
    }
    
    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        // Panneau « algorithme » : occupe désormais toute la largeur de son conteneur
        // (intégré comme colonne droite de la vue AFM, à l'image du « ALGORITHM » de l'éditeur Atari).
        sliderAlgo.setBoundsRelative(0.04f, 0.06f, 0.92f, 0.08f);
        algoFm.setBoundsRelative(0.04f, 0.18f, 0.92f, 0.78f);
        repaint();
    }
    
private:
    AlgoDraw algoFm;
    MidiSlider sliderAlgo;
    Label labelAlgo { "algo", TRANS("AFM Algorithm")};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Operator)
};
