/*
 ==============================================================================
 
 This file is part of the JUCE examples.
 Copyright (c) 2017 - ROLI Ltd.
 
 The code included in this file is provided under the terms of the ISC license
 http://www.isc.org/downloads/software-support-policy/isc-license. Permission
 To use, copy, modify, and/or distribute this software for any purpose with or
 without fee is hereby granted provided that the above copyright notice and
 this permission notice appear in all copies.
 
 THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
 WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
 PURPOSE, ARE DISCLAIMED.
 
 ==============================================================================
 */

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.
 
 BEGIN_JUCE_PIP_METADATA
 
 name:             MidiDemo
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Handles incoming and outcoming midi messages.
 
 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
 juce_audio_processors, juce_audio_utils, juce_core,
 juce_data_structures, juce_events, juce_graphics,
 juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2017, linux_make, xcode_iphone, androidstudio
 
 moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1
 
 type:             Component
 mainClass:        MidiDemo
 
 useLocalCopy:     1
 
 END_JUCE_PIP_METADATA
 
 
 *******************************************************************************/
#include "Values.h"
#include "SysexBus.h"
#include "SysexUtils.h"   // SyVoice:: (builder sysex, device, helpers) — visible par tous les widgets
#include "Version.h"      // Sysex77::kVersion / versionString()
#include "AppSettings.h"  // getAppSettings() — persistance partagée (devices MIDI, fenêtre…)

//==============================================================================
// État partagé de l'application.
//
// Architecture : build « unity » (tout transite par Main.cpp -> MidiDemo.h), donc
// cet état vit en variables de portée fichier. Les `static const String` ci-dessous
// sont des CONSTANTES (adresses du bus interne, cf. SysexBus.h) ; le bloc « état de
// session » plus bas regroupe l'état MUTABLE d'exécution (sélection courante, device,
// thème via LookAndFeel.h…). NB : Declarations.h (anciens globals d'UI) a été supprimé,
// c'était du code mort — les vrais objets d'UI sont des membres de MidiDemo.
//==============================================================================

// --- Adresses du bus interne (constantes) ---
static const String adresseOscFoot = "/77Foot";
static const String adresseOscMod = "/77Mod";
static const String adresseOpMode = "/77OpMode";
static const String adresseOscSendBank ="/77SendBank";
static const String adresseOscRepaint = "/77Repaint";
static const String adresseOscParseVoices = "/77ParseVoices";
static const String adresseOscSendVoice = "/77SendVoice";
static const String oscTotalVoiceVolume = "/77TotalVoiceVolume";


static const String oscVoicePan1 = "/77VoicePan1";
static const String oscVoicePan2 = "/77VoicePan2";
static const String oscVoicePan3 = "/77VoicePan3";
static const String oscVoicePan4 = "/77VoicePan4";
static const String oscVoiceGrp1 = "/77VoiceGroupe1";
static const String oscVoiceGrp2 = "/77VoiceGroupe2";
static const String oscVoiceGrp3 = "/77VoiceGroupe3";
static const String oscVoiceGrp4 = "/77VoiceGroupe4";
static const String oscVoicePitch1 = "/77VoicePitch1";
static const String oscVoicePitch2 = "/77VoicePitch2";
static const String oscVoicePitch3 = "/77VoicePitch3";
static const String oscVoicePitch4 = "/77VoicePitch4";
static const String oscVoiceFine1 = "/77VoiceFine1";
static const String oscVoiceFine2 = "/77VoiceFine2";
static const String oscVoiceFine3 = "/77VoiceFine3";
static const String oscVoiceFine4 = "/77VoiceFine4";
static const String oscVoiceFixe1 = "/77VoiceFixe1";
static const String oscVoiceFixe2 = "/77VoiceFixe2";
static const String oscVoiceFixe3 = "/77VoiceFixe3";
static const String oscVoiceFixe4 = "/77VoiceFixe4";
static Array<MidiMessage>    oscMidiMessage; // tampon des 10 messages du nom de voix (cf. Voice.h)
static const String oscSendMidiMessage = "/77MidiMessage";

static   StringArray  arrayBank;    //la liste des banques
static  StringArray arrayListVoices; // liste des voices
static MemoryBlock currentBankData;  // octets bruts de la banque sélectionnée (pour envoi d'une voix)
static const int maxFiles = 512;

static        Array<File> BankFiles; //les fichiers des banques
Array<MidiMessage> messages;
Array<MidiMessage> arraySysex;
int SYModel =1;
int CommonFoot;
int CommonMod;

// --- État de session (mutable, runtime : sélection courante, flags, tampons) ---
static bool newMessage;
static bool requestSysex;
static bool doubleClickBank = false;
static    bool bankDeleteKey = false;
static int rowSelectedBank;
static bool loadBankRequest = false;
static     int timeOut;
static String bankSelected;

static Value valueSysexIn; //values from sysex midi in
static bool boolStopReceive; //to shunt the midi in when sending

