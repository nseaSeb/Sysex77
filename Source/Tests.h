/*
  ==============================================================================

    Tests.h
    Tests unitaires JUCE (juce::UnitTest) pour la logique « pure » de Sysex77.
    Lancés via l'argument de ligne de commande --test (voir Main.cpp).

  ==============================================================================
*/

#pragma once
#include "SysexUtils.h"
#include "SysexBus.h"
#include "SyParam.h"
#include "Version.h"
#include "PresetHexView.h"   // outil dev : sondage offsets -> params
#include "VoiceSession.h"   // decoupage MidiDemo : branchee dans MidiDemo (etape 2), caracterisee ici
#include "TestBench.h"      // banc de test hardware : logique pure SyBench

#include "tests/EchoLoopTests.h"
#include "tests/SysexUtilsTests.h"
#include "tests/SequencerTests.h"
#include "tests/EditHistoryTests.h"
#include "tests/VoiceTemplatesTests.h"
#include "tests/ShortcutsTests.h"
#include "tests/VoiceSessionHelpersTests.h"
#include "tests/VoiceSessionTests.h"
#include "tests/TestBenchTests.h"
#include "tests/MidiMonitorTests.h"
#include "tests/RxClassifierTests.h"
