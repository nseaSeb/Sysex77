// MidiMonitor.h - rendu texte du moniteur MIDI (logique pure, testee dans tests/).
// Extrait verbatim de MidiDemo : l'etat (mode brut, device, omni) est passe en parametre.
#pragma once

namespace SyMonitor
{
    // Octets bruts d'un message MIDI en hex ("F0 43 10 …"). Sysex inclut F0/F7.
    inline juce::String bytesToHex (const juce::MidiMessage& m)
    {
        const juce::uint8* d = m.getRawData();
        const int n = m.getRawDataSize();
        juce::String s;
        for (int i = 0; i < n; ++i)
            s << juce::String::toHexString (d[i]).paddedLeft ('0', 2).toUpperCase() << (i + 1 < n ? " " : "");
        return s;
    }

    // Rend UNE ligne de moniteur pour un message, selon le mode courant (brut/décodé).
    // Retourne false si le message doit être OMIS (bruit filtré, ou dédup en mode décodé).
    // `dedup` porte l'état de dédup entre appels (en mode décodé : une ligne par adresse).
    inline bool formatLine (const juce::MidiMessage& message, bool monitorRaw, int sysexDeviceNumber,
                            bool sysexReceiveOmni, juce::String& out, juce::String& dedup)
    {
        // --- Mode BRUT : chaque message en hex, AUCUNE déduplication (on veut tout voir).
        if (monitorRaw)
        {
            out << bytesToHex (message) << "\n";
            return true;
        }

        // --- Mode DÉCODÉ : param-change SY77 9 octets -> G/AH/AL/P/val, dédup par adresse.
        if (message.getSysExDataSize() == 9)
        {
            const juce::uint8* d = message.getSysExData();
            if (d[0] == 0x43 && d[2] == 0x34
                && SyVoice::acceptsDevice (d[1], sysexDeviceNumber, sysexReceiveOmni))
            {
                const juce::String key = juce::String (d[3]) + "." + juce::String (d[4]) + "."
                                 + juce::String (d[5]) + "." + juce::String (d[6]);
                if (key == dedup) return false;          // même adresse que la précédente -> omise
                dedup = key;
                auto hx = [] (int v) { return juce::String::toHexString (v).paddedLeft ('0', 2); };
                out << ">> SY77  G=" << hx (d[3])
                    << "  AH=" << hx (d[4]) << "  AL=" << hx (d[5])
                    << "  P="  << hx (d[6])
                    << "   val=" << (int) d[8] << " (0x" << hx (d[8]) << ")\n";
                return true;
            }
            return false; // sysex 9 octets non-SY77 : pas de description brute en mode décodé
        }

        // Bruit filtré pour la RE : CC data-entry, program change, horloge, active-sense, transport.
        if (message.isController() || message.isProgramChange() || message.isMidiClock()
            || message.isActiveSense() || message.isMidiStart() || message.isMidiContinue()
            || message.isMidiStop() || message.isSongPositionPointer())
            return false;

        out << message.getDescription() << "\n";
        return true;
    }

}
