// Tests unitaires : TestBench (inclus par Source/Tests.h, ordre d'enregistrement conserve).
#pragma once

//==============================================================================
// Banc de test hardware (TestBench.h) : verdicts de la boucle fermee dump -> envoi -> dump.
class TestBenchTests : public juce::UnitTest
{
public:
    TestBenchTests() : juce::UnitTest ("TestBench", "Sysex77") {}

    static juce::MemoryBlock fakeVoice (int size)
    {
        juce::MemoryBlock b ((size_t) size, true);
        auto* d = (juce::uint8*) b.getData();
        d[0] = 0xF0; d[1] = 0x43; d[2] = 0x00; d[3] = 0x7A;
        std::memcpy (d + 6, "LM  8101VC", 10);
        d[30] = 0x7F; d[98] = 100;
        d[size - 2] = SyVoice::yamahaChecksum (d + 6, size - 8);
        d[size - 1] = 0xF7;
        return b;
    }
    static juce::MemoryBlock with (juce::MemoryBlock b, int off, int v)
    {
        auto* d = (juce::uint8*) b.getData();
        d[off] = (juce::uint8) v;
        d[b.getSize() - 2] = SyVoice::yamahaChecksum (d + 6, (int) b.getSize() - 8);
        return b;
    }

    void runTest() override
    {
        using namespace SyBench;
        const auto a = fakeVoice (469);

        beginTest ("isVoiceBulk recognises a framed 8101VC block only");
        expect (isVoiceBulk ((const juce::uint8*) a.getData(), (int) a.getSize()));
        auto bad = a; ((juce::uint8*) bad.getData())[14] = 'M';
        expect (! isVoiceBulk ((const juce::uint8*) bad.getData(), (int) bad.getSize()));

        beginTest ("diffOffsets ignores the checksum byte, flags size mismatch");
        expect (diffOffsets (a, a).isEmpty());
        expect (diffOffsets (a, with (a, 98, 7)) == juce::Array<int> { 98 });
        expect (diffOffsets (a, fakeVoice (341)).contains (-1));

        beginTest ("classify verdicts");
        expect (classify (a, with (a, 98, 37), 98, 37).verdict == Verdict::Ok);
        expect (classify (a, with (with (a, 98, 37), 120, 1), 98, 37).verdict == Verdict::OkWithSideEffects);
        expect (classify (a, a, 98, 37).verdict == Verdict::NoChange);
        expect (classify (a, with (a, 99, 37), 98, 37).verdict == Verdict::WrongOffset);
        expect (classify (a, with (a, 98, 36), 98, 37).verdict == Verdict::WrongValue);
        expect (classify (a, fakeVoice (341), 98, 37).verdict == Verdict::SizeMismatch);

        beginTest ("lot transplant: plan, then OK / ignored / other value / side effect");
        {
            auto sv = with (fakeVoice (466), 32, 0x03);               // 1 AFM poly : decodable
            auto t  = with (with (with (sv, 98, 50), 99, 3), 100, 70); // ELVL, ELDT, ELNS different
            const auto plan = planLot (sv, t);
            expect (plan.compatible);
            expectEquals (plan.toSend.size(), 3);
            expect (plan.uncovered > 0);
            expect (! planLot (sv, with (t, 32, 0x05)).compatible);   // autre type -> refuse
            // Relu : ELVL exact, ELDT ignore (reste a 0), ELNS autre valeur, + effet de bord @101.
            auto after = with (with (with (sv, 98, 50), 100, 71), 101, 9);
            const auto r = checkLot (sv, plan, after);
            expectEquals (r.ok, 1);
            expectEquals (r.ignored, 1);
            expectEquals (r.other, 1);
            expectEquals (r.sideEffects.size(), 1);
        }

        beginTest ("multiPicks covers each VMEM code and switch state once");
        {
            juce::Array<SyMulti::Multi> ms;
            SyMulti::Multi m; m.loaded = true;
            for (int p = 0; p < SyMulti::kParts; ++p) { m.part[p].raw[0] = 0x40; m.part[p].raw[1] = 0; }
            m.part[3].raw[1] = 1; m.part[7].raw[0] = 0x04;
            ms.add (m);
            const auto picks = multiPicks (ms);
            expectEquals (picks.size(), 3);                          // part 1 (VMEM0,bit6=1,bit2=0), 4 (VMEM1), 8 (bit6=0,bit2=1)
            expectEquals (picks[1].part, 3);
            expectEquals (picks[2].part, 7);
        }
    }
};

static TestBenchTests testBenchTests;
