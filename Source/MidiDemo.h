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
static const String adresseOscRequestDump ="/77RequestDump";
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

static int sysexModel;
// Device number sysex SY77 (canal) : un seul réglage global, piloté par ConfigPage.
static int  sysexDeviceNumber = 1;     // 1..16 ; octet émis = 0x10 | (n-1)
static bool sysexReceiveOmni  = false; // "ALL" : accepte les messages entrants de tout device
static bool followSynth       = false; // "Suivre le synthé" : ouvre la vue du paramètre reçu
static bool monitorRaw        = false; // moniteur : false = décodé (G/AH/AL/P/val), true = octets bruts hex
static bool devMode           = false; // Mode développeur : affiche le moniteur MIDI + outils RE (onglet "Midi Setting")
// Interrupteurs rapides écoute/envoi (bandeau bas) — diagnostic hardware.
// midiListen OFF : l'app IGNORE tout MIDI entrant (gate handleIncomingMidiMessage).
// midiSend   OFF : l'app n'émet RIEN vers les sorties (gate sendToOutputs, coupe aussi le Thru).
static bool midiListen        = true;  // ON par défaut
static bool midiSend          = true;  // ON par défaut
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
#include "BankSendProgress.h"      // fenêtre de progression de l'envoi de banque
#include "BankReceiveProgress.h"   // fenêtre de progression de la réception de banque
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
/** Vue "Midi Setting" (MODE DÉVELOPPEUR uniquement) : moniteur MIDI + outils de
    rétro-ingénierie (fingerprint, lecture valeurs, diff de dumps), Bluetooth et le
    "MIDI Thru" (gardé ici car il écrit vers le synthé / risque d'écho). Le choix des
    interfaces In/Out, "Suivre le synthé", le canal et "Bulk Protect" ont été rapatriés
    dans l'onglet "Setting" (ConfigPage). Cet onglet n'est ajouté à la barre que si
    devMode == true (toggle dans Settings). Les composants restent la propriété de
    MidiDemo ; cette page ne fait que les re-parenter et les disposer. */
struct MidiSettingsPage : public Component
{
    MidiSettingsPage (TextButton& pair, Label& monLabel, TextEditor& monitor)
        : pairBtn (pair), monLab (monLabel), monitorRef (monitor)
    {
        setOpaque (false);
        addAndMakeVisible (pairBtn);
        addAndMakeVisible (monLab);
        addAndMakeVisible (monitorRef);

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

        // Outil RE (jetable) : « empreinte » — envoie à chaque contrôle câblé de l'éditeur
        // une valeur brute distinctive + journalise (addr -> valeur) dans re_fingerprint.csv.
        // Workflow : dump AVANT -> clic -> dump APRÈS ; le diff isole nos octets, le CSV les nomme.
        addAndMakeVisible (reBtn);
        reBtn.onClick = [this] { if (onFingerprint) onFingerprint(); };

        // RE : exporte la valeur ACTUELLE de tous les contrôles câblés (sans rien envoyer).
        // Corréler avec un dump de la même voix = carte d'offsets, sans toucher au synthé.
        addAndMakeVisible (reReadBtn);
        reReadBtn.onClick = [this] { if (onReadValues) onReadValues(); };

        // Moniteur : effacer + copier (pratique pour la rétro-ingénierie).
        addAndMakeVisible (clearBtn);
        clearBtn.onClick = [this] { monitorRef.clear(); lastMonitorKey = {}; if (onClearHistory) onClearHistory(); };
        addAndMakeVisible (copyBtn);
        copyBtn.onClick = [this] { juce::SystemClipboard::copyTextToClipboard (monitorRef.getText()); };

        // Toggle "Raw / Interprété" : bascule l'affichage du moniteur entre octets bruts (hex)
        // et forme décodée (G/AH/AL/P/val). Persisté. Re-rend l'historique -> s'applique aussi
        // aux lignes déjà listées (cf. MidiDemo::rebuildMonitorFromHistory).
        addAndMakeVisible (rawBtn);
        monitorRaw = getAppSettings()->getBoolValue ("MonitorRaw", false);
        rawBtn.setToggleState (monitorRaw, dontSendNotification);
        rawBtn.setColour (ToggleButton::tickColourId, SYColSelected);
        rawBtn.onClick = [this]
        {
            monitorRaw = rawBtn.getToggleState();
            getAppSettings()->setValue ("MonitorRaw", monitorRaw);
            getAppSettings()->saveIfNeeded();
            if (onRawToggled) onRawToggled();   // re-rend le moniteur dans le nouveau mode
        };
    }

