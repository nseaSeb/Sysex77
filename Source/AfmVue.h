/*
  ==============================================================================

    AfmVue.h
    Created: 29 Nov 2018 12:59:02am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Operator.h"

//==============================================================================
//==============================================================================

struct TabAFM  : public TabbedComponent
{
    TabAFM (bool isRunningComponenTransformsDemo)
    : TabbedComponent (TabbedButtonBar::TabsAtTop)
    {
        auto colour = findColour (ResizableWindow::backgroundColourId);
        addTab (TRANS("AFM OP"),     colour, new Operator (), true);
        addTab (TRANS("AFM OSC"),     colour, new Oscillator (), true);
        addTab (TRANS("Volume EG"),     colour, new WaveEg (), true);
        addTab (TRANS("Pitch EG"),     colour, new PitchEg (), true);
        addTab  (("Close"), colour, nullptr,false);
        
    }
    
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabAFM)
};

//=============================================================================
class AFMVue    : public Component, public Timer
{
public:
    AFMVue(bool isRunningComponenTransformsDemo = false)
    :tabs (isRunningComponenTransformsDemo)
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        addAndMakeVisible(tabs);
        
        addAndMakeVisible(comboPreset);
        comboPreset.addItem("Inactif pour le moment", 1);
        comboPreset.setSelectedId(1);
        addAndMakeVisible(labelPreset);
        addAndMakeVisible(txtSave);
        labelPreset.attachToComponent(&comboPreset, true);
        
        startTimer(500);
        
        
    }
    
    ~AFMVue()
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
        
        comboPreset.setBounds(400,2,getWidth()-460,24);
        txtSave.setBounds(getWidth()-54,2,48,24);
        
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
        
        if(tabs.getCurrentTabIndex()==4)
        {
            tabs.setCurrentTabIndex(0);
            setVisible(false);
            
            //repaint();
        }
    }
    
private:
    ComboBox comboPreset;
    Label labelPreset {"preset","Presets : "};
    TextButton txtSave {"save","Save"};
    
    TabAFM tabs;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AFMVue)
};

//==============================================================================
