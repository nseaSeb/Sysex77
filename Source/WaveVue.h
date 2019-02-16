/*
  ==============================================================================

    WaveVue.h
    Created: 29 Nov 2018 12:58:40am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiObjects.h"
#include "AWMVue.h"
#include "WaveEg.h"
#include "PitchEG.h"
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
        addTab  (TRANS("Close"), colour, nullptr,false);
        
    }
    
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabWave)
};

//=============================================================================
class WaveVue    : public Component, public Timer
{
public:
    WaveVue(bool isRunningComponenTransformsDemo = false)
    :tabs (isRunningComponenTransformsDemo)
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        addAndMakeVisible(tabs);
        startTimer(500);
        
        
    }
    
    ~WaveVue()
    {
        //    removeKeyListener(this);
        
        stopTimer();
        
        
    }
    void setTabVolume()
    {
        tabs.setCurrentTabIndex(1);
    }
    void setElementNumber ( int number, UndoManager& undoman)
    {
        
        for(auto i =0; i<tabs.getNumTabs();i++)
        {
            if (auto* filterTab = dynamic_cast<ElementComponent*> (tabs.getTabContentComponent(i)))
            {
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
    void timerCallback() override
    {
        
        if(tabs.getCurrentTabIndex()==3)
        {
            tabs.setCurrentTabIndex(0);
            setVisible(false);
            
            //repaint();
        }
    }
    
private:

    TabWave tabs;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveVue)
};

//==============================================================================
