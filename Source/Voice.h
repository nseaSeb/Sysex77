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
/*  En-tete commun des editeurs ouverts par-dessus l'ecran Voice (AFM, Wave, Filtre, Pan).
    Une seule convention partout : en haut a gauche un bouton retour « < Voice » puis un fil
    d'Ariane discret « VOICE / Element 1 / AFM ». Remplace l'ancien faux onglet « Close » de
    chaque editeur (place a une position differente selon le nombre d'onglets). Le bouton ne
    fait qu'appeler onBack -> VoicePage::closeOpenEditor() (meme chemin que l'onglet VOICE
    re-clique et la touche Echap). Couleurs lues dans SYPal au paint : suit le theme. */
struct EditorHeader : public Component
{
    struct BackButton : public Button
    {
        BackButton() : Button ("back") { setMouseCursor (MouseCursor::PointingHandCursor); }
        void paintButton (Graphics& g, bool over, bool down) override
        {
            auto r = getLocalBounds().toFloat().reduced (0.5f, 2.5f);
            const auto accent = SYPal.accent;
            g.setColour (accent.withAlpha (down ? 0.30f : over ? 0.18f : 0.08f));
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (accent.withAlpha (over ? 0.9f : 0.55f));
            g.drawRoundedRectangle (r, 4.0f, 1.0f);

            // Chevron « < » dessine (pas de glyphe : rendu net quelle que soit la police).
            const float cx = r.getX() + 12.0f, cy = r.getCentreY();
            Path p;
            p.startNewSubPath (cx + 3.0f, cy - 5.0f);
            p.lineTo (cx - 2.0f, cy);
            p.lineTo (cx + 3.0f, cy + 5.0f);
            g.setColour (over ? SYPal.textPrimary : accent);
            g.strokePath (p, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));

