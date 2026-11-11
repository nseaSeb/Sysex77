// Tests unitaires : VoiceTemplates (inclus par Source/Tests.h, ordre d'enregistrement conserve).
#pragma once

//==============================================================================
// Voice templates (Templates.h): framing, checksum, type, decodable by the editor.
class VoiceTemplatesTests : public juce::UnitTest
{
public:
    VoiceTemplatesTests() : juce::UnitTest ("VoiceTemplates", "Sysex77") {}
    void runTest() override
    {
        const int types[4] = { 3, 3, 5, 8 };
        const int sizes[4] = { 466, 466, 221, 587 };
        for (int i = 0; i < (int) VoiceTemplates::list().size(); ++i)
        {
            beginTest (juce::String ("template ") + VoiceTemplates::list()[(size_t) i].name);
            auto m = VoiceTemplates::make (i);
            auto* b = (const juce::uint8*) m.getData();
            expectEquals ((int) m.getSize(), sizes[i]);
            expect (b[0] == 0xF0 && b[1] == 0x43 && b[3] == 0x7A && b[m.getSize() - 1] == 0xF7);
            expectEquals ((int) b[32], types[i]);
            expectEquals (((int) b[4] << 7) | b[5], (int) m.getSize() - 8);
            expect (SyVoice::verifyYamahaBulkChecksum (b, (int) m.getSize()));
            expect (SyVoice::detectSynthKind (b, m.getSize()) == SyVoice::SynthKind::SY77);
            expect (SyVoice::voiceBlobToParams (b, (int) m.getSize()).size() > 0);
        }
        expect (VoiceTemplates::make (99).getSize() == 0);

        beginTest ("setBulkDevice re-stamps only Yamaha bulk blocks, checksum untouched");
        auto m = VoiceTemplates::make (0);
        auto* b = (juce::uint8*) m.getData();
        SyVoice::setBulkDevice (b, (int) m.getSize(), 3);
        expectEquals ((int) b[2], 0x02);
        expect (SyVoice::verifyYamahaBulkChecksum (b, (int) m.getSize()));
        juce::uint8 param[11] = { 0xF0, 0x43, 0x10, 0x34, 0, 0, 0, 0, 0, 0, 0xF7 };
        SyVoice::setBulkDevice (param, 11, 5);
        expectEquals ((int) param[2], 0x10);   // not a bulk: unchanged
    }
};

static VoiceTemplatesTests voiceTemplatesTests;
