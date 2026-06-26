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
    // Chaque message paramétrique a la forme { 0x43, device, 0x34, group, addrHi, addrLo, param, 0x00, valeur },
    // construite par SyVoice::paramBytes ci-dessous. L'octet device est posé par le builder
    // (placeholder) puis réécrit avec le device global dans sendSysex() (source de vérité unique).
    const struct { const String& osc; uint8 group, addrHi, addrLo, param; } sysexMap[] =
    {
        { adresseOscFoot,      0x0f, 0x00, 0x00, 0x2d },
        { adresseOscMod,       0x0f, 0x00, 0x00, 0x2c },
        { oscTotalVoiceVolume, 0x02, 0x00, 0x00, 0x3f },
        { adresseOpMode,       0x02, 0x00, 0x00, 0x00 },
        { oscVoiceGrp1,        0x03, 0x00, 0x00, 0x08 },
        { oscVoiceGrp2,        0x03, 0x20, 0x00, 0x08 },
        { oscVoiceGrp3,        0x03, 0x40, 0x00, 0x08 },
        { oscVoiceGrp4,        0x03, 0x60, 0x00, 0x08 },
        { oscVoicePitch1,      0x03, 0x00, 0x00, 0x02 },
        { oscVoicePitch2,      0x03, 0x20, 0x00, 0x02 },
        { oscVoicePitch3,      0x03, 0x40, 0x00, 0x02 },
        { oscVoicePitch4,      0x03, 0x60, 0x00, 0x02 },
        { oscVoiceFine1,       0x03, 0x00, 0x00, 0x01 },
        { oscVoiceFine2,       0x03, 0x20, 0x00, 0x01 },
        { oscVoiceFine3,       0x03, 0x40, 0x00, 0x01 },
        { oscVoiceFine4,       0x03, 0x60, 0x00, 0x01 },
        { oscVoiceFixe1,       0x07, 0x00, 0x00, 0x02 },
        { oscVoiceFixe2,       0x07, 0x20, 0x00, 0x02 },
        { oscVoiceFixe3,       0x07, 0x40, 0x00, 0x02 },
        { oscVoiceFixe4,       0x07, 0x60, 0x00, 0x02 },
    };

    for (auto& e : sysexMap)
    {
        if (address.matches (e.osc))
        {
            // Format filaire construit par le builder unique (cf. SyVoice::paramBytes).
            // L'octet device [1] est un placeholder ici : sendSysex() le réécrit avec le
            // device global (source de vérité unique pour le canal). value = 0 (sendSysex
            // pose [8] depuis le message).
            auto sysexdata = SyVoice::paramBytes (sysexDeviceNumber, e.group, e.addrHi,
                                                  e.addrLo, e.param, 0x00);
            sendSysex (message, sysexdata.data());
            break; // les adresses sont mutuellement exclusives
        }
    }
    if (address.matches ( oscSendMidiMessage))
    {
        if (message.size() == 1 && message[0].isInt32())
        {
            // FIXME (à tester sur matériel) : cette fonctionnalité « envoi du nom de voix »
            // est aujourd'hui inactive — oscMidiMessage est déclaré `const` et n'est jamais
            // Envoi du nom de voix : oscMidiMessage contient les 10 messages VNAM0..VNAM9
            // construits dans Voice.h. Boucle bornée par la taille réelle ET le nombre
            // demandé (défense contre toute incohérence).
            const int requested = jmin (message[0].getInt32(), oscMidiMessage.size());
            for (int i = 0; i < requested; ++i)
                sendToOutputs (oscMidiMessage[i]);
        }
    }
    if (address.matches(adresseOscSendBank))
    {
        // Envoi de la banque SÉLECTIONNÉE. On réutilise currentBankData (déjà chargé à la
        // sélection, MÊME source que l'envoi d'une voix) au lieu de reconstruire
        // appDirPath + nom de fichier : cette reconstruction échouait pour toute banque rangée
        // en SOUS-DOSSIER (ex. les banques importées dans "SY77 (web)/", dont bankSelected ne
        // porte que le nom de fichier) -> l'envoi de banque ne partait jamais pour celles-ci.
        if (currentBankData.getSize() > 1)
        {
            auto blocks = SyVoice::splitSysexMessages ((const uint8*) currentBankData.getData(),
                                                       currentBankData.getSize());
            if (blocks.isEmpty())
            {
                // Pas de cadrage F0…F7 reconnu -> envoi brut (comportement d'avant).
                sendRaw (currentBankData.getData(), (long) currentBankData.getSize());
            }
            else
            {
                // Fenêtre de progression modale (barre + Annuler), envoi throttlé sur un thread
                // d'arrière-plan -> plus d'« envoi en aveugle » ni de gel de l'UI. Se ferme à la fin.
                Logger::writeToLog ("Send BANK : " + String (blocks.size()) + " messages");
                BankSendProgressWindow w (std::move (blocks),
                                          [this] (const MidiMessage& m) { sendToOutputs (m); });
                w.runThread();
            }
        }
        else
            Logger::writeToLog ("Send BANK : aucune banque chargee");
    }
    if (address.matches (adresseOscRequestDump))
    {
        // Dump request USER-TRIGGERED (bouton RECEIVE) : demande au SY77 d'émettre le bulk
        // d'UNE voix interne (numéro de mémoire passé en argument). Le pacing requête→réponse
        // est piloté par BankReceiveProgressWindow (envoyer les 64 d'un coup saturait le SY77).
        // Les réponses sont captées par handleIncomingMidiMessage (requestSysex==true).
        // FIABILITÉ : seul ce bouton déclenche un envoi vers le synthé (jamais au démarrage).
        if (message.size() == 1 && message[0].isInt32())
        {
            const int mem = juce::jlimit (0, 63, message[0].getInt32());
            sendToOutputs (SyVoice::voiceDumpRequest (sysexDeviceNumber, 0x00, (uint8) mem));
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
            {
                // AUDITION : on redirige la voix vers l'EDIT BUFFER ($7F) au lieu de son slot
                // mémoire d'origine -> le SY77 l'affiche/joue tout de suite, SANS écraser une
                // voix stockée. getVoiceBlock renvoie une COPIE : currentBankData reste intact.
                SyVoice::retargetVoiceToEditBuffer ((uint8*) block.getData(), (int) block.getSize());
                sendRaw (block.getData(), (long) block.getSize());

                // Ouvre aussi la voix dans l'éditeur : on décode ce qui est FIABLE
                // (vérifié sur banques réelles) — type/mode (offset 32) et nom (offset 33,
                // 10 car.). Les paramètres sonores détaillés nécessitent la carte d'offsets
                // vérifiée (la structure du codec Elixir ne correspond PAS aux banques réelles).
                const uint8* d = (const uint8*) block.getData();
                const int sz = (int) block.getSize();
                if (sz > 42)
                {
                    const int type = d[32];                          // 0..10 (cf. enum voice type)
                    if (type >= 0 && type <= 9)
                        valueTreeVoice.setProperty (IDs::VOICEMODE, type + 1, nullptr); // id comboMode
                    String name;
                    for (int i = 33; i < 43; ++i)
                        name += String::charToString ((juce_wchar) d[i]);
                    name = name.trimEnd();
                    valueTreeVoice.setProperty (IDs::VOICENAME, name, nullptr);

                    // Affiche la voix ouverte dans le titre de la fenêtre (visible sur tous les onglets).
                    if (auto* tlw = getTopLevelComponent())
                        tlw->setName ("Sysex77 v" + String (Sysex77::kVersion)
                                      + (name.isNotEmpty() ? "  —  " + name : String()));

                    // Charge aussi les paramètres sonores CONFIRMÉS hardware (opérateurs AFM,
                    // élément 1) en les rejouant comme des messages param-change entrants :
                    // chaque widget se met à jour via son propre chemin de réception (vérifié
                    // Track A). Notification SYNCHRONE par param, sinon les Value JUCE coalescent
                    // et seul le dernier paramètre serait appliqué.
                    // ANTI-ÉCHO : on rejoue le contenu de la voix comme des RX, mais on NE veut
                    // RIEN renvoyer au synthé (ce serait un flot de param-changes = re-écriture
                    // non sollicitée + risque de boucle). Suppression d'envoi sur tout le replay.
                    {
                        const ScopedEchoSuppress noEcho;
                        for (auto& vp : SyVoice::voiceBlobToParams (d, sz))
                        {
                            valueSysexIn = make_var_array (vp.group, vp.addrHi, vp.addrLo, vp.param, 0, vp.value);
                            valueSysexIn.getValueSource().sendChangeMessage (true);
                        }
                    }
                }

                // Bascule sur l'éditeur (onglet Voice = index 2) pour éditer la voix ouverte.
                tabs.setCurrentTabIndex (2);
            }
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
