// SysexMix.h - "Quadratic Mixture" (facon SynthWorks) : melange de 4 voix SY77 du meme type.
// Logique pure, testee dans tests/MixtureTests.h.
//
// Principe :
//  - Poids radiaux : coin i a la distance d_i (carre unite) -> w_i = max(0, 1 - d_i), normalises.
//    Les arcs du pad (quart de cercle centre sur un coin, rayon = cote) sont les lignes w_i = 0.
//    Exact a chaque coin, egal (1/4) au centre.
//  - Voix produite = BLOC du coin dominant (structure, enums, switches, octets packes intacts),
//    dans lequel seuls les params d'une LISTE BLANCHE (valeurs plain continues : EG, niveaux,
//    cutoff/resonance, LFO) sont interpoles.
//  - Encodeur exact vis-a-vis du decodeur : la position (offset) de chaque param est trouvee
//    par PERTURBATION d'un bloc du meme type (un octet modifie -> quel param decode a change).
//    Les params a position ambigue sont ecartes.
#pragma once
#include "SysexUtils.h"
#include <map>
#include <set>

namespace SyMix
{
    using VP = SyVoice::VoiceParam;

    inline juce::int64 keyOf (int g, int hi, int lo, int p)
    {
        return ((juce::int64) (g & 0xFF) << 24) | ((hi & 0xFF) << 16) | ((lo & 0xFF) << 8) | (p & 0xFF);
    }
    inline juce::int64 keyOf (const VP& v) { return keyOf (v.group, v.addrHi, v.addrLo, v.param); }

    inline std::map<juce::int64, VP> decode (const juce::MemoryBlock& b)
    {
        std::map<juce::int64, VP> m;
        for (auto& v : SyVoice::voiceBlobToParams ((const juce::uint8*) b.getData(), (int) b.getSize()))
            m[keyOf (v)] = v;
        return m;
    }

    //--------------------------------------------------------------------------
    /** Poids des coins 0 haut-gauche, 1 haut-droit, 2 bas-gauche, 3 bas-droit ; x, y dans [0,1]
        (y vers le bas). */
    inline std::array<double, 4> weights (double x, double y)
    {
        x = juce::jlimit (0.0, 1.0, x); y = juce::jlimit (0.0, 1.0, y);
        const double cx[4] = { 0, 1, 0, 1 }, cy[4] = { 0, 0, 1, 1 };
        std::array<double, 4> w {};
        double sum = 0;
        for (int i = 0; i < 4; ++i)
        {
            const double d = std::hypot (x - cx[i], y - cy[i]);
            w[(size_t) i] = juce::jmax (0.0, 1.0 - d);
            sum += w[(size_t) i];
        }
        for (auto& v : w) v = sum > 0 ? v / sum : 0.25;
        return w;
    }

    /** Coin dominant (poids max ; egalite -> plus petit index). */
    inline int baseCorner (const std::array<double, 4>& w)
    {
        int b = 0;
        for (int i = 1; i < 4; ++i) if (w[(size_t) i] > w[(size_t) b] + 1e-9) b = i;
        return b;
    }

    //--------------------------------------------------------------------------
    /** Liste blanche : params plain continus (un octet, valeur lineaire). Tout le reste vient
        du coin dominant (enums, switches, s/m, o/b, octets packes, params 2 octets). */
    inline bool isMixable (int g, int addrHi, int p)
    {
        juce::ignoreUnused (addrHi);
        if ((g & 0x0F) == 0x06) return p <= 0x0B || p == 0x0E || p == 0x1B;   // EG op R/RR/L/RL, L0, TL
        if (g == 0x03) return p == 0x00;                                       // ELVL
        if (g == 0x02) return p == 0x3F;                                       // VVOL
        if (g == 0x09) return p == 0x01 || p == 0x32;                          // cutoff, resonance
        if (g == 0x05) return p == 0x0D || p == 0x0E || p == 0x0F || p == 0x10 || p == 0x11
                           || p == 0x16 || p == 0x18 || p == 0x19;             // LFO vitesse/delai/prof.
        if (g == 0x07) return p >= 0x50 && p <= 0x56;                          // amp-EG AWM
        return false;
    }

    //--------------------------------------------------------------------------
    /** Position d'un param dans le bloc (offset de V2, et de V1 si 2 octets ; -1 sinon). */
    struct Slot { int off = -1, offV1 = -1; };
    using OffsetMap = std::map<juce::int64, Slot>;

