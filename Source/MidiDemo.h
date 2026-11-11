#pragma once
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

// Sysex77 : composant racine (onglets, ports MIDI, bus sysex interne). Derive de l'exemple
// JUCE MidiDemo (licence ISC ci-dessus).
#include "Values.h"
#include "SysexBus.h"
#include "SysexUtils.h"   // SyVoice:: (builder sysex, device, helpers) — visible par tous les widgets
#include "LibraryIndex.h" // index librairie (métadonnées/tags/recherche, library.json)
#include "Updater.h"      // mise à jour in-app depuis GitHub Releases
#include "Version.h"      // Sysex77::kVersion / versionString()
#include "AppSettings.h"  // getAppSettings() — persistance partagée (devices MIDI, fenêtre…)

#include "MidiGlobals.h"   // etat partage (device, gates In/Out, mode dev, adresses du bus)

#include "LookAndFeel.h"
#include "Controller.h"
#include "Config.h"
#include "Sequencer.h"
#include "EditHistory.h"
#include "VoiceSession.h"      // session d'edition (undo/redo, A/B, revert, chargement edit buffer)
#include "Templates.h"           // "New voice" templates (INIT voices)
#include "BankSendProgress.h"      // fenêtre de progression de l'envoi de banque
#include "BankReceiveProgress.h"   // fenêtre de progression de la réception de banque
#include "Voice.h"
#include "Librairie.h"
#include "Effects.h"
#include "ControllersPage.h"
#include "MultiPage.h"
#include "DumpDiffView.h"   // outil RE : comparer 2 dumps .syx
#include "TestBench.h"      // banc de test hardware (mode developpeur)
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
        addTab (TRANS("Multi"), colour, new MultiPage(), true);   // index 5 (followSynth groupes 0x00/0x01)
        // L'onglet "Midi Setting" (index 6, mode développeur) est ajouté par MidiDemo avec un vrai
        // contenu (MidiSettingsPage), car il réutilise des composants de MidiDemo.

        // ConfigPage (1er onglet) a appliqué le thème pendant sa construction :
        // on aligne donc le fond des onglets sur la couleur de fond du thème.
        for (int i = 0; i < getNumTabs(); ++i)
            setTabBackgroundColour (i, SYColBackground);
    }
    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName)override
    {

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

#include "MidiMonitor.h"        // rendu texte du moniteur MIDI
#include "MidiSettingsPage.h"   // onglet Midi Setting (mode developpeur)

//==============================================================================
class MidiDemo  : public Component,
private Timer,
private MidiKeyboardStateListener,
private MidiInputCallback,
private AsyncUpdater,
private TextButton::Listener,
 public ComboBox::Listener
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
            if (session.hasUnsavedEdits())
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
        if (id == "ab")       { session.setShowingA (! session.isShowingA()); return true; }
        if (id == "presethex")
        {
            for (int i = 0; i < tabs.getNumTabs(); ++i)
                if (auto* l = dynamic_cast<LibrairiePage*> (tabs.getTabContentComponent (i)))
                    return l->isShowing() && l->showSelectedPresetHex();   // seulement si la Librairie est affichee
            return false;
        }
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
        if (id == "undo") { session.undo(); return true; }
        if (id == "redo") { session.redo(); return true; }
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
            h.undo     = [this] { session.undo(); };
            h.redo     = [this] { session.redo(); };
            h.toggleAB = [this] { session.setShowingA (! session.isShowingA()); };
            h.revert   = [this] { askRevert(); };
        }

        // (Images de fond Sysex77/99.png supprimées : jamais dessinées — nettoyage des ressources.)
        addAndMakeVisible (tabs);
  
        addLabelAndSetStyle (midiInputLabel);
        addLabelAndSetStyle (midiOutputLabel);
        addLabelAndSetStyle (incomingMidiLabel);
        
        addAndMakeVisible(btBulk);
        btBulk.setClickingTogglesState(true);
        // Etat memorise (defaut ON = etat du SY77 a l'allumage). Choix OFF de l'utilisateur
        // -> renvoye a la reouverture de la sortie MIDI (cf. restoreEnabledDevices).
        btBulk.setToggleState (getAppSettings()->getBoolValue ("BulkProtect", true), dontSendNotification);
        btBulk.setTooltip (TRANS ("Bulk data memory protect of the SY77. When off, it is sent again at startup."));
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
        btMidiIn .setTooltip (TRANS ("Incoming MIDI: ON = processed, OFF = ignored (diagnostics)"));
        btMidiOut.setTooltip (TRANS ("Outgoing MIDI: ON = sent, OFF = nothing goes out (Thru included)"));
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
        // Bouton Play de la vue drum : joue la note via le clavier a l'ecran (canal de jeu, touche allumee).
        drumAuditionHook() = [this] (int note, bool on)
        {
            if (on) keyboardState.noteOn (1, note, 0.8f);
            else    keyboardState.noteOff (1, note, 0.0f);
        };
        
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


        // RE : exporte la valeur ACTUELLE de chaque contrôle câblé (lecture seule, aucun envoi).
        // L'éditeur reflétant la voix dumpée, ce CSV = texte clair complet à corréler au dump.
        midiSettingsPage->onReadValues = [this]
        {
            juce::File csv = appDirPath.getChildFile ("re_values.csv");
            csv.deleteFile();
            juce::FileOutputStream os (csv);
            os << "group,addrHi,addrLo,param,value,type\n";
            int counter = 0;
            forEachWiredWidget ([&] (const int* a, int val, const char* type)
            {
                os << a[3] << "," << a[4] << "," << a[5] << "," << a[6] << "," << val << "," << type << "\n";
                ++counter;
            });
            os.flush();
            midiMonitor.moveCaretToEnd();
            midiMonitor.insertTextAtCaret ("\n[RE] valeurs: " + juce::String (counter) + " controles -> " + csv.getFullPathName() + "\n");
        };

        // Moniteur : bascule Raw/Interprété -> re-rend tout l'historique borné dans le nouveau mode.
        midiSettingsPage->onRawToggled   = [this] { rebuildMonitorFromHistory(); };
        // Bouton Clear : vide aussi l'historique borné (sinon un re-render le ferait réapparaître).
        midiSettingsPage->onClearHistory = [this] { monitorHistory.clearQuick(); };

        // Banc de test : envois HORS historique d'edition (benchSending), memes gates In/Out
        // que le test de connexion.
        midiSettingsPage->bench.ports.send = [this] (const MidiMessage& m)
        {
            MidiMessage msg (m);
            msg.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            benchSending = true;
            sendToOutputs (msg);
            benchSending = false;
        };
        midiSettingsPage->bench.ports.device    = [] { return sysexDeviceNumber; };
        // Rapport ecrit aussi sur disque (lu directement pour l'analyse, sans copier-coller).
        midiSettingsPage->bench.ports.logFile   = getAppSettings()->getFile().getSiblingFile ("testbench.log");
        midiSettingsPage->bench.ports.libraryDir = appDirPath;
        // Croisement widgets/decodeur : meme parcours que onReadValues, AUCUN envoi.
        midiSettingsPage->bench.ports.widgetAddresses = [this]
        {
            std::vector<std::array<int, 4>> out;
            forEachWiredWidget ([&] (const int* a, int, const char*) { out.push_back ({ a[3], a[4], a[5], a[6] }); });
            return out;
        };
        midiSettingsPage->bench.ports.preflight = [this]() -> String
        {
            if (! midiSend)   return "sortie MIDI coupee (activer Out)";
            if (! midiListen) return "entree MIDI coupee (activer In)";
            return {};
        };

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
        drumAuditionHook() = nullptr;
        keyboardState.removeListener (this);
        btBulk.removeListener(this);
      
      
        midiInputSelector .reset();
        midiOutputSelector.reset();
        comboFoot.removeListener(this);
        comboMod.removeListener(this);
        
        
    }
    

    void buttonClicked (Button* button) override
    {
        if(button == &btBulk)
        {
            getAppSettings()->setValue ("BulkProtect", btBulk.getToggleState());
            getAppSettings()->saveIfNeeded();
            sendBulkProtect();
        }
    }

    /** Bulk Protect ON/OFF : param-change standard 0x34, group 0x0F (System Setup), param 0x34
        (BLUKMPRT off/on, OCR l.820). Construit par le builder unique. */
    void sendBulkProtect()
    {
        Logger::writeToLog ("Bulk Protect " + String (btBulk.getToggleState() ? "ON" : "OFF"));
        auto sysexdata = SyVoice::paramBytes (sysexDeviceNumber, 0x0f, 0x00, 0x00, 0x34,
                                              (uint8) btBulk.getToggleState());
        MidiMessage m = MidiMessage::createSysExMessage (sysexdata.data(), 9);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }
    /** Parcourt (LECTURE SEULE, aucun envoi) chaque widget Midi* cable de TOUS les onglets,
        sous-onglets imbriques compris (un TabbedComponent ne garde dans l'arbre que l'onglet
        actif -> descente explicite via getTabContentComponent). fn (addr[9], valeur, type). */
    template <typename Fn>
    void forEachWiredWidget (Fn&& fn)
    {
        std::function<void (juce::Component*)> walk = [&] (juce::Component* c)
        {
            if (auto* tc = dynamic_cast<juce::TabbedComponent*> (c))
            {
                for (int t = 0; t < tc->getNumTabs(); ++t)
                    if (auto* cc = tc->getTabContentComponent (t)) walk (cc);
                return;
            }
            for (auto* k : c->getChildren())
            {
                int a[9]; int val = 0; bool ok = false; const char* type = "";
                if      (auto* sl = dynamic_cast<MidiSlider*>(k)) { ok = sl->reRead (a, val); type = "slider"; }
                else if (auto* b  = dynamic_cast<MidiButton*>(k)) { ok = b->reRead  (a, val); type = "button"; }
                else if (auto* cb = dynamic_cast<MidiCombo*> (k)) { ok = cb->reRead (a, val); type = "combo";  }
                else if (auto* r  = dynamic_cast<MidiRadio*> (k)) { ok = r->reRead  (a, val); type = "radio";  }
                if (ok) fn ((const int*) a, val, type);
                walk (k);
            }
        };
        for (int i = 0; i < tabs.getNumTabs(); ++i)
            if (auto* content = tabs.getTabContentComponent (i)) walk (content);
    }

    // Mode développeur : ajoute/retire l'onglet "Midi Setting" à chaud.
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
            midiSettingsPage->bench.abort ("mode developpeur desactive");
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

    // Include TEXTUEL dans la classe : routine bus interne (OSC historique) -> envoi sysex MIDI.
