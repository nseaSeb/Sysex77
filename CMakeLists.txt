# =============================================================================
# Sysex77 — build CMake (alternative à Projucer, pour contribution / CI).
#
# Coexiste avec Sysex77.jucer : le .jucer reste la référence pour le build Xcode
# historique ; ce CMake permet un build sans Projucer (utile en CI et pour les
# contributeurs). Les ressources binaires régénèrent les mêmes symboles
# BinaryData:: que Projucer.
#
# Usage :
#   cmake -B build -DJUCE_DIR=/Applications/JUCE
#   cmake --build build
#   ./build/Sysex77_artefacts/Sysex77.app/Contents/MacOS/Sysex77 --test
# =============================================================================
cmake_minimum_required(VERSION 3.22)
project(Sysex77 VERSION 1.1.0 LANGUAGES C CXX)

# --- Résolution de JUCE -------------------------------------------------------
# 1) -DJUCE_DIR=/chemin/vers/JUCE (arbre source local), sinon
# 2) /Applications/JUCE (install Projucer macOS), sinon
# 3) téléchargement automatique de JUCE 8.0.7 (FetchContent) -> « just works »,
#    notamment pour les utilisateurs non-Mac sans JUCE installé.
set(JUCE_DIR "" CACHE PATH "Path to an existing JUCE source tree (optional)")
if(JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(STATUS "Sysex77: JUCE local = ${JUCE_DIR}")
    add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
elseif(EXISTS "/Applications/JUCE/CMakeLists.txt")
    message(STATUS "Sysex77: JUCE = /Applications/JUCE")
    add_subdirectory("/Applications/JUCE" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
else()
    message(STATUS "Sysex77: JUCE introuvable -> téléchargement (FetchContent) de JUCE 8.0.7")
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.7
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_gui_app(Sysex77
    PRODUCT_NAME "Sysex77"
    COMPANY_NAME "Sebastien Portrait"
    BUNDLE_ID    "com.sebastienportrait.sysex77"
    ICON_BIG     "${CMAKE_CURRENT_SOURCE_DIR}/Ressources/AppIcon.png")

juce_generate_juce_header(Sysex77)

target_sources(Sysex77 PRIVATE Source/Main.cpp)

# Ressources binaires — mêmes fichiers que le .jucer ; JUCE dérive les symboles
# BinaryData:: du nom de fichier (espaces/points -> '_'), identiques à Projucer.
juce_add_binary_data(Sysex77Data SOURCES
    Ressources/OscAfm.png
    Ressources/TableData.xml
    Ressources/Audio.png
    "Ressources/SY77 YAMAHA.png"
    Ressources/AFM.png
    Ressources/French.txt
    Ressources/SY77Waves.xml
    Ressources/SY99Waves.xml)

target_compile_features(Sysex77 PRIVATE cxx_std_17)

target_compile_definitions(Sysex77 PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1       # l'app utilise des dialogues modaux synchrones (AlertWindow::showOkCancelBox…)
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Sysex77,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Sysex77,JUCE_VERSION>")

target_link_libraries(Sysex77 PRIVATE
    Sysex77Data
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_osc
    juce::juce_gui_extra
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_events
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
