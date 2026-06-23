/*
  ==============================================================================

    DumpDiffView.h
    Outil de rétro-ingénierie : compare deux dumps .syx (avant / après un
    changement d'UN paramètre sur le synthé) et liste, par voix, les offsets
    d'octets qui diffèrent → permet de bâtir la carte d'offsets bulk.

    (L'octet de checksum, juste avant 0xF7, change toujours : à ignorer.)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SysexUtils.h"   // SyVoice::getVoiceBlock / diffVoiceBlocks
#include "LookAndFeel.h"

class DumpDiffView : public juce::Component
{
public:
    DumpDiffView()
    {
        addAndMakeVisible (btA);    btA.onClick    = [this] { pick (true); };
        addAndMakeVisible (btB);    btB.onClick    = [this] { pick (false); };
        addAndMakeVisible (btDiff); btDiff.setButtonText ("Comparer A / B");
        btDiff.onClick = [this] { runDiff(); };

        addAndMakeVisible (results);
        results.setMultiLine (true);
        results.setReadOnly (true);
        results.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain)));
        results.setText ("1) Charge le dump A (avant)\n"
                         "2) Change UN seul parametre sur le synthe, re-dumpe\n"
                         "3) Charge le dump B (apres) puis Comparer\n\n"
                         "-> chaque offset listé = l'octet de ce parametre dans le bloc de voix.\n"
                         "   (L'octet de checksum, avant F7, change toujours : ignore-le.)");
    }

    void paint (juce::Graphics& g) override { g.fillAll (SYColBackground); }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8);
        auto top = r.removeFromTop (28);
        const int w = (top.getWidth() - 12) / 3;
        btA.setBounds (top.removeFromLeft (w));   top.removeFromLeft (6);
        btB.setBounds (top.removeFromLeft (w));   top.removeFromLeft (6);
        btDiff.setBounds (top);
        r.removeFromTop (6);
        results.setBounds (r);
    }

private:
    void pick (bool a)
    {
        chooser = std::make_unique<juce::FileChooser> ("Choisir un dump .syx", juce::File(), "*.syx;*.SYX");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, a] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (! f.existsAsFile()) return;
                juce::MemoryBlock mb;
                if (f.loadFileAsData (mb))
                {
                    (a ? dataA : dataB) = mb;
                    (a ? btA : btB).setButtonText ((a ? "A: " : "B: ") + f.getFileName());
                }
            });
    }

    static juce::Array<juce::MemoryBlock> splitVoices (const juce::MemoryBlock& mb)
    {
        juce::Array<juce::MemoryBlock> out;
        const auto* d = (const juce::uint8*) mb.getData();
        const auto  n = mb.getSize();
        for (int i = 0; i < 1024; ++i)
        {
            auto b = SyVoice::getVoiceBlock (d, n, i);
            if (b.getSize() == 0) break;
            out.add (b);
        }
        return out;
    }

    void runDiff()
    {
        auto A = splitVoices (dataA), B = splitVoices (dataB);
        juce::String s;
        s << "Voix A: " << A.size() << "   Voix B: " << B.size() << "\n\n";
        const int n = juce::jmin (A.size(), B.size());
        int changed = 0;
        for (int i = 0; i < n; ++i)
        {
            auto diffs = SyVoice::diffVoiceBlocks ((const juce::uint8*) A[i].getData(), A[i].getSize(),
                                                   (const juce::uint8*) B[i].getData(), B[i].getSize());
            if (diffs.isEmpty()) continue;
            ++changed;

            juce::String name;
            const auto* db = (const juce::uint8*) A[i].getData();
            if (A[i].getSize() > 42)
                for (int k = 33; k < 43; ++k) name += juce::String::charToString ((juce::juce_wchar) db[k]);

            s << "Voix " << i << "  \"" << name.trim() << "\"  (" << diffs.size() << " octet(s)) :\n";
            for (auto& d : diffs)
                s << "    offset " << d.offset
                  << " : " << (int) d.before << " -> " << (int) d.after
                  << "   (0x" << juce::String::toHexString ((int) d.before).paddedLeft ('0', 2)
                  << " -> 0x" << juce::String::toHexString ((int) d.after ).paddedLeft ('0', 2) << ")\n";
            s << "\n";
        }
        if (changed == 0)
            s << "Aucune difference (voix identiques, ou nombre/tailles de voix differents).";
        results.setText (s);
    }

    juce::TextButton btA { "Charger A (.syx)" }, btB { "Charger B (.syx)" }, btDiff;
    juce::TextEditor results;
    juce::MemoryBlock dataA, dataB;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DumpDiffView)
};
