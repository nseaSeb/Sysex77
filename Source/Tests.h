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
    }
};

static SysexUtilsTests sysexUtilsTests;
