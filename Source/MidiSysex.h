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
            // Envoi du nom de voix (ACTIF, non vérifié hardware : VNAM0..9 group 0x02 N2 0x01-0x0A) : oscMidiMessage contient les 10 messages VNAM0..VNAM9
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
        // GARDE SY77 : on n'envoie une banque qu'au SY77 (seule cible câblée). Une banque
        // d'un autre synthé est stockée/cherchable mais pas envoyée (éviterait de pousser des
        // octets étrangers au SY77). Envoi cross-synthé = chantier futur ([[hardware-safety]]).
        if (currentBankData.getSize() > 1
            && SyVoice::detectSynthKind ((const uint8*) currentBankData.getData(),
                                         currentBankData.getSize()) != SyVoice::SynthKind::SY77)
        {
            Logger::writeToLog ("Send BANK ignore : banque non-SY77");
        }
        else if (currentBankData.getSize() > 1)
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
                for (auto& b : blocks)   // device number of the app, not the one stored in the file
                    SyVoice::setBulkDevice ((uint8*) b.getData(), (int) b.getSize(), sysexDeviceNumber);
                BankSendProgressWindow w (std::move (blocks),
                                          [this] (const MidiMessage& m) { sendToOutputs (m); });
                w.runThread();
            }
        }
        else
            Logger::writeToLog ("Send BANK : aucune banque chargee");
    }
    if (address.matches (adresseOscRequestMulti) && message.size() == 1 && message[0].isInt32())
    {
        // Onglet Multi, clic utilisateur : demande au SY77 d'EMETTRE un Multi memorise (INT n).
        // Lecture seule cote synthe (aucune memoire ecrite).
        auto m = SyMulti::dumpRequest (sysexDeviceNumber, 0x00, jlimit (0, 15, message[0].getInt32()));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
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

            // GARDE SY77 : audition + ouverture éditeur réservées aux voix SY77. Une voix d'un
            // autre synthé n'est ni envoyée (octets étrangers) ni décodée par l'éditeur SY77.
            if (block.getSize() > 1
                && SyVoice::detectSynthKind ((const uint8*) block.getData(), block.getSize())
                     != SyVoice::SynthKind::SY77)
            {
                Logger::writeToLog ("SendVoice ignore : voix non-SY77");
            }
            else if (block.getSize() > 1)
            {
                openVoiceBlock (block);
            }
        }
    }
    //        rotaryKnob.setValue (jlimit (0.0f, 10.0f, message[0].getFloat32())); // [6]
}

/** Sends a voice block to the EDIT BUFFER and opens it in the editor (library audition,
    templates). The block is a copy: retargeted here, never written to a memory slot.
    Coque UI : le flux sysex (redirection $7F + device courant, garde de securite, settle,
    bulk, rejeu anti-echo, historique) est dans VoiceSession::loadBlock. */
void openVoiceBlock (MemoryBlock block)
{
    const auto r = session.loadBlock (std::move (block),
        [this] (int t, const String& n)
        {
            // Props AVANT le rejeu des params (meme ordre qu'avant le decoupage).
            if (t >= 0 && t <= 10)   // 10 = DRUM SET -> id 11 (vue drum)
            {
                for (int i = 0; i < tabs.getNumTabs(); ++i)   // le mode vient du bulk : pas d'ELMODE
                    if (auto* vp = dynamic_cast<VoicePage*> (tabs.getTabContentComponent (i)))
                        vp->prepareVoiceLoad (t + 1);
                valueTreeVoice.setProperty (IDs::VOICEMODE, t + 1, nullptr); // id comboMode
            }
            valueTreeVoice.setProperty (IDs::VOICEUNKNOWN, false, nullptr);
            valueTreeVoice.setProperty (IDs::VOICENAME, n, nullptr);   // peut etre vide (vraie voix)
            // Affiche la voix ouverte dans le titre de la fenetre (visible sur tous les onglets).
            if (auto* tlw = getTopLevelComponent())
                tlw->setName (Sysex77::windowTitle (n));
        });

    // SECURITE : bloc non redirigeable vers $7F -> rien envoye, rien modifie, pas de bascule.
    if (r == VoiceSession::LoadResult::Refused) return;

    if (r == VoiceSession::LoadResult::Discarded)
    {
        // Presentation seulement : le nom affiche n'est plus celui de l'edit buffer.
        // VOICEUNKNOWN + VOICENAME vide -> Voice.h vide le champ et affiche « (unknown voice) »
        // (dontSendNotification, aucun VNAM emis : seul Return dans le champ envoie le nom).
        // Distinct d'une vraie voix au nom vide (VOICEUNKNOWN false : champ vide sans texte).
        valueTreeVoice.setProperty (IDs::VOICEUNKNOWN, true, nullptr);
        valueTreeVoice.setProperty (IDs::VOICENAME, String(), nullptr);
        if (auto* tlw = getTopLevelComponent())
            tlw->setName (Sysex77::windowTitle ({}));
    }

    // Bascule sur l'editeur (onglet Voice = index 2) pour editer la voix ouverte.
    tabs.setCurrentTabIndex (2);
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
    // Helper commun avec VoiceSession (meme log, meme horodatage).
    sendRawSysex (sysexData, dataSize, Time::getMillisecondCounterHiRes(),
                  [this] (const MidiMessage& m) { sendToOutputs (m); });
}
