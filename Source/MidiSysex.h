/*
 ==============================================================================
 
 MidiSysex.h
 Created: 13 Nov 2018 8:45:55pm
 Author:  Sébastien Portrait
 
 ==============================================================================
 */

#pragma once
void addOscListener ()
{
    
    addListener (this, adresseOscFoot); // [4]
    addListener (this, adresseOscMod);
    addListener(this, adresseOpMode);
    addListener(this, adresseOscSendBank);
    addListener(this,adresseOscRepaint);
    addListener(this, oscTotalVoiceVolume);

    

    addListener(this, oscVoicePan1);
    addListener(this, oscVoicePan2);
        addListener(this, oscVoicePan3);
        addListener(this, oscVoicePan4);
    addListener(this,oscVoiceGrp1);
     addListener(this,oscVoiceGrp2);
        addListener(this,oscVoiceGrp3);
        addListener(this,oscVoiceGrp4);
    addListener(this, oscVoicePitch1);
        addListener(this, oscVoicePitch2);
        addListener(this, oscVoicePitch3);
        addListener(this, oscVoicePitch4);
    addListener(this, oscVoiceFine1);
        addListener(this, oscVoiceFine2);
        addListener(this, oscVoiceFine3);
        addListener(this, oscVoiceFine4);
    addListener(this, oscVoiceFixe1);
    addListener(this, oscVoiceFixe2);
        addListener(this, oscVoiceFixe3);
        addListener(this, oscVoiceFixe4);
    addListener(this, oscSendMidiMessage);

    

    addListener(this, "/SYSEX");
}





void oscMessageReceived (const OSCMessage& message) override
{
    auto address = message.getAddressPattern();

    if (address.matches("/SYSEX"))
    {
        if (message.size() > 0 )
        {
            uint8 sysexdata[9];
            //     Logger::writeToLog("Foot " + String( message[0].getInt32()));
            for(int i =0; i < message.size() ;i++)
                sysexdata[i] = message[i].getInt32();
            
            MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
      //      m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
    }
    if (address.matches(adresseOscRepaint))
        repaint();
    
    if (address.matches(adresseOscFoot))
    {
        uint8 sysexdata[9] = { 0x43, sysexEngine, 0x34, 0x0f, 0x00, 0x00, 0x2d, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    
    if (address.matches(adresseOscMod))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x0f, 0x00, 0x00, 0x2c, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscTotalVoiceVolume))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x02, 0x00, 0x00, 0x3f, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    
   
     if (address.matches(oscVoiceGrp1))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x00, 0x00, 0x08, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceGrp2))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x20, 0x00, 0x08, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceGrp3))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x40, 0x00, 0x08, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceGrp4))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x60, 0x00, 0x08, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoicePitch1))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoicePitch2))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x20, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoicePitch3))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x40, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoicePitch4))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x60, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFine1))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x00, 0x00, 0x01, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFine2))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x20, 0x00, 0x01, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFine3))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x40, 0x00, 0x01, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFine4))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x60, 0x00, 0x01, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFixe1))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x07, 0x00, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFixe2))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x07, 0x20, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFixe3))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x07, 0x40, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFixe4))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x07, 0x60, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(adresseOpMode))
    {
        Logger::writeToLog("Operateur mode envoi");
        uint8 sysexdata[9] = { 0x43, 0X10, 0x34, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 };
        sendSysex(message, sysexdata);
        
    }
    if (address.matches ( oscSendMidiMessage))
    {
        if (message.size() == 1 && message[0].isInt32())
        {
            for(auto i = 0 ; 0 == message[0].getInt32(); i++)
                 sendToOutputs (oscMidiMessage[i]);
        }
    }
    if (address.matches(adresseOscSendBank))
    {
        File file {(appDirPath.getFullPathName() + "/" + message[0].getString())};
        MemoryBlock mb;
        if(file.loadFileAsData(mb))
        {
            const uint8* const data = (const uint8*) mb.getData();
            sendRaw(data, mb.getSize());
        }
        
    }
    //        rotaryKnob.setValue (jlimit (0.0f, 10.0f, message[0].getFloat32())); // [6]
}
void sendSysex(const OSCMessage& message, uint8 sysexdata[0])
{
    if (message.size() == 1 && message[0].isInt32())
    {
        //     Logger::writeToLog("Foot " + String( message[0].getInt32()));
        
        sysexdata[8] = message[0].getInt32();
        MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }
    
}
void sendRaw(const void* sysexData, const long dataSize)
{
    if (dataSize >1)
    {
        Logger::writeToLog(String(dataSize));
        //    Logger::writeToLog(String(dataSize));
        //Envoi midi brut depuis la mémoire
        MidiMessage m = MidiMessage (sysexData, dataSize);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }
    else
    {
        Logger::writeToLog("Error Send RAW");
    }
}
