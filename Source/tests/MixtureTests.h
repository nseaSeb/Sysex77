// Tests unitaires : Quadratic Mixture (SysexMix.h).
#pragma once
#include "../SysexMix.h"

class MixtureTests : public juce::UnitTest
{
public:
    MixtureTests() : juce::UnitTest ("Mixture", "Sysex77") {}

    static juce::MemoryBlock steelBlock() { return { Fixture::steel, sizeof (Fixture::steel) }; }
    static juce::MemoryBlock with (juce::MemoryBlock b, int off, int v)
    {
        auto* d = (juce::uint8*) b.getData();
        d[off] = (juce::uint8) v;
        d[b.getSize() - 2] = SyVoice::yamahaChecksum (d + 6, (int) b.getSize() - 8);
        return b;
    }
    static bool sameExceptName (const juce::MemoryBlock& a, const juce::MemoryBlock& b)
    {
        if (a.getSize() != b.getSize()) return false;
        auto* x = (const juce::uint8*) a.getData(); auto* y = (const juce::uint8*) b.getData();
        for (int i = 0; i < (int) a.getSize() - 2; ++i)
            if ((i < 33 || i > 42) && x[i] != y[i]) return false;
        return true;
    }

    void runTest() override
    {
        using namespace SyMix;
        const auto s = steelBlock();

        beginTest ("weights: exact at corners, equal at centre, sum to 1");
        for (int i = 0; i < 4; ++i)
        {
            const auto w = weights (i % 2, i / 2);
            expectWithinAbsoluteError (w[(size_t) i], 1.0, 1e-9);
        }
        for (auto v : weights (0.5, 0.5)) expectWithinAbsoluteError (v, 0.25, 1e-9);
        juce::Random r (5);
        for (int k = 0; k < 200; ++k)
        {
            const auto w = weights (r.nextDouble(), r.nextDouble());
            expectWithinAbsoluteError (w[0] + w[1] + w[2] + w[3], 1.0, 1e-9);
        }

        beginTest ("offset map: encode(decode(b)) == b (round trip, SteelStrng)");
        const auto map = buildOffsetMap (s);
        expect (map.size() > 200, "map " + juce::String ((int) map.size()));
        expect (encode (s, decode (s), map) == s);

        beginTest ("ELVL elem 1 maps to the known offset 98");
        auto it = map.find (keyOf (0x03, 0x00, 0x00, 0x00));
        expect (it != map.end() && it->second.off == 98);

        beginTest ("mix: corners are byte-identical (except name), centre interpolates whitelist only");
        std::array<juce::MemoryBlock, 4> c { s, with (s, 98, 20), with (s, 98, 60), with (with (s, 98, 100), 99, 5) };
        expect (incompatibility (c).isEmpty());
        for (int i = 0; i < 4; ++i)
        {
            const auto res = mix (c, i % 2, i / 2, map, "TEST");
            expectEquals (res.base, i);
            expect (sameExceptName (res.block, c[(size_t) i]));
        }
        const auto mid = mix (c, 0.5, 0.5, map, "MIX");
        const auto* d = (const juce::uint8*) mid.block.getData();
        expectEquals ((int) d[98], juce::roundToInt ((Fixture::steel[98] + 20 + 60 + 100) / 4.0));   // ELVL interpole
        expectEquals ((int) d[99], (int) Fixture::steel[99]);   // ELDT (s/m, hors liste) = coin dominant (0)
        expect (SyBank::isVoiceBlock (mid.block));
        expect (SyVoice::detectSynthKind (d, mid.block.getSize()) == SyVoice::SynthKind::SY77);
        expectEquals ((int) d[mid.block.getSize() - 2], (int) SyVoice::yamahaChecksum (d + 6, (int) mid.block.getSize() - 8));

        beginTest ("incompatibility: different types refused");
        std::array<juce::MemoryBlock, 4> bad { s, s, s, with (s, 32, 0x05) };
        expect (incompatibility (bad).isNotEmpty());
    }
};

static MixtureTests mixtureTests;
