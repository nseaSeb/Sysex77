/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
 This component lives inside our window, and this is where you should put all
 your controls and content.
 */
class FMOperator   : public Component
{
public:
    //==============================================================================
    FMOperator()
    {

        setSliderStyle(sliderIn1);
        setSliderStyle(sliderIn2);

        
        addAndMakeVisible(sliderOut1);
        sliderOut1.setSliderStyle(Slider::SliderStyle::LinearBar);

        sliderOut1.setTextBoxStyle(Slider::NoTextBox, true, 10, 10);
        sliderOut1.setRange(0, 127);
        sliderOut1.setNumDecimalPlacesToDisplay(0);
        sliderOut1.setColour(Slider::ColourIds::trackColourId, Colours::darkorange);
        sliderOut1.setPopupDisplayEnabled(false, true, this);
        sliderOut1.setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::darkorange);
        
        
    }
    ~FMOperator()
    {
        
    }
    void setSliderStyle(Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setSliderStyle(Slider::SliderStyle::LinearBarVertical);
        
        slider.setTextBoxStyle(Slider::NoTextBox, true, 10, 10);
        slider.setRange(0, 7);
        slider.setNumDecimalPlacesToDisplay(0);
        slider.setColour(Slider::ColourIds::trackColourId, Colours::darkorange);
        slider.setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::darkorange);
        slider.setPopupDisplayEnabled(false, true, this);
        
    }
    //==============================================================================
    void paint (Graphics& g) override
    {
        
        g.setColour ( Colours::darkorange);
        g.drawRect(2, 6, getWidth()-4, getHeight()-12);
        g.fillRect(10, getHeight()-8, 3, 6);
        g.fillRect(10, 0, 3, 6);
        g.fillRect(getWidth()-14, 0, 3, 6);
        
        g.setColour(Colours::white);
        g.drawText(getName(), 0, 10, getWidth(), getHeight(), Justification::centredTop);
        
    }
    void resized() override
    {
     //   sliderIn1.setBoundsRelative(0.02f, 0.14f, 0.2f, 0.5f);
        /*
        sliderIn1.setBounds(2, 6, 8, getHeight()-20);
        sliderIn2.setBounds(getWidth()-12,6,8,getHeight()-20);
        sliderOut1.setBounds(2, getHeight()-20, getWidth()-4, 8);
         */
    }
    
private:
    //==============================================================================
    // Your private member variables go here...
    Slider sliderIn1;
    Slider sliderIn2;
    Slider sliderOut1;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FMOperator)
};

