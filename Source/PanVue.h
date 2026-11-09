/*
  ==============================================================================

    PanVue.h
    Overlay plein-page d'édition de la table de PAN d'un élément (EG de pan).
    Calqué sur FilterVue : un TabbedComponent { "Pan EG" } ; fermeture via l'en-tete
    commun de VoicePage (closeOpenEditor). Cf. PanEg.h pour le contenu et les adresses sysex.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PanEg.h"

//==============================================================================
struct TabPan  : public TabbedComponent
{
    TabPan() : TabbedComponent (TabbedButtonBar::TabsAtTop)
    {
        auto colour = findColour (ResizableWindow::backgroundColourId);
        addTab (TRANS ("Pan EG"), colour, new PanEg(), true);
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabPan)
};

//==============================================================================
class PanVue    : public Component
{
public:
    PanVue()
    {
        addAndMakeVisible (tabs);
    }
    void resetTab() { tabs.setCurrentTabIndex (0); }

    void setElementNumber (int number, UndoManager& um)
    {
        for (int i = 0; i < tabs.getNumTabs(); ++i)
            if (auto* tab = dynamic_cast<ElementComponent*> (tabs.getTabContentComponent (i)))
                tab->setElementNumber (number, um);
    }

    // À appeler à l'ouverture : relit la table PANNM courante et réapplique les adresses.
    void refresh()
    {
        if (auto* eg = dynamic_cast<PanEg*> (tabs.getTabContentComponent (0)))
            eg->refresh();
    }

    void resized() override { tabs.setBounds (0, 0, getWidth(), getHeight()); }
    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    }


private:
    TabPan tabs;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanVue)
};
