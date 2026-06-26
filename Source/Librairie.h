/*
 ==============================================================================
 
 Librairie.h
 Created: 13 Nov 2018 11:23:35pm
 Author:  Sébastien Portrait
 
 Notes:
 BANK limité à 64 PATCH pour pouvoir transferer la banque
 
 Objectifs:
 Se rapprocher de la Librairie a la "massive"
 [VOICES/MULTI/SEQ] [DUMP]
 [BANK] [PATCHES]
 
 *Lecture/Sauvegarde des fichiers SYX
 *Lecture/Sauvegarde des fichier MID
 
 Eventualitées
 *Ajout de TAG
 
 ==============================================================================
 */

#pragma once
#include "BankTableModel.h"
#include "VoicesTableModel.h"


struct LibrairiePage   : public Component,public Button::Listener, private Timer,public ChangeListener, public ChangeBroadcaster, public KeyListener
{
    LibrairiePage()
    {
        setOpaque(false);
        // Easter-egg : Cmd+Alt+X (quand la page Librairie est visible) -> dialogue
        // (code « sysex ») -> import des banques web embarquées. Le KeyListener est
        // attaché au composant de plus haut niveau (cf. parentHierarchyChanged) :
        // c'est le seul point qui reçoit les touches quel que soit le composant focus.
        setWantsKeyboardFocus(true);
        addBtAndMakeStyle(btVoice);
        addBtAndMakeStyle(btMulti);
        btMulti.setEnabled(false);
        addBtAndMakeStyle(btSeq);
        btSeq.setEnabled(false);
        addAndMakeVisible(labelInfoLine);
        
        //    labelInfoLine.setJustificationType(Justification::centred);
        addAndMakeVisible(btSend);
        btSend.addListener(this);
        addAndMakeVisible(btReceive);
        addAndMakeVisible(btStop);
        btStop.setVisible(false);
        btStop.setColour(TextButton::ColourIds::buttonColourId, Colours::red);
        btStop.addListener(this);
        //    btReceive.setEnabled(false);
        btReceive.addListener(this);
        addAndMakeVisible(bankList);
        bankList.addChangeListener(this);

        // Champ de recherche : filtre la liste des banques (devenue longue). Filtre par
        // sous-chaîne insensible à la casse, appliqué à la vue de bankList.
        addAndMakeVisible (searchBox);
        searchBox.setTextToShowWhenEmpty (TRANS("Rechercher une banque..."), Colours::grey);
        searchBox.onTextChange = [this] { bankList.setFilter (searchBox.getText()); };
        
        addAndMakeVisible(voicesListA);
        addAndMakeVisible(voicesListB);
        addAndMakeVisible(voicesListC);
        addAndMakeVisible(voicesListD);

        // Sélection unique parmi les 64 voix : sélectionner dans une colonne
        // désélectionne les trois autres.
        auto selectOnlyThis = [this] (BankVoicesTable* selected)
        {
            for (auto* col : { &voicesListA, &voicesListB, &voicesListC, &voicesListD })
                if (col != selected)
                    col->deselectAllRows();

            // Info à la sélection : nom + type de la voix (octet @32 du bloc) + indication
            // si elle est chargeable dans l'éditeur (loader v1 = 1 AFM POLY, type 3).
            const int idx = selected->getSelectedGlobalIndex();
            auto block = SyVoice::getVoiceBlock ((const uint8*) currentBankData.getData(),
                                                 currentBankData.getSize(), idx);
            if (block.getSize() > 33)
            {
                const int type = ((const uint8*) block.getData())[32];
                const String name = arrayListVoices[idx].trim();
                // AFM 1/2/4 (0-4) + AWM 1/2/4 (5-7) + mixtes AFM+AWM (8-9). Drum (10) non géré.
                const bool loadable = (type >= 0 && type <= 9);
                labelInfoLine.setText (name + "  —  " + SyVoice::voiceTypeLabel (type)
                                       + (loadable ? "   · chargeable dans l'editeur" : ""),
                                       dontSendNotification);
            }
        };
        voicesListA.onRowSelected = selectOnlyThis;
        voicesListB.onRowSelected = selectOnlyThis;
        voicesListC.onRowSelected = selectOnlyThis;
        voicesListD.onRowSelected = selectOnlyThis;
        
        btVoice.setToggleState(true, NotificationType::dontSendNotification);
        
        
    }
    ~LibrairiePage()
    {
        btSend.removeListener(this);
        btStop.removeListener(this);
        btReceive.removeListener(this);
        bankList.removeChangeListener(this);
        if (keyHost != nullptr)
            keyHost->removeKeyListener (this);
        stopTimer();

    }