    // Câblés par MidiDemo (qui possède le moniteur et l'historique des messages).
    std::function<void()> onRawToggled;    // re-rend le moniteur (changement Raw/Interprété)
    std::function<void()> onClearHistory;  // vide l'historique borné (bouton Clear)

    void resized() override
    {
        const int m = 10;
        auto r = getLocalBounds().reduced (m);
        const int half = (r.getWidth() - m) / 2;

        // Rangée Bluetooth (pleine largeur)
        pairBtn.setBounds (r.removeFromTop (24));
        r.removeFromTop (8);

        // Outils RE : Diff | Fingerprint, puis Lire valeurs (pleine largeur)
        auto row2 = r.removeFromTop (24);
        diffBtn.setBounds (row2.removeFromLeft (half));
        reBtn  .setBounds (row2.removeFromRight (half));
        r.removeFromTop (8);
        reReadBtn.setBounds (r.removeFromTop (24));
        r.removeFromTop (8);

        // En-tête du moniteur : libellé + toggle Raw à gauche, Copier/Clear à droite.
        auto head = r.removeFromTop (24);
        copyBtn .setBounds (head.removeFromRight (80));
        head.removeFromRight (6);
        clearBtn.setBounds (head.removeFromRight (80));
        head.removeFromRight (10);
        rawBtn.setBounds (head.removeFromLeft (220));
        monLab.setBounds (head);
        r.removeFromTop (4);

        monitorRef.setBounds (r);
    }

    // Déclenché par le bouton RE ; câblé par MidiDemo, qui possède les onglets et peut
    // parcourir TOUS les contenus (même non-courants — un TabbedComponent ne garde dans
    // l'arbre que l'onglet actif). Cf. la lambda onFingerprint dans MidiDemo.
    std::function<void()> onFingerprint;
    std::function<void()> onReadValues;

    TextButton& pairBtn;
    Label&      monLab;
    TextEditor& monitorRef;
    ToggleButton rawBtn    { "Moniteur : octets bruts (hex) au lieu de la forme decodee" };
    TextButton   diffBtn   { "Diff 2 dumps (.syx)..." };
    TextButton   reBtn     { "RE fingerprint -> CSV" };
    TextButton   reReadBtn { "RE lire valeurs -> CSV" };
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

