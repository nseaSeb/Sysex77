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
        if (onMessage != nullptr)
            onMessage (message);
    }
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
};
