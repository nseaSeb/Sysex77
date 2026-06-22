/*
  ==============================================================================

    SysexUtils.h
    Fonctions « pures » (sans dépendance GUI) liées au MIDI/Sysex du SY77.
    Extraites des composants pour être réutilisables ET testables (voir Tests.h).

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

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
}