        // INTERRUPTEURS écoute/envoi TOUJOURS VISIBLES (à gauche du toggle clavier).
        // Ce ne sont PLUS des menus de choix de périphérique (celui-ci reste dans l'onglet
        // Midi Setting) : ce sont deux toggles rapides ON/OFF pour DIAGNOSTIQUER.
        //  - btMidiIn  (« In ») : ON = l'app traite le MIDI entrant ; OFF = ignore tout entrant.
        //  - btMidiOut (« Out »): ON = l'app émet ; OFF = n'émet RIEN (coupe aussi le Thru).
        // Allumés (couleur ON du thème) quand actifs, atténués quand coupés. Défaut ON/ON, persistés.
        for (TextButton* b : { &btMidiIn, &btMidiOut })
        {
            addAndMakeVisible (*b);
            b->setAlwaysOnTop (true);
            b->setClickingTogglesState (true);
            b->setColour (TextButton::ColourIds::buttonOnColourId, SYColSelected);
        }
        midiListen = getAppSettings()->getBoolValue ("MidiListen", true);
        midiSend   = getAppSettings()->getBoolValue ("MidiSend",   true);
        btMidiIn .setToggleState (midiListen, dontSendNotification);
        btMidiOut.setToggleState (midiSend,   dontSendNotification);
        btMidiIn .setTooltip ("Écoute MIDI entrant : ON = traité, OFF = ignoré (diagnostic)");
        btMidiOut.setTooltip ("Envoi MIDI sortant : ON = émis, OFF = rien ne sort (coupe aussi le Thru)");
        btMidiIn .onClick = [this]
        {
            midiListen = btMidiIn.getToggleState();
            getAppSettings()->setValue ("MidiListen", midiListen);
            getAppSettings()->saveIfNeeded();
        };
        btMidiOut.onClick = [this]
        {
            midiSend = btMidiOut.getToggleState();
            getAppSettings()->setValue ("MidiSend", midiSend);
            getAppSettings()->saveIfNeeded();
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

        // Onglet "Midi Setting" : moniteur MIDI + outils RE (Bluetooth, Bulk). Réutilise les
        // composants de MidiDemo (re-parentés dans la page). Toujours construit (re-parente le
        // moniteur), mais AJOUTÉ À LA BARRE seulement en mode développeur (devMode).
        midiSettingsPage.reset (new MidiSettingsPage (pairButton, incomingMidiLabel, midiMonitor));
        devMode = getAppSettings()->getBoolValue ("DevMode", false);
        if (devMode)
            tabs.addTab (TRANS("Midi Setting"), SYColBackground, midiSettingsPage.get(), false);

        // Réglages MIDI rapatriés dans l'onglet "Setting" (ConfigPage) : on injecte les sélecteurs
        // d'interface In/Out et le bouton Bulk Protect (propriété de MidiDemo) dans la carte MIDI,
        // et on câble le toggle "Mode développeur" pour ajouter/retirer l'onglet "Midi Setting".
        if (auto* cfg = dynamic_cast<ConfigPage*> (tabs.getTabContentComponent (0)))
        {
            configPage = cfg;
            cfg->attachMidiInterface (midiInputLabel, *midiInputSelector,
                                      midiOutputLabel, *midiOutputSelector, btBulk);
            cfg->onDevModeChanged   = [this] (bool on) { setMidiSettingsTabVisible (on); };
            cfg->onTestConnection   = [this] { sendConnectionTest(); };
        }

        // Outil RE (jetable) : « empreinte ». Parcourt le contenu de CHAQUE onglet (via
        // getTabContentComponent, qui les renvoie même non-courants), envoie à chaque contrôle
        // câblé une valeur brute distinctive (rampe 1..120) et journalise (addr -> valeur) dans
        // re_fingerprint.csv. Workflow : dump AVANT -> clic -> dump APRÈS ; diff = nos octets, CSV = noms.
        midiSettingsPage->onFingerprint = [this]
        {
            juce::File csv = appDirPath.getChildFile ("re_fingerprint.csv");
            csv.deleteFile();
            juce::FileOutputStream os (csv);
            os << "idx,group,addrHi,addrLo,param,wireValue,type\n";
            int counter = 0;
            std::function<void(juce::Component*)> walk = [&] (juce::Component* c)
            {
                // Descend dans les sous-onglets imbriqués (AfmVue/FilterVue/WaveVue) : un
                // TabbedComponent ne garde dans l'arbre que l'onglet ACTIF -> on parcourt
                // explicitement TOUS ses contenus via getTabContentComponent, sinon on ne
                // voit que ~95 contrôles (les onglets affichés) au lieu de la totalité.
                if (auto* tc = dynamic_cast<juce::TabbedComponent*> (c))
                {
                    for (int t = 0; t < tc->getNumTabs(); ++t)
                        if (auto* cc = tc->getTabContentComponent (t))
                            walk (cc);
                    return;
                }
                for (auto* k : c->getChildren())
                {
                    int a[9]; bool sent = false; const char* type = "";
                    // Valeur distinctive et bien espacée, bornée 1..60 pour ne JAMAIS écrêter
                    // (la plupart des params sont 0..63/0..127). ×23 mod 60 (coprime) -> les
                    // contrôles voisins reçoivent des valeurs très différentes => lecture facile
                    // dans le dump. Période 60 ; au-delà, on lève l'ambiguïté par zone (OCR) + idx.
                    const int v = ((counter * 23) % 60) + 1;
                    if      (auto* s  = dynamic_cast<MidiSlider*>(k)) { sent = s->reSend  (v, a); type = "slider"; }
                    else if (auto* b  = dynamic_cast<MidiButton*>(k)) { sent = b->reSend  (v, a); type = "button"; }
                    else if (auto* cb = dynamic_cast<MidiCombo*> (k)) { sent = cb->reSend (v, a); type = "combo";  }
                    else if (auto* r  = dynamic_cast<MidiRadio*> (k)) { sent = r->reSend  (v, a); type = "radio";  }
                    if (sent)
                    {
                        os << counter << "," << a[3] << "," << a[4] << "," << a[5] << "," << a[6] << "," << a[8] << "," << type << "\n";
                        ++counter;
                        // Throttle : la chaîne d'envoi est SYNCHRONE (SysexBus -> sendMessageNow),
                        // donc ce sleep espace réellement les messages MIDI et évite la saturation
                        // de l'entrée du synthé. ~10 ms/msg -> ~2 s pour ~200 contrôles (UI figée
                        // le temps de l'opération, acceptable pour un outil RE jetable).
                        juce::Thread::sleep (10);
                    }
                    walk (k);
                }
            };
            for (int i = 0; i < tabs.getNumTabs(); ++i)
                if (auto* content = tabs.getTabContentComponent (i))
                    walk (content);
            os.flush();
            midiMonitor.moveCaretToEnd();
            midiMonitor.insertTextAtCaret ("\n[RE] fingerprint: " + juce::String (counter) + " controles -> " + csv.getFullPathName() + "\n");
        };

        // RE : exporte la valeur ACTUELLE de chaque contrôle câblé (lecture seule, aucun envoi).
        // L'éditeur reflétant la voix dumpée, ce CSV = texte clair complet à corréler au dump.
        midiSettingsPage->onReadValues = [this]
        {
            juce::File csv = appDirPath.getChildFile ("re_values.csv");
            csv.deleteFile();
            juce::FileOutputStream os (csv);
            os << "group,addrHi,addrLo,param,value,type\n";
            int counter = 0;
            std::function<void(juce::Component*)> walk = [&] (juce::Component* c)
            {
                // Même descente dans les TabbedComponent imbriqués que pour le fingerprint
                // (sinon la lecture ne couvre que les sous-onglets actifs).
                if (auto* tc = dynamic_cast<juce::TabbedComponent*> (c))
                {
                    for (int t = 0; t < tc->getNumTabs(); ++t)
                        if (auto* cc = tc->getTabContentComponent (t))
                            walk (cc);
                    return;
                }
                for (auto* k : c->getChildren())
                {
                    int a[9]; int val = 0; bool ok = false; const char* type = "";
                    if      (auto* s  = dynamic_cast<MidiSlider*>(k)) { ok = s->reRead  (a, val); type = "slider"; }
                    else if (auto* b  = dynamic_cast<MidiButton*>(k)) { ok = b->reRead  (a, val); type = "button"; }
                    else if (auto* cb = dynamic_cast<MidiCombo*> (k)) { ok = cb->reRead (a, val); type = "combo";  }
                    else if (auto* r  = dynamic_cast<MidiRadio*> (k)) { ok = r->reRead  (a, val); type = "radio";  }
                    if (ok) { os << a[3] << "," << a[4] << "," << a[5] << "," << a[6] << "," << val << "," << type << "\n"; ++counter; }
                    walk (k);
                }
            };
            for (int i = 0; i < tabs.getNumTabs(); ++i)
                if (auto* content = tabs.getTabContentComponent (i))
                    walk (content);
            os.flush();
            midiMonitor.moveCaretToEnd();
            midiMonitor.insertTextAtCaret ("\n[RE] valeurs: " + juce::String (counter) + " controles -> " + csv.getFullPathName() + "\n");
        };

        // Moniteur : bascule Raw/Interprété -> re-rend tout l'historique borné dans le nouveau mode.
        midiSettingsPage->onRawToggled   = [this] { rebuildMonitorFromHistory(); };
        // Bouton Clear : vide aussi l'historique borné (sinon un re-render le ferait réapparaître).
        midiSettingsPage->onClearHistory = [this] { monitorHistory.clearQuick(); };

        // Barre de navigation visible dès le démarrage (on n'atterrit plus sur une vue
        // sans menu). On ouvre "Setting" en premier : moyeu de configuration.
        tabs.setVisible(true);
        tabs.setAlwaysOnTop(true);

        setSize (1280, 820);
        // Onglet de démarrage (préférence "StartupTab", réglée dans Settings) :
        //   1 = "Dernier utilisé" -> restaure CurrentTab mémorisé ; 2.. = onglet fixe d'index 0..
        const int startPref = getAppSettings()->getIntValue ("StartupTab", 1);
        const int target    = (startPref <= 1) ? getAppSettings()->getIntValue ("CurrentTab", 0)
                                               : startPref - 2;
        tabs.setCurrentTabIndex (jlimit (0, tabs.getNumTabs() - 1, target));
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
            // Bulk Protect ON/OFF : param-change standard 0x34, group 0x0F (System Setup),
            // param 0x34. Construit par le builder unique (octets identiques à l'ancien inline).
            auto sysexdata = SyVoice::paramBytes (sysexDeviceNumber, 0x0f, 0x00, 0x00, 0x34,
                                                  (uint8) btBulk.getToggleState());
            MidiMessage m = MidiMessage::createSysExMessage (sysexdata.data(), 9);
            m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
    }
    // Mode développeur : ajoute/retire l'onglet "Midi Setting" (moniteur + outils RE) à chaud.
    // L'onglet est toujours le dernier ; midiSettingsPage reste vivant (non possédé par tabs).
    void setMidiSettingsTabVisible (bool on)
    {
        int found = -1;
        for (int i = 0; i < tabs.getNumTabs(); ++i)
            if (tabs.getTabNames()[i] == TRANS("Midi Setting")) { found = i; break; }

        if (on && found < 0)
            tabs.addTab (TRANS("Midi Setting"), SYColBackground, midiSettingsPage.get(), false);
        else if (! on && found >= 0)
        {
            if (tabs.getCurrentTabIndex() == found)
                tabs.setCurrentTabIndex (0);   // on quitte l'onglet avant de le retirer
            tabs.removeTab (found);
        }
    }

