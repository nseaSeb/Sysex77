/*
 ==============================================================================
 
 Voice.h
 Created: 13 Nov 2018 5:16:09pm
 Author:  Seb & Ludo
 
 ==============================================================================
 */
#include "Element.h"
#include "Volume.h"
#pragma once

//==============================================================================
struct VoicePage   : public Component, public Slider::Listener, public ComboBox::Listener, public TextEditor::Listener, public TextButton::Listener,public ChangeListener, public ChangeBroadcaster, public Value::Listener, public ValueTree::Listener, public KeyListener , public Timer
{
    VoicePage()
    {
        
        setOpaque(false);
        
        editAfm1.setElementNumber(1,undoManager);
        addAndMakeVisible(editAfm1);
        editAfm1.setAlwaysOnTop(true);
        editAfm1.setVisible(false);
        

        editAfm2.setElementNumber(2,undoManager);
        addAndMakeVisible(editAfm2);
        editAfm2.setAlwaysOnTop(true);
        editAfm2.setVisible(false);
        
        editAfm3.setElementNumber(3,undoManager);
        addAndMakeVisible(editAfm3);
        editAfm3.setAlwaysOnTop(true);
        editAfm3.setVisible(false);
        
        editAfm4.setElementNumber(4,undoManager);
        addAndMakeVisible(editAfm4);
        editAfm4.setAlwaysOnTop(true);
        editAfm4.setVisible(false);
        
        editFilter1.setElementNumber(1, undoManager);
        addAndMakeVisible(editFilter1);
        editFilter1.setAlwaysOnTop(true);
        editFilter1.setVisible(false);
        
        editFilter2.setElementNumber(2, undoManager);
        addAndMakeVisible(editFilter2);
        editFilter2.setAlwaysOnTop(true);
        editFilter2.setVisible(false);
        
        editFilter3.setElementNumber(3, undoManager);
        addAndMakeVisible(editFilter3);
        editFilter3.setAlwaysOnTop(true);
        editFilter3.setVisible(false);
        
        editFilter4.setElementNumber(4, undoManager);
        addAndMakeVisible(editFilter4);
        editFilter4.setAlwaysOnTop(true);
        editFilter4.setVisible(false);
        
        editWave1.setElementNumber(1, undoManager);
        addAndMakeVisible(editWave1);
        editWave1.setAlwaysOnTop(true);
        editWave1.setVisible(false);
        
        editWave2.setElementNumber(2, undoManager);
        addAndMakeVisible(editWave2);
        editWave2.setAlwaysOnTop(true);
        editWave2.setVisible(false);
        
        editWave3.setElementNumber(3, undoManager);
        addAndMakeVisible(editWave3);
        editWave3.setAlwaysOnTop(true);
        editWave3.setVisible(false);
        
        editWave4.setElementNumber(4, undoManager);
        addAndMakeVisible(editWave4);
        editWave4.setAlwaysOnTop(true);
        editWave4.setVisible(false);
        
        

        addAndMakeVisible(comboMode);
        
        addAndMakeVisible(element1);
        addAndMakeVisible(element2);
        addAndMakeVisible(element3);
        addAndMakeVisible(element4);
        
        element1.setOpNumber(1, undoManager);
        element2.setOpNumber(2, undoManager);
        element3.setOpNumber(3, undoManager);
        element4.setOpNumber(4, undoManager);
        
        element1.addChangeListener(this);
        element2.addChangeListener(this);
        element3.addChangeListener(this);
        element4.addChangeListener(this);
        
        element1.elementValue.addListener(this);
        element2.elementValue.addListener(this);
        element3.elementValue.addListener(this);
        element4.elementValue.addListener(this);
        
        comboMode.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::VOICEMODE, &undoManager));
        comboMode.setEditableText (false);
        comboMode.setJustificationType (Justification::centredBottom);
        comboMode.addListener(this);
        comboMode.addItem("1 AFM MONO", 1);
        comboMode.addItem("2 AFM MONO", 2);
        comboMode.addItem("4 AFM MONO", 3);
        comboMode.addItem("1 AFM POLY", 4);
        comboMode.addItem("2 AFM POLY", 5);
        comboMode.addItem("1 AWM POLY", 6);
        comboMode.addItem("2 AWM POLY", 7);
        comboMode.addItem("4 AWM POLY", 8);
        comboMode.addItem("1 AFM & 1 AWM POLY", 9);
        comboMode.addItem("2 AFM & 2 AWM POLY", 10);
        comboMode.setSelectedId(1);
        
       
 
    
        addAndMakeVisible(editName);
        editName.setText("INIT");
        editName.setMultiLine(false);
        editName.setInputRestrictions(10);
        editName.setJustification(Justification::centred);
        //      editName.setColour(TextEditor::ColourIds::textColourId, SYColText);
        editName.addListener(this);
        // Nom de voix chargé depuis la librairie : suit VOICENAME -> met à jour le champ.
        voiceNameVal = valueTreeVoice.getPropertyAsValue (IDs::VOICENAME, &undoManager);
        voiceNameVal.addListener (this);
        
    int sysexdata[9] = { 0x43, 0X10, 0x34, 0x0d, 0x00, 0x00, 0x32, 0x00, 0x7f };
        op1.setMidiSysex(sysexdata);
        addAndMakeVisible(op1);
        op1.setTextOnOff("1", "1");
        
        addBtAndMakeStyle(op2);
        addBtAndMakeStyle(op3);
        addBtAndMakeStyle(op4);
        addLabelStyle(labelOpOn);
        addLabelStyle(labelMode);
        addLabelStyle(labelName);

        for (auto* l : { &labColPitch, &labColWave, &labColFilter, &labColVolume, &labColPan })
        {
            l->setJustificationType (Justification::centred);
            l->setColour (Label::textColourId, SYColLabel);
            l->setFont (Font (12.0f, Font::bold));
            addAndMakeVisible (*l);
        }

        // Cadres décoratifs de la colonne droite (transparents, ne captent pas la souris).
        for (auto* gp : { &grpAlgo })
        {
            gp->setColour (GroupComponent::textColourId,    SYColLabel);
            gp->setColour (GroupComponent::outlineColourId, SYColLabel);
            gp->setInterceptsMouseClicks (false, false);
            addAndMakeVisible (*gp);
        }
        labelMode.setVisible (false); // remplacé par le cadre "VOICE"

        
