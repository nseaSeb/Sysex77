//
//  Declarations.h
//  MidiDemo
//
//  Created by Sébastien Portrait on 11/11/2018.
//  Copyright © 2018 JUCE. All rights reserved.
//

#ifndef Declarations_h
#define Declarations_h


//==============================================================================
//Session generale

Image imgBack;

MidiKeyboardState keyboardState;
MidiKeyboardComponent midiKeyboard;

OwnedArray<TextButton> radioButtons;

TextButton ledMidiOut;
TextButton ledMidiIn;

std::unique_ptr<MidiDeviceListBox> midiInputSelector, midiOutputSelector;
ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;

CriticalSection midiMonitorLock;
Array<MidiMessage> incomingMessages;

//Constantes

const int margeKey = 120;
const int marge = 8;
const int margeBt = 40;
//==============================================================================
//Session Setting
Label midiInputLabel    { "Midi Input Label",  "MIDI Input:" };
Label midiOutputLabel   { "Midi Output Label", "MIDI Output:" };
Label incomingMidiLabel { "Incoming Midi Label", "Received MIDI messages:" };

TextEditor midiMonitor  { "MIDI Monitor" };
TextButton pairButton   { "MIDI Bluetooth devices..." };

///==============================================================================
// Session controleur
Label   ctrSwitchLabel { "","Foot Switch Controller"};
Label   ctrModLabel {"","Modulation Controller"};
ListBox listSwitch;
ListBox listMod;
TableTutorialComponent tableMod;
//TableListBox tableMod {"tableMod",&gridModel};
//TableListBox tableMod;
TableTutorialComponent    tableSwitch;

    
Array<String> midiCtr;

#endif /* Declarations_h */