static int sysexModel;
// Device number sysex SY77 (canal) : un seul réglage global, piloté par ConfigPage.
static int  sysexDeviceNumber = 1;     // 1..16 ; octet émis = 0x10 | (n-1)
static bool sysexReceiveOmni  = false; // "ALL" : accepte les messages entrants de tout device
static bool followSynth       = false; // "Suivre le synthé" : ouvre la vue du paramètre reçu
static juce::String lastMonitorKey;    // moniteur compact : dédup des param-changes par adresse
static     float fAngle = -90 * (juce::MathConstants<float>::pi  / 180.0); //Radiant to draw at 90°
File appDirPath = File::getSpecialLocation(File::userApplicationDataDirectory ).getChildFile("Application Support/Sysex77");

static Path pathFilter1;
static Path pathFilter2;
static Path pathFilter3;
static Path pathFilter4;
int intTabIndex;
// ValueTree for the Voice
//==============================================================================





//==============================================================================


#pragma once
#include "LookAndFeel.h"
#include "Controller.h"
#include "Config.h"
#include "Voice.h"
#include "Librairie.h"
#include "Effects.h"
#include "DumpDiffView.h"   // outil RE : comparer 2 dumps .syx
#include "MidiObjects.h"

//==============================================================================
struct MidiDeviceListEntry : ReferenceCountedObject
{
    MidiDeviceListEntry (MidiDeviceInfo info) : deviceInfo (info) {}

    MidiDeviceInfo deviceInfo;
    std::unique_ptr<MidiInput> inDevice;
    std::unique_ptr<MidiOutput> outDevice;
    
    using Ptr = ReferenceCountedObjectPtr<MidiDeviceListEntry>;
};

//==============================================================================
struct DemoTabbedComponent  : public TabbedComponent
{
    DemoTabbedComponent (bool isRunningComponenTransformsDemo)
    : TabbedComponent (TabbedButtonBar::TabsAtBottom)   // navigation en bas, façon SynthWorks
    {
        setTabBarDepth (32);
        auto colour = findColour (ResizableWindow::backgroundColourId);
        addTab (TRANS("Setting"),     colour, new ConfigPage (), true);  
        addTab (TRANS("Librairie"),     colour, new LibrairiePage (), true);
       // addTab (TRANS("Midi"),     colour, new ControllerPage (), true);
        addTab (TRANS("Voice"), colour, new VoicePage(), true);
        addTab (TRANS("Effects"), colour, new EffectsPage(), true);
        // L'onglet "Midi Setting" (index 3) est ajouté par MidiDemo avec un vrai
        // contenu (MidiSettingsPage), car il réutilise des composants de MidiDemo.

        // ConfigPage (1er onglet) a appliqué le thème pendant sa construction :
        // on aligne donc le fond des onglets sur la couleur de fond du thème.
        for (int i = 0; i < getNumTabs(); ++i)
            setTabBackgroundColour (i, SYColBackground);
   
        
        
        
        //        getTabbedButtonBar().getTabButton (5)->setExtraComponent (new CustomTabButton (isRunningComponenTransformsDemo),
        //                                                                  TabBarButton::afterText);
    }
    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName)override
    {
        if(newCurrentTabIndex<3)
        intTabIndex = newCurrentTabIndex;

        // Mémorise l'onglet courant (seulement après restauration, pour ne pas écraser
        // la préférence avec les index intermédiaires émis pendant la construction).
        if (allowTabSave)
        {
            getAppSettings()->setValue ("CurrentTab", newCurrentTabIndex);
            getAppSettings()->saveIfNeeded();
        }
    }
    bool allowTabSave = false; // activé par MidiDemo une fois l'onglet restauré
    void popupMenuClickOnTab (int tabIndex, const String& tabName)override
    {
        Logger::writeToLog("TabbedComponent: popupMenuClick");
    }
    // This is a small star button that is put inside one of the tabs. You can
    // use this technique to create things like "close tab" buttons, etc.
    class CustomTabButton  : public Component
    {
    public:
        CustomTabButton (bool isRunningComponenTransformsDemo)
        : runningComponenTransformsDemo (isRunningComponenTransformsDemo)
        {
            setSize (20, 20);
        }
        
        void paint (Graphics& g) override
        {
            Path star;
            star.addStar ({}, 7, 1.0f, 2.0f);
            
            g.setColour (Colours::green);
            g.fillPath (star, star.getTransformToScaleToFit (getLocalBounds().reduced (2).toFloat(), true));
        }
        void mouseEnter (const MouseEvent& mouseEvent) override
        {
            
        }
        void mouseDown (const MouseEvent&) override
        {
            
        }
    private:
        bool runningComponenTransformsDemo;
        //  std::unique_ptr<BubbleMessageComponent> bubbleMessage;
    };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoTabbedComponent)
};

//==============================================================================
/** Vue "Midi Setting" : héberge le choix des interfaces (in/out), le bouton
    Bluetooth, le Bulk Protect et le moniteur MIDI. Ces composants restent la
    propriété de MidiDemo (câblés à sa logique MIDI) ; cette page ne fait que les
    re-parenter et les disposer, pour en faire un onglet à part entière (la barre
    de navigation ne disparaît donc plus). */
