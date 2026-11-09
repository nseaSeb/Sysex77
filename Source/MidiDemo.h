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
#include "LibraryIndex.h" // index librairie (métadonnées/tags/recherche, library.json)
#include "Updater.h"      // mise à jour in-app depuis GitHub Releases
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
static String currentBankRelPath;    // chemin relatif de la banque sélectionnée (clé LibraryIndex)
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
// Channel of the notes played by the on-screen keyboard and the sequencer.
// 0 = follow the device number (channel 1 when the device is ALL), 1..16 = fixed.
static int  playChannelSetting = 0;
inline int currentPlayChannel()
{
    if (playChannelSetting >= 1 && playChannelSetting <= 16) return playChannelSetting;
    return sysexReceiveOmni ? 1 : juce::jlimit (1, 16, sysexDeviceNumber);
}
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
#include "Sequencer.h"
#include "EditHistory.h"
#include "Templates.h"           // "New voice" templates (INIT voices)         // sysex-level undo/redo + A/B compare            // bottom-bar phrase player (Seq mode)
#include "BankSendProgress.h"      // fenêtre de progression de l'envoi de banque
#include "BankReceiveProgress.h"   // fenêtre de progression de la réception de banque
#include "Voice.h"
#include "Librairie.h"
#include "Effects.h"
#include "ControllersPage.h"
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
    : TabbedComponent (TabbedButtonBar::TabsAtTop)   // navigation en haut, sous le bandeau
    {
        setTabBarDepth (32);
        getTabbedButtonBar().getProperties().set ("syEqualTabs", true);   // largeur egale (LookAndFeel)
        auto colour = findColour (ResizableWindow::backgroundColourId);
        addTab (TRANS("Setting"),     colour, new ConfigPage (), true);  
        addTab (TRANS("Librairie"),     colour, new LibrairiePage (), true);
       // addTab (TRANS("Midi"),     colour, new ControllerPage (), true);
        addTab (TRANS("Voice"), colour, new VoicePage(), true);
        addTab (TRANS("Effects"), colour, new EffectsPage(), true);
        addTab (TRANS("Controllers"), colour, new ControllersPage(), true);
        // L'onglet "Midi Setting" (index 5, mode développeur) est ajouté par MidiDemo avec un vrai
        // contenu (MidiSettingsPage), car il réutilise des composants de MidiDemo.

        // ConfigPage (1er onglet) a appliqué le thème pendant sa construction :
        // on aligne donc le fond des onglets sur la couleur de fond du thème.
        for (int i = 0; i < getNumTabs(); ++i)
            setTabBackgroundColour (i, SYColBackground);
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

    // Re-clic sur l'onglet DEJA actif : currentTabChanged n'est pas emis (index inchange).
    // Sur VOICE, cela ferme l'editeur ouvert et ramene a la vue des 4 elements (reflexe
    // « remonter d'un niveau »). Revenir sur VOICE depuis un autre onglet, lui, garde l'editeur.
    struct ReclickTabButton : public TabBarButton
    {
        ReclickTabButton (const String& name, TabbedButtonBar& bar, DemoTabbedComponent& tc)
            : TabBarButton (name, bar), tabs (tc) {}
        void clicked (const ModifierKeys& mods) override
        {
            if (! mods.isPopupMenu() && getIndex() == owner.getCurrentTabIndex())
                tabs.currentTabReclicked (getIndex());
            TabBarButton::clicked (mods);
        }
        DemoTabbedComponent& tabs;
    };
    TabBarButton* createTabButton (const String& tabName, int /*tabIndex*/) override
    {
        return new ReclickTabButton (tabName, getTabbedButtonBar(), *this);
    }
    void currentTabReclicked (int index)
    {
        if (auto* vp = dynamic_cast<VoicePage*> (getTabContentComponent (index)))
            vp->closeOpenEditor();
    }
    void popupMenuClickOnTab (int tabIndex, const String& tabName)override
    {
        Logger::writeToLog("TabbedComponent: popupMenuClick");
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoTabbedComponent)
};

//==============================================================================
/** Fleche du bandeau superieur (voix precedente/suivante) : triangle dessine en Path,
    couleurs du theme (pas de glyphe UTF-8). */
