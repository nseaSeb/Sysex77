// Tests unitaires : Shortcuts (inclus par Source/Tests.h, ordre d'enregistrement conserve).
#pragma once

//==============================================================================
// Shortcuts (Shortcuts.h): every default parses, no two actions share a key.
class ShortcutsTests : public juce::UnitTest
{
public:
    ShortcutsTests() : juce::UnitTest ("Shortcuts", "Sysex77") {}
    void runTest() override
    {
        beginTest ("defaults parse and are unique");
        juce::Array<juce::KeyPress> seen;
        for (auto& a : Shortcuts::actions())
        {
            const auto k = juce::KeyPress::createFromDescription (a.defaultKey);
            expect (k.isValid(), a.id);
            for (auto& s : seen) expect (! (Shortcuts::normalise (s) == Shortcuts::normalise (k)), a.id);
            seen.add (k);
        }
        beginTest ("arrow defaults map to the arrow key codes");
        expectEquals (juce::KeyPress::createFromDescription ("command + cursor down").getKeyCode(), juce::KeyPress::downKey);
        expectEquals (juce::KeyPress::createFromDescription ("command + cursor left").getKeyCode(), juce::KeyPress::leftKey);
    }
};

static ShortcutsTests shortcutsTests;
