// Tests unitaires : preparation du test de connexion et de l'envoi de banque, comparees
// octet pour octet a une copie de l'ancien code inline (MidiDemo / MidiSysex).
#pragma once

class SendPathTests : public juce::UnitTest
{
public:
    SendPathTests() : juce::UnitTest ("SendPath", "Sysex77") {}

    static juce::MemoryBlock bytesOf (const juce::MidiMessage& m) { return { m.getRawData(), (size_t) m.getRawDataSize() }; }

    // Ancien code de l'envoi de banque : -1 vide, -2 non SY77, -3 brut, sinon blocs restampes.
    static int legacyBank (const juce::MemoryBlock& data, int dev, juce::Array<juce::MemoryBlock>& out)
    {
        if (data.getSize() > 1
            && SyVoice::detectSynthKind ((const juce::uint8*) data.getData(), data.getSize()) != SyVoice::SynthKind::SY77)
            return -2;
        if (data.getSize() > 1)
        {
            out = SyVoice::splitSysexMessages ((const juce::uint8*) data.getData(), data.getSize());
            if (out.isEmpty()) return -3;
            for (auto& b : out) SyVoice::setBulkDevice ((juce::uint8*) b.getData(), (int) b.getSize(), dev);
            return out.size();
        }
        return -1;
    }

    static int kindCode (const SyBank::SendPlan& p)
    {
        using K = SyBank::SendPlan::Kind;
        return p.kind == K::empty ? -1 : p.kind == K::notSY77 ? -2 : p.kind == K::raw ? -3 : p.blocks.size();
    }

    static juce::MemoryBlock fakeVoice (int name)
    {
        juce::MemoryBlock b (469, true);
        auto* d = (juce::uint8*) b.getData();
        d[0] = 0xF0; d[1] = 0x43; d[2] = 0x00; d[3] = 0x7A; d[4] = 0x03; d[5] = 0x4B;
        std::memcpy (d + 6, "LM  8101VC", 10);
        d[32] = 0x03; for (int i = 33; i < 43; ++i) d[i] = (juce::uint8) ('A' + name);
        d[467] = SyVoice::yamahaChecksum (d + 6, 461); d[468] = 0xF7;
        return b;
    }

    void runTest() override
    {
        beginTest ("connectionTestMessages == legacy inline messages, devices 1..16");
        for (int dev = 1; dev <= 16; ++dev)
        {
            const juce::uint8 inquiry[] = { 0x7E, 0x7F, 0x06, 0x01 };
            const auto legacyId   = juce::MidiMessage::createSysExMessage (inquiry, (int) sizeof (inquiry));
            const auto legacyDump = SyVoice::voiceDumpRequest (dev, 0x00, 0x00);
            const auto now = SyVoice::connectionTestMessages (dev);
            expectEquals (now.size(), 2);
            expect (bytesOf (now[0]) == bytesOf (legacyId));
            expect (bytesOf (now[1]) == bytesOf (legacyDump));
        }

        beginTest ("prepareForSend == legacy bank send preparation");
        juce::Array<juce::MemoryBlock> inputs;
        inputs.add ({});                                                   // vide
        { juce::MemoryBlock one (1, true); inputs.add (one); }             // 1 octet
        { juce::MemoryBlock junk; for (int i = 0; i < 300; ++i) { juce::uint8 v = (juce::uint8) (i * 7); junk.append (&v, 1); } inputs.add (junk); }
        { juce::MemoryBlock bank; for (int v = 0; v < 3; ++v) bank.append (fakeVoice (v).getData(), 469); inputs.add (bank); }
        expectEquals (kindCode (SyBank::prepareForSend (inputs[3], 1)), 3);   // le chemin 'blocs' est bien exerce
        expectEquals (kindCode (SyBank::prepareForSend (inputs[2], 1)), -2);  // octets etrangers : jamais envoyes
        for (auto& in : inputs)
            for (int dev : { 1, 5, 16 })
            {
                juce::Array<juce::MemoryBlock> legacy;
                const int lk = legacyBank (in, dev, legacy);
                const auto p = SyBank::prepareForSend (in, dev);
                expectEquals (kindCode (p), lk);
                if (lk > 0)
                    for (int i = 0; i < lk; ++i) expect (p.blocks[i] == legacy[i]);
            }
    }
};

static SendPathTests sendPathTests;