    // Test de connexion : demande au synthé d'émettre quelque chose et attend SA réponse.
    // Le SY77 (1989) n'implémente pas forcément l'Identity Request universel (F0 7E 06 01) ; en
    // revanche il répond de façon FIABLE à une demande de dump de voix (Yamaha F0 43 ...). On
    // envoie donc les deux : la réponse (dump Yamaha OU Identity Reply) est détectée dans
    // handleAsyncUpdate, et un timeout est géré dans timerCallback. Statut affiché dans la carte MIDI.
    void sendConnectionTest()
    {
        if (configPage == nullptr) return;
        if (! midiSend)
        {
            configPage->setConnStatus (TRANS("MIDI output is off (enable \"Out\")"), false);
            awaitingConnReply = false;
            return;
        }
        if (! midiListen)
        {
            configPage->setConnStatus (TRANS("MIDI input is off (enable \"In\")"), false);
            awaitingConnReply = false;
            return;
        }
        awaitingConnReply = true;
        connDeadlineMs    = Time::getMillisecondCounter() + 1500;
        configPage->setConnStatus (TRANS("Testing connection..."), false);

        // Identity Request universel (matériel générique) — inoffensif si ignoré.
        const uint8 inquiry[] = { 0x7E, 0x7F, 0x06, 0x01 };   // sans F0/F7 (ajoutés par JUCE)
        MidiMessage idReq = MidiMessage::createSysExMessage (inquiry, (int) sizeof (inquiry));
        idReq.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (idReq);

        // Demande de dump de voix (interne, slot 0) — le SY77 y répond par un bulk Yamaha.
        MidiMessage dumpReq = SyVoice::voiceDumpRequest (sysexDeviceNumber, 0x00, 0x00);
        dumpReq.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (dumpReq);
    }

