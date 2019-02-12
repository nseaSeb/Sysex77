/*
  ==============================================================================

    Hook.h
    Created: 27 Nov 2018 12:40:11am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class Hook    : public Component, public ChangeBroadcaster
{
public:
    Hook()
    {
    }

    ~Hook()
    {
    }

    void paint (Graphics& g) override
    {
        auto area = getLocalBounds();
        g.setColour (myColor);
 //       Logger::writeToLog("Hook Area X " + String(area.getX()));
        g.drawRoundedRectangle (area.toFloat(), 5.0f, 3.0f);
        g.setColour(Colour(0x20303030));
        g.fillRoundedRectangle(area.toFloat(), 5.0f);

    }


    void resized() override
    {
        // Just set the limits of our constrainer so that we don't drag ourselves off the screen
        constrainer.setMinimumOnscreenAmounts (getHeight(), getWidth(),
                                               getHeight(), getWidth());
    
        
    }
 
    void mouseEnter	(	const MouseEvent & 	event	) override
    {
            myColor = Colour(0xffffffff);
    }
    void mouseExit	(	const MouseEvent & 	event	) override
    {
        myColor = Colour(0x40ffffff);
    }
    void mouseDown (const MouseEvent& e) override
    {
        // Prepares our dragger to drag this Component
        dragger.startDraggingComponent (this, e);
    }
    
    void mouseDrag (const MouseEvent& e) override
    {
     
        // Moves this Component according to the mouse drag event and applies our constraints to it
        dragger.dragComponent (this, e, &constrainer);
                sendChangeMessage();
    }
    
private:

    ComponentBoundsConstrainer constrainer;
    ComponentDragger dragger;
    Colour myColor {0x40ffffff};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Hook)
};

class Segment    : public Component, public ChangeBroadcaster, public ChangeListener
{
public:
    Segment()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        addAndMakeVisible(hook);
        hook.addChangeListener(this);
     //   hook.setBounds(20, 80, 16, 16);
        // specify here where to send OSC messages to: host URL and UDP port number
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
        
        
    }
    
    ~Segment()
    {
        hook.removeChangeListener(this);
    }
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        intLevel = 100 - (hook.getY()*100)/(getHeight()-16);
        intRate = 100 - (hook.getX()*100) / (getWidth()-16);
   //     Logger::writeToLog(String(hook.getX()));
   //     Logger::writeToLog(String(getWidth()));
   //     Logger::writeToLog("Hook change message"); //Notify a change from a hook
        sendChangeMessage();
        sendOsc();
        repaint(); //force the redraw when hook dragg
    }
    
    void paint (Graphics& g) override
    {
    //    auto area = getLocalBounds();
        g.setColour (myColor);
        
      //  g.drawRoundedRectangle (area.toFloat(), 5.0f, 1.0f);
        g.setColour(Colour(0x20303030));
      //  g.fillRoundedRectangle(area.toFloat(), 5.0f);
/*        String str = strName + " Rate : " + String(intRate);
        str = str + "%  Level : " + String(intLevel) + "%";
        g.setColour(Colour(0xf0ffffff));
        g.addTransform(AffineTransform::rotation(fAngle));
       // g.drawText (str, getLocalBounds(), Justification::centredLeft, true);
        g.drawFittedText( str, -getHeight(), 0, getHeight(), 24, Justification::centred,1);
*/       
    }
    void setLevel (int level)
    {
        intLevel = level;
        hook.setBounds((getWidth()*intRate)/100, (getHeight()*level)/100, 16, 16);
        repaint();
    }
    void setRange (int rateRange, int levelRange)
    {
        intRangeRate = rateRange; // 100% = range
        intRangeLevel = levelRange;
    }
    void setValue (int rate, int level)
    {
        intRate = rate;
        intLevel = level;
        hook.setBounds((getWidth()*rate)/100, (getHeight()*level)/100, 16, 16);
        repaint();
    }
    void sendOsc()
    {
        int rate;
        int level;
        //20 * 100 /5 = 400
        if(intRate == 0)
        {
            rate = intRangeRate * 100;
        }
        else
        {
        rate = intRangeRate * 100 / intRate;
        }
        
        if(intLevel == 0)
        {
            level = intRangeLevel * 100;
        }
        else
        {
        level = intRangeLevel * 100 / intLevel;
        }
        sysexRate[8] = rate;
        sysexLevel[8] = level;
        sender.send(oscAddressPatern, (uint8) sysexRate[0], sysexRate[1], sysexRate[2], sysexRate[3], sysexRate[4], sysexRate[5], sysexRate[6], sysexRate[7], sysexRate[8]);
        sender.send(oscAddressPatern, (uint8) sysexLevel[0], sysexLevel[1], sysexLevel[2], sysexLevel[3], sysexLevel[4], sysexLevel[5], sysexLevel[6], sysexLevel[7], sysexLevel[8]);
        
    }
    void setOscSender (String oscRate, String oscLevel)
    {
        strOscLevel = oscLevel;
        strOscRate = oscRate;
        
    }
    void setMidiSysexRate (int sysexdata[0])
    {
        for(int i = 0; i < 9 ;i++)
            sysexRate[i] = sysexdata[i];
    }
    void setMidiSysexLevel (int sysexdata[0])
    {
        for(int i = 0; i < 9 ;i++)
            sysexLevel[i] = sysexdata[i];
    }
    void setName(String name)
    {
        strName = name;
    }
    int getRate()
    {

        return intRate;
    }
    int getLevel()
    {
        return intLevel;
    }
    int getPositionY()
    {
        return hook.getY();
    }
    int getPositionX()
    {
        return hook.getX();
    }
    void resized() override
    {
   //     Logger::writeToLog( "Segment resize height: " + String(getHeight()));
   //     Logger::writeToLog("Level: " + String(intLevel));

        int calc = (getHeight()) * (100-intLevel)/100;
   //     Logger::writeToLog("Calc:" + String(calc));
        
        hook.setBounds(((getWidth()*intRate)/100)-16, calc - 16, 16, 16);
        // Just set the limits of our constrainer so that we don't drag ourselves off the screen
        constrainer.setMinimumOnscreenAmounts (getHeight(), getWidth(),
                                               getHeight(), getWidth());
        
        
    }
    
    void mouseEnter	(	const MouseEvent & 	event	) override
    {
      //  myColor = Colour(0xffffffff);
    }
    void mouseExit	(	const MouseEvent & 	event	) override
    {
        //myColor = Colour(0x40ffffff);
    }
    void mouseDown (const MouseEvent& e) override
    {
        // Prepares our dragger to drag this Component
 //       dragger.startDraggingComponent (this, e);
    }
    
    void mouseDrag (const MouseEvent& e) override
    {
        
        // Moves this Component according to the mouse drag event and applies our constraints to it
//        dragger.dragComponent (this, e, &constrainer);
//        sendChangeMessage();
    }
    
    
private:
    Hook hook;
    String strOscRate;
    String strOscLevel;
    OSCSender sender;
    String oscAddressPatern {"/SYSEX"};
    String strName ="";
    int intLevel = 0;
    int intRate = 0;
    int intRangeRate = 100;
    int intRangeLevel = 100;

    ComponentBoundsConstrainer constrainer;
    ComponentDragger dragger;
    Colour myColor {0x40ffffff};
      uint8 sysexRate[9];
      uint8 sysexLevel[9];
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Segment)
};