    /** Encodeur par perturbation, a partir d'un bloc d'exemple du type voulu. Octets exclus :
        en-tete, mem 30/31, type 32, nom 33..42, checksum. Un param lu a plusieurs positions,
        ou une position qui change plusieurs params, est ecarte (ambigu). */
    inline OffsetMap buildOffsetMap (const juce::MemoryBlock& sample)
    {
        OffsetMap map;
        const int n = (int) sample.getSize();
        if (n < 50) return map;
        const auto m0 = decode (sample);
        std::map<juce::int64, int> hitsV, hitsV1;
        std::set<juce::int64> bad;
        const auto* s0 = (const juce::uint8*) sample.getData();
        for (int o = 43; o < n - 2; ++o)
        {
            // Deux perturbations (bit 0 et bit 6) : l'octet doit porter UN param, tel quel
            // (valeur decodee == octet brut apres chaque flip : pas de masque, biais ni bits voisins).
            int changed = 0;
            juce::int64 kV = -1, kV1 = -1;
            bool rawMismatch = false;
            for (juce::uint8 bit : { (juce::uint8) 0x01, (juce::uint8) 0x40 })
            {
                auto t = sample;
                auto* d = (juce::uint8*) t.getData();
                d[o] = (juce::uint8) (s0[o] ^ bit);
                const auto m1 = decode (t);
                int c = 0;
                for (auto& [k, v] : m1)
                {
                    auto it = m0.find (k);
                    if (it == m0.end()) continue;
                    if (it->second.value != v.value) { ++c; if (kV >= 0 && kV != k) ++changed; kV = k; if (v.value != d[o]) rawMismatch = true; }
                    if (it->second.v1 != v.v1)       { ++c; if (kV1 >= 0 && kV1 != k) ++changed; kV1 = k; if (v.v1 != d[o]) rawMismatch = true; }
                }
                changed += c;
            }
            const bool one = (kV >= 0) != (kV1 >= 0);
            if (changed != 2 || ! one || rawMismatch)
            { if (kV >= 0) bad.insert (kV); if (kV1 >= 0) bad.insert (kV1); continue; }
            if (kV >= 0)  { if (map[kV].off >= 0) bad.insert (kV);    map[kV].off = o; }
            if (kV1 >= 0) { if (map[kV1].offV1 >= 0) bad.insert (kV1); map[kV1].offV1 = o; }
        }
        for (auto k : bad) map.erase (k);
        for (auto it = map.begin(); it != map.end();)
            it = it->second.off < 0 ? map.erase (it) : std::next (it);
        return map;
    }

    /** Ecrit les params (valeurs filaires) aux positions de la carte, checksum recalcule. */
    inline juce::MemoryBlock encode (juce::MemoryBlock base, const std::map<juce::int64, VP>& params, const OffsetMap& map)
    {
        auto* d = (juce::uint8*) base.getData();
        const int n = (int) base.getSize();
        for (auto& [k, v] : params)
        {
            auto it = map.find (k);
            if (it == map.end()) continue;
            d[it->second.off] = (juce::uint8) (v.value & 0x7F);
            if (it->second.offV1 >= 0) d[it->second.offV1] = (juce::uint8) (v.v1 & 0x7F);
        }
        if (n > 8) d[n - 2] = SyVoice::yamahaChecksum (d + 6, n - 8);
        return base;
    }

    //--------------------------------------------------------------------------
    /** Les 4 coins sont-ils melangeables ? "" = oui, sinon raison (ASCII, a passer par TRANS). */
    inline juce::String incompatibility (const std::array<juce::MemoryBlock, 4>& c)
    {
        for (auto& b : c)
            if (! SyBank::isVoiceBlock (b)
                || SyVoice::detectSynthKind ((const juce::uint8*) b.getData(), b.getSize()) != SyVoice::SynthKind::SY77)
                return "Each corner needs a SY77 voice.";
        const int t = SyBank::typeOf (c[0]);
        if (t == 0x0A) return "Drum voices cannot be mixed.";
        for (auto& b : c)
            if (SyBank::typeOf (b) != t || b.getSize() != c[0].getSize())
                return "All four voices must have the same voice type.";
        return {};
    }

    struct Result { juce::MemoryBlock block; int base = 0, mixed = 0; };

    using Decoded = std::array<std::map<juce::int64, VP>, 4>;
    inline Decoded decodeAll (const std::array<juce::MemoryBlock, 4>& c)
    {
        Decoded d;
        for (int i = 0; i < 4; ++i) d[(size_t) i] = decode (c[(size_t) i]);
        return d;
    }

    /** Melange (coins deja valides par incompatibility()). `map` = buildOffsetMap d'un coin,
        `dec` = decodeAll (c), calcules une fois quand les coins changent (pas a chaque geste). */
    inline Result mix (const std::array<juce::MemoryBlock, 4>& c, const Decoded& dec, double x, double y,
                       const OffsetMap& map, const juce::String& name)
    {
        Result r;
        const auto w = weights (x, y);
        r.base = baseCorner (w);
        std::map<juce::int64, VP> out;
        for (auto& [k, vb] : dec[(size_t) r.base])
        {
            if (! isMixable (vb.group, vb.addrHi, vb.param) || vb.v1 != 0 || ! map.count (k)) continue;
            double acc = 0; bool ok = true;
            for (int i = 0; i < 4 && ok; ++i)
            {
                auto it = dec[(size_t) i].find (k);
                if (it == dec[(size_t) i].end() || it->second.v1 != 0) ok = false;
                else acc += w[(size_t) i] * it->second.value;
            }
            if (! ok) continue;
            VP v = vb; v.value = juce::jlimit (0, 127, juce::roundToInt (acc));
            out[k] = v; ++r.mixed;
        }
        auto blk = encode (c[(size_t) r.base], out, map);
        auto* d = (juce::uint8*) blk.getData();
        const auto nm = name.paddedRight (' ', 10);
        for (int i = 0; i < 10; ++i) d[33 + i] = (juce::uint8) juce::jlimit (32, 126, (int) nm[i]);
        d[blk.getSize() - 2] = SyVoice::yamahaChecksum (d + 6, (int) blk.getSize() - 8);
        r.block = blk;
        return r;
    }
    inline Result mix (const std::array<juce::MemoryBlock, 4>& c, double x, double y, const OffsetMap& map,
                       const juce::String& name)
    { return mix (c, decodeAll (c), x, y, map, name); }
}
