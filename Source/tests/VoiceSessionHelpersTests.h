// Tests unitaires : VoiceSessionHelpers (inclus par Source/Tests.h, ordre d'enregistrement conserve).
#pragma once

//==============================================================================
// Decoupage MidiDemo, etape 0 : helpers purs extraits de MidiDemo/MidiSysex. Les attendus
// reproduisent le code inline AVANT extraction (vecteurs figes).
class VoiceSessionHelpersTests : public juce::UnitTest
{
public:
    VoiceSessionHelpersTests() : juce::UnitTest ("VoiceSessionHelpers", "Sysex77") {}

    // Ancien code inline de MidiDemo::sendParams (copie verbatim, reference).
    static std::array<juce::uint8, 9> legacySendParams (int dev, EditHist::KV kv)
    {
        const juce::uint8 d[9] = { 0x43, SyVoice::deviceByte (dev), 0x34,
                                   (juce::uint8) EditHist::gOf (kv.first), (juce::uint8) EditHist::hiOf (kv.first),
                                   (juce::uint8) EditHist::loOf (kv.first), (juce::uint8) EditHist::pOf (kv.first),
                                   (juce::uint8) EditHist::v1Of (kv.second), (juce::uint8) EditHist::v2Of (kv.second) };
        std::array<juce::uint8, 9> a {};
        std::copy (d, d + 9, a.begin());
        return a;
    }
    // Ancien test inline de MidiDemo::sendToOutputs (isSysEx + 11 + bytes).
    static bool legacyRecordable (const juce::uint8* d, int n)
    {
        return n == 11 && d[0] == 0xF0 && d[1] == 0x43 && (d[2] & 0xF0) == 0x10 && d[3] == 0x34 && d[4] != 0x0F;
    }