struct VoiceNavArrow : public Button
{
    VoiceNavArrow (bool pointsLeft) : Button (pointsLeft ? "prevVoice" : "nextVoice"), left (pointsLeft) {}
    void paintButton (Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat();
        if (over || down)
        {
            g.setColour (SYPal.accent.withAlpha (down ? 0.30f : 0.16f));
            g.fillEllipse (r.withSizeKeepingCentre (r.getHeight() - 4.0f, r.getHeight() - 4.0f));
        }
        const float s = jmin (r.getWidth(), r.getHeight()) * 0.22f;
        const float cx = r.getCentreX(), cy = r.getCentreY();
        Path p;
        if (left) p.addTriangle (cx + s * 0.6f, cy - s, cx + s * 0.6f, cy + s, cx - s * 0.8f, cy);
        else      p.addTriangle (cx - s * 0.6f, cy - s, cx - s * 0.6f, cy + s, cx + s * 0.8f, cy);
        g.setColour (isEnabled() ? (over ? SYPal.accent : SYPal.textPrimary) : SYPal.textMuted);
        g.fillPath (p);
    }
    const bool left;
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
    /** Action de raccourci clavier (cf. Shortcuts.h). true si traitee. */
    bool handleShortcut (const juce::String& id)
    {
        if (id.startsWith ("tab"))
        {
            const int i = id.substring (3).getIntValue();
            if (i < 0 || i >= tabs.getNumTabs()) return false;
            tabs.setCurrentTabIndex (i);
            return true;
        }
        if (id == "seqplay")
        {
            if (barMode != 2) setBarMode (2);
            seqPanel.togglePlay();
            return true;
        }
        if (id == "voicenext" || id == "voiceprev" || id == "banknext" || id == "bankprev")
        {
            auto* lib = [this]() -> LibrairiePage*
            {
                for (int i = 0; i < tabs.getNumTabs(); ++i)
                    if (auto* l = dynamic_cast<LibrairiePage*> (tabs.getTabContentComponent (i))) return l;
                return nullptr;
            }();
            if (lib == nullptr) return false;
            if (id == "banknext" || id == "bankprev") { lib->stepBank (id == "banknext" ? 1 : -1); return true; }
            const int delta = id == "voicenext" ? 1 : -1;
            // Browsing replaces the voice being edited: ask once if there are unsaved edits.
            if (editHistory.isLoaded() && ! editHistory.diffFromOriginal().empty())
            {
                Component::SafePointer<LibrairiePage> safeLib (lib);
                NativeMessageBox::showOkCancelBox (MessageBoxIconType::QuestionIcon, TRANS ("New voice"),
                    TRANS ("Replace the voice being edited? Unsaved edits will be lost."), this,
                    ModalCallbackFunction::create ([safeLib, delta] (int r)
                    { if (r == 1 && safeLib != nullptr) safeLib->stepVoice (delta); }));
                return true;
            }
            lib->stepVoice (delta);
            return true;
        }
        if (id == "ab")       { setShowingA (! showingA); return true; }
        if (id == "newvoice")
        {
            tabs.setCurrentTabIndex (2);   // meme effet qu'avant : on atterrit sur la page Voice
            MessageManager::callAsync ([safe = Component::SafePointer<MidiDemo> (this)]
                                       { if (safe != nullptr) safe->showNewVoiceMenu(); });
            return true;
        }
        if (id == "help")
        {
            DialogWindow::LaunchOptions o;
            o.content.setOwned (new Shortcuts::CheatSheet());
            o.dialogTitle = TRANS ("Keyboard shortcuts");
            o.dialogBackgroundColour = SYPal.background;
            o.escapeKeyTriggersCloseButton = true;
            o.useNativeTitleBar = true;
            o.resizable = false;
            o.componentToCentreAround = this;
            o.launchAsync();
            return true;
        }
        if (id == "undo") { historyUndo(); return true; }
        if (id == "redo") { historyRedo(); return true; }
        if (id == "close")   // Echap : seulement si l'ecran Voice est affiche (pas un editeur cache)
            if (auto* vp = dynamic_cast<VoicePage*> (tabs.getCurrentContentComponent()))
                return vp->closeOpenEditor();
        return false;
    }

    //==============================================================================
    MidiDemo (bool isRunningComponenTransformsDemo = false)
    :  midiKeyboard       (keyboardState, MidiKeyboardComponent::horizontalKeyboard),
    midiInputSelector  (new MidiDeviceListBox ("Midi Input Selector",  *this, true)),
    midiOutputSelector (new MidiDeviceListBox ("Midi Output Selector", *this, false)),
    tabs (isRunningComponenTransformsDemo)
    
    {
        // Index de la librairie : racine = dossier des banques, chargé puis réconcilié au disque
        // en tâche de fond (scan des .syx -> synthé détecté + cache des noms pour la recherche).
        LibraryIndex::get().setRoot (appDirPath);
        LibraryIndex::get().load();
        juce::Thread::launch ([] { LibraryIndex::get().reconcile(); });

        // Mise à jour in-app : vérif GitHub au démarrage (non bloquante, throttlée, opt-out).
        Updater::get().onUpdateAvailable = [this] (Updater::Info i) { showUpdateDialog (i); };
        Updater::get().onMessage = [] (juce::String m)
        {
            juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                                         TRANS("Mise a jour"), m);
        };
        Updater::get().checkAsync (false);

        // Edit history (undo/redo, A/B, revert) — actions used by the Voice header bar.
        {
            auto& h = EditHist::Hooks::get();
            h.undo     = [this] { historyUndo(); };
            h.redo     = [this] { historyRedo(); };
            h.toggleAB = [this] { setShowingA (! showingA); };
            h.revert   = [this] { askRevert(); };
        }