            g.setFont (Font (FontOptions (13.0f)).boldened());
            g.drawText (getButtonText(), r.withTrimmedLeft (22.0f).withTrimmedRight (6.0f),
                        Justification::centredLeft, false);
        }
    };

    EditorHeader()
    {
        addAndMakeVisible (btBack);
        btBack.setButtonText (TRANS ("Voice").trim());
        btBack.onClick = [this] { if (onBack) onBack(); };
        refreshTooltip();
    }

    void refreshTooltip() { btBack.setTooltip (TRANS ("Back to the voice overview") + Shortcuts::hint ("close")); }

    void setPath (const StringArray& crumbs) { path = crumbs; repaint(); }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6, 2);
        const int w = roundToInt (GlyphArrangement::getStringWidth (Font (FontOptions (13.0f)).boldened(), btBack.getButtonText())) + 34;
        btBack.setBounds (r.removeFromLeft (jmax (78, w)));
    }

    void paint (Graphics& g) override
    {
        g.fillAll (SYPal.background);
        g.setColour (SYPal.panelBorder.withAlpha (0.6f));
        g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());

        // Fil d'Ariane : segments discrets, le dernier (editeur courant) en clair.
        auto r = getLocalBounds().withTrimmedLeft (btBack.getRight() + 14).reduced (0, 2);
        Font f (FontOptions (13.0f));
        for (int i = 0; i < path.size(); ++i)
        {
            const bool last = i == path.size() - 1;
            const auto seg = path[i];
            g.setFont (last ? f.boldened() : f);
            g.setColour (last ? SYPal.textPrimary : SYPal.textMuted);
            const int w = roundToInt (GlyphArrangement::getStringWidth (g.getCurrentFont(), seg)) + 2;
            g.drawText (seg, r.removeFromLeft (w), Justification::centredLeft, true);
            if (! last)
            {
                g.setFont (f);
                g.setColour (SYPal.textMuted.withAlpha (0.6f));
                g.drawText ("/", r.removeFromLeft (18), Justification::centred, false);
            }
        }
    }

    std::function<void()> onBack;
    BackButton btBack;
    StringArray path;
};

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
        editName.setTextToShowWhenEmpty (TRANS ("(unknown voice)"), Colours::grey);
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
        for (auto* b : { &op1, &op2, &op3, &op4 })
            b->setTooltip (TRANS ("Element on/off (mute)"));
        addLabelStyle(labelMode);

        // (En-tetes de colonnes PITCH/WAVE/... supprimes : chaque carte d'element porte deja son
        // titre — « PITCH C3 », « WAVE »... — un seul niveau de titre suffit.)

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
        // Libelle NON attache : il etait tronque (« Master V... ») par la largeur imposee par
        // attachToComponent ; il est pose dans resized() a la largeur de son texte.
        addAndMakeVisible (labelMasterVolume);
        labelMasterVolume.setJustificationType (Justification::centredLeft);
        
        Value valueCommonVolume = valueTreeVoice.getPropertyAsValue(IDs::COMMONVOLUME.toString(), &undoManager);
        sliderMaster.getValueObject().referTo(valueCommonVolume);

        valueTreeVoice.addListener(this);
        addKeyListener(this); //to manage undo redo key
        addAndMakeVisible (historyBar);

        // En-tete commun des editeurs (retour + fil d'Ariane) : visible seulement editeur ouvert.
        addChildComponent (editorHeader);
        editorHeader.setAlwaysOnTop (true);
        editorHeader.onBack = [this] { closeOpenEditor(); };
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
        element1.elementValue.removeListener(this);
        element2.elementValue.removeListener(this);
        element3.elementValue.removeListener(this);
        element4.elementValue.removeListener(this);
        voiceNameVal.removeListener(this);
    }
    
    /** A/B compare showing A: every control is disabled except the history bar. */
    void setEditLocked (bool locked)
    {
        for (auto* c : getChildren())
            if (! historyBar.isLockExempt (c) && c != &editorHeader)   // quitter l'editeur n'est pas une edition
                c->setEnabled (! locked);
        // Unlock: re-derive the element rows from the CURRENT mode (inactive rows stay grey,
        // even if the mode changed while locked).
        if (! locked) setNombreElements (nombreElements);
    }

    /** Tous les editeurs plein-page (overlay au-dessus des 4 elements). */
    Array<Component*> allEditors()
    {
        return { &editAfm1, &editAfm2, &editAfm3, &editAfm4,
                 &editWave1, &editWave2, &editWave3, &editWave4,
                 &editFilter1, &editFilter2, &editFilter3, &editFilter4,
                 &editPan1, &editPan2, &editPan3, &editPan4 };
    }

    bool isEditorOpen()
    {
        for (auto* c : allEditors())
            if (c->isVisible()) return true;
        return false;
    }

    /** POINT D'ENTREE UNIQUE de fermeture (bouton « < Voice », onglet VOICE re-clique, Echap).
        Reprend ce que faisait l'ancien onglet « Close » : retour au 1er sous-onglet puis
        masquage. Pure presentation : rien n'est envoye au synthe. true si un etait ouvert. */
    bool closeOpenEditor()
    {
        bool any = false;
        auto closeOne = [&any] (auto& ed)
        {
            if (! ed.isVisible()) return;
            ed.resetTab();
            ed.setVisible (false);
            any = true;
        };
        for (auto* e : { &editAfm1, &editAfm2, &editAfm3, &editAfm4 })         closeOne (*e);
        for (auto* e : { &editWave1, &editWave2, &editWave3, &editWave4 })     closeOne (*e);
        for (auto* e : { &editFilter1, &editFilter2, &editFilter3, &editFilter4 }) closeOne (*e);
        for (auto* e : { &editPan1, &editPan2, &editPan3, &editPan4 })         closeOne (*e);
        editorHeader.setVisible (false);
        openEditorElement = 0;
        if (any) repaint();
        return any;
    }

    /** Ouvre un editeur (un seul a la fois) et renseigne le fil d'Ariane. */
    void openEditor (Component& ed, int elementNumber, const String& kind)
    {
        for (auto* c : allEditors())
            if (c != &ed && c->isVisible()) c->setVisible (false);
        editorHeader.refreshTooltip();
        editorHeader.setPath ({ TRANS ("Voice").trim().toUpperCase(),
                                TRANS ("Element") + " " + String (elementNumber), kind });
        editorHeader.setVisible (true);
        ed.setVisible (true);
        openEditorElement = elementNumber;
        repaint();
    }
    int openEditorElement = 0;   // element (1..4) de l'editeur ouvert ; 0 = aucun

    #include "ValueTrees.h"
    
    // Appele par le TabbedComponent quand on revient sur l'onglet VOICE depuis un autre onglet :
    // on GARDE l'editeur ouvert (on reprend ou on etait). Avant, tous les overlays etaient fermes
    // ici ; la fermeture passe desormais uniquement par closeOpenEditor().
    void broughtToFront() override {}
    
    // Sur changement de thème (sendLookAndFeelChange), ré-applique l'accent aux libellés qui
    // figent leur couleur via setColour (sinon ils gardent l'accent du thème de départ).
    void lookAndFeelChanged() override
    {
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
        if (isEditorOpen())
            return;

        // Carte ROUTAGE : contour ACCENT + titre, dessinés à la main (le LnF ignore l'accent
        // sur GroupComponent, cf. constructeur).
        Rectangle<float> algoArea ((float) algoX, (float) algoY, (float) algoW, (float) algoH);
        if (syPanelStudio())
        {
            // Style sans cadre : simple filet sous le titre, comme les GroupComponent. Pose SOUS
            // les radios Effect Mode (algoY+3..+23) et au-dessus de Stereo Mix (algoY+26) : dessine
            // en paintOverChildren, il les barrait a algoY+22.
            g.setColour (SYPal.panelBorder.withAlpha (0.6f));
            g.drawHorizontalLine ((int) algoY + 24, algoArea.getX() + 2.0f, algoArea.getRight() - 2.0f);
        }
        else
        {
            g.setColour (syCardBorder());
            g.drawRoundedRectangle (algoArea.reduced (1.0f), 8.0f, 1.5f);
        }
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
        const float bw = jmin (jmax (algoW * 0.17f, 58.0f), (outX - inX) / 3.0f * 0.9f);
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
            // Vide = voix inconnue (bloc court charge) : on vide le champ, le texte neutre
            // « (unknown voice) » s'affiche. dontSendNotification : aucun VNAM n'est emis
            // (seul Return dans le champ envoie le nom au synthe).
            const auto n = voiceNameVal.getValue().toString();
            editName.setText (n, dontSendNotification);
        }

        // Clic sur une carte d'element -> ouvre l'editeur correspondant (presentation seule :
        // setVisible / choix du sous-onglet, aucun envoi sysex). Libelles = ceux des cartes.
        Element*   els[4]  = { &element1, &element2, &element3, &element4 };
        WaveVue*   wave[4] = { &editWave1, &editWave2, &editWave3, &editWave4 };
        AFMVue*    afm[4]  = { &editAfm1, &editAfm2, &editAfm3, &editAfm4 };
        FilterVue* filt[4] = { &editFilter1, &editFilter2, &editFilter3, &editFilter4 };
        PanVue*    pan[4]  = { &editPan1, &editPan2, &editPan3, &editPan4 };
        for (int i = 0; i < 4; ++i)
        {
            if (! value.refersToSameSourceAs (els[i]->elementValue))
                continue;
            const int cmd = (int) els[i]->elementValue.getValue();
            const int n = i + 1;
            if (cmd == Element::commande::WaveEdit)
                openEditor (*wave[i], n, TRANS ("Wave"));
            else if (cmd == Element::commande::AfmEdit)
                openEditor (*afm[i], n, "AFM");
            else if (cmd == Element::commande::FilterEdit)
            {
                filt[i]->setAwmMode (els[i]->getOpMode() == Element::mode::AWM);
                openEditor (*filt[i], n, TRANS ("Filter"));
            }
            else if (cmd == Element::commande::VolumeEdit)
            {
                wave[i]->setTabVolume();
                openEditor (*wave[i], n, TRANS ("Wave"));
            }
            else if (cmd == Element::commande::VolumeAFM)
            {
                afm[i]->setTabVolume();
                openEditor (*afm[i], n, "AFM");
            }
            else if (cmd == Element::commande::PanEdit)
            {
                openEditor (*pan[i], n, TRANS ("Pan"));
                pan[i]->refresh();   // relit la table PANNM courante (comme avant, apres affichage)
            }
            if (cmd != 0)
                els[i]->elementValue = 0;
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
        // Un editeur reste ouvert entre les onglets : s'il vise un element devenu inactif
        // (voix chargee avec moins d'elements), on le ferme (pure presentation).
        if (openEditorElement > nombre)
            closeOpenEditor();
        // Façon SynthWorks : on affiche toujours les 4 rangées ; les éléments hors du
        // mode courant sont grisés et non interactifs (structure visible en permanence).
        Element*    els[4] = { &element1, &element2, &element3, &element4 };
        TextButton* ons[4] = { &op1, &op2, &op3, &op4 };
        for (int i = 0; i < 4; ++i)
        {
            const bool active = (i < nombre);
            els[i]->setVisible (true);
            els[i]->setEnabled (active);
            els[i]->setAlpha (active ? 1.0f : 0.6f);   // replie mais lisible (0.35 = illisible)

            // Bouton on/off : actif = allumé/cliquable, hors mode courant = grisé/désactivé.
            // Pas d'alpha en plus : le LookAndFeel grise deja un bouton desactive (texte attenue,
            // fond attenue) ; un alpha 0.35 par-dessus rendait le selecteur 1 2 3 4 illisible.
            ons[i]->setEnabled (active);
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
        // Editeurs plein-page sous l'en-tete commun (retour + fil d'Ariane).
        const int headerH = 30;
        editorHeader.setBounds (0, 0, getWidth(), headerH);
        auto boundsZone = getLocalBounds().withTrimmedTop (headerH);
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
        // Colonne droite (mode, master, ROUTAGE) : ~22 % de la largeur, bornee 300..440 px.
        // Avant : 30 % fixe -> carte de routage surtout vide ; la place revient aux elements.
        const int rightW = jlimit (300, 440, (int) (getWidth() * 0.22f));
        const auto wGrid = (double) jmax (300, getWidth() - 20 - 12 - rightW);
        const int topElem = 36;                 // sous la rangee [1 2 3 4] + historique

        // Rangées : les éléments ACTIFS reçoivent la majeure partie de la hauteur,
        // les inactifs sont réduits à une bande compacte (titres lisibles + étiquette OPn).
        Element* els[4] = { &element1, &element2, &element3, &element4 };
        const int nAct      = jlimit (1, 4, nombreElements);
        const int inactiveH = 40;
        const int avail     = jmax (80, getHeight() - topElem - 2);
        const int actH      = jmax (60, (avail - (4 - nAct) * inactiveH) / nAct);
        int yEl = topElem;
        for (int i = 0; i < 4; ++i)
        {
            const int h = (i < nAct) ? actH : inactiveH;
            els[i]->setBounds (10, yEl, (int) wGrid, h);
            yEl += h;
        }

        // Rangee du haut : [1][2][3][4] (on/off elements) puis historique. Le nom de voix a
        // ete deplace dans le bandeau superieur de la fenetre (MidiDemo) quand il y est.
        op1.setBounds (10,4,24,24);
        op2.setBounds (10+24, 4,24,24);
        op3.setBounds (10+48, 4,24,24);
        op4.setBounds (10+72, 4,24,24);
        int histX = 10 + 96 + 10;
        if (editName.getParentComponent() == this)
        {
            editName.setBounds (histX, 4, 100, 24);
            histX += 110;
        }
        historyBar.setBounds (histX, 4, jmax (120, jmin (500, (int) wGrid - histX)), 24);

        // --- Colonne droite (façon SynthWorks) : VOICE (mode + master) / ROUTAGE ---
        const int rightX = 10 + (int) wGrid + 12;
        comboMode.setBounds (rightX, 4, rightW, 26);
        {
            const auto f = labelMasterVolume.getFont();
            const int lw = roundToInt (GlyphArrangement::getStringWidth (f, labelMasterVolume.getText())) + 10;
            labelMasterVolume.setBounds (rightX, 36, lw, 20);
            sliderMaster.setBounds (rightX + lw + 4, 38, jmax (40, rightW - lw - 4), 16);
        }

        // Routage : sous le master jusqu'en bas. Dessiné dans paintOverChildren (lignes
        // depuis les vraies sorties pan des éléments).
        algoX = rightX;
        algoY = 62;
        algoW = rightW;
        algoH = jmax (60, getHeight() - algoY - 4);
        grpAlgo.setBounds (algoX, algoY, algoW, algoH);
        // Sélecteur d'Effect Mode (radio) + toggle Stereo Mix en haut-droite de la carte routage.
        radioMode.setBounds (algoX + algoW - 156, algoY + 3, 148, 20);
        mixBtn1.setBounds (algoX + algoW - 156, algoY + 26, 148, 18);
    }
    void addBtAndMakeStyle (TextButton& textButton)
    {
        textButton.setClickingTogglesState(true);
        addAndMakeVisible (textButton);
    }
    // Bouton on/off d'ÉLÉMENT : toggle ON = élément actif (accent thème), OFF = muté.
    void addElementOnOff (TextButton& bt)
    {
        bt.setClickingTogglesState (true);
        bt.setToggleState (true, dontSendNotification);  // élément actif par défaut
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
        slider.setRange(0, 127);
        slider.setNumDecimalPlacesToDisplay(0);
        slider.addListener(this);
    }
    void addButtonState (TextButton& bt)
    {
        addAndMakeVisible(bt);
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
    
    Label labelMode{"", "Operateurs Mode"};

    
    Label   labelMasterVolume{"M",TRANS("Master Volume")};


    // Colonne droite (façon SynthWorks).
    GroupComponent grpAlgo     { "grpAlgo",    TRANS ("ROUTAGE") };
    int algoX = 0, algoY = 0, algoW = 0, algoH = 0; // zone de la matrice (utilisée par paint)
    Array<Rectangle<float>> fxBoxes; // unités d'effet dessinées (cliquables -> onglet Effects)
    MidiRadio radioMode;             // sélecteur d'Effect Mode 0-3 (group 0x08/0x00) dans le routage
    MidiButton mixBtn1;              // Stereo Mix on/off (group 0x08 / 0x1B) — sur le groupe 1

    int nombreElements = 1;
    SysexBusSender sender;  // [2]

    
    UndoManager undoManager;
    EditHist::HistoryBar historyBar;   // undo / redo / A-B / original
    EditorHeader editorHeader;         // « < Voice  VOICE / Element n / AFM » des editeurs
};

//==============================================================================
