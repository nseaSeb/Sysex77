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
            { 0x1A, 28 }                                          // FPD (Detune ±15 s/m) -> colonne DET
            // PHASE (0x19) NON chargé ici : l'octet 0x19 packe phase + enable-sync, et le bouton
            // SYNC lit l'octet brut -> le charger corrompt le SYNC. À traiter par extraction de
            // bits dans le panel opérateur (phase value vs sync enable séparés).
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
                // OMIS : type filtre @296/+325 (encodage bulk ambigu), niveaux EG (o/b — repr.
                // éditeur 0..64 à aligner), FRS/FPRS/FYPSW (s/m), pitch-EG levels (o/b).
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
    /** Checksum Yamaha standard : (somme des octets + checksum) ≡ 0 (mod 128). */
    inline juce::uint8 yamahaChecksum (const juce::uint8* data, int count)
    {
        int sum = 0;
        for (int i = 0; i < count; ++i)
            sum += data[i];
        return (juce::uint8) ((0x80 - (sum & 0x7F)) & 0x7F);
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

    /** Message Sysex paramétrique SY77 prêt à émettre (ajoute F0/F7). */
    inline juce::MidiMessage paramMessage (int deviceNumber, juce::uint8 group,
                                           juce::uint8 addrHi, juce::uint8 addrLo,
                                           juce::uint8 param,  juce::uint8 value)
    {
        auto b = paramBytes (deviceNumber, group, addrHi, addrLo, param, value);
        return juce::MidiMessage::createSysExMessage (b.data(), (int) b.size());
    }
}