struct MidiSettingsPage : public Component
{
    MidiSettingsPage (Label& inLabel, Component& inSel,
                      Label& outLabel, Component& outSel,
                      TextButton& pair, TextButton& bulk,
                      Label& monLabel, TextEditor& monitor)
        : inLab (inLabel), inSelector (inSel), outLab (outLabel), outSelector (outSel),
          pairBtn (pair), bulkBtn (bulk), monLab (monLabel), monitorRef (monitor)
    {
        setOpaque (false);
        addAndMakeVisible (inLab);
        addAndMakeVisible (inSelector);
        addAndMakeVisible (outLab);
        addAndMakeVisible (outSelector);
        addAndMakeVisible (pairBtn);
        addAndMakeVisible (bulkBtn);
        addAndMakeVisible (monLab);
        addAndMakeVisible (monitorRef);

        // "Suivre le synthé" : quand un paramètre arrive du SY77, ouvrir la vue concernée
        // (la valeur s'y met à jour en direct = retour visuel). Persisté.
        addAndMakeVisible (followBtn);
        followSynth = getAppSettings()->getBoolValue ("FollowSynth", false);
        followBtn.setToggleState (followSynth, dontSendNotification);
        followBtn.setColour (ToggleButton::tickColourId, SYColSelected);
        followBtn.onClick = [this]
        {
            followSynth = followBtn.getToggleState();
            getAppSettings()->setValue ("FollowSynth", followSynth);
            getAppSettings()->saveIfNeeded();
        };

        // Outil RE : comparer deux dumps .syx (avant/après un changement d'UN paramètre)
        // pour localiser l'octet -> bâtir la carte d'offsets bulk.
        addAndMakeVisible (diffBtn);
        diffBtn.onClick = [this]
        {
            auto* v = new DumpDiffView();
            v->setSize (740, 540);
            juce::DialogWindow::LaunchOptions o;
            o.content.setOwned (v);
            o.dialogTitle = "Diff 2 dumps (.syx)";
            o.dialogBackgroundColour = SYColBackground;
            o.resizable = true;
            o.useNativeTitleBar = true;
            o.launchAsync();
        };

        // Moniteur : effacer + copier (pratique pour la rétro-ingénierie).
        addAndMakeVisible (clearBtn);
        clearBtn.onClick = [this] { monitorRef.clear(); lastMonitorKey = {}; };
        addAndMakeVisible (copyBtn);
        copyBtn.onClick = [this] { juce::SystemClipboard::copyTextToClipboard (monitorRef.getText()); };
    }

    void resized() override
    {
        const int m = 10;
        const int half = getWidth() / 2;
        const int selH = getHeight() / 2 - 80;

        inLab .setBounds (m,        m,       half - 2*m, 24);
        outLab.setBounds (half + m, m,       half - 2*m, 24);
        inSelector .setBounds (m,        m + 28, half - 2*m, selH);
        outSelector.setBounds (half + m, m + 28, half - 2*m, selH);

        const int rowY = m + 28 + selH + m;
        pairBtn.setBounds (m,        rowY, half - 2*m, 24);
        bulkBtn.setBounds (half + m, rowY, half - 2*m, 24);

        followBtn.setBounds (m, rowY + 30, getWidth() - 2*m, 22);
        diffBtn  .setBounds (m, rowY + 56, half - 2*m, 24);

        const int my = rowY + 86;
        clearBtn.setBounds (getWidth() - m - 80,            my, 80, 22);
        copyBtn .setBounds (getWidth() - m - 80 - 6 - 80,   my, 80, 22);
        monLab  .setBounds (m, my, getWidth() - 2*m - 172, 24);
        monitorRef.setBounds (m, my + 26, getWidth() - 2*m, getHeight() - (my + 26) - m);
    }

    Label&      inLab;
    Component&  inSelector;
    Label&      outLab;
    Component&  outSelector;
    TextButton& pairBtn;
    TextButton& bulkBtn;
    Label&      monLab;
    TextEditor& monitorRef;
    ToggleButton followBtn { "Suivre le synthe : ouvrir la vue du parametre recu depuis le SY77" };
    TextButton   diffBtn   { "Diff 2 dumps (.syx)..." };
    TextButton   copyBtn   { "Copier" };
    TextButton   clearBtn  { "Clear" };
};

