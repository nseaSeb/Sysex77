/*
  ==============================================================================

    WaveVue.h
    Created: 29 Nov 2018 12:58:40am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "MidiObjects.h"
#include "AWMVue.h"
#include "WaveEg.h"
#include "PitchEG.h"
#include "AfmLfo.h"
//==============================================================================

struct TabWave  : public TabbedComponent
{
    TabWave (bool isRunningComponenTransformsDemo)
    : TabbedComponent (TabbedButtonBar::TabsAtTop)
    {
        auto colour = findColour (ResizableWindow::backgroundColourId);
        addTab (TRANS("WAVE"),     colour, new AWMVue (), true);
        addTab (TRANS("Volume EG"),     colour, new WaveEg (), true);
        addTab (TRANS("Pitch EG"),     colour, new PitchEg (), true);
        addTab (TRANS("LFO"),          colour, new AfmLfo (), true);   // multi LFO AWM (0x07/0x12-0x18)
        // (Ancien faux onglet « Close » supprime : fermeture via l'en-tete commun de VoicePage.)
        
    }
    
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabWave)
};

//=============================================================================
class WaveVue    : public Component
{
public:
    WaveVue(bool isRunningComponenTransformsDemo = false)
    :tabs (isRunningComponenTransformsDemo)
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        addAndMakeVisible(tabs);
        
        
    }
    
    ~WaveVue()
    {
        //    removeKeyListener(this);
        
        
        
    }
    void setTabVolume()
    {
        tabs.setCurrentTabIndex(1);
    }
    /** A la fermeture (VoicePage::closeOpenEditor) : la prochaine ouverture repart sur WAVE. */
    void resetTab() { tabs.setCurrentTabIndex (0); }
    void setElementNumber ( int number, UndoManager& undoman)
    {
        
        for(auto i =0; i<tabs.getNumTabs();i++)
        {
            auto* content = tabs.getTabContentComponent (i);
            // Vue Wave : Volume EG = amp-EG AWM (0x07), Pitch EG = PEG AWM (0x07), pas AFM.
            if (auto* filterTab = dynamic_cast<ElementComponent*> (content))
            {
                filterTab->setAwmMode (true);
                filterTab->setElementNumber (number, undoman);
            }
        }
    }

    
    void resized() override
    {
  
        tabs.setBounds (0,0,getWidth(),getHeight());
        
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
        
        
    }
//==============================================================================
    
private:

    TabWave tabs;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveVue)
};

//==============================================================================
