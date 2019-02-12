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
        
        btVoice.setToggleState(true, NotificationType::dontSendNotification);
        
        
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
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
    BankATableModel voicesListA;
    BankBTableModel voicesListB;
    BankCTableModel voicesListC;
    BankDTableModel voicesListD;
    BankTableModel bankList;
    
    int rowSelected;
    
    
    int oldSize;
    
    int numRows;
    OSCSender sender;  // [2]
    
};
//==============================================================================

//==============================================================================
/**
 This class shows how to implement a TableListBoxModel to show in a TableListBox.
 */
//==============================================================================
