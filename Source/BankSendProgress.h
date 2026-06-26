/*
 ==============================================================================

 BankSendProgress.h

 Fenêtre de progression modale pour l'envoi d'une banque (jusqu'à 64 voix) au
 synthé. Le throttle inter-message (~10 ms, calqué sur l'outil RE, pour ne pas
 saturer le buffer d'entrée du SY77) tourne sur le THREAD D'ARRIÈRE-PLAN de
 ThreadWithProgressWindow : l'interface reste fluide, la barre de progression
 s'anime, et la fenêtre se referme toute seule à la fin (ou sur « Annuler »).

 Remplace l'envoi throttlé qui bouclait avec Thread::sleep SUR LE THREAD MESSAGE
 -> l'UI gelait ~0,6 s sans aucun retour visuel (cf. demande utilisateur :
 « envoi en aveugle »).

 Note threading : la fonction d'envoi fournie (sendOne) appelle
 MidiDemo::sendToOutputs depuis le thread d'arrière-plan. MidiOutput::send-
 MessageNow est appelable hors thread message ; le seul cas limite est un
 débranchement à chaud PENDANT l'envoi (~0,6 s), assumé négligeable pour une
 action volontaire et ponctuelle.

 ==============================================================================
 */

#pragma once
#include <JuceHeader.h>

struct BankSendProgressWindow : public juce::ThreadWithProgressWindow
{
    // baseDelayMs = marge de TRAITEMENT ajoutée, par message, au temps de transmission
    // calculé d'après la taille (cf. run()). Augmenter si le SY77 sature encore.
    BankSendProgressWindow (juce::Array<juce::MemoryBlock> blocksToSend,
                            std::function<void (const juce::MidiMessage&)> sendOne,
                            int baseDelayMs = 60)
        : juce::ThreadWithProgressWindow ("Envoi de la banque au synthe", true, true),
          blocks   (std::move (blocksToSend)),
          send     (std::move (sendOne)),
          baseDelay (baseDelayMs)
    {
        setStatusMessage ("Envoi de " + juce::String (blocks.size()) + " voix...");
    }

    void run() override
    {
        const int n = blocks.size();
        for (int i = 0; i < n; ++i)
        {
            if (threadShouldExit())                 // bouton « Annuler »
                break;

            setProgress ((i + 1) / (double) n);
            setStatusMessage ("Voix " + juce::String (i + 1) + " / " + juce::String (n));

            auto& b = blocks.getReference (i);
            juce::MidiMessage m (b.getData(), (int) b.getSize());
            m.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
            send (m);

            // Throttle = temps de TRANSMISSION du message (MIDI 31250 bauds = ~10 bits/octet
            // -> ~0,32 ms/octet, soit taille/3 ms) + marge de traitement. Sans ça, sendMessageNow
            // met tout en file et le driver stream en continu : le SY77 reçoit plus vite qu'il
            // n'écrit chaque voix -> débordement du buffer d'entrée (voix perdues).
            const int wireMs = (int) (b.getSize() / 3);
            wait (wireMs + baseDelay);
        }
    }

    juce::Array<juce::MemoryBlock> blocks;
    std::function<void (const juce::MidiMessage&)> send;
    int baseDelay;
};