class AlgoDraw   : public Component
{
public:
    //==============================================================================
    AlgoDraw()
    {
        addAndMakeVisible(FMOp1);
        addAndMakeVisible(FMOp2);
        addAndMakeVisible(FMOp3);
        addAndMakeVisible(FMOp4);
        addAndMakeVisible(FMOp5);
        addAndMakeVisible(FMOp6);
        FMOp1.setName("op1");
        FMOp2.setName("op2");
        FMOp3.setName("op3");
        FMOp4.setName("op4");
        FMOp5.setName("op5");
        FMOp6.setName("op6");
    }
    ~AlgoDraw()
    {
        
    }
    void setAlgo (int number)
    {
        intAlgoNumber = number;
    }
    //==============================================================================
    void paint (Graphics& g) override
    {

     
        
        auto colour = Colours::darkorange;
        g.setColour (colour);
        g.drawRect(0, 0, getWidth(), getHeight());
        float h6 = 0.85f;
        float h5 = 0.68f;
        float h4 = 0.52f;
        float h3 = 0.35f;
        float h2 = 0.18f;
        float h1 = 0.01f;
        
        if(intAlgoNumber == 1)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h2, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h3, h1, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 2)
        {
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h2, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h2, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 3)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h2, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h3, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 4)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h2, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h4, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 5)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h2, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h5, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 6)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h3, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h5, h3, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 7)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h3, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 8)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h5, h4, 0.5f, 0.14f);


        }
        if(intAlgoNumber == 9)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h4, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h2, h3, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 10)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h4, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 11)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h5, h5, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 12)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h3, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 13)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h2, h4, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 14)
        {
            
            FMOp1.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h2, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h2, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h5, h4, 0.5f, 0.14f);
    
        }
        if(intAlgoNumber == 15)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h2, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h5, h4, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 16)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h5, h5, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 17)
        {
            
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h5, h4, 0.5f, 0.14f);

        }
        if(intAlgoNumber == 18)
        {
            FMOp1.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h2, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h2, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h5, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 19)
        {
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h5, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h4, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 20)
        {
            FMOp1.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h2, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h5, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h6, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 21)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h3, h2, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 22)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h3, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 23)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h4, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 24)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h3, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 25)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h5, h4, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 26)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h4, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 27)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h5, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 28)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h2, h4, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 29)
        {
            FMOp1.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h2, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h5, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 30)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h3, h3, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 31)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h4, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 32)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h2, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h2, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h3, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 33)
        {
            FMOp1.setBoundsRelative(h5, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h5, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h2, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber==34)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h4, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h3, h4, 0.5f, 0.14f);
        }
        if(intAlgoNumber==35)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h4, h4, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h2, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h3, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber==36)
        {
            FMOp1.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h5, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h2, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h3, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber==37)
        {
            FMOp1.setBoundsRelative(h5, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h3, h3, 0.5f, 0.14f);
        }
        if(intAlgoNumber==38)
        {
            FMOp1.setBoundsRelative(h5, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h4, 0.5f, 0.14f);
        }
        if(intAlgoNumber==39)
        {
            FMOp1.setBoundsRelative(h5, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h4, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h4, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber==40)
        {
            FMOp1.setBoundsRelative(h5, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h3, h4, 0.5f, 0.14f);
        }
        if(intAlgoNumber==41)
        {
            FMOp1.setBoundsRelative(h5, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h2, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h3, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber==42)
        {
            FMOp1.setBoundsRelative(h5, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h5, h5, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h4, h5, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h3, h5, 0.5f, 0.14f);
        }
        if(intAlgoNumber==43)
        {
            FMOp1.setBoundsRelative(h5, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h2, h5, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h2, h4, 0.5f, 0.14f);
        }
        if(intAlgoNumber == 44)
        {
            FMOp1.setBoundsRelative(h6, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h5, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h2, h5, 0.5f, 0.14f);

        }
        if(intAlgoNumber==45)
        {
            
            FMOp1.setBoundsRelative(h6, h6, 0.5f, 0.14f);
            FMOp2.setBoundsRelative(h5, h6, 0.5f, 0.14f);
            FMOp3.setBoundsRelative(h4, h6, 0.5f, 0.14f);
            FMOp4.setBoundsRelative(h3, h6, 0.5f, 0.14f);
            FMOp5.setBoundsRelative(h2, h6, 0.5f, 0.14f);
            FMOp6.setBoundsRelative(h1, h6, 0.5f, 0.14f);

        }
        roundSize(FMOp1);
        roundSize(FMOp2);
        roundSize(FMOp3);
        roundSize(FMOp4);
        roundSize(FMOp5);
        roundSize(FMOp6);
    }
    void resized() override
    {
 
    }
    void roundSize (FMOperator& slider)
    {
        int w;
        w = jmin(slider.getWidth(),slider.getHeight());
        slider.setBounds(slider.getX(), slider.getY(), w, w);
    }
private:
    //==============================================================================
    // Your private member variables go here...
    FMOperator FMOp1;
    FMOperator FMOp2;
    FMOperator FMOp3;
    FMOperator FMOp4;
    FMOperator FMOp5;
    FMOperator FMOp6;
    
    int intAlgoNumber;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AlgoDraw)
};