    //==============================================================================
    void timerCallback() override
    {
        // Test de connexion : pas de réponse dans le délai -> on le signale.
        if (awaitingConnReply && Time::getMillisecondCounter() > connDeadlineMs)
        {
            awaitingConnReply = false;
            if (configPage != nullptr)
                configPage->setConnStatus (TRANS("No response from the synth"), false);
        }

        // "Midi Setting" est désormais un onglet à part entière (MidiSettingsPage) :
        // plus besoin de masquer la barre de navigation à sa sélection.
        updateDeviceList (true);
        updateDeviceList (false);
        refreshMidiPortButtons();   // tient à jour le compteur de ports ouverts (hot-plug inclus)
    }

    // Affiche le nombre de ports ouverts DANS le libellé des interrupteurs ("In", "In·2"…),
    // sans toucher à leur état ON/OFF (qui est désormais le gate écoute/envoi, pas le compte
    // de ports). Source de vérité partagée avec l'onglet Midi Setting (midiInputs/midiOutputs).
    void refreshMidiPortButtons()
    {
        auto openCount = [] (const ReferenceCountedArray<MidiDeviceListEntry>& devs, bool isInput)
        {
            int n = 0;
            for (auto& d : devs)
                if (isInput ? d->inDevice.get() != nullptr : d->outDevice.get() != nullptr) ++n;
            return n;
        };
        const int nin = openCount (midiInputs, true), nout = openCount (midiOutputs, false);
        btMidiIn .setButtonText (nin  > 0 ? "In "  + String (nin)  : String ("In"));
        btMidiOut.setButtonText (nout > 0 ? "Out " + String (nout) : String ("Out"));
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
        auto bounds = getLocalBounds().toFloat();

        // Fond : dégradé vertical subtil (haut légèrement plus clair) -> donne de la
        // profondeur sans dénaturer la couleur du thème. Pilote par SYPal.dark.
        g.setGradientFill (ColourGradient (
            SYPal.background.brighter (SYPal.dark ? 0.06f : 0.015f), bounds.getCentreX(), 0.0f,
            SYPal.background.darker   (SYPal.dark ? 0.16f : 0.02f),  bounds.getCentreX(), bounds.getBottom(), false));
        g.fillRect (bounds);

        // Halo d'accent diffus, centre haut (signature « synth »), très discret.
        {
            ColourGradient glow (SYPal.accent.withAlpha (SYPal.dark ? 0.09f : 0.05f),
                                 bounds.getCentreX(), 0.0f,
                                 Colours::transparentBlack,
                                 bounds.getCentreX(), bounds.getHeight() * 0.55f, true);
            g.setGradientFill (glow);
            g.fillRect (bounds);
        }

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
        const int toggleX = getWidth() - margin - 70;
        btToggleKeyboard.setBounds (toggleX, barTop + 3, 70, 26);
        btToggleKeyboard.toFront (false); // au-dessus de la barre d'onglets (sinon le clic part dans les onglets)

        // Sélecteurs in/out TOUJOURS VISIBLES, à GAUCHE du toggle clavier, sur la même rangée.
        // Compacts (54 px) ; ordre [In][Out][Clavier] de gauche à droite.
        const int selW = 54, gap = 4;
        const int outX = toggleX - gap - selW;
        const int inX  = outX  - gap - selW;
        btMidiIn .setBounds (inX,  barTop + 3, selW, 26);
        btMidiOut.setBounds (outX, barTop + 3, selW, 26);
        btMidiIn .toFront (false);
        btMidiOut.toFront (false);
    }

