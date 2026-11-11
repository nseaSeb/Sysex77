// SysexDrum.h - SyDrum : voix drum (type 10).
// Partie de SysexUtils.h (en-tete parapluie). Logique pure, testee dans tests/.
#pragma once
#include <JuceHeader.h>
#include "SysexVoice.h"

//==============================================================================
/** DRUM SET (voice type 10) : etat des 61 notes (MIDI 36..96 = C1..C6), 8 octets bruts par note
    dans l'ordre du bulk (@98 + 8*k) : [ALTGRP b6 | OUT2 b1 | OUT1 b0, WSRC, WAV MSB, WAV LSB,
    WVL, WTN o/b 64, WNS o/b 64, WPN o/b 31]. Alimente par les param-change group 0x04
    (Table 1-5 : T2 = note, N2 = 0..6 ; WAV seul porte un MSB en V1).
    Offsets d'affichage verifies sur 127 voix drum reelles (docs/) : WTN et WNS centres sur 64,
    WPN centre sur 31 (valeurs 0..62 = -31..+31, PAS 64), octet 0 = 1..3 (+64 si groupe alterne). */
namespace SyDrum
{
    constexpr int firstNote = 36, numNotes = 61;

    struct Kit
    {
        juce::uint8 raw[numNotes][8] {};
        bool loaded = false;

        /** Applique un param-change decode (g hi lo p v1 v2). true si c'etait un param drum. */
        bool apply (int g, int hi, int lo, int p, int v1, int v2)
        {
            if (g != 0x04 || lo != 0 || hi < firstNote || hi >= firstNote + numNotes || p < 0 || p > 6)
                return false;
            auto* r = raw[hi - firstNote];
            if (p <= 1)       r[p] = (juce::uint8) v2;
            else if (p == 2) { r[2] = (juce::uint8) v1; r[3] = (juce::uint8) v2; }
            else              r[p + 1] = (juce::uint8) v2;
            loaded = true;
            return true;
        }
    };

    inline int wave   (const juce::uint8* r) { return (r[2] & 0x7F) * 128 + (r[3] & 0x7F); }
    inline int source (const juce::uint8* r) { return r[1]; }
    inline int volume (const juce::uint8* r) { return r[4]; }
    inline int tune   (const juce::uint8* r) { return (int) r[5] - 64; }   // -64..+63
    inline int shift  (const juce::uint8* r) { return (int) r[6] - 64; }   // -48..+36
    inline int pan    (const juce::uint8* r) { return (int) r[7] - 31; }   // -31..+31
    inline bool alternate (const juce::uint8* r) { return (r[0] & 0x40) != 0; }
    /** N2 = 0 recompose (octet PACKE ALTGRP b6 | OUT2 b1 | OUT1 b0) ; autres bits de cur conserves. */
    inline int packOutAlt (int cur, bool out1, bool out2, bool alt)
    {
        return ((cur & ~0x43) | (alt ? 0x40 : 0) | (out2 ? 0x02 : 0) | (out1 ? 0x01 : 0)) & 0x7F;
    }
    /** Sorties : "1", "2", "1+2" ou "-" (aucune). */
    inline juce::String outputs (const juce::uint8* r)
    {
        const bool o1 = (r[0] & 0x01) != 0, o2 = (r[0] & 0x02) != 0;
        return o1 && o2 ? "1+2" : o1 ? "1" : o2 ? "2" : "-";
    }
}