//==============================================================================
class MidiDemo  : public Component,
private Timer,
private MidiKeyboardStateListener,
private MidiInputCallback,
private AsyncUpdater,
private TextButton::Listener,
 public ComboBox::Listener,
 public Value::Listener
{
public:
    //==============================================================================
    MidiDemo (bool isRunningComponenTransformsDemo = false)
    :  midiKeyboard       (keyboardState, MidiKeyboardComponent::horizontalKeyboard),
    midiInputSelector  (new MidiDeviceListBox ("Midi Input Selector",  *this, true)),
    midiOutputSelector (new MidiDeviceListBox ("Midi Output Selector", *this, false)),
    tabs (isRunningComponenTransformsDemo)
    
    {
        // (Images de fond Sysex77/99.png supprimées : jamais dessinées — nettoyage des ressources.)
        addAndMakeVisible (tabs);
  
        addLabelAndSetStyle (midiInputLabel);
        addLabelAndSetStyle (midiOutputLabel);
        addLabelAndSetStyle (incomingMidiLabel);
        
        addAndMakeVisible(btBulk);
        btBulk.setClickingTogglesState(true);
        btBulk.setColour(TextButton::ColourIds::buttonOnColourId, SYColSelected);
        btBulk.addListener(this);

        // Toggle d'affichage du clavier (gardé en bas) — masque/affiche le clavier.
        addAndMakeVisible (btToggleKeyboard);
        btToggleKeyboard.setAlwaysOnTop (true);   // reste visible au-dessus des onglets (barre de nav)
        btToggleKeyboard.setClickingTogglesState (true);
        btToggleKeyboard.setToggleState (getAppSettings()->getBoolValue ("KeyboardVisible", true), dontSendNotification);
        btToggleKeyboard.setColour (TextButton::ColourIds::buttonOnColourId, SYColSelected);
        btToggleKeyboard.onClick = [this]
        {
            midiKeyboard.setVisible (btToggleKeyboard.getToggleState());
            getAppSettings()->setValue ("KeyboardVisible", btToggleKeyboard.getToggleState());
            getAppSettings()->saveIfNeeded();
            resized();
        };

        midiKeyboard.setName ("MIDI Keyboard");
        addAndMakeVisible (midiKeyboard);
        midiKeyboard.setVisible (btToggleKeyboard.getToggleState()); // applique l'état mémorisé
        
        midiMonitor.setMultiLine (true);
        midiMonitor.setReturnKeyStartsNewLine (false);
        midiMonitor.setReadOnly (true);
        midiMonitor.setScrollbarsShown (true);
        midiMonitor.setCaretVisible (false);
        midiMonitor.setPopupMenuEnabled (false);
        midiMonitor.setText ({});
        addAndMakeVisible (midiMonitor);
        
        if (! BluetoothMidiDevicePairingDialogue::isAvailable())
            pairButton.setEnabled (false);
        
        addAndMakeVisible (pairButton);
        pairButton.onClick = []
        {
            RuntimePermissions::request (RuntimePermissions::bluetoothMidi,
                                         [] (bool wasGranted)
                                         {
                                             if (wasGranted)
                                                 BluetoothMidiDevicePairingDialogue::open();
                                         });
        };
        keyboardState.addListener (this);
        
        addAndMakeVisible (midiInputSelector .get());
        addAndMakeVisible (midiOutputSelector.get());
        
        
        loadData();
        Logger::writeToLog("XML Num" + String(numRows));
        addAndMakeVisible(labelFoot);
        addAndMakeVisible(labelMod);
        
        addAndMakeVisible (comboFoot);
        
        comboFoot.setEditableText (false);
        comboFoot.setJustificationType (Justification::left);
        comboFoot.addListener(this);
        

        addAndMakeVisible (comboMod);
        
        comboMod.setEditableText (false);
        comboMod.setJustificationType (Justification::left);
        comboMod.addListener(this);

        
        for (int i = 1; i < numRows; ++i)
            //  comboBox.addItem ("combo box item " + String (i), i);
            //   forEachXmlChildElement (*dataList, "name")
        {
            if (auto* rowElement = dataList->getChildElement(i))
            {
                auto text = rowElement->getStringAttribute (getAttributeNameForColumnId (1));
                text = text + " " + rowElement->getStringAttribute (getAttributeNameForColumnId (2));
                comboFoot.addItem (text,i);
                comboMod.addItem (text,i);
            }
            
            
        }
                comboFoot.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::COMMONFOOT, nullptr));
                comboMod.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::COMMONMOD, nullptr));
        
// Si pas de selection on evite un champ vide en selectionnant le premier
        if(comboFoot.getSelectedItemIndex()<1)
            comboFoot.setSelectedId(1);
        if(comboMod.getSelectedItemIndex()<1)
            comboMod.setSelectedId(1);
