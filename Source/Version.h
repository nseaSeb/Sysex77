/*
  ==============================================================================

    Version.h
    Numéro de version centralisé + identifiant de build (date/heure de compilation).
    Affiché dans le titre de la fenêtre et dans l'onglet Setting, pour qu'on parle
    toujours de la même version.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace Sysex77
{
    // Bump à chaque jalon notable.
    static constexpr const char* kVersion = "1.5.1-dev";

    /** Version + horodatage de compilation (identifie un build précis). */
    inline juce::String versionString()
    {
        return juce::String ("Sysex77 v") + kVersion + "  (build " __DATE__ " " __TIME__ ")";
    }

    /** Titre de la fenetre principale : version + nom de la voix ouverte (rien si inconnu,
        ex. apres un bloc trop court qui a remplace l'edit buffer). */
    inline juce::String windowTitle (const juce::String& voiceName)
    {
        return juce::String ("Sysex77 v") + kVersion
               + (voiceName.isNotEmpty() ? "  -  " + voiceName : juce::String());
    }
}
