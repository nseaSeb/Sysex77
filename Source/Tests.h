/*
  ==============================================================================

    Tests.h
    Tests unitaires JUCE (juce::UnitTest) pour la logique « pure » de Sysex77.
    Lancés via l'argument de ligne de commande --test (voir Main.cpp).

  ==============================================================================
*/

#pragma once
#include "SysexUtils.h"

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
            // 6 op×15 + (ELVL,ALGNUM,FCTOF1,FCTOF2,FFRES) + 4 pitch-rates + 2×7 filtre + VVOL.
            expectEquals (params.size(), 6 * 29 + 6 + 4 + 14 + 1);

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

            // 1 AFM MONO (type $00) partage la structure 1AFM -> doit charger aussi (Table 2).
            juce::MemoryBlock mono (steel, sizeof (steel));
            ((juce::uint8*) mono.getData())[32] = 0x00;
            expectEquals (SyVoice::voiceBlobToParams ((const juce::uint8*) mono.getData(),
                                                      (int) mono.getSize()).size(), 6 * 29 + 6 + 4 + 14 + 1);

            // 2 AFM / 4 AFM : N « layers » identiques (Table 2). On vérifie le compte et que
            // l'adressage par élément (addrHi = élément<<5) est bien produit.
            const int perElem = 6 * 29 + 6 + 4 + 14;   // op + 5 abs + 4 pitch-rates + 2×7 filtre
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
            expectEquals (params.size(), 198 + 9 + 1);   // AFM él.(119) + AWM él.(9) + VVOL

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
