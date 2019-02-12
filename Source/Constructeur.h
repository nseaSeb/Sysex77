//
//  Constructeur.h
//  MidiDemo
//
//  Created by Sébastien Portrait on 11/11/2018.
//  Copyright © 2018 JUCE. All rights reserved.
//

#ifndef Constructeur_h
#define Constructeur_h

addAndMakeVisible(ledMidiIn);
ledMidiIn.setColour(TextButton::buttonOnColourId, Colours::white);

addAndMakeVisible(ledMidiOut);
ledMidiOut.setColour(TextButton::buttonOnColourId, Colours::orangered);


//Setting
addLabelAndSetStyle (midiInputLabel, true);
addLabelAndSetStyle (midiOutputLabel, true);
addLabelAndSetStyle (incomingMidiLabel, true);

//controller
addLabelAndSetStyle (ctrSwitchLabel, false);
addLabelAndSetStyle (ctrModLabel, false);



addAndMakeVisible (tableMod);
//tableMod.setVisible(false);
addAndMakeVisible (tableSwitch);
winCtrHide();


addAndMakeVisible(listSwitch);
addAndMakeVisible(listMod);
listMod.setModel(this);
listMod.setMultipleSelectionEnabled (false);
midiCtr.add ("Test");
midiCtr.add ("Exemple");


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

addAndMakeVisible (midiInputSelector.get());
addAndMakeVisible (midiOutputSelector.get());



for (int i = 0; i < 4; ++i)
{
    auto* b = radioButtons.add (new TextButton );
    
    addAndMakeVisible (b);
    b->setRadioGroupId (42);
    b->setClickingTogglesState (true);
    b->addListener(this);
    b->setColour(TextButton::buttonOnColourId, Colours::orangered);
    
    switch (i)
    {
        case 0:     b->setConnectedEdges (Button::ConnectedOnRight);
            b->setButtonText("Setting");
            
            break;
        case 1:     b->setConnectedEdges (Button::ConnectedOnRight + Button::ConnectedOnLeft);
            b->setButtonText("Librairie");
            break;
        case 2:     b->setConnectedEdges (Button::ConnectedOnLeft);
            b->setButtonText("Controller");
            break;
        case 3:     b->setConnectedEdges (Button::ConnectedOnLeft);
            b->setButtonText("Voice");
            break;
        default:    break;
    }
}

radioButtons.getUnchecked (0)->setToggleState (true, dontSendNotification);

setSize (800, 700);

startTimer (500);
}

~MidiDemo()
{
    stopTimer();
    midiInputs .clear();
    midiOutputs.clear();
    keyboardState.removeListener (this);
    
    midiInputSelector .reset();
    midiOutputSelector.reset();
}
//=================


//==============================================================================
void addLabelAndSetStyle (Label& label, bool visible)
{
    label.setFont (Font (15.00f, Font::plain));
    label.setJustificationType (Justification::centredLeft);
    label.setEditable (false, false, false);
    label.setColour (TextEditor::textColourId, Colours::black);
    label.setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    
    addAndMakeVisible (label);
    label.setVisible(visible);
}

#endif /* Constructeur_h */