    // On attache le KeyListener au composant de PLUS HAUT NIVEAU (et non à la page) :
    // c'est le seul point qui reçoit toutes les touches non consommées, quel que soit le
    // composant qui a le focus -> Cmd+Alt+X marche sans devoir cliquer la page d'abord.
    // Appelé à chaque (dé)rattachement de la hiérarchie ; idempotent.
    void parentHierarchyChanged() override
    {
        auto* top = getTopLevelComponent();
        if (top == keyHost.getComponent())
            return;
        if (keyHost != nullptr)
            keyHost->removeKeyListener (this);
        keyHost = top;
        if (keyHost != nullptr)
            keyHost->addKeyListener (this);
    }

    //==============================================================================
    // Easter-egg caché : Cmd+Alt+X ouvre un dialogue ; saisir « sysex » (insensible à
    // la casse) lance l'import des banques web embarquées dans la librairie locale.
    // N'écrit QUE des fichiers locaux — jamais vers le synthé (sûreté matérielle).
    bool keyPressed (const KeyPress& key, Component* /*origin*/) override
    {
        // Listener global (top-level) : ne réagir que lorsque l'onglet Librairie est affiché.
        if (! isShowing())
            return false;

        const auto mods = key.getModifiers();
        if (mods.isCommandDown() && mods.isAltDown()
            && (key.getKeyCode() == 'X' || key.getKeyCode() == 'x'))
        {
            AlertWindow w (TRANS("Sysex77"), TRANS("Code :"), AlertWindow::QuestionIcon);
            w.addTextEditor ("code", String());
            w.addButton (TRANS("OK"),      1, KeyPress (KeyPress::returnKey));
            w.addButton (TRANS("Annuler"), 0, KeyPress (KeyPress::escapeKey));

            if (w.runModalLoop() == 1
                && w.getTextEditorContents ("code").trim().equalsIgnoreCase ("sysex"))
                importWebBanksFromBinary();

            return true;
        }
        return false;
    }
    void mouseDown (const MouseEvent& e) override
    {
        Logger::writeToLog("Librairie mouse event");
    }
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        Logger::writeToLog("Librairie: changeListener");
        voicesListA.loadBank();
        voicesListB.loadBank();
        voicesListC.loadBank();
        voicesListD.loadBank();

