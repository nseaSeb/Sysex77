/*
  ==============================================================================

    AppSettings.h
    Stockage de préférences PARTAGÉ (un seul fichier .settings).

    IMPORTANT : tout le monde doit passer par getAppProps()/getAppSettings().
    Deux ApplicationProperties distincts sur le même fichier s'écraseraient
    mutuellement leurs clés à la sauvegarde (perte de données).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

inline juce::ApplicationProperties& getAppProps()
{
    static juce::ApplicationProperties props;
    static const bool once = [&]
    {
        juce::PropertiesFile::Options options;
        options.applicationName     = ProjectInfo::projectName;
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        options.folderName          = juce::File::getSpecialLocation (juce::File::SpecialLocationType::userApplicationDataDirectory)
                                          .getChildFile ("SYSEX77").getFullPathName();
        options.storageFormat       = juce::PropertiesFile::storeAsXML;
        props.setStorageParameters (options);
        return true;
    }();
    juce::ignoreUnused (once);
    return props;
}

inline juce::PropertiesFile* getAppSettings() { return getAppProps().getUserSettings(); }
