/*
  ==============================================================================

    Oscillator.h
    Created: 25 Nov 2018 8:29:30pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"   // AfmOscLookAndFeel + AfmWaveLookAndFeel (sprite teinté au thème)
#include "Operator.h"      // panneau algorithme (AlgoDraw + sélecteur 1..45) embarqué à droite

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

        // Colonne FINE (fréquence fine) : même style barre que coarse/det.
        for (MidiSlider* s : { &sliderFreqFine1,&sliderFreqFine2,&sliderFreqFine3,
                               &sliderFreqFine4,&sliderFreqFine5,&sliderFreqFine6 })
            setSliderStyle(*s);

        // Panel zoom : sensibilités, switches, scaling (cachés en mode table, cf. updateZoomVisibility).
        for (int o = 0; o < 6; ++o)
        {
            for (MidiSlider* s : { &sensVel[o], &sensAm[o], &sensPm[o] })
            { addChildComponent(*s); s->setSliderStyle(Slider::LinearBar); s->setTextBoxStyle(Slider::TextBoxRight,false,38,16); }
            for (int k = 0; k < 4; ++k)
            {
                addChildComponent(bpScl[o][k]);  bpScl[o][k].setSliderStyle(Slider::LinearBar);  bpScl[o][k].setTextBoxStyle(Slider::TextBoxRight,false,34,16);
                addChildComponent(offScl[o][k]); offScl[o][k].setSliderStyle(Slider::LinearBar); offScl[o][k].setTextBoxStyle(Slider::TextBoxRight,false,34,16);
            }
            addChildComponent(velSw[o]); velSw[o].setTextOnOff("Vel On","Vel Off");
            addChildComponent(pegSw[o]); pegSw[o].setTextOnOff("PEG On","PEG Off");
        }
        for (Label* l : { &lblSensVel,&lblSensAm,&lblSensPm,&lblVelSw,&lblPegSw,&lblScaling })
        { addChildComponent(*l); l->setJustificationType(Justification::centredLeft); }

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

        setSliderLevel(sliderLevel1);
        setSliderLevel(sliderLevel2);
        setSliderLevel(sliderLevel3);
        setSliderLevel(sliderLevel4);
        setSliderLevel(sliderLevel5);
        setSliderLevel(sliderLevel6);
        labelLevel1.attachToComponent(&sliderLevel1, false);
        labelLevel2.attachToComponent(&sliderLevel2, false);
        labelLevel3.attachToComponent(&sliderLevel3, false);
        labelLevel4.attachToComponent(&sliderLevel4, false);
        labelLevel5.attachToComponent(&sliderLevel5, false);
        labelLevel6.attachToComponent(&sliderLevel6, false);

        // Navigation de la vue zoom (masquée en mode table).
        addChildComponent(btZoomBack);
        addChildComponent(btZoomPrev);
        addChildComponent(btZoomNext);
        btZoomBack.onClick = [this] { setZoomOp(-1); };
        btZoomPrev.onClick = [this] { if (zoomOp > 0) setZoomOp(zoomOp - 1); };
        btZoomNext.onClick = [this] { if (zoomOp >= 0 && zoomOp < 5) setZoomOp(zoomOp + 1); };
        btZoomBack.setTooltip("Retour à la table des 6 opérateurs");

        // Panneau algorithme (colonne droite), masqué automatiquement en mode zoom.
        addAndMakeVisible(algoPanel);
    }

    void setSliderLevel (Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setSliderStyle(Slider::SliderStyle::LinearBar);
        slider.setTextValueSuffix(" Lvl");
        slider.setRange(0, 127, 1);
        slider.setNumDecimalPlacesToDisplay(0);
        slider.setPopupDisplayEnabled(true, true, this);
        slider.setColour(Slider::ColourIds::trackColourId, SYColSelected);
    }

    ~Oscillator()
    {

    }
    void setSliderDetune (Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setSliderStyle(Slider::SliderStyle::LinearBar);   // barre + valeur (cohérent avec la table)
        slider.setRange(-15, 15);
        slider.setNumDecimalPlacesToDisplay(0);
        slider.setLookAndFeel(nullptr);   // suit le LnF du thème (plat en Light)
        slider.setPopupDisplayEnabled(true, true,this);

        
 
    }
    void setSliderStyle (Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setPopupDisplayEnabled(true, true, this);
        slider.setRange(0, 127);
        slider.setNumDecimalPlacesToDisplay(0);
        // Même look « flat » que les levels : barre = rectangle rempli proportionnel.
        slider.setSliderStyle(Slider::SliderStyle::LinearBar);
        // slider.setTextBoxStyle(Slider::TextBoxBelow, false, 40, 18);
    }
    void setOscSliderStyle (Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setPopupDisplayEnabled(true, true, this);
        slider.setRange(0, 15);
        slider.setNumDecimalPlacesToDisplay(0);
        // Waveform affiché 1-indexé (le SY77 montre 1..16), MAIS la valeur reste brute (0..15)
        // pour que la forme dessinée (waveLook) corresponde au bon waveform. Affichage seul.
        slider.textFromValueFunction = [] (double v) { return juce::String ((int) v + 1); };
        slider.valueFromTextFunction = [] (const juce::String& t) { return (double) (t.getIntValue() - 1); };
        slider.setLookAndFeel(&waveLook);   // rendu de la vraie forme SY77, teintée au thème actif
        slider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
       // slider.setTextBoxStyle(Slider::TextBoxBelow, false, 40, 18);
    }
    
    void setElementNumber ( int element, UndoManager& um) override
    {
 
        int sysexdata2[9] = { 0x43, 0X10, 0x34, 0x56, 0x00, 0x00, 0x26, 0x00, 0x00 };
        // [6] = 0x25 (FPC, frequency Coarse). Auparavant 0x17 = PWAVE (waveform) -> bouger
        // le "coarse" changeait la forme d'onde de l'opérateur sur le synthé. Spec table 1-7
        // (sy77midi_ocr.txt l.458) + carte TG77 vérifiée hardware.
        int sysexdata[9] = { 0x43, 0X10, 0x34, 0x46, 0x00, 0x00, 0x25, 0x00, 0x00 };
        
        if(element == 1)
        {
            sliderOsc1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC1, &um));
            sliderOsc2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC2, &um));
            sliderOsc3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC3, &um));
            sliderOsc4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC4, &um));
            sliderOsc5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC5, &um));
            sliderOsc6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSC6, &um));

            sliderFine1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFINE1, &um));
            sliderFine2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFINE2, &um));
            sliderFine3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFINE3, &um));
            sliderFine4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFINE4, &um));
            sliderFine5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFINE5, &um));
            sliderFine6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1OSCFINE6, &um));

            sliderLevel1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1LEVEL1, &um));
            sliderLevel2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1LEVEL2, &um));
            sliderLevel3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1LEVEL3, &um));
            sliderLevel4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1LEVEL4, &um));
            sliderLevel5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1LEVEL5, &um));
            sliderLevel6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT1LEVEL6, &um));

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

            sliderFine1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFINE1, &um));
            sliderFine2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFINE2, &um));
            sliderFine3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFINE3, &um));
            sliderFine4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFINE4, &um));
            sliderFine5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFINE5, &um));
            sliderFine6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2OSCFINE6, &um));

            sliderLevel1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2LEVEL1, &um));
            sliderLevel2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2LEVEL2, &um));
            sliderLevel3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2LEVEL3, &um));
            sliderLevel4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2LEVEL4, &um));
            sliderLevel5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2LEVEL5, &um));
            sliderLevel6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT2LEVEL6, &um));

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

            sliderFine1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFINE1, &um));
            sliderFine2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFINE2, &um));
            sliderFine3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFINE3, &um));
            sliderFine4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFINE4, &um));
            sliderFine5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFINE5, &um));
            sliderFine6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3OSCFINE6, &um));

            sliderLevel1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3LEVEL1, &um));
            sliderLevel2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3LEVEL2, &um));
            sliderLevel3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3LEVEL3, &um));
            sliderLevel4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3LEVEL4, &um));
            sliderLevel5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3LEVEL5, &um));
            sliderLevel6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT3LEVEL6, &um));

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

            sliderFine1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFINE1, &um));
            sliderFine2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFINE2, &um));
            sliderFine3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFINE3, &um));
            sliderFine4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFINE4, &um));
            sliderFine5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFINE5, &um));
            sliderFine6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4OSCFINE6, &um));

            sliderLevel1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4LEVEL1, &um));
            sliderLevel2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4LEVEL2, &um));
            sliderLevel3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4LEVEL3, &um));
            sliderLevel4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4LEVEL4, &um));
            sliderLevel5.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4LEVEL5, &um));
            sliderLevel6.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMELEMENT4LEVEL6, &um));

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
        // Colonne WAVE (roue waveLook) = forme d'onde de l'opérateur (PWAVE, param 0x17, 0..15).
        // (Avant : 0x25 = coarse, ce qui mettait le coarse dans la colonne WAVE et décalait tout.)
        sysexdata[6] = 0x17;
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
        
        // Colonne COARSE = fréquence Coarse (FPC, param 0x25). (Avant : 0x26 = fine -> la
        // colonne COARSE affichait le fine, toujours 0 sur beaucoup de presets.)
        sysexdata[6] = 0x25;
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

        // Coarse affiché en 1-indexé (le SY77 montre 1..N, le bulk/sysex stocke 0..N-1).
        for (MidiSlider* s : { &sliderFine1,&sliderFine2,&sliderFine3,&sliderFine4,&sliderFine5,&sliderFine6 })
            s->setMidiValueOffset(-1);

        // Colonne FINE (param 0x26, fréquence fine 0..99) : referTo + adresse par opérateur.
        {
            MidiSlider* fin[6] = { &sliderFreqFine1,&sliderFreqFine2,&sliderFreqFine3,
                                   &sliderFreqFine4,&sliderFreqFine5,&sliderFreqFine6 };
            const int grp[6] = { 0x56,0x46,0x36,0x26,0x16,0x06 };   // OP1..OP6
            int sx[9] = { 0x43, 0X10, 0x34, 0x56, sysexdata[4], 0x00, 0x26, 0x00, 0x00 };
            for (int o = 0; o < 6; ++o)
            {
                fin[o]->getValueObject().referTo (valueTreeVoice.getPropertyAsValue (
                    Identifier ("ELEMENT" + String (element) + "FREQFINE" + String (o + 1)), &um));
                sx[3] = grp[o];
                fin[o]->setMidiSysex (sx);
                fin[o]->setRangeAndRound (0, 99, 0);
            }
        }

        // --- Panel zoom : SENSIT / SCALING / switches (par opérateur). Adresses spec (Table 1-7).
        // Offsets bulk des SENSIT = meilleure estimation (à confirmer hardware) ; le reste est plain.
        {
            const int grp[6] = { 0x56,0x46,0x36,0x26,0x16,0x06 };   // OP1..OP6
            const int aH = sysexdata[4];
            for (int o = 0; o < 6; ++o)
            {
                auto wireS = [&] (MidiSlider& s, int param, int maxv, const String& prop)
                {
                    int sx[9] = { 0x43, 0X10, 0x34, grp[o], aH, 0x00, param, 0x00, 0x00 };
                    s.setMidiSysex (sx);  s.setRangeAndRound (0, maxv, 0);
                    s.getValueObject().referTo (valueTreeVoice.getPropertyAsValue (
                        Identifier ("ELEMENT" + String (element) + prop + String (o + 1)), &um));
                };
                wireS (sensVel[o], 0x11, 15, "SVEL");
                wireS (sensAm[o],  0x10,  7, "SAM");
                wireS (sensPm[o],  0x18, 31, "SPM");
                for (int k = 0; k < 4; ++k)
                {
                    wireS (bpScl[o][k],  0x1C + k, 127, "BP"  + String (k + 1) + "_");
                    wireS (offScl[o][k], 0x20 + k, 127, "OFF" + String (k + 1) + "_");
                }
                int sv[9] = { 0x43, 0X10, 0x34, grp[o], aH, 0x00, 0x24, 0x00, 0x00 };   // RVSW
                velSw[o].setMidiSysex (sv);
                velSw[o].getToggleStateValue().referTo (valueTreeVoice.getPropertyAsValue (
                    Identifier ("ELEMENT" + String (element) + "VELSW" + String (o + 1)), &um));
                int sp[9] = { 0x43, 0X10, 0x34, 0x05, aH, 0x00, 0x0C, 0x00, 0x00 };     // FYPSW (élément)
                pegSw[o].setMidiSysex (sp);
                pegSw[o].getToggleStateValue().referTo (valueTreeVoice.getPropertyAsValue (
                    Identifier ("ELEMENT" + String (element) + "PEGSW"), &um));
            }
        }
        
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

        // Niveau de sortie par opérateur -> TL (param 0x1B, 0~127, octet simple).
        // Spec table 1-7 (sy77midi_ocr.txt "27 1B TL 0~127 out_level") + carte TG77.
        // Groupe par-opérateur OP1..OP6 = 0x56..0x06 ; addrHi (élément) déjà dans [4].
        sysexdata[6] = 0x1b;
        sysexdata[3] = 0x56;
        sliderLevel1.setMidiSysex(sysexdata);
        sysexdata[3] = 0x46;
        sliderLevel2.setMidiSysex(sysexdata);
        sysexdata[3] = 0x36;
        sliderLevel3.setMidiSysex(sysexdata);
        sysexdata[3] = 0x26;
        sliderLevel4.setMidiSysex(sysexdata);
        sysexdata[3] = 0x16;
        sliderLevel5.setMidiSysex(sysexdata);
        sysexdata[3] = 0x06;
        sliderLevel6.setMidiSysex(sysexdata);

        // Route aussi l'algorithme de l'élément courant vers le panneau embarqué.
        algoPanel.setElementNumber(element, um);
    }
    
    void buttonClicked (Button* button) override
    {

    }

    // Clic dans la gouttière de gauche (nom OPn) en mode table -> zoom sur cet opérateur.
    void mouseDown (const MouseEvent& e) override
    {
        if (zoomOp >= 0) return;
        auto area = getLocalBounds();
        area.removeFromRight (algoPanelWidth());   // exclut la colonne algo (cf. layoutTable)
        const int headerH = tableHeaderH (area.getHeight());
        area.removeFromTop (headerH);
        const int rowH = tableRowH (area.getHeight());
        const int gutter = (int) (0.05f * (float) area.getWidth());
        if (e.x >= area.getX() && e.x < area.getX() + gutter && e.y >= area.getY())
        {
            const int row = (e.y - area.getY()) / rowH;
            if (row >= 0 && row < 6) setZoomOp (row);
        }
    }

    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
           draws some placeholder text to get you started.

           You should replace everything in this method with your own
           drawing code..
        */

        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        if (zoomOp >= 0) paintZoom (g);
        else             paintTable (g);
    }

    // Table « façon éditeur SY77 » : un opérateur par ligne, entêtes de colonnes partagés.
    void paintTable (Graphics& g)
    {
        auto area = getLocalBounds();
        // Réserve (et matérialise) la colonne droite du panneau algo, cohérente avec layoutTable().
        auto algoArea = area.removeFromRight (algoPanelWidth());
        g.setColour (SYPal.panelBorder);
        g.drawVerticalLine (algoArea.getX(), (float) area.getY(), (float) area.getBottom());
        const int headerH = tableHeaderH (area.getHeight());
        auto header = area.removeFromTop (headerH);
        const int rowH = tableRowH (area.getHeight());
        const int W = area.getWidth();
        auto cx = [&] (float f) { return area.getX() + (int) (f * (float) W); };
        auto cw = [&] (float f) { return (int) (f * (float) W); };

        // Entêtes de colonnes (doivent coïncider avec les fractions de resized()).
        g.setColour (SYPal.textMuted);
        g.setFont (Font (FontOptions (12.0f)).boldened());
        auto head = [&] (float f, float w, const String& t)
        { g.drawText (t, cx (f), header.getY(), cw (w), headerH, Justification::centred, false); };
        g.drawText ("OP", area.getX() + 4, header.getY(), cw (0.05f), headerH, Justification::centredLeft, false);
        head (0.055f, 0.17f,  "LEVEL");
        head (0.235f, 0.06f,  "WAVE");
        head (0.31f,  0.085f, "COARSE");
        head (0.40f,  0.085f, "FINE");
        head (0.49f,  0.075f, "DET");
        head (0.575f, 0.085f, "PHASE");
        head (0.665f, 0.13f,  "SYNC");
        head (0.80f,  0.19f,  "MODE");

        g.setColour (SYPal.panelBorder);
        g.drawHorizontalLine (header.getBottom(), (float) area.getX(), (float) area.getRight());

        for (int i = 0; i < 6; ++i)
        {
            const int y = area.getY() + i * rowH;
            if (i > 0)
            {
                g.setColour (SYPal.panelBorder.withAlpha (0.5f));
                g.drawHorizontalLine (y, (float) area.getX(), (float) area.getRight());
            }
            // Nom OPn cliquable (légère pastille pour signaler l'interaction).
            g.setColour (SYPal.accent);
            g.setFont (Font (FontOptions ((float) jmin (18, rowH / 2))).boldened());
            g.drawText ("OP" + String (i + 1), area.getX() + 4, y, cw (0.05f), rowH,
                        Justification::centredLeft, false);
        }
    }

    // Page pleine d'un seul opérateur : juste le titre « OPn » (les contrôles et leurs
    // légendes sont des composants positionnés par resized()).
    void paintZoom (Graphics& g)
    {
        g.setColour (SYPal.accent);
        g.setFont (Font (FontOptions (22.0f)).boldened());
        g.drawText ("OP" + String (zoomOp + 1), zoomTitleArea, Justification::centred, false);
    }

    void resized() override
    {
        updateZoomVisibility();
        if (zoomOp >= 0) layoutZoom();
        else             layoutTable();
    }

    // Affiche/masque les contrôles selon le mode (table = tout ; zoom = un seul op + nav).
    void updateZoomVisibility()
    {
        const bool zoom = zoomOp >= 0;
        btZoomBack.setVisible (zoom);
        btZoomPrev.setVisible (zoom);
        btZoomNext.setVisible (zoom);
        algoPanel.setVisible (! zoom);   // panneau algo visible seulement en mode table
        btZoomPrev.setEnabled (zoomOp > 0);
        btZoomNext.setEnabled (zoomOp >= 0 && zoomOp < 5);

        GroupComponent* grp[6] = { &groupOP1,&groupOP2,&groupOP3,&groupOP4,&groupOP5,&groupOP6 };
        for (auto* gc : grp) gc->setVisible (false);

        for (int i = 0; i < 6; ++i)
        {
            const bool vis = zoom ? (i == zoomOp) : true;
            auto c = op (i);
            c.osc->setVisible (vis); c.lvl->setVisible (vis); c.crs->setVisible (vis); c.fin->setVisible (vis);
            c.det->setVisible (vis); c.pha->setVisible (vis); c.syn->setVisible (vis); c.mod->setVisible (vis);
            Label* L[4]; opLabels (i, L);
            for (auto* l : L) l->setVisible (zoom && i == zoomOp);   // légendes seulement en zoom

            // Contrôles détaillés (SENSIT/SCALING/switches) : uniquement l'op zoomé.
            const bool zv = zoom && i == zoomOp;
            for (MidiSlider* s : { &sensVel[i], &sensAm[i], &sensPm[i] }) s->setVisible (zv);
            for (int k = 0; k < 4; ++k) { bpScl[i][k].setVisible (zv); offScl[i][k].setVisible (zv); }
            velSw[i].setVisible (zv); pegSw[i].setVisible (zv);
        }
        for (Label* l : { &lblSensVel,&lblSensAm,&lblSensPm,&lblVelSw,&lblPegSw,&lblScaling })
            l->setVisible (zoom);
    }

    // Table : 1 opérateur par ligne. Les fractions doivent coïncider avec paintTable().
    void layoutTable()
    {
        auto content = getLocalBounds();
        // Colonne droite : schéma d'algorithme (comme l'éditeur Atari d'origine).
        algoPanel.setBounds (content.removeFromRight (algoPanelWidth()).reduced (6));
        const int headerH = tableHeaderH (content.getHeight());
        content.removeFromTop (headerH);
        const int rowH = tableRowH (content.getHeight());   // plafonnée, lignes calées en haut
        const int W = content.getWidth();
        auto cx = [&] (float f) { return content.getX() + (int) (f * (float) W); };
        auto cw = [&] (float f) { return (int) (f * (float) W); };

        for (int i = 0; i < 6; ++i)
        {
            auto c = op (i);
            // Retour table : rebascule les 4 champs en barres plates (le zoom les passe en potards).
            for (MidiSlider* sl : { c.lvl, c.crs, c.fin, c.det, c.pha })
            {
                sl->setSliderStyle (Slider::LinearBar);
                sl->setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
            }
            const int y  = content.getY() + i * rowH;
            const int ry = y + 4, rh = rowH - 8;
            c.lvl->setBounds (cx (0.055f), ry, cw (0.17f),  rh);
            // Roue WAVE : carré centré dans sa cellule (sinon elle s'étire et devient laide).
            auto oscCell = Rectangle<int> (cx (0.235f), y + 2, cw (0.06f), rowH - 4);
            const int oscSide = jmin (oscCell.getWidth(), oscCell.getHeight());
            c.osc->setBounds (oscCell.withSizeKeepingCentre (oscSide, oscSide));
            c.crs->setBounds (cx (0.31f),  ry, cw (0.085f), rh);
            c.fin->setBounds (cx (0.40f),  ry, cw (0.085f), rh);
            c.det->setBounds (cx (0.49f),  ry, cw (0.075f), rh);
            c.pha->setBounds (cx (0.575f), ry, cw (0.085f), rh);
            c.syn->setBounds (cx (0.665f), ry, cw (0.13f),  rh);
            c.mod->setBounds (cx (0.80f),  ry, cw (0.19f),  rh);
        }
    }

    // Page pleine d'un seul opérateur : nav + grande forme d'onde + champs empilés.
    void layoutZoom()
    {
        auto c = op (zoomOp);
        auto area = getLocalBounds().reduced (14);

        // Barre de nav : [<] OPn [>] ........ [Table]
        auto nav = area.removeFromTop (jmax (30, area.getHeight() / 11));
        btZoomBack.setBounds (nav.removeFromRight (120).reduced (3));
        auto grp = nav.removeFromLeft (jmin (260, nav.getWidth()));
        btZoomPrev.setBounds (grp.removeFromLeft (52).reduced (3));
        btZoomNext.setBounds (grp.removeFromRight (52).reduced (3));
        zoomTitleArea = grp;

        area.removeFromTop (8);

        // Forme d'onde (réduite pour laisser la place au détail complet de l'opérateur).
        auto waveRow = area.removeFromTop ((int) (area.getHeight() * 0.26f));
        const int wsz = jmin (waveRow.getHeight(), waveRow.getWidth());
        c.osc->setBounds (waveRow.withSizeKeepingCentre (wsz, wsz));
        area.removeFromTop (8);

        // SYNC / MODE en bas.
        auto btnRow = area.removeFromBottom (36);
        c.syn->setBounds (btnRow.removeFromLeft (btnRow.getWidth() / 2).reduced (8, 4));
        c.mod->setBounds (btnRow.reduced (8, 4));
        area.removeFromBottom (8);

        const int rowH = jmax (44, area.getHeight() / 3);

        // Rangée 1 : LEVEL / COARSE / FINE / DET / PHASE (potards).
        auto r1 = area.removeFromTop (rowH);
        {
            MidiSlider* knobs[5] = { c.lvl, c.crs, c.fin, c.det, c.pha };
            const int kw = jmax (1, r1.getWidth() / 5);
            for (int i = 0; i < 5; ++i)
            {
                knobs[i]->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
                knobs[i]->setTextBoxStyle (Slider::TextBoxBelow, false, jmin (70, kw - 12), 18);
                auto cell = Rectangle<int> (r1.getX() + i * kw, r1.getY() + 18, kw, r1.getHeight() - 18);
                knobs[i]->setBounds (cell.reduced (10, 4));
            }
        }

        // Rangée 2 : SENSIT (VEL / AM / PM) + VEL SW + PEG SW.
        auto r2 = area.removeFromTop (rowH);
        {
            const int w = jmax (1, r2.getWidth() / 5);
            auto put = [&] (Label& lab, Component& ctl, int i)
            {
                auto cell = Rectangle<int> (r2.getX() + i * w, r2.getY(), w, r2.getHeight());
                lab.setBounds (cell.removeFromTop (16));
                ctl.setBounds (cell.reduced (6, 4));
            };
            put (lblSensVel, sensVel[zoomOp], 0);
            put (lblSensAm,  sensAm[zoomOp],  1);
            put (lblSensPm,  sensPm[zoomOp],  2);
            put (lblVelSw,   velSw[zoomOp],   3);
            put (lblPegSw,   pegSw[zoomOp],   4);
        }

        // Rangée 3 : SCALING (4 break-points + 4 offsets de niveau).
        auto r3 = area.removeFromTop (rowH);
        {
            lblScaling.setBounds (r3.removeFromTop (16));
            const int w = jmax (1, r3.getWidth() / 8);
            for (int k = 0; k < 4; ++k)
            {
                bpScl [zoomOp][k].setBounds (Rectangle<int> (r3.getX() + k       * w, r3.getY(), w, r3.getHeight()).reduced (4, 4));
                offScl[zoomOp][k].setBounds (Rectangle<int> (r3.getX() + (4 + k) * w, r3.getY(), w, r3.getHeight()).reduced (4, 4));
            }
        }
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

    // Colonne FINE (fréquence fine, param 0x26) — distincte du coarse (sliderFine* = COARSE).
    MidiSlider sliderFreqFine1, sliderFreqFine2, sliderFreqFine3, sliderFreqFine4, sliderFreqFine5, sliderFreqFine6;

    // Panel détaillé (vue zoom) : sensibilités, switches, scaling — par opérateur (tableaux).
    MidiSlider sensVel[6], sensAm[6], sensPm[6];   // SENSIT VEL/AM/PM (params 0x11/0x10/0x18)
    MidiButton velSw[6], pegSw[6];                 // VEL SW (RVSW 0x24), PEG SW (FYPSW elem 0x0C)
    MidiSlider bpScl[6][4], offScl[6][4];          // SCALING : break-points (0x1C-1F) + offsets (0x20-23)
    Label lblSensVel { "", "Vel" }, lblSensAm { "", "AM" }, lblSensPm { "", "PM" },
          lblVelSw { "", "Vel SW" }, lblPegSw { "", "PEG SW" }, lblScaling { "", "Scaling (BP / Offset)" };

    // Niveau de sortie par opérateur (alimente le rendu FM ; pas d'envoi sysex pour l'instant)
    MidiSlider sliderLevel1;
    MidiSlider sliderLevel2;
    MidiSlider sliderLevel3;
    MidiSlider sliderLevel4;
    MidiSlider sliderLevel5;
    MidiSlider sliderLevel6;
    Label labelLevel1 {"l1", "Level"};
    Label labelLevel2 {"l2", "Level"};
    Label labelLevel3 {"l3", "Level"};
    Label labelLevel4 {"l4", "Level"};
    Label labelLevel5 {"l5", "Level"};
    Label labelLevel6 {"l6", "Level"};

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

    // ---- Mode zoom : -1 = table 6 lignes ; 0..5 = un opérateur en pleine page ----
    int zoomOp = -1;
    TextButton btZoomBack { "Table" };
    TextButton btZoomPrev { "<" };
    TextButton btZoomNext { ">" };
    Rectangle<int> zoomTitleArea;   // zone du titre « OP3 » (dessiné dans paint)

    struct OpCtrls { MidiSlider* osc; MidiSlider* lvl; MidiSlider* crs; MidiSlider* fin;
                     MidiSlider* det; MidiSlider* pha; MidiButton* syn; MidiButton* mod; };
    OpCtrls op (int i)
    {
        MidiSlider* osc[6] = { &sliderOsc1,&sliderOsc2,&sliderOsc3,&sliderOsc4,&sliderOsc5,&sliderOsc6 };
        MidiSlider* lvl[6] = { &sliderLevel1,&sliderLevel2,&sliderLevel3,&sliderLevel4,&sliderLevel5,&sliderLevel6 };
        MidiSlider* crs[6] = { &sliderFine1,&sliderFine2,&sliderFine3,&sliderFine4,&sliderFine5,&sliderFine6 };
        MidiSlider* fin[6] = { &sliderFreqFine1,&sliderFreqFine2,&sliderFreqFine3,&sliderFreqFine4,&sliderFreqFine5,&sliderFreqFine6 };
        MidiSlider* det[6] = { &sliderDetune1,&sliderDetune2,&sliderDetune3,&sliderDetune4,&sliderDetune5,&sliderDetune6 };
        MidiSlider* pha[6] = { &sliderPhase1,&sliderPhase2,&sliderPhase3,&sliderPhase4,&sliderPhase5,&sliderPhase6 };
        MidiButton* syn[6] = { &btPhase1,&btPhase2,&btPhase3,&btPhase4,&btPhase5,&btPhase6 };
        MidiButton* mod[6] = { &btFix1,&btFix2,&btFix3,&btFix4,&btFix5,&btFix6 };
        return { osc[i],lvl[i],crs[i],fin[i],det[i],pha[i],syn[i],mod[i] };
    }
    // Labels d'un opérateur (réutilisés comme légendes en mode zoom).
    void opLabels (int i, Label* out[4])
    {
        Label* lv[6] = { &labelLevel1,&labelLevel2,&labelLevel3,&labelLevel4,&labelLevel5,&labelLevel6 };
        Label* cr[6] = { &labelFine1,&labelFine2,&labelFine3,&labelFine4,&labelFine5,&labelFine6 };
        Label* dt[6] = { &labelDetune1,&labelDetune2,&labelDetune3,&labelDetune4,&labelDetune5,&labelDetune6 };
        Label* ph[6] = { &labelPhase1,&labelPhase2,&labelPhase3,&labelPhase4,&labelPhase5,&labelPhase6 };
        out[0] = lv[i]; out[1] = cr[i]; out[2] = dt[i]; out[3] = ph[i];
    }

    void setZoomOp (int o)
    {
        zoomOp = jlimit (-1, 5, o);
        resized();
        repaint();
    }

    AfmWaveLookAndFeel waveLook;   // formes d'onde des 6 opérateurs (vraie image SY77, teintée au thème)

    // Panneau algorithme embarqué (colonne droite) — réutilise toute la logique d'Operator
    // (sélecteur 1..45, schéma AlgoDraw, sysex ALGNUM avec offset -1). Masqué en mode zoom.
    Operator algoPanel;

    // Largeur réservée au panneau algorithme à droite ; layoutTable() et paintTable()
    // doivent utiliser la même valeur pour rester cohérents.
    int algoPanelWidth() const { return jmax (180, getWidth() * 30 / 100); }

    // Hauteur de ligne plafonnée : évite que les potards (roue WAVE) soient étirés quand la
    // vue est haute. En dessous, la ligne peut rétrécir (scroll à prévoir si trop petit).
    // layoutTable(), paintTable() et mouseDown() DOIVENT utiliser ces mêmes helpers.
    static constexpr int kMaxRowH = 60;
    int tableHeaderH (int totalH) const { return jlimit (18, 28, totalH / 14); }
    int tableRowH    (int bodyH)  const { return jmin (kMaxRowH, jmax (1, bodyH / 6)); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oscillator)
};