// init master volume slider
//   { 0x43, 0x10, 0x34, 0x02, 0x00, 0x00, 0x3f, 0x00, 0x00 };
        sysexdata[3] = 0x02;
        sysexdata[6] = 0x3f;
        sliderMaster.setMidiSysex(sysexdata);
        sliderMaster.setRangeAndRound(0, 127, 127) ;
        sliderMaster.setSliderStyle(MidiSlider::SliderStyle::LinearHorizontal);
        sliderMaster.setPopupDisplayEnabled(true, true, nullptr);
        
        addAndMakeVisible(sliderMaster);
        labelMasterVolume.attachToComponent(&sliderMaster, true);
        
        Value valueCommonVolume = valueTreeVoice.getPropertyAsValue(IDs::COMMONVOLUME.toString(), &undoManager);
        sliderMaster.getValueObject().referTo(valueCommonVolume);

        valueTreeVoice.addListener(this);
        addKeyListener(this); //to manage undo redo key
        startTimer(500);
        // specify here where to send OSC messages to: host URL and UDP port number
        
  
        
    }
    ~VoicePage()
    {
        stopTimer();
          valueTreeVoice.removeListener(this);
          removeKeyListener(this);
        element1.removeChangeListener(this);
        element2.removeChangeListener(this);
        element3.removeChangeListener(this);
        element4.removeChangeListener(this);
        editName.removeListener(this);
        comboMode.removeListener(this);
 
    }
    
    #include "ValueTrees.h"
    
        virtual void broughtToFront() override
    {
    Logger::writeToLog("Voice: BroughtToFront");
   
        editWave1.setVisible(false);
        editAfm1.setVisible(false);
        editFilter1.setVisible(false);
    
        editWave2.setVisible(false);
        editAfm2.setVisible(false);
        editFilter2.setVisible(false);

        editWave2.setVisible(false);
        editAfm2.setVisible(false);
        editFilter2.setVisible(false);

        editWave2.setVisible(false);
        editAfm2.setVisible(false);
        editFilter2.setVisible(false);

        
    }
    
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
       
        
        Logger::writeToLog("Voice: changeListener");
    //    Logger::writeToLog(source);
    //    test.setVisible(true);
        
    }
    // Routage façon SynthWorks : on relie les VRAIES sorties pan L/R des éléments
    // actifs aux bus Reverb Hall/Room, puis aux sorties L/R. Dessiné ici (et pas dans
    // un sous-composant) car VoicePage connaît la position des éléments ET de la zone algo.
    // Dessiné APRÈS les enfants : le routage/algorithme passe par-dessus la carte ALGORITHME
    // (et les cartes d'éléments), au lieu d'être masqué derrière.
    void paintOverChildren (Graphics& g) override
    {
        if (algoW <= 0)
            return;

        // Ne pas dessiner le routage si un éditeur est ouvert par-dessus
        if (editAfm1.isVisible()    || editAfm2.isVisible()    || editAfm3.isVisible()    || editAfm4.isVisible()    ||
            editWave1.isVisible()   || editWave2.isVisible()   || editWave3.isVisible()   || editWave4.isVisible()   ||
            editFilter1.isVisible() || editFilter2.isVisible() || editFilter3.isVisible() || editFilter4.isVisible())
            return;

        const float bx0  = (float) algoX;
        const float boxL = bx0 + algoW * 0.34f;
        const float boxR = bx0 + algoW * 0.74f;
        const float outX = bx0 + algoW - 16.0f;
        const float aTop = (float) algoY + 22.0f;
        const float aBot = (float) (algoY + algoH) - 14.0f;
        const float hallY = aTop + (aBot - aTop) * 0.30f;
        const float roomY = aTop + (aBot - aTop) * 0.70f;
        const float boxH  = jmin ((aBot - aTop) * 0.26f, 40.0f);

        Element* els[4] = { &element1, &element2, &element3, &element4 };
        for (int i = 0; i < nombreElements && i < 4; ++i)
        {
            auto b = els[i]->getBounds().toFloat();
            const float ox = b.getRight() - 4.0f;            // sortie pan de l'élément
            const float lY = b.getY() + b.getHeight() * 0.32f;
            const float rY = b.getY() + b.getHeight() * 0.68f;
            // Routage piloté par les bascules de groupe de l'élément :
            // Groupe 1 -> Reverb Hall, Groupe 2 -> Reverb Room (aucune = non routé).
            const bool g1 = (bool) valueTreeVoice.getProperty (Identifier ("ELEMENT" + String (i + 1) + "GROUP1"), false);
            const bool g2 = (bool) valueTreeVoice.getProperty (Identifier ("ELEMENT" + String (i + 1) + "GROUP2"), false);
            g.setColour (SYColSelected);
            if (g1) g.drawLine (ox, lY, boxL, hallY, 1.3f);  // -> Reverb Hall (groupe 1)
            if (g2) g.drawLine (ox, rY, boxL, roomY, 1.3f);  // -> Reverb Room (groupe 2)
        }

        g.setColour (SYColLabel);
        auto box = [&] (float cy, const String& t)
        {
            Rectangle<float> r (boxL, cy - boxH * 0.5f, boxR - boxL, boxH);
            g.drawRect (r, 1.0f);
            g.drawFittedText (t, r.toNearestInt(), Justification::centred, 2);
        };
        box (hallY, "REVERB\nHALL");
        box (roomY, "REVERB\nROOM");

        g.drawLine (boxR, hallY, outX, aTop, 1.0f);          // -> L
        g.drawLine (boxR, roomY, outX, aBot, 1.0f);          // -> R
        g.drawText ("L", (int) outX, (int) aTop - 8, 14, 14, Justification::left);
        g.drawText ("R", (int) outX, (int) aBot - 6, 14, 14, Justification::left);
    }
    void valueChanged(Value & value) override
    {
        // Nom de voix chargé depuis la librairie -> reflète dans le champ d'édition.
        if (value.refersToSameSourceAs (voiceNameVal))
        {
            const auto n = voiceNameVal.getValue().toString();
            if (n.isNotEmpty())
                editName.setText (n, dontSendNotification);
        }

        if(value.refersToSameSourceAs(element1.elementValue))
        {
            Logger::writeToLog("Value change element1");
        if(element1.elementValue == Element::commande::WaveEdit)
            editWave1.setVisible(true);
        if(element1.elementValue == Element::commande::AfmEdit)
                editAfm1.setVisible(true);
        if(element1.elementValue == Element::commande::FilterEdit)
        {
                editFilter1.setAwmMode (element1.getOpMode() == Element::mode::AWM);
                editFilter1.setVisible(true);
        }
        if(element1.elementValue == Element::commande::VolumeEdit)
        {
            //editVolume1.setVisible(true);
            editWave1.setVisible(true);
            editWave1.setTabVolume();
        }
        if(element1.elementValue == Element::commande::VolumeAFM)
            {
                //editVolume1.setVisible(true);
                editAfm1.setVisible(true);
                editAfm1.setTabVolume();
            }
            
             element1.elementValue =0;
        }



        
        if(value.refersToSameSourceAs(element2.elementValue))
        {
                      Logger::writeToLog("Value change element2");
            if(element2.elementValue == Element::commande::WaveEdit)
                editWave2.setVisible(true);
            if(element2.elementValue == Element::commande::AfmEdit)
                editAfm2.setVisible(true);
            if(element2.elementValue == Element::commande::FilterEdit)
            {
                editFilter2.setAwmMode (element2.getOpMode() == Element::mode::AWM);
                editFilter2.setVisible(true);
            }
            if(element2.elementValue == Element::commande::VolumeEdit)
            {
                editWave2.setVisible(true);
                editWave2.setTabVolume();
            }
            if(element2.elementValue == Element::commande::VolumeAFM)
            {
                //editVolume1.setVisible(true);
                editAfm2.setVisible(true);
                editAfm2.setTabVolume();
            }
             element2.elementValue =0;
        }
 
        if(value.refersToSameSourceAs(element3.elementValue))
        {
            Logger::writeToLog("Value change element3");
            if(element3.elementValue == Element::commande::WaveEdit)
                editWave3.setVisible(true);
            if(element3.elementValue == Element::commande::AfmEdit)
                editAfm3.setVisible(true);
            if(element3.elementValue == Element::commande::FilterEdit)
            {
                editFilter3.setAwmMode (element3.getOpMode() == Element::mode::AWM);
                editFilter3.setVisible(true);
            }
            if(element3.elementValue == Element::commande::VolumeEdit)
            {
                editWave3.setVisible(true);
                editWave3.setTabVolume();
            }
            if(element3.elementValue == Element::commande::VolumeAFM)
            {
                //editVolume1.setVisible(true);
                editAfm3.setVisible(true);
                editAfm3.setTabVolume();
            }
             element3.elementValue =0;
        }
        
        if(value.refersToSameSourceAs(element4.elementValue))
        {
            Logger::writeToLog("Value change element4");
            if(element4.elementValue == Element::commande::WaveEdit)
                editWave4.setVisible(true);
            if(element4.elementValue == Element::commande::AfmEdit)
                editAfm4.setVisible(true);
            if(element4.elementValue == Element::commande::FilterEdit)
            {
                editFilter4.setAwmMode (element4.getOpMode() == Element::mode::AWM);
                editFilter4.setVisible(true);
            }
            if(element4.elementValue == Element::commande::VolumeEdit)
            {
                editWave4.setVisible(true);
                editWave4.setTabVolume();
            }
            if(element4.elementValue == Element::commande::VolumeAFM)
            {
                //editVolume1.setVisible(true);
                editAfm4.setVisible(true);
                editAfm4.setTabVolume();
            }
             element4.elementValue =0;
        }
    }
    void buttonClicked (Button* button) override
    {
            Logger::writeToLog("Voice: ButtonClicked");
    }
    void textEditorReturnKeyPressed	(	TextEditor & 	editText	) override
    {
        Logger::writeToLog("Voice TextEditor return");
        // Nom de voix = 10 caractères -> Voice Common (groupe 0x02), params VNAM0..VNAM9
        // = 0x01..0x0A (spec « SY77 MIDI Data Format », table 1-3). Le nom est complété
        // à droite par des espaces (convention Yamaha). Un message par caractère.
        const String name = editText.getText();
        oscMidiMessage.clearQuick();
        for (int i = 0; i < 10; ++i)
        {
            const juce::uint8 ch = (juce::uint8) (i < name.length()
                                                  ? (int) (juce::juce_wchar) name[i]
                                                  : 0x20); // espace de remplissage
            oscMidiMessage.set (i, SyVoice::paramMessage (sysexDeviceNumber, 0x02, 0x00, 0x00,
                                                          (juce::uint8) (0x01 + i), ch));
        }
        sender.send (oscSendMidiMessage, (int) oscMidiMessage.size());
        }
  

    
    void textEditorFocusLost	(	TextEditor & 		) override
    {
        Logger::writeToLog("Voice TextEditor lostFocus");
    }
    void 	sliderValueChanged (Slider *slider) override
    {
    
        
        
 }
    
    void comboBoxChanged	(	ComboBox * 	comboBoxThatHasChanged	) override
    {
        Logger::writeToLog("Voice comboBox Changed");
        if(comboBoxThatHasChanged == &comboMode)
        {
            if (! sender.send (adresseOpMode, (int) comboMode.getSelectedItemIndex())) // [5]
                Logger::writeToLog ("OSC erreur voice opMode");;
 
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMMono1)
            {
                setNombreElements(Element::mode::AFMmono);
                element1.setOpMode(1);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMMono2)
            {
                setNombreElements(2);
                                element1.setOpMode(Element::mode::AFMmono);
                                element2.setOpMode(Element::mode::AFMmono);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMMono4)
            {
                setNombreElements(4);
                element1.setOpMode(Element::mode::AFMmono);
                element2.setOpMode(Element::mode::AFMmono);
                element3.setOpMode(Element::mode::AFMmono);
                element4.setOpMode(Element::mode::AFMmono);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFM2AWM2)
            {
                setNombreElements(4);
                element1.setOpMode(Element::mode::AFMPoly);
                element2.setOpMode(Element::mode::AFMPoly);
                element3.setOpMode(Element::mode::AWM);
                element4.setOpMode(Element::mode::AWM);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AWMPoly4)
            {
                setNombreElements(4);
                element1.setOpMode(Element::mode::AWM);
                element2.setOpMode(Element::mode::AWM);
                element3.setOpMode(Element::mode::AWM);
                element4.setOpMode(Element::mode::AWM);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AWMPoly1)
            {
                setNombreElements(1);
                element1.setOpMode(Element::mode::AWM);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AWMPoly2)
            {
                setNombreElements(2);
                element1.setOpMode(Element::mode::AWM);
                element2.setOpMode(Element::mode::AWM);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMPoly2)
            {
                setNombreElements(2);
                element1.setOpMode(Element::mode::AFMPoly);
                element2.setOpMode(Element::mode::AFMPoly);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFM1AWM1)
            {
                setNombreElements(2);
                element1.setOpMode(Element::mode::AFMPoly);
                element2.setOpMode(Element::mode::AWM);            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMPoly1)
            {
                setNombreElements(1);
                element1.setOpMode(Element::mode::AFMPoly);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMPoly2)
            {
                setNombreElements(2);
                element1.setOpMode(Element::mode::AFMPoly);
                element2.setOpMode(Element::mode::AFMPoly);
            }
        }

    }
