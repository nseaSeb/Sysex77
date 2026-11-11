// Tests unitaires : MidiMonitor (rendu texte du moniteur MIDI).
#pragma once
#include "../MidiMonitor.h"

class MidiMonitorTests : public juce::UnitTest
{
public:
    MidiMonitorTests() : juce::UnitTest ("MidiMonitor", "Sysex77") {}

    static juce::MidiMessage param (int devByte, int g, int hi, int lo, int p, int v)
    {
        const juce::uint8 b[] = { 0x43, (juce::uint8) devByte, 0x34, (juce::uint8) g, (juce::uint8) hi,
                                  (juce::uint8) lo, (juce::uint8) p, 0x00, (juce::uint8) v };
        return juce::MidiMessage::createSysExMessage (b, 9);
    }

    void runTest() override
    {
        using namespace SyMonitor;
        juce::String out, dedup;

        beginTest ("raw mode: hex of every byte, no dedup");
        const auto m = param (0x10, 0x03, 0x00, 0x00, 0x00, 0x7F);
        expect (formatLine (m, true, 1, false, out, dedup));
        expectEquals (out, juce::String ("F0 43 10 34 03 00 00 00 00 7F F7\n"));
        expect (formatLine (m, true, 1, false, out, dedup));   // repete : toujours affiche

        beginTest ("decoded mode: SY77 param-change line, dedup by address");
        out = {}; dedup = {};
        expect (formatLine (m, false, 1, false, out, dedup));
        expectEquals (out, juce::String (">> SY77  G=03  AH=00  AL=00  P=00   val=127 (0x7f)\n"));
        expect (! formatLine (param (0x10, 0x03, 0x00, 0x00, 0x00, 5), false, 1, false, out, dedup));  // meme adresse
        expect (formatLine (param (0x10, 0x03, 0x00, 0x00, 0x01, 5), false, 1, false, out, dedup));

        beginTest ("decoded mode: other device omitted unless omni");
        out = {}; dedup = {};
        expect (! formatLine (param (0x13, 0x03, 0, 0, 0, 1), false, 1, false, out, dedup));
        expect (formatLine (param (0x13, 0x03, 0, 0, 0, 1), false, 1, true, out, dedup));

        beginTest ("decoded mode: noise filtered, other messages described");
        out = {};
        expect (! formatLine (juce::MidiMessage::controllerEvent (1, 6, 10), false, 1, false, out, dedup));
        expect (! formatLine (juce::MidiMessage::midiClock(), false, 1, false, out, dedup));
        expect (formatLine (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), false, 1, false, out, dedup));
        expect (out.isNotEmpty());
    }
};

static MidiMonitorTests midiMonitorTests;
