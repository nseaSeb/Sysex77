/*
  ==============================================================================

    Tests.h
    Tests unitaires JUCE (juce::UnitTest) pour la logique « pure » de Sysex77.
    Lancés via l'argument de ligne de commande --test (voir Main.cpp).

  ==============================================================================
*/

#pragma once
#include "SysexUtils.h"
#include "SysexBus.h"

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

//==============================================================================
struct SysexUtilsTests : public juce::UnitTest
{
    SysexUtilsTests() : juce::UnitTest ("SysexUtils") {}

    // Construit un faux dump : pour chaque voix, 0xF0 puis 32 octets, puis 10 chars de nom.
    static void appendFakeVoice (juce::MemoryBlock& mb, const char* name10)
    {
        juce::uint8 header[33] = { 0xF0 }; // 0xF0 + 32 octets à zéro -> nom à l'offset 33
        mb.append (header, sizeof (header));
        mb.append (name10, 10);
    }

    void runTest() override
    {
        beginTest ("extractVoiceNames reads names at offset 33");
        {
            juce::MemoryBlock mb;
            appendFakeVoice (mb, "GrandPiano");
            appendFakeVoice (mb, "Arianne   ");

            auto names = SyVoice::extractVoiceNames ((const juce::uint8*) mb.getData(), mb.getSize());
            expectEquals (names.size(), 2);
            expectEquals (names[0], juce::String ("GrandPiano"));
            expectEquals (names[1].trim(), juce::String ("Arianne"));
        }

        beginTest ("extractVoiceNames truncated buffer is bounds-safe");
        {
            const juce::uint8 truncated[] = { 0xF0, 0x43, 0x10 }; // 0xF0 mais pas la place du nom
            auto names = SyVoice::extractVoiceNames (truncated, sizeof (truncated));
            expect (names.isEmpty());
        }

        beginTest ("extractVoiceNames null input");
        {
            expect (SyVoice::extractVoiceNames (nullptr, 0).isEmpty());
        }

        beginTest ("getVoiceBlock extracts the Nth F0..F7 block");
        {
            const juce::uint8 syx[] = {
                0xF0, 0x43, 0x10, 0x01, 0xF7,       // voix 0 (5 octets)
                0xF0, 0x43, 0x10, 0x02, 0x03, 0xF7  // voix 1 (6 octets)
            };
            auto b0 = SyVoice::getVoiceBlock (syx, sizeof (syx), 0);
            expectEquals ((int) b0.getSize(), 5);
            expectEquals ((int) ((const juce::uint8*) b0.getData())[0], 0xF0);
            expectEquals ((int) ((const juce::uint8*) b0.getData())[4], 0xF7);

            auto b1 = SyVoice::getVoiceBlock (syx, sizeof (syx), 1);
            expectEquals ((int) b1.getSize(), 6);
            expectEquals ((int) ((const juce::uint8*) b1.getData())[5], 0xF7);

            expect (SyVoice::getVoiceBlock (syx, sizeof (syx), 9).getSize() == 0);
        }

        beginTest ("yamahaChecksum sum+checksum mod 128 == 0");
        {
            const juce::uint8 data[] = { 0x12, 0x34, 0x7F, 0x01, 0x40 };
            auto ck = SyVoice::yamahaChecksum (data, (int) sizeof (data));
            int sum = ck;
            for (auto b : data) sum += b;
            expectEquals (sum & 0x7F, 0);
            expect (ck <= 0x7F);
        }

        beginTest ("verifyYamahaBulkChecksum accepts a well-formed bulk block");
        {
            // Construit un bulk minimal F0 43 0n 7A cMSB cLSB <data> <ck> F7 dont le checksum
            // est calculé par yamahaChecksum sur la zone data (index 6..ck-1).
            juce::uint8 blk[] = { 0xF0, 0x43, 0x10, 0x7A, 0x00, 0x04,
                                  'L', 'M', ' ', ' ',          // 4 octets de data factices
                                  0x00,                        // emplacement du checksum
                                  0xF7 };
            const int dataStart = 6, ckIndex = (int) sizeof (blk) - 2;
            blk[ckIndex] = SyVoice::yamahaChecksum (blk + dataStart, ckIndex - dataStart);
            expect (SyVoice::verifyYamahaBulkChecksum (blk, (int) sizeof (blk)));

            // Un octet de data corrompu -> le checksum ne correspond plus -> rejet.
            juce::uint8 bad[sizeof (blk)];
            memcpy (bad, blk, sizeof (blk));
            bad[dataStart] ^= 0x01;
            expect (! SyVoice::verifyYamahaBulkChecksum (bad, (int) sizeof (bad)));
        }

        beginTest ("verifyYamahaBulkChecksum rejects non-bulk / malformed blocks");
        {
            // Pas F0…F7
            const juce::uint8 noFrame[] = { 0x43, 0x10, 0x7A, 0x00, 0x04, 0x00, 0x00, 0x00 };
            expect (! SyVoice::verifyYamahaBulkChecksum (noFrame, (int) sizeof (noFrame)));
            // Pas Yamaha (octet [1] != 0x43)
            const juce::uint8 notYam[] = { 0xF0, 0x7E, 0x10, 0x7A, 0x00, 0x04, 0x00, 0x00, 0xF7 };
            expect (! SyVoice::verifyYamahaBulkChecksum (notYam, (int) sizeof (notYam)));
            // Message paramétrique court (0x34) : pas un bulk -> false (non concerné)
            const juce::uint8 paramMsg[] = { 0xF0, 0x43, 0x10, 0x34, 0x0f, 0x00, 0x00, 0x2d, 0x00, 0x05, 0xF7 };
            expect (! SyVoice::verifyYamahaBulkChecksum (paramMsg, (int) sizeof (paramMsg)));
            // Trop court / nullptr
            expect (! SyVoice::verifyYamahaBulkChecksum (nullptr, 0));
        }

        beginTest ("splitSysexMessages splits successive F0..F7 blocks");
        {
            const juce::uint8 syx[] = {
                0xF0, 0x43, 0x10, 0x01, 0xF7,        // bloc 0 (5 octets)
                0xF0, 0x43, 0x10, 0x02, 0x03, 0xF7   // bloc 1 (6 octets)
            };
            auto blocks = SyVoice::splitSysexMessages (syx, sizeof (syx));
            expectEquals (blocks.size(), 2);
            expectEquals ((int) blocks[0].getSize(), 5);
            expectEquals ((int) ((const juce::uint8*) blocks[0].getData())[0], 0xF0);
            expectEquals ((int) ((const juce::uint8*) blocks[0].getData())[4], 0xF7);
            expectEquals ((int) blocks[1].getSize(), 6);
            expectEquals ((int) ((const juce::uint8*) blocks[1].getData())[5], 0xF7);

            // Buffer non terminé (dernier bloc sans F7) : borné, pas de débordement.
            const juce::uint8 trunc[] = { 0xF0, 0x43, 0x10, 0xF7, 0xF0, 0x43, 0x10 };
            auto t = SyVoice::splitSysexMessages (trunc, sizeof (trunc));
            expectEquals (t.size(), 2);
            expectEquals ((int) t[0].getSize(), 4);     // F0 43 10 F7
            expectEquals ((int) t[1].getSize(), 3);     // F0 43 10 (non terminé)
            expect (SyVoice::splitSysexMessages (nullptr, 0).isEmpty());
        }

        beginTest ("voiceDumpRequest is a well-framed VCED request");
        {
            auto m = SyVoice::voiceDumpRequest (3, 0x00, 0x05);   // device 3, Internal, A6
            const juce::uint8* d = m.getSysExData();
            const int sz = m.getSysExDataSize();                  // sans F0/F7
            expectEquals (sz, 29);                                // 31 octets - F0 - F7
            expectEquals ((int) d[0], 0x43);                      // Yamaha
            expectEquals ((int) d[1], 0x22);                      // 0x20 | (3-1) -> dump request, dev 3
            expectEquals ((int) d[2], 0x7A);                      // format Voice Bulk Dump
            // Identifiant VCED "LM  8101VC" (octets 3..12 du corps = 4..13 avec F0)
            const char* id = "LM  8101VC";
            for (int i = 0; i < 10; ++i)
                expectEquals ((int) d[3 + i], (int) (juce::uint8) id[i]);
            expectEquals ((int) d[sz - 2], 0x00);                 // Memory_type = Internal
            expectEquals ((int) d[sz - 1], 0x05);                 // Memory# = A6
            // dumpRequestDeviceByte : nibble haut 0x2, borné
            expectEquals ((int) SyVoice::dumpRequestDeviceByte (1),  0x20);
            expectEquals ((int) SyVoice::dumpRequestDeviceByte (16), 0x2F);
        }

        beginTest ("diffVoiceBlocks reports differing byte offsets");
        {
            const juce::uint8 a[] = { 0xF0, 0x43, 0x10, 0x20, 0x30, 0x40, 0xF7 };
            const juce::uint8 b[] = { 0xF0, 0x43, 0x10, 0x25, 0x30, 0x44, 0xF7 };
            auto d = SyVoice::diffVoiceBlocks (a, sizeof (a), b, sizeof (b));
            expectEquals (d.size(), 2);
            expectEquals (d[0].offset, 3);
            expectEquals ((int) d[0].before, 0x20);
            expectEquals ((int) d[0].after,  0x25);
            expectEquals (d[1].offset, 5);

            // blocs identiques -> aucune différence
            expect (SyVoice::diffVoiceBlocks (a, sizeof (a), a, sizeof (a)).isEmpty());
            // longueurs différentes : compare sur la plus courte, pas de débordement
            expect (SyVoice::diffVoiceBlocks (a, sizeof (a), b, 3).isEmpty());
        }

        beginTest ("parseParam9 valid SY77 message");
        {
            const juce::uint8 msg[] = { 0x43, 0x10, 0x34, 0x0f, 0x00, 0x00, 0x2d, 0x00, 0x05 };
            SyVoice::Param9 p;
            expect (SyVoice::parseParam9 (msg, 9, p));
            expectEquals ((int) p.param,  0x2d);
            expectEquals ((int) p.dataLo, 5);
            expectEquals ((int) p.channel, 0x10);
        }

        beginTest ("parseParam9 rejects bad size or maker");
        {
            SyVoice::Param9 p;
            const juce::uint8 wrongMaker[] = { 0x00, 0x10, 0x34, 0,0,0,0,0,0 };
            expect (! SyVoice::parseParam9 (wrongMaker, 9, p));
            const juce::uint8 wrongSize[]  = { 0x43, 0x10, 0x34 };
            expect (! SyVoice::parseParam9 (wrongSize, 3, p));
        }

        beginTest ("deviceByte encodes 0x1n and clamps to 1..16");
        {
            expectEquals ((int) SyVoice::deviceByte (1),  0x10);
            expectEquals ((int) SyVoice::deviceByte (4),  0x13);
            expectEquals ((int) SyVoice::deviceByte (16), 0x1F);
            expectEquals ((int) SyVoice::deviceByte (0),  0x10); // borné en bas
            expectEquals ((int) SyVoice::deviceByte (99), 0x1F); // borné en haut
        }

        beginTest ("deviceNumberFromByte decodes 0x1n, -1 otherwise");
        {
            expectEquals (SyVoice::deviceNumberFromByte (0x10), 1);
            expectEquals (SyVoice::deviceNumberFromByte (0x1F), 16);
            expectEquals (SyVoice::deviceNumberFromByte (0x20), -1);
            expectEquals (SyVoice::deviceNumberFromByte (0x00), -1);
        }

        beginTest ("acceptsDevice honours selection and omni");
        {
            expect (  SyVoice::acceptsDevice (0x13, 4, false));   // device 4 attendu, reçu 4
            expect (! SyVoice::acceptsDevice (0x13, 5, false));   // device 5 attendu, reçu 4
            expect (  SyVoice::acceptsDevice (0x13, 5, true));    // omni : accepte
            expect (! SyVoice::acceptsDevice (0x25, 5, true));    // pas un 0x1n -> refusé
        }

        beginTest ("paramBytes builds the 9-byte SY77 message");
        {
            auto b = SyVoice::paramBytes (1, 0x02, 0x00, 0x00, 0x3F, 0x40); // volume voix
            const juce::uint8 expected[9] = { 0x43, 0x10, 0x34, 0x02, 0x00, 0x00, 0x3F, 0x00, 0x40 };
            for (int i = 0; i < 9; ++i)
                expectEquals ((int) b[(size_t) i], (int) expected[i]);

            // le device se reflète dans l'octet [1]
            expectEquals ((int) SyVoice::paramBytes (5, 0, 0, 0, 0, 0)[1], 0x14);
        }

        beginTest ("paramBytes is byte-identical to the legacy inline format (anti-regression)");
        {
            // Preuve de non-régression pour la TÂCHE 1.3 : les ~205 widgets Midi* et les
            // littéraux de MidiDemo/MidiSysex construisaient à la main le tableau
            //   sysexData[9] = { 0x43, 0x10, 0x34, group, addrHi, addrLo, param, 0x00, value }
            // (octet [1] = device placeholder 0x10). On les a unifiés sur SyVoice::paramBytes.
            // Ce test reconstruit l'ANCIEN format inline EN DUR et le compare octet-pour-octet
            // au builder, pour un échantillon de (group, addrHi, addrLo, param, value).
            //
            // Sur le fil, l'octet device [1] est de toute façon réécrit par le choke-point
            // (MidiSysex.h : sysexdata[1] = deviceByte(sysexDeviceNumber)), donc placeholder
            // sans effet ; ici on passe device=1 -> 0x10, ce qui reproduit l'ancien littéral
            // 0x10 et permet une comparaison des 9 octets, [1] compris.
            const struct { juce::uint8 group, addrHi, addrLo, param, value; } cases[] =
            {
                { 0x03, 0x00, 0x00, 0x08, 0x40 },   // voice group / élément 1
                { 0x03, 0x60, 0x00, 0x02, 0x10 },   // pitch élément 4 (addrHi=0x60)
                { 0x09, 0x20, 0x00, 0x01, 0x7F },   // cutoff filtre, élément 2
                { 0x06, 0x00, 0x00, 0x00, 0x00 },   // op AFM (group 0x06)
                { 0x56, 0x00, 0x00, 0x05, 0x2A },   // op AFM OP1 (group 0x56)
                { 0x0f, 0x00, 0x00, 0x2d, 0x03 },   // System Setup (foot / bulk-protect)
                { 0x02, 0x00, 0x00, 0x3f, 0x64 },   // total voice volume
            };

            for (auto& c : cases)
            {
                // ANCIEN format, écrit à la main (device placeholder 0x10 == deviceByte(1)).
                const juce::uint8 legacy[9] =
                    { 0x43, 0x10, 0x34, c.group, c.addrHi, c.addrLo, c.param, 0x00, c.value };

                auto built = SyVoice::paramBytes (1, c.group, c.addrHi, c.addrLo, c.param, c.value);
                for (int i = 0; i < 9; ++i)
                    expectEquals ((int) built[(size_t) i], (int) legacy[i]);
            }
        }

        beginTest ("looksLikeYamahaSysex detects F0 43 header");
        {
            const juce::uint8 yam[] = { 0xF0, 0x43, 0x00, 0x12 };
            const juce::uint8 other[] = { 0x00, 0x43, 0x00 };
            expect (  SyVoice::looksLikeYamahaSysex (yam, sizeof (yam)));
            expect (! SyVoice::looksLikeYamahaSysex (other, sizeof (other)));
            expect (! SyVoice::looksLikeYamahaSysex (nullptr, 0));
        }

        beginTest ("elementAddrHi and afmOperatorGroup encodings");
        {
            expectEquals ((int) SyVoice::elementAddrHi (0), 0x00);
            expectEquals ((int) SyVoice::elementAddrHi (1), 0x20);
            expectEquals ((int) SyVoice::elementAddrHi (2), 0x40);
            expectEquals ((int) SyVoice::elementAddrHi (3), 0x60);
            // OP6..OP1 = 0x06,0x16,0x26,0x36,0x46,0x56
            expectEquals ((int) SyVoice::afmOperatorGroup (0), 0x06);
            expectEquals ((int) SyVoice::afmOperatorGroup (4), 0x46);
            expectEquals ((int) SyVoice::afmOperatorGroup (5), 0x56);
        }

        beginTest ("egGroupFor locks EG send-side group bytes (anti-regression)");
        {
            // Filet anti-régression du bug « group-byte 0x00 » (cf. [[project-eg-sysex-bug]]).
            // Les 4 éditeurs d'EG (WaveEg/PitchEG/Filter1/Filter2) CONSOMMENT egGroupFor pour
            // l'octet [3] (group) de leur message param-change. Ce test verrouille la valeur
            // attendue : si quelqu'un repasse un group à 0x00 (ou une valeur fausse), il casse.
            using EG = SyVoice::EgKind;
            expectEquals ((int) SyVoice::egGroupFor (EG::filter),       0x09);  // Filtre 1 & 2
            expectEquals ((int) SyVoice::egGroupFor (EG::pitch),        0x05);  // AFM pitch-EG
            expectEquals ((int) SyVoice::egGroupFor (EG::awmAmplitude), 0x07);  // AWM amp-EG

            // AFM amp-EG = PAR OPÉRATEUR : group = (op<<4)|6, op 0..5 = OP6..OP1.
            const int expectedOp[6] = { 0x06, 0x16, 0x26, 0x36, 0x46, 0x56 };
            for (int op = 0; op < 6; ++op)
                expectEquals ((int) SyVoice::egGroupFor (EG::afmAmplitude, op), expectedOp[op]);
            // cohérent avec afmOperatorGroup (même source).
            for (int op = 0; op < 6; ++op)
                expectEquals ((int) SyVoice::egGroupFor (EG::afmAmplitude, op),
                              (int) SyVoice::afmOperatorGroup (op));

            // INVARIANT CLÉ : aucun type d'EG ne doit JAMAIS renvoyer 0x00 (le group du bug).
            for (auto k : { EG::filter, EG::pitch, EG::afmAmplitude, EG::awmAmplitude })
                expect (SyVoice::egGroupFor (k, 0) != 0x00);
        }

        beginTest ("egLevelToDisplay/egLevelToWire — offset-binary (o/b) inversible + valeurs lua");
        {
            // Inversibilité sur TOUTE la plage filaire 0..127 (round-trip wire->display->wire).
            for (int w = 0; w <= 127; ++w)
            {
                const int disp = SyVoice::egLevelToDisplay ((juce::uint8) w);
                expectEquals ((int) SyVoice::egLevelToWire (disp), w);
                expect (disp >= -64 && disp <= 63);   // plage d'affichage TG77_Voice.json
            }
            // Inversibilité sur la plage d'affichage -64..+63 (display->wire->display).
            for (int dpy = -64; dpy <= 63; ++dpy)
                expectEquals (SyVoice::egLevelToDisplay (SyVoice::egLevelToWire (dpy)), dpy);

            // Valeurs ANCRES dérivées du codec lua/json (offset 64 ; centre par défaut = wire 64).
            //   main.lua l.22 : « (o/b) offset binary -64..+63 -> E1 msg 0..127 passes through »
            //   TG77_Voice.json PEG/FEG L : display{-64,63} message{0,127} defaultValue 64.
            expectEquals (SyVoice::egLevelToDisplay (64), 0);     // centre
            expectEquals (SyVoice::egLevelToDisplay (0), -64);    // minimum
            expectEquals (SyVoice::egLevelToDisplay (127), 63);   // maximum
            expectEquals ((int) SyVoice::egLevelToWire (0), 64);  // display 0 -> wire 64 (defaultValue)
            expectEquals ((int) SyVoice::egLevelToWire (-64), 0);
            expectEquals ((int) SyVoice::egLevelToWire (63), 127);
            // Bornage : un display hors plage ne déborde jamais l'octet filaire 7 bits.
            expectEquals ((int) SyVoice::egLevelToWire (200), 127);
            expectEquals ((int) SyVoice::egLevelToWire (-200), 0);
        }

        beginTest ("fpdDetuneToWire/Display — détune s/m inversible + valeurs lua (half=15 signbit=16)");
        {
            // Inversibilité sur la plage d'affichage -15..+15.
            for (int d = -15; d <= 15; ++d)
                expectEquals (SyVoice::fpdDetuneToDisplay (SyVoice::fpdDetuneToWire (d)), d);

            // Valeurs ANCRES (main.lua l.45 : SM[op*1000+0x1A] = {15,16} ; OCR l.438 « FPD -15~+15 s/m »).
            expectEquals ((int) SyVoice::fpdDetuneToWire (0),   0);    // 0 -> 0
            expectEquals ((int) SyVoice::fpdDetuneToWire (15),  15);   // +max -> 15
            expectEquals ((int) SyVoice::fpdDetuneToWire (-1),  17);   // signbit 16 | 1
            expectEquals ((int) SyVoice::fpdDetuneToWire (-15), 31);   // signbit 16 | 15
            // L'octet filaire 29/30 vu dans la trace SY77 = display -13/-14 (16|13, 16|14).
            expectEquals (SyVoice::fpdDetuneToDisplay (29), -13);
            expectEquals (SyVoice::fpdDetuneToDisplay (30), -14);
            expectEquals (SyVoice::fpdDetuneToDisplay (15),  15);
            expectEquals (SyVoice::fpdDetuneToDisplay (0),    0);
            // Concorde EXACTEMENT avec le chemin boolNegative de MidiSlider (intNegativeDelta=17, ~v) :
            //   TX d<0 : (~d)+17 == 16|(-d). Vérifie l'équivalence pour toute la plage négative.
            for (int d = -15; d <= -1; ++d)
                expectEquals ((int) SyVoice::fpdDetuneToWire (d), (~d) + 17);
            // Bornage.
            expectEquals ((int) SyVoice::fpdDetuneToWire (100),  15);
            expectEquals ((int) SyVoice::fpdDetuneToWire (-100), 31);
        }

        beginTest ("voiceBlobToParams decodes confirmed AFM operator params (SteelStrng)");
        {
            // Dump réel de la voix « SteelStrng » (1AFM, type@32 == 0x03), F0..F7 (466 o.).
            // Sert d'oracle : valeurs confirmées hardware (TL 6/6, Fine 6/6, R/L/RR/RL/L0).
            static const juce::uint8 steel[] = {
            0xf0, 0x43, 0x00, 0x7a, 0x03, 0x4a, 0x4c, 0x4d, 0x20, 0x20, 0x38, 0x31, 0x30, 0x31, 0x56, 0x43,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x1b,
            0x03, 0x53, 0x74, 0x65, 0x65, 0x6c, 0x53, 0x74, 0x72, 0x6e, 0x67, 0x03, 0x04, 0x05, 0x1c, 0x33,
            0x0e, 0x10, 0x3c, 0x04, 0x2e, 0x09, 0x20, 0x37, 0x10, 0x29, 0x04, 0x1b, 0x32, 0x0d, 0x0e, 0x32,
            0x16, 0x2d, 0x08, 0x1f, 0x26, 0x11, 0x01, 0x01, 0x02, 0x00, 0x01, 0x28, 0x00, 0x00, 0x79, 0x0e,
            0x00, 0x00, 0x0d, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x3c,
            0x00, 0x00, 0x01, 0x0f, 0x18, 0x00, 0x7f, 0x01, 0x7f, 0x45, 0x06, 0x18, 0x3f, 0x1b, 0x31, 0x00,
            0x3f, 0x20, 0x1a, 0x0e, 0x0a, 0x07, 0x00, 0x03, 0x3f, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
            0x05, 0x36, 0x00, 0x0c, 0x0c, 0x00, 0x01, 0x15, 0x55, 0x00, 0x1f, 0x3e, 0x5d, 0x01, 0x00, 0x01,
            0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x0f, 0x00, 0x18, 0x3f, 0x1b, 0x31, 0x00, 0x3f, 0x20, 0x1a,
            0x0e, 0x0a, 0x07, 0x00, 0x03, 0x3f, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x12, 0x3f, 0x00,
            0x0f, 0x0d, 0x00, 0x00, 0x03, 0x36, 0x00, 0x1f, 0x3e, 0x5d, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
            0x01, 0x00, 0x01, 0x04, 0x2e, 0x18, 0x3f, 0x1b, 0x31, 0x00, 0x3f, 0x20, 0x1a, 0x0e, 0x0a, 0x07,
            0x00, 0x03, 0x3f, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x13, 0x36, 0x00, 0x0c, 0x0c, 0x01,
            0x00, 0x0e, 0x3c, 0x00, 0x1f, 0x3e, 0x5d, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
            0x0b, 0x00, 0x18, 0x3f, 0x1b, 0x31, 0x00, 0x3f, 0x20, 0x1a, 0x0e, 0x0a, 0x07, 0x00, 0x03, 0x3f,
            0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x01, 0x10, 0x3f, 0x00, 0x0f, 0x01, 0x00, 0x00, 0x00, 0x1e,
            0x00, 0x1f, 0x3e, 0x5d, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x04, 0x2e, 0x18,
            0x1d, 0x1b, 0x31, 0x00, 0x3f, 0x20, 0x1a, 0x0e, 0x0a, 0x07, 0x00, 0x03, 0x3f, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x01, 0x11, 0x04, 0x36, 0x01, 0x0c, 0x00, 0x01, 0x00, 0x00, 0x30, 0x00, 0x1f, 0x3e,
            0x5d, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x05, 0x00, 0x18, 0x3f, 0x1b, 0x31,
            0x00, 0x3f, 0x20, 0x1a, 0x0e, 0x0a, 0x07, 0x00, 0x03, 0x3f, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00,
            0x00, 0x14, 0x3f, 0x01, 0x0f, 0x01, 0x01, 0x1e, 0x00, 0x5b, 0x00, 0x1f, 0x3e, 0x5d, 0x01, 0x00,
            0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x2e, 0x17, 0x00, 0x10, 0x0d, 0x12, 0x37, 0x0e,
            0x22, 0x27, 0x15, 0x01, 0x00, 0x00, 0x40, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46,
            0x00, 0x0f, 0x04, 0x02, 0x1b, 0x02, 0x0b, 0x00, 0x14, 0x2d, 0x00, 0x00, 0x40, 0x2d, 0x25, 0x20,
            0x19, 0x14, 0x22, 0x06, 0x00, 0x30, 0x54, 0x7f, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
            0x01, 0x32, 0x02, 0x0a, 0x0c, 0x00, 0x3f, 0x00, 0x00, 0x1c, 0x36, 0x17, 0x36, 0x1c, 0x2c, 0x2c,
            0x01, 0x00, 0x30, 0x54, 0x7f, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x3b, 0x0f, 0x0d,
            0x41, 0xf7
            };

            auto params = SyVoice::voiceBlobToParams (steel, (int) sizeof (steel));
            // 6 op×29 + (ELVL,ALGNUM,FCTOF1,FCTOF2,FFRES,FYPSW) + 4 pitch-rates + 5 pitch-levels
            // (FPL0-3+FPRL1) + 2×7 filtre-rates + 2×7 filtre-niveaux + VVOL.
            expectEquals (params.size(), 6 * 29 + 6 + 4 + 5 + 14 + 14 + 1);

            auto val = [&params] (int group, int param) -> int
            {
                for (auto& p : params)
                    if (p.group == group && p.param == param && p.addrHi == 0 && p.addrLo == 0)
                        return p.value;
                return -1;
            };

            // TL (param 0x1B) — confirmé 6/6 contre le dump (OP1=0x56 … OP6=0x06).
            expectEquals (val (0x56, 0x1B), 91);   // OP1 (offset 361)
            expectEquals (val (0x46, 0x1B), 48);   // OP2 (offset 316)
            expectEquals (val (0x06, 0x1B), 85);   // OP6 (offset 136)
            // Coarse (FPC, param 0x25, interne 43) + Waveform (PWAVE, param 0x17, interne 24), OP1.
            expectEquals (val (0x56, 0x25), 1);    // coarse OP1 (offset 375)
            expectEquals (val (0x56, 0x17), 15);   // waveform OP1
            expectEquals (val (0x56, 0x26), 46);   // fine OP1 (offset 376)
            // R1 (param 0x00) OP1 = octet @332.
            expectEquals (val (0x56, 0x00), 24);
            // ALGNUM (group 0x05, param 0x00) = octet @377 (confirmé par diff single-param).
            expectEquals (val (0x05, 0x00), 23);
            // Cutoff filtre 1 (group 0x09, addrHi 0, param 0x01) = octet @404 (confirmé par diff).
            expectEquals (val (0x09, 0x01), 27);
            // Autres params à offset absolu (offsets Table 2 / spot-check SteelStrng).
            expectEquals (val (0x02, 0x3F), 60);   // VVOL  @95  (volume de voix)
            expectEquals (val (0x03, 0x00), 1);    // ELVL0 @98  (niveau élément 1)
            // cutoff filtre 2 (addrHi 1) et résonance (addrHi 2) : helper val() ne filtre que
            // addrHi==0, donc on les lit directement dans la liste.
            auto valH = [&params] (int g, int h, int p) -> int {
                for (auto& q : params)
                    if (q.group == g && q.addrHi == h && q.param == p) return q.value;
                return -1; };
            expectEquals (valH (0x09, 1, 0x01), 50);   // FCTOF2 @433 (cutoff filtre 2)
            expectEquals (valH (0x09, 2, 0x32), 59);   // FFRES  @461 (résonance)
            // Pitch-EG rates (group 0x05) + filtre 1 mode/rate (group 0x09) : bloc N2-ordonné.
            expectEquals (val (0x05, 0x02), 16);   // FPR2  @379
            expectEquals (val (0x05, 0x03), 13);   // FPR3  @380
            expectEquals (val (0x09, 0x02), 2);    // FMODE filtre 1 @405
            expectEquals (val (0x09, 0x03), 11);   // FR1   filtre 1 @406

            // NIVEAUX D'EG (o/b) — chargés en octet filaire (0..127). Valeurs lues directement
            // dans le dump SteelStrng (cf. recoupement codec lua : passthrough wire == octet).
            // Pitch-EG levels FPL0-3 (group 0x05, N2 0x05-0x08) = octets @382-385.
            expectEquals (val (0x05, 0x05), 55);   // FPL0  @382 -> display egLevelToDisplay = -9
            expectEquals (val (0x05, 0x06), 14);   // FPL1  @383 -> display -50
            expectEquals (val (0x05, 0x07), 34);   // FPL2  @384 -> display -30
            expectEquals (val (0x05, 0x08), 39);   // FPL3  @385 -> display -25
            // Filtre-1 EG levels FL0-4 (group 0x09, addrHi 0, N2 0x09-0x0D) = octets @412-416.
            expectEquals (val (0x09, 0x09), 64);   // FL0 @412 == CENTRE o/b par défaut -> display 0
            expectEquals (val (0x09, 0x0A), 45);   // FL1 @413 -> display -19
            expectEquals (val (0x09, 0x0D), 25);   // FL4 @416 -> display -39
            expectEquals (val (0x09, 0x0E), 20);   // FRL1 @417 -> display -44
            // Filtre-2 EG level FL0 (group 0x09, addrHi 1, N2 0x09) = octet @441.
            expectEquals (valH (0x09, 1, 0x09), 28);   // FL0 filtre 2 @441 -> display -36
            // Tous les niveaux d'EG chargés DOIVENT être dans la plage filaire valide 0..127.
            for (auto& q : params)
                if (q.group == 0x05 && q.param >= 0x05 && q.param <= 0x09)   // pitch levels
                    expect (q.value >= 0 && q.value <= 127);
            for (auto& q : params)
                if (q.group == 0x09 && q.param >= 0x09 && q.param <= 0x0F)   // filter levels
                    expect (q.value >= 0 && q.value <= 127);

            // 1 AFM MONO (type $00) partage la structure 1AFM -> doit charger aussi (Table 2).
            juce::MemoryBlock mono (steel, sizeof (steel));
            ((juce::uint8*) mono.getData())[32] = 0x00;
            expectEquals (SyVoice::voiceBlobToParams ((const juce::uint8*) mono.getData(),
                                                      (int) mono.getSize()).size(), 6 * 29 + 6 + 4 + 5 + 14 + 14 + 1);

            // 2 AFM / 4 AFM : N « layers » identiques (Table 2). On vérifie le compte et que
            // l'adressage par élément (addrHi = élément<<5) est bien produit.
            const int perElem = 6 * 29 + 6 + 4 + 5 + 14 + 14;   // op + 6 + 4 pitch-rates + 5 pitch-levels + 2×7 filtre-rates + 2×7 filtre-niveaux
            {
                juce::MemoryBlock two; two.setSize (832, true);
                ((juce::uint8*) two.getData())[32] = 0x01;   // 2 AFM
                auto p2 = SyVoice::voiceBlobToParams ((const juce::uint8*) two.getData(), 832);
                expectEquals (p2.size(), perElem * 2 + 1);   // + VVOL commun

                juce::MemoryBlock four; four.setSize (1564, true);
                ((juce::uint8*) four.getData())[32] = 0x02;  // 4 AFM
                auto p4 = SyVoice::voiceBlobToParams ((const juce::uint8*) four.getData(), 1564);
                expectEquals (p4.size(), perElem * 4 + 1);
                bool e2 = false, e4 = false;
                for (auto& q : p4) { if (q.addrHi == 32) e2 = true; if (q.addrHi == 96) e4 = true; }
                expect (e2);   // élément 2 (addrHi 0x20)
                expect (e4);   // élément 4 (addrHi 0x60)
            }

            // Garde-fous « fiabilité d'abord » : un type mixte AFM+AWM ($08) -> rien (non géré).
            juce::uint8 unh[466] = { 0 }; unh[32] = 0x08;
            expect (SyVoice::voiceBlobToParams (unh, 466).isEmpty());
            expect (SyVoice::voiceBlobToParams (steel, 100).isEmpty());   // tronqué
            expect (SyVoice::voiceBlobToParams (nullptr, 466).isEmpty());
        }

        beginTest ("voiceBlobToParams decodes AWM waveform (TARKUSCYMB, 1 AWM)");
        {
            // Dump réel « TARKUSCYMB » (1 AWM, type @32 == 0x05), 221 octets.
            static const juce::uint8 awm[] = {
            0xf0, 0x43, 0x00, 0x7a, 0x01, 0x55, 0x4c, 0x4d, 0x20, 0x20, 0x38, 0x31, 0x30, 0x31, 0x56, 0x43,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x24,
            0x05, 0x54, 0x41, 0x52, 0x4b, 0x55, 0x53, 0x43, 0x59, 0x4d, 0x42, 0x00, 0x03, 0x64, 0x64, 0x0c,
            0x3f, 0x00, 0x00, 0x03, 0x64, 0x64, 0x03, 0x50, 0x00, 0x00, 0x24, 0x64, 0x64, 0x00, 0x00, 0x08,
            0x24, 0x64, 0x64, 0x0c, 0x0c, 0x0c, 0x00, 0x00, 0x02, 0x00, 0x01, 0x19, 0x01, 0x1b, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x7f,
            0x00, 0x00, 0x7f, 0x02, 0x34, 0x00, 0x7f, 0x01, 0x7f, 0x20, 0x06, 0x00, 0x00, 0x69, 0x00, 0x3c,
            0x40, 0x03, 0x3f, 0x3f, 0x3f, 0x3f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x01, 0x00, 0x00, 0x43, 0x00,
            0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x7f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
            0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x2a, 0x4a, 0x60, 0x01, 0x00, 0x01, 0x00, 0x01,
            0x00, 0x00, 0x48, 0x00, 0x7f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40,
            0x40, 0x40, 0x40, 0x00, 0x00, 0x2a, 0x4a, 0x60, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x48,
            0x00, 0x00, 0x05, 0x00, 0x20, 0x11, 0x17, 0x00, 0x1c, 0x30, 0x3f, 0x00, 0x2d, 0x43, 0x48, 0x5b,
            0x01, 0x00, 0x01, 0x00, 0x00, 0x76, 0x00, 0x76, 0x00, 0x00, 0x03, 0x43, 0xf7
            };

            auto p = SyVoice::voiceBlobToParams (awm, (int) sizeof (awm));
            // 1 AWM : par élément = ELVL + waveform + 7 params amp-EG ; + VVOL commun = 10.
            expectEquals (p.size(), 10);

            auto val = [&p] (int g, int h, int prm) -> int {
                for (auto& q : p) if (q.group == g && q.addrHi == h && q.param == prm) return q.value;
                return -1; };
            expectEquals (val (0x07, 0, 0x01), 105);   // AWMWAVE @109 (low7) -> wave "106" (0-indexé)
            expectEquals (val (0x03, 0, 0x00), 127);   // ELVL0 @98
            expectEquals (val (0x02, 0, 0x3F), 127);   // VVOL  @95
            // amp-EG AWM (offsets @base+89.., valeurs confirmées par l'utilisateur).
            expectEquals (val (0x07, 0, 0x50), 32);    // PAR1
            expectEquals (val (0x07, 0, 0x51), 17);    // PAR2
            expectEquals (val (0x07, 0, 0x52), 23);    // PAR3
            expectEquals (val (0x07, 0, 0x53), 0);     // PAR4
            expectEquals (val (0x07, 0, 0x54), 28);    // PARR1
            expectEquals (val (0x07, 0, 0x55), 48);    // PAL2
            expectEquals (val (0x07, 0, 0x56), 63);    // PAL3
        }

        beginTest ("voiceBlobToParams handles mixed AFM+AWM (1AFM_1AWM, type 8)");
        {
            // type $08 : élément 1 AFM (357 o.) + élément 2 AWM (112 o.). firstBase=116,
            // taille = 116 + 357 + 112 = 585 (+F7).
            juce::MemoryBlock mb; mb.setSize (586, true);
            auto* p = (juce::uint8*) mb.getData();
            p[0] = 0xF0; p[32] = 0x08;
            auto params = SyVoice::voiceBlobToParams (p, 586);
            expectEquals (params.size(), 217 + 9 + 1);   // AFM él.(217, dont niveaux EG o/b) + AWM él.(9) + VVOL

            bool afmE1 = false, awmE2 = false;
            for (auto& q : params)
            {
                if (q.addrHi == 0  && q.group == 0x56) afmE1 = true;   // OP1 élément 1 (AFM)
                if (q.addrHi == 32 && q.group == 0x07) awmE2 = true;   // AWM élément 2
            }
            expect (afmE1);   // l'élément 1 est bien décodé en AFM
            expect (awmE2);   // l'élément 2 est bien décodé en AWM (addrHi 0x20)
        }
    }
};

static SysexUtilsTests sysexUtilsTests;
