// Tests unitaires : EchoLoop (inclus par Source/Tests.h, ordre d'enregistrement conserve).
#pragma once

//==============================================================================
/** Boucle d'écho éditeur<->synthé : le garde anti-écho doit empêcher TOUT envoi
    sur le bus pendant l'application d'un message param-change reçu (RX).
    On simule un widget qui RÉ-ÉMET en réaction à la réception : avec le garde,
    aucun message ne doit atteindre l'abonné du bus -> la boucle est coupée. */
struct EchoLoopTests : public juce::UnitTest
{
    EchoLoopTests() : juce::UnitTest ("EchoLoop") {}

    void runTest() override
    {
        auto& bus = SysexBus::get();
        auto savedHandler = bus.onMessage;     // restauré en fin de test (abonné réel = MidiDemo)
        const int savedDepth = bus.suppressSend.load();

        int published = 0;
        bus.onMessage = [&published] (const juce::OSCMessage&) { ++published; };
        bus.suppressSend.store (0);

        SysexBusSender sender;
        juce::uint8 b[9] = { 0x43, 0x10, 0x34, 0x56, 0x00, 0x00, 0x19, 0x00, 0x00 };

        beginTest ("send passes through when not suppressed");
        {
            published = 0;
            sender.sendParam9 ("/SYSEX", b);
            expectEquals (published, 1);
        }

        beginTest ("a received param applied under the guard re-emits NOTHING");
        {
            published = 0;
            {
                // Simule l'application d'un message RX : un widget ré-émet en cascade.
                const ScopedEchoSuppress noEcho;
                sender.sendParam9 ("/SYSEX", b);   // ré-émission widget -> doit être supprimée
                sender.sendParam9 ("/SYSEX", b);
            }
            expectEquals (published, 0);           // AUCUN renvoi vers le synthé -> pas d'écho
        }

        beginTest ("guard is reentrant (nested), send resumes only at depth 0");
        {
            published = 0;
            {
                const ScopedEchoSuppress a;
                {
                    const ScopedEchoSuppress nested;
                    sender.sendParam9 ("/SYSEX", b);
                }
                sender.sendParam9 ("/SYSEX", b);   // toujours sous le garde externe -> supprimé
                expectEquals (published, 0);
            }
            sender.sendParam9 ("/SYSEX", b);       // garde dépilé -> envoi rétabli
            expectEquals (published, 1);
        }

        bus.onMessage = savedHandler;              // ne pas laisser un abonné parasite
        bus.suppressSend.store (savedDepth);
    }
};

static EchoLoopTests echoLoopTests;
