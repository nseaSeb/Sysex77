/*
 ==============================================================================
 
 MidiSysex.h
 Created: 13 Nov 2018 8:45:55pm
 Author:  Sébastien Portrait
 
 ==============================================================================
 */

#pragma once
#include "SysexUtils.h"

// Installe le gestionnaire du bus intra-processus (remplace l'ancien enregistrement
// d'écouteurs OSC sur le port UDP 9001). Un seul abonné : cette instance de MidiDemo.
// Le dispatch par adresse se fait toujours dans oscMessageReceived ci-dessous.
void addOscListener ()
{
    SysexBus::get().onMessage = [this] (const OSCMessage& message) { oscMessageReceived (message); };
}





void oscMessageReceived (const OSCMessage& message)
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

            // Canal : l'octet device est imposé ici (source de vérité unique), ce qui
            // rend le device sélectionnable pour TOUS les widgets sans toucher aux
            // ~30 tableaux sysexdata[9] qui codent 0x10 en dur ailleurs.
            sysexdata[1] = SyVoice::deviceByte (sysexDeviceNumber);

            MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
      //      m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
    }
    if (address.matches(adresseOscRepaint))
        repaint();
    
    // Table OSC -> paramètre Sysex SY77 (remplace ~20 blocs copier-collés identiques).
    // Chaque message paramétrique a la forme { 0x43, canal, 0x34, group, addrHi, addrLo, param, 0x00, valeur }.
    // L'octet canal indiqué ici n'a pas d'importance : il est réécrit avec le device
    // global dans sendSysex() (source de vérité unique pour le canal).
    const struct { const String& osc; uint8 channel, group, addrHi, addrLo, param; } sysexMap[] =
    {
        { adresseOscFoot,      0x10, 0x0f, 0x00, 0x00, 0x2d },
        { adresseOscMod,       0x10,        0x0f, 0x00, 0x00, 0x2c },
        { oscTotalVoiceVolume, 0x10,        0x02, 0x00, 0x00, 0x3f },
        { adresseOpMode,       0x10,        0x02, 0x00, 0x00, 0x00 },
        { oscVoiceGrp1,        0x10,        0x03, 0x00, 0x00, 0x08 },
        { oscVoiceGrp2,        0x10,        0x03, 0x20, 0x00, 0x08 },
        { oscVoiceGrp3,        0x10,        0x03, 0x40, 0x00, 0x08 },
        { oscVoiceGrp4,        0x10,        0x03, 0x60, 0x00, 0x08 },
        { oscVoicePitch1,      0x10,        0x03, 0x00, 0x00, 0x02 },
        { oscVoicePitch2,      0x10,        0x03, 0x20, 0x00, 0x02 },
        { oscVoicePitch3,      0x10,        0x03, 0x40, 0x00, 0x02 },
        { oscVoicePitch4,      0x10,        0x03, 0x60, 0x00, 0x02 },
        { oscVoiceFine1,       0x10,        0x03, 0x00, 0x00, 0x01 },
        { oscVoiceFine2,       0x10,        0x03, 0x20, 0x00, 0x01 },
        { oscVoiceFine3,       0x10,        0x03, 0x40, 0x00, 0x01 },
        { oscVoiceFine4,       0x10,        0x03, 0x60, 0x00, 0x01 },
        { oscVoiceFixe1,       0x10,        0x07, 0x00, 0x00, 0x02 },
        { oscVoiceFixe2,       0x10,        0x07, 0x20, 0x00, 0x02 },
        { oscVoiceFixe3,       0x10,        0x07, 0x40, 0x00, 0x02 },
        { oscVoiceFixe4,       0x10,        0x07, 0x60, 0x00, 0x02 },
    };

    for (auto& e : sysexMap)
    {
        if (address.matches (e.osc))
        {
            uint8 sysexdata[9] = { 0x43, e.channel, 0x34, e.group, e.addrHi, e.addrLo, e.param, 0x00, 0x00 };
            sendSysex (message, sysexdata);
            break; // les adresses sont mutuellement exclusives
        }
    }
    if (address.matches ( oscSendMidiMessage))
    {
        if (message.size() == 1 && message[0].isInt32())
        {
            // FIXME (à tester sur matériel) : cette fonctionnalité « envoi du nom de voix »
            // est aujourd'hui inactive — oscMidiMessage est déclaré `const` et n'est jamais
            // rempli (voir Voice.h::textEditorReturnKeyPressed, l'écriture y est perdue car
            // l'array est const). La boucle ci-dessous est rendue défensive (bornée par la
            // taille réelle ET par le nombre demandé) pour ne plus risquer de lecture hors
            // limites / boucle infinie. Avant de réactiver l'envoi réel, vérifier le format
            // sysex (l'adresse du caractère de nom ne semble pas incrémentée).
            const int requested = jmin (message[0].getInt32(), oscMidiMessage.size());
            for (int i = 0; i < requested; ++i)
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
    if (address.matches(adresseOscSendVoice))
    {
        // Envoie au synthé le bloc sysex d'une seule voix de la banque courante.
        if (message.size() == 1 && message[0].isInt32())
        {
            auto block = SyVoice::getVoiceBlock ((const uint8*) currentBankData.getData(),
                                                 currentBankData.getSize(),
                                                 message[0].getInt32());
            if (block.getSize() > 1)
                sendRaw (block.getData(), (long) block.getSize());
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
        sysexdata[1] = SyVoice::deviceByte (sysexDeviceNumber); // canal global (cf. /SYSEX)
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
        MidiMessage m = MidiMessage (sysexData, (int) dataSize);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }
    else
    {
        Logger::writeToLog("Error Send RAW");
    }
}