#include "MidiSysex.h"
    
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
                }
                else
                {
                    Logger::writeToLog("other message");
                }
                
            }
            triggerAsyncUpdate();
        }
        
    }
    template<typename... Ts>
    inline var make_var_array(Ts... values)
    {
        return Array<var>(values...);
    }
    // Une ligne de moniteur (logique pure dans MidiMonitor.h), avec l'etat courant.
    bool formatMonitorLine (const MidiMessage& message, String& out, String& dedup)
    {
        return SyMonitor::formatLine (message, monitorRaw, sysexDeviceNumber, sysexReceiveOmni, out, dedup);
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
            // Bulk Multi recu (reponse a une demande de l'onglet Multi, ou envoi depuis le synthe).
            // Banc de test : voit TOUT sysex entrant, avant le filtre device.
            if (midiSettingsPage != nullptr)
                midiSettingsPage->bench.handleIncoming (message);

            // (pendant une sonde du banc, les Multi recus sont pour le banc : ne pas ecraser l'onglet Multi)
            if (message.isSysEx() && SyMulti::isMultiBlock (message.getRawData(), message.getRawDataSize())
                && ! (midiSettingsPage != nullptr && midiSettingsPage->bench.isBusy()))
            {
                valueMultiBulk = var (message.getRawData(), (size_t) message.getRawDataSize());
                valueMultiBulk.getValueSource().sendChangeMessage (false);
            }

            if (awaitingConnReply && message.isSysEx())
            {
                const auto reply = SyVoice::classifyConnReply (message.getSysExData(), message.getSysExDataSize());
                if (reply != SyVoice::ConnReply::none)
                {
                    awaitingConnReply = false;
                    const bool isYam = reply != SyVoice::ConnReply::identity;
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
                    applyParamToUi (data[3], data[4], data[5], data[6], data[7], data[8]);
                    // Keep the edit history's state true (never creates an undo step).
                    // Ignored when no voice is loaded (else revert/undo would resend these).
                    session.observeIncomingParam (&data[3]);

                    // "Suivre le synthé" : ouvre la vue correspondant au groupe reçu, pour que
                    // le paramètre modifié sur le SY77 soit visible (sa valeur s'y met à jour).
                    // (suspendu pendant une sonde du banc : ne pas arracher l'utilisateur a l'onglet)
                    if (followSynth && ! (midiSettingsPage != nullptr && midiSettingsPage->bench.isBusy()))
                    {
                        const int target = SyVoice::followSynthTarget (data[3], data[6]);
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
        // undo/redo/B sends, not while A is shown, not system setup group 0x0F). AVANT le gate
        // Out (fidele) : un B differe en attente est avance ici, avant que l'edit ne sorte.
        if (MessageManager::existsAndIsCurrentThread() && ! benchSending.load())
            session.observeOutgoing (msg.getRawData(), msg.getRawDataSize(),
                                     ModifierKeys::currentModifiers.isAnyMouseButtonDown());
        // Note-offs still pass when Out is OFF: otherwise a note started before the switch
        // (sequencer or keyboard) would hang on the synth with no way to release it.
        if (! midiSend && ! msg.isNoteOff()) return;
        // The sequencer thread, the bank-send thread and the message thread all send here:
        // serialise so messages never interleave on a port (and the port list is stable).
        const ScopedLock sl (outLock);
        // Retrig mode: restart the phrase after a VOICE PARAM CHANGE only (F0 43 1n 34 ...,
        // 11 bytes) — not after dump requests, system setup or bulk transfers.
        // (pas pour les envois du banc de test : un LOT en enverrait des centaines)
        if (msg.isSysEx() && msg.getRawDataSize() == 11 && ! benchSending.load())
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
        bool reopened = false;
        for (int i = 0; i < devices.size(); ++i)
        {
            const bool isOpen = isInput ? devices[i]->inDevice.get() != nullptr
                                        : devices[i]->outDevice.get() != nullptr;
            if (! isOpen && saved.contains (devices[i]->deviceInfo.identifier))
            {
                openDevice (isInput, i);
                reopened = true;
            }
        }
        // Sortie memorisee rouverte (demarrage, interface rebranchee) et Bulk Protect leve
        // par l'utilisateur : on le releve dans le synthe (choix explicite, persiste). Leger
        // delai : laisser le port s'ouvrir. Rien n'est envoye si le choix est ON (defaut).
        if (! isInput && reopened && ! btBulk.getToggleState())
        {
            SafePointer<MidiDemo> self (this);
            Timer::callAfterDelay (400, [self] { if (self && ! self->btBulk.getToggleState()) self->sendBulkProtect(); });
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
    // Interrupteurs In/Out de la barre du haut : gates ecoute/envoi (midiListen/midiSend), libelle =
    // nombre de ports ouverts. Le CHOIX des ports se fait dans l'onglet Setting (MidiDeviceListBox).
    TextButton  btMidiIn  {"In"};
    TextButton  btMidiOut {"Out"};
    std::unique_ptr<MidiDeviceListBox> midiInputSelector, midiOutputSelector;
    std::unique_ptr<MidiSettingsPage>  midiSettingsPage;
    ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;
    CriticalSection outLock;   // guards midiOutputs + sendMessageNow (see sendToOutputs)

    void askRevert()
    {
        if (! session.canRevert()) return;
        NativeMessageBox::showOkCancelBox (MessageBoxIconType::QuestionIcon, TRANS ("Original"),
            TRANS ("Go back to the voice as loaded? Your edits stay in the history (Cmd+Z restores them)."),
            this, ModalCallbackFunction::create ([this] (int r) { if (r == 1) session.revert(); }));
    }

    void askNewVoice (int i)
    {
        auto go = [this, i] { auto b = VoiceTemplates::make (i); if (b.getSize() > 1) openVoiceBlock (b); };
        if (! session.hasUnsavedEdits()) { go(); return; }   // nothing to lose
        NativeMessageBox::showOkCancelBox (MessageBoxIconType::QuestionIcon, TRANS ("New voice"),
            TRANS ("Replace the voice being edited? Unsaved edits will be lost."),
            this, ModalCallbackFunction::create ([go] (int r) { if (r == 1) go(); }));
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

    Seq::Player seqPlayer { [this] (const MidiMessage& m) { sendToOutputs (m); },
                            [] { return currentPlayChannel(); } };
    std::array<int, 128> keyChannel {};   // channel each on-screen key was sent on
    Seq::Panel  seqPanel  { seqPlayer };

    //==========================================================================
    // Session d'edition de la voix ouverte (VoiceSession.h) : undo/redo, A/B, revert, flux
    // de chargement edit buffer. Thread message uniquement. Declaree APRES seqPlayer, outLock
    // et midiOutputs (detruite avant eux). Seuls send/replayParam/lockEditing/editHappened
    // capturent this ; post/schedule n'en capturent rien (le settle peut survivre a MidiDemo).
    /** Applique un param-change (g hi lo p v1 v2) aux widgets : valueSysexIn + notification
        SYNCHRONE (true), pour que les widgets s'appliquent DANS la portee du ScopedEchoSuppress
        de l'appelant (param recu dans handleAsyncUpdate, rejeu de VoiceSession). */
    void applyParamToUi (int g, int hi, int lo, int p, int v1, int v2)
    {
        // Multi (0x00/0x01) : canal dedie -> jamais vu par les widgets de voix (un widget non
        // cable, sysexData a zero, aurait capte MNAM0 = 00 00 00 00).
        auto& v = SyVoice::isMultiGroup (g) ? valueMultiIn : valueSysexIn;
        v = make_var_array (g, hi, lo, p, v1, v2);
        v.getValueSource().sendChangeMessage (true);
    }

    VoiceSession::Ports makeSessionPorts()
    {
        VoiceSession::Ports p;
        // sendToOutputs (pas sendRaw : sendRawSysex logge deja la taille). Passe par
        // observeOutgoing puis le gate Out, exactement comme les envois d'avant.
        p.send         = [this] (const MidiMessage& m) { sendToOutputs (m); };
        p.replayParam  = [this] (int g, int hi, int lo, int pp, int v1, int v2)
        {
            applyParamToUi (g, hi, lo, pp, v1, v2);   // appele sous ScopedEchoSuppress par VoiceSession
        };
        p.lockEditing  = [this] (bool locked) { lockEditing (locked); };
        p.editHappened = [this] { seqPlayer.editHappened(); };
        p.nowMs        = [] { return Time::getMillisecondCounterHiRes(); };
        p.post         = [] (std::function<void()> f) { MessageManager::callAsync (std::move (f)); };
        p.schedule     = [] (int ms, std::function<void()> f) { Timer::callAfterDelay (ms, std::move (f)); };
        p.sendEnabled  = [] { return midiSend; };
        p.device       = [] { return sysexDeviceNumber; };
        p.historyState = [] (const VoiceSession::HistoryState& s)
        {
            auto& h = EditHist::Hooks::get();
            h.canUndo  = s.canUndo;
            h.canRedo  = s.canRedo;
            h.loaded   = s.loaded;
            h.showingA = s.showingA;
            h.status   = s.status;
            h.sendChangeMessage();
        };
        return p;
    }
    VoiceSession session { makeSessionPorts() };
    
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
    std::atomic<bool> benchSending { false };   // lu aussi par les threads sequenceur/banque   // envoi du banc de test : hors historique voix
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
