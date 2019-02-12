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

static const String adresseOscFoot = "/77Foot";
static const String adresseOscMod = "/77Mod";
static const String adresseOpMode = "/77OpMode";
static const String adresseOscSendBank ="/77SendBank";
static const String adresseOscRepaint = "/77Repaint";
static const String adresseOscParseVoices = "/77ParseVoices";
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
static const Array<MidiMessage>    oscMidiMessage;
static const String oscSendMidiMessage = "/77MidiMessage";

static   StringArray  arrayBank;    //la liste des banques
static  StringArray arrayListVoices; // liste des voices
static const int maxFiles = 512;

static        Array<File> BankFiles; //les fichiers des banques
Array<MidiMessage> messages;
Array<MidiMessage> arraySysex;
int SYModel =1;
int CommonFoot;
int CommonMod;

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
static uint8 sysexEngine;
static     float fAngle = -90 * (float_Pi / 180.0); //Radiant to draw at 90°
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
#include "MidiObjects.h"

//==============================================================================
struct MidiDeviceListEntry : ReferenceCountedObject
{
    MidiDeviceListEntry (const String& deviceName) : name (deviceName) {}
    
    String name;
    std::unique_ptr<MidiInput> inDevice;
    std::unique_ptr<MidiOutput> outDevice;
    
    using Ptr = ReferenceCountedObjectPtr<MidiDeviceListEntry>;
};

//==============================================================================
struct DemoTabbedComponent  : public TabbedComponent
{
    DemoTabbedComponent (bool isRunningComponenTransformsDemo)
    : TabbedComponent (TabbedButtonBar::TabsAtTop)
    {
        auto colour = findColour (ResizableWindow::backgroundColourId);
        addTab (TRANS("Setting"),     colour, new ConfigPage (), true);  
        addTab (TRANS("Librairie"),     colour, new LibrairiePage (), true);
       // addTab (TRANS("Midi"),     colour, new ControllerPage (), true);
        addTab (TRANS("Voice"), colour, new VoicePage(), true);
        addTab (("MidiSetting"),colour,nullptr,false);
   
        
        
        
        //        getTabbedButtonBar().getTabButton (5)->setExtraComponent (new CustomTabButton (isRunningComponenTransformsDemo),
        //                                                                  TabBarButton::afterText);
    }
    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName)override
    {
        if(newCurrentTabIndex<3)
        intTabIndex = newCurrentTabIndex;
    }
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


//==============================================================================
class MidiDemo  : public Component,
private Timer,
private MidiKeyboardStateListener,
private MidiInputCallback,
private AsyncUpdater,
private TextButton::Listener,
 public ComboBox::Listener,
 public Value::Listener,