// ----------------------------
        // Plus aucune connexion UDP : le bus est intra-processus (voir SysexBus.h).
        // addOscListener() installe le callback du bus au lieu de binder un port.
        addOscListener();

        // Onglet "Midi Setting" : vraie page (choix interfaces + Bluetooth + Bulk + moniteur).
        // Réutilise les composants de MidiDemo (re-parentés dans la page).
        midiSettingsPage.reset (new MidiSettingsPage (midiInputLabel, *midiInputSelector,
                                                      midiOutputLabel, *midiOutputSelector,
                                                      pairButton, btBulk,
                                                      incomingMidiLabel, midiMonitor));
        tabs.addTab (TRANS("Midi Setting"), SYColBackground, midiSettingsPage.get(), false);

        // Barre de navigation visible dès le démarrage (on n'atterrit plus sur une vue
        // sans menu). On ouvre "Midi Setting" en premier : choix de l'interface.
        tabs.setVisible(true);
        tabs.setAlwaysOnTop(true);

        setSize (1280, 820);
        // Restaure l'onglet mémorisé (défaut : 3 = Midi Setting), puis autorise la sauvegarde.
        tabs.setCurrentTabIndex (jlimit (0, tabs.getNumTabs() - 1,
                                         getAppSettings()->getIntValue ("CurrentTab", 4))); // 4 = Midi Setting
        tabs.allowTabSave = true;
        startTimer (500);
    }
    
    ~MidiDemo()
    {
        stopTimer();
  
        

        midiInputs .clear();
        midiOutputs.clear();
        keyboardState.removeListener (this);
        btBulk.removeListener(this);
      
      
        midiInputSelector .reset();
        midiOutputSelector.reset();
        comboFoot.removeListener(this);
        comboMod.removeListener(this);
        
        
    }
    

    void valueChanged (Value& value) override
    {
        Logger::writeToLog("midi demo value change");
    }
    
    void buttonClicked (Button* button) override
    {
        if(button == &btBulk)
        {
            Logger::writeToLog("Bulk Protect");
            uint8 sysexdata[9] = { 0x43, SyVoice::deviceByte (sysexDeviceNumber), 0x34, 0x0f, 0x00, 0x00, 0x34, 0x00, 0x00 };
            sysexdata[8] = btBulk.getToggleState();
            MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
            m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
    }
    //==============================================================================
    void timerCallback() override
    {
        // "Midi Setting" est désormais un onglet à part entière (MidiSettingsPage) :
        // plus besoin de masquer la barre de navigation à sa sélection.
        updateDeviceList (true);
        updateDeviceList (false);
    }
    
    //==============================================================================

#include "MidiSysex.h"
    
    /*
     ==============================================================================
     
     Pour réduire le code et retrouver facilement la routine l'include externalise
     le code de la routine qui recupere l'osc et envoi le sysex en midi
     
     ==============================================================================
     */
    
    void handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        MidiMessage m (MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }
    
    void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        MidiMessage m (MidiMessage::noteOff (midiChannel, midiNoteNumber, velocity));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }
    
    void paint (Graphics& g) override
    {
        g.fillAll (SYColBackground);

        // Image de fond du thème (PNG/JPG défini dans theme.xml <ASSETS background="…">).
        // Mode "cover" : remplit la zone en gardant les proportions, bords éventuellement rognés.
        if (SYBackgroundImage.isValid())
            g.drawImage (SYBackgroundImage, getLocalBounds().toFloat(),
                         RectanglePlacement (RectanglePlacement::fillDestination));

        // Logo vectoriel « SYSEX 77/99 » (texte, suit le thème)
        auto logoText = (SYModel == 3 ? String ("SYSEX 99") : String ("SYSEX 77"));
        Rectangle<int> logoBox (getWidth() - 250, 6, 230, 46);
        g.setColour (SYColSelected);
        g.setFont (Font (FontOptions (34.0f, Font::bold)));
        g.drawText (logoText, logoBox, Justification::centredRight, false);
    }
    void mouseDown (const MouseEvent&) override
    {
        if (tabs.isVisible()  ==true)
        {
            tabs.setVisible(false);
        }
        else
        {
            tabs.setVisible(true);
        }
        
    }
    void resized() override
    {
        auto margin = 10;
        
        
        
        // Les composants MIDI (interfaces, moniteur, pair, bulk) sont désormais
        // disposés par MidiSettingsPage (onglet 3) — plus ici.

        // Bandeau bas (clavier + pédale/molette) repliable : quand on le masque,
        // tout le bandeau disparaît ET la vue s'étend pour combler la place.
        const bool kb = btToggleKeyboard.getToggleState();
        midiKeyboard.setVisible (kb);
        labelFoot.setVisible (kb); comboFoot.setVisible (kb);
        labelMod .setVisible (kb); comboMod .setVisible (kb);

        if (kb)
        {
            tabs.setBounds (0, 10, getWidth(), getHeight() - 84);
            midiKeyboard.setBounds (margin + 140, getHeight() - 70, getWidth() - (2 * margin) - 140, 70);
            labelFoot.setBounds (margin, getHeight() - 77, 134, 20);
            comboFoot.setBounds (margin, getHeight() - 58, 134, 20);
            labelMod .setBounds (margin, getHeight() - 40, 134, 20);
            comboMod .setBounds (margin, getHeight() - 24, 134, 20);
        }
        else
        {
            // Clavier masqué : la vue prend toute la hauteur.
            tabs.setBounds (0, 10, getWidth(), getHeight() - 14);
        }

        // Bouton de toggle calé à droite, sur la rangée d'onglets (barre de nav en bas).
        const int barTop = tabs.getBottom() - 32; // 32 = profondeur de la barre d'onglets
        btToggleKeyboard.setBounds (getWidth() - margin - 70, barTop + 3, 70, 26);
        btToggleKeyboard.toFront (false); // au-dessus de la barre d'onglets (sinon le clic part dans les onglets)
    }
    
    void openDevice (bool isInput, int index)
    {
        if (isInput)
        {
            jassert (midiInputs[index]->inDevice.get() == nullptr);
            midiInputs[index]->inDevice = MidiInput::openDevice (midiInputs[index]->deviceInfo.identifier, this);
            
            if (midiInputs[index]->inDevice.get() == nullptr)
            {
                DBG ("MidiDemo::openDevice: open input device for index = " << index << " failed!");
                return;
            }
            
            midiInputs[index]->inDevice->start();
        }
        else
        {
            jassert (midiOutputs[index]->outDevice.get() == nullptr);
            midiOutputs[index]->outDevice = MidiOutput::openDevice (midiOutputs[index]->deviceInfo.identifier);
            
            if (midiOutputs[index]->outDevice.get() == nullptr)
            {
                DBG ("MidiDemo::openDevice: open output device for index = " << index << " failed!");
            }
        }
    }
    
    void closeDevice (bool isInput, int index)
    {
        if (isInput)
        {
            jassert (midiInputs[index]->inDevice.get() != nullptr);
            midiInputs[index]->inDevice->stop();
            midiInputs[index]->inDevice.reset();
        }
        else
        {
            jassert (midiOutputs[index]->outDevice.get() != nullptr);
            midiOutputs[index]->outDevice.reset();
        }
    }
    
    int getNumMidiInputs() const noexcept
    {
        return midiInputs.size();
    }
    
    int getNumMidiOutputs() const noexcept
    {
        return midiOutputs.size();
    }
    
    ReferenceCountedObjectPtr<MidiDeviceListEntry> getMidiDevice (int index, bool isInput) const noexcept
    {
        return isInput ? midiInputs[index] : midiOutputs[index];
    }
    