void setNombreElements (int nombre)
    {
        nombreElements = nombre;
        // Façon SynthWorks : on affiche toujours les 4 rangées ; les éléments hors du
        // mode courant sont grisés et non interactifs (structure visible en permanence).
        Element* els[4] = { &element1, &element2, &element3, &element4 };
        for (int i = 0; i < 4; ++i)
        {
            const bool active = (i < nombre);
            els[i]->setVisible (true);
            els[i]->setEnabled (active);
            els[i]->setAlpha (active ? 1.0f : 0.35f);
        }
        Logger::writeToLog("setNombre elements");
        resized();
        repaint(); // redessine le routage (dépend du nombre d'éléments actifs)
    }

    void resized() override
    {
       Logger::writeToLog("Voice: Resized");
        auto boundsZone = getBounds();
        boundsZone.setY(0);
        //boundsZone.setHeight(getHeight()+20);
        editWave1.setBounds(boundsZone);
        editWave2.setBounds(boundsZone);
        editWave3.setBounds(boundsZone);
        editWave4.setBounds(boundsZone);
       
        editFilter1.setBounds(boundsZone);
        editFilter2.setBounds(boundsZone);
        editFilter3.setBounds(boundsZone);
        editFilter4.setBounds(boundsZone);
        editAfm1.setBounds(boundsZone);
        editAfm2.setBounds(boundsZone);
        editAfm3.setBounds(boundsZone);
        editAfm4.setBounds(boundsZone);
        
        grid = getWidth()/10;
        auto wGrid = (getWidth()-20) * 0.7;
        const int topElem = 68;                 // sous la barre du haut + les en-têtes de colonnes

        // En-têtes de colonnes alignés sur les fractions internes d'un Element :
        // PITCH .18 | WAVE .18 | FILTER .26 | VOLUME .26 | PAN .12
        const int hy = 48, hh = 18;
        auto fx = [wGrid] (float f) { return 10 + (int) (wGrid * f); };
        labColPitch .setBounds (fx (0.00f), hy, fx (0.18f) - fx (0.00f), hh);
        labColWave  .setBounds (fx (0.18f), hy, fx (0.36f) - fx (0.18f), hh);
        labColFilter.setBounds (fx (0.36f), hy, fx (0.62f) - fx (0.36f), hh);
        labColVolume.setBounds (fx (0.62f), hy, fx (0.88f) - fx (0.62f), hh);
        labColPan   .setBounds (fx (0.88f), hy, fx (1.00f) - fx (0.88f), hh);

        // Rangées : les éléments ACTIFS reçoivent la majeure partie de la hauteur,
        // les inactifs sont réduits à une fine bande (l'algo/les contrôles de l'élément
        // actif restent grands quelle que soit la taille de l'écran).
        Element* els[4] = { &element1, &element2, &element3, &element4 };
        const int nAct      = jlimit (1, 4, nombreElements);
        const int inactiveH = 30;
        const int avail     = jmax (80, getHeight() - topElem - 2);
        const int actH      = jmax (60, (avail - (4 - nAct) * inactiveH) / nAct);
        int yEl = topElem;
        for (int i = 0; i < 4; ++i)
        {
            const int h = (i < nAct) ? actH : inactiveH;
            els[i]->setBounds (10, yEl, (int) wGrid, h);
            yEl += h;
        }
        
        op1.setBounds (10,4,24,24);
        op2.setBounds (10+24, 4,24,24);
        op3.setBounds (10+48, 4,24,24);
        op4.setBounds (10+72, 4,24,24);
        editName.setBounds(20 + 96, 4, 100, 24);
        labelName.setBounds(116,20,100,24);
        
        labelOpOn.setBounds(10, 20, 96, 24);

        // --- Colonne droite (façon SynthWorks) : VOICE (mode + master) / ALGO / EFFETS ---
        const int rightX = 10 + (int) wGrid + 12;
        const int rightW = jmax (40, getWidth() - rightX - 10);
        // Mode + Master tout en haut (l'algo récupère toute la hauteur, comme l'original).
        comboMode.setBounds (rightX, 6, rightW, 24);
        labelMasterVolume.setBounds (rightX, 34, 56, 18);
        sliderMaster.setBounds (rightX + 58, 36, rightW - 58, 16);

        // Algo/routage : du sommet des rangées d'éléments jusqu'en bas. Le routage est
        // dessiné dans paint() (lignes depuis les vraies sorties pan des éléments).
        algoX = rightX;
        algoY = topElem;
        algoW = rightW;
        algoH = jmax (60, getHeight() - topElem - 4);
        grpAlgo.setBounds (algoX, algoY, algoW, algoH);
    }
    void addBtAndMakeStyle (TextButton& textButton)
    {
        textButton.setClickingTogglesState(true);
        textButton.setColour(TextButton::ColourIds::buttonOnColourId, Colours::red);
        addAndMakeVisible (textButton);
    }
    void addAndMakePitch ( Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setLookAndFeel(nullptr);   // suit le ModernLookAndFeel du thème
        slider.addListener(this);
        slider.setRange(0, 127);
        slider.setNumDecimalPlacesToDisplay(0);
    }
    void addSliderStyleV (Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setPopupDisplayEnabled(true, true, this);
        slider.setPopupDisplayEnabled(true, true, this);
        slider.setColour(Slider::ColourIds::thumbColourId, Colours::red);
        slider.setColour(Slider::ColourIds::trackColourId, SYColSelected);
        slider.setRange(0, 127);
        slider.setNumDecimalPlacesToDisplay(0);
        slider.addListener(this);
    }
    void addButtonState (TextButton& bt)
    {
        addAndMakeVisible(bt);
        bt.setColour(TextButton::ColourIds::buttonOnColourId, SYColSelected);
        bt.setClickingTogglesState(true);
        bt.addListener(this);
    }
    void addLabelStyle (Label& label)
    {
        label.setJustificationType(Justification::centredBottom);
        label.setColour(Label::ColourIds::textColourId, SYColSelected);
        addAndMakeVisible (label);
    }

    enum voiceMode
    {
        AFMMono1 = 0,
        AFMMono2 = 1,
        AFMMono4 = 2,
        AFMPoly1 = 3,
        AFMPoly2 = 4,
        AWMPoly1 = 5,
        AWMPoly2 = 6,
        AWMPoly4 = 7,
        AFM1AWM1 = 8,
        AFM2AWM2 = 9,
        DrumSet = 10,
        
    };
    
  
    WaveVue editWave1;
    WaveVue editWave2;
    WaveVue editWave3;
    WaveVue editWave4;


    
    FilterVue editFilter1;
    FilterVue editFilter2;
    FilterVue editFilter3;
    FilterVue editFilter4;
    
    AFMVue editAfm1;
    AFMVue editAfm2;
    AFMVue editAfm3;
    AFMVue editAfm4;
    
    int grid;
    MidiSlider sliderMaster;
    
    ComboBox    comboMode;
    TextEditor  editName {TRANS("Edit Name")};
    Value       voiceNameVal; // -> VOICENAME (nom chargé depuis la librairie)
    MidiButton op1;
    TextButton op2 {"2"};
    TextButton op3{"3"};
    TextButton op4{"4"};
    

    Element element1;
    Element element2;
    Element element3;
    Element element4;
    
    Label labelOpOn{"-Operateurs-", "Operateurs On-Off"};
    Label labelName {" ", TRANS("Name")};
    Label labelMode{"", "Operateurs Mode"};

    
    Label   labelMasterVolume{"M",TRANS("Master Volume")};

    // En-têtes de colonnes (façon SynthWorks) au-dessus des rangées d'éléments.
    Label labColPitch  { "", "PITCH" };
    Label labColWave   { "", "WAVE" };
    Label labColFilter { "", "FILTER" };
    Label labColVolume { "", "VOLUME" };
    Label labColPan    { "", "PAN" };

    // Colonne droite (façon SynthWorks).
    GroupComponent grpAlgo     { "grpAlgo",    TRANS ("ALGORITHME / ROUTAGE") };
    int algoX = 0, algoY = 0, algoW = 0, algoH = 0; // zone de la matrice (utilisée par paint)

    int nombreElements = 1;
    SysexBusSender sender;  // [2]

    
    UndoManager undoManager;
};

//==============================================================================
