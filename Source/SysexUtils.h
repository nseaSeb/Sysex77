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

        Couverture v1 — UNIQUEMENT les params confirmés hardware : pour chaque
        opérateur AFM (élément 1, OP1..OP6) les rates/levels d'EG (R1-4, RR1-2,
        L1-4, RL1-2, L0), le niveau de sortie (TL) et le fine ; plus l'algorithme
        (ALGNUM, element common). Les params dont l'offset/encodage n'est pas encore
        vérifié (coarse, detune, RS, SLP, HT, filtres, effets, voice-common,
        éléments 2-4) sont VOLONTAIREMENT omis : l'éditeur conserve sa valeur, on
        n'affiche jamais de donnée erronée (« fiabilité d'abord »).

        Provenance des offsets : carte calée sur les dumps SteelStrng + recoupement
        du dump fingerprinté avec re_fingerprint.csv (match exact 6/6 sur TL & Fine ;
        match exact sur R/L/RR/RL/L0). Voir docs/bulk_offset_map_WIP.md. */
    inline juce::Array<VoiceParam> voiceBlobToParams (const juce::uint8* d, int sz)
    {
        juce::Array<VoiceParam> out;
        if (d == nullptr || sz < 466) return out;   // bloc 1AFM complet F0..F7
        if (d[32] != 0x03)            return out;    // type 1AFM uniquement (layout vérifié)

        // 6 opérateurs : ordre de stockage OP6->OP1, 45 octets/record, OP6 @107.
        // group param-change : OP6=0x06 … OP1=0x56 (cf. afmOperatorGroup), addrHi=0.
        struct Op { int group, base; };
        const Op ops[6] = { { 0x06, 107 }, { 0x16, 152 }, { 0x26, 197 },
                            { 0x36, 242 }, { 0x46, 287 }, { 0x56, 332 } };

        // param-change (N2) -> offset interne dans le record de 45 octets (CONFIRMÉS).
        struct Pm { int param, intOff; };
        const Pm pm[] = {
            { 0x00, 0 }, { 0x01, 1 }, { 0x02, 2 }, { 0x03, 3 },   // R1-R4
            { 0x04, 4 }, { 0x05, 5 },                             // RR1, RR2
            { 0x06, 6 }, { 0x07, 7 }, { 0x08, 8 }, { 0x09, 9 },   // L1-L4
            { 0x0A, 10 }, { 0x0B, 11 },                           // RL1, RL2
            { 0x0E, 14 },                                         // L0
            { 0x1B, 29 },                                         // TL (niveau de sortie)
            { 0x26, 44 }                                          // Fine
        };

        for (auto& op : ops)
            for (auto& m : pm)
            {
                const int off = op.base + m.intOff;
                if (off < sz)
                    out.add ({ op.group, 0, 0, m.param, (int) d[off] });
            }

        // AFM element common (group 0x05, addrHi=0).
        // ALGNUM @377 (0-indexé) : confirmé par diff single-param (RikielBass algo 16->1
        // = byte 377 15->0). Le slider algo applique son propre offset d'affichage (1..45).
        out.add ({ 0x05, 0, 0, 0x00, (int) d[377] });

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
