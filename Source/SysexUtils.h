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
}