private OSCReceiver,                                                          // [1]
private OSCReceiver::ListenerWithOSCAddress<OSCReceiver::MessageLoopCallback> // [2]
{
public:
    //==============================================================================
    MidiDemo (bool isRunningComponenTransformsDemo = false)
    :  midiKeyboard       (keyboardState, MidiKeyboardComponent::horizontalKeyboard),
    midiInputSelector  (new MidiDeviceListBox ("Midi Input Selector",  *this, true)),
    midiOutputSelector (new MidiDeviceListBox ("Midi Output Selector", *this, false)),
    tabs (isRunningComponenTransformsDemo)
    
    {
        // Init to light the Paint call
        auto dir = File::getSpecialLocation(File::currentExecutableFile);
        
        int numTries = 0;
        
        while (! dir.getChildFile ("Resources").exists() && numTries++ < 15)
            dir = dir.getParentDirectory();
        Logger::writeToLog("Midi Demo init : dir exist");
        //
       // File fileImageSY77 = dir.getFullPathName() + "/Resources/Sysex77.png";
       // imgBack = ImageCache::getFromFile(fileImageSY77);
        imgBack = ImageCache::getFromMemory(BinaryData::Sysex77_png, BinaryData::Sysex77_pngSize);
        if(SYModel==3)
            imgBack = ImageCache::getFromMemory(BinaryData::Sysex99_png, BinaryData::Sysex99_pngSize);
        

        
        addAndMakeVisible (tabs);
  
        addLabelAndSetStyle (midiInputLabel);
        addLabelAndSetStyle (midiOutputLabel);
        addLabelAndSetStyle (incomingMidiLabel);
        
        addAndMakeVisible(btBulk);
        btBulk.setClickingTogglesState(true);
        btBulk.setColour(TextButton::ColourIds::buttonOnColourId, Colours::darkorange);
        btBulk.addListener(this);
        
        midiKeyboard.setName ("MIDI Keyboard");
        addAndMakeVisible (midiKeyboard);
        
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
        if (! connect (9001))                   // [3]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
        // tell the component to listen for OSC messages matching this address:
        //       addListener (this, "/77Foot"); // [4]
        //       addListener (this, "/77Mod");
        
        if (! senderMidiIn.connect ("127.0.0.1", 9002)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
        

        addOscListener();
        tabs.setVisible(false);
        tabs.setAlwaysOnTop(true);
        
        
        setSize (732, 520);
        tabs.setCurrentTabIndex(1);
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
            uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x0f, 0x00, 0x00, 0x34, 0x00, 0x00 };
            sysexdata[8] = btBulk.getToggleState();
            MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
            m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
    }
    //==============================================================================
    void timerCallback() override
    {
        if(tabs.getCurrentTabIndex()==3)
        {
            tabs.setCurrentTabIndex(intTabIndex);
           tabs.setVisible(false);
            
            //repaint();
        }
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
        //Logger::writeToLog ("MidiDemo Paint");
        g.fillAll (SYColBackground);
        g.drawImageAt(imgBack,getLocalBounds().getWidth() - imgBack.getWidth() - 20, 10);
        
        
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
        
        
        
        midiInputLabel.setBounds (margin, margin + 34,
                                  (getWidth() / 2) - (2 * margin), 24);
        
        midiOutputLabel.setBounds ((getWidth() / 2) + margin, margin + 34,
                                   (getWidth() / 2) - (2 * margin), 24);
        
        midiInputSelector->setBounds (margin, (2 * margin) + 48,
                                      (getWidth() / 2) - (2 * margin),
                                      (getHeight() / 2) - ((4 * margin) + 64 ));
        
        midiOutputSelector->setBounds ((getWidth() / 2) + margin, (2 * margin) + 48,
                                       (getWidth() / 2) - (2 * margin),
                                       (getHeight() / 2) - ((4 * margin) + 64 ));
        
        pairButton.setBounds (margin, (getHeight() / 2) - (margin + 24),
                              getWidth() - (2 * margin), 24);
        
        incomingMidiLabel.setBounds (margin, getHeight() / 2, getWidth() - (2 * margin), 24);
        
        midiMonitor.setBounds (margin, (getHeight()/2 + (24 + margin)), getWidth() - (2 * margin), getHeight()/2 - 128);
        btBulk.setBounds(getWidth()/2, getHeight()/2, getWidth()/4, 24);
        
        tabs.setBounds (0,10,getWidth(),getHeight()-84);
        
        
        midiKeyboard.setBounds (margin +140, (getHeight() - 70 ), getWidth() - (2 * margin) - 140, 70);
        labelFoot.setBounds(margin,(getHeight() - 77 ), 134, 20);
        comboFoot.setBounds(margin,(getHeight() - 58 ), 134, 20);
        labelMod.setBounds(margin,(getHeight() - 40 ), 134, 20);
        comboMod.setBounds(margin,(getHeight() - 24 ), 134, 20);
    }
    
    void openDevice (bool isInput, int index)
    {
        if (isInput)
        {
            jassert (midiInputs[index]->inDevice.get() == nullptr);
            midiInputs[index]->inDevice.reset (MidiInput::openDevice (index, this));
            
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
            midiOutputs[index]->outDevice.reset (MidiOutput::openDevice (index));
            
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
                    g.drawText (parent.getMidiDevice (rowNumber, true)->name,
                                5, 0, width, height,
                                Justification::centredLeft, true);
            }
            else
            {
                if (rowNumber < parent.getNumMidiOutputs())
                    g.drawText (parent.getMidiDevice (rowNumber, false)->name,
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
                    
                    /*
                   senderMidiIn.send("/SYSEXinput", (int) message[0], message[1], message[2], message[3], message[4], message[5], message[6], message[7], message[8]);
                    */
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
                if(data[0]==0x43)
                    if(data[2] == 0x34)
                        valueSysexIn = make_var_array(data[3], data[4],data[5],data[6],data[7],data[8]);
                
                
            }
            
            messageText << message.getDescription() << "\n";

        }
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
    bool hasDeviceListChanged (const StringArray& deviceNames, bool isInputDevice)
    {
        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
        : midiOutputs;
        
        if (deviceNames.size() != midiDevices.size())
            return true;
        
        for (auto i = 0; i < deviceNames.size(); ++i)
            if (deviceNames[i] != midiDevices[i]->name)
                return true;
        
        return false;
    }
    
    ReferenceCountedObjectPtr<MidiDeviceListEntry> findDeviceWithName (const String& name, bool isInputDevice) const
    {
        const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
        : midiOutputs;
        
        for (auto midiDevice : midiDevices)
            if (midiDevice->name == name)
                return midiDevice;
        
        return nullptr;
    }
    
    void closeUnpluggedDevices (StringArray& currentlyPluggedInDevices, bool isInputDevice)
    {
        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
        : midiOutputs;
        
        for (auto i = midiDevices.size(); --i >= 0;)
        {
            auto& d = *midiDevices[i];
            
            if (! currentlyPluggedInDevices.contains (d.name))
            {
                if (isInputDevice ? d.inDevice .get() != nullptr
                    : d.outDevice.get() != nullptr)
                    closeDevice (isInputDevice, i);
                
                midiDevices.remove (i);
            }
        }
    }
    
    void updateDeviceList (bool isInputDeviceList)
    {
        auto newDeviceNames = isInputDeviceList ? MidiInput::getDevices()
        : MidiOutput::getDevices();
        
        if (hasDeviceListChanged (newDeviceNames, isInputDeviceList))
        {
            
            ReferenceCountedArray<MidiDeviceListEntry>& midiDevices
            = isInputDeviceList ? midiInputs : midiOutputs;
            
            closeUnpluggedDevices (newDeviceNames, isInputDeviceList);
            
            ReferenceCountedArray<MidiDeviceListEntry> newDeviceList;
            
            // add all currently plugged-in devices to the device list
            for (auto newDeviceName : newDeviceNames)
            {
                MidiDeviceListEntry::Ptr entry = findDeviceWithName (newDeviceName, isInputDeviceList);
                
                if (entry == nullptr)
                    entry = new MidiDeviceListEntry (newDeviceName);
                
                newDeviceList.add (entry);
            }
            
            // actually update the device list
            midiDevices = newDeviceList;
            
            // update the selection status of the combo-box
            if (auto* midiSelector = isInputDeviceList ? midiInputSelector.get() : midiOutputSelector.get())
                midiSelector->syncSelectedItemsWithDeviceList (midiDevices);
        }
    }
    
    //==============================================================================
    void addLabelAndSetStyle (Label& label)
    {
        label.setFont (Font (15.00f, Font::plain));
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
        uint8 sysexdata[9] = { 0x43, sysexEngine, 0x34, 0x0f, 0x00, 0x00, 0x2d, 0x00, 0x00 };
        sysexdata[8] = comboFoot.getSelectedItemIndex()+1;
        MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
        }
        if(comboBoxThatHasChanged == &comboMod)
        {
            uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x0f, 0x00, 0x00, 0x2c, 0x00, 0x00 };
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
        
        tutorialData.reset (XmlDocument::parse (tableFile));
        
        dataList   = tutorialData->getChildByName ("DATA");
        columnList = tutorialData->getChildByName ("HEADERS");
        
        numRows = dataList->getNumChildElements();
        
    }
    
    String getAttributeNameForColumnId (const int columnId) const
    {
        forEachXmlChildElement (*columnList, columnXml)
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
    std::unique_ptr<MidiDeviceListBox> midiInputSelector, midiOutputSelector;
    ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;
    
    CriticalSection midiMonitorLock;
    Array<MidiMessage> incomingMessages;
    DemoTabbedComponent tabs;
    Image imgBack;
    OSCSender   senderMidiIn;

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