        // (Images de fond Sysex77/99.png supprimées : jamais dessinées — nettoyage des ressources.)
        addAndMakeVisible (tabs);
  
        addLabelAndSetStyle (midiInputLabel);
        addLabelAndSetStyle (midiOutputLabel);
        addLabelAndSetStyle (incomingMidiLabel);
        
        addAndMakeVisible(btBulk);
        btBulk.setClickingTogglesState(true);
        btBulk.addListener(this);

        // Bottom bar mode selector [None | Keys | Seq] (replaces the old keyboard toggle).
        // Default migrated from the former "KeyboardVisible" setting.
        barMode = jlimit (0, 2, getAppSettings()->getIntValue ("BottomBarMode",
                                getAppSettings()->getBoolValue ("KeyboardVisible", true) ? 1 : 0));
        // One compact button (same width as the old toggle, keeps the tab bar free on
        // narrow windows) opening a menu None / Keys / Seq.
        addAndMakeVisible (btBarMode);
        btBarMode.setAlwaysOnTop (true);   // stays above the tab bar
        btBarMode.setTooltip (TRANS ("Bottom bar: none, on-screen keyboard or phrase player (loops a phrase while you edit)"));
        btBarMode.onClick = [this]
        {
            PopupMenu m;
            m.addItem (1, TRANS ("None"), true, barMode == 0);
            m.addItem (2, TRANS ("Keys"), true, barMode == 1);
            m.addItem (3, TRANS ("Seq"),  true, barMode == 2);
            m.showMenuAsync (PopupMenu::Options().withTargetComponent (&btBarMode),
                             [this] (int r) { if (r > 0) setBarMode (r - 1); });
        };
        refreshBarModeButton();
        addChildComponent (seqPanel);

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
        midiKeyboard.setVisible (barMode != 0); // applique l'état mémorisé
        
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

        // Bandeau superieur : < nom de voix > au centre. Les fleches font EXACTEMENT les
        // raccourcis voix precedente/suivante (handleShortcut : meme confirmation si edits
        // non sauves, meme stepVoice de la librairie). Le champ nom est DEPLACE depuis la
        // page Voice (pas duplique) : meme TextEditor, meme envoi au Retour (VNAM).
        for (auto* a : { &btVoicePrev, &btVoiceNext })
        {
            addAndMakeVisible (*a);
            a->setAlwaysOnTop (true);
        }
        btVoicePrev.onClick = [this] { handleShortcut ("voiceprev"); };
        // « New... » (voix depuis un modele) : DEPLACE depuis la barre d'historique de la page
        // Voice. Meme menu, meme askNewVoice (meme confirmation si edits non sauves).
        addAndMakeVisible (btNewVoice);
        btNewVoice.setAlwaysOnTop (true);
        btNewVoice.onClick = [this] { showNewVoiceMenu(); };
        btVoiceNext.onClick = [this] { handleShortcut ("voicenext"); };
        refreshVoiceNavTooltips();
        tipRefresher.f = [this] { refreshVoiceNavTooltips(); };
        Shortcuts::changed().addChangeListener (&tipRefresher);
        for (int i = 0; i < tabs.getNumTabs(); ++i)
            if (auto* vp = dynamic_cast<VoicePage*> (tabs.getTabContentComponent (i)))
            {
                nameEditor = &vp->editName;
                addAndMakeVisible (vp->editName);
                vp->editName.setAlwaysOnTop (true);
                vp->editName.setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
                vp->editName.setColour (TextEditor::outlineColourId, Colours::transparentBlack);
                vp->editName.setFont (Font (FontOptions (15.0f)).boldened());
                vp->editName.applyFontToAllText (Font (FontOptions (15.0f)).boldened());
                vp->editName.setTooltip (TRANS ("Voice name (click to rename, Return sends it to the synth)"));
                vp->resized();
            }

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
    
    // Dialogue « mise à jour disponible » (sur le thread message). Récursif via « Notes ».
    void showUpdateDialog (Updater::Info info)
    {
        juce::AlertWindow w (TRANS("Mise a jour disponible"),
            "Sysex77 " + info.tag + TRANS(" est disponible (vous avez v") + Sysex77::kVersion + ").\n\n"
                + info.notes.substring (0, 500),
            juce::MessageBoxIconType::InfoIcon);
       #if JUCE_MAC
        w.addButton (TRANS("Installer"),               1, juce::KeyPress (juce::KeyPress::returnKey));
       #else
        // No auto-install outside macOS: "Download" opens the release page (Windows zip / Linux tar.gz).
        w.addButton (TRANS("Telecharger"),             5, juce::KeyPress (juce::KeyPress::returnKey));
       #endif
        w.addButton (TRANS("Notes"),                   2);
        w.addButton (TRANS("Plus tard"),               3, juce::KeyPress (juce::KeyPress::escapeKey));
        w.addButton (TRANS("Ignorer cette version"),   4);

        const int r = w.runModalLoop();
        if (r == 1)      Updater::get().downloadAndInstall (info);
        else if (r == 2) { juce::URL (info.htmlUrl).launchInDefaultBrowser(); showUpdateDialog (info); }
        else if (r == 4) Updater::get().skipVersion (info.tag);
        else if (r == 5) juce::URL (info.htmlUrl).launchInDefaultBrowser();
    }