private:
    //==============================================================================
    struct MidiDeviceListBox : public ListBox,
    private ListBoxModel
    {
        MidiDeviceListBox (const String& name,
                           MidiDemo& contentComponent,
                           bool isInputDeviceList)
        : ListBox (name, this),
        parent (contentComponent),
        isInput (isInputDeviceList)
        {
            setOutlineThickness (1);
            setMultipleSelectionEnabled (true);
            setClickingTogglesRowSelection (true);
            
        }
        
        //==============================================================================
        int getNumRows() override
        {
            return isInput ? parent.getNumMidiInputs()
            : parent.getNumMidiOutputs();
        }
        
        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            auto textColour = getLookAndFeel().findColour (ListBox::textColourId);
            
            if (rowIsSelected)
                g.fillAll (textColour.interpolatedWith (getLookAndFeel().findColour (ListBox::backgroundColourId), 0.5));
            
            
            g.setColour (textColour);
            g.setFont (height * 0.7f);
            
            if (isInput)
            {
                if (rowNumber < parent.getNumMidiInputs())
                    g.drawText (parent.getMidiDevice (rowNumber, true)->deviceInfo.name,
                                5, 0, width, height,
                                Justification::centredLeft, true);
            }
            else
            {
                if (rowNumber < parent.getNumMidiOutputs())
                    g.drawText (parent.getMidiDevice (rowNumber, false)->deviceInfo.name,
                                5, 0, width, height,
                                Justification::centredLeft, true);
            }
        }
        
        //==============================================================================
        void selectedRowsChanged (int) override
        {
            auto newSelectedItems = getSelectedRows();
            if (newSelectedItems != lastSelectedItems)
            {
                for (auto i = 0; i < lastSelectedItems.size(); ++i)
                {
                    if (! newSelectedItems.contains (lastSelectedItems[i]))
                        parent.closeDevice (isInput, lastSelectedItems[i]);
                }
                
                for (auto i = 0; i < newSelectedItems.size(); ++i)
                {
                    if (! lastSelectedItems.contains (newSelectedItems[i]))
                        parent.openDevice (isInput, newSelectedItems[i]);
                    
                }
                
                lastSelectedItems = newSelectedItems;
                parent.saveEnabledDevices (isInput); // mémorise la sélection d'interfaces
            }
        }

        //==============================================================================
        void syncSelectedItemsWithDeviceList (const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices)
        {
            SparseSet<int> selectedRows;
            for (auto i = 0; i < midiDevices.size(); ++i)
                if (midiDevices[i]->inDevice.get() != nullptr || midiDevices[i]->outDevice.get() != nullptr)
                    selectedRows.addRange (Range<int> (i, i + 1));
            
            lastSelectedItems = selectedRows;
            updateContent();
            setSelectedRows (selectedRows, dontSendNotification);
        }
        
    private:
        //==============================================================================
        MidiDemo& parent;
        bool isInput;
        SparseSet<int> lastSelectedItems;
    };
    
    //==============================================================================
    void handleIncomingMidiMessage (MidiInput* /*source*/, const MidiMessage& message) override
    {
        // This is called on the MIDI thread
        const ScopedLock sl (midiMonitorLock);
        if(!boolStopReceive)
        if(!message.isActiveSense())
        {
            incomingMessages.add (message);
            if(requestSysex == true)
            {
                if(message.isSysEx())
                {
                    Logger::writeToLog("Add sysex");
                    arraySysex.add(message);
                    timeOut = 0;
                }
                else
                {
                    Logger::writeToLog("other message");
                }
                
            }
            else
            {
               // Logger::writeToLog(String(message.getSysExDataSize()));
                
           
            }
            triggerAsyncUpdate();
        }
        
    }
    template<typename... Ts>
    inline var make_var_array(Ts... values)
    {
        return Array<var>(values...);
    }
    void handleAsyncUpdate() override
    {
        // This is called on the message loop
        
        {
            const ScopedLock sl (midiMonitorLock);
            messages.swapWith (incomingMessages);
            
        }
        
        String messageText;
        
        for (auto& message : messages)
        {
            if(message.getSysExDataSize()==9)
            {
                memcpy(&data, message.getSysExData(), message.getSysExDataSize());
                // Message paramétrique SY77 (0x43 .. 0x34) ET adressé au device sélectionné
                // (ou tout device en mode ALL). On ignore le reste (autres machines/canaux).
                if (data[0] == 0x43 && data[2] == 0x34
                    && SyVoice::acceptsDevice (data[1], sysexDeviceNumber, sysexReceiveOmni))
                {
                    valueSysexIn = make_var_array(data[3], data[4],data[5],data[6],data[7],data[8]);

                    // "Suivre le synthé" : ouvre la vue correspondant au groupe reçu, pour que
                    // le paramètre modifié sur le SY77 soit visible (sa valeur s'y met à jour).
                    if (followSynth)
                    {
                        const int g = data[3];
                        int target = -1;
                        if (g == 0x08)                                   target = 3; // Effets
                        else if (g == 0x02 || g == 0x03 || g == 0x05 || g == 0x07
                                 || g == 0x09 || g == 0x0A || g == 0x00
                                 || (g & 0x0F) == 0x06)                  target = 2; // Voice
                        if (target >= 0 && tabs.getCurrentTabIndex() != target)
                            tabs.setCurrentTabIndex (target);
                    }

                    // Moniteur COMPACT : une SEULE ligne par adresse (G/AH/AL/P) touchée.
                    // Un balayage de valeurs sur le même paramètre = une ligne (on imprime
                    // uniquement quand l'adresse change). Évite le déluge de lignes.
                    const String key = String (data[3]) + "." + String (data[4]) + "."
                                     + String (data[5]) + "." + String (data[6]);
                    if (key != lastMonitorKey)
                    {
                        lastMonitorKey = key;
                        auto hx = [] (int v) { return String::toHexString (v).paddedLeft ('0', 2); };
                        messageText << ">> SY77  G=" << hx (data[3])
                                    << "  AH=" << hx (data[4]) << "  AL=" << hx (data[5])
                                    << "  P="  << hx (data[6])
                                    << "   val=" << (int) data[8] << " (0x" << hx (data[8]) << ")\n";
                    }
                }
                continue; // bloc sysex 9 octets : pas de description brute (déjà décodé ci-dessus)
            }

            // Bruit filtré pour la RE : CC data-entry, horloge, active-sense, transport.
            if (message.isController() || message.isMidiClock() || message.isActiveSense()
                || message.isMidiStart() || message.isMidiContinue() || message.isMidiStop()
                || message.isSongPositionPointer())
                continue;

            messageText << message.getDescription() << "\n";
        }
        if (messageText.isNotEmpty())
            midiMonitor.insertTextAtCaret (messageText);
        
    }