    // Sélecteur de ports compact (bandeau bas) : PopupMenu à cases cochables (multi-sélection).
    // RÉUTILISE la gestion de ports existante -> SOURCE DE VÉRITÉ UNIQUE partagée avec l'onglet
    // Midi Setting : mêmes listes (midiInputs/midiOutputs), mêmes openDevice/closeDevice, même
    // persistance (saveEnabledDevices -> MidiInDevices/MidiOutDevices). Après bascule, on re-synchronise
    // la ListBox de l'onglet (syncSelectedItemsWithDeviceList) -> les deux vues restent COHÉRENTES.
    void showMidiPortMenu (bool isInput, Component& anchor)
    {
        auto& devices = isInput ? midiInputs : midiOutputs;

        PopupMenu menu;
        menu.addSectionHeader (isInput ? "Entrées MIDI" : "Sorties MIDI");
        if (devices.isEmpty())
            menu.addItem (-1, "(aucun périphérique)", false, false);

        for (int i = 0; i < devices.size(); ++i)
        {
            const bool isOpen = isInput ? devices[i]->inDevice.get()  != nullptr
                                        : devices[i]->outDevice.get() != nullptr;
            // itemID = index+1 (0 = menu annulé) ; ticked = port ouvert ; toggle au clic.
            menu.addItem (i + 1, devices[i]->deviceInfo.name, true, isOpen);
        }

        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&anchor),
                            [this, isInput] (int result)
        {
            if (result <= 0) return;                       // menu annulé / item désactivé
            const int index = result - 1;
            auto& devs = isInput ? midiInputs : midiOutputs;
            if (! isPositiveAndBelow (index, devs.size())) return;

            const bool isOpen = isInput ? devs[index]->inDevice.get()  != nullptr
                                        : devs[index]->outDevice.get() != nullptr;
            if (isOpen) closeDevice (isInput, index);      // bascule : ouvre/ferme le port
            else        openDevice  (isInput, index);

            saveEnabledDevices (isInput);                  // persistance (même clé que l'onglet)

            // Reflète l'état dans la ListBox de l'onglet Midi Setting -> vues cohérentes.
            if (auto* sel = isInput ? midiInputSelector.get() : midiOutputSelector.get())
                sel->syncSelectedItemsWithDeviceList (isInput ? midiInputs : midiOutputs);

            refreshMidiPortButtons();   // met à jour le libellé/état des sélecteurs compacts
        });
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
                // Échec d'ouverture du port : visible (Logger, pas DBG strippé en Release) ->
                // diagnostic quand « le synthé ne répond pas » (le port n'a pas pu s'ouvrir).
                Logger::writeToLog ("ERREUR : ouverture de la sortie MIDI #" + String (index) + " echouee");
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
        // Interrupteur « In » (écoute) OFF : on IGNORE tout MIDI entrant -> rien n'est traité
        // ni affiché ni appliqué aux widgets (utile pour isoler un écho reçu).
        if (! midiListen) return;
        const ScopedLock sl (midiMonitorLock);
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
    // Octets bruts d'un message MIDI en hex ("F0 43 10 …"). Sysex inclut F0/F7.
    static String midiBytesToHex (const MidiMessage& m)
    {
        const uint8* d = m.getRawData();
        const int n = m.getRawDataSize();
        String s;
        for (int i = 0; i < n; ++i)
            s << String::toHexString (d[i]).paddedLeft ('0', 2).toUpperCase() << (i + 1 < n ? " " : "");
        return s;
    }

    // Rend UNE ligne de moniteur pour un message, selon le mode courant (brut/décodé).
    // Retourne false si le message doit être OMIS (bruit filtré, ou dédup en mode décodé).
    // `dedup` porte l'état de dédup entre appels (en mode décodé : une ligne par adresse).
    bool formatMonitorLine (const MidiMessage& message, String& out, String& dedup)
    {
        // --- Mode BRUT : chaque message en hex, AUCUNE déduplication (on veut tout voir).
        if (monitorRaw)
        {
            out << midiBytesToHex (message) << "\n";
            return true;
        }

        // --- Mode DÉCODÉ : param-change SY77 9 octets -> G/AH/AL/P/val, dédup par adresse.
        if (message.getSysExDataSize() == 9)
        {
            const uint8* d = message.getSysExData();
            if (d[0] == 0x43 && d[2] == 0x34
                && SyVoice::acceptsDevice (d[1], sysexDeviceNumber, sysexReceiveOmni))
            {
                const String key = String (d[3]) + "." + String (d[4]) + "."
                                 + String (d[5]) + "." + String (d[6]);
                if (key == dedup) return false;          // même adresse que la précédente -> omise
                dedup = key;
                auto hx = [] (int v) { return String::toHexString (v).paddedLeft ('0', 2); };
                out << ">> SY77  G=" << hx (d[3])
                    << "  AH=" << hx (d[4]) << "  AL=" << hx (d[5])
                    << "  P="  << hx (d[6])
                    << "   val=" << (int) d[8] << " (0x" << hx (d[8]) << ")\n";
                return true;
            }
            return false; // sysex 9 octets non-SY77 : pas de description brute en mode décodé
        }

        // Bruit filtré pour la RE : CC data-entry, program change, horloge, active-sense, transport.
        if (message.isController() || message.isProgramChange() || message.isMidiClock()
            || message.isActiveSense() || message.isMidiStart() || message.isMidiContinue()
            || message.isMidiStop() || message.isSongPositionPointer())
            return false;

        out << message.getDescription() << "\n";
        return true;
    }

    // Re-rend tout le moniteur depuis l'historique borné (ex. au changement Raw/Interprété).
    // Reproduit la dédup « par adresse » du flux temps réel (clé réinitialisée au début).
    void rebuildMonitorFromHistory()
    {
        String text, dedup;
        for (auto& m : monitorHistory)
        {
            String line;
            if (formatMonitorLine (m, line, dedup))
                text << line;
        }
        midiMonitor.clear();
        midiMonitor.setText (text, dontSendNotification);
        lastMonitorKey = dedup;   // poursuit la dédup pour les messages suivants
        midiMonitor.moveCaretToEnd();
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
            // (MIDI Thru logiciel supprimé : le SY77 a son propre Thru hardware, et un thru
            //  logiciel sur un port bidirectionnel renvoie au synthé ce qu'il vient d'émettre
            //  -> doublons/écho. On ne forwarde donc plus l'entrée vers la sortie.)

            // Historique borné pour pouvoir RE-RENDRE le moniteur au changement de mode.
            monitorHistory.add (message);
            if (monitorHistory.size() > kMonitorHistoryMax)
                monitorHistory.removeRange (0, monitorHistory.size() - kMonitorHistoryMax);

            // Test de connexion : on accepte SOIT un dump/param Yamaha (F0 43 ..., réponse à notre
            // demande de dump = preuve que le SY77 parle), SOIT un Identity Reply universel
            // (F0 7E <dev> 06 02 <fab> ...) pour du matériel générique.
            if (awaitingConnReply && message.isSysEx())
            {
                const uint8* id = message.getSysExData();
                const int    n  = message.getSysExDataSize();
                const bool yamaha   = (n >= 1 && id[0] == 0x43);
                const bool identity = (n >= 5 && id[0] == 0x7E && id[2] == 0x06 && id[3] == 0x02);
                if (yamaha || identity)
                {
                    awaitingConnReply = false;
                    const bool isYam = yamaha || (identity && id[4] == 0x43);
                    if (configPage != nullptr)
                        configPage->setConnStatus (isYam ? TRANS("Connected - Yamaha synth detected")
                                                         : TRANS("Connected - device responded"), true);
                }
            }

            // Application aux widgets : UNIQUEMENT pour les param-changes SY77 9 octets adressés.
            if (message.getSysExDataSize() == 9)
            {
                memcpy (&data, message.getSysExData(), message.getSysExDataSize());
                if (data[0] == 0x43 && data[2] == 0x34
                    && SyVoice::acceptsDevice (data[1], sysexDeviceNumber, sysexReceiveOmni))
                {
                    // ANTI-ÉCHO : on APPLIQUE le param reçu (mise à jour des widgets) sous
                    // suppression d'envoi. Tout widget qui ré-émettrait en cascade (combo,
                    // octet packé recomposé, etc.) est neutralisé -> pas de renvoi vers le
                    // SY77, donc pas de ré-écho -> la boucle A↔B est coupée à la source.
                    const ScopedEchoSuppress noEcho;
                    // Notification SYNCHRONE (true) : les widgets s'appliquent ICI, DANS la
                    // portée du garde anti-écho. La notification ASYNC par défaut de Value
                    // s'exécuterait APRÈS destruction du garde -> un éventuel ré-envoi
                    // échapperait à la suppression (c'était le trou de la boucle d'écho).
                    valueSysexIn = make_var_array(data[3], data[4],data[5],data[6],data[7],data[8]);
                    valueSysexIn.getValueSource().sendChangeMessage (true);

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
                }
            }

            // Affichage moniteur (mode courant brut/décodé + dédup), via le formateur partagé.
            String line;
            if (formatMonitorLine (message, line, lastMonitorKey))
                messageText << line;
        }
        if (messageText.isNotEmpty())
            midiMonitor.insertTextAtCaret (messageText);

    }
