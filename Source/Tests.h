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
#include "SyParam.h"
#include "Version.h"
#include "PresetHexView.h"   // outil dev : sondage offsets -> params
#include "VoiceSession.h"   // decoupage MidiDemo : branchee dans MidiDemo (etape 2), caracterisee ici

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
    // Contrôleurs voix commune N2 0x28..0x3D décodés de chaque dump (d[32 + N2]).
    static constexpr int kCtrl = 0x3D - 0x28 + 1;   // 22
    static constexpr int kFx = 0x1C + 1;            // effets group 0x08 N2 0x00..0x1C (29)
    static constexpr int kElemCom = 8;              // element commun group 0x03 N2 0x01..0x08
    static constexpr int kFilt = 2 * (1 + 5 + 4) + 2;  // FTYPE, FRS, BP1-4, FOS1-4 x2 filtres + FVSON/FCMS
    static constexpr int kAfmLfo = 14;              // PEG range/RS + Main LFO 0x0D-13 + Sub LFO 0x15-19
    // Par élément AWM : N2 0x02..0x18 (23) + HOLD, PARS, BP1-4, VEL, ARVSW, AMS (9).
    static constexpr int kAwmExtra = 1 + 23 + 9 + 4 + 2 * (21 + 4) + 3;   // + WSOURCE   // + filtres AWM (2 x (N2 0x00-0x14 + FOS1-4) + commun)   // + PAOS1-4 (o/b2)
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

        beginTest ("mixed bank: only LM  8101VC blocks are presets");
        {
            auto mk = [] (const char* id, const char* name)
            {
                juce::MemoryBlock b (50, true);
                auto* d = (juce::uint8*) b.getData();
                d[0] = 0xF0; d[1] = 0x43; d[2] = 0x00; d[3] = 0x7A;
                for (int k = 0; k < 10; ++k) d[6 + k] = (juce::uint8) id[k];
                for (int k = 0; k < 10; ++k) d[33 + k] = (juce::uint8) name[k];
                d[49] = 0xF7;
                return b;
            };
            juce::MemoryBlock bank;
            bank.append (mk ("LM  8101MU", "MULTI 1   ").getData(), 50);
            bank.append (mk ("LM  8101VC", "VOICE A   ").getData(), 50);
            bank.append (mk ("LM  8101PN", "PAN 1     ").getData(), 50);
            bank.append (mk ("LM  8101VC", "VOICE B   ").getData(), 50);
            bank.append (mk ("LM  8101SY", "SYSTEM    ").getData(), 50);
            const auto* d = (const juce::uint8*) bank.getData();
            auto names = SyVoice::extractVoiceNames (d, bank.getSize());
            expectEquals (names.size(), 2);
            expectEquals (names[0].trim(), juce::String ("VOICE A"));
            expectEquals (names[1].trim(), juce::String ("VOICE B"));
            auto b1 = SyVoice::getVoiceBlock (d, bank.getSize(), 1);
            expectEquals ((int) b1.getSize(), 50);
            expectEquals ((int) ((const juce::uint8*) b1.getData())[33], (int) 'V');
            expectEquals ((int) ((const juce::uint8*) b1.getData())[39], (int) 'B');
            expect (SyVoice::getVoiceBlock (d, bank.getSize(), 2).getSize() == 0);
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

        beginTest ("retargetVoiceToEditBuffer met memtype=$7F, mem#=0 et recalcule le checksum");
        {
            // Bulk voix synthétique (>= 34 octets) : F0 43 0n 7A cMSB cLSB "LM  8101VC" ... ck F7.
            // Offsets 30 = Memory type, 31 = Memory#. On part d'un slot interne (type 0, mem# 5).
            juce::uint8 blk[40] = { 0 };
            blk[0] = 0xF0; blk[1] = 0x43; blk[2] = 0x10; blk[3] = 0x7A;
            blk[4] = 0x00; blk[5] = (juce::uint8) (sizeof (blk) - 8);   // count data factice
            const char* id = "LM  8101VC";
            for (int i = 0; i < 10; ++i) blk[6 + i] = (juce::uint8) id[i];
            blk[30] = 0x00;   // Memory type = internal
            blk[31] = 0x05;   // Memory# = un slot quelconque
            blk[sizeof (blk) - 1] = 0xF7;
            // Checksum initial cohérent (pour partir d'un bloc valide).
            blk[sizeof (blk) - 2] = SyVoice::yamahaChecksum (blk + 6, (int) sizeof (blk) - 8);
            expect (SyVoice::verifyYamahaBulkChecksum (blk, (int) sizeof (blk)));

            SyVoice::retargetVoiceToEditBuffer (blk, (int) sizeof (blk));
            expectEquals ((int) blk[30], 0x7F);     // -> Edit Buffer
            expectEquals ((int) blk[31], 0x00);     // Memory# remis à 0
            // Le checksum a été recalculé -> le bloc reste valide.
            expect (SyVoice::verifyYamahaBulkChecksum (blk, (int) sizeof (blk)));

            // No-op si pas un bulk voix (octet [3] != 0x7A) : rien n'est modifié.
            juce::uint8 notVoice[40] = { 0 };
            notVoice[0] = 0xF0; notVoice[1] = 0x43; notVoice[3] = 0x7E; notVoice[30] = 0x11;
            notVoice[sizeof (notVoice) - 1] = 0xF7;
            SyVoice::retargetVoiceToEditBuffer (notVoice, (int) sizeof (notVoice));
            expectEquals ((int) notVoice[30], 0x11);   // inchangé
        }

        beginTest ("detectSynthKind reconnait SY77 et les autres fabricants");
        {
            // Bloc voix SY77 : F0 43 0n 7A cMSB cLSB "LM  8101VC" ...
            juce::uint8 sy77[20] = { 0 };
            sy77[0] = 0xF0; sy77[1] = 0x43; sy77[2] = 0x10; sy77[3] = 0x7A;
            const char* id = "LM  8101VC";
            for (int i = 0; i < 10; ++i) sy77[6 + i] = (juce::uint8) id[i];
            expect (SyVoice::detectSynthKind (sy77, sizeof (sy77)) == SyVoice::SynthKind::SY77);

            // Yamaha mais pas le classifieur SY77 -> OtherYamaha
            juce::uint8 yam[20] = { 0xF0, 0x43, 0x10, 0x7A };
            expect (SyVoice::detectSynthKind (yam, sizeof (yam)) == SyVoice::SynthKind::OtherYamaha);

            // Roland / Korg / inconnu
            const juce::uint8 roland[] = { 0xF0, 0x41, 0x10, 0x42 };
            expect (SyVoice::detectSynthKind (roland, sizeof (roland)) == SyVoice::SynthKind::Roland);
            const juce::uint8 korg[] = { 0xF0, 0x42, 0x30 };
            expect (SyVoice::detectSynthKind (korg, sizeof (korg)) == SyVoice::SynthKind::Korg);
            expect (SyVoice::detectSynthKind (nullptr, 0) == SyVoice::SynthKind::Unknown);
        }

        beginTest ("isNewerVersion compare en ignorant 'v' et le suffixe -dev");
        {
            expect (  SyVoice::isNewerVersion ("1.3.0", "1.2.72-dev"));
            expect (  SyVoice::isNewerVersion ("v1.3.0", "1.2.99"));
            expect (  SyVoice::isNewerVersion ("2.0.0", "1.9.9"));
            expect (! SyVoice::isNewerVersion ("1.2.72", "1.2.72-dev"));   // égales (suffixe ignoré)
            expect (! SyVoice::isNewerVersion ("1.2.5", "1.2.5"));
            expect (! SyVoice::isNewerVersion ("1.2.0", "1.3.0"));
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
            expectEquals ((int) SyVoice::egGroupFor (EG::awmPitch),     0x07);  // AWM pitch-EG
            // PEG : PPR1 = FPR1 + 5 (0x01 -> 0x06), PPEGR 0x0F, PPRS 0x10, PVPSW 0x11 (OCR l.492-503).
            expectEquals (0x01 + SyVoice::pitchEgN2Offset (true), 0x06);
            expectEquals (0x0C + SyVoice::pitchEgN2Offset (true), 0x11);
            expectEquals (SyVoice::pitchEgN2Offset (false), 0);

            // AFM amp-EG = PAR OPÉRATEUR : group = (op<<4)|6, op 0..5 = OP6..OP1.
            const int expectedOp[6] = { 0x06, 0x16, 0x26, 0x36, 0x46, 0x56 };
            for (int op = 0; op < 6; ++op)
                expectEquals ((int) SyVoice::egGroupFor (EG::afmAmplitude, op), expectedOp[op]);
            // cohérent avec afmOperatorGroup (même source).
            for (int op = 0; op < 6; ++op)
                expectEquals ((int) SyVoice::egGroupFor (EG::afmAmplitude, op),
                              (int) SyVoice::afmOperatorGroup (op));

            // INVARIANT CLÉ : aucun type d'EG ne doit JAMAIS renvoyer 0x00 (le group du bug).
            for (auto k : { EG::filter, EG::pitch, EG::afmAmplitude, EG::awmAmplitude, EG::awmPitch })
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

        beginTest ("panLevelToDisplay/panLevelToWire — pan EG offset-binary (o/b) offset 32 inversible");
        {
            // Round-trip sur toute la plage filaire pan 0..63 (centre 32).
            for (int w = 0; w <= 63; ++w)
            {
                const int disp = SyVoice::panLevelToDisplay ((juce::uint8) w);
                expectEquals ((int) SyVoice::panLevelToWire (disp), w);
                expect (disp >= -32 && disp <= 31);   // plage d'affichage spec (-32~+31 o/b)
            }
            for (int dpy = -32; dpy <= 31; ++dpy)
                expectEquals (SyVoice::panLevelToDisplay (SyVoice::panLevelToWire (dpy)), dpy);

            // Ancres : centre = octet 32 ; extrêmes -32/+31. Cf. sy77midi_ocr.txt l.563-584.
            expectEquals (SyVoice::panLevelToDisplay (32), 0);    // centre (pan = milieu)
            expectEquals (SyVoice::panLevelToDisplay (0), -32);   // gauche
            expectEquals (SyVoice::panLevelToDisplay (63), 31);   // droite
            expectEquals ((int) SyVoice::panLevelToWire (0), 32); // display 0 -> octet 32
            expectEquals ((int) SyVoice::panLevelToWire (-32), 0);
            expectEquals ((int) SyVoice::panLevelToWire (31), 63);
            // Bornage : ne déborde jamais 0..63 (distinct de l'offset-64 des EG pitch/filtre).
            expectEquals ((int) SyVoice::panLevelToWire (200), 63);
            expectEquals ((int) SyVoice::panLevelToWire (-200), 0);
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

        beginTest ("codec déclaratif — primitives s/m, o/b1, o/b2 (oracle lua) + équivalence wrappers");
        {
            using namespace SyVoice;
            // s/m générique : ±7 {7,8}, ±12 {12,16}, ±15 {15,16} — inversible + ancres lua.
            for (auto sb : { std::pair<int,int>{7,8}, {12,16}, {15,16} })
                for (int d = -sb.first; d <= sb.first; ++d)
                    expectEquals (smToDisplay (smToWire (d, sb.first, sb.second), sb.first, sb.second), d);
            expectEquals (smToWire ( 0, 7, 8), 0);
            expectEquals (smToWire (-1, 7, 8), 9);    // signbit 8 | 1
            expectEquals (smToWire (-7, 7, 8), 15);   // 8 | 7
            expectEquals (smToWire (-12, 12, 16), 28);// 16 | 12 (AT PB range)
            // Formule ad hoc de MidiSlider::sliderValueChanged (setRangeAndRound(min<0) :
            // d<0 -> (~d) + (max+2)) == codec s/m pour ±7 (RS/VEL/PARS) et ±15 (detune).
            for (auto sb : { std::pair<int,int>{7,8}, {15,16} })
                for (int d = -sb.first; d < 0; ++d)
                    expectEquals ((~d) + (sb.first + 2), smToWire (d, sb.first, sb.second));
            // fpdDetune == primitive s/m {15,16} (le wrapper ne change rien).
            for (int d = -15; d <= 15; ++d)
                expectEquals ((int) fpdDetuneToWire (d), smToWire (d, 15, 16));

            // o/b1 générique : EG filtre (offset 64) et pan (offset 32) == wrappers.
            for (int w = 0; w <= 127; ++w) expectEquals (egLevelToDisplay ((juce::uint8) w), obToDisplay (w, 64));
            for (int d = -64; d <= 63; ++d) expectEquals ((int) egLevelToWire (d), obToWire (d, 64, 127));
            for (int d = -32; d <= 31; ++d) expectEquals ((int) panLevelToWire (d), obToWire (d, 32, 63));

            // o/b2 (FOS break-point offsets) : display -127..+127 -> combined=display+128, V1=bit7,V2=low7.
            expectEquals (ob2Combined (0),   128);
            expectEquals (ob2Combined (-127), 1);
            expectEquals (ob2Combined (127), 255);
            expectEquals ((int) ob2V1 (127), 1);  expectEquals ((int) ob2V2 (127), 127);   // 255 = (1<<7)|127
            expectEquals ((int) ob2V1 (0),   1);  expectEquals ((int) ob2V2 (0),   0);      // 128 = (1<<7)|0
            for (int d = -127; d <= 127; ++d)
                expectEquals (ob2ToDisplay (ob2V1 (d), ob2V2 (d)), d);   // inversibilité 2 octets
        }

        beginTest ("SyParamTable — dico déclaratif : 228 entrées, ui uniques, codec aller-retour");
        {
            using namespace SyVoice;
            expectEquals (SyParam::kNumParams, 228);
            std::set<int> uis;
            for (const auto& e : SyParam::kParams)
            {
                expect (uis.insert (e.ui).second, juce::String ("ui dupliqué: ") + juce::String (e.ui));
                for (int dsp = e.dispMin; dsp <= e.dispMax; ++dsp)
                {
                    switch (e.enc)
                    {
                        case SyEnc::plain:   break;   // display == octet filaire (rien à convertir)
                        case SyEnc::signMag: expectEquals (smToDisplay (smToWire (dsp, e.encA, e.encB), e.encA, e.encB), dsp); break;
                        case SyEnc::offBin1: expectEquals (obToDisplay (obToWire (dsp, e.encA, e.encB), e.encA), dsp); break;
                        case SyEnc::offBin2: expectEquals (ob2ToDisplay (ob2V1 (dsp), ob2V2 (dsp)), dsp); break;
                    }
                }
            }
            // Ancrages (oracle lua/Electra) : encodage correct des params « pièges ».
            auto find = [] (int ui) -> const SyParam::Entry* {
                for (const auto& e : SyParam::kParams) if (e.ui == ui) return &e; return nullptr; };
            expect (find (1026) != nullptr && find (1026)->enc == SyEnc::signMag && find (1026)->encA == 15); // DETUNE OP1
            expect (find (7309) != nullptr && find (7309)->enc == SyEnc::offBin1 && find (7309)->encA == 64); // FEG L0
            expect (find (7321) != nullptr && find (7321)->enc == SyEnc::offBin2);                            // BP1 OFFSET
            expect (find (1000) != nullptr && find (1000)->enc == SyEnc::plain);                              // R1 OP1
        }

        beginTest ("syTranslate — resolver d'adresse (port lua) : ancrages + cohérence table/helpers");
        {
            using namespace SyVoice;
            auto A = [] (int ui, int el, int fl) { return syTranslate (ui, el, fl); };
            // Ancrages (oracle main.lua translate()).
            { auto a = A(1026,0,0); expect (a.ok && a.group==0x56 && a.t2==0x00 && a.n2==0x1A); } // DETUNE OP1
            { auto a = A(6000,0,0); expect (a.ok && a.group==0x06 && a.t2==0x00 && a.n2==0x00); } // R1 OP6
            { auto a = A(7000,1,0); expect (a.ok && a.group==0x05 && a.t2==0x20 && a.n2==0x00); } // AFM commun, élém 2
            { auto a = A(7141,0,0); expect (a.ok && a.group==0x02 && a.t2==0x00 && a.n2==0x29); } // AT PB (voice commun)
            { auto a = A(7201,2,0); expect (a.ok && a.group==0x03 && a.t2==0x40 && a.n2==0x01); } // ELDT, élém 3
            { auto a = A(7309,0,0); expect (a.ok && a.group==0x09 && a.t2==0x00 && a.n2==0x09); } // FEG L0 (par-élément)
            { auto a = A(7351,0,0); expect (a.ok && a.group==0x09 && a.t2==0x02 && a.n2==0x33); } // VEL SENS commun AFM (fn=2)
            { auto a = A(7351,3,3); expect (a.ok && a.group==0x09 && a.t2==0x65 && a.n2==0x33); } // commun AWM (fn=5) + élém 4
            { auto a = A(7400,0,0); expect (a.ok && a.group==0x08 && a.t2==0x00 && a.n2==0x00); } // effet
            expect (! A(9001,0,0).ok && ! A(500,0,0).ok);                                          // non transmissibles

            // T2 = bits d'élément (sauf voice-commun/effet) : doit suivre elementAddrHi.
            for (int el = 0; el <= 3; ++el)
                expectEquals ((int) A(7000, el, 0).t2, (int) elementAddrHi (el));

            // Cohérence avec la TABLE : chaque entrée se résout, son N2 == table, et le groupe
            // d'op AFM == afmOperatorGroup(6-op).
            for (const auto& e : SyParam::kParams)
            {
                auto a = A(e.ui, 0, 0);
                expect (a.ok, juce::String ("ui non résolu: ") + juce::String (e.ui));
                expectEquals ((int) a.n2, (int) e.n2);
                if (e.zone == SyParam::Zone::afmOp)
                    expectEquals ((int) a.group, (int) afmOperatorGroup (6 - e.op));
            }
        }

        beginTest ("SyParam::bytesFor — pipeline bout-en-bout (table+adresse+encodage) byte-identique");
        {
            const int dev = 1;
            const auto db = SyVoice::deviceByte (dev);
            auto eq = [&] (SyParam::Msg m, std::array<juce::uint8,9> exp)
            { expect (m.ok); for (int i = 0; i < 9; ++i) expectEquals ((int) m.bytes[(size_t) i], (int) exp[(size_t) i]); };

            // TL OP1 (plain) : group 0x56, N2 0x1B, valeur brute 100.
            eq (SyParam::bytesFor (1027, 100, 0, 0, dev),
                { { 0x43, db, 0x34, 0x56, 0x00, 0x00, 0x1B, 0x00, 100 } });
            // DETUNE OP1 (s/m 15/16) : display -3 -> wire 16|3 = 19.
            eq (SyParam::bytesFor (1026, -3, 0, 0, dev),
                { { 0x43, db, 0x34, 0x56, 0x00, 0x00, 0x1A, 0x00, 19 } });
            // FEG L0 (offBin1 offset 64), élément 2 : T2 0x20, N2 0x09, display 0 -> wire 64.
            eq (SyParam::bytesFor (7309, 0, 1, 0, dev),
                { { 0x43, db, 0x34, 0x09, 0x20, 0x00, 0x09, 0x00, 64 } });
            // BP1 OFFSET (offBin2) : display 0 -> combined 128 -> V1=1 (octet[7]), V2=0 (octet[8]).
            eq (SyParam::bytesFor (7321, 0, 0, 0, dev),
                { { 0x43, db, 0x34, 0x09, 0x00, 0x00, 0x15, 0x01, 0x00 } });
            // Effet « EF MODE » (group 0x08, N2 0x00, plage 0..3) : display 2 -> V2 2.
            eq (SyParam::bytesFor (7400, 2, 0, 0, dev),
                { { 0x43, db, 0x34, 0x08, 0x00, 0x00, 0x00, 0x00, 2 } });
            // La valeur affichée est BORNÉE à la plage déclarée (7 > max 3 -> 3).
            expectEquals ((int) SyParam::bytesFor (7400, 7, 0, 0, dev).bytes[8], 3);
            // ui inconnu -> ok=false.
            expect (! SyParam::bytesFor (424242, 0, 0, 0, dev).ok);

            // Cohérence : pour CHAQUE entrée de la table, bytesFor concorde octet-pour-octet avec
            // syTranslate + le codec (sur la borne basse de la plage). Verrouille tout le pipeline.
            for (const auto& e : SyParam::kParams)
            {
                auto a = SyVoice::syTranslate (e.ui, 0, 0);
                auto m = SyParam::bytesFor (e.ui, e.dispMin, 0, 0, dev);
                expect (m.ok);
                expectEquals ((int) m.bytes[3], (int) a.group);
                expectEquals ((int) m.bytes[6], (int) a.n2);
            }
        }

        beginTest ("Phase 4 pilote Effects — adresses/plages du dico == ancien inline (byte-identité)");
        {
            // Effects.h émettait {0x43,dev,0x34, 0x08, 0x00, 0x00, n2, 0x00, val} pour n2=0x00..0x1C.
            // Migré : adresse via syTranslate(7400+n2) + plage via SyParamTable. Doit être IDENTIQUE.
            for (int n2 = 0x00; n2 <= 0x1C; ++n2)
            {
                auto a = SyVoice::syTranslate (7400 + n2, 0, 0);
                expect (a.ok, juce::String ("effet ui non résolu: ") + juce::String (7400 + n2));
                expectEquals ((int) a.group, 0x08);
                expectEquals ((int) a.t2,    0x00);
                expectEquals ((int) a.n2,    n2);
            }
            // Plages du dico == maxVal historiquement codés à la main (échantillon).
            expectEquals (SyParam::entry (7400)->dispMax,  3);   // EF MODE
            expectEquals (SyParam::entry (7401)->dispMax,  4);   // CH1 TYPE
            expectEquals (SyParam::entry (7415)->dispMax, 40);   // RV1 TYPE
            expectEquals (SyParam::entry (7404)->dispMax, 127);  // CH1 PRM1
            expectEquals (SyParam::entry (7427)->dispMax,  1);   // ST MIX1
        }

        beginTest ("Phase 4 filtres — syTranslate reproduit l'adressage Filter1/Filter2 (byte-identité)");
        {
            // Filter1/2 émettaient group 0x09, addrHi = ((élém-1)<<5) | fN, N2 (rates 03-08, levels
            // 09-0F, slope 10). fN : Filtre1 = 0 (AFM)/3 (AWM) ; Filtre2 = 1 (AFM)/4 (AWM).
            const int filterN2[] = { 0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10 };
            for (int el = 0; el <= 3; ++el)
                for (int fn : { 0, 1, 3, 4 })
                    for (int n2 : filterN2)
                    {
                        auto a = SyVoice::syTranslate (7300 + n2, el, fn);
                        expect (a.ok);
                        expectEquals ((int) a.group, 0x09);
                        expectEquals ((int) a.t2,    (el << 5) | fn);
                        expectEquals ((int) a.n2,    n2);
                    }
            // CommonFilter : cutoff/mode (N2 0x00-0x02) sur fN f1=fnBase+0 / f2=fnBase+1.
            for (int el = 0; el <= 3; ++el)
                for (int fn : { 0, 1, 3, 4 })
                    for (int n2 : { 0x00, 0x01, 0x02 })
                    {
                        auto a = SyVoice::syTranslate (7300 + n2, el, fn);
                        expect (a.ok); expectEquals ((int) a.group, 0x09);
                        expectEquals ((int) a.t2, (el << 5) | fn);
                        expectEquals ((int) a.n2, n2);
                    }
            // Params filtre-COMMUNS (N2 0x32-0x34 : FRES/FVSON/FCMS) -> fN FORCÉ 2 (AFM) / 5 (AWM),
            // quel que soit le filterTarget passé (CommonFilter passe fnBase+2 = 2 ou 5).
            for (int el = 0; el <= 3; ++el)
                for (int n2 : { 0x32, 0x33, 0x34 })
                {
                    expectEquals ((int) SyVoice::syTranslate (7300 + n2, el, 0 + 2).t2, (el << 5) | 2); // AFM
                    expectEquals ((int) SyVoice::syTranslate (7300 + n2, el, 3 + 2).t2, (el << 5) | 5); // AWM
                }
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
            // Offsets de scaling EGOS (0x20-0x23) : 2 octets o/b2, V1 = MSB (0/1) porte par vp.v1.
            for (auto& q : params)
                if ((q.group & 0x0F) == 0x06 && q.param >= 0x20 && q.param <= 0x23)
                {
                    expect (q.v1 == 0 || q.v1 == 1);
                    const int d = SyVoice::ob2ToDisplay (q.v1, q.value);
                    expect (d >= -128 && d <= 127);
                    expectEquals ((int) SyVoice::ob2V1 (juce::jlimit (-127, 127, d)), q.v1 & 1);   // aller-retour
                }
            // 6 op×29 + (ELVL,ALGNUM,FCTOF1,FCTOF2,FFRES,FYPSW) + 4 pitch-rates + 5 pitch-levels
            // (FPL0-3+FPRL1) + 2×7 filtre-rates + 2×7 filtre-niveaux + VVOL.
            expectEquals (params.size(), 6 * 36 + 6 + 4 + 5 + 14 + 14 + kElemCom + kAfmLfo + kFilt + 1 + kCtrl + kFx);

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
                                                      (int) mono.getSize()).size(), 6 * 36 + 6 + 4 + 5 + 14 + 14 + kElemCom + kAfmLfo + kFilt + 1 + kCtrl + kFx);

            // 2 AFM / 4 AFM : N « layers » identiques (Table 2). On vérifie le compte et que
            // l'adressage par élément (addrHi = élément<<5) est bien produit.
            const int perElem = 6 * 36 + 6 + 4 + 5 + 14 + 14 + kElemCom + kAfmLfo + kFilt;   // op + 6 + 4 pitch-rates + 5 pitch-levels + 2×7 filtre-rates + 2×7 filtre-niveaux
            {
                juce::MemoryBlock two; two.setSize (832, true);
                ((juce::uint8*) two.getData())[32] = 0x01;   // 2 AFM
                auto p2 = SyVoice::voiceBlobToParams ((const juce::uint8*) two.getData(), 832);
                expectEquals (p2.size(), perElem * 2 + 1 + kCtrl + kFx);   // + VVOL + controleurs

                juce::MemoryBlock four; four.setSize (1564, true);
                ((juce::uint8*) four.getData())[32] = 0x02;  // 4 AFM
                auto p4 = SyVoice::voiceBlobToParams ((const juce::uint8*) four.getData(), 1564);
                expectEquals (p4.size(), perElem * 4 + 1 + kCtrl + kFx);
                bool e2 = false, e4 = false;
                for (auto& q : p4) { if (q.addrHi == 32) e2 = true; if (q.addrHi == 96) e4 = true; }
                expect (e2);   // élément 2 (addrHi 0x20)
                expect (e4);   // élément 4 (addrHi 0x60)
            }

            // Garde-fous « fiabilité d'abord » : un type mixte AFM+AWM ($08) -> rien (non géré).
            juce::uint8 unh[466] = { 0 }; unh[32] = 0x08;
            expect (SyVoice::voiceBlobToParams (unh, 466).isEmpty());
            expect (SyVoice::voiceBlobToParams (steel, 100).isEmpty());   // tronqué

            // Vue hexa dev (PresetHexView) : le sondage par inversion de bit retrouve les
            // offsets du loader. Base E1 = 107 : ALGNUM @+270, TL OP1 = record 5 (OP6..OP1) +29.
            {
                juce::MemoryBlock blk (steel, sizeof (steel));
                const auto notes = PresetHex::probe (blk);
                expect (notes[377].joinIntoString (";").contains ("ALGORITHM"));
                expect (notes[107 + 5 * 45 + 29].joinIntoString (";").contains ("LEVEL OP1"));
                expect (notes[35].isEmpty());                                   // nom : non sonde
                expect (PresetHex::regionOf (107 + 5 * 45 + 29, (int) blk.getSize(),
                                             PresetHex::layoutFor (0x03)).contains ("OP1 +29"));
                expect (PresetHex::report (blk, "t").contains ("Checksum : OK"));

                // Lot effets / LFO / element commun (offsets par statistiques, 12 089 voix).
                auto val = [&] (int g, int hi, int n2) -> int
                {
                    for (auto& vp : SyVoice::voiceBlobToParams (steel, (int) sizeof (steel)))
                        if (vp.group == g && vp.addrHi == hi && vp.param == n2) return vp.value;
                    return -1;
                };
                expectEquals (val (0x08, 0, 0x00), (int) steel[43]);          // EFMODE
                expectEquals (val (0x08, 0, 0x01), (int) steel[44]);          // CHR1 type
                expectEquals (val (0x08, 0, 0x1C), (int) steel[71]);          // ST_MIX2
                expectEquals (val (0x05, 0, 0x0D), (int) steel[107 + 270 + 0x0D]);   // LFO speed
                expectEquals (val (0x05, 0, 0x19), (int) steel[107 + 270 + 0x19]);   // sub LFO PMD
                expectEquals (val (0x05, 0, 0x14), -1);                      // N2 0x14 omis
                expectEquals (val (0x03, 0, 0x07), (int) steel[98 + 7]);      // PANNM
                expectEquals (val (0x03, 0, 0x08), (int) steel[98 + 8]);      // OUTSEL
                expect (notes[43].joinIntoString (";").contains ("EF MODE"));
                // Sante d'un bloc (flag rouge librairie) + tailles attendues (12 086/12 089 voix OK).
                expectEquals (SyVoice::expectedVoiceSize (0x03), 466);
                expectEquals (SyVoice::expectedVoiceSize (0x06), 342);
                expectEquals (SyVoice::expectedVoiceSize (0x07), 584);
                expectEquals (SyVoice::expectedVoiceSize (0x0A), 588);
                expect (SyVoice::voiceProblem (steel, (int) sizeof (steel)).isEmpty());
                {
                    juce::MemoryBlock bad (steel, sizeof (steel));
                    ((juce::uint8*) bad.getData())[200] ^= 0x01;
                    expect (SyVoice::voiceProblem ((const juce::uint8*) bad.getData(), (int) bad.getSize()).contains ("checksum"));
                    expect (SyVoice::voiceProblem (steel, 400).contains ("taille"));
                }
                {   // DRUM : commun (29 effets + 22 controleurs + VVOL) + 61 notes x 7 params.
                    juce::MemoryBlock dr; dr.setSize (588, true);
                    auto* q = (juce::uint8*) dr.getData();
                    q[32] = 0x0A; q[98 + 8 * 2 + 3] = 0x15; q[98 + 8 * 2 + 2] = 0x01;   // note 38 : WAV = 0x95
                    const auto dp = SyVoice::voiceBlobToParams (q, 588);
                    expectEquals (dp.size(), 29 + 22 + 1 + 61 * 7);
                    bool found = false;
                    for (auto& x : dp) if (x.group == 0x04 && x.addrHi == 38 && x.param == 0x02) found = x.value == 0x15 && x.v1 == 1;
                    expect (found);
                }
                // Filtres (Table 2) : FTYPE1 @403, FTYPE2 @432, FOS11 (MSB @424, low @425), FVSON @462.
                expectEquals (val (0x09, 0, 0x00), (int) steel[403]);
                expectEquals (val (0x09, 1, 0x00), (int) steel[432]);
                expectEquals (val (0x09, 0, 0x14), (int) steel[423]);   // FBP4 filtre 1
                expectEquals (val (0x09, 0, 0x15), (int) steel[425]);   // FOS1 V2 (low 7 bits)
                expectEquals (val (0x09, 2, 0x33), (int) steel[462]);
                expectEquals (val (0x09, 2, 0x34), (int) steel[463]);
                // Octets packes operateur (OP1 = record 5) : RS @15, 0x18 @25, 0x19 V1 @26 / V2 @27.
                const int op1 = 107 + 5 * 45;
                expectEquals (val (0x56, 0, 0x0F), (int) steel[op1 + 15]);
                expectEquals (val (0x56, 0, 0x18), (int) steel[op1 + 25]);
                expectEquals (val (0x56, 0, 0x19), (int) steel[op1 + 27]);
                for (auto& vp : SyVoice::voiceBlobToParams (steel, (int) sizeof (steel)))
                    if (vp.group == 0x56 && vp.addrHi == 0 && vp.param == 0x19)
                        expectEquals (vp.v1, (int) steel[op1 + 26]);
            }
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
            {   // Filtres AWM : FTYPE f1 @107+27, f2 @107+56, FRES @107+85 (fN 3/4/5).
                auto v = [&] (int hi, int n2) { for (auto& q : p) if (q.group == 0x09 && q.addrHi == hi && q.param == n2) return q.value; return -1; };
                expectEquals (v (3, 0x00), (int) awm[107 + 27]);
                expectEquals (v (4, 0x00), (int) awm[107 + 56]);
                expectEquals (v (3, 0x14), (int) awm[107 + 27 + 0x14]);
                expectEquals (v (5, 0x32), (int) awm[107 + 85]);
            }
            // 1 AWM : par élément = ELVL + waveform + 7 params amp-EG ; + VVOL commun = 10 ; + contrôleurs.
            expectEquals (p.size(), 10 + kCtrl + kAwmExtra + kElemCom + kFx);

            auto val = [&p] (int g, int h, int prm) -> int {
                for (auto& q : p) if (q.group == g && q.addrHi == h && q.param == prm) return q.value;
                return -1; };
            expectEquals (val (0x07, 0, 0x01), 105);   // AWMWAVE @109 (low7) -> wave "106" (0-indexé)
            // Bloc AWM N2+1 / N2+9 : plages de la spec sur ce dump réel.
            expect (val (0x07, 0, 0x0F) >= 1 && val (0x07, 0, 0x0F) <= 3);   // PPEGR 1~3
            expect (val (0x07, 0, 0x17) >= 0 && val (0x07, 0, 0x17) <= 5);   // PLFWAV 0~5
            expect (val (0x07, 0, 0x4F) == 0 || val (0x07, 0, 0x4F) == 1);   // PAEGMD normal/hold
            expect (val (0x07, 0, 0x58) <= val (0x07, 0, 0x5B));            // BP1 <= BP4
            // VEL / ARVSW / AMS apres les 4 PAOS (2 octets) : base + N2 + 13 (revue : pas +9).
            expectEquals (val (0x07, 0, 0x60), (int) awm[107 + 0x60 + 13]);
            expect (val (0x07, 0, 0x61) == 0 || val (0x07, 0, 0x61) == 1);
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
            expectEquals (params.size(), 241 + 6 * 3 + 9 + kAwmExtra + 1 + kCtrl + 2 * kElemCom + kAfmLfo + kFilt + kFx);   // AFM él.(217, dont niveaux EG o/b) + AWM él.(9) + VVOL

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

//==============================================================================
// Phrase player (Sequencer.h): phrase data sanity + timing math.
class SequencerTests : public juce::UnitTest
{
public:
    SequencerTests() : juce::UnitTest ("Sequencer", "Sysex77") {}

    void runTest() override
    {
        beginTest ("every phrase schedules balanced, sorted, in-loop events");
        for (auto& ph : Seq::phrases())
        {
            expect (! ph.evs.empty(), ph.name);
            auto t = Seq::schedule (ph, 60);
            int ons = 0, offs = 0;
            for (size_t i = 0; i < t.size(); ++i)
            {
                (t[i].on ? ons : offs)++;
                expect (t[i].beat >= 0.0 && t[i].beat <= ph.loopBeats, ph.name);
                expect (t[i].note >= 0 && t[i].note <= 127);
                if (i > 0) expect (t[i - 1].beat <= t[i].beat, ph.name);
            }
            expectEquals (ons, offs);
            for (auto& e : ph.evs) expect (e.beat + e.len <= ph.loopBeats, ph.name);
        }

        beginTest ("note-off sorts before note-on on the same beat");
        Seq::Phrase p { "t", 2.0, { { 0.0, 0, 100, 1.0 }, { 1.0, 0, 100, 1.0 } } };
        auto t = Seq::schedule (p, 60);
        expect (! t[1].on && t[2].on && t[1].beat == 1.0 && t[2].beat == 1.0);

        beginTest ("notes are clamped to 0..127 around the root");
        Seq::Phrase hi { "h", 1.0, { { 0.0, 24, 100, 0.5 } } };
        expectEquals (Seq::schedule (hi, 120)[0].note, 127);

        beginTest ("timing math: beats to ms, tempo re-anchor keeps the playhead");
        expectWithinAbsoluteError (Seq::beatsToMs (4.0, 120.0), 2000.0, 1e-9);
        expectWithinAbsoluteError (Seq::beatsToMs (1.0, 60.0), 1000.0, 1e-9);
        // 1 s at 120 BPM from beat 0 -> beat 2.
        expectWithinAbsoluteError (Seq::beatAt (1000.0, 0.0, 120.0, 2000.0), 2.0, 1e-9);
    }
};

static SequencerTests sequencerTests;

//==============================================================================
// Edit history (EditHistory.h): grouping, undo/redo, unknown values, B diff, revert.
class EditHistoryTests : public juce::UnitTest
{
public:
    EditHistoryTests() : juce::UnitTest ("EditHistory", "Sysex77") {}

    void runTest() override
    {
        using namespace EditHist;
        const Key a = keyOf (0x56, 0, 0, 0x10), b = keyOf (0x08, 0, 0, 0x02), u = keyOf (0x07, 0, 0, 0x30);

        beginTest ("key/value packing round-trips");
        expectEquals (gOf (a), 0x56); expectEquals (pOf (a), 0x10);
        expectEquals (v1Of (valOf (1, 5)), 1); expectEquals (v2Of (valOf (1, 5)), 5);

        beginTest ("one drag = one step, separate edits = separate steps");
        History h; h.load ({ { a, 10 }, { b, 3 } });
        h.recordEdit (a, 11, 0, true); h.recordEdit (a, 12, 1000, true); h.recordEdit (a, 20, 2000, true);
        expectEquals (h.numSteps(), 1);
        h.recordEdit (b, 4, 5000, false);
        expectEquals (h.numSteps(), 2);
        h.recordEdit (b, 5, 5100, false);   // within 300 ms: same step
        expectEquals (h.numSteps(), 2);

        beginTest ("undo restores the value before the gesture, redo re-applies");
        int unk = 0;
        auto r = h.undo (unk);
        expect (r.size() == 1 && r[0].first == b && r[0].second == 3 && unk == 0);
        r = h.undo (unk);
        expect (r.size() == 1 && r[0].first == a && r[0].second == 10);
        expect (! h.canUndo() && h.canRedo());
        r = h.redo (unk);
        expect (r.size() == 1 && r[0].second == 20);

        beginTest ("a new edit after undo drops the redo tail");
        h.recordEdit (a, 30, 9000, false);
        expect (! h.canRedo());
        expectEquals (h.numSteps(), 2);

        beginTest ("unknown old value is counted, not applied");
        History h2; h2.load ({ { a, 1 } });
        h2.recordEdit (u, 7, 0, false);
        r = h2.undo (unk);
        expect (r.empty() && unk == 1);

        beginTest ("RX keeps the state without creating a step");
        History h3; h3.load ({ { a, 1 } });
        h3.noteRx (a, 50);
        expect (! h3.canUndo());
        h3.recordEdit (a, 60, 0, false);
        r = h3.undo (unk);
        expect (r.size() == 1 && r[0].second == 50);

        beginTest ("noteRx is ignored when no voice is loaded (never loaded / unloaded)");
        {
            History h5;
            h5.noteRx (a, 5);
            expect (h5.diffFromOriginal().empty());
            h5.load ({ { a, 1 } });
            h5.unload();
            h5.noteRx (a, 7);
            expect (h5.diffFromOriginal().empty());
            expect (h5.makeRevertStep (0).empty() && ! h5.canUndo());
        }

        beginTest ("B diff and undoable revert");
        History h4; h4.load ({ { a, 1 }, { b, 2 } });
        h4.recordEdit (a, 9, 0, false);
        h4.recordEdit (u, 4, 1000, false);   // not in the original
        auto diff = h4.diffFromOriginal();
        expectEquals ((int) diff.size(), 2);
        auto rev = h4.makeRevertStep (2000);
        expect (rev.size() == 1 && rev[0].first == a && rev[0].second == 1);
        expect (h4.diffFromOriginal().empty());
        r = h4.undo (unk);   // undo the revert: both edits come back
        expectEquals ((int) r.size(), 2);
        expectEquals ((int) h4.diffFromOriginal().size(), 2);
        r = h4.redo (unk);   // redo: u has no known original -> left to the bulk
        expect (r.size() == 1 && unk == 1);

        beginTest ("no-op edit records nothing");
        History h5; h5.load ({ { a, 5 } });
        h5.recordEdit (a, 5, 0, false);
        expect (! h5.canUndo());
    }
};

static EditHistoryTests editHistoryTests;

//==============================================================================
// Voice templates (Templates.h): framing, checksum, type, decodable by the editor.
class VoiceTemplatesTests : public juce::UnitTest
{
public:
    VoiceTemplatesTests() : juce::UnitTest ("VoiceTemplates", "Sysex77") {}
    void runTest() override
    {
        const int types[4] = { 3, 3, 5, 8 };
        const int sizes[4] = { 466, 466, 221, 587 };
        for (int i = 0; i < (int) VoiceTemplates::list().size(); ++i)
        {
            beginTest (juce::String ("template ") + VoiceTemplates::list()[(size_t) i].name);
            auto m = VoiceTemplates::make (i);
            auto* b = (const juce::uint8*) m.getData();
            expectEquals ((int) m.getSize(), sizes[i]);
            expect (b[0] == 0xF0 && b[1] == 0x43 && b[3] == 0x7A && b[m.getSize() - 1] == 0xF7);
            expectEquals ((int) b[32], types[i]);
            expectEquals (((int) b[4] << 7) | b[5], (int) m.getSize() - 8);
            expect (SyVoice::verifyYamahaBulkChecksum (b, (int) m.getSize()));
            expect (SyVoice::detectSynthKind (b, m.getSize()) == SyVoice::SynthKind::SY77);
            expect (SyVoice::voiceBlobToParams (b, (int) m.getSize()).size() > 0);
        }
        expect (VoiceTemplates::make (99).getSize() == 0);

        beginTest ("setBulkDevice re-stamps only Yamaha bulk blocks, checksum untouched");
        auto m = VoiceTemplates::make (0);
        auto* b = (juce::uint8*) m.getData();
        SyVoice::setBulkDevice (b, (int) m.getSize(), 3);
        expectEquals ((int) b[2], 0x02);
        expect (SyVoice::verifyYamahaBulkChecksum (b, (int) m.getSize()));
        juce::uint8 param[11] = { 0xF0, 0x43, 0x10, 0x34, 0, 0, 0, 0, 0, 0, 0xF7 };
        SyVoice::setBulkDevice (param, 11, 5);
        expectEquals ((int) param[2], 0x10);   // not a bulk: unchanged
    }
};

static VoiceTemplatesTests voiceTemplatesTests;

//==============================================================================
// Shortcuts (Shortcuts.h): every default parses, no two actions share a key.
class ShortcutsTests : public juce::UnitTest
{
public:
    ShortcutsTests() : juce::UnitTest ("Shortcuts", "Sysex77") {}
    void runTest() override
    {
        beginTest ("defaults parse and are unique");
        juce::Array<juce::KeyPress> seen;
        for (auto& a : Shortcuts::actions())
        {
            const auto k = juce::KeyPress::createFromDescription (a.defaultKey);
            expect (k.isValid(), a.id);
            for (auto& s : seen) expect (! (Shortcuts::normalise (s) == Shortcuts::normalise (k)), a.id);
            seen.add (k);
        }
        beginTest ("arrow defaults map to the arrow key codes");
        expectEquals (juce::KeyPress::createFromDescription ("command + cursor down").getKeyCode(), juce::KeyPress::downKey);
        expectEquals (juce::KeyPress::createFromDescription ("command + cursor left").getKeyCode(), juce::KeyPress::leftKey);
    }
};

static ShortcutsTests shortcutsTests;

//==============================================================================
// Decoupage MidiDemo, etape 0 : helpers purs extraits de MidiDemo/MidiSysex. Les attendus
// reproduisent le code inline AVANT extraction (vecteurs figes).
class VoiceSessionHelpersTests : public juce::UnitTest
{
public:
    VoiceSessionHelpersTests() : juce::UnitTest ("VoiceSessionHelpers", "Sysex77") {}

    // Ancien code inline de MidiDemo::sendParams (copie verbatim, reference).
    static std::array<juce::uint8, 9> legacySendParams (int dev, EditHist::KV kv)
    {
        const juce::uint8 d[9] = { 0x43, SyVoice::deviceByte (dev), 0x34,
                                   (juce::uint8) EditHist::gOf (kv.first), (juce::uint8) EditHist::hiOf (kv.first),
                                   (juce::uint8) EditHist::loOf (kv.first), (juce::uint8) EditHist::pOf (kv.first),
                                   (juce::uint8) EditHist::v1Of (kv.second), (juce::uint8) EditHist::v2Of (kv.second) };
        std::array<juce::uint8, 9> a {};
        std::copy (d, d + 9, a.begin());
        return a;
    }
    // Ancien test inline de MidiDemo::sendToOutputs (isSysEx + 11 + bytes).
    static bool legacyRecordable (const juce::uint8* d, int n)
    {
        return n == 11 && d[0] == 0xF0 && d[1] == 0x43 && (d[2] & 0xF0) == 0x10 && d[3] == 0x34 && d[4] != 0x0F;
    }

    void runTest() override
    {
        using namespace EditHist;
        beginTest ("paramChangeBytes == legacy sendParams inline array (v1 != 0, device clamp)");
        {
            const KV kvs[] = { { keyOf (0x56, 0x00, 0x00, 0x10), valOf (0, 99) },
                               { keyOf (0x03, 0x20, 0x00, 0x2A), valOf (1, 5) },     // v1 != 0
                               { keyOf (0x08, 0x00, 0x00, 0x02), valOf (0x7F, 0x7F) },
                               { keyOf (0x0F, 0x7F, 0x7F, 0x7F), valOf (0, 0) } };
            for (int dev : { 0, 1, 3, 16, 17 })
                for (auto& kv : kvs)
                {
                    const auto got = SyVoice::paramChangeBytes (dev, gOf (kv.first), hiOf (kv.first), loOf (kv.first),
                                                                pOf (kv.first), v1Of (kv.second), v2Of (kv.second));
                    expect (got == legacySendParams (dev, kv));
                }
            // Vecteurs figes.
            auto a = SyVoice::paramChangeBytes (3, 0x03, 0x20, 0x00, 0x2A, 1, 5);
            const std::array<juce::uint8, 9> ea { { 0x43, 0x12, 0x34, 0x03, 0x20, 0x00, 0x2A, 0x01, 0x05 } };
            expect (a == ea);
            expectEquals ((int) SyVoice::paramChangeBytes (0,  0, 0, 0, 0, 0, 0)[1], 0x10);
            expectEquals ((int) SyVoice::paramChangeBytes (16, 0, 0, 0, 0, 0, 0)[1], 0x1F);
            expectEquals ((int) SyVoice::paramChangeBytes (17, 0, 0, 0, 0, 0, 0)[1], 0x1F);
        }

        beginTest ("isRecordableVoiceParamChange == legacy sendToOutputs test");
        {
            juce::uint8 ok[11]   = { 0xF0, 0x43, 0x12, 0x34, 0x56, 0, 0, 0x10, 0, 0x40, 0xF7 };
            juce::uint8 sys[11]  = { 0xF0, 0x43, 0x10, 0x34, 0x0F, 0, 0, 0x01, 0, 0x01, 0xF7 };  // system setup
            juce::uint8 dev2[11] = { 0xF0, 0x43, 0x20, 0x34, 0x56, 0, 0, 0x10, 0, 0x40, 0xF7 };  // 0x2n
            juce::uint8 blk[11]  = { 0xF0, 0x43, 0x00, 0x7A, 0x56, 0, 0, 0x10, 0, 0x40, 0xF7 };  // bulk
            juce::uint8 maker[11]= { 0xF0, 0x41, 0x10, 0x34, 0x56, 0, 0, 0x10, 0, 0x40, 0xF7 };
            juce::uint8 nsx[11]  = { 0x90, 0x43, 0x10, 0x34, 0x56, 0, 0, 0x10, 0, 0x40, 0xF7 };  // pas F0
            expect (SyVoice::isRecordableVoiceParamChange (ok, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (sys, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (dev2, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (blk, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (maker, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (nsx, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (ok, 9));
            expect (! SyVoice::isRecordableVoiceParamChange (nullptr, 11));
            for (auto* v : { ok, sys, dev2, blk, maker, nsx })
                for (int n : { 9, 11 })
                    expect (SyVoice::isRecordableVoiceParamChange (v, n) == legacyRecordable (v, n));
        }

        beginTest ("prepareEditBufferBulk == legacy setBulkDevice + retarget (template + vector)");
        {
            for (int i = 0; i < (int) VoiceTemplates::list().size(); ++i)
            {
                auto a = VoiceTemplates::make (i), b = a;
                expect (SyVoice::prepareEditBufferBulk ((juce::uint8*) a.getData(), (int) a.getSize(), 5));
                SyVoice::setBulkDevice ((juce::uint8*) b.getData(), (int) b.getSize(), 5);
                SyVoice::retargetVoiceToEditBuffer ((juce::uint8*) b.getData(), (int) b.getSize());
                expect (a == b);
                auto* d = (const juce::uint8*) a.getData();
                const int n = (int) a.getSize();
                expectEquals ((int) d[2], 0x04);
                expectEquals ((int) d[30], 0x7F);
                expectEquals ((int) d[31], 0x00);
                expectEquals ((int) d[n - 2], (int) SyVoice::yamahaChecksum (d + 6, n - 8));
                expect (SyVoice::verifyYamahaBulkChecksum (d, n));
            }
            juce::uint8 p[11] = { 0xF0, 0x43, 0x10, 0x34, 1, 2, 3, 4, 5, 6, 0xF7 };   // pas un bulk : intact
            const juce::uint8 p0[11] = { 0xF0, 0x43, 0x10, 0x34, 1, 2, 3, 4, 5, 6, 0xF7 };
            expect (! SyVoice::prepareEditBufferBulk (p, 11, 9));
            expect (std::equal (p, p + 11, p0));
            juce::uint8 tiny[20] = { 0xF0, 0x43, 0x00, 0x7A };            // bulk mais < 34 : refuse
            tiny[19] = 0xF7;
            expect (! SyVoice::prepareEditBufferBulk (tiny, 20, 9));
            juce::uint8 unframed[40] = { 0xF0, 0x43, 0x00, 0x7A };        // pas de F7 final : refuse
            expect (! SyVoice::prepareEditBufferBulk (unframed, 40, 9));
            expectEquals ((int) unframed[30], 0x00);
        }

        beginTest ("constat 5: window title without a voice name has no stale name");
        {
            const juce::String base = juce::String ("Sysex77 v") + Sysex77::kVersion;
            expect (Sysex77::windowTitle ({}) == base);
            expect (Sysex77::windowTitle ("BRASS 1") == base + "  -  BRASS 1");
        }
    }
};

static VoiceSessionHelpersTests voiceSessionHelpersTests;

//==============================================================================
// Caracterisation de VoiceSession (seul proprietaire de la logique de session, branchee
// dans MidiDemo). Ports factices ; le faux `send` modelise sendToOutputs :
// observeOutgoing AVANT le gate Out, puis coupe si Out OFF (note-off exceptes), puis trace.
class VoiceSessionTests : public juce::UnitTest
{
public:
    VoiceSessionTests() : juce::UnitTest ("VoiceSession", "Sysex77") {}

    using Bytes = std::vector<juce::uint8>;

    struct Rig
    {
        std::vector<Bytes> out;                        // ce qui sort reellement (apres gate)
        std::vector<std::array<int, 6>> replay;
        std::vector<int> replayDepth;                  // suppressSend vu pendant chaque rejeu
        std::vector<bool> locks;
        std::vector<VoiceSession::HistoryState> states;
        std::vector<std::function<void()>> posted;
        struct T { double due; int ms; std::function<void()> fn; };
        std::vector<T> timers;
        int editHappened = 0;
        double now = 1000.0;
        bool outOn = true, mouse = false;
        int device = 3;
        std::unique_ptr<VoiceSession> s;
        std::function<void (const juce::MidiMessage&)> sendToOutputs;

        Rig()
        {
            VoiceSession::Ports p;
            sendToOutputs = [this] (const juce::MidiMessage& m)
            {
                if (s != nullptr) s->observeOutgoing (m.getRawData(), m.getRawDataSize(), mouse);
                if (! outOn && ! m.isNoteOff()) return;
                out.push_back (Bytes (m.getRawData(), m.getRawData() + m.getRawDataSize()));
            };
            p.send = sendToOutputs;
            p.replayParam = [this] (int g, int hi, int lo, int pp, int v1, int v2)
            {
                replay.push_back ({ { g, hi, lo, pp, v1, v2 } });
                replayDepth.push_back (SysexBus::get().suppressSend.load());
            };
            p.lockEditing  = [this] (bool l) { locks.push_back (l); };
            p.editHappened = [this] { ++editHappened; };
            p.nowMs        = [this] { return now; };
            p.post         = [this] (std::function<void()> f) { posted.push_back (std::move (f)); };
            p.schedule     = [this] (int ms, std::function<void()> f) { timers.push_back ({ now + ms, ms, std::move (f) }); };
            p.sendEnabled  = [this] { return outOn; };
            p.device       = [this] { return device; };
            p.historyState = [this] (const VoiceSession::HistoryState& h) { states.push_back (h); };
            s = std::make_unique<VoiceSession> (p);
        }

        void drain()   // boucle de messages : FIFO, les posts imbriques passent en fin de file
        {
            for (size_t i = 0; i < posted.size(); ++i) { auto f = posted[i]; f(); }
            posted.clear();
        }
        void stepOne()   // un seul message de la file (pour entrelacer les sauts FIFO)
        {
            if (posted.empty()) return;
            auto f = posted.front();
            posted.erase (posted.begin());
            f();
        }
        void advance (double ms)
        {
            now += ms;
            auto due = timers;
            timers.clear();
            for (auto& t : due) { if (t.due <= now) t.fn(); else timers.push_back (t); }
        }
        // Edit utilisateur tel qu'il arrive a sendToOutputs (widget -> bus -> choke-point).
        void userEdit (EditHist::Key k, int v)
        {
            auto b = SyVoice::paramChangeBytes (device, EditHist::gOf (k), EditHist::hiOf (k), EditHist::loOf (k),
                                                EditHist::pOf (k), EditHist::v1Of (v), EditHist::v2Of (v));
            sendToOutputs (juce::MidiMessage::createSysExMessage (b.data(), 9));
        }
    };

    // Attendu derive du code MidiDemo actuel : bulk = copie, device byte 0n, $7F/0, checksum.
    static Bytes expectedLoadBulk (const juce::MemoryBlock& tpl, int device)
    {
        Bytes e ((const juce::uint8*) tpl.getData(), (const juce::uint8*) tpl.getData() + tpl.getSize());
        const int n = (int) e.size();
        e[2] = (juce::uint8) (device - 1);
        e[30] = 0x7F; e[31] = 0x00;
        e[(size_t) n - 2] = SyVoice::yamahaChecksum (e.data() + 6, n - 8);
        return e;
    }
    static Bytes paramMsg (int device, EditHist::Key k, int v)
    {
        auto b = SyVoice::paramChangeBytes (device, EditHist::gOf (k), EditHist::hiOf (k), EditHist::loOf (k),
                                            EditHist::pOf (k), EditHist::v1Of (v), EditHist::v2Of (v));
        Bytes m { 0xF0 }; m.insert (m.end(), b.begin(), b.end()); m.push_back (0xF7);
        return m;
    }

    void runTest() override
    {
        using namespace EditHist;
        auto& bus = SysexBus::get();
        const int depth0 = bus.suppressSend.load();
        const auto tpl = VoiceTemplates::make (0);
        const auto params = SyVoice::voiceBlobToParams ((const juce::uint8*) tpl.getData(), (int) tpl.getSize());
        // Cle editee : premier param decode (connue de l'original).
        const Key k = keyOf (params[0].group, params[0].addrHi, params[0].addrLo, params[0].param);
        const int orig = valOf (params[0].v1, params[0].value);
        const int edited = orig == 5 ? 6 : 5;

        auto loadTpl = [&] (Rig& r)
        {
            const bool ok = r.s->loadBlock (tpl) == VoiceSession::LoadResult::Loaded;
            r.drain();
            return ok;
        };

        beginTest ("load: one bulk to the edit buffer ($7F) at the current device, verbatim replay");
        {
            Rig r;
            int type = -1; juce::String name; size_t outAtHeader = 99, replayAtHeader = 99;
            const bool ok = r.s->loadBlock (tpl, [&] (int t, const juce::String& n)
                                            { type = t; name = n; outAtHeader = r.out.size(); replayAtHeader = r.replay.size(); })
                            == VoiceSession::LoadResult::Loaded;
            expect (ok);
            expectEquals (type, 3);
            expect (name == juce::String ((const char*) tpl.getData() + 33, 10).trimEnd());
            expect (name.isNotEmpty());
            expectEquals ((int) r.out.size(), 1);
            expect (r.out[0] == expectedLoadBulk (tpl, 3));
            expect (outAtHeader == 1 && replayAtHeader == 0);       // ordre : bulk -> props -> rejeu
            expectEquals ((int) r.replay.size(), params.size());
            bool verbatim = (int) r.replay.size() == params.size(), guarded = true;
            for (int i = 0; verbatim && i < params.size(); ++i)
            {
                const auto& vp = params.getReference (i);
                verbatim = r.replay[(size_t) i] == std::array<int, 6> { { vp.group, vp.addrHi, vp.addrLo, vp.param, vp.v1, vp.value } };
            }
            for (int dpt : r.replayDepth) guarded = guarded && dpt >= depth0 + 2;   // settle + ScopedEchoSuppress
            expect (verbatim);
            expect (guarded);
            expect (r.s->history().isLoaded() && ! r.s->history().canUndo());
            expect (! r.states.empty() && r.states.back().loaded && ! r.states.back().showingA);
            expect (r.locks.empty() && r.editHappened == 0 && r.timers.empty());
            expectEquals (bus.suppressSend.load(), depth0 + 1);    // settle tenu
            r.drain();
            expectEquals (bus.suppressSend.load(), depth0);        // relache apres 2 sauts FIFO
        }

        beginTest ("edits during settle are not recorded; recorded once settled");
        {
            Rig r;
            r.s->loadBlock (tpl);
            r.userEdit (k, edited);
            expect (! r.s->history().canUndo());
            r.drain();
            r.userEdit (k, edited == 5 ? 7 : 8);
            expect (r.s->history().canUndo());
            expectEquals (bus.suppressSend.load(), depth0);
        }

        beginTest ("edit -> undo/redo: sent at the CURRENT device, not recorded");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            expectEquals (r.s->history().numSteps(), 1);
            r.device = 5;
            const size_t n0 = r.out.size(), rp0 = r.replay.size();
            r.s->undo();
            expectEquals ((int) r.out.size(), (int) n0 + 1);
            expect (r.out.back() == paramMsg (5, k, orig));
            expect (r.out.back()[2] == 0x14);
            expectEquals ((int) r.replay.size(), (int) rp0 + 1);
            expect (r.replayDepth.back() > depth0);
            expect (! r.s->history().canUndo() && r.s->history().canRedo());   // undo's own send not recorded
            r.s->redo();
            expect (r.out.back() == paramMsg (5, k, edited));
            expect (r.s->history().canUndo() && ! r.s->history().canRedo());
            expectEquals (r.s->history().numSteps(), 1);
            expect (r.states.back().status.isEmpty());
        }

        beginTest ("A: whole original bulk (current device), editing locked, editHappened; B: diff after delay");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            r.device = 2;
            const size_t n0 = r.out.size();
            r.s->setShowingA (true);
            expectEquals ((int) r.out.size(), (int) n0 + 1);
            expect (r.out.back() == expectedLoadBulk (tpl, 2));
            expect (r.s->isShowingA() && r.locks.back() && r.editHappened == 1);
            expect (r.states.back().showingA);
            // While A is shown nothing is recorded.
            r.userEdit (k, 99);
            expectEquals (r.s->history().numSteps(), 1);
            r.out.pop_back();   // (le faux edit est sorti : en vrai l'UI est verrouillee)

            const size_t n1 = r.out.size(), rp1 = r.replay.size();
            r.s->setShowingA (false);
            expectEquals ((int) r.out.size(), (int) n1);                 // pas encore : SY77 occupe
            expectEquals ((int) r.replay.size(), (int) rp1 + 1);         // UI tout de suite (diff)
            expect (! r.s->isShowingA() && ! r.locks.back() && r.editHappened == 1);
            expectEquals ((int) r.timers.size(), 1);
            expectEquals (r.timers[0].ms, (int) ((double) tpl.getSize() / 3.0 + 60.0) + 1);
            r.advance (r.timers[0].ms);
            expectEquals ((int) r.out.size(), (int) n1 + 1);
            expect (r.out.back() == paramMsg (2, k, edited));
            expectEquals (r.s->history().numSteps(), 1);                  // B's send not recorded
        }

        beginTest ("B delayed send cancelled when historyGen changes (edit / A again / undo)");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            r.s->setShowingA (true);
            r.s->setShowingA (false);
            const Key k2 = keyOf (params[1].group, params[1].addrHi, params[1].addrLo, params[1].param);
            const int v2 = valOf (params[1].v1, params[1].value) == 3 ? 4 : 3;
            r.now += 1000;                          // pas de regroupement avec l'edit precedent
            r.userEdit (k2, v2);                    // ++historyGen
            const size_t n0 = r.out.size();   // (l'edit a avance le diff B, cf. constat 4)
            r.advance (10000);
            expectEquals ((int) r.out.size(), (int) n0);   // le timer ne renvoie rien

            r.s->setShowingA (true);
            r.s->setShowingA (false);
            r.s->setShowingA (true);                // A again: showingA + gen
            const size_t n1 = r.out.size();
            r.advance (10000);
            expectEquals ((int) r.out.size(), (int) n1);

            // Undo while A (bulk fraichement envoye) : setShowingA(false) programme le diff B
            // (UI deja sur B), puis undo l'AVANCE (flushPendingB) avant ++historyGen.
            // CORRIGE (revue, bug 1) : ce test exigeait auparavant que seules les valeurs d'undo
            // sortent (B annule) -> le synthe restait sur le bulk A alors que l'UI affichait B.
            // Attendu desormais : diff B (ordre de la map) puis la valeur d'undo, rien ensuite.
            r.s->setShowingA (false);
            r.s->setShowingA (true);
            const auto diffB = r.s->history().diffFromOriginal();
            expect (diffB.size() == 2);
            const size_t n2 = r.out.size();
            r.s->undo();
            expectEquals ((int) r.out.size(), (int) (n2 + diffB.size() + 1));
            for (size_t i = 0; i < diffB.size() && n2 + i < r.out.size(); ++i)
                expect (r.out[n2 + i] == paramMsg (3, diffB[i].first, diffB[i].second));
            expect (r.out.back() == paramMsg (3, k2, valOf (params[1].v1, params[1].value)));
            expectEquals ((int) r.timers.size(), 1);   // programme...
            r.advance (10000);                         // ...deja envoye : le timer ne renvoie rien
            expectEquals ((int) (r.out.size()), (int) (n2 + diffB.size() + 1));
        }

        beginTest ("B sends immediately once the bulk is absorbed");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            r.s->setShowingA (true);
            r.now += 5000;
            const size_t n0 = r.out.size();
            r.s->setShowingA (false);
            expect (r.timers.empty());
            expectEquals ((int) r.out.size(), (int) n0 + 1);
            expect (r.out.back() == paramMsg (3, k, edited));
        }

        beginTest ("revert: original bulk, undoable step, editHappened");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            expect (r.s->hasUnsavedEdits());
            const size_t n0 = r.out.size();
            r.s->revert();
            expectEquals ((int) r.out.size(), (int) n0 + 1);
            expect (r.out.back() == expectedLoadBulk (tpl, 3));
            expect (r.editHappened == 1 && ! r.s->hasUnsavedEdits());
            expectEquals (r.s->history().numSteps(), 2);
            r.s->undo();
            expect (r.out.back() == paramMsg (3, k, edited));
            expect (r.s->hasUnsavedEdits());
            // revert from A: unlocks first
            r.s->setShowingA (true);
            r.s->revert();
            expect (! r.s->isShowingA() && ! r.locks.back());
        }

        beginTest ("Out OFF: nothing is emitted but the edit is recorded; B not delayed");
        {
            Rig r; r.outOn = false;
            loadTpl (r);
            expect (r.out.empty() && r.s->history().isLoaded());
            r.userEdit (k, edited);
            expect (r.out.empty());
            expect (r.s->history().canUndo());
            r.s->setShowingA (true);
            r.s->setShowingA (false);
            expect (r.timers.empty());               // bulkBusyUntilMs pas mis a jour
            r.s->undo();
            r.s->revert();
            expect (r.out.empty());
            expectEquals (bus.suppressSend.load(), depth0);
        }

        beginTest ("sz <= 42: bulk still sent + settle, no replay, previous history discarded");
        {
            juce::uint8 small[40] = { 0xF0, 0x43, 0x00, 0x7A };
            small[39] = 0xF7;
            Bytes e (small, small + 40);
            e[2] = 0x02; e[30] = 0x7F; e[31] = 0x00; e[38] = SyVoice::yamahaChecksum (e.data() + 6, 32);
            {
                Rig r;
                int type = -1; juce::String name; bool called = false;
                const auto res = r.s->loadBlock (juce::MemoryBlock (small, 40),
                                                 [&] (int t, const juce::String& n) { called = true; type = t; name = n; });
                expect (res == VoiceSession::LoadResult::Discarded && ! called && type == -1 && name.isEmpty());
                expectEquals ((int) r.out.size(), 1);
                expect (r.out[0] == e);
                expect (r.replay.empty() && ! r.s->history().isLoaded());
                // L'etat publie dit « rien de charge » (HistoryBar desactivee).
                expect (r.states.size() == 1 && ! r.states.back().loaded);
                expectEquals (bus.suppressSend.load(), depth0 + 1);
                r.drain();
                expectEquals (bus.suppressSend.load(), depth0);
            }
            {
                Rig r; loadTpl (r);
                r.userEdit (k, edited);
                r.s->loadBlock (juce::MemoryBlock (small, 40));
                r.drain();
                // L'edit buffer ne contient plus la voix precedente : rien a annuler ni a
                // comparer, et A / undo / revert n'emettent plus rien.
                expect (! r.s->history().isLoaded() && ! r.s->history().canUndo());
                expect (! r.s->hasUnsavedEdits() && ! r.s->canRevert());
                const auto sent = r.out.size();
                r.s->setShowingA (true);
                r.s->undo();
                r.s->revert();
                r.drain();
                expectEquals ((int) r.out.size(), (int) sent);
                expect (! r.s->isShowingA());
            }
        }

        beginTest ("incoming params keep the state (noteRx), ignored for group 0F and while A");
        {
            Rig r; loadTpl (r);
            const int rx = edited == 5 ? 9 : 10;
            const juce::uint8 d[6] = { (juce::uint8) gOf (k), (juce::uint8) hiOf (k), (juce::uint8) loOf (k),
                                       (juce::uint8) pOf (k), (juce::uint8) v1Of (rx), (juce::uint8) v2Of (rx) };
            r.s->observeIncomingParam (d);
            expect (! r.s->history().canUndo() && r.s->hasUnsavedEdits());
            r.userEdit (k, edited);
            r.s->undo();
            expect (r.out.back() == paramMsg (3, k, rx));
            const juce::uint8 sys[6] = { 0x0F, 0, 0, 0x01, 0, 0x05 };
            const int diffBefore = (int) r.s->history().diffFromOriginal().size();
            r.s->observeIncomingParam (sys);
            expectEquals ((int) r.s->history().diffFromOriginal().size(), diffBefore);

            // While A: the received value is NOT noted -> undo restores the pre-A value.
            r.now += 1000;
            r.userEdit (k, edited);                     // state = edited (new step)
            r.s->setShowingA (true);
            const int third = edited == 5 ? 11 : 12;
            const juce::uint8 dA[6] = { d[0], d[1], d[2], d[3], (juce::uint8) v1Of (third), (juce::uint8) v2Of (third) };
            r.s->observeIncomingParam (dA);
            r.s->setShowingA (false);
            r.advance (10000);
            r.now += 1000;
            r.userEdit (k, edited == 5 ? 13 : 14);
            r.s->undo();
            expect (r.out.back() == paramMsg (3, k, edited));
        }

        beginTest ("constat 1: non-bulk or < 34-byte block is refused: nothing sent, history kept");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            const size_t n0 = r.out.size();
            juce::uint8 tiny[20] = { 0xF0, 0x43, 0x00, 0x7A };        // bulk voix, mais < 34
            tiny[19] = 0xF7;
            juce::uint8 notBulk[60] = { 0xF0, 0x43, 0x00, 0x7B };     // > 42 mais pas 7A
            notBulk[59] = 0xF7;
            for (auto mb : { juce::MemoryBlock (tiny, sizeof (tiny)), juce::MemoryBlock (notBulk, sizeof (notBulk)) })
            {
                expect (r.s->loadBlock (mb) == VoiceSession::LoadResult::Refused);
                expectEquals ((int) r.out.size(), (int) n0);                    // rien n'est emis
                expectEquals (bus.suppressSend.load(), depth0);                 // pas de settle
                expect (r.s->history().isLoaded() && r.s->history().canUndo()); // historique intact
                r.drain();
            }
            expectEquals (bus.suppressSend.load(), depth0);
        }

        beginTest ("constat 2: after unload an incoming param + revert + undo emit nothing");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            juce::uint8 small[40] = { 0xF0, 0x43, 0x00, 0x7A };
            small[39] = 0xF7;
            r.s->loadBlock (juce::MemoryBlock (small, 40));
            r.drain();
            expect (! r.s->history().isLoaded());
            const size_t n0 = r.out.size();
            const int rx = edited == 5 ? 9 : 10;
            const juce::uint8 d[6] = { (juce::uint8) gOf (k), (juce::uint8) hiOf (k), (juce::uint8) loOf (k),
                                       (juce::uint8) pOf (k), (juce::uint8) v1Of (rx), (juce::uint8) v2Of (rx) };
            r.s->observeIncomingParam (d);
            expect (! r.s->hasUnsavedEdits());
            r.s->revert();
            r.s->undo();
            r.drain();
            r.advance (10000);
            expectEquals ((int) r.out.size(), (int) n0);
            expect (! r.s->history().canUndo() && r.editHappened == 0);
        }

        beginTest ("constat 3: two queued loads: echoes of the 2nd are not recorded after the 1st settles");
        {
            Rig r;
            r.s->loadBlock (tpl);
            r.s->loadBlock (tpl);
            expectEquals ((int) r.posted.size(), 2);
            r.stepOne();            // L1 saut 1 -> poste L1 saut 2
            r.stepOne();            // L2 saut 1 -> poste L2 saut 2
            r.stepOne();            // L1 saut 2 : le 1er chargement est stabilise...
            r.userEdit (k, edited); // ...mais un echo du 2e arrive encore
            expect (! r.s->history().canUndo());
            expect (! r.s->hasUnsavedEdits());
            r.drain();              // L2 saut 2
            expectEquals (bus.suppressSend.load(), depth0);
            r.now += 1000;
            r.userEdit (k, edited == 5 ? 7 : 8);   // une fois tout stabilise : enregistre
            expect (r.s->history().canUndo());
        }

        beginTest ("constat 4: an edit during the bulk window sends the pending B diff first, then the edit");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            r.s->setShowingA (true);
            r.s->setShowingA (false);
            expectEquals ((int) r.timers.size(), 1);
            const Key k2 = keyOf (params[1].group, params[1].addrHi, params[1].addrLo, params[1].param);
            const int v2 = valOf (params[1].v1, params[1].value) == 3 ? 4 : 3;
            const size_t n0 = r.out.size();
            r.now += 10;                            // avant l'echeance du B differe
            r.userEdit (k2, v2);
            expectEquals ((int) r.out.size(), (int) n0 + 2);
            if (r.out.size() == n0 + 2)
            {
                expect (r.out[n0]     == paramMsg (3, k, edited));   // diff B d'abord
                expect (r.out[n0 + 1] == paramMsg (3, k2, v2));      // puis l'edit
            }
            r.advance (10000);
            expectEquals ((int) r.out.size(), (int) n0 + 2);         // rien apres l'echeance
        }

        beginTest ("bug 1: undo/redo during the B delay send the pending B diff first, then their values");
        {
            {
                Rig r; loadTpl (r);
                r.userEdit (k, edited);
                r.s->setShowingA (true);
                r.s->setShowingA (false);          // UI sur B, envoi B differe (SY77 occupe)
                expectEquals ((int) r.timers.size(), 1);
                const size_t n0 = r.out.size();
                r.now += 10;
                r.s->undo();
                expectEquals ((int) r.out.size(), (int) n0 + 2);
                if (r.out.size() == n0 + 2)
                {
                    expect (r.out[n0]     == paramMsg (3, k, edited));   // diff B d'abord
                    expect (r.out[n0 + 1] == paramMsg (3, k, orig));     // puis l'undo
                }
                r.advance (10000);
                expectEquals ((int) r.out.size(), (int) n0 + 2);
            }
            {
                Rig r; loadTpl (r);
                const Key k2 = keyOf (params[1].group, params[1].addrHi, params[1].addrLo, params[1].param);
                const int v2 = valOf (params[1].v1, params[1].value) == 3 ? 4 : 3;
                r.userEdit (k, edited);
                r.now += 1000;
                r.userEdit (k2, v2);
                r.s->undo();                        // diff = { k }
                r.s->setShowingA (true);
                r.s->setShowingA (false);
                expectEquals ((int) r.timers.size(), 1);
                const size_t n0 = r.out.size();
                r.now += 10;
                r.s->redo();
                expectEquals ((int) r.out.size(), (int) n0 + 2);
                if (r.out.size() == n0 + 2)
                {
                    expect (r.out[n0]     == paramMsg (3, k, edited));   // diff B d'abord
                    expect (r.out[n0 + 1] == paramMsg (3, k2, v2));      // puis le redo
                }
                r.advance (10000);
                expectEquals ((int) r.out.size(), (int) n0 + 2);
            }
        }

        beginTest ("bug 2: values received for an undecoded key (AFM common reserve) are not unsaved edits");
        {
            // Cle filtre (groupe 09, type FTYPE N2 0x00 : encodage bulk non verifie, donc NON
            // decode) absente des params decodes, mais enregistrable (g != 0F). Les effets et
            // le LFO sont desormais decodes du dump : ils ne peuvent plus servir ici.
            Key fx = 0;
            for (int p = 0; p < 128 && fx == 0; ++p)
            {
                const Key c = keyOf (0x05, 0, 0, p);   // 1er non decode = N2 0x14 (reserve)
                bool decoded = false;
                for (auto& vp : params)
                    decoded = decoded || keyOf (vp.group, vp.addrHi, vp.addrLo, vp.param) == c;
                if (! decoded) fx = c;
            }
            expect (fx != 0);
            {
                const Bytes probe = paramMsg (3, fx, 1);
                expect (SyVoice::isRecordableVoiceParamChange (probe.data(), (int) probe.size()));
            }
            auto rx = [&] (Rig& r, Key key, int v)
            {
                const juce::uint8 d[6] = { (juce::uint8) gOf (key), (juce::uint8) hiOf (key), (juce::uint8) loOf (key),
                                           (juce::uint8) pOf (key), (juce::uint8) v1Of (v), (juce::uint8) v2Of (v) };
                r.s->observeIncomingParam (d);
            };
            {
                Rig r; loadTpl (r);
                rx (r, fx, 10);
                rx (r, fx, 20);                     // flux d'un bouton tourne sur le SY77
                expect (! r.s->hasUnsavedEdits());
                expect (! r.s->history().hasDiff() && r.s->history().diffFromOriginal().empty());
                r.s->setShowingA (true);
                const size_t n0 = r.out.size();
                r.s->setShowingA (false);
                r.advance (10000);
                expectEquals ((int) r.out.size(), (int) n0);   // B ne renvoie pas la valeur recue

                // Un edit UTILISATEUR de cette cle reste compte, meme apres une valeur recue.
                r.now += 1000;
                r.userEdit (fx, 30);
                expect (r.s->hasUnsavedEdits() && r.s->history().hasDiff());
                rx (r, fx, 40);
                expect (r.s->hasUnsavedEdits() && r.s->history().hasDiff());
                // Undo -> retour a la derniere valeur recue avant l'edit : plus d'ecart.
                r.s->undo();
                expect (r.out.back() == paramMsg (3, fx, 20));
                expect (! r.s->hasUnsavedEdits());
            }
            {
                Rig r; loadTpl (r);
                r.userEdit (fx, 30);                // cle jamais recue : edit compte (inchange)
                expect (r.s->hasUnsavedEdits());
                const auto diff = r.s->history().diffFromOriginal();
                expect (diff.size() == 1 && diff[0] == KV (fx, 30));
            }
        }

        beginTest ("bug 3: blank-named real voice is Loaded with an empty name; placeholder only if unknown");
        {
            juce::MemoryBlock blank (tpl);
            for (int i = 33; i < 43; ++i) ((juce::uint8*) blank.getData())[i] = ' ';
            Rig r;
            juce::uint8 small[40] = { 0xF0, 0x43, 0x00, 0x7A };
            small[39] = 0xF7;
            bool called = false;
            expect (r.s->loadBlock (juce::MemoryBlock (small, 40), [&] (int, const juce::String&) { called = true; })
                    == VoiceSession::LoadResult::Discarded);
            expect (! called);
            juce::String name = "x";
            expect (r.s->loadBlock (blank, [&] (int, const juce::String& n) { called = true; name = n; })
                    == VoiceSession::LoadResult::Loaded);
            expect (called && name.isEmpty());
            // Le champ nom : texte neutre pour une voix INCONNUE seulement, rien pour un nom vide.
            expect (SyVoice::voiceNameFieldPlaceholder (false).isEmpty());
            expect (SyVoice::voiceNameFieldPlaceholder (true) == TRANS ("(unknown voice)"));
            r.drain();
        }

        beginTest ("lifetime: settle decrement and B cancellation survive session destruction");
        {
            Rig r;
            r.s->loadBlock (tpl);
            expectEquals (bus.suppressSend.load(), depth0 + 1);
            r.s.reset();
            r.drain();
            expectEquals (bus.suppressSend.load(), depth0);

            Rig r2; loadTpl (r2);
            r2.userEdit (k, edited);
            r2.s->setShowingA (true);
            r2.s->setShowingA (false);
            expectEquals ((int) r2.timers.size(), 1);
            const size_t n0 = r2.out.size();
            r2.s.reset();
            r2.advance (10000);
            expectEquals ((int) r2.out.size(), (int) n0);
        }

        bus.suppressSend.store (depth0);
    }
};

static VoiceSessionTests voiceSessionTests;
