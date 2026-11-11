// Tests unitaires : EditHistory (inclus par Source/Tests.h, ordre d'enregistrement conserve).
#pragma once

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
