/*
  ==============================================================================

    SysexUtils.h
    Fonctions « pures » (sans dépendance GUI) liées au MIDI/Sysex du SY77.
    Extraites des composants pour être réutilisables ET testables (voir Tests.h).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace SyVoice
{
    //==============================================================================
    /** Extrait les noms de voix (10 caractères) d'un dump de banque .syx du SY77.

        Chaque voix commence par 0xF0 ; le nom se trouve 33 octets après le 0xF0.
        Robuste aux fins de buffer tronquées (pas de lecture hors limites, contrairement
        à l'ancien code inline qui pouvait planter sur un fichier incomplet).
    */
    inline juce::StringArray extractVoiceNames (const juce::uint8* data, size_t size)
    {
        juce::StringArray names;
        if (data == nullptr || size == 0)
            return names;

        for (size_t i = 0; i + 1 < size; ++i)
        {
            if (data[i] == 0xF0)
            {
                const size_t nameStart = i + 33;
                if (nameStart + 10 <= size)
                {
                    juce::String str;
                    for (int a = 0; a < 10; ++a)
                        str += juce::String::charToString ((juce::juce_wchar) data[nameStart + a]);
                    names.add (str);
                }
                i = nameStart; // saute l'en-tête (le ++i de la boucle reprend ensuite)
            }
        }

        return names;
    }

    //==============================================================================
    /** Renvoie le bloc Sysex (de 0xF0 à 0xF7 inclus) de la voix d'index `index`
        dans un dump de banque .syx (les voix y sont des messages F0…F7 successifs).
        Renvoie un bloc vide si l'index est hors plage. */
    inline juce::MemoryBlock getVoiceBlock (const juce::uint8* data, size_t size, int index)
    {
        juce::MemoryBlock block;
        if (data == nullptr || index < 0)
            return block;

        int count = 0;
        for (size_t i = 0; i < size; ++i)
        {
            if (data[i] == 0xF0)
            {
                if (count == index)
                {
                    size_t end = i + 1;
                    while (end < size && data[end] != 0xF7 && data[end] != 0xF0)
                        ++end;
                    if (end < size && data[end] == 0xF7)
                        ++end; // inclut le 0xF7 terminal
                    block.append (data + i, end - i);
                    return block;
                }
                ++count;
            }
        }
        return block;
    }

    //==============================================================================
    /** Découpe un buffer .syx contenant PLUSIEURS messages sysex successifs en blocs
        individuels (chacun de 0xF0 à son 0xF7 inclus). Réutilise la même logique de
        bornage que `getVoiceBlock` (s'arrête au 0xF7 OU au 0xF0 suivant, ce qui protège
        des buffers tronqués / non terminés).

        Sert à envoyer une banque message-par-message (avec throttle entre chaque) au lieu
        d'un seul gros MidiMessage qui saturerait le buffer d'entrée du SY77. Pure et
        testable (cf. Tests.h). Les octets hors d'un bloc F0…F7 sont ignorés. */
    inline juce::Array<juce::MemoryBlock> splitSysexMessages (const juce::uint8* data, size_t size)
    {
        juce::Array<juce::MemoryBlock> blocks;
        if (data == nullptr)
            return blocks;

        for (size_t i = 0; i < size; ++i)
        {
            if (data[i] == 0xF0)
            {
                size_t end = i + 1;
                while (end < size && data[end] != 0xF7 && data[end] != 0xF0)
                    ++end;
                const bool terminated = (end < size && data[end] == 0xF7);
                if (terminated)
                    ++end;                       // inclut le 0xF7 terminal

                juce::MemoryBlock b;
                b.append (data + i, end - i);
                blocks.add (b);

                i = end - 1;                     // le ++i de la boucle passe au prochain octet
            }
        }
        return blocks;
    }

    //==============================================================================
    /** Libellé lisible du type de voix SY77, lu à l'octet @32 du bloc voix.
        Mêmes intitulés que le sélecteur comboMode (id = typeByte + 1). */
    inline juce::String voiceTypeLabel (int typeByte)
    {
        static const char* const labels[] = {
            "1 AFM MONO", "2 AFM MONO", "4 AFM MONO", "1 AFM POLY", "2 AFM POLY",
            "1 AWM POLY", "2 AWM POLY", "4 AWM POLY", "1 AFM & 1 AWM POLY", "2 AFM & 2 AWM POLY"
        };
        if (typeByte >= 0 && typeByte < (int) (sizeof (labels) / sizeof (labels[0])))
            return labels[typeByte];
        return "type " + juce::String (typeByte) + " ?";
    }

    //==============================================================================
    /** Une différence d'octet entre deux blocs de voix : position + ancienne/nouvelle valeur. */
    struct VoiceByteDiff { int offset; juce::uint8 before; juce::uint8 after; };

    /** Compare deux blocs de voix (F0…F7) octet à octet et renvoie les positions qui
        diffèrent. Outil de rétro-ingénierie : on dumpe une voix, on modifie UN seul
        paramètre sur le synthé, on re-dumpe, et le diff révèle l'offset exact de ce
        paramètre (plus l'octet de checksum, qui change toujours). Permet de bâtir une
        carte d'offsets VÉRIFIÉE sans deviner (cf. [[project-eg-sysex-bug]]).
        Les octets au-delà de la longueur du plus court bloc sont ignorés. */
    inline juce::Array<VoiceByteDiff> diffVoiceBlocks (const juce::uint8* a, size_t aSize,
                                                       const juce::uint8* b, size_t bSize)
    {
        juce::Array<VoiceByteDiff> diffs;
        if (a == nullptr || b == nullptr)
            return diffs;
        const size_t n = juce::jmin (aSize, bSize);
        for (size_t i = 0; i < n; ++i)
            if (a[i] != b[i])
                diffs.add ({ (int) i, a[i], b[i] });
        return diffs;
    }

    //==============================================================================
    /** Un paramètre décodé d'un bloc voix, exprimé comme un message param-change
        équivalent (mêmes group/addrHi/addrLo/param que les widgets de l'éditeur).
        Injecté dans `valueSysexIn` au chargement -> chaque widget se met à jour via
        son propre chemin de réception, déjà vérifié hardware (cf. Track A). */
    struct VoiceParam { int group, addrHi, addrLo, param, value; };

    /** Décode les paramètres FIABLES (vérifiés contre de vrais dumps SY77) d'un bloc
        voix 1AFM (F0…F7, 466 octets, type@32 == 0x03) et les renvoie sous forme de
        messages param-change équivalents, prêts à rejouer dans l'éditeur.

        Gère 1/2/4 AFM, 1/2/4 AWM ET les voix mixtes AFM+AWM (1AFM_1AWM, 2AFM_2AWM),
        mono ET poly — moteur par élément, blocs cumulatifs (AFM 357 o. / AWM 112 o.).
        AFM, par élément : opérateurs (R1-4, RR1-2, L1-4, RL1-2, L0, TL, Fine) + niveau
        d'élément + algorithme + cutoff filtre 1 & 2 + résonance.
        AWM, par élément : waveform (+ niveau d'élément). Plus le volume de voix (commun).
        Les params à encodage/câblage non vérifié (type filtre, coarse, detune, s/m,
        pitch-EG/LFO, effets, AWM fine/fixed/amp-EG/filtre) et les voix mixtes AFM+AWM /
        drum sont VOLONTAIREMENT omis : l'éditeur conserve sa valeur (« fiabilité d'abord »).

        Provenance des offsets : Table 2 « Voice Bulk Dump » du manuel (lecture claire,
        pattern base=107+9*(N-1) / stride 357 vérifié sur 1/2/4 AFM) + recoupement diff
        single-param (algo @377, cutoff @404) et fingerprint (TL/Fine/R/L/RR/RL/L0). */
    inline juce::Array<VoiceParam> voiceBlobToParams (const juce::uint8* d, int sz)
    {
        juce::Array<VoiceParam> out;
        if (d == nullptr || sz < 43) return out;         // au moins en-tête + type + nom
        // (la vraie borne dépend du type -> contrôle « bloc trop court » plus bas)

        // Type @32 (Table 2) -> nombre d'éléments + MOTEUR PAR ÉLÉMENT (true=AFM, false=AWM).
        // MONO/POLY partagent la structure ; les voix mixtes alternent (ex. 1AFM_1AWM = [AFM,AWM]).
        bool eng[4] = { true, true, true, true };
        int nElem = 0;
        switch (d[32])
        {
            case 0x00: case 0x03: nElem = 1; break;                                      // 1 AFM
            case 0x01: case 0x04: nElem = 2; break;                                      // 2 AFM
            case 0x02:            nElem = 4; break;                                      // 4 AFM
            case 0x05:            nElem = 1; eng[0] = false; break;                      // 1 AWM
            case 0x06:            nElem = 2; eng[0] = eng[1] = false; break;             // 2 AWM
            case 0x07:            nElem = 4; eng[0]=eng[1]=eng[2]=eng[3]=false; break;   // 4 AWM
            case 0x08:            nElem = 2; eng[1] = false; break;                      // 1AFM_1AWM
            case 0x09:            nElem = 4; eng[2] = eng[3] = false; break;             // 2AFM_2AWM
            default: return out;            // drum (10) : autre layout -> intact
        }

        // Table 2 : base de l'élément 1 = 107 + 9*(N-1) (octets common par layer) ; les blocs
        // d'éléments se SUIVENT (cumulatif) — AFM = 357 o., AWM = 112 o. ; ELVL_e @98+9*e ;
        // addrHi = élément<<5.
        const int firstBase = 107 + 9 * (nElem - 1);
        int total = firstBase;
        for (int e = 0; e < nElem; ++e) total += eng[e] ? 357 : 112;
        if (sz < total) return out;   // bloc trop court

        // AFM : param-change (N2) -> offset interne dans le record opérateur de 45 o. (CONFIRMÉS).
        struct Pm { int param, intOff; };
        const Pm pm[] = {
            { 0x00, 0 }, { 0x01, 1 }, { 0x02, 2 }, { 0x03, 3 },   // R1-R4
            { 0x04, 4 }, { 0x05, 5 },                             // RR1, RR2
            { 0x06, 6 }, { 0x07, 7 }, { 0x08, 8 }, { 0x09, 9 },   // L1-L4
            { 0x0A, 10 }, { 0x0B, 11 },                           // RL1, RL2
            { 0x0E, 14 },                                         // L0
            { 0x1B, 29 },                                         // TL (niveau de sortie)
            { 0x25, 43 },                                         // FPC (Coarse) -> colonne COARSE
            { 0x17, 24 },                                         // PWAVE (waveform 0..15) -> colonne WAVE
            { 0x1A, 28 },                                         // FPD (Detune ±15 s/m) -> colonne DET
            { 0x26, 44 },                                         // FPF (Fine 0..99) -> colonne FINE
            // Panel détaillé. SENSIT : offsets bulk ESTIMÉS (à confirmer hardware) ; le reste plain.
            { 0x11, 17 }, { 0x10, 16 },                           // SENSIT VEL / AM (PM=0x18 NON chargé :
            // packé avec le MODE freq (Fixed/Ratio) -> le charger basculait MODE. Bit-split à faire.
            { 0x1C, 30 }, { 0x1D, 31 }, { 0x1E, 32 }, { 0x1F, 33 }, // SCALING break-points BP1-4
            { 0x20, 35 }, { 0x21, 37 }, { 0x22, 39 }, { 0x23, 41 }, // SCALING offset-levels (LSB EGOS)
            { 0x24, 42 }                                          // VEL SW (RVSW, 0/1)
            // PHASE (0x19) NON chargé : octet packé phase+sync, le bouton SYNC lit l'octet brut
            // -> le charger corromprait le SYNC (extraction de bits à faire).
        };
        const int opGroup[6] = { 0x06, 0x16, 0x26, 0x36, 0x46, 0x56 };   // OP6 … OP1

        int base = firstBase;
        for (int e = 0; e < nElem; ++e)
        {
            const int aH = e << 5;                     // addrHi de l'élément (0/32/64/96)

            out.add ({ 0x03, aH, 0, 0x00, (int) d[98 + 9 * e] });    // ELVL (niveau élément, commun)

            if (eng[e])
            {
                for (int op = 0; op < 6; ++op)
                    for (auto& m : pm)
                        out.add ({ opGroup[op], aH, 0, m.param, (int) d[base + op * 45 + m.intOff] });
                out.add ({ 0x05, aH,        0, 0x00, (int) d[base + 270] }); // ALGNUM
                // Pitch EG rates (element commun, group 0x05). Bloc element-commun = base+270+N2
                // (bornes vérifiées : ALGNUM@+270 ... avant le filtre @+296).
                out.add ({ 0x05, aH, 0, 0x01, (int) d[base + 271] }); // FPR1
                out.add ({ 0x05, aH, 0, 0x02, (int) d[base + 272] }); // FPR2
                out.add ({ 0x05, aH, 0, 0x03, (int) d[base + 273] }); // FPR3
                out.add ({ 0x05, aH, 0, 0x04, (int) d[base + 274] }); // FPRR1
                // Pitch-EG LEVELS FPL0-3 (N2 0x05-0x08). Octet filaire o/b 0..127 (centre 64) ;
                // l'éditeur stocke l'octet filaire (display signé = egLevelToDisplay au moment de
                // l'affichage). Provenance offset : main.lua l.22 + TG77_Voice.json pNum 7005-7008.
                out.add ({ 0x05, aH, 0, 0x05, (int) d[base + 275] }); // FPL0
                out.add ({ 0x05, aH, 0, 0x06, (int) d[base + 276] }); // FPL1
                out.add ({ 0x05, aH, 0, 0x07, (int) d[base + 277] }); // FPL2
                out.add ({ 0x05, aH, 0, 0x08, (int) d[base + 278] }); // FPL3
                out.add ({ 0x05, aH, 0, 0x09, (int) d[base + 279] }); // FPRL1 (release level, o/b)
                out.add ({ 0x05, aH, 0, 0x0C, (int) d[base + 282] }); // FYPSW (PEG switch, élément)
                // Filtres (group 0x09). Blocs N2-ordonnés (bornes vérifiées FTYPE@+296,
                // FCTOF@+297, FFTYPE2@+325, BP4@+316) : filtre 1 = base+296+N2, filtre 2 = base+325+N2.
                out.add ({ 0x09, aH,        0, 0x01, (int) d[base + 297] }); // FCTOF1
                out.add ({ 0x09, aH | 0x01, 0, 0x01, (int) d[base + 326] }); // FCTOF2
                out.add ({ 0x09, aH | 0x02, 0, 0x32, (int) d[base + 354] }); // FFRES (common)
                // Mode de contrôle (FMODE@02) + rates EG (FR1-4@03-06, FRR1-2@07-08) des 2 filtres.
                for (int n2 = 0x02; n2 <= 0x08; ++n2)
                {
                    out.add ({ 0x09, aH,        0, n2, (int) d[base + 296 + n2] }); // filtre 1
                    out.add ({ 0x09, aH | 0x01, 0, n2, (int) d[base + 325 + n2] }); // filtre 2
                }
                // Filter-EG LEVELS FL0-4 (N2 0x09-0x0D) + FRL1-2 (0x0E-0x0F) des 2 filtres.
                // Octet filaire o/b 0..127 (centre 64). Bloc N2-ordonné : filtre 1 = base+296+N2,
                // filtre 2 = base+325+N2. Provenance : main.lua l.22 + TG77_Voice.json pNum 7309-7315.
                // Oracle : SteelStrng filtre-1 FL0 = octet 64 (== centre par défaut de l'o/b).
                for (int n2 = 0x09; n2 <= 0x0F; ++n2)
                {
                    out.add ({ 0x09, aH,        0, n2, (int) d[base + 296 + n2] }); // filtre 1
                    out.add ({ 0x09, aH | 0x01, 0, n2, (int) d[base + 325 + n2] }); // filtre 2
                }
                // OMIS : type filtre @296/+325 (encodage bulk ambigu), FRS/FPRS/FYPSW (s/m),
                // FOS (filter break-point offsets, o/b SUR 2 OCTETS, layout bulk non confirmé).
                base += 357;   // bloc AFM
            }
            else
            {
                // AWM (Table 1-8, group 0x07). Offsets confirmés sur TARKUSCYMB.
                out.add ({ 0x07, aH, 0, 0x01, (int) d[base + 2]  });   // AWMWAVE (low 7 bits)
                // amp-EG AWM (params @50-56, désormais câblés dans WaveEg mode AWM) :
                out.add ({ 0x07, aH, 0, 0x50, (int) d[base + 89] });   // PAR1
                out.add ({ 0x07, aH, 0, 0x51, (int) d[base + 90] });   // PAR2
                out.add ({ 0x07, aH, 0, 0x52, (int) d[base + 91] });   // PAR3
                out.add ({ 0x07, aH, 0, 0x53, (int) d[base + 92] });   // PAR4
                out.add ({ 0x07, aH, 0, 0x54, (int) d[base + 93] });   // PARR1
                out.add ({ 0x07, aH, 0, 0x55, (int) d[base + 94] });   // PAL2
                out.add ({ 0x07, aH, 0, 0x56, (int) d[base + 95] });   // PAL3
                // PPF/fine (@+5 o/b) & PPM/fixed (@+3) & PARS (@+96 s/m) : encodage à lever -> omis.
                base += 112;   // bloc AWM
            }
        }

        out.add ({ 0x02, 0, 0, 0x3F, (int) d[95] });   // VVOL — volume de voix (commun)
        // Volontairement OMIS (offset connu, encodage/câblage non vérifié) : type filtre @+296
        // (bulk≠enum), FFVSON/FFCMS (s/m), coarse/detune/RS/SLP opérateur, LFO, effets,
        // AWM fine/fixed/filtre, niveaux EG (o/b) ; voix drum (type 10).

        return out;
    }

    //==============================================================================
    /** Heuristique : ce buffer ressemble-t-il à un dump Sysex Yamaha (commence par
        0xF0 0x43) ? Sert à éviter de charger n'importe quel fichier comme banque. */
    inline bool looksLikeYamahaSysex (const juce::uint8* data, size_t size)
    {
        return data != nullptr && size >= 2 && data[0] == 0xF0 && data[1] == 0x43;
    }

    //==============================================================================
    /** Famille de synthé d'un dump .syx — pour une librairie multi-synthés. */
    enum class SynthKind { Unknown, SY77, OtherYamaha, Roland, Korg, Kawai, Other };

    /** Détecte la famille de synthé à partir des premiers octets d'un dump.

        - octet [1] = fabricant : 0x43 Yamaha, 0x41 Roland, 0x42 Korg, 0x40 Kawai.
        - Yamaha + format bulk 0x7A (octet [3]) + identifiant « LM  8101VC » (offset 6)
          ⇒ SY77 (c'est le classifieur de voix SY77, cf. voiceDumpRequest). Les autres
          dumps Yamaha sont marqués OtherYamaha (TG77/SY99/DX… à affiner au besoin).

        Pure et testable (cf. Tests.h). */
    inline SynthKind detectSynthKind (const juce::uint8* data, size_t size)
    {
        if (data == nullptr || size < 2 || data[0] != 0xF0)
            return SynthKind::Unknown;

        switch (data[1])
        {
            case 0x43: // Yamaha
            {
                static const char* kSY77Id = "LM  8101VC";
                if (size >= 16 && data[3] == 0x7A
                    && std::memcmp (data + 6, kSY77Id, 10) == 0)
                    return SynthKind::SY77;
                return SynthKind::OtherYamaha;
            }
            case 0x41: return SynthKind::Roland;
            case 0x42: return SynthKind::Korg;
            case 0x40: return SynthKind::Kawai;
            default:   return SynthKind::Other;
        }
    }

    //==============================================================================
    /** Compare deux numéros de version « MAJ.MIN.PATCH ». Le préfixe « v » et tout suffixe
        de pré-release (« -dev », « -rc1 »…) sont ignorés. Renvoie true si `remote` est
        STRICTEMENT plus récente que `local`. Pure et testable (cf. Tests.h, mise à jour in-app). */
    inline bool isNewerVersion (const juce::String& remote, const juce::String& local)
    {
        auto parse = [] (juce::String v) -> std::array<int, 3>
        {
            v = v.trim();
            if (v.startsWithIgnoreCase ("v")) v = v.substring (1);
            v = v.upToFirstOccurrenceOf ("-", false, false);          // retire -dev / -rc…
            auto parts = juce::StringArray::fromTokens (v, ".", "");
            std::array<int, 3> out { { 0, 0, 0 } };
            for (int i = 0; i < 3 && i < parts.size(); ++i)
                out[(size_t) i] = parts[i].getIntValue();
            return out;
        };
        const auto r = parse (remote), l = parse (local);
        for (size_t i = 0; i < 3; ++i)
            if (r[i] != l[i]) return r[i] > l[i];
        return false;
    }

    /** Libellé court d'affichage / clé stockée dans library.json. */
    inline juce::String synthKindLabel (SynthKind k)
    {
        switch (k)
        {
            case SynthKind::SY77:        return "SY77";
            case SynthKind::OtherYamaha: return "Yamaha";
            case SynthKind::Roland:      return "Roland";
            case SynthKind::Korg:        return "Korg";
            case SynthKind::Kawai:       return "Kawai";
            case SynthKind::Other:       return "Autre";
            case SynthKind::Unknown:
            default:                     return "?";
        }
    }

    //==============================================================================
    /** Checksum Yamaha standard : (somme des octets + checksum) ≡ 0 (mod 128). */
    inline juce::uint8 yamahaChecksum (const juce::uint8* data, int count)
    {
        int sum = 0;
        for (int i = 0; i < count; ++i)
            sum += data[i];
        return (juce::uint8) ((0x80 - (sum & 0x7F)) & 0x7F);
    }

    //==============================================================================
    /** Vérifie le checksum d'UN bloc bulk Yamaha SY77 complet (de 0xF0 à 0xF7 inclus).

        Format bulk (cf. SY77_PARAMETERS.md §1.2, spec « SY77 MIDI Data Format », Table 2) :
            F0 43 0n 7A <count MSB> <count LSB> <data...> <checksum> F7
        Le checksum couvre les octets de DONNÉES (l'identifiant « LM  8101.. » + le corps),
        c.-à-d. depuis l'octet d'index 6 (après les deux octets de count) jusqu'à l'octet
        qui précède le checksum lui-même. La règle Yamaha : (somme des data + checksum) ≡ 0
        (mod 128). On la vérifie via `yamahaChecksum(data) == checksumStocké`.

        Renvoie false (= « ne pas prétendre que c'est sain ») si :
          - le bloc est trop court / mal encadré (pas F0…F7),
          - ce n'est pas un bulk Yamaha 0x43 (on n'invente pas de verdict),
          - le format n'est pas un bulk dump connu (octet [3] != 0x7A / 0x7E).
        Les messages paramétriques courts (0x34) n'ont PAS de checksum -> renvoie false
        (ils ne sont pas concernés : c'est l'appelant qui ne doit vérifier que les bulks).

        Pure et testable (cf. Tests.h) : aucune dépendance GUI. */
    inline bool verifyYamahaBulkChecksum (const juce::uint8* block, int size)
    {
        // En-tête minimal : F0 43 0n fmt cMSB cLSB <>=1 data <checksum> F7  -> au moins 9 octets.
        if (block == nullptr || size < 9)               return false;
        if (block[0] != 0xF0 || block[size - 1] != 0xF7) return false;
        if (block[1] != 0x43)                            return false;     // pas Yamaha
        // Format bulk dump : 0x7A (voice/multi/pan/microtuning) ou 0x7E (system setup).
        if (block[3] != 0x7A && block[3] != 0x7E)        return false;

        // La zone « data » va de l'index 6 (après count MSB/LSB) jusqu'à l'octet juste
        // avant le checksum. Le checksum est l'octet juste avant le F7 final.
        const int checksumIndex = size - 2;
        const int dataStart      = 6;
        const int dataCount      = checksumIndex - dataStart;
        if (dataCount <= 0)                              return false;

        const juce::uint8 expected = yamahaChecksum (block + dataStart, dataCount);
        return expected == block[checksumIndex];
    }

    //==============================================================================
    /** Réécrit un bloc bulk voix SY77 pour qu'il soit reçu dans l'EDIT BUFFER plutôt
        que dans son slot mémoire interne d'origine.

        Le bulk voix porte (cf. spec « SY77 MIDI Data Format », Table 2 « Voice Bulk Dump ») :
          - offset 30 : Memory type — $00 internal / $02 preset1 / $03 preset2 / $7F Edit Buffer ;
          - offset 31 : Memory#     — slot de destination (A1..D16) quand type ≠ $7F.
        Les banques stockent leurs voix avec Memory type = internal -> envoyées telles quelles,
        elles ÉCRASENT la voix du slot correspondant (et ne sont pas auditionnées si le synthé
        est sur une autre voix). En forçant Memory type = $7F (Memory# ignoré en réception), la
        voix est chargée dans l'edit buffer : le SY77 l'affiche/joue immédiatement SANS toucher
        à la mémoire. Le checksum Yamaha est RECALCULÉ (offsets 30/31 sont dans la zone couverte).

        Pure et testable (cf. Tests.h). No-op si le bloc n'est pas un bulk voix bien cadré. */
    inline void retargetVoiceToEditBuffer (juce::uint8* block, int size)
    {
        if (block == nullptr || size < 34)                 return;  // besoin des offsets 30/31 + checksum
        if (block[0] != 0xF0 || block[size - 1] != 0xF7)   return;
        if (block[1] != 0x43 || block[3] != 0x7A)          return;  // bulk voix Yamaha

        block[30] = 0x7F;   // Memory type = Edit Buffer
        block[31] = 0x00;   // Memory# (ignoré en réception pour l'edit buffer)

        const int checksumIndex = size - 2;
        const int dataStart     = 6;
        const int dataCount     = checksumIndex - dataStart;
        if (dataCount <= 0) return;
        block[checksumIndex] = yamahaChecksum (block + dataStart, dataCount);
    }

    //==============================================================================
    /** Message Sysex paramétrique court (9 octets) du SY77/TG77.
        Correspond au format { 0x43, ch, 0x34, group, addrHi, addrLo, param, dataHi, dataLo }.
    */
    struct Param9
    {
        juce::uint8 manufacturer, channel, model, group, addrHi, addrLo, param, dataHi, dataLo;
    };

    /** Parse un bloc de 9 octets (données Sysex sans F0/F7, comme getSysExData()).
        Renvoie false si la taille n'est pas 9 ou si ce n'est pas un message Yamaha SY77.
    */
    inline bool parseParam9 (const juce::uint8* sysex, int size, Param9& out)
    {
        if (sysex == nullptr || size != 9)        return false;
        if (sysex[0] != 0x43 || sysex[2] != 0x34) return false;

        out = { sysex[0], sysex[1], sysex[2], sysex[3], sysex[4],
                sysex[5], sysex[6], sysex[7], sysex[8] };
        return true;
    }

    //==============================================================================
    // Device number (canal sysex SY77). Les messages paramétriques portent en
    // octet [1] la valeur 0x1n, où n = (deviceNumber - 1), deviceNumber ∈ [1..16].

    /** Octet device à émettre pour un numéro de device 1..16 (borné). */
    inline juce::uint8 deviceByte (int deviceNumber)
    {
        const int n = juce::jlimit (1, 16, deviceNumber) - 1;
        return (juce::uint8) (0x10 | (n & 0x0F));
    }

    /** Numéro de device (1..16) encodé dans un octet 0x1n, ou -1 si l'octet
        n'est pas de la forme 0x1n. */
    inline int deviceNumberFromByte (juce::uint8 b)
    {
        if ((b & 0xF0) != 0x10)
            return -1;
        return (b & 0x0F) + 1;
    }

    /** Faut-il accepter un message reçu (octet device = b) pour le device
        sélectionné ? En mode omni, tout message 0x1n est accepté. */
    inline bool acceptsDevice (juce::uint8 b, int selectedDeviceNumber, bool omni)
    {
        const int n = deviceNumberFromByte (b);
        if (n < 0)
            return false;
        return omni || n == juce::jlimit (1, 16, selectedDeviceNumber);
    }

    //==============================================================================
    // Builder de message paramétrique SY77 — une seule définition du format filaire
    // { 0x43, device, 0x34, group, addrHi, addrLo, param, dataHi, dataLo }.
    // (cf. SY77_PARAMETERS.md et la spec « SY77 MIDI Data Format ».)

    /** Corps 9 octets (sans F0/F7) d'un message paramétrique, device inclus. */
    inline std::array<juce::uint8, 9> paramBytes (int deviceNumber, juce::uint8 group,
                                                  juce::uint8 addrHi, juce::uint8 addrLo,
                                                  juce::uint8 param,  juce::uint8 value)
    {
        return { { 0x43, deviceByte (deviceNumber), 0x34, group,
                   addrHi, addrLo, param, 0x00, value } };
    }

    /** Octet addrHi encodant le numéro d'élément (0..3) : element << 5
        (0x00, 0x20, 0x40, 0x60). Vaut pour les groupes 03/05/07/09. */
    inline juce::uint8 elementAddrHi (int elementIndex)
    {
        return (juce::uint8) ((juce::jlimit (0, 3, elementIndex) & 0x03) << 5);
    }

    /** Groupe d'un opérateur AFM (op 0..5 = OP6..OP1) : (op << 4) | 0x06
        -> 0x06, 0x16, 0x26, 0x36, 0x46, 0x56. */
    inline juce::uint8 afmOperatorGroup (int op)
    {
        return (juce::uint8) (((juce::jlimit (0, 5, op) & 0x0F) << 4) | 0x06);
    }

    //==============================================================================
    // RESOLVER D'ADRESSE — port PUR de translate() du codec lua (main.lua l.74-98).
    // Transforme un `ui` (parameterNumber de SyParamTable) + l'élément courant (0..3) et
    // la cible filtre (0..5) en (group, T2, N2) du message param-change. C'est le 2e pilier
    // du dictionnaire déclaratif (avec SyEnc/SyParamTable) : un widget n'aura plus à recopier
    // {0x43,…,group,addrHi,…,N2,…} — il référencera un `ui` et passera par ici + paramBytes.
    struct SyAddr { juce::uint8 group; juce::uint8 t2; juce::uint8 n2; bool ok; };

    inline SyAddr syTranslate (int ui, int element, int filterTarget)
    {
        const juce::uint8 el = elementAddrHi (element);          // (element & 3) << 5
        if (ui >= 1000 && ui < 7000)                              // AFM opérateur (ui = 1000*op + n2)
        {
            const int op = ui / 1000, n2 = ui % 1000;
            if (op >= 1 && op <= 6)                               // op 1..6 = OP1..OP6
                return { (juce::uint8) (((6 - op) << 4) | 0x06), el, (juce::uint8) n2, true };
        }
        else if (ui >= 7000 && ui < 7100) return { 0x05, el,   (juce::uint8) (ui - 7000), true }; // AFM commun
        else if (ui >= 7100 && ui < 7200) return { 0x02, 0x00, (juce::uint8) (ui - 7100), true }; // Voice commun
        else if (ui >= 7200 && ui < 7300) return { 0x03, el,   (juce::uint8) (ui - 7200), true }; // Voice élément
        else if (ui >= 7300 && ui < 7400)                        // Filtre
        {
            const int n2 = ui - 7300;
            int fn = filterTarget;
            if (n2 >= 0x32) fn = (filterTarget <= 2) ? 2 : 5;    // params filtre-COMMUNS (FRES/FVSON/FCMS)
            return { 0x09, (juce::uint8) (el | (fn & 0x07)), (juce::uint8) n2, true };
        }
        else if (ui >= 7400 && ui < 7500) return { 0x08, 0x00, (juce::uint8) (ui - 7400), true }; // Effets
        return { 0, 0, 0, false };
    }

    //==============================================================================
    // Group byte des ENVELOPPES (EG) — source unique de vérité pour les 4 éditeurs d'EG.
    //
    // Le bug historique « group-byte 0x00 » (cf. [[project-eg-sysex-bug]]) venait de ce que
    // chaque éditeur d'EG laissait l'octet [3] (group) à 0x00 — un group jamais valide sur le
    // SY77, donc le synthé ignorait silencieusement chaque slider d'EG. Le bug est corrigé
    // (Filter=0x09, Pitch=0x05, Volume AFM = per-op (op<<4)|6, Volume AWM=0x07), mais centraliser
    // ce mapping ici donne un POINT DE VÉRITÉ testable : les éditeurs CONSOMMENT cette fonction
    // pour leur octet [3], et un test pur la verrouille (cf. Tests.h) — toute régression du group
    // (retour à 0x00 ou valeur fausse) casse le test, même si les éditeurs GUI ne sont pas testés.

    /** Type d'enveloppe édité (un éditeur GUI par type). */
    enum class EgKind
    {
        filter,         // Filtre 1 & 2 (group 0x09 ; le n° de filtre va dans addrHi, pas le group)
        pitch,          // AFM pitch-EG (élément commun, group 0x05)
        afmAmplitude,   // AFM amp-EG = PAR OPÉRATEUR (group (op<<4)|6) ; op 0..5 = OP6..OP1
        awmAmplitude    // AWM amp-EG (élément, group 0x07)
    };

    /** Group byte (octet [3] du message param-change) attendu pour un éditeur d'EG.
        Spec SY77/TG77 (cf. [[project-sy77-addresses]] / [[project-eg-sysex-bug]] §RESOLUTION).
        Pour `afmAmplitude`, l'EG est par opérateur -> passer op (0..5, OP6..OP1) ; ignoré
        pour les autres types. Ne renvoie JAMAIS 0x00 (le group invalide à l'origine du bug). */
    inline juce::uint8 egGroupFor (EgKind kind, int afmOp = 0)
    {
        switch (kind)
        {
            case EgKind::filter:        return 0x09;
            case EgKind::pitch:         return 0x05;
            case EgKind::afmAmplitude:  return afmOperatorGroup (afmOp);   // (op<<4)|6
            case EgKind::awmAmplitude:  return 0x07;
        }
        return 0x09;   // défaut sûr (un group valide), jamais 0x00
    }

    //==============================================================================
    // EG LEVELS — encodage offset-binary (o/b) pour les NIVEAUX d'EG bipolaires.
    //
    // Concerne : pitch-EG levels (FPL0-3, group 0x05) et filter-EG levels
    // (FL0-4 / FRL1-2, group 0x09). Ce sont des niveaux SIGNÉS (l'EG peut monter
    // OU descendre depuis le centre), encodés en offset-binary sur l'octet filaire.
    //
    // FORMULE (offset 64) :   display = wire - 64       (plage display -64..+63)
    //                         wire    = display + 64     (plage filaire 0..127)
    //   => centre (display 0) = octet filaire 64.
    //
    // PROVENANCE (codec lua INDÉPENDANT, fait autorité) :
    //   docs/TG-77 Voice lua and json/main.lua l.22 :
    //     « (o/b) offset binary -64..+63 -> E1 msg 0..127 passes through. »
    //   docs/TG-77 Voice lua and json/TG77_Voice.json : les contrôles « PEG L0-3 »
    //     (pNum 7005-7008) et « FEG L0-4 / RL1-2 » (pNum 7309-7315) déclarent
    //     display {min:-64, max:63}, message {min:0, max:127}, defaultValue 64.
    //     => offset 64 confirmé mot pour mot ; le centre par défaut EST 64.
    //   Recoupement oracle : SteelStrng filtre-1 FL0 = octet 64 (== centre par défaut).
    //
    // NB encodages VOISINS, hors de ces deux fonctions :
    //   - Les niveaux d'EG d'OPÉRATEUR AFM (L0-L4, pNum 100x) sont PLAIN 0..63
    //     (json display==message 0..63, PAS d'offset) — ne PAS leur appliquer l'o/b.
    //   - Les FOS (filter break-point offsets) sont un o/b SUR 2 OCTETS
    //     (json display -127..+127, message 0..254 ; lua OB2 combined=msg+1,
    //     V1=bit7, V2=low7) — encodage distinct, non couvert ici.

    //==============================================================================
    // CODEC D'ENCODAGE DÉCLARATIF — cœur du futur dictionnaire de paramètres.
    // Chaque paramètre a UN encodage ; ces primitives PURES centralisent la logique
    // jusqu'ici éparpillée (fonctions par cas + chemin boolNegative des widgets).
    // Oracle : docs/.../main.lua (encodeValue + tables SM/OB2), bench-verified.
    enum class SyEnc { plain, signMag, offBin1, offBin2 };

    /** Signe-magnitude : affichage signé (-half..+half) -> octet filaire.
        signbit = bit posé pour le négatif (detune {15,16} ; ±7 {7,8} ; ±12 {12,16}).
        Ex. lua encodeValue : signed=msg-half ; <0 -> signbit+(-signed). */
    inline int smToWire (int display, int half, int signbit) noexcept
    {
        display = juce::jlimit (-half, half, display);
        return display >= 0 ? display : (signbit | (-display));
    }
    inline int smToDisplay (int wire, int /*half*/, int signbit) noexcept
    {
        const int w = wire & 0x7F;
        return (w & signbit) ? -(w & (signbit - 1)) : (w & (signbit - 1));
    }

    /** Offset-binary 1 octet : display signé centré sur `offset` (ex. EG filtre offset 64
        octet 0..127 ; pan offset 32 octet 0..63). maxWire borne l'octet. */
    inline int obToWire (int display, int offset, int maxWire) noexcept
    {
        return juce::jlimit (0, maxWire, display + offset);
    }
    inline int obToDisplay (int wire, int offset) noexcept
    {
        return (int) (wire & 0x7F) - offset;
    }

    /** Offset-binary 2 octets (FOS / break-point offsets) : display -127..+127.
        combined = display + 128 (1..255) ; V1 = bit7, V2 = low7 (cf. lua OB2). */
    inline int   ob2Combined  (int display) noexcept { return juce::jlimit (-127, 127, display) + 128; }
    inline juce::uint8 ob2V1   (int display) noexcept { return (juce::uint8) ((ob2Combined (display) >> 7) & 0x01); }
    inline juce::uint8 ob2V2   (int display) noexcept { return (juce::uint8) (ob2Combined (display) & 0x7F); }
    inline int   ob2ToDisplay (int v1, int v2) noexcept { return (((v1 & 0x01) << 7) | (v2 & 0x7F)) - 128; }

    //==============================================================================
    /** Octet filaire o/b (0..127) -> valeur d'affichage signée (-64..+63).
        Inverse exact de egLevelToWire. Source : main.lua l.22 + TG77_Voice.json. */
    inline int egLevelToDisplay (juce::uint8 wire) noexcept
    {
        return obToDisplay (wire, 64);
    }

    /** Valeur d'affichage signée (-64..+63) -> octet filaire o/b (0..127).
        Inverse exact de egLevelToDisplay ; borne le résultat dans 0..127. */
    inline juce::uint8 egLevelToWire (int display) noexcept
    {
        return (juce::uint8) obToWire (display, 64, 127);
    }

    //==============================================================================
    // FPD — Pitch Detune opérateur (param 0x1A), encodage SIGNE-MAGNITUDE.
    // PROVENANCE (hardware-verified, codec lua INDÉPENDANT) :
    //   docs/TG-77 Voice lua and json/main.lua l.45 : SM[1000*op+0x1A] = {15, 16}
    //   -> half=15, signbit=16 ; encodeValue : signed = msg-15 ; si signed>=0 -> signed,
    //      sinon -> 16 + (-signed). Donc pour un affichage signé d (-15..+15) :
    //        d >= 0  -> wire = d            (0..15)
    //        d <  0  -> wire = 16 | (-d)    (17..31)
    //   docs/sy77midi_ocr.txt l.438 : « FPD -15~+15 (s/m) Pitch Detune ».
    // NB : c'est EXACTEMENT le chemin boolNegative de MidiSlider (intNegativeDelta=max+2=17,
    //   ~v) : -d -> (~(-d)) + 17 = 16 + d. fpdDetuneToWire/FromWire sont la version PURE
    //   testable, et permettent de re-vérifier ce chemin (cf. Tests.h).

    /** Affichage détune signé (-15..+15) -> octet filaire s/m (0..15 / 17..31). */
    inline juce::uint8 fpdDetuneToWire (int display) noexcept
    {
        return (juce::uint8) smToWire (display, 15, 16);
    }

    /** Octet filaire s/m FPD -> affichage signé (-15..+15). Inverse de fpdDetuneToWire. */
    inline int fpdDetuneToDisplay (juce::uint8 wire) noexcept
    {
        return smToDisplay (wire, 15, 16);
    }

    /** Configure un slider de NIVEAU d'EG pour afficher le signé o/b (-64..+63) tout en
        stockant l'octet filaire 0..127 (centre 64). À appeler une fois sur chaque slider de
        niveau pitch-EG / filter-EG. La VALEUR du slider reste l'octet filaire (passthrough
        TX/RX, midiTxOffset 0) ; seul le TEXTE affiché est converti via egLevelToDisplay. */
    inline void applyEgLevelDisplay (juce::Slider& s)
    {
        s.textFromValueFunction = [] (double v) { return juce::String (egLevelToDisplay ((juce::uint8) juce::roundToInt (v))); };
        s.valueFromTextFunction = [] (const juce::String& t) { return (double) egLevelToWire (t.getIntValue()); };
    }

    //==============================================================================
    // PAN EG LEVELS — offset-binary OFFSET 32 (distinct du 64 des EG pitch/filtre).
    //
    // Les niveaux de l'EG de pan (PNL0-4, PNRL1-2 ; group 0x0A) sont SIGNÉS −32~+31
    // « (o/b) » d'après docs/sy77midi_ocr.txt l.563-584. Même principe que egLevelTo*,
    // mais l'octet filaire fait 0..63 (centre 32) au lieu de 0..127 (centre 64) :
    //   display = (wire & 0x7F) − 32     (plage −32..+31)
    //   wire    = jlimit(0,63, d + 32)   (centre = octet 32)
    // 🟡 NON vérifié hardware (centre = octet 32 à confirmer par dump pan réel) ; on N'utilise
    // PAS le mode négatif de MidiSlider (= signe-magnitude, ≠ offset-binary) pour ces niveaux.

    /** Octet filaire o/b pan (0..63) -> valeur d'affichage signée (-32..+31). */
    inline int panLevelToDisplay (juce::uint8 wire) noexcept
    {
        return obToDisplay (wire, 32);
    }

    /** Valeur d'affichage signée (-32..+31) -> octet filaire o/b pan (0..63). */
    inline juce::uint8 panLevelToWire (int display) noexcept
    {
        return (juce::uint8) obToWire (display, 32, 63);
    }

    /** Configure un slider de NIVEAU d'EG de pan : valeur = octet filaire 0..63 (centre 32),
        texte affiché = signé -32..+31 (via panLevelToDisplay). Cf. applyEgLevelDisplay. */
    inline void applyPanLevelDisplay (juce::Slider& s)
    {
        s.textFromValueFunction = [] (double v) { return juce::String (panLevelToDisplay ((juce::uint8) juce::roundToInt (v))); };
        s.valueFromTextFunction = [] (const juce::String& t) { return (double) panLevelToWire (t.getIntValue()); };
    }

    /** Message Sysex paramétrique SY77 prêt à émettre (ajoute F0/F7). */
    inline juce::MidiMessage paramMessage (int deviceNumber, juce::uint8 group,
                                           juce::uint8 addrHi, juce::uint8 addrLo,
                                           juce::uint8 param,  juce::uint8 value)
    {
        auto b = paramBytes (deviceNumber, group, addrHi, addrLo, param, value);
        return juce::MidiMessage::createSysExMessage (b.data(), (int) b.size());
    }

    //==============================================================================
    // DUMP REQUEST (USER-TRIGGERED) — demande au SY77 d'émettre un bulk dump de voix.
    //
    // Format (spec « SY77 MIDI Data Format », docs/sy77midi_ocr.txt « (10) dump request »
    // p.17 ; recoupé sur les tables de dump request Multi/Pan, plus lisibles, mêmes
    // octets de cadrage) :
    //     F0 43 2n 7A  'L' 'M' ' ' ' ' '8' '1' '0' '1' 'V' 'C'  <14 octets 0x00>
    //                  Memory_type  Memory#  F7
    //   - 2n   : sous-statut 2 = « dump request » (vs 0n = bulk dump émis par le synthé).
    //            n = deviceNumber-1 (même encodage que deviceByte, mais nibble haut = 0x2).
    //   - 7A   : format de l'objet demandé = Voice Bulk Dump (idem en-tête du dump reçu).
    //   - "LM  8101VC" : identifiant du format VCED (voice) — octets 14/15 de l'en-tête
    //            du bulk voix lus dans l'OCR (= 'V','C') et confirmé hors-repo (format VCED
    //            « LM  8101VC », rétro-compatible SY99 hors effets).
    //   - Memory_type : 0x00 = Internal, 0x02/0x03 = Preset 1/2, 0x7F = Edit Buffer.
    //   - Memory#     : 0x00..0x0F=A1..16, 0x10..0x1F=B, 0x20..0x2F=C, 0x30..0x3F=D.
    //
    // FIABILITÉ : on N'ÉMET QUE des octets dont le cadrage est documenté (≤32 octets,
    // cf. « $F0 system exclusive 32 bytes or less »). La réponse du synthé est captée par
    // le chemin de réception bulk existant. Reste USER-TRIGGERED (jamais au démarrage).

    /** Octet « device » d'un dump request : nibble haut 0x2 (vs 0x1 d'un param-change). */
    inline juce::uint8 dumpRequestDeviceByte (int deviceNumber)
    {
        const int n = juce::jlimit (1, 16, deviceNumber) - 1;
        return (juce::uint8) (0x20 | (n & 0x0F));
    }

    /** Dump request d'UNE voix interne (Memory_type/Memory# par défaut = Internal A1).
        `memoryType` : 0x00 Internal, 0x02/0x03 Preset, 0x7F Edit Buffer.
        `memoryNumber` : 0x00..0x3F (banque A1..D16) ; ignoré pour l'Edit Buffer côté synthé. */
    inline juce::MidiMessage voiceDumpRequest (int deviceNumber,
                                               juce::uint8 memoryType   = 0x00,
                                               juce::uint8 memoryNumber = 0x00)
    {
        const juce::uint8 body[] = {
            0x43, dumpRequestDeviceByte (deviceNumber), 0x7A,
            'L', 'M', ' ', ' ', '8', '1', '0', '1', 'V', 'C',   // identifiant VCED (octets 4..13)
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,            // padding (octets 14..27)
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            (juce::uint8) (memoryType   & 0x7F),                 // octet 28
            (juce::uint8) (memoryNumber & 0x7F)                  // octet 29
        };
        // createSysExMessage encadre par F0…F7 (octet 30 = F7) -> 31 octets, « 32 ou moins ».
        return juce::MidiMessage::createSysExMessage (body, (int) sizeof (body));
    }
}
