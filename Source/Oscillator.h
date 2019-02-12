/*
  ==============================================================================

    Oscillator.h
    Created: 25 Nov 2018 8:29:30pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class Oscillator    : public Component, public TextButton::Listener
{
public:
    Oscillator()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
     //   setBounds(getBoundsInParent());
        
        addAndMakeVisible(btClose);
        btClose.addListener(this);
        
    }

    ~Oscillator()
    {
        btClose.removeListener(this);
    }
 
    void buttonClicked (Button* button) override
    {
        setAlwaysOnTop(false);
        setVisible(false);
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
           draws some placeholder text to get you started.

           You should replace everything in this method with your own
           drawing code..
        */

        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background

        g.setColour (Colours::green);
      //  g.fillAll();
        g.drawRect (getLocalBounds(), 4);   // draw an outline around the component

        g.setColour (Colours::white);
        g.setFont (14.0f);
        g.drawText ("Oscillator", getLocalBounds(),
                    Justification::centred, true);   // draw some placeholder text
    }

    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        btClose.setBounds(0, 0, 48, 48);
    }

private:
    TextButton  btClose { "X" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oscillator)
};