    void runTest() override
    {
        using namespace EditHist;
        beginTest ("paramChangeBytes == legacy sendParams inline array (v1 != 0, device clamp)");
        {
            const KV kvs[] = { { keyOf (0x56, 0x00, 0x00, 0x10), valOf (0, 99) },
                               { keyOf (0x03, 0x20, 0x00, 0x2A), valOf (1, 5) },     // v1 != 0
                               { keyOf (0x08, 0x00, 0x00, 0x02), valOf (0x7F, 0x7F) },
                               { keyOf (0x0F, 0x7F, 0x7F, 0x7F), valOf (0, 0) } };
            for (int dev : { 0, 1, 3, 16, 17 })
                for (auto& kv : kvs)
                {
                    const auto got = SyVoice::paramChangeBytes (dev, gOf (kv.first), hiOf (kv.first), loOf (kv.first),
                                                                pOf (kv.first), v1Of (kv.second), v2Of (kv.second));
                    expect (got == legacySendParams (dev, kv));
                }
            // Vecteurs figes.
            auto a = SyVoice::paramChangeBytes (3, 0x03, 0x20, 0x00, 0x2A, 1, 5);
            const std::array<juce::uint8, 9> ea { { 0x43, 0x12, 0x34, 0x03, 0x20, 0x00, 0x2A, 0x01, 0x05 } };
            expect (a == ea);
            expectEquals ((int) SyVoice::paramChangeBytes (0,  0, 0, 0, 0, 0, 0)[1], 0x10);
            expectEquals ((int) SyVoice::paramChangeBytes (16, 0, 0, 0, 0, 0, 0)[1], 0x1F);
            expectEquals ((int) SyVoice::paramChangeBytes (17, 0, 0, 0, 0, 0, 0)[1], 0x1F);
        }

        beginTest ("isRecordableVoiceParamChange == legacy sendToOutputs test");
        {
            juce::uint8 ok[11]   = { 0xF0, 0x43, 0x12, 0x34, 0x56, 0, 0, 0x10, 0, 0x40, 0xF7 };
            juce::uint8 sys[11]  = { 0xF0, 0x43, 0x10, 0x34, 0x0F, 0, 0, 0x01, 0, 0x01, 0xF7 };  // system setup
            juce::uint8 dev2[11] = { 0xF0, 0x43, 0x20, 0x34, 0x56, 0, 0, 0x10, 0, 0x40, 0xF7 };  // 0x2n
            juce::uint8 blk[11]  = { 0xF0, 0x43, 0x00, 0x7A, 0x56, 0, 0, 0x10, 0, 0x40, 0xF7 };  // bulk
            juce::uint8 maker[11]= { 0xF0, 0x41, 0x10, 0x34, 0x56, 0, 0, 0x10, 0, 0x40, 0xF7 };
            juce::uint8 nsx[11]  = { 0x90, 0x43, 0x10, 0x34, 0x56, 0, 0, 0x10, 0, 0x40, 0xF7 };  // pas F0
            expect (SyVoice::isRecordableVoiceParamChange (ok, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (sys, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (dev2, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (blk, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (maker, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (nsx, 11));
            expect (! SyVoice::isRecordableVoiceParamChange (ok, 9));
            expect (! SyVoice::isRecordableVoiceParamChange (nullptr, 11));
            for (auto* v : { ok, sys, dev2, blk, maker, nsx })
                for (int n : { 9, 11 })
                    expect (SyVoice::isRecordableVoiceParamChange (v, n) == legacyRecordable (v, n));
        }

        beginTest ("prepareEditBufferBulk == legacy setBulkDevice + retarget (template + vector)");
        {
            for (int i = 0; i < (int) VoiceTemplates::list().size(); ++i)
            {
                auto a = VoiceTemplates::make (i), b = a;
                expect (SyVoice::prepareEditBufferBulk ((juce::uint8*) a.getData(), (int) a.getSize(), 5));
                SyVoice::setBulkDevice ((juce::uint8*) b.getData(), (int) b.getSize(), 5);
                SyVoice::retargetVoiceToEditBuffer ((juce::uint8*) b.getData(), (int) b.getSize());
                expect (a == b);
                auto* d = (const juce::uint8*) a.getData();
                const int n = (int) a.getSize();
                expectEquals ((int) d[2], 0x04);
                expectEquals ((int) d[30], 0x7F);
                expectEquals ((int) d[31], 0x00);
                expectEquals ((int) d[n - 2], (int) SyVoice::yamahaChecksum (d + 6, n - 8));
                expect (SyVoice::verifyYamahaBulkChecksum (d, n));
            }
            juce::uint8 p[11] = { 0xF0, 0x43, 0x10, 0x34, 1, 2, 3, 4, 5, 6, 0xF7 };   // pas un bulk : intact
            const juce::uint8 p0[11] = { 0xF0, 0x43, 0x10, 0x34, 1, 2, 3, 4, 5, 6, 0xF7 };
            expect (! SyVoice::prepareEditBufferBulk (p, 11, 9));
            expect (std::equal (p, p + 11, p0));
            juce::uint8 tiny[20] = { 0xF0, 0x43, 0x00, 0x7A };            // bulk mais < 34 : refuse
            tiny[19] = 0xF7;
            expect (! SyVoice::prepareEditBufferBulk (tiny, 20, 9));
            juce::uint8 unframed[40] = { 0xF0, 0x43, 0x00, 0x7A };        // pas de F7 final : refuse
            expect (! SyVoice::prepareEditBufferBulk (unframed, 40, 9));
            expectEquals ((int) unframed[30], 0x00);
        }

        beginTest ("constat 5: window title without a voice name has no stale name");
        {
            const juce::String base = juce::String ("Sysex77 v") + Sysex77::kVersion;
            expect (Sysex77::windowTitle ({}) == base);
            expect (Sysex77::windowTitle ("BRASS 1") == base + "  -  BRASS 1");
        }
    }
};

static VoiceSessionHelpersTests voiceSessionHelpersTests;
