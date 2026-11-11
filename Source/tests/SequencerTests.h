// Tests unitaires : Sequencer (inclus par Source/Tests.h, ordre d'enregistrement conserve).
#pragma once

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
