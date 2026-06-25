/*
 ==============================================================================
 
 Voice.h
 Created: 13 Nov 2018 5:16:09pm
 Author:  Seb & Ludo
 
 ==============================================================================
 */
#include "Element.h"
#include "Volume.h"
#include "PanVue.h"
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

        editPan1.setElementNumber(1, undoManager);
        addAndMakeVisible(editPan1);
        editPan1.setAlwaysOnTop(true);
        editPan1.setVisible(false);

        editPan2.setElementNumber(2, undoManager);
        addAndMakeVisible(editPan2);
        editPan2.setAlwaysOnTop(true);
        editPan2.setVisible(false);

        editPan3.setElementNumber(3, undoManager);
        addAndMakeVisible(editPan3);
        editPan3.setAlwaysOnTop(true);
        editPan3.setVisible(false);

        editPan4.setElementNumber(4, undoManager);
        addAndMakeVisible(editPan4);
        editPan4.setAlwaysOnTop(true);
        editPan4.setVisible(false);


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
        // Boutons « 1 2 3 4 » = on/off des 4 ÉLÉMENTS (MUTE éditeur via ELVL).
        addElementOnOff (op1);
        addElementOnOff (op2);
        addElementOnOff (op3);
        addElementOnOff (op4);
        op1.onClick = [this] { element1.setElementMuted (! op1.getToggleState()); };
        op2.onClick = [this] { element2.setElementMuted (! op2.getToggleState()); };
        op3.onClick = [this] { element3.setElementMuted (! op3.getToggleState()); };
        op4.onClick = [this] { element4.setElementMuted (! op4.getToggleState()); };
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

        // La carte ROUTAGE (contour accent + titre) est dessinée À LA MAIN dans
        // paintOverChildren : le LookAndFeel du thème dessine drawGroupComponentOutline en
        // SYPal.panelBorder (gris) en DUR et ignore outlineColourId -> impossible d'obtenir
        // l'accent via GroupComponent. grpAlgo n'est donc plus affiché (gardé pour les bornes).

        // Sélecteur d'Effect Mode (radio 0-3) en haut-droite du routage : change EFFECTMODE
        // (group 0x08 / param 0x00) directement -> le diagramme de topologie se redessine.
        addAndMakeVisible (radioMode);
        radioMode.addRadio ("0", 7777);
        radioMode.addRadio ("1", 7777);
        radioMode.addRadio ("2", 7777);
        radioMode.addRadio ("3", 7777);
        {
            int sx[9] = { 0x43, 0x10, 0x34, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00 };
            radioMode.setMidiSysex (sx);
        }
        radioMode.getValueObject().referTo (valueTreeVoice.getPropertyAsValue (IDs::EFFECTMODE, &undoManager));

        // Stereo Mix 1/2 on/off (group 0x08 / 0x1B-0x1C) : injecte le signal direct (dry).
        auto setupMix = [this] (MidiButton& b, const Identifier& id, int n2, const String& txt)
        {
            addAndMakeVisible (b);
            b.setTextOnOff (txt, txt);   // libellé conservé on ET off (sinon vidé au clic)
            b.setClickingTogglesState (true);
            b.getToggleStateValue().referTo (valueTreeVoice.getPropertyAsValue (id, &undoManager));
            int sx[9] = { 0x43, 0x10, 0x34, 0x08, 0x00, 0x00, n2, 0x00, 0x00 };
            b.setMidiSysex (sx);
        };
        setupMix (mixBtn1, IDs::STMIX1, 0x1B, "Stereo Mix");
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
   
        // Ferme TOUS les overlays d'édition des 4 éléments (avant : élément 2 masqué 3× par
        // copier-coller -> les overlays des éléments 3/4 ne se fermaient pas).
        WaveVue*   wave[4]   = { &editWave1, &editWave2, &editWave3, &editWave4 };
        AFMVue*    afm[4]    = { &editAfm1, &editAfm2, &editAfm3, &editAfm4 };
        FilterVue* filt[4]   = { &editFilter1, &editFilter2, &editFilter3, &editFilter4 };
        PanVue*    pan[4]    = { &editPan1, &editPan2, &editPan3, &editPan4 };
        for (int i = 0; i < 4; ++i)
        {
            wave[i]->setVisible (false);
            afm[i]->setVisible (false);
            filt[i]->setVisible (false);
            pan[i]->setVisible (false);
        }
    }
    
    // Sur changement de thème (sendLookAndFeelChange), ré-applique l'accent aux libellés qui
    // figent leur couleur via setColour (sinon ils gardent l'accent du thème de départ).
    void lookAndFeelChanged() override
    {
        labelOpOn.setColour (Label::ColourIds::textColourId, SYColSelected);
        labelName.setColour (Label::ColourIds::textColourId, SYColSelected);
        labelMode.setColour (Label::ColourIds::textColourId, SYColSelected);
        repaint();
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
            editFilter1.isVisible() || editFilter2.isVisible() || editFilter3.isVisible() || editFilter4.isVisible() ||
            editPan1.isVisible()    || editPan2.isVisible()    || editPan3.isVisible()    || editPan4.isVisible())
            return;

        // Carte ROUTAGE : contour ACCENT + titre, dessinés à la main (le LnF ignore l'accent
        // sur GroupComponent, cf. constructeur).
        Rectangle<float> algoArea ((float) algoX, (float) algoY, (float) algoW, (float) algoH);
        g.setColour (SYColSelected);
        g.drawRoundedRectangle (algoArea.reduced (1.0f), 8.0f, 1.5f);
        g.setColour (SYColLabel);
        g.setFont (Font (13.0f, Font::bold));
        g.drawText ("ROUTAGE", (int) algoX + 12, (int) algoY + 4, 160, 16, Justification::left);

        // ROUTAGE FIDÈLE (façon SynthWorks voice-routing) : chaque GROUPE de sortie a 2 unités
        // d'effet EN SÉRIE (Chorus/Mod -> Reverb) puis -> L/R. Les types affichés sont les VRAIS
        // (lus dans valueTreeVoice, édités dans l'onglet Effects) ; le spec ne les nomme pas ->
        // on affiche l'unité + le n° de type (« CHORUS 1 / t3 »). Group 1 = Chorus1->Reverb1,
        // Group 2 = Chorus2->Reverb2. Les lignes éléments->groupe suivent les bascules G1/G2.
        // ⚠️ La topologie exacte dépend de EFMODE (4 modes serial/parallel, non décrits au spec) :
        // on montre la chaîne série + l'étiquette « MODE n » (topologie inter-groupe approchée).
        const float bx0  = (float) algoX;
        const float inX  = bx0 + algoW * 0.11f;     // entrée des groupes
        const float outX = bx0 + algoW - 48.0f;     // sorties (marge pour les labels)
        const float aTop = (float) algoY + 30.0f;
        const float aBot = (float) (algoY + algoH) - 16.0f;
        const float g1Y  = aTop + (aBot - aTop) * 0.26f;
        const float g2Y  = aTop + (aBot - aTop) * 0.74f;
        const float boxH = jmin ((aBot - aTop) * 0.20f, 34.0f);

        auto eff = [this] (const Identifier& id) { return (int) valueTreeVoice.getProperty (id, 0); };
        const int mode = eff (IDs::EFFECTMODE);

        // Topologies des 4 EFMODE (RELEVÉES SUR LE SY77). Unités : 0=Chorus1(Mod1),
        // 1=Chorus2(Mod2), 2=Reverb1, 3=Reverb2. Chaque groupe = suite d'ÉTAGES ; un étage =
        // 1 unité (série) ou 2 (parallèle ∥). Liste d'étages vide = sortie DIRECTE (sans effet).
        //   1: g1=mod1->rev1              g2=mod2->rev2
        //   2: g1=mod1->rev1->rev2        g2=mod2
        //   3: g1=(mod1∥mod2)->rev1->rev2 g2=direct
        //   0: bypass (les deux directes)
        Array<Array<int>> chain1, chain2;
        auto stg = [] (std::initializer_list<int> u) { return Array<int> (u); };
        switch (mode)
        {
            case 1: chain1.add (stg ({ 0 })); chain1.add (stg ({ 2 }));
                    chain2.add (stg ({ 1 })); chain2.add (stg ({ 3 })); break;
            case 2: chain1.add (stg ({ 0 })); chain1.add (stg ({ 2 })); chain1.add (stg ({ 3 }));
                    chain2.add (stg ({ 1 }));                            break;
            case 3: chain1.add (stg ({ 0, 1 })); chain1.add (stg ({ 2 })); chain1.add (stg ({ 3 }));
                    /* chain2 = direct */                                break;
            default: break;   // 0 = bypass
        }

        // Lignes éléments -> entrée du groupe (matrice, pilotée par les bascules G1/G2).
        Element* els[4] = { &element1, &element2, &element3, &element4 };
        for (int i = 0; i < nombreElements && i < 4; ++i)
        {
            auto b = els[i]->getBounds().toFloat();
            const float ox = b.getRight() - 4.0f;            // sortie pan de l'élément
            const float lY = b.getY() + b.getHeight() * 0.32f;
            const float rY = b.getY() + b.getHeight() * 0.68f;
            const bool eg1 = (bool) valueTreeVoice.getProperty (Identifier ("ELEMENT" + String (i + 1) + "GROUP1"), false);
            const bool eg2 = (bool) valueTreeVoice.getProperty (Identifier ("ELEMENT" + String (i + 1) + "GROUP2"), false);
            g.setColour (SYColSelected);
            if (eg1) g.drawLine (ox, lY, inX, g1Y, 1.3f);
            if (eg2) g.drawLine (ox, rY, inX, g2Y, 1.3f);
        }

        // Une unité d'effet = CARTE cliquable (-> onglet Effects, cf. mouseUp/fxBoxes).
        fxBoxes.clearQuick();
        auto unitName = [] (int u) { static const char* const n[4] = { "CHORUS 1", "CHORUS 2", "REVERB 1", "REVERB 2" }; return String (n[u & 3]); };
        auto unitType = [&] (int u) { const Identifier ids[4] = { IDs::CHR1TYPE, IDs::CHR2TYPE, IDs::REV1TYPE, IDs::REV2TYPE }; return eff (ids[u & 3]); };
        const float bw = algoW * 0.15f;
        const float uh = jmin (boxH, 30.0f);
        auto drawUnitBox = [&] (Rectangle<float> r, int u)
        {
            SyDraw::drawPanel (g, r, SYColSelected);
            g.setColour (SYColBackground.contrasting());
            g.setFont (11.0f);
            g.drawFittedText (unitName (u) + "\nt" + String (unitType (u)), r.toNearestInt().reduced (3), Justification::centred, 2);
            fxBoxes.add (r);
        };

        // Chaîne d'un groupe (étages série/parallèle) -> sa sortie (1 point). `mixOn` = Stereo
        // Mix : injecte le signal DIRECT (dry) du pan en BYPASS des effets (tracé pointillé).
        auto drawChain = [&] (float y, const Array<Array<int>>& stages, bool mixOn)
        {
            g.setColour (SYColSelected);
            g.fillEllipse (inX - 2.5f, y - 2.5f, 5.0f, 5.0f);
            float prevX = inX;
            for (int s = 0; s < stages.size(); ++s)
            {
                const auto& st = stages[s];
                const float cx = inX + (outX - inX) * (s + 0.5f) / (float) stages.size();
                const float x0 = cx - bw * 0.5f, x1 = cx + bw * 0.5f;
                g.setColour (SYColSelected);
                g.drawLine (prevX, y, x0, y, 1.2f);
                if (st.size() <= 1)
                {
                    drawUnitBox ({ x0, y - uh * 0.5f, bw, uh }, st[0]);
                }
                else  // étage parallèle : fork à l'entrée, merge à la sortie
                {
                    const float off = uh * 0.66f;
                    for (int k = 0; k < st.size() && k < 2; ++k)
                    {
                        const float uy = y + (k == 0 ? -off : off);
                        g.setColour (SYColSelected);
                        g.drawLine (x0, y, x0, uy, 1.0f);
                        g.drawLine (x1, uy, x1, y, 1.0f);
                        drawUnitBox ({ x0, uy - uh * 0.5f, bw, uh }, st[k]);
                    }
                }
                prevX = x1;
            }
            g.setColour (SYColSelected);
            g.drawLine (prevX, y, outX, y, 1.2f);
            g.fillEllipse (outX - 3.0f, y - 3.0f, 6.0f, 6.0f);    // sortie du groupe (1 point)

            // Stereo Mix ON : signal direct (dry) injecté en bypass des effets -> ligne
            // pointillée de l'entrée à la sortie, décalée sous la chaîne.
            if (mixOn)
            {
                const float yd = y + boxH * 0.62f;
                g.setColour (SYColSelected.withAlpha (0.55f));
                g.drawLine (inX, y, inX, yd, 1.0f);
                const float dash[] = { 5.0f, 3.0f };
                Line<float> ln (inX, yd, outX, yd);
                g.drawDashedLine (ln, dash, 2, 1.0f);
                g.drawLine (outX, yd, outX, y, 1.0f);
            }
        };

        g.setColour (SYColLabel);
        g.setFont (11.0f);
        g.drawText ("GROUP 1", (int) inX - 6, (int) (g1Y - boxH * 0.5f - 24), 80, 13, Justification::left);
        g.drawText ("GROUP 2", (int) inX - 6, (int) (g2Y - boxH * 0.5f - 24), 80, 13, Justification::left);
        drawChain (g1Y, chain1, (bool) eff (IDs::STMIX1));
        drawChain (g2Y, chain2, false);   // un seul Stereo Mix sur le SY77 (groupe 1)

        // « Mode » : libellé près du sélecteur radio (top-right, posé dans resized).
        g.setColour (SYColLabel);
        g.setFont (11.0f);
        g.drawText ("Mode", radioMode.getX() - 44, radioMode.getY() + 2, 40, 16, Justification::right);
    }

    // Clic sur une boîte d'effet du routage -> ouvre l'onglet Effects pour l'éditer.
    void mouseUp (const MouseEvent& e) override
    {
        if (e.eventComponent != this || ! e.mouseWasClicked()) return;
        for (auto& r : fxBoxes)
            if (r.contains (e.position)) { openEffectsTab(); return; }
    }
    // Curseur « main » au survol d'une boîte d'effet (affordance : c'est cliquable).
    void mouseMove (const MouseEvent& e) override
    {
        bool over = false;
        for (auto& r : fxBoxes) if (r.contains (e.position)) over = true;
        setMouseCursor (over ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
    }
    void openEffectsTab()
    {
        if (auto* tabs = findParentComponentOfClass<TabbedComponent>())
        {
            const auto names = tabs->getTabNames();
            for (int i = 0; i < names.size(); ++i)
                if (names[i] == TRANS ("Effects")) { tabs->setCurrentTabIndex (i); return; }
        }
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
        if(element1.elementValue == Element::commande::PanEdit)
            {
                editPan1.setVisible(true);
                editPan1.refresh();
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
            if(element2.elementValue == Element::commande::PanEdit)
            {
                editPan2.setVisible(true);
                editPan2.refresh();
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
            if(element3.elementValue == Element::commande::PanEdit)
            {
                editPan3.setVisible(true);
                editPan3.refresh();
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
            if(element4.elementValue == Element::commande::PanEdit)
            {
                editPan4.setVisible(true);
                editPan4.refresh();
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
        Element*    els[4] = { &element1, &element2, &element3, &element4 };
        TextButton* ons[4] = { &op1, &op2, &op3, &op4 };
        for (int i = 0; i < 4; ++i)
        {
            const bool active = (i < nombre);
            els[i]->setVisible (true);
            els[i]->setEnabled (active);
            els[i]->setAlpha (active ? 1.0f : 0.35f);

            // Bouton on/off : actif = allumé/cliquable, hors mode courant = grisé/désactivé.
            ons[i]->setEnabled (active);
            ons[i]->setAlpha (active ? 1.0f : 0.35f);
            if (! active)
            {
                // Élément absent du type de voix : repli en état « on » (non muté) sans envoi.
                ons[i]->setToggleState (true, dontSendNotification);
                els[i]->setElementMuted (false);
            }
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

        editPan1.setBounds(boundsZone);
        editPan2.setBounds(boundsZone);
        editPan3.setBounds(boundsZone);
        editPan4.setBounds(boundsZone);

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
        // Sélecteur d'Effect Mode (radio) + toggles Stereo Mix en haut-droite de la carte routage.
        radioMode.setBounds (algoX + algoW - 156, algoY + 3, 148, 20);
        mixBtn1.setBounds (algoX + algoW - 156, algoY + 26, 148, 18);
    }
    void addBtAndMakeStyle (TextButton& textButton)
    {
        textButton.setClickingTogglesState(true);
        textButton.setColour(TextButton::ColourIds::buttonOnColourId, Colours::red);
        addAndMakeVisible (textButton);
    }
    // Bouton on/off d'ÉLÉMENT : toggle ON = élément actif (accent thème), OFF = muté.
    void addElementOnOff (TextButton& bt)
    {
        bt.setClickingTogglesState (true);
        bt.setToggleState (true, dontSendNotification);  // élément actif par défaut
        bt.setColour (TextButton::ColourIds::buttonOnColourId, SYColSelected);
        addAndMakeVisible (bt);
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

    // Éditeurs d'EG de pan (overlay plein-page, un par élément), ouverts via PanEdit.
    PanVue editPan1;
    PanVue editPan2;
    PanVue editPan3;
    PanVue editPan4;

    int grid;
    MidiSlider sliderMaster;
    
    ComboBox    comboMode;
    TextEditor  editName {TRANS("Edit Name")};
    Value       voiceNameVal; // -> VOICENAME (nom chargé depuis la librairie)
    // Boutons « 1 2 3 4 » (libellé « Operateurs On-Off ») = on/off des 4 ÉLÉMENTS de la voix.
    // MUTE éditeur via ELVL (cf. Element::setElementMuted) ; pas de vrai param on/off élément.
    TextButton op1 {"1"};
    TextButton op2 {"2"};
    TextButton op3 {"3"};
    TextButton op4 {"4"};
    

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
    GroupComponent grpAlgo     { "grpAlgo",    TRANS ("ROUTAGE") };
    int algoX = 0, algoY = 0, algoW = 0, algoH = 0; // zone de la matrice (utilisée par paint)
    Array<Rectangle<float>> fxBoxes; // unités d'effet dessinées (cliquables -> onglet Effects)
    MidiRadio radioMode;             // sélecteur d'Effect Mode 0-3 (group 0x08/0x00) dans le routage
    MidiButton mixBtn1;              // Stereo Mix on/off (group 0x08 / 0x1B) — sur le groupe 1

    int nombreElements = 1;
    SysexBusSender sender;  // [2]

    
    UndoManager undoManager;
};

//==============================================================================
