/*
  ==============================================================================

    SysexBus.h
    Bus d'événements INTRA-PROCESSUS remplaçant l'ancien transport OSC sur
    127.0.0.1:9001.

    Pourquoi : auparavant chaque widget ouvrait un OSCSender vers 127.0.0.1:9001
    et MidiDemo écoutait ce port pour relayer le sysex vers le synthé. Utiliser la
    boucle réseau comme bus interne posait deux problèmes :
      1. Conflit de port / échec silencieux si l'app était lancée deux fois.
      2. SÉCURITÉ : n'importe quel autre process local pouvait envoyer sur 9001 et
         donc injecter du sysex arbitraire vers le synthé (risque d'effacement de
         patchs).

    Ce bus garde exactement le même format de message (juce::OSCMessage) et la même
    logique de dispatch (MidiDemo::oscMessageReceived), mais sans réseau. Le type
    `SysexBusSender` reproduit l'API d'OSCSender (connect / disconnect / send) afin
    que les sites d'appel existants compilent sans modification.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/** Point de rendez-vous unique entre les émetteurs et l'unique abonné (MidiDemo). */
struct SysexBus
{
    static SysexBus& get()
    {
        static SysexBus instance;
        return instance;
    }

    /** MidiDemo installe ici son gestionnaire (voir addOscListener dans MidiSysex.h). */
    std::function<void (const juce::OSCMessage&)> onMessage;

    void publish (const juce::OSCMessage& message)
    {
        // GARDE ANTI-ÉCHO (remplace l'ancien boolStopReceive global/racy) :
        // pendant l'APPLICATION d'un param-change REÇU (RX), tout envoi déclenché en
        // cascade par un widget est SUPPRIMÉ. Sans cela, certains chemins de réception
        // RÉ-ÉMETTENT (ex. ComboBox::valueChanged re-publie via setSelectedId() en
        // sendNotification ; cf. juce_ComboBox.cpp) -> le SY77 ré-échoie -> boucle A↔B.
        // Compteur ATOMIQUE et RÉENTRANT (pose/dépose en pile), posé uniquement autour
        // de l'application RX sur le thread message (cf. ScopedEchoSuppress).
        if (suppressSend.load (std::memory_order_acquire) > 0)
            return;
        if (onMessage != nullptr)
            onMessage (message);
    }

    /** Profondeur de suppression d'envoi (cf. ScopedEchoSuppress). 0 = envoi normal. */
    std::atomic<int> suppressSend { 0 };
};

/** Suppresseur d'envoi à portée (RAII). À poser autour de TOUTE application d'un
    message param-change reçu (assignation de valueSysexIn / replay de voice-load) :
    aucun envoi vers le synthé ne part tant qu'un de ces gardes est vivant. */
struct ScopedEchoSuppress
{
    ScopedEchoSuppress()  { SysexBus::get().suppressSend.fetch_add (1, std::memory_order_acq_rel); }
    ~ScopedEchoSuppress() { SysexBus::get().suppressSend.fetch_sub (1, std::memory_order_acq_rel); }
    JUCE_DECLARE_NON_COPYABLE (ScopedEchoSuppress)
};

/** Émetteur compatible avec l'API d'OSCSender utilisée dans le projet, mais qui
    publie sur le bus interne au lieu d'émettre un datagramme UDP. Remplacer
    `OSCSender` par `SysexBusSender` suffit : connect()/disconnect() deviennent des
    no-op et send(...) route le message en intra-processus. */
struct SysexBusSender
{
    bool connect (const juce::String&, int) { return true; }  // no-op (compatibilité API)
    bool disconnect()                       { return true; }  // no-op (compatibilité API)

    template <typename... Args>
    bool send (const juce::String& address, Args&&... args)
    {
        SysexBus::get().publish (juce::OSCMessage (juce::OSCAddressPattern (address),
                                                   std::forward<Args> (args)...));
        return true;
    }

    /** Publie un message param-change SY77 (9 octets) sur le bus /SYSEX.
        POINT UNIQUE de construction du format filaire côté widgets : au lieu de
        recopier `send(addr, b[0], b[1], ... b[8])` (9 arguments explicites) dans
        chaque widget Midi*, ceux-ci appellent ce helper. La sortie est OCTET-POUR-
        OCTET identique à l'ancien code : on émet exactement les 9 octets du tableau.
        NOTE device : l'octet [1] transporté ici est un PLACEHOLDER (0x10) — il est
        réécrit avec le device global au choke-point (MidiSysex.h::oscMessageReceived),
        seule autorité sur l'octet device. Sa valeur ici est donc sans effet sur le fil. */
    bool sendParam9 (const juce::String& address, const juce::uint8 b[9])
    {
        return send (address, (uint8) b[0], (uint8) b[1], (uint8) b[2],
                              (uint8) b[3], (uint8) b[4], (uint8) b[5],
                              (uint8) b[6], (uint8) b[7], (uint8) b[8]);
    }
};
