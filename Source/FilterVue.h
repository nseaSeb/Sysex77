/*
  ==============================================================================

    FilterVue.h
    Created: 29 Nov 2018 12:59:25am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

int intNumber;

#include "../JuceLibraryCode/JuceHeader.h"
#include "ADSR.h"
#include "Filter1.h"
#include "Filter2.h"
#include "CommonFilter.h"



//==============================================================================

struct TabFilter  : public TabbedComponent
{
    TabFilter (bool isRunningComponenTransformsDemo)
    : TabbedComponent (TabbedButtonBar::TabsAtTop)
    {
        auto colour = findColour (ResizableWindow::backgroundColourId);
        addTab (TRANS("Common "),     colour, new CommonFilter (), true);
        addTab (TRANS("Filter 1"),     colour, new Filter1 (), true);
        addTab (TRANS("Filter 2"),     colour, new Filter2 (), true);
        addTab  (TRANS("Close"), colour, nullptr,false);
        
                                                              
    }

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabFilter)
};

//=============================================================================

/*
*/
class FilterVue    : public Component,public Timer
{
public:
    FilterVue(bool isRunningComponenTransformsDemo = false)
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

    ~FilterVue()
    {
    //    removeKeyListener(this);

        stopTimer();
 
        
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
        
        comboPreset.setBounds(340,2,getWidth()-400,24);
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
        
        if(tabs.getCurrentTabIndex()==3)
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
    
TabFilter tabs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterVue)
};