        labelInfoLine.setText (bankSelected.isNotEmpty()
                                 ? bankSelected + "  —  " + String (arrayListVoices.size()) + " voix"
                                 : String ("Aucune banque sélectionnée"),
                               dontSendNotification);
    }
    void resized() override
    {
        // const int bankWidth = 140; // Largeur de la table BANK
        int tableWidth;
        tableWidth = (getWidth()-  64)/5;
        
        btVoice.setBounds (tableWidth + 20,10,64,24);
        btMulti.setBounds (tableWidth + 94, 10,64,24);
        btSeq.setBounds (tableWidth + 168, 10,64,24);
        btSend.setBounds (getWidth()-74,10,64,24);
        btReceive.setBounds(getWidth()-140, 10, 64, 24);
        btStop.setBounds(btReceive.getBounds());
        voicesListA.setBounds(tableWidth + 16, 44, tableWidth, getHeight()-44);
        voicesListB.setBounds(tableWidth + 26 + tableWidth, 44, tableWidth, getHeight()-44);
        voicesListC.setBounds(tableWidth + 36 + tableWidth+ tableWidth, 44, tableWidth, getHeight()-44);
        voicesListD.setBounds(tableWidth + 46 + tableWidth+ tableWidth + tableWidth, 44, tableWidth, getHeight()-44);
        
        searchBox.setBounds(8, 10, tableWidth, 22);
        bankList.setBounds(8, 36, tableWidth, getHeight()-46);

        // Info-line : dans l'espace libre de la barre du haut (entre les onglets et RECEIVE).
        const int infoX = tableWidth + 240;
        labelInfoLine.setBounds (infoX, 10, jmax (0, getWidth() - 150 - infoX), 24);
    }
    //==============================================================================
    void timerCallback() override   // (plus utilisé pour la réception — gardé pour compat Timer)
    {
        stopTimer();
    }

    // Demande un nom puis écrit la banque capturée. Appelé à la fin de RECEIVE (après la
    // fenêtre de progression), pour proposer le nom AVANT d'écrire (plus de « UNNAMED.syx »
    // à renommer ensuite).
    void saveCapturedBankWithPrompt()
    {
        if (arraySysex.isEmpty() || ! arraySysex[0].isSysEx())
        {
            labelInfoLine.setText ("Aucune voix recue (synthe muet ou non branche ?)",
                                   dontSendNotification);
            labelInfoLine.setColour (Label::textColourId, Colours::orange);
            return;
        }

        AlertWindow w (TRANS("Banque recue"), TRANS("Nom de la banque :"),
                       AlertWindow::QuestionIcon);
        w.addTextEditor ("name", "INTERNAL");
        w.addButton (TRANS("Enregistrer"), 1, KeyPress (KeyPress::returnKey));
        w.addButton (TRANS("Annuler"),     0, KeyPress (KeyPress::escapeKey));

        if (w.runModalLoop() != 1)
        {
            arraySysex.clear();
            labelInfoLine.setText ("Reception annulee", dontSendNotification);
            return;
        }

        String name = w.getTextEditorContents ("name").trim();
        if (name.isEmpty()) name = "INTERNAL";
        if (! name.endsWithIgnoreCase (".syx")) name += ".syx";

        File target = appDirPath.getChildFile (File::createLegalFileName (name));
        if (target.existsAsFile())
        {
            // Nom déjà pris : demander confirmation avant d'écraser (pas d'écrasement silencieux).
            if (! AlertWindow::showOkCancelBox (AlertWindow::WarningIcon, TRANS("Fichier existant"),
                    target.getFileName() + TRANS(" existe deja. L'ecraser ?"),
                    TRANS("Ecraser"), TRANS("Annuler")))
            {
                arraySysex.clear();
                labelInfoLine.setText ("Enregistrement annule", dontSendNotification);
                return;
            }
        }

        writeCapturedBank (target);
    }

    void writeCapturedBank (const File& target)
    {
        target.deleteFile();   // écriture propre (sinon FileOutputStream ajoute à la fin)
        FileOutputStream fos (target);

        // Échec d'ouverture du flux (disque plein, permissions…) : ne PAS prétendre que la
        // capture est OK. On signale et on s'arrête (le « Capture OK » était mensonger).
        if (fos.failedToOpen())
        {
            labelInfoLine.setText ("ERREUR : impossible d'ecrire " + target.getFileName(),
                                   dontSendNotification);
            labelInfoLine.setColour (Label::textColourId, Colours::red);
            arraySysex.clear();
            sendChangeMessage();
            return;
        }

        // Validation du checksum Yamaha À LA RÉCEPTION : on ne prétend PAS que la capture est
        // saine sans l'avoir vérifiée. Chaque message F0…F7 reçu est un bulk dump -> on contrôle
        // son checksum (cf. SyVoice::verifyYamahaBulkChecksum). Les blocs invalides sont quand
        // même écrits (on n'altère pas la capture brute) mais signalés. « Fiabilité d'abord ».
        int total = 0, bad = 0;
        for (auto& m : arraySysex)
        {
            fos.write (m.getRawData(), m.getRawDataSize());
            ++total;
            if (! SyVoice::verifyYamahaBulkChecksum ((const uint8*) m.getRawData(),
                                                     m.getRawDataSize()))
            {
                ++bad;
                Logger::writeToLog ("Checksum INVALIDE (bloc " + String (total) + ")");
            }
        }
        fos.flush();
        arraySysex.clear();

        labelInfoLine.setText ((bad == 0
                                  ? "Recu : " + String (total) + " voix (checksum OK) -> "
                                  : "ATTENTION : " + String (bad) + "/" + String (total)
                                      + " blocs checksum INVALIDE -> ")
                                 + target.getFileName(),
                               dontSendNotification);
        if (bad > 0) labelInfoLine.setColour (Label::textColourId, Colours::red);
        else         labelInfoLine.removeColour (Label::textColourId);

        loadBankRequest = true;
        sendChangeMessage();
        repaint();
    }
    void buttonClicked (Button* button) override
    {
        Logger::writeToLog("Librairie -> ButtonClicked");
        if(button == &btSend)
        {
            String  bankName =appDirPath.getFullPathName()+"/";
            if (bankSelected.isNotEmpty())
            {
                bankName = bankName + bankSelected;
                Logger::writeToLog(bankName);
                if (! sender.send (adresseOscSendBank, bankSelected)) // [5]
                    Logger::writeToLog ("OSC erreur");;
            }
        }
        else if(button == &btReceive)
        {
            btReceive.setEnabled(false);
            btSend.setEnabled(false);

            // Capture fraîche : on vide AVANT d'armer (handleIncomingMidiMessage n'ajoute que
            // si requestSysex==true, donc clear pendant qu'il est false est sûr).
            arraySysex.clear();
            requestSysex = true;

            // Pacing requête→réponse piloté par la fenêtre : pour chaque voix, on émet le dump
            // request PUIS on attend sa réponse avant la suivante (rafale = SY77 saturé -> dump
            // tronqué). User-triggered : seul effet « écriture vers le synthé » (il LIT la banque).
            BankReceiveProgressWindow w (64,
                [] { return arraySysex.size(); },
                [this] (int mem) { sender.send (adresseOscRequestDump, mem); });
            w.runThread();   // modal, animé ; se ferme quand complet / Annuler

            requestSysex = false;
            btReceive.setEnabled(true);
            btSend.setEnabled(true);

            // Propose un nom puis écrit la banque (ou signale « aucune voix reçue »).
            saveCapturedBankWithPrompt();
        }
    }
    
