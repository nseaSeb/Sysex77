// SysexMulti.h - SyMulti : bloc Multi 8101MU (parse, encode, dump request).
// Partie de SysexUtils.h (en-tete parapluie). Logique pure, testee dans tests/.
#pragma once
#include <JuceHeader.h>
#include "SysexVoice.h"

//==============================================================================
/** MULTI (bloc bulk « LM  8101MU », 204 octets) — decodage pur. Layout VERIFIE (spec l.1434 +
    346 blocs reels de docs/, checksums valides) : @30 type memoire (00 INT, 02 preset), @31 n°
    0..15, @32..51 nom (20 car.), @52..80 effets (meme ordre que le bulk voix, EFMODE..ST_MIX2),
    @81..89 reserve, partie ch (0..15) @ 90 + 7*ch : [OFV/OUT packe, VMEM, VNUM 0..63, VOL,
    MTUN o/b 64, MNSF o/b 64, STPAN (0 = pan de la voix, 1..63 = -31..+31, centre 32)].
    Param-change : groupe 0x00 = commun (MNAM0-19, N2 0..19), groupe 0x01 / T2 = partie / N2 0..6
    = memes champs que le bulk. Le blocs 8104MU (68 o., non documente) ne sont PAS decodes.
    INCERTAIN : ordre VMEM (OCR int/crd/p1/p2 ; presets usine -> 0 INT, 1 PRE1, 2 PRE2 ?),
    polarite OFVCSW (b6) et bit 2 non documente de l'octet packe. */
namespace SyMulti
{
    constexpr int kSize = 204, kParts = 16, kPartBase = 90, kStride = 7;

    struct Part { juce::uint8 raw[kStride] {}; };
    struct Multi
    {
        juce::String name;
        int memType = 0, memNumber = 0;
        juce::uint8 fx[29] {};
        Part part[kParts];
        bool loaded = false;

        /** Param-change decode (g hi lo p v1 v2) -> etat. true si c'etait un param Multi. */
        bool apply (int g, int hi, int lo, int p, int, int v2)
        {
            if (g == 0x00 && hi == 0 && lo == 0 && p >= 0 && p < 20)
            {
                auto padded = name.paddedRight (' ', 20).substring (0, 20);
                name = (padded.substring (0, p) + juce::String::charToString ((juce::juce_wchar) juce::jlimit (32, 126, v2))
                        + padded.substring (p + 1)).trimEnd();
                return true;
            }
            if (g == 0x01 && lo == 0 && hi >= 0 && hi < kParts && p >= 0 && p < kStride)
            {
                if (! loaded) return false;   // pas de Multi affiche : ne pas inventer 16 parties
                part[hi].raw[p] = (juce::uint8) (v2 & 0x7F);
                return true;
            }
            return false;
        }
    };

    inline bool isMultiBlock (const juce::uint8* d, int size)
    {
        return d != nullptr && size == kSize && d[0] == 0xF0 && d[size - 1] == 0xF7
            && std::memcmp (d + 6, "LM  8101MU", 10) == 0;
    }

    /** Bloc valide (cadre + checksum) -> Multi ; loaded = false sinon. */
    inline Multi parse (const juce::uint8* d, int size)
    {
        Multi m;
        if (! isMultiBlock (d, size) || d[size - 2] != SyVoice::yamahaChecksum (d + 6, size - 8))
            return m;
        m.memType = d[30]; m.memNumber = d[31];
        m.name = juce::String::fromUTF8 ((const char*) d + 32, 20).trimEnd();
        std::memcpy (m.fx, d + 52, 29);
        for (int ch = 0; ch < kParts; ++ch)
            std::memcpy (m.part[ch].raw, d + kPartBase + kStride * ch, kStride);
        m.loaded = true;
        return m;
    }

