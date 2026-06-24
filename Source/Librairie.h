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


struct LibrairiePage   : public Component,public Button::Listener, private Timer,public ChangeListener, public ChangeBroadcaster
{
    LibrairiePage()
    {
        setOpaque(false);
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
                labelInfoLine.setText (name + "  —  " + SyVoice::voiceTypeLabel (type)
                                       + (type == 3 ? "   · chargeable dans l'editeur" : ""),
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
        stopTimer();
        
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
        
        bankList.setBounds(8, 10, tableWidth, getHeight()-20);

        // Info-line : dans l'espace libre de la barre du haut (entre les onglets et RECEIVE).
        const int infoX = tableWidth + 240;
        labelInfoLine.setBounds (infoX, 10, jmax (0, getWidth() - 150 - infoX), 24);
    }
    //==============================================================================
    void timerCallback() override   // Timer of Sysex DUMP (receive sysex)
    {
        Logger::writeToLog("Timer : " + String(timeOut));
        timeOut ++;
        if(timeOut >20)
        {
            saveDump();
        }
    }
    void saveDump()  //When Sysex is dumped
    {
        stopTimer();
        btReceive.setEnabled(true);
        btSend.setEnabled(true);
        newMessage = false;
        requestSysex = false;
        btStop.setVisible(false);
        
        if (arraySysex[0].isSysEx())
        {
            File myFile {(appDirPath.getFullPathName() + "/UNNAMED.syx")};
            myFile.deleteFile();
            FileOutputStream fos (myFile);
            
            for (auto& m : arraySysex)
            {
                Logger::writeToLog(m.getDescription());
                fos.write(m.getRawData(), m.getRawDataSize());
                
            }
            fos.flush();
            arraySysex.clear();
            loadBankRequest = true; //Make a better code later!
            sendChangeMessage();
            repaint();
            
        }
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
            newMessage = false;
            requestSysex = true;
            timeOut=0;
            startTimer(500);
            btStop.setVisible(true);
            
        }
        else if(button == &btStop)
        {
            saveDump(); //la fonction save dump vérifie la presence de sysex
        }
    }
    
private:
    //       LibrairiePage& owner;
    ToggleButton toggleButton;
    
    
    
    
    // FIN DES XML FONCTIONS
    void addBtAndMakeStyle (TextButton& textButton)
    {
        textButton.setClickingTogglesState(true);
        textButton.setRadioGroupId(77);
        textButton.setColour(TextButton::ColourIds::buttonOnColourId, Colours::red);
        addAndMakeVisible (textButton);
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
