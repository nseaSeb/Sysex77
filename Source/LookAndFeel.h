/*
  ==============================================================================

    LookAndFeel.h
    Created: 18 Nov 2018 8:34:30pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
static Colour SYColBackground;
static Colour   SYColAlt;
//static Colour   SYColText;
static Colour   SYColLabel;
static Colour   SYColSelected;


class AfmOscLookAndFeel : public LookAndFeel_V4
{
public:
    AfmOscLookAndFeel()
    {

        imageKnob = ImageCache::getFromMemory(BinaryData::AfmOsc_png, BinaryData::AfmOsc_pngSize);
  
}

//==============================================================================
void drawRotarySlider(Graphics& g,
                                       int x, int y, int width, int height, float sliderPos,
                                       float rotaryStartAngle, float rotaryEndAngle, Slider& slider)
{
    
    if (imageKnob.isValid())
    {
        const double rotation = (slider.getValue()
                                 - slider.getMinimum())
        / (slider.getMaximum()
           - slider.getMinimum());
        
        const int frames = imageKnob.getHeight() / imageKnob.getWidth();
        const int frameId = (int)ceil(rotation * ((double)frames - 1.0));
        const float radius = jmin(width / 2.0f, height / 2.0f);
        const float centerX = x + width * 0.5f;
        const float centerY = y + height * 0.5f;
        const float rx = centerX - radius - 1.0f;
        const float ry = centerY - radius;
        
        
        g.drawImage(imageKnob,(int)rx,(int)ry,2 * (int)radius,2 * (int)radius,0,frameId*imageKnob.getWidth(),imageKnob.getWidth(), imageKnob.getWidth());
    }

}
    Image imageKnob;
};
//==============================================================================

class CustomLookAndFeel : public LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        
    }
    
    void drawRotarySlider(Graphics& g,
                          int x, int y, int width, int height, float sliderPos,
                          float rotaryStartAngle, float rotaryEndAngle, Slider& slider) override
    {
        
        const float centreX = width * 0.5f;
        const float centreY = height * 0.5f;
        const float radius = jmin(centreX, centreY)-2;
        const float rx = centreX - radius;
        const float ry = centreY - radius;
        const float rw = radius + radius;
        
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        // Cercle interieur
        const float dotradius = radius * (float)0.7;
        const float dotradius2 = rw * (float)0.7;
        
        //    g.setColour(Colour(0x60000000));
        //     g.fillAll();
        
        g.setColour(SYColBackground); //colour background
        Path filledArc; //fond eteind
        filledArc.addPieSegment(rx, ry, rw +1 , rw +1, rotaryStartAngle, rotaryEndAngle, 0.6);
        g.fillPath(filledArc);
       
        //====
      
      //  g.setGradientFill(ColourGradient(SYColSelected, centreX, centreY, SYColBackground, rx,ry, true) );
        g.setColour(SYColSelected); // level draw
        Path filledArc1;
        filledArc1.addPieSegment(rx, ry, rw + 2, rw + 2, (360 * (float_Pi / 180.0)), angle, 0.75);
        g.fillPath(filledArc1);
        
        //==
        
        Path bouton;
        
        
        g.setColour(SYColLabel);
       
        
        bouton.addEllipse(centreX - (dotradius),
                          centreY - (dotradius),
                          dotradius2, dotradius2);
        
        g.fillPath(bouton);
        
        Path p; // marque du bouton
        const float pointerThickness = radius * 0.1f;
        p.addRectangle(-pointerThickness * 0.5f, -radius - 1, pointerThickness, dotradius);
        
        p.applyTransform(AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(Colour(0xffffffff));
        g.fillPath(p);
        
        
    }
//==============================================================================

};
class CustomLookAndFeelV2 : public LookAndFeel_V4
{
public:
    CustomLookAndFeelV2()
    {
        
    }
    void drawRotarySlider(Graphics& g,
                          int x, int y, int width, int height, float sliderPos,
                          float rotaryStartAngle, float rotaryEndAngle, Slider& slider) override
    {
        
        const float centreX = width * 0.5f;
        const float centreY = height * 0.5f;
        const float radius = jmin(centreX, centreY);
        const float rx = centreX - radius;
        const float ry = centreY - radius;
        const float rw = radius + radius;
        
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        // Cercle interieur
        const float dotradius = radius * (float)0.7;
        const float dotradius2 = rw * (float)0.7;
        
        //    g.setColour(Colour(0x60000000));
        //     g.fillAll();
        
        g.setColour(SYColBackground); //colour background
        Path filledArc; //fond eteind
        filledArc.addPieSegment(rx, ry, rw + 1, rw + 1, rotaryStartAngle, rotaryEndAngle, 0.7);
        g.fillPath(filledArc);
        
        //====
        
        g.setColour(SYColSelected);
        Path filledArc1;
        filledArc1.addPieSegment(rx, ry, rw + 1, rw + 1, rotaryStartAngle, angle, 0.8);
        g.fillPath(filledArc1);
        
        //==
        
        Path bouton;
        
        
        g.setColour(SYColLabel);
        //       g.setGradientFill(ColourGradient(Colour(0xff20a0ff), centreX, centreY, Colour(0xff000020), rx,ry, true) );
        
        bouton.addEllipse(centreX - (dotradius),
                          centreY - (dotradius),
                          dotradius2, dotradius2);
        
        g.fillPath(bouton);
        
        Path p; // marque du bouton
        const float pointerThickness = radius * 0.1f;
        p.addRectangle(-pointerThickness * 0.5f, -radius - 1, pointerThickness, dotradius);
        
        p.applyTransform(AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(Colour(0xffffffff));
        g.fillPath(p);
        
        
    }
    //==============================================================================
    
};
