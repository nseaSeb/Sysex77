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
#include "AfmLfo.h"        // panneau Main/Sub LFO (colonne droite, sous l'algo)

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
        labelFine2.attachToComponent(&sliderFine2, false);
        setSliderDetune(sliderDetune2);
        labelDetune2.attachToComponent(&sliderDetune2, false);
        labelDetune2.setJustificationType(Justification::centred);
        setSliderStyle(sliderPhase2);
        labelPhase2.attachToComponent(&sliderPhase2, false);
        
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

        // Panneau détail : sensibilités, switches, scaling (visibles pour l'op sélectionné, cf. updateSelVisibility).
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

        // Barre d'onglets « OP1 | … | OP6 » (une seule ligne fine) pour choisir l'op à éditer.
        // Boutons radio mutuellement exclusifs (même radioGroupId) façon segmented tabs.
        for (int i = 0; i < 6; ++i)
        {
            auto& tab = opTab[i];
            addAndMakeVisible (tab);
            tab.setButtonText ("OP" + String (i + 1));
            tab.setClickingTogglesState (true);
            tab.setRadioGroupId (1001);
            tab.setConnectedEdges (  (i > 0 ? Button::ConnectedOnLeft  : 0)
                                   | (i < 5 ? Button::ConnectedOnRight : 0));
            tab.setColour (TextButton::buttonOnColourId, SYPal.accent);
            tab.onClick = [this, i] { setSelOp (i); };
        }
        opTab[selOp].setToggleState (true, dontSendNotification);

        // Interrupteurs ON/OFF des 6 opérateurs (à DROITE de la barre, sur la même ligne fine).
        // MUTE éditeur via le niveau de sortie TL (param 0x1B, déjà câblé sur sliderLevel*) :
        // la spec AFM (Table 1-7) n'expose AUCUN « operator on/off » dédié (comme sur DX7) — seuls
        // des switches PEG/VEL existent. OFF = mémorise le TL courant puis force le niveau à 0
        // (l'envoi sysex TL=0 part via le wiring existant) ; ON = restaure le TL mémorisé.
        for (int i = 0; i < 6; ++i)
        {
            auto& sw = opOn[i];
            addAndMakeVisible (sw);
            sw.setButtonText (String (i + 1));
            sw.setClickingTogglesState (true);
            sw.setToggleState (true, dontSendNotification);   // défaut : tous ON
            sw.setColour (TextButton::buttonOnColourId,  SYPal.accent);      // allumé = ON (accent)
            sw.setColour (TextButton::textColourOnId,    SYPal.background);   // texte lisible sur accent
            sw.setColour (TextButton::textColourOffId,   SYPal.textMuted);    // atténué = OFF
            sw.setTooltip ("Active/désactive l'opérateur " + String (i + 1) + " (mute via niveau)");
            sw.onClick = [this, i] { toggleOpActive (i); };
        }

        // Panneau algorithme (colonne droite), masqué automatiquement en mode zoom.
        addAndMakeVisible(algoPanel);
        // Panneau LFO (Main + Sub) sous l'algo, comme l'écran SynthWorks. Masqué en zoom.
        addAndMakeVisible(lfoPanel);
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
    void setSliderDetune (MidiSlider& slider)
    {
        addAndMakeVisible(slider);
        slider.setSliderStyle(Slider::SliderStyle::LinearBar);   // barre + valeur (cohérent avec la table)
        // FPD = détune signé -15..+15, encodage SIGNE-MAGNITUDE (param 0x1A).
        // setRangeAndRound(min<0) ACTIVE le chemin boolNegative de MidiSlider :
        //   TX  d<0 -> ~d + 17 = 16 | (-d)   (cf. SyVoice::fpdDetuneToWire)
        //   RX  wire>=15 -> ~(wire-17)       (cf. SyVoice::fpdDetuneToDisplay)
        // Auparavant setRange(-15,15) ne touchait PAS boolNegative -> les négatifs partaient
        // bruts (wrap 7 bits) et la lecture du dump s/m (wire 17..31) s'affichait telle quelle.
        slider.setRangeAndRound(-15, 15, 0);
        slider.setSliderStyle(Slider::SliderStyle::LinearBar);   // setRangeAndRound force Rotary pour min<0 -> on rétablit la barre
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
        
        // --- Octets PACKÉS 0x18 (MODE/Fixed = FPM b0) et 0x19 (SYNC = KOE V1 b0, PHASE = V2).
        // Adresse posée pour la composition (groupe par op + addrHi élément).
        packedAddrHi = sysexdata[4];
        {
            MidiButton* fix[6] = { &btFix1,&btFix2,&btFix3,&btFix4,&btFix5,&btFix6 };
            MidiButton* syn[6] = { &btPhase1,&btPhase2,&btPhase3,&btPhase4,&btPhase5,&btPhase6 };
            MidiSlider* pha[6] = { &sliderPhase1,&sliderPhase2,&sliderPhase3,&sliderPhase4,&sliderPhase5,&sliderPhase6 };
            for (int o = 0; o < 6; ++o)
            {
                // btFix = MODE (FPM, octet 0x18 b0). Auto-send OFF -> composition de l'octet complet.
                fix[o]->setAutoSend (false);
                fix[o]->onClick = [this, o] { send18 (o); };
                // btPhase = SYNC (KOE, octet 0x19 V1 b0). Auto-send OFF.
                syn[o]->setAutoSend (false);
                syn[o]->onClick = [this, o] { send19 (o); };
                // sliderPhase = PHASE (octet 0x19 V2 0~127). Auto-send OFF + plage 0..127.
                pha[o]->setAutoSend (false);
                pha[o]->setRangeAndRound (0, 127, 0);
                pha[o]->onValueChange = [this, o] { send19 (o); };
            }
        }

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

        // Coarse (FPC, param 0x25) = octet brut, AUCUN décalage d'affichage.
        // L'éditeur TG77 vérifié hardware (TG77_Voice.json "COARSE OP*") envoie l'octet
        // tel quel sur 0..127 (display==message, pas d'offset). L'ancien setMidiValueOffset(-1)
        // « 1-indexait » l'affichage : display 0 -> wire -1 (wrap 127), rendant le bas de la
        // plage inutilisable (« coarse pas utilisable »). On retire l'offset et on borne 0..127.
        for (MidiSlider* s : { &sliderFine1,&sliderFine2,&sliderFine3,&sliderFine4,&sliderFine5,&sliderFine6 })
        {
            s->setMidiValueOffset(0);
            s->setRangeAndRound(0, 127, 0);
        }

        // Colonne FINE (FPF, param 0x26) : referTo + adresse par opérateur.
        // PLAGE 0..127 (octet brut, display==wire), PAS 0..99 : la carte TG77 vérifiée
        // hardware (TG77_Voice.json "FINE OP*", paramNumber *038 = 0x26) borne 0..127.
        // L'ancien 0..99 (#3) rendait inaccessibles les valeurs 100..127 et faussait la
        // proportion/affichage de la barre (« comportement du bouton »). OCR Table 1-7
        // l.460 (FPF) sans plage lisible -> on suit la carte hardware.
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
                fin[o]->setRangeAndRound (0, 127, 0);
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
                // SENSIT PM = FMLPMS (octet packé 0x18 b4~2, plage 0~7). Auto-send OFF :
                // recomposition de l'octet 0x18 (sinon écrase MODE/FPM b0 + PES b1). Spec l.431.
                sensPm[o].setAutoSend (false);
                sensPm[o].setRangeAndRound (0, 7, 0);
                sensPm[o].getValueObject().referTo (valueTreeVoice.getPropertyAsValue (
                    Identifier ("ELEMENT" + String (element) + "SPM" + String (o + 1)), &um));
                sensPm[o].onValueChange = [this, o] { send18 (o); };
                for (int k = 0; k < 4; ++k)
                {
                    wireS (bpScl[o][k],  0x1C + k, 127, "BP"  + String (k + 1) + "_");
                    wireS (offScl[o][k], 0x20 + k, 127, "OFF" + String (k + 1) + "_");
                }
                int sv[9] = { 0x43, 0X10, 0x34, grp[o], aH, 0x00, 0x24, 0x00, 0x00 };   // RVSW
                velSw[o].setMidiSysex (sv);
                velSw[o].getToggleStateValue().referTo (valueTreeVoice.getPropertyAsValue (
                    Identifier ("ELEMENT" + String (element) + "VELSW" + String (o + 1)), &um));
                // PEG SW = PES (Pitch-EG Switch), PAR OPÉRATEUR, octet packé 0x18 b1 (spec l.432).
                // CORRIGÉ : auparavant FYPSW group 0x05/0x0C = « Velocity Switch » du pitch-EG
                // d'ÉLÉMENT (1 seul, spec l.356), avec une propriété PARTAGÉE -> 6 boutons / 1 état
                // / aucun effet (mauvaise adresse). Maintenant : 6 états distincts, recompose 0x18.
                pegSw[o].setAutoSend (false);
                pegSw[o].getToggleStateValue().referTo (valueTreeVoice.getPropertyAsValue (
                    Identifier ("ELEMENT" + String (element) + "PEGSW" + String (o + 1)), &um));
                pegSw[o].onClick = [this, o] { send18 (o); };
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
        // ... et le LFO (Main + Sub) de l'élément courant (group 0x05).
        lfoPanel.setElementNumber(element, um);
    }
    
    void buttonClicked (Button* button) override
    {

    }

    // (Sélection désormais gérée par la barre d'onglets opTab[] — plus de clic sur rangée.)

    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
           draws some placeholder text to get you started.

           You should replace everything in this method with your own
           drawing code..
        */

        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

        // Séparateur de la colonne droite (algo/LFO), cohérent avec layoutMaster().
        g.setColour (SYPal.panelBorder);
        g.drawVerticalLine (getLocalBounds().getRight() - algoPanelWidth(),
                            (float) getLocalBounds().getY(), (float) getLocalBounds().getBottom());

        // Mini-libellés distinguant la zone SÉLECTION (gauche) de la zone ACTIF/ON-OFF (droite).
        g.setColour (SYPal.textMuted);
        g.setFont (Font (FontOptions (10.0f)).boldened());
        g.drawText ("EDIT",  editLabelArea,  Justification::centred, false);
        g.drawText ("ACTIF", onOffLabelArea, Justification::centred, false);

        // Cadre + titre du panneau détail (sous la barre d'onglets). Les contrôles labellisés
        // sont des composants posés par layoutDetail() ; la barre d'onglets = boutons opTab[].
        g.setColour (SYPal.panelBorder);
        g.drawHorizontalLine (detailArea.getY(), (float) detailArea.getX(), (float) detailArea.getRight());
        g.setColour (SYPal.accent);
        g.setFont (Font (FontOptions (16.0f)).boldened());
        g.drawText ("OP" + String (selOp + 1) + " — détail", detailTitleArea,
                    Justification::centredLeft, false);

        // Étiquettes des 8 barres SCALING (BP1..BP4 / OF1..OF4).
        g.setColour (SYPal.textMuted);
        g.setFont (Font (FontOptions (10.0f)));
        for (auto& t : g_scalingTags)
            g.drawText (t.text, t.area, Justification::centred, false);
    }

    void resized() override
    {
        updateSelVisibility();
        layoutMaster();
        layoutDetail();
    }

    // Onglets + détail : la barre d'onglets (opTab[]) choisit selOp ; le panneau détail montre
    // TOUS les contrôles labellisés du seul op sélectionné (les autres op sont masqués).
    void updateSelVisibility()
    {
        for (int i = 0; i < 6; ++i)
            opTab[i].setToggleState (i == selOp, dontSendNotification);
        algoPanel.setVisible (true);   // colonne droite toujours présente
        lfoPanel.setVisible (true);

        GroupComponent* grp[6] = { &groupOP1,&groupOP2,&groupOP3,&groupOP4,&groupOP5,&groupOP6 };
        for (auto* gc : grp) gc->setVisible (false);

        for (int i = 0; i < 6; ++i)
        {
            const bool sel = (i == selOp);
            auto c = op (i);
            // Détail (op sélectionné seulement) : TOUS les contrôles de l'opérateur.
            c.lvl->setVisible (sel); c.osc->setVisible (sel);
            c.crs->setVisible (sel); c.fin->setVisible (sel);
            c.det->setVisible (sel); c.pha->setVisible (sel); c.syn->setVisible (sel); c.mod->setVisible (sel);
            for (MidiSlider* s : { &sensVel[i], &sensAm[i], &sensPm[i] }) s->setVisible (sel);
            for (int k = 0; k < 4; ++k) { bpScl[i][k].setVisible (sel); offScl[i][k].setVisible (sel); }
            velSw[i].setVisible (sel); pegSw[i].setVisible (sel);

            Label* L[4]; opLabels (i, L);
            for (auto* l : L) l->setVisible (sel);   // légendes du détail
        }
        // Légendes SENSIT/SCALING/VEL SW/PEG SW du détail.
        for (Label* l : { &lblSensVel,&lblSensAm,&lblSensPm,&lblVelSw,&lblPegSw,&lblScaling })
            l->setVisible (true);
    }

    // Disposition globale : colonne droite (algo/LFO) ; à gauche une barre fine en haut
    // (onglets de SÉLECTION à gauche + interrupteurs ON/OFF à droite) ; le reste = détail.
    void layoutMaster()
    {
        auto content = getLocalBounds();
        // Colonne droite : ALGORITHM en haut, puis MAIN/SUB LFO dessous (inchangée).
        {
            auto rightCol = content.removeFromRight (algoPanelWidth());
            auto algoArea = rightCol.removeFromTop (jmax (150, rightCol.getHeight() * 42 / 100));
            algoPanel.setBounds (algoArea.reduced (6));
            lfoPanel.setBounds (rightCol.reduced (6, 2));
        }

        // --- Barre fine du haut : GAUCHE = onglets « OP1..OP6 » (édition) ; DROITE = ON/OFF (actif).
        const int barH = jlimit (26, 34, content.getHeight() / 14);
        auto bar = content.removeFromTop (barH);

        // Réserve la zone ON/OFF à droite : 6 petits carrés + un mini-libellé « ACTIF » au-dessus.
        const int swW   = jmax (16, barH - 4);                       // interrupteur ~carré
        const int lblW  = 44;                                        // libellés « ÉDIT » / « ACTIF »
        auto onOffZone  = bar.removeFromRight (6 * swW + 8);
        onOffLabelArea  = bar.removeFromRight (lblW);                // « ACTIF » (dessiné dans paint)
        bar.removeFromRight (8);
        for (int i = 0; i < 6; ++i)
            opOn[i].setBounds (Rectangle<int> (onOffZone.getX() + i * swW, onOffZone.getY(),
                                               swW, onOffZone.getHeight()).reduced (1, 2));

        // GAUCHE : un mini-libellé « ÉDIT » puis la barre d'onglets sur la place restante.
        editLabelArea = bar.removeFromLeft (lblW);                   // « ÉDIT » (dessiné dans paint)
        auto tabs = bar;
        const int tw = jmax (1, tabs.getWidth() / 6);
        for (int i = 0; i < 6; ++i)
        {
            const int x = tabs.getX() + i * tw;
            const int w = (i == 5) ? (tabs.getRight() - x) : tw;     // dernier onglet -> bord exact
            opTab[i].setBounds (x, tabs.getY(), w, tabs.getHeight());
        }

        // Tout l'espace restant va au panneau détail (la pièce maîtresse).
        detailArea = content;
    }

    // Panneau détail (sous les onglets) : TOUS les contrôles de l'op selOp, labellisés et au large.
    // Reprend la disposition labellisée de l'ancien layoutZoom (potards + SENSIT/SCALING/switches).
    void layoutDetail()
    {
        auto c = op (selOp);
        auto area = detailArea.reduced (14);

        // Bandeau titre + roue WAVE de l'op sélectionné (le titre est dessiné dans paint()).
        auto top = area.removeFromTop (jmax (40, area.getHeight() / 5));
        {
            auto waveCell = top.removeFromRight (jmin (top.getWidth() / 3, top.getHeight()));
            const int s = jmin (waveCell.getWidth(), waveCell.getHeight());
            c.osc->setBounds (waveCell.withSizeKeepingCentre (s, s));
            detailTitleArea = top;   // zone titre « OPn — détail »
        }
        area.removeFromTop (8);

        // SYNC / MODE en bas.
        auto btnRow = area.removeFromBottom (36);
        c.syn->setBounds (btnRow.removeFromLeft (btnRow.getWidth() / 2).reduced (8, 4));
        c.mod->setBounds (btnRow.reduced (8, 4));
        area.removeFromBottom (8);

        const int rowH = jmax (44, area.getHeight() / 3);

        // Rangée 1 : LEVEL / COARSE / FINE / DET / PHASE (potards labellisés).
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
        // SENSIT = barres horizontales LinearBar AVEC valeur (lisibles, plus de pavés vides).
        auto r2 = area.removeFromTop (rowH);
        {
            for (MidiSlider* sl : { &sensVel[selOp],&sensAm[selOp],&sensPm[selOp] })
            { sl->setSliderStyle (Slider::LinearBar); sl->setTextBoxStyle (Slider::NoTextBox, true, 0, 0); }
            const int w = jmax (1, r2.getWidth() / 5);
            auto putBar = [&] (Label& lab, Component& ctl, int i)   // libellé au-dessus, barre au milieu
            {
                auto cell = Rectangle<int> (r2.getX() + i * w, r2.getY(), w, r2.getHeight());
                lab.setBounds (cell.removeFromTop (16));
                ctl.setBounds (cell.withSizeKeepingCentre (cell.getWidth() - 12, jmin (24, cell.getHeight() - 6)));
            };
            auto putBtn = [&] (Label& lab, Component& ctl, int i)   // switch : libellé + bouton
            {
                auto cell = Rectangle<int> (r2.getX() + i * w, r2.getY(), w, r2.getHeight());
                lab.setBounds (cell.removeFromTop (16));
                ctl.setBounds (cell.reduced (6, 4));
            };
            putBar (lblSensVel, sensVel[selOp], 0);
            putBar (lblSensAm,  sensAm[selOp],  1);
            putBar (lblSensPm,  sensPm[selOp],  2);
            putBtn (lblVelSw,   velSw[selOp],   3);
            putBtn (lblPegSw,   pegSw[selOp],   4);
        }

        // Rangée 3 : SCALING (4 break-points + 4 offsets de niveau).
        // 8 barres horizontales LinearBar AVEC valeur, libellées BP1..BP4 / OF1..OF4.
        auto r3 = area.removeFromTop (rowH);
        {
            for (int k = 0; k < 4; ++k)
                for (MidiSlider* sl : { &bpScl[selOp][k], &offScl[selOp][k] })
                { sl->setSliderStyle (Slider::LinearBar); sl->setTextBoxStyle (Slider::NoTextBox, true, 0, 0); }

            lblScaling.setBounds (r3.removeFromTop (16));
            const int w = jmax (1, r3.getWidth() / 8);
            g_scalingTags.clear();
            for (int k = 0; k < 4; ++k)
            {
                auto cellBp = Rectangle<int> (r3.getX() + k       * w, r3.getY(), w, r3.getHeight());
                auto cellOf = Rectangle<int> (r3.getX() + (4 + k) * w, r3.getY(), w, r3.getHeight());
                g_scalingTags.add ({ cellBp.removeFromTop (14), "BP" + String (k + 1) });
                g_scalingTags.add ({ cellOf.removeFromTop (14), "OF" + String (k + 1) });
                bpScl [selOp][k].setBounds (cellBp.withSizeKeepingCentre (cellBp.getWidth() - 8, jmin (22, cellBp.getHeight() - 4)));
                offScl[selOp][k].setBounds (cellOf.withSizeKeepingCentre (cellOf.getWidth() - 8, jmin (22, cellOf.getHeight() - 4)));
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
    MidiButton velSw[6], pegSw[6];                 // VEL SW (RVSW 0x24), PEG SW (PES, octet packé 0x18 b1, par op)
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

    // ---- Onglets + détail : selOp = opérateur sélectionné (0..5), jamais -1 (détail jamais vide).
    int selOp = 0;
    TextButton opTab[6];   // barre d'onglets « OP1 | … | OP6 » (radioGroup, choix de l'op à éditer)
    TextButton opOn[6];    // interrupteurs ON/OFF (mute via TL) des 6 op, à droite de la barre
    int savedLevel[6] = { 0,0,0,0,0,0 };   // niveau TL mémorisé pendant un mute (restauré à ON)
    // Rectangles calculés par layoutMaster()/layoutDetail(), réutilisés par paint().
    Rectangle<int> detailArea, detailTitleArea, editLabelArea, onOffLabelArea;

    // Étiquettes BP1..BP4 / OF1..OF4 des barres SCALING (dessinées dans paint, posées par layoutDetail).
    struct ScalingTag { Rectangle<int> area; String text; };
    Array<ScalingTag> g_scalingTags;

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

    void setSelOp (int o)
    {
        selOp = jlimit (0, 5, o);
        resized();
        repaint();
    }

    // MUTE éditeur d'un opérateur via le niveau TL (param 0x1B, 0x1B déjà câblé sur sliderLevel*).
    // PAS de paramètre « on/off » dédié dans la spec AFM (Table 1-7) -> on coupe le niveau :
    //   OFF -> mémorise le niveau courant puis force la Value de level à 0 (envoi TL=0 via le wiring
    //          existant ; le potard Level se met aussi à 0, reflétant l'état).
    //   ON  -> restaure le niveau mémorisé (re-déclenche l'envoi TL + met à jour le potard).
    // Mute LOCAL à l'éditeur (pas de param hardware dédié) : un (re)chargement de voix remet les
    // niveaux réels ; il faut alors re-cliquer pour re-muter. Documenté tel quel.
    void toggleOpActive (int i)
    {
        auto* lvl = op (i).lvl;
        const bool on = opOn[i].getToggleState();
        if (! on)
        {
            savedLevel[i] = (int) lvl->getValue();   // mémorise avant de couper
            lvl->setValue (0.0, sendNotificationSync);
        }
        else
        {
            lvl->setValue ((double) savedLevel[i], sendNotificationSync);
        }
    }

    AfmWaveLookAndFeel waveLook;   // formes d'onde des 6 opérateurs (vraie image SY77, teintée au thème)

    //==============================================================================
    // OCTETS PACKÉS de l'opérateur AFM (Table 1-7) — composés ici, pas auto-envoyés.
    //   0x18 : FMLPMS (M_LFO PM sens, b4~2) | PES (Pitch-EG switch, b1) | FPM (freq Mode, b0).
    //          -> sensPm, pegSw, btFix écrivaient TOUS l'octet 0x18 en BRUT et s'écrasaient.
    //   0x19 : KOE (init-phase set Enable = SYNC) en V1 b0 ; PHASE (0~127) en V2.
    //          -> btPhase (sync) et sliderPhase (valeur) partagent le même frame V1/V2.
    // On garde un cache par opérateur et on RECOMPOSE l'octet complet à chaque édition.
    SysexBusSender packedSender;
    int packedGroup[6] = { 0x56,0x46,0x36,0x26,0x16,0x06 };   // OP1..OP6
    int packedAddrHi = 0;                                     // élément<<5 (posé dans setElementNumber)

    // Octet 0x18 recomposé À PARTIR DE L'ÉTAT DES 3 WIDGETS de l'op (pas de cache à resynchroniser
    // -> robuste au chargement de voix) : FMLPMS (sensPm b4~2) | PES (pegSw b1) | FPM (btFix b0).
    void send18 (int op)
    {
        MidiButton* fix[6] = { &btFix1,&btFix2,&btFix3,&btFix4,&btFix5,&btFix6 };
        const int pm   = (int) sensPm[op].getValue() & 0x07;
        const int pes  = pegSw[op].getToggleState() ? 1 : 0;
        const int fpm  = fix[op]->getToggleState() ? 1 : 0;
        const int v    = (pm << 2) | (pes << 1) | fpm;
        uint8 b[9] = { 0x43, 0x10, 0x34, (uint8) packedGroup[op], (uint8) packedAddrHi,
                       0x00, 0x18, 0x00, (uint8) (v & 0x7F) };
        packedSender.sendParam9 ("/SYSEX", b);
    }
    // Octet 0x19 recomposé : V1 = KOE/SYNC (btPhase) b0 ; V2 = PHASE (sliderPhase, 0~127).
    void send19 (int op)
    {
        MidiButton* syn[6] = { &btPhase1,&btPhase2,&btPhase3,&btPhase4,&btPhase5,&btPhase6 };
        MidiSlider* pha[6] = { &sliderPhase1,&sliderPhase2,&sliderPhase3,&sliderPhase4,&sliderPhase5,&sliderPhase6 };
        const int koe = syn[op]->getToggleState() ? 1 : 0;
        const int ph  = (int) pha[op]->getValue() & 0x7F;
        uint8 b[9] = { 0x43, 0x10, 0x34, (uint8) packedGroup[op], (uint8) packedAddrHi,
                       0x00, 0x19, (uint8) koe, (uint8) ph };
        packedSender.sendParam9 ("/SYSEX", b);
    }

    // Panneau algorithme embarqué (colonne droite) — réutilise toute la logique d'Operator
    // (sélecteur 1..45, schéma AlgoDraw, sysex ALGNUM avec offset -1). Masqué en mode zoom.
    Operator algoPanel;

    // Panneau LFO (Main + Sub) — colonne droite, sous l'algo. Masqué en mode zoom.
    AfmLfo lfoPanel;

    // Largeur réservée au panneau algorithme à droite ; layoutMaster() et paint()
    // doivent utiliser la même valeur pour rester cohérents.
    int algoPanelWidth() const { return jmax (180, getWidth() * 30 / 100); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oscillator)
};