public:
    void sendToOutputs (const MidiMessage& msg)
    {
        boolStopReceive = true; //shunt the midi in
        
        for (auto midiOutput : midiOutputs)
            if (midiOutput->outDevice.get() != nullptr)
            {
                
                midiOutput->outDevice->sendMessageNow (msg);
                Logger::writeToLog("Envoi msg midi");
                Logger::writeToLog(String(msg.getDescription()) );
            }
        boolStopReceive = false; //receive unShunt
    }
    
    //==============================================================================
    bool hasDeviceListChanged (const Array<MidiDeviceInfo>& availableDevices, bool isInputDevice)
    {
        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
        : midiOutputs;

        if (availableDevices.size() != midiDevices.size())
            return true;

        for (auto i = 0; i < availableDevices.size(); ++i)
            if (availableDevices[i].identifier != midiDevices[i]->deviceInfo.identifier)
                return true;

        return false;
    }

    ReferenceCountedObjectPtr<MidiDeviceListEntry> findDevice (MidiDeviceInfo deviceInfo, bool isInputDevice) const
    {
        const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
        : midiOutputs;

        for (auto& midiDevice : midiDevices)
            if (midiDevice->deviceInfo.identifier == deviceInfo.identifier)
                return midiDevice;

        return nullptr;
    }

    void closeUnpluggedDevices (const Array<MidiDeviceInfo>& currentlyPluggedInDevices, bool isInputDevice)
    {
        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
        : midiOutputs;

        for (auto i = midiDevices.size(); --i >= 0;)
        {
            auto& d = *midiDevices[i];

            if (! currentlyPluggedInDevices.contains (d.deviceInfo))
            {
                if (isInputDevice ? d.inDevice .get() != nullptr
                    : d.outDevice.get() != nullptr)
                    closeDevice (isInputDevice, i);
                
                midiDevices.remove (i);
            }
        }
    }
    
    // Persistance des interfaces MIDI choisies : on mémorise les identifiants des
    // devices OUVERTS, et on les réouvre au lancement s'ils sont encore disponibles.
    void saveEnabledDevices (bool isInput)
    {
        auto& devices = isInput ? midiInputs : midiOutputs;
        StringArray ids;
        for (auto& d : devices)
            if (isInput ? d->inDevice.get() != nullptr : d->outDevice.get() != nullptr)
                ids.add (d->deviceInfo.identifier);
        getAppSettings()->setValue (isInput ? "MidiInDevices" : "MidiOutDevices", ids.joinIntoString ("\n"));
        getAppSettings()->saveIfNeeded();
    }

    void restoreEnabledDevices (bool isInput)
    {
        auto saved = StringArray::fromLines (getAppSettings()->getValue (isInput ? "MidiInDevices" : "MidiOutDevices"));
        saved.removeEmptyStrings();
        if (saved.isEmpty()) return;
        auto& devices = isInput ? midiInputs : midiOutputs;
        for (int i = 0; i < devices.size(); ++i)
        {
            const bool isOpen = isInput ? devices[i]->inDevice.get() != nullptr
                                        : devices[i]->outDevice.get() != nullptr;
            if (! isOpen && saved.contains (devices[i]->deviceInfo.identifier))
                openDevice (isInput, i);
        }
    }

    void updateDeviceList (bool isInputDeviceList)
    {
        auto availableDevices = isInputDeviceList ? MidiInput::getAvailableDevices()
        : MidiOutput::getAvailableDevices();

        if (hasDeviceListChanged (availableDevices, isInputDeviceList))
        {

            ReferenceCountedArray<MidiDeviceListEntry>& midiDevices
            = isInputDeviceList ? midiInputs : midiOutputs;

            closeUnpluggedDevices (availableDevices, isInputDeviceList);

            ReferenceCountedArray<MidiDeviceListEntry> newDeviceList;

            // add all currently plugged-in devices to the device list
            for (auto& newDevice : availableDevices)
            {
                MidiDeviceListEntry::Ptr entry = findDevice (newDevice, isInputDeviceList);

                if (entry == nullptr)
                    entry = new MidiDeviceListEntry (newDevice);

                newDeviceList.add (entry);
            }
            
            // actually update the device list
            midiDevices = newDeviceList;

            // réouvre les interfaces mémorisées encore disponibles (persistance)
            restoreEnabledDevices (isInputDeviceList);

            // update the selection status of the combo-box
            if (auto* midiSelector = isInputDeviceList ? midiInputSelector.get() : midiOutputSelector.get())
                midiSelector->syncSelectedItemsWithDeviceList (midiDevices);
        }
    }
    
    //==============================================================================
    void addLabelAndSetStyle (Label& label)
    {
        label.setFont (Font (FontOptions (15.00f, Font::plain)));
        label.setJustificationType (Justification::centredLeft);
        label.setEditable (false, false, false);
        label.setColour (TextEditor::textColourId, Colours::black);
        label.setColour (TextEditor::backgroundColourId, Colour (0x00000000));
        
        addAndMakeVisible (label);
    }
    
    void comboBoxChanged	(	ComboBox * 	comboBoxThatHasChanged	) override
    {
        if(comboBoxThatHasChanged == &comboFoot)
        {
        uint8 sysexdata[9] = { 0x43, SyVoice::deviceByte (sysexDeviceNumber), 0x34, 0x0f, 0x00, 0x00, 0x2d, 0x00, 0x00 };
        sysexdata[8] = comboFoot.getSelectedItemIndex()+1;
        MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
        }
        if(comboBoxThatHasChanged == &comboMod)
        {
            uint8 sysexdata[9] = { 0x43, SyVoice::deviceByte (sysexDeviceNumber), 0x34, 0x0f, 0x00, 0x00, 0x2c, 0x00, 0x00 };
            sysexdata[8] = comboMod.getSelectedItemIndex()+1;
            MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
            m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
        //MidiMessage m (MidiMessage::noteOn (1, 64, 120));
        // create and send an OSC message with an address and a float value:
//        if (! sender.send (adresseOscFoot, (int) comboFoot.getSelectedItemIndex()+1)) // [5]
//            Logger::writeToLog ("OSC erreur");;
//        if (! sender.send (adresseOscMod, (int) comboMod.getSelectedItemIndex()+1)) // [5]
//            Logger::writeToLog ("OSC erreur");;
    }
    
    void loadData()
    {
        
        
        auto tableFile = BinaryData::TableData_xml;
        
        tutorialData = XmlDocument::parse (tableFile);
        
        dataList   = tutorialData->getChildByName ("DATA");
        columnList = tutorialData->getChildByName ("HEADERS");
        
        numRows = dataList->getNumChildElements();
        
    }
    
    String getAttributeNameForColumnId (const int columnId) const
    {
        for (auto* columnXml : columnList->getChildIterator())
        {
            if (columnXml->getIntAttribute ("columnId") == columnId)
                return columnXml->getStringAttribute ("name");
        }
        
        return {};
    }
    //==============================================================================
    Label midiInputLabel    { "Midi Input Label", TRANS( "MIDI Input:" )};
    Label midiOutputLabel   { "Midi Output Label", TRANS("MIDI Output:") };
    Label incomingMidiLabel { "Incoming Midi Label", TRANS("Received MIDI messages:") };
    
    MidiKeyboardState keyboardState;
    MidiKeyboardComponent midiKeyboard;
    TextEditor midiMonitor  { TRANS("MIDI Monitor" )};
    TextButton pairButton   { TRANS("MIDI Bluetooth devices...") };
    
    TextButton  btBulk {"Bulk Protect"};
    TextButton  btToggleKeyboard {"Clavier"};
    std::unique_ptr<MidiDeviceListBox> midiInputSelector, midiOutputSelector;
    std::unique_ptr<MidiSettingsPage>  midiSettingsPage;
    ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;
    
    CriticalSection midiMonitorLock;
    Array<MidiMessage> incomingMessages;
    DemoTabbedComponent tabs;

    uint8 data[12];
    
    // controler
    ComboBox comboFoot { "Pedale de sustain"};
    ComboBox comboMod  { "Molette de modulation" };
    Label   labelFoot { "","Pedale de sustain :"};
    Label   labelMod { "","Molette de modulation :"};
    std::unique_ptr<XmlElement> tutorialData;
    XmlElement* columnList = nullptr;
    XmlElement* dataList = nullptr;
    int numRows = 0;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiDemo)
};
