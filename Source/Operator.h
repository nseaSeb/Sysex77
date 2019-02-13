/*
  ==============================================================================

    Operator.h
    Created: 12 Feb 2019 11:48:10pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
 */
class Operator    : public Component, public TextButton::Listener
{
public:
    Operator()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        //   setBounds(getBoundsInParent());
        
        
        
    }
    
    ~Operator()
    {
        
    }
    
    void buttonClicked (Button* button) override
    {
        
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
        g.setColour (Colours::darkorange);
        
        g.drawText ("AFM operator a implementer", getLocalBounds(),
                    Justification::centred, true);   // draw some placeholder text
    }
    
    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        
    }
    
private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Operator)
};
