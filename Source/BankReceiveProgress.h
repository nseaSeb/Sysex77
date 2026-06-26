/*
 ==============================================================================

 BankReceiveProgress.h

 Fenêtre de progression modale pour la RÉCEPTION d'une banque (RECEIVE).

 Pacing REQUÊTE → RÉPONSE : on demande la voix `mem`, on ATTEND que le compteur
 de voix reçues augmente (réponse captée sur le thread MIDI -> arraySysex), PUIS
 on demande la suivante. Indispensable : envoyer les 64 dump requests en rafale
 sature le buffer d'entrée du SY77, qui n'en honore que quelques-unes (dump
 tronqué). Ici on s'adapte au débit réel du synthé.

 Par voix : si aucune réponse après `perVoiceTimeoutMs`, on passe à la suivante
 (slot muet) sans bloquer. runThread() renvoie false si l'utilisateur annule.

 ==============================================================================
 */

#pragma once
#include <JuceHeader.h>

struct BankReceiveProgressWindow : public juce::ThreadWithProgressWindow
{
    BankReceiveProgressWindow (int expectedVoices,
                               std::function<int()>      getCount,
                               std::function<void (int)> requestVoice,
                               int perVoiceTimeoutMs = 1000)
        : juce::ThreadWithProgressWindow ("Reception de la banque...", true, true),
          expected   (expectedVoices),
          count      (std::move (getCount)),
          request    (std::move (requestVoice)),
          perVoice   (perVoiceTimeoutMs)
    {
        setStatusMessage ("Demande au SY77...");
    }

    void run() override
    {
        for (int mem = 0; mem < expected; ++mem)
        {
            if (threadShouldExit())                 // bouton « Annuler »
                return;

            const int before = count();
            request (mem);                          // demande la voix `mem`

            // Attend la réponse (le compteur augmente) ou le timeout par voix.
            int waited = 0;
            while (! threadShouldExit() && count() <= before && waited < perVoice)
            {
                wait (20);
                waited += 20;
            }

            setProgress ((mem + 1) / (double) expected);
            setStatusMessage (juce::String (count()) + " / " + juce::String (expected)
                              + " voix recues...");
        }
    }

    int expected;
    std::function<int()>      count;
    std::function<void (int)> request;
    int perVoice;
};