private:
    //       LibrairiePage& owner;
    // Composant top-level sur lequel le KeyListener de l'easter-egg est posé (cf.
    // parentHierarchyChanged). SafePointer : se nullifie si le top-level est détruit avant nous.
    Component::SafePointer<Component> keyHost;
    ToggleButton toggleButton;
    
    
    
    
    // FIN DES XML FONCTIONS
    void addBtAndMakeStyle (TextButton& textButton)
    {
        textButton.setClickingTogglesState(true);
        textButton.setRadioGroupId(77);
        textButton.setColour(TextButton::ColourIds::buttonOnColourId, Colours::red);
        addAndMakeVisible (textButton);
    }

    //==============================================================================
    // Import des banques « SY77 AllTheWeb » embarquées (BinaryData::SY77_AllTheWeb_zip).
    // Le zip est DÉJÀ curé (231 banques SY77 valides, vérifiées bloc par bloc lors de sa
    // génération) -> simple décompression dans appDirPath/"SY77 (web)", sans filtre runtime.
    // overwrite=true rend l'easter-egg idempotent (rejouer ne duplique pas).
    // N'écrit QUE des fichiers locaux — jamais vers le synthé (sûreté matérielle).
    void importWebBanksFromBinary()
    {
        File destDir = appDirPath.getChildFile ("SY77 (web)");
        destDir.createDirectory();

        ZipFile zip (new MemoryInputStream (BinaryData::SY77_AllTheWeb_zip,
                                            (size_t) BinaryData::SY77_AllTheWeb_zipSize, false),
                     true);
        if (zip.uncompressTo (destDir, true).failed())
        {
            labelInfoLine.setText ("ERREUR : decompression des banques web echouee",
                                   dontSendNotification);
            labelInfoLine.setColour (Label::textColourId, Colours::red);
            return;
        }

        // Comptage (le système de fichiers macOS est insensible à la casse : "*.syx" couvre
        // aussi les ".SYX", comme le loadBank de la librairie).
        Array<File> syxFiles;
        destDir.findChildFiles (syxFiles, File::TypesOfFileToFind::findFiles, true, "*.syx");

        // Rafraîchissement de l'UI (cf. fin de filesDropped). Synchrone : le
        // changeListenerCallback recharge les 4 colonnes de voix ET réécrit labelInfoLine ;
        // on l'exécute d'abord pour pouvoir afficher notre bilan ensuite.
        bankList.loadBank();
        bankList.sendSynchronousChangeMessage();
        labelInfoLine.removeColour (Label::textColourId);
        labelInfoLine.setText (String (syxFiles.size()) + " banques web disponibles dans la librairie",
                               dontSendNotification);
        repaint();
    }
    Label labelInfoLine {"","Test Info Line -- -- -- -- -- -- -- -- -- -- --"};
    TextButton btSend {TRANS("SEND BANK->")};
    TextButton btReceive {TRANS("RECEIVE BANK <-")};
    TextButton btStop {"STOP"};
    
    TextButton btVoice {"VOICE"};
    TextButton btMulti {"MULTI"};
    TextButton btSeq{"SEQ"};
    
    //    TableListBox table { {}, this };;
    //   TableListBox tableBank { {}, this };;
    BankVoicesTable voicesListA { 0,  "Bank A" };
    BankVoicesTable voicesListB { 16, "Bank B" };
    BankVoicesTable voicesListC { 32, "Bank C" };
    BankVoicesTable voicesListD { 48, "Bank D" };
    BankTableModel bankList;
    TextEditor searchBox;
    
    int rowSelected;
    
    
    int oldSize;
    
    int numRows;
    SysexBusSender sender;  // [2]
    
};
//==============================================================================

//==============================================================================
/**
 This class shows how to implement a TableListBoxModel to show in a TableListBox.
 */
//==============================================================================