    /** Champs lisibles d'une partie. */
    inline int voiceMem (const Part& p) { return p.raw[1]; }
    inline int voiceNum (const Part& p) { return p.raw[2]; }
    inline int volume   (const Part& p) { return p.raw[3]; }
    inline int tune     (const Part& p) { return (int) p.raw[4] - 64; }
    inline int shift    (const Part& p) { return (int) p.raw[5] - 64; }
    inline bool panFromVoice (const Part& p) { return p.raw[6] == 0; }
    inline int pan      (const Part& p) { return (int) p.raw[6] - 32; }   // si != 0
    inline bool out1    (const Part& p) { return (p.raw[0] & 0x01) != 0; }
    inline bool out2    (const Part& p) { return (p.raw[0] & 0x02) != 0; }
    inline bool ofvcsw  (const Part& p) { return (p.raw[0] & 0x40) != 0; }   // polarite NON confirmee
    /** N2 = 0 recompose : OUT1 b0, OUT2 b1, switch b6 ; AUTRES bits gardes (bit 2 non documente). */
    inline int packPart (int cur, bool o1, bool o2, bool sw)
    {
        return ((cur & ~0x43) | (sw ? 0x40 : 0) | (o2 ? 0x02 : 0) | (o1 ? 0x01 : 0)) & 0x7F;
    }
    /** Code memoire brut + slot A1..D16 (libelle de banque non confirme -> code affiche). */
    inline juce::String voiceLabel (const Part& p)
    {
        const int n = voiceNum (p);
        return "M" + juce::String (voiceMem (p)) + " "
             + juce::String::charToString ((juce::juce_wchar) ('A' + (n / 16) % 4)) + juce::String (n % 16 + 1);
    }

    /** Bloc 8101MU a envoyer : base (bloc d'origine, effets/reserve conserves) + nom et parties
        courants, memoire = EDIT BUFFER ($7F, n° 0), checksum recalcule. Vide si base invalide.
        NB : reception d'un bulk Multi $7F par le SY77 NON verifiee hardware. */
    inline juce::MemoryBlock encodeForEditBuffer (const Multi& m, const juce::MemoryBlock& base)
    {
        juce::MemoryBlock out (base);
        auto* d = (juce::uint8*) out.getData();
        if (! isMultiBlock (d, (int) out.getSize())) return {};
        const auto nm = m.name.paddedRight (' ', 20);
        for (int k = 0; k < 20; ++k) d[32 + k] = (juce::uint8) juce::jlimit (32, 126, (int) nm[k]);
        for (int ch = 0; ch < kParts; ++ch)
            std::memcpy (d + kPartBase + kStride * ch, m.part[ch].raw, kStride);
        d[30] = 0x7F; d[31] = 0x00;
        d[kSize - 2] = SyVoice::yamahaChecksum (d + 6, kSize - 8);
        return out;
    }

    /** Dump request d'un Multi memorise (memType 00 INT / 02 preset, n° 0..15). La spec interdit
        $7F en requete : le Multi en cours d'edition sur le synthe n'est pas lisible. */
    inline juce::MidiMessage dumpRequest (int deviceNumber, int memType, int memNumber)
    {
        const juce::uint8 body[] = {
            0x43, SyVoice::dumpRequestDeviceByte (deviceNumber), 0x7A,
            'L', 'M', ' ', ' ', '8', '1', '0', '1', 'M', 'U',
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            (juce::uint8) (memType & 0x7F), (juce::uint8) (memNumber & 0x7F)
        };
        return juce::MidiMessage::createSysExMessage (body, (int) sizeof (body));
    }

    /** Blocs Multi (8101MU valides) d'une banque .syx, dans l'ordre du fichier. */
    inline juce::Array<juce::MemoryBlock> multisIn (const juce::uint8* data, size_t size)
    {
        juce::Array<juce::MemoryBlock> out;
        for (auto& b : SyVoice::splitSysexMessages (data, size))
            if (parse ((const juce::uint8*) b.getData(), (int) b.getSize()).loaded) out.add (b);
        return out;
    }
}
