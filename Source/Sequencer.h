/*
  ==============================================================================

    Sequencer.h
    Phrase player for the bottom bar ("Seq" mode): loops a built-in phrase on the
    synth while a preset is being edited, so every edit can be heard hands-free.

    Timing: dedicated high-priority thread, events scheduled on ABSOLUTE times
    (anchor + beat * 60000 / bpm) -> no drift accumulates; coarse wait then a short
    spin for the last ~2 ms. A tempo change re-anchors at the current beat (no jump).
    Pauses itself (all its notes off) while a bank is sent/received (Seq::ScopedBulk).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace Seq
{
    struct Ev     { double beat; int note; int vel; double len; };   // note relative to root
    struct Phrase { const char* name; double loopBeats; std::vector<Ev> evs; };

    inline const std::vector<Phrase>& phrases()
    {
        static const std::vector<Phrase> p {
            { "Held note",      4.0, { { 0.0, 0, 100, 3.75 } } },
            { "Repeated notes", 4.0, { { 0.0, 0, 100, 0.5 }, { 1.0, 0, 100, 0.5 },
                                       { 2.0, 0, 100, 0.5 }, { 3.0, 0, 100, 0.5 } } },
            { "Velocity ramp",  4.0, { { 0.0, 0, 32, 0.8 },  { 1.0, 0, 64, 0.8 },
                                       { 2.0, 0, 96, 0.8 },  { 3.0, 0, 127, 0.8 } } },
            { "Chord",          4.0, { { 0.0, 0, 96, 3.5 }, { 0.0, 4, 90, 3.5 },
                                       { 0.0, 7, 90, 3.5 }, { 0.0, 12, 90, 3.5 } } },
            { "Arpeggio",       4.0, {} },   // filled below
            { "Bass line",      4.0, {} },
            { "Melody",         8.0, {} },
            { "Pad",            8.0, { { 0.0, 0, 80, 7.5 }, { 0.0, 7, 75, 7.5 },
                                       { 0.0, 12, 75, 7.5 }, { 0.0, 16, 70, 7.5 } } } };

        static const std::vector<Phrase> built = []
        {
            auto v = p;
            // Arpeggio : 16ths, up-down over the major chord, twice per bar.
            const int arp[8] = { 0, 4, 7, 12, 16, 12, 7, 4 };
            for (int i = 0; i < 16; ++i)
                v[4].evs.push_back ({ i * 0.25, arp[i % 8], i % 4 == 0 ? 110 : 85, 0.2 });
            // Bass line : 8ths, one octave down.
            const int bass[8] = { 0, 0, 12, 0, 7, 0, 10, 12 };
            for (int i = 0; i < 8; ++i)
                v[5].evs.push_back ({ i * 0.5, bass[i] - 12, i % 2 == 0 ? 110 : 80, 0.4 });
            // Melody : two bars (beat, note, len).
            const double mel[][3] = { { 0, 0, 1 },   { 1, 2, 0.5 }, { 1.5, 4, 0.5 }, { 2, 7, 1 },
                                      { 3, 9, 1 },   { 4, 7, 0.5 }, { 4.5, 4, 0.5 }, { 5, 2, 1 },
                                      { 6, 0, 1.75 } };
            for (auto& m : mel)
                v[6].evs.push_back ({ m[0], (int) m[1], 100, m[2] - 0.1 });
            return v;
        }();
        return built;
    }

    /** One MIDI edge of a loop: note-ons and note-offs, sorted by beat (offs first on ties). */
    struct Tick { double beat; bool on; int note; int vel; };

    inline std::vector<Tick> schedule (const Phrase& ph, int root)
    {
        std::vector<Tick> t;
        for (auto& e : ph.evs)
        {
            const int n = juce::jlimit (0, 127, root + e.note);
            t.push_back ({ e.beat, true, n, juce::jlimit (1, 127, e.vel) });
            t.push_back ({ juce::jmin (e.beat + e.len, ph.loopBeats), false, n, 0 });
        }
        std::stable_sort (t.begin(), t.end(), [] (const Tick& a, const Tick& b)
        {
            if (a.beat != b.beat) return a.beat < b.beat;
            return (! a.on) && b.on;
        });
        return t;
    }

    inline double beatsToMs (double beats, double bpm) { return beats * 60000.0 / bpm; }

    /** Tempo change at nowMs: new anchor beat so the playhead does not jump. */
    inline double beatAt (double anchorMs, double anchorBeat, double bpm, double nowMs)
    {
        return anchorBeat + (nowMs - anchorMs) * bpm / 60000.0;
    }

    /** Count of bulk transfers in progress; the player is silent while > 0. */
    inline std::atomic<int>& bulkBusy() { static std::atomic<int> n { 0 }; return n; }
    struct ScopedBulk
    {
        ScopedBulk()  { ++bulkBusy(); }
        ~ScopedBulk() { --bulkBusy(); }
    };

    inline juce::String noteName (int n) { return juce::MidiMessage::getMidiNoteName (n, true, true, 3); }

    //==========================================================================
    class Player : private juce::Thread
    {
    public:
        explicit Player (std::function<void (const juce::MidiMessage&)> sendFn)
            : juce::Thread ("Sysex77 sequencer"), send (std::move (sendFn)) {}

        ~Player() override { shutdown(); }
        void shutdown()    { stop(); stopThread (2000); }

        void play()
        {
            if (! isThreadRunning()) startThread (juce::Thread::Priority::highest);
            restartPending = true;
            playing = true;
            notify();
        }
        void stop()        { playing = false; notify(); }
        bool isPlaying() const { return playing; }

        void setPhrase (int i)  { phraseIndex = juce::jlimit (0, (int) phrases().size() - 1, i); }
        void setRoot (int n)    { root = juce::jlimit (12, 108, n); }
        void setTempo (double b){ bpm = juce::jlimit (30.0, 300.0, b); }
        void setChannel (int c) { channel = juce::jlimit (1, 16, c); }
        int  getRoot() const    { return root; }

        /** Retrig mode: restart the phrase once edits have settled (debounced). */
        void setRetrig (bool r) { retrig = r; }
        void editHappened()
        {
            if (retrig && playing)
                retrigAtMs = juce::Time::getMillisecondCounterHiRes() + 200.0;
        }

    private:
        void run() override
        {
            std::vector<Tick> ticks;
            size_t next = 0;
            double anchorMs = 0, anchorBeat = 0, loopStart = 0, curBpm = 120;

            auto startLoop = [&] (double beat0)
            {
                ticks = schedule (phrases()[(size_t) phraseIndex.load()], root.load());
                next = 0;
                loopStart = beat0;
            };

            while (! threadShouldExit())
            {
                const double now = juce::Time::getMillisecondCounterHiRes();

                if (! playing || bulkBusy() > 0)
                {
                    allOff();
                    restartPending = true;
                    wait (20);
                    continue;
                }

                const double rt = retrigAtMs.load();
                if (restartPending || (rt > 0 && now >= rt))
                {
                    allOff();
                    restartPending = false;
                    retrigAtMs = 0;
                    curBpm = bpm;
                    anchorMs = now + 5.0;   // tiny lead so the first note is not late
                    anchorBeat = 0;
                    startLoop (0);
                }

                if (bpm.load() != curBpm)   // re-anchor: playhead keeps its beat
                {
                    anchorBeat = beatAt (anchorMs, anchorBeat, curBpm, now);
                    anchorMs = now;
                    curBpm = bpm;
                }

                if (next >= ticks.size())
                {
                    startLoop (loopStart + phrases()[(size_t) phraseIndex.load()].loopBeats);
                    if (ticks.empty()) { wait (20); continue; }
                }

                const auto& t = ticks[next];
                const double due = anchorMs + beatsToMs (loopStart + t.beat - anchorBeat, curBpm);
                const double remaining = due - now;

                if (remaining > 2.0)
                {
                    wait ((int) juce::jmin (10.0, remaining - 2.0));   // coarse; notify() wakes it
                    continue;                                            // re-check state & tempo
                }
                while (juce::Time::getMillisecondCounterHiRes() < due && ! threadShouldExit())
                    juce::Thread::yield();                                // fine: last ~2 ms

                emit (t);
                ++next;
            }
            allOff();
        }

        void emit (const Tick& t)
        {
            const int ch = channel;
            if (t.on)
            {
                if (active[(size_t) t.note]) emitOff (ch, t.note);   // overlapping repeat
                active[(size_t) t.note] = true;
                out (juce::MidiMessage::noteOn (ch, t.note, (juce::uint8) t.vel));
            }
            else if (active[(size_t) t.note])
                emitOff (ch, t.note);
        }
        void emitOff (int ch, int n)
        {
            active[(size_t) n] = false;
            out (juce::MidiMessage::noteOff (ch, n));
        }
        void allOff()
        {
            for (int n = 0; n < 128; ++n)
                if (active[(size_t) n]) emitOff (channel, n);
        }
        void out (juce::MidiMessage m)
        {
            m.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
            if (send) send (m);
        }

        std::function<void (const juce::MidiMessage&)> send;
        std::atomic<bool>   playing { false }, restartPending { true }, retrig { false };
        std::atomic<int>    phraseIndex { 0 }, root { 60 }, channel { 1 };
        std::atomic<double> bpm { 120.0 }, retrigAtMs { 0.0 };
        std::array<bool, 128> active {};   // touched by the player thread only
    };

    //==========================================================================
    /** Bottom-bar controls of the Seq mode (left of the keyboard). */
    class Panel : public juce::Component
    {
    public:
        explicit Panel (Player& p) : player (p)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &btPlay, &combo, &tempo, &btRetrig, &rootLabel })
                addAndMakeVisible (c);

            btPlay.setClickingTogglesState (true);
            btPlay.setTooltip (TRANS ("Play / stop the phrase in a loop"));
            btPlay.onClick = [this] { applyPlay(); };

            int id = 1;
            for (auto& ph : phrases()) combo.addItem (TRANS (ph.name), id++);
            combo.setSelectedId (juce::jlimit (1, (int) phrases().size(), getAppSettings()->getIntValue ("SeqPhrase", 1)),
                                 juce::dontSendNotification);
            combo.onChange = [this]
            {
                player.setPhrase (combo.getSelectedId() - 1);
                player.editHappened();
                save ("SeqPhrase", combo.getSelectedId());
            };

            tempo.setSliderStyle (juce::Slider::LinearHorizontal);
            tempo.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
            tempo.setRange (40.0, 240.0, 1.0);
            tempo.setTextValueSuffix (" BPM");
            tempo.setValue (getAppSettings()->getDoubleValue ("SeqTempo", 120.0), juce::dontSendNotification);
            tempo.onValueChange = [this]
            {
                player.setTempo (tempo.getValue());
                save ("SeqTempo", (int) tempo.getValue());
            };

            btRetrig.setClickingTogglesState (true);
            btRetrig.setToggleState (getAppSettings()->getBoolValue ("SeqRetrig", false), juce::dontSendNotification);
            btRetrig.setTooltip (TRANS ("Restart the phrase after each edit (otherwise it loops continuously)"));
            btRetrig.onClick = [this]
            {
                player.setRetrig (btRetrig.getToggleState());
                save ("SeqRetrig", btRetrig.getToggleState());
            };

            rootLabel.setJustificationType (juce::Justification::centredLeft);
            rootLabel.setTooltip (TRANS ("Click a key of the keyboard to transpose the phrase"));

            player.setPhrase (combo.getSelectedId() - 1);
            player.setTempo (tempo.getValue());
            player.setRetrig (btRetrig.getToggleState());
            setRoot (getAppSettings()->getIntValue ("SeqRoot", 60));
        }

        void setRoot (int n)
        {
            player.setRoot (n);
            player.editHappened();
            rootLabel.setText (TRANS ("Root") + " " + noteName (player.getRoot()), juce::dontSendNotification);
            save ("SeqRoot", player.getRoot());
        }

        void togglePlay()            { btPlay.setToggleState (! btPlay.getToggleState(), juce::sendNotification); }
        void stopPlaying()           { btPlay.setToggleState (false, juce::sendNotification); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (0, 2);
            auto row1 = r.removeFromTop (22);
            btPlay.setBounds (row1.removeFromLeft (52));
            row1.removeFromLeft (4);
            combo.setBounds (row1);
            r.removeFromTop (3);
            auto row2 = r.removeFromTop (22);
            btRetrig.setBounds (row2.removeFromRight (56));
            row2.removeFromRight (4);
            tempo.setBounds (row2);
            r.removeFromTop (3);
            rootLabel.setBounds (r.removeFromTop (18));
        }

    private:
        void applyPlay()
        {
            const bool on = btPlay.getToggleState();
            btPlay.setButtonText (on ? TRANS ("Stop") : TRANS ("Play"));
            if (on) player.play(); else player.stop();
        }
        template <typename V> static void save (const char* key, V v)
        {
            getAppSettings()->setValue (key, v);
            getAppSettings()->saveIfNeeded();
        }

        Player& player;
        juce::TextButton btPlay { TRANS ("Play") }, btRetrig { TRANS ("Retrig") };
        juce::ComboBox   combo;
        juce::Slider     tempo;
        juce::Label      rootLabel;
    };
}