    ~MidiDemo()
    {
        Shortcuts::changed().removeChangeListener (&tipRefresher);
        if (nameEditor != nullptr) removeChildComponent (nameEditor);   // rendu a personne avant la VoicePage
        seqPlayer.shutdown();   // its thread sends through sendToOutputs: stop it first
        {
            auto& h = EditHist::Hooks::get();
            h.undo = h.redo = h.toggleAB = h.revert = nullptr;
        }
        stopTimer();
        Updater::get().onUpdateAvailable = nullptr;
        Updater::get().onMessage = nullptr;
        SysexBus::get().onMessage = nullptr;   // lambda posée dans MidiSysex.h (capture this)

        midiInputs .clear();
        {
            const ScopedLock sl (outLock);
            midiOutputs.clear();
        }
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
        if (barMode == 2)   // Seq mode: a key sets the phrase root instead of sounding
        {
            seqPanel.setRoot (midiNoteNumber);
            return;
        }
        // Channel chosen in Settings (play channel), not the keyboard component's own one.
        juce::ignoreUnused (midiChannel);
        const int ch = currentPlayChannel();
        keyChannel[(size_t) midiNoteNumber] = ch;   // note-off goes to the same channel
        MidiMessage m (MidiMessage::noteOn (ch, midiNoteNumber, velocity));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }
    
    void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (barMode == 2) return;
        juce::ignoreUnused (midiChannel);
        const int ch = keyChannel[(size_t) midiNoteNumber] > 0 ? keyChannel[(size_t) midiNoteNumber]
                                                               : currentPlayChannel();
        keyChannel[(size_t) midiNoteNumber] = 0;
        MidiMessage m (MidiMessage::noteOff (ch, midiNoteNumber, velocity));
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

        // Bandeau superieur persistant : fond surface + filet bas.
        auto bar = getLocalBounds().removeFromTop (kTopBarH).toFloat();
        g.setColour (SYPal.surface.withAlpha (0.55f));
        g.fillRect (bar);
        g.setColour (SYPal.panelBorder.withAlpha (0.7f));
        g.drawHorizontalLine (kTopBarH - 1, 0.0f, bar.getRight());

        // Nom de l'app, discret, a gauche (suit le modele 77/99).
        auto logoText = (SYModel == 3 ? String ("SYSEX 99") : String ("SYSEX 77"));
        g.setColour (SYPal.textMuted);
        g.setFont (Font (FontOptions (13.0f)).boldened().withExtraKerningFactor (0.12f));
        g.drawText (logoText, 14, 0, 140, kTopBarH, Justification::centredLeft, true);