public:
    void sendToOutputs (const MidiMessage& msg)
    {
        // Interrupteur « Out » (envoi) OFF : l'app n'émet RIEN vers le synthé. Point de coupe
        // unique -> couvre notes, param-changes/sysex, bulk ET le MIDI Thru (qui passe par ici),
        // donc OFF = rien ne sort, Thru compris (cohérence demandée).
        if (! midiSend) return;
        // Anti-écho assuré à la RÉCEPTION par dontSendNotification dans chaque widget Midi*
        // (et par setValue() sans renvoi pour Segment) ; pas de flag global non thread-safe.
        for (auto midiOutput : midiOutputs)
            if (midiOutput->outDevice.get() != nullptr)
            {

                midiOutput->outDevice->sendMessageNow (msg);
                // (Avant : 2 lignes de log PAR message sortant -> inondait le log lors d'un
                //  envoi de banque. Retiré du chemin chaud.)
            }
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
        auto sysexdata = SyVoice::paramBytes (sysexDeviceNumber, 0x0f, 0x00, 0x00, 0x2d,
                                              (uint8) (comboFoot.getSelectedItemIndex()+1));
        MidiMessage m = MidiMessage::createSysExMessage (sysexdata.data(), 9);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
        }
        if(comboBoxThatHasChanged == &comboMod)
        {
            auto sysexdata = SyVoice::paramBytes (sysexDeviceNumber, 0x0f, 0x00, 0x00, 0x2c,
                                                  (uint8) (comboMod.getSelectedItemIndex()+1));
            MidiMessage m = MidiMessage::createSysExMessage (sysexdata.data(), 9);
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
    // Sélecteurs de ports MIDI TOUJOURS VISIBLES (bandeau bas, à gauche du toggle clavier).
    // Ouvrent un PopupMenu à cases cochables ; même source de vérité que l'onglet Midi Setting
    // (midiInputs/midiOutputs + openDevice/closeDevice + saveEnabledDevices). Cf. showMidiPortMenu.
    TextButton  btMidiIn  {"In"};
    TextButton  btMidiOut {"Out"};
    std::unique_ptr<MidiDeviceListBox> midiInputSelector, midiOutputSelector;
    std::unique_ptr<MidiSettingsPage>  midiSettingsPage;
    ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;
    
    CriticalSection midiMonitorLock;
    Array<MidiMessage> incomingMessages;
    // Historique borné des messages affichés -> permet de RE-RENDRE le moniteur quand on
    // bascule Raw/Interprété (s'applique donc aussi aux lignes déjà présentes). Borne mémoire.
    Array<MidiMessage> monitorHistory;
    static constexpr int kMonitorHistoryMax = 2000;
    DemoTabbedComponent tabs;
    // Active TOUTES les infobulles (setTooltip) de l'app : sans un TooltipWindow vivant,
    // aucun tooltip ne s'affiche. Un seul suffit pour tout l'arbre.
    TooltipWindow tooltipWindow { this, 600 };

    // Test de connexion (carte MIDI de Settings) : envoi d'une requête + attente de la réponse.
    ConfigPage* configPage      = nullptr;  // page "Setting" (pour publier le statut de connexion)
    bool        awaitingConnReply = false;
    uint32      connDeadlineMs   = 0;

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
