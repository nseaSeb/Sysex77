/*
  ==============================================================================

    EditHistory.h
    Voice edit history at the SYSEX level: every voice param-change sent to the
    synth (F0 43 1n 34 g hi lo p v1 v2 F7) is recorded, keyed on g/hi/lo/p, so
    undo/redo, A/B compare and revert work for every screen (sliders, graphs,
    algorithm...) without touching each widget.

    History (pure, unit-tested): shadow state, original (decoded from the loaded
    voice), steps with gesture grouping, undo/redo, B diff, revert step.
    Hooks + HistoryBar: UI glue; MidiDemo installs the actions.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <map>

namespace EditHist
{
    using Key = juce::uint32;
    using KV  = std::pair<Key, int>;   // value = v1 << 7 | v2 ; -1 = unknown

    inline Key keyOf (int g, int hi, int lo, int p)
    {
        return (Key) ((g & 0xFF) << 24 | (hi & 0xFF) << 16 | (lo & 0xFF) << 8 | (p & 0xFF));
    }
    inline int valOf (int v1, int v2)  { return (v1 & 0x7F) << 7 | (v2 & 0x7F); }
    inline int gOf  (Key k) { return (int) (k >> 24) & 0xFF; }
    inline int hiOf (Key k) { return (int) (k >> 16) & 0xFF; }
    inline int loOf (Key k) { return (int) (k >> 8) & 0xFF; }
    inline int pOf  (Key k) { return (int) k & 0xFF; }
    inline int v1Of (int v) { return (v >> 7) & 0x7F; }
    inline int v2Of (int v) { return v & 0x7F; }

    struct Change { Key k; int oldV, newV; };
    struct Step   { std::vector<Change> changes; double lastMs = 0; };

    class History
    {
    public:
        static constexpr double kJoinMs = 300.0;   // edits closer than this form one step

        /** A voice was loaded: its decoded values become the original AND the state. */
        void load (const std::vector<KV>& orig)
        {
            original.clear();
            for (auto& kv : orig) original[kv.first] = kv.second;
            state = original;
            steps.clear();
            pos = 0;
            open = false;
            loaded = true;
        }
        bool isLoaded() const { return loaded; }

        /** A user edit went out. mouseDown keeps the current step open (one drag = one step). */
        void recordEdit (Key k, int v, double nowMs, bool mouseDown)
        {
            const auto it = state.find (k);
            const int old = it != state.end() ? it->second : -1;
            if (old == v) return;                      // no actual change
            state[k] = v;

            if ((int) steps.size() > pos) { steps.resize ((size_t) pos); open = false; }   // drop redo tail

            const bool join = open && pos > 0
                              && (mouseDown || nowMs - steps[(size_t) pos - 1].lastMs < kJoinMs);
            if (! join) { steps.push_back ({}); ++pos; }

            auto& s = steps[(size_t) pos - 1];
            bool found = false;
            for (auto& c : s.changes)
                if (c.k == k) { c.newV = v; found = true; break; }
            if (! found) s.changes.push_back ({ k, old, v });
            s.lastMs = nowMs;
            open = true;
        }

        /** Value received FROM the synth: keeps the state true, never creates a step. */
        void noteRx (Key k, int v) { state[k] = v; open = false; }

        bool canUndo() const { return pos > 0; }
        bool canRedo() const { return pos < (int) steps.size(); }
        int  numSteps() const { return (int) steps.size(); }
        int  position() const { return pos; }

        /** Values to apply to undo the last step; unknownOut = changes that cannot be restored. */
        std::vector<KV> undo (int& unknownOut)
        {
            unknownOut = 0;
            std::vector<KV> out;
            if (! canUndo()) return out;
            open = false;
            const auto& s = steps[(size_t) --pos];
            for (auto c = s.changes.rbegin(); c != s.changes.rend(); ++c)
            {
                if (c->oldV < 0) { ++unknownOut; continue; }
                state[c->k] = c->oldV;
                out.push_back ({ c->k, c->oldV });
            }
            return out;
        }

        std::vector<KV> redo (int& unknownOut)
        {
            unknownOut = 0;
            std::vector<KV> out;
            if (! canRedo()) return out;
            open = false;
            const auto& s = steps[(size_t) pos++];
            for (auto& c : s.changes)
            {
                if (c.newV < 0) { state.erase (c.k); ++unknownOut; continue; }   // restored by a bulk
                state[c.k] = c.newV;
                out.push_back ({ c.k, c.newV });
            }
            return out;
        }

        /** Edited values that differ from the original (what B must re-send after A). */
        std::vector<KV> diffFromOriginal() const
        {
            std::vector<KV> out;
            for (auto& kv : state)
            {
                const auto o = original.find (kv.first);
                if (o == original.end() || o->second != kv.second) out.push_back (kv);
            }
            return out;
        }

        std::vector<KV> originalValues() const { return { original.begin(), original.end() }; }

        /** Revert = an ordinary undoable step back to the original (the caller also sends the
            original bulk, which covers params with no known original value). */
        std::vector<KV> makeRevertStep (double nowMs)
        {
            const auto diff = diffFromOriginal();
            if (diff.empty()) return {};
            if ((int) steps.size() > pos) steps.resize ((size_t) pos);
            Step s;
            s.lastMs = nowMs;
            std::vector<KV> out;
            for (auto& kv : diff)
            {
                const auto o = original.find (kv.first);
                const int nv = o != original.end() ? o->second : -1;
                s.changes.push_back ({ kv.first, kv.second, nv });
                if (nv >= 0) { state[kv.first] = nv; out.push_back ({ kv.first, nv }); }
                else           state.erase (kv.first);
            }
            steps.push_back (std::move (s));
            ++pos;
            open = false;
            return out;
        }

    private:
        std::map<Key, int> state, original;
        std::vector<Step> steps;
        int pos = 0;
        bool open = false, loaded = false;
    };

    //==========================================================================
    /** UI glue: actions installed by MidiDemo, state broadcast to the HistoryBar. */
    struct Hooks : juce::ChangeBroadcaster
    {
        static Hooks& get() { static Hooks h; return h; }
        std::function<void()> undo, redo, toggleAB, revert;
        std::function<void (int)> newVoice;                 // template index (VoiceTemplates)
        juce::StringArray templateNames;
        bool canUndo = false, canRedo = false, loaded = false, showingA = false;
        juce::String status;   // short transient message (e.g. params that could not be restored)
    };

    /** Undo / Redo / A-B / Original buttons, placed in the Voice editor header. */
    class HistoryBar : public juce::Component, private juce::ChangeListener
    {
    public:
        HistoryBar()
        {
            for (auto* b : { &btNew, &btUndo, &btRedo, &btAB, &btOrig })
                addAndMakeVisible (*b);
            addAndMakeVisible (lab);
            lab.setFont (juce::Font (12.0f));
            btUndo.setTooltip (TRANS ("Undo the last edit (Cmd+Z)"));
            btRedo.setTooltip (TRANS ("Redo (Cmd+Shift+Z)"));
            btAB  .setTooltip (TRANS ("Compare: A = voice as loaded, B = your edits"));
            btOrig.setTooltip (TRANS ("Back to the voice as loaded (can be undone)"));
            btNew.setTooltip (TRANS ("New voice from a template (sent to the edit buffer)"));
            btNew.onClick = [this]
            {
                juce::PopupMenu m;
                auto& names = Hooks::get().templateNames;
                for (int i = 0; i < names.size(); ++i) m.addItem (i + 1, TRANS (names[i]));
                m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&btNew),
                                 [] (int r) { if (r > 0) if (auto& f = Hooks::get().newVoice) f (r - 1); });
            };
            btUndo.onClick = [] { if (auto& f = Hooks::get().undo) f(); };
            btRedo.onClick = [] { if (auto& f = Hooks::get().redo) f(); };
            btAB  .onClick = [] { if (auto& f = Hooks::get().toggleAB) f(); };
            btOrig.onClick = [] { if (auto& f = Hooks::get().revert) f(); };
            Hooks::get().addChangeListener (this);
            refresh();
        }
        ~HistoryBar() override { Hooks::get().removeChangeListener (this); }

        bool isLockExempt (juce::Component* c) const { return c == this; }

        void resized() override
        {
            auto r = getLocalBounds();
            btNew.setBounds (r.removeFromLeft (64)); r.removeFromLeft (8);
            btUndo.setBounds (r.removeFromLeft (30)); r.removeFromLeft (2);
            btRedo.setBounds (r.removeFromLeft (30)); r.removeFromLeft (8);
            btAB  .setBounds (r.removeFromLeft (64)); r.removeFromLeft (4);
            btOrig.setBounds (r.removeFromLeft (70)); r.removeFromLeft (6);
            lab.setBounds (r);
        }

    private:
        void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }
        void refresh()
        {
            auto& h = Hooks::get();
            btUndo.setEnabled (h.canUndo && ! h.showingA);
            btRedo.setEnabled (h.canRedo && ! h.showingA);
            btAB.setEnabled (h.loaded);
            btOrig.setEnabled (h.loaded);
            btAB.setToggleState (h.showingA, juce::dontSendNotification);
            btAB.setButtonText (h.showingA ? TRANS ("A: original") : TRANS ("B: edited"));
            lab.setText (h.status, juce::dontSendNotification);
        }

        juce::TextButton btUndo { juce::String::charToString ((juce::juce_wchar) 0x21B6) },   // undo arrow
                         btRedo { juce::String::charToString ((juce::juce_wchar) 0x21B7) },   // redo arrow
                         btAB   { "B" },
                         btOrig { TRANS ("Original") },
                         btNew  { TRANS ("New...") };
        juce::Label lab;
    };
}
