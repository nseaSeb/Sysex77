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
    }
};

static SysexUtilsTests sysexUtilsTests;