        // Pilule du nom de voix : fond + contour, derriere les fleches et le champ nom.
        if (! namePill.isEmpty())
        {
            auto p = namePill.toFloat().reduced (0.5f);
            g.setColour (SYPal.surfaceAlt);
            g.fillRoundedRectangle (p, p.getHeight() * 0.5f);
            g.setColour (SYPal.panelBorder);
            g.drawRoundedRectangle (p, p.getHeight() * 0.5f, 1.0f);
        }
    }
    // (L'ancien clic sur le fond qui masquait/affichait les onglets est supprime : avec un
    // bandeau superieur cliquable, il cachait toute l'interface par erreur.)
    void resized() override
    {
        auto margin = 10;
        
        
        
        // Les composants MIDI (interfaces, moniteur, pair, bulk) sont désormais
        // disposés par MidiSettingsPage (onglet 3) — plus ici.

        // Bandeau bas (clavier + pédale/molette) repliable : quand on le masque,
        // tout le bandeau disparaît ET la vue s'étend pour combler la place.
        const bool kb = barMode != 0, seq = barMode == 2;
        midiKeyboard.setVisible (kb);
        seqPanel.setVisible (seq);
        labelFoot.setVisible (kb && ! seq); comboFoot.setVisible (kb && ! seq);
        labelMod .setVisible (kb && ! seq); comboMod .setVisible (kb && ! seq);

        const int topH = kTopBarH;
        if (kb)
        {
            const int leftW = seq ? 290 : 140;   // Seq panel is wider than the pedal/wheel block
            tabs.setBounds (0, topH, getWidth(), getHeight() - topH - 74);
            midiKeyboard.setBounds (margin + leftW, getHeight() - 70, getWidth() - (2 * margin) - leftW, 70);
            seqPanel.setBounds (margin, getHeight() - 74, leftW - 8, 72);
            labelFoot.setBounds (margin, getHeight() - 77, 134, 20);
            comboFoot.setBounds (margin, getHeight() - 58, 134, 20);
            labelMod .setBounds (margin, getHeight() - 40, 134, 20);
            comboMod .setBounds (margin, getHeight() - 24, 134, 20);
        }
        else
        {
            // Clavier masqué : la vue prend toute la hauteur sous le bandeau.
            tabs.setBounds (0, topH, getWidth(), getHeight() - topH - 4);
        }

        // --- Bandeau superieur : [SYSEX 77]   [< nom >]   [In][Out][Barre] ---
        const int by = (topH - 26) / 2;
        const int toggleX = getWidth() - margin - 70;
        btBarMode.setBounds (toggleX, by, 70, 26);
        const int selW = 54, gap = 4;
        const int outX = toggleX - gap - selW;
        const int inX  = outX  - gap - selW;
        btMidiIn .setBounds (inX,  by, selW, 26);
        btMidiOut.setBounds (outX, by, selW, 26);

        // Pilule centree sur la fenetre, retrecie si la place manque (entre [logo New] et In).
        // « New... » colle a gauche de la pilule ; la pilule ne descend jamais sous logo+New.
        const int newW = 64, newGap = 8, logoEnd = 112;
        const int leftLimit = logoEnd + newW + newGap, rightLimit = inX - 12;
        const int pillH = 30, arrowW = 30;
        int pillW = jmin (320, rightLimit - leftLimit);
        int pillX = jlimit (leftLimit, jmax (leftLimit, rightLimit - pillW), getWidth() / 2 - pillW / 2);
        namePill = { pillX, (topH - pillH) / 2, jmax (2 * arrowW + 40, pillW), pillH };
        auto pr = namePill;
        btVoicePrev.setBounds (pr.removeFromLeft (arrowW).reduced (0, 1));
        btVoiceNext.setBounds (pr.removeFromRight (arrowW).reduced (0, 1));
        btNewVoice.setBounds (namePill.getX() - newGap - newW, by, newW, 26);
        if (nameEditor != nullptr)
            nameEditor->setBounds (pr.reduced (4, 2));
        for (Component* c : { (Component*) &btBarMode, (Component*) &btMidiIn, (Component*) &btMidiOut,
                              (Component*) &btVoicePrev, (Component*) &btVoiceNext, (Component*) &btNewVoice })
            c->toFront (false);
        if (nameEditor != nullptr) nameEditor->toFront (false);
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
            const ScopedLock sl (outLock);
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
            const ScopedLock sl (outLock);
            jassert (midiOutputs[index]->outDevice.get() != nullptr);
            // Release anything still sounding through this port before it disappears.
            if (auto* dev = midiOutputs[index]->outDevice.get())
                for (int ch = 1; ch <= 16; ++ch)
                    dev->sendMessageNow (MidiMessage::allNotesOff (ch));
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
            // Device ACTIVÉ = ligne sélectionnée : surbrillance à l'accent du thème (comme les
            // tables Bank/Voices/AWM), texte contrasté. Sinon couleur de texte normale du thème.
            auto textColour = rowIsSelected ? SYColSelected.contrasting()
                                            : getLookAndFeel().findColour (ListBox::textColourId);

            if (rowIsSelected)
                g.fillAll (SYColSelected);

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
                    // Keep the edit history's state true (never creates an undo step).
                    // Ignored when no voice is loaded (else revert/undo would resend these).
                    if (data[3] != 0x0F && ! showingA && editHistory.isLoaded())
                        editHistory.noteRx (EditHist::keyOf (data[3], data[4], data[5], data[6]),
                                            EditHist::valOf (data[7], data[8]));

                    // "Suivre le synthé" : ouvre la vue correspondant au groupe reçu, pour que
                    // le paramètre modifié sur le SY77 soit visible (sa valeur s'y met à jour).
                    if (followSynth)
                    {
                        const int g = data[3];
                        int target = -1;
                        if (g == 0x08)                                   target = 3; // Effets
                        else if (g == 0x02 && data[6] >= 0x28 && data[6] <= 0x3D) target = 4; // Controleurs
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
        // Edit history: record user voice param-changes (message thread only; not our own
        // undo/redo/B sends, not while A is shown, not system setup group 0x0F).
        if (SyVoice::isRecordableVoiceParamChange (msg.getRawData(), msg.getRawDataSize())
            && MessageManager::existsAndIsCurrentThread()
            && ! applyingHistory && ! showingA && loadSettling == 0 && editHistory.isLoaded())
        {
            // A delayed B still pending (SY77 absorbing the A bulk): ADVANCE it here, before
            // this edit goes out below -> the synth gets the B diff THEN the edit, UI and synth
            // stay aligned. Not a new write: the B send already requested (under applyingHistory).
            flushPendingB();
            const uint8* d = msg.getRawData();
            editHistory.recordEdit (EditHist::keyOf (d[4], d[5], d[6], d[7]), EditHist::valOf (d[8], d[9]),
                                    Time::getMillisecondCounterHiRes(),
                                    ModifierKeys::currentModifiers.isAnyMouseButtonDown());
            ++historyGen;   // any pending delayed B send is now stale
            publishHistoryState ({});
        }
        // Note-offs still pass when Out is OFF: otherwise a note started before the switch
        // (sequencer or keyboard) would hang on the synth with no way to release it.
        if (! midiSend && ! msg.isNoteOff()) return;
        // The sequencer thread, the bank-send thread and the message thread all send here:
        // serialise so messages never interleave on a port (and the port list is stable).
        const ScopedLock sl (outLock);
        // Retrig mode: restart the phrase after a VOICE PARAM CHANGE only (F0 43 1n 34 ...,
        // 11 bytes) — not after dump requests, system setup or bulk transfers.
        if (msg.isSysEx() && msg.getRawDataSize() == 11)
        {
            const uint8* d = msg.getRawData();
            if (d[1] == 0x43 && (d[2] & 0xF0) == 0x10 && d[3] == 0x34)
                seqPlayer.editHappened();
        }
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
            const ScopedLock sl (outLock);   // outputs list is read by the sequencer thread

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
    TextButton  btBarMode;
    int barMode = 1;   // 0 = none, 1 = keyboard, 2 = phrase player

    void refreshBarModeButton()
    {
        const char* names[3] = { "None", "Keys", "Seq" };
        btBarMode.setButtonText (TRANS (names[barMode]));
        btBarMode.setToggleState (barMode != 0, dontSendNotification);
    }

    void setBarMode (int m)
    {
        // Release held keyboard notes BEFORE switching: in Seq mode handleNoteOff swallows them.
        keyboardState.allNotesOff (0);
        barMode = jlimit (0, 2, m);
        refreshBarModeButton();
        if (barMode != 2) seqPanel.stopPlaying();
        getAppSettings()->setValue ("BottomBarMode", barMode);
        getAppSettings()->saveIfNeeded();
        resized();
    }
    // Sélecteurs de ports MIDI TOUJOURS VISIBLES (bandeau bas, à gauche du toggle clavier).
    // Ouvrent un PopupMenu à cases cochables ; même source de vérité que l'onglet Midi Setting
    // (midiInputs/midiOutputs + openDevice/closeDevice + saveEnabledDevices). Cf. showMidiPortMenu.
    TextButton  btMidiIn  {"In"};
    TextButton  btMidiOut {"Out"};
    std::unique_ptr<MidiDeviceListBox> midiInputSelector, midiOutputSelector;
    std::unique_ptr<MidiSettingsPage>  midiSettingsPage;
    ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;
    CriticalSection outLock;   // guards midiOutputs + sendMessageNow (see sendToOutputs)

    //==========================================================================
    // Edit history (EditHistory.h): undo/redo, A/B compare, revert. Message thread only.
    EditHist::History editHistory;
    MemoryBlock originalVoiceBlock;   // edit-buffer copy of the loaded voice (A side)
    bool applyingHistory = false;     // our own sends: not recorded
    bool showingA = false;
    int loadSettling = 0;             // voice loads whose async widget echoes are still pending
    std::vector<EditHist::KV> pendingB;   // delayed B diff not yet sent...
    int pendingBGen = -1;                 // ...valid only while historyGen == pendingBGen
    double bulkBusyUntilMs = 0;       // SY77 still absorbing the last voice bulk (A / Original)
    int historyGen = 0;               // bumped by every edit/undo/redo/load/revert/A: cancels a delayed B send

    /** A load sets props (VOICEMODE...) whose widgets react ASYNCHRONOUSLY (Value update, then the
        ComboBox notification: two message-loop hops) and would re-send them to the SY77 and
        record them as edits. Hold the send guard and stop recording until both hops ran:
        callAsync messages are FIFO, so the nested one lands after those notifications. */
    void settleAfterLoad()
    {
        SysexBus::get().suppressSend.fetch_add (1, std::memory_order_acq_rel);
        ++loadSettling;   // counter: with two queued loads the 1st settle must not release the 2nd
        Component::SafePointer<MidiDemo> safe (this);
        MessageManager::callAsync ([safe]
        {
            MessageManager::callAsync ([safe]
            {
                SysexBus::get().suppressSend.fetch_sub (1, std::memory_order_acq_rel);
                if (safe != nullptr && safe->loadSettling > 0) --safe->loadSettling;
            });
        });
    }

    /** Sends the pending delayed B if still valid (same generation, not showing A), then
        drops it. Called by the timer or, early, by the first recorded edit. */
    void flushPendingB()
    {
        if (pendingBGen < 0) return;
        const bool valid = pendingBGen == historyGen && ! showingA;
        auto diff = std::move (pendingB);
        pendingB.clear();
        pendingBGen = -1;
        if (valid) sendParams (diff);
    }

    /** Sends the original voice to the edit buffer, stamped with the CURRENT device number. */
    void sendOriginalBulk()
    {
        SyVoice::setBulkDevice ((uint8*) originalVoiceBlock.getData(), (int) originalVoiceBlock.getSize(),
                                sysexDeviceNumber);
        sendRaw (originalVoiceBlock.getData(), (long) originalVoiceBlock.getSize());
        noteBulkSent (originalVoiceBlock);
    }

    /** Time the SY77 needs for a voice bulk: wire time (~size/3 ms) + processing margin. */
    void noteBulkSent (const MemoryBlock& b)
    {
        if (! midiSend) return;   // nothing went out
        bulkBusyUntilMs = Time::getMillisecondCounterHiRes() + (double) b.getSize() / 3.0 + 60.0;
    }

    static std::vector<EditHist::KV> toKV (const Array<SyVoice::VoiceParam>& ps)
    {
        std::vector<EditHist::KV> out;
        for (auto& vp : ps)
            out.push_back ({ EditHist::keyOf (vp.group, vp.addrHi, vp.addrLo, vp.param),
                             EditHist::valOf (vp.v1, vp.value) });
        return out;
    }

    /** Called by every voice-load path (library audition, templates). */
    void onVoiceLoaded (const MemoryBlock& editBufferBlock)
    {
        if (showingA) { showingA = false; lockEditing (false); }
        originalVoiceBlock = editBufferBlock;
        ++historyGen;
        noteBulkSent (editBufferBlock);
        editHistory.load (toKV (SyVoice::voiceBlobToParams ((const uint8*) editBufferBlock.getData(),
                                                            (int) editBufferBlock.getSize())));
        publishHistoryState ({});
    }

    /** A block too short to decode (sz <= 42) replaced the edit buffer: the previous
        voice's history/original no longer match the synth, so A, undo and revert must not
        resend it. Presentation/state only: nothing is sent here. */
    void discardLoadedVoice()
    {
        if (showingA) { showingA = false; lockEditing (false); }
        originalVoiceBlock.reset();
        ++historyGen;          // any pending delayed B send is now stale
        editHistory.unload();
        publishHistoryState ({});
    }

    void replayToUi (const std::vector<EditHist::KV>& kvs)
    {
        const ScopedEchoSuppress noEcho;   // UI follows, nothing is re-emitted by the widgets
        for (auto& kv : kvs)
        {
            valueSysexIn = make_var_array (EditHist::gOf (kv.first), EditHist::hiOf (kv.first),
                                           EditHist::loOf (kv.first), EditHist::pOf (kv.first),
                                           EditHist::v1Of (kv.second), EditHist::v2Of (kv.second));
            valueSysexIn.getValueSource().sendChangeMessage (true);
        }
    }

    void sendParams (const std::vector<EditHist::KV>& kvs)
    {
        const ScopedValueSetter<bool> svs (applyingHistory, true);
        for (auto& kv : kvs)
        {
            const auto d = SyVoice::paramChangeBytes (sysexDeviceNumber,
                                 EditHist::gOf (kv.first), EditHist::hiOf (kv.first),
                                 EditHist::loOf (kv.first), EditHist::pOf (kv.first),
                                 EditHist::v1Of (kv.second), EditHist::v2Of (kv.second));
            auto m = MidiMessage::createSysExMessage (d.data(), 9);
            m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
    }

    void applyKV (const std::vector<EditHist::KV>& kvs) { replayToUi (kvs); sendParams (kvs); }

    static String unknownStatus (int n)
    {
        return n > 0 ? String (n) + " " + TRANS ("value(s) not restorable (not read from the dump)") : String();
    }

    void historyUndo()
    {
        if (showingA) setShowingA (false);
        ++historyGen;
        int unknown = 0;
        applyKV (editHistory.undo (unknown));
        publishHistoryState (unknownStatus (unknown));
    }
    void historyRedo()
    {
        if (showingA) setShowingA (false);
        ++historyGen;
        int unknown = 0;
        applyKV (editHistory.redo (unknown));
        publishHistoryState (unknownStatus (unknown));
    }

    void setShowingA (bool a)
    {
        if (a == showingA || ! editHistory.isLoaded() || originalVoiceBlock.getSize() < 2) return;
        if (a)
        {
            // A: the voice as loaded, sent whole to the edit buffer (covers every param).
            ++historyGen;
            sendOriginalBulk();
            replayToUi (editHistory.originalValues());
            seqPlayer.editHappened();   // a bulk is not a param-change: restart explicitly
        }
        else
        {
            // B: only what was edited. UI now; the sends wait until the SY77 has absorbed
            // the A bulk (fast A/B toggling would otherwise lose param-changes).
            auto diff = editHistory.diffFromOriginal();
            replayToUi (diff);
            const double wait = bulkBusyUntilMs - Time::getMillisecondCounterHiRes();
            if (wait <= 0) sendParams (diff);
            else
            {
                Component::SafePointer<MidiDemo> safe (this);
                const int gen = historyGen;   // cancelled by any later undo/load/revert/A;
                pendingB = diff;              // advanced (flushPendingB) by a later edit
                pendingBGen = gen;
                Timer::callAfterDelay ((int) wait + 1, [safe, gen]
                {
                    if (safe != nullptr && safe->historyGen == gen) safe->flushPendingB();
                });
            }
        }
        showingA = a;
        lockEditing (a);
        publishHistoryState (a ? TRANS ("Listening to the original - editing locked") : String());
    }

    void askRevert()
    {
        if (! editHistory.isLoaded() || originalVoiceBlock.getSize() < 2) return;
        NativeMessageBox::showOkCancelBox (MessageBoxIconType::QuestionIcon, TRANS ("Original"),
            TRANS ("Go back to the voice as loaded? Your edits stay in the history (Cmd+Z restores them)."),
            this, ModalCallbackFunction::create ([this] (int r) { if (r == 1) revertToOriginal(); }));
    }

    void askNewVoice (int i)
    {
        auto go = [this, i] { auto b = VoiceTemplates::make (i); if (b.getSize() > 1) openVoiceBlock (b); };
        if (! editHistory.isLoaded() || editHistory.diffFromOriginal().empty()) { go(); return; }   // nothing to lose
        NativeMessageBox::showOkCancelBox (MessageBoxIconType::QuestionIcon, TRANS ("New voice"),
            TRANS ("Replace the voice being edited? Unsaved edits will be lost."),
            this, ModalCallbackFunction::create ([go] (int r) { if (r == 1) go(); }));
    }

    void revertToOriginal()
    {
        // Nothing loaded: no undoable step from an empty original (askRevert's condition).
        if (! editHistory.isLoaded() || originalVoiceBlock.getSize() < 2) return;
        if (showingA) { showingA = false; lockEditing (false); }
        ++historyGen;
        sendOriginalBulk();
        replayToUi (editHistory.originalValues());
        editHistory.makeRevertStep (Time::getMillisecondCounterHiRes());
        seqPlayer.editHappened();
        publishHistoryState ({});
    }

    void lockEditing (bool locked)
    {
        for (int i = 0; i < tabs.getNumTabs(); ++i)
        {
            auto* c = tabs.getTabContentComponent (i);
            if (auto* vp = dynamic_cast<VoicePage*> (c))           vp->setEditLocked (locked);
            else if (dynamic_cast<EffectsPage*> (c) != nullptr
                     || dynamic_cast<ControllersPage*> (c) != nullptr) c->setEnabled (! locked);
        }
    }

    void publishHistoryState (const String& status)
    {
        auto& h = EditHist::Hooks::get();
        h.canUndo  = editHistory.canUndo();
        h.canRedo  = editHistory.canRedo();
        h.loaded   = editHistory.isLoaded() && originalVoiceBlock.getSize() > 1;
        h.showingA = showingA;
        h.status   = status;
        h.sendChangeMessage();
    }
    Seq::Player seqPlayer { [this] (const MidiMessage& m) { sendToOutputs (m); },
                            [] { return currentPlayChannel(); } };
    std::array<int, 128> keyChannel {};   // channel each on-screen key was sent on
    Seq::Panel  seqPanel  { seqPlayer };
    
    CriticalSection midiMonitorLock;
    Array<MidiMessage> incomingMessages;
    // Historique borné des messages affichés -> permet de RE-RENDRE le moniteur quand on
    // bascule Raw/Interprété (s'applique donc aussi aux lignes déjà présentes). Borne mémoire.
    Array<MidiMessage> monitorHistory;
    static constexpr int kMonitorHistoryMax = 2000;
    DemoTabbedComponent tabs;

    // Bandeau superieur (cf. paint/resized) : < nom de voix > + In/Out/barre du bas a droite.
    static constexpr int kTopBarH = 42;
    VoiceNavArrow btVoicePrev { true }, btVoiceNext { false };
    TextButton    btNewVoice { TRANS ("New...") };   // menu des modeles (Templates.h)
    void showNewVoiceMenu()
    {
        PopupMenu m;
        int id = 1;
        for (auto& t : VoiceTemplates::list()) m.addItem (id++, TRANS (t.name));
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&btNewVoice),
            [safe = Component::SafePointer<MidiDemo> (this)] (int r)
            {
                if (r <= 0 || safe == nullptr) return;
                safe->tabs.setCurrentTabIndex (2);   // le resultat s'affiche dans l'editeur Voice
                safe->askNewVoice (r - 1);
            });
    }
    TextEditor*   nameEditor = nullptr;    // TextEditor de la VoicePage, re-parente ici
    Rectangle<int> namePill;               // fond de la pilule (dessine dans paint)
    struct TipRefresher : ChangeListener
    {
        std::function<void()> f;
        void changeListenerCallback (ChangeBroadcaster*) override { if (f) f(); }
    } tipRefresher;
    void refreshVoiceNavTooltips()
    {
        btVoicePrev.setTooltip (TRANS ("Previous voice (audition)") + Shortcuts::hint ("voiceprev"));
        btVoiceNext.setTooltip (TRANS ("Next voice (audition)")     + Shortcuts::hint ("voicenext"));
        btNewVoice .setTooltip (TRANS ("New voice from a template (sent to the edit buffer)") + Shortcuts::hint ("newvoice"));
    }
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
