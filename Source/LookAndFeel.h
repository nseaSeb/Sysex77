/*
  ==============================================================================

    LookAndFeel.h
    Created: 18 Nov 2018 8:34:30pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
static Colour SYColBackground { Colours::black };
static Colour   SYColAlt      { Colour (0xff202020) };
//static Colour   SYColText;
static Colour   SYColLabel    { Colours::grey };
static Colour   SYColSelected { Colours::darkorange };

// Thème de l'interface : 0 = Moderne (sombre/orange), 1 = Atari vintage (GEM mono)
static int SYTheme = 0;

// Applique les couleurs globales courantes (SYCol*) au LookAndFeel par défaut.
// Ces couleurs sont lues au moment du paint -> un repaint suffit pour les voir.
inline void syncSyLookAndFeel()
{
    auto& lf = LookAndFeel::getDefaultLookAndFeel();
    auto  ink = SYColBackground.contrasting(); // texte lisible sur le fond

    lf.setColour (ResizableWindow::backgroundColourId, SYColBackground);
    lf.setColour (DocumentWindow::backgroundColourId,  SYColBackground);

    lf.setColour (Label::textColourId,             ink);
    lf.setColour (GroupComponent::textColourId,    ink);
    lf.setColour (GroupComponent::outlineColourId, ink.withAlpha (0.5f));

    lf.setColour (TextButton::buttonColourId,      SYColAlt);
    lf.setColour (TextButton::textColourOffId,     ink);
    lf.setColour (TextButton::textColourOnId,      SYColBackground);

    lf.setColour (ComboBox::backgroundColourId,    SYColAlt);
    lf.setColour (ComboBox::textColourId,          ink);
    lf.setColour (ComboBox::arrowColourId,         ink);
    lf.setColour (ComboBox::outlineColourId,       ink.withAlpha (0.4f));
}

inline void applySyTheme (int theme)
{
    SYTheme = theme;

    if (theme == 1) // Atari ST / GEM monochrome
    {
        SYColBackground = Colour (0xffb9b9b9); // gris « bureau » ST
        SYColAlt        = Colour (0xffe6e6e6);
        SYColLabel      = Colours::black;
        SYColSelected   = Colour (0xff1a1a1a); // encre quasi-noire
    }
    else // Moderne (défaut)
    {
        SYColBackground = Colours::black;
        SYColAlt        = Colour (0xff202020);
        SYColLabel      = Colours::grey;
        SYColSelected   = Colours::darkorange;
    }

    syncSyLookAndFeel();
}


class AfmOscLookAndFeel : public LookAndFeel_V4
{
public:
    AfmOscLookAndFeel()
    {

        imageKnob = ImageCache::getFromMemory(BinaryData::OscAfm_png, BinaryData::OscAfm_pngSize);
  
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
        filledArc1.addPieSegment(rx, ry, rw + 2, rw + 2, (360 * (juce::MathConstants<float>::pi  / 180.0)), angle, 0.75);
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
        g.setColour(SYColBackground.contrasting());
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
        g.setColour(SYColBackground.contrasting());
        g.fillPath(p);
        
        
    }
    //==============================================================================
    
};
