// Tests unitaires : classifieurs purs de la reception (followSynthTarget, classifyConnReply),
// compares a une copie de l'ancien code inline de MidiDemo::handleAsyncUpdate.
#pragma once

class RxClassifierTests : public juce::UnitTest
{
public:
    RxClassifierTests() : juce::UnitTest ("RxClassifiers", "Sysex77") {}

    static int legacyTarget (int g, int p)
    {
        int target = -1;
        if (g == 0x08)                                   target = 3;
        else if (g == 0x02 && p >= 0x28 && p <= 0x3D)    target = 4;
        else if (g == 0x02 || g == 0x03 || g == 0x05 || g == 0x07
                 || g == 0x09 || g == 0x0A
                 || (g & 0x0F) == 0x06)                  target = 2;
        else if (SyVoice::isMultiGroup (g))              target = 5;
        return target;
    }

    void runTest() override
    {
        beginTest ("followSynthTarget == legacy inline code, all groups x all N2");
        int mismatches = 0;
        for (int g = 0; g < 128; ++g)
            for (int p = 0; p < 128; ++p)
                if (SyVoice::followSynthTarget (g, p) != legacyTarget (g, p)) ++mismatches;
        expectEquals (mismatches, 0);

        beginTest ("classifyConnReply == legacy (yamaha / identity / isYam)");
        juce::Random r (77);
        int bad = 0;
        for (int k = 0; k < 20000; ++k)
        {
            juce::uint8 id[8];
            for (auto& b : id) b = (juce::uint8) r.nextInt (128);
            if (k % 3 == 0) { id[0] = 0x7E; id[2] = 0x06; id[3] = 0x02; }
            if (k % 7 == 0) id[4] = 0x43;
            const int n = r.nextInt (9);
            const bool yamaha   = (n >= 1 && id[0] == 0x43);
            const bool identity = (n >= 5 && id[0] == 0x7E && id[2] == 0x06 && id[3] == 0x02);
            const auto c = SyVoice::classifyConnReply (id, n);
            if ((c != SyVoice::ConnReply::none) != (yamaha || identity)) ++bad;
            if (c != SyVoice::ConnReply::none
                && (c != SyVoice::ConnReply::identity) != (yamaha || (identity && id[4] == 0x43))) ++bad;
        }
        expectEquals (bad, 0);
    }
};

static RxClassifierTests rxClassifierTests;
