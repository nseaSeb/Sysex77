/*
  ==============================================================================

    Effects.h
    Éditeur de la section effets du SY77 (structure du manuel) : Effect Mode +
    2 unités de Modulation + 2 unités de Reverb/effet, chacune type + paramètres.

    RENDU / ÉTAT SEUL pour l'instant : tout est lié à valueTreeVoice, AUCUN envoi
    sysex (les adresses/listes de types exactes seront affinées via la spec / le
    synthé, cf. project-backlog). Voir [[feedback-hardware-safety]].

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Values.h"
#include "LookAndFeel.h"

class EffectsPage : public juce::Component
{
public:
    EffectsPage()
    {
        addAndMakeVisible (labMode);
        labMode.setText ("Effect Mode", juce::dontSendNotification);
        labMode.setColour (juce::Label::textColourId, SYColLabel);
        setupCombo (comboMode, IDs::EFFECTMODE, { "Mode 1", "Mode 2", "Mode 3", "Mode 4" });

        // Unités de modulation (chorus/flanger/symphonic…)
        const juce::StringArray modTypes { "Off", "Chorus", "Flanger", "Symphonic", "Celeste", "Phaser", "Tremolo" };
        setupUnit (grpMod1, "Modulation 1", comboMod1Type, IDs::MOD1TYPE, modTypes,
                   { { &sliderMod1Rate, IDs::MOD1RATE, "Rate" }, { &sliderMod1Depth, IDs::MOD1DEPTH, "Depth" }, { &sliderMod1Level, IDs::MOD1LEVEL, "Level" } });
        setupUnit (grpMod2, "Modulation 2", comboMod2Type, IDs::MOD2TYPE, modTypes,
                   { { &sliderMod2Rate, IDs::MOD2RATE, "Rate" }, { &sliderMod2Depth, IDs::MOD2DEPTH, "Depth" }, { &sliderMod2Level, IDs::MOD2LEVEL, "Level" } });

        // Unités de reverb/effet (reverb/delay/EQ/distorsion…)
        const juce::StringArray revTypes { "Off", "Hall", "Room", "Plate", "Delay", "Echo", "Distortion", "EQ" };
        setupUnit (grpRev1, "Reverb / Effect 1", comboRev1Type, IDs::REV1TYPE, revTypes,
                   { { &sliderRev1Time, IDs::REV1TIME, "Time" }, { &sliderRev1Level, IDs::REV1LEVEL, "Level" } });
        setupUnit (grpRev2, "Reverb / Effect 2", comboRev2Type, IDs::REV2TYPE, revTypes,
                   { { &sliderRev2Time, IDs::REV2TIME, "Time" }, { &sliderRev2Level, IDs::REV2LEVEL, "Level" } });
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (12);
        auto top = r.removeFromTop (28);
        labMode.setBounds (top.removeFromLeft (90));
        comboMode.setBounds (top.removeFromLeft (160).reduced (0, 2));

        r.removeFromTop (8);
        // grille 2x2 des unités
        auto rowH = r.getHeight() / 2;
        auto top2 = r.removeFromTop (rowH).reduced (0, 4);
        auto bot2 = r.reduced (0, 4);
        const int colW = top2.getWidth() / 2;
        grpMod1.setBounds (top2.removeFromLeft (colW).reduced (4));
        grpMod2.setBounds (top2.reduced (4));
        grpRev1.setBounds (bot2.removeFromLeft (colW).reduced (4));
        grpRev2.setBounds (bot2.reduced (4));

        layoutUnit (grpMod1, comboMod1Type, { &sliderMod1Rate, &sliderMod1Depth, &sliderMod1Level });
        layoutUnit (grpMod2, comboMod2Type, { &sliderMod2Rate, &sliderMod2Depth, &sliderMod2Level });
        layoutUnit (grpRev1, comboRev1Type, { &sliderRev1Time, &sliderRev1Level });
        layoutUnit (grpRev2, comboRev2Type, { &sliderRev2Time, &sliderRev2Level });
    }

private:
    struct SliderSpec { juce::Slider* slider; juce::Identifier id; juce::String name; };

    void setupCombo (juce::ComboBox& c, const juce::Identifier& id, const juce::StringArray& items, int firstId = 1)
    {
        addAndMakeVisible (c);
        for (int i = 0; i < items.size(); ++i)
            c.addItem (items[i], firstId + i);
        c.setSelectedId (juce::jlimit (firstId, firstId + items.size() - 1,
                                       (int) valueTreeVoice.getProperty (id, firstId)), juce::dontSendNotification);
        c.onChange = [this, &c, id] { valueTreeVoice.setProperty (id, c.getSelectedId(), nullptr); };
    }

    void setupSlider (juce::Slider& s, const juce::Identifier& id, juce::Label& lab, const juce::String& name)
    {
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 44, 16);
        s.setRange (0, 127, 1);
        s.setColour (juce::Slider::thumbColourId, SYColSelected);
        s.getValueObject().referTo (valueTreeVoice.getPropertyAsValue (id, nullptr));
        addAndMakeVisible (lab);
        lab.setText (name, juce::dontSendNotification);
        lab.setJustificationType (juce::Justification::centred);
        lab.setColour (juce::Label::textColourId, SYColLabel);
        lab.attachToComponent (&s, false);
    }

    void setupUnit (juce::GroupComponent& grp, const juce::String& title,
                    juce::ComboBox& typeCombo, const juce::Identifier& typeId, const juce::StringArray& types,
                    std::initializer_list<SliderSpec> sliders)
    {
        grp.setText (title);
        addAndMakeVisible (grp);
        setupCombo (typeCombo, typeId, types);
        for (auto& sp : sliders)
        {
            sliderLabels.add (new juce::Label());
            setupSlider (*sp.slider, sp.id, *sliderLabels.getLast(), sp.name);
        }
    }

    void layoutUnit (juce::GroupComponent& grp, juce::ComboBox& typeCombo, std::initializer_list<juce::Slider*> sliders)
    {
        auto b = grp.getBounds().reduced (10, 22);
        typeCombo.setBounds (b.removeFromTop (24));
        b.removeFromTop (16); // place pour les libellés des knobs
        const int n = (int) sliders.size();
        if (n == 0) return;
        const int kw = b.getWidth() / n;
        int i = 0;
        for (auto* s : sliders)
            s->setBounds (b.getX() + i++ * kw, b.getY(), kw, b.getHeight());
    }

    juce::Label labMode;
    juce::ComboBox comboMode;

    juce::GroupComponent grpMod1, grpMod2, grpRev1, grpRev2;
    juce::ComboBox comboMod1Type, comboMod2Type, comboRev1Type, comboRev2Type;
    juce::Slider sliderMod1Rate, sliderMod1Depth, sliderMod1Level;
    juce::Slider sliderMod2Rate, sliderMod2Depth, sliderMod2Level;
    juce::Slider sliderRev1Time, sliderRev1Level;
    juce::Slider sliderRev2Time, sliderRev2Level;
    juce::OwnedArray<juce::Label> sliderLabels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsPage)
};
