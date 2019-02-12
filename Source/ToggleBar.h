//
//  ToggleBar.h
//  MidiDemo
//
//  Created by Sébastien Portrait on 10/11/2018.
//  Copyright © 2018 JUCE. All rights reserved.
//

#ifndef ToggleBar_h
#define ToggleBar_h

void buttonClicked (Button* button) override
{
    
    
    Logger::writeToLog("Bouton clicked");
    Logger::writeToLog(  button->getButtonText());
    
    if (radioButtons.operator[](0) == button)
    {
        Logger::writeToLog("ZERO");
        winSettingShow();
        winCtrHide();
    }
    else if (radioButtons.operator[](1) == button)
    {
        winSettingHide();
        winCtrHide();
        Logger::writeToLog("Un");
    }
    
    else if (radioButtons.operator[](2) == button)
    {
        winSettingHide();
        winCtrShow();
        Logger::writeToLog("Un");
    }
    else if (radioButtons.operator[](3) == button)
    {
        winSettingHide();
        winCtrHide();
    }
}

void winSettingHide ()
{
    
    midiInputLabel.setVisible(false);
    midiOutputLabel.setVisible(false);
    incomingMidiLabel.setVisible(false);
    midiMonitor.setVisible(false);
    pairButton.setVisible(false);
    midiInputSelector->setVisible(false);
    midiOutputSelector->setVisible(false);
    
    
}
void winSettingShow ()
{
  //  winSetting.setVisible(true);
    midiInputLabel.setVisible(true);
    midiOutputLabel.setVisible(true);
    incomingMidiLabel.setVisible(true);
    midiMonitor.setVisible(true);
    pairButton.setVisible(true);
    midiInputSelector->setVisible(true);
    midiOutputSelector->setVisible(true);
}
void winCtrHide()
{
    ctrSwitchLabel.setVisible(false);
    ctrModLabel.setVisible(false);
    tableMod.setVisible(false);
    tableSwitch.setVisible(false);
}
void winCtrShow()
{
    ctrSwitchLabel.setVisible(true);
    ctrModLabel.setVisible(true);
    tableMod.setVisible(true);
    tableSwitch.setVisible(true);
}
#endif /* ToggleBar_h */
