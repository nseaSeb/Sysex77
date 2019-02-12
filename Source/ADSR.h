/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

  name:             ADSR

  dependencies:     juce_core, juce_data_structures, juce_events, juce_graphics, juce_gui_basics
  exporters:        xcode_mac

  moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1

  type:             Component
  mainClass:        MainComponent

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once
#include "Hook.h"

//==============================================================================
class SADSR   : public Component, public ChangeListener, public ChangeBroadcaster
{
public:
    //==============================================================================
    SADSR()
    {
      //  setSize (600, 400);
    /*
        addSegment(0,10,"R1"); // add a segment of envelope
        addSegment(80,50, "R2");
        addSegment(20,30, "R3");
        addSegment(20,30, "R4");
        setModeHold(true);
        setRelease(50);
        resized();
        repaint();
*/

        
    }

    ~SADSR()
    {
        for (int i = 0; i < arraySegment.size();i++)
        {
            arraySegment[i]->removeChangeListener(this);
        }
    }

    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        Logger::writeToLog("change message"); //Notify a change from a hook
        resized();
        repaint(); //force the redraw when hook dragg
    }
    //==============================================================================
    void paint (Graphics& g) override
    {
        
        g.setColour(Colour(0x10000000));
        
        
        for (int i = 0; i < 17; i++)
        {
           auto  a = getHeight()/16;
            auto b = getWidth()/16;
            g.drawLine(0, a*i, getWidth(), a*i);
            g.drawLine(b*i,0,b*i,getHeight());
        }
        // draw the mid line
        g.setColour(Colour(0x50808080));
        g.drawLine(0, getHeight()/2, getWidth(), getHeight()/2, 3);
        g.setColour (Colours::darkorange); //color of envelope
        Path myPath;    // create the path
        if(boolModeHold)
        {
               myPath.startNewSubPath (-8, getHeight()+8);
            myPath.lineTo(0, arraySegment[0]->getPositionY()+8);
        }
        else
        {
            myPath.startNewSubPath (-4, getHeight() + 4);         // move the first point
            myPath.lineTo(0, getHeight() - 8);
        }
         
        for (int i = 0; i < arraySegment.size();i++) //create each segment
        {
            //int w = ((arraySegment[i]->getWidth()/100)*arraySegment[i]->getRate())+arraySegment[i]->getX() + 8;
            int w = arraySegment[i]->getPositionX() + 8 + arraySegment[i]->getX();
            int y = arraySegment[i]->getPositionY() + 8 + arraySegment[i]->getY();
//            int y = (arraySegment[i]->getHeight()/100)*(100 - arraySegment[i]->getLevel());
            myPath.lineTo (w, y);
            
        }
        int wgrid =(arraySegment[arraySegment.size()-1]->getPositionX() + arraySegment[arraySegment.size()-1]->getX()) + 8;
      
        
        myPath.lineTo (wgrid + ((getWidth()/(arraySegment.size()+1)*intRelease)/100), getHeight()+4 ); // end close point
      //  Logger::writeToLog( " intRelease" + String(intRelease));
        myPath.closeSubPath();
        g.strokePath (myPath, PathStrokeType (5.0f));
        
       // g.setColour(Colours::white);
            ColourGradient myGradient {	Colour(0xe0ff8000),static_cast<float>(getWidth()/2),0,Colour(0x30804000),static_cast<float>(getWidth()/2),static_cast<float>(getHeight()),false };
        g.setGradientFill(myGradient);
//        g.setColour(Colour(0x10000000));
        g.fillPath(myPath);
        g.setColour(Colours::white);
        g.drawText (strName, getLocalBounds(), Justification::topRight, true);
 
        String str = "Note Release : " + String(intRelease);
        g.setColour(Colour(0x80ffffff));
     
        // g.drawText (str, getLocalBounds(), Justification::centredLeft, true);
        g.drawFittedText( str, getWidth() - 100, 20, 90, 24, Justification::right,1);
        
        
        sendChangeMessage();
        
    }
    void setR4 (bool enable, int rate)
    {
        
    }
    void setL1 (int level)
    {
        arraySegment[0]->setLevel(level);
    }
    int getL1 ()
    {
        return arraySegment[0]->getLevel();
    }
    void setRelease(int release)
    {
    //    Logger::writeToLog("setRelease " + String(release));
        intRelease = release;
        repaint();
    }
    int getRelease ()
    {
        return intRelease;
    }
    void setName(String name)
    {
        strName = name;
    }
    String getName()
    {
        return strName;
    }
    void setEasyDraw (bool easyDraw)
    {
        boolEasyDraw = easyDraw;
        resized();
    }
    void setModeHold(bool trueOrFalse)
    {
        boolModeHold = trueOrFalse;
        repaint();
    }
    void addSegment(int level, int rate, String name,int rateRange, int sysexRate[0],int levelRange, int sysexLevel[0])
    {
        auto* segment = arraySegment.add (new Segment);
        
        addAndMakeVisible(segment);
        segment->addChangeListener(this);
        segment->setMidiSysexRate(sysexRate);
        segment->setMidiSysexLevel(sysexLevel);
        segment->setValue(rate, level);
        segment->setRange (rateRange, levelRange); // 100 % = Range
        segment->setName(name);
        
  //      arrayHook[arrayHook.size()]->setTopRightPosition(getWidth(), getHeight());
/*
        int wgrid = getWidth()/arraySegment.size();
        for(int i = 0; i < arraySegment.size();i++)
        {
     
        arraySegment[i]->setBounds(getWidth()-wgrid, 0, wgrid, getHeight());
        }
*/        
    }
    void resized() override
    {
        int w = 0;
       // int height = getHeight() - arraySegment[0]->getPositionY();
        
        for (int i = 0; i < arraySegment.size();i++)
        {
   
            if(i > 0)
            {
              //  w = (arraySegment[i-1]->getWidth()/100) * 100; //arraySegment[i-1]->getRate();
           // w = (getWidth()/arraySegment.size()/100)*arraySegment[i-1]->getRate();
           //     w = w + 16;
                w = w + arraySegment[i-1]->getPositionX()+16;
                if(boolEasyDraw)
                {
                arraySegment[i]->setBounds(w,0,getWidth()/(arraySegment.size()+1),getHeight());
                }
                else
                {
                arraySegment[i]->setBounds(w,arraySegment[0]->getPositionY(),getWidth()/(arraySegment.size()+1),getHeight() -arraySegment[0]->getPositionY() );
                }
            }
            else
            {
            // first segment resized
            arraySegment[i]->setBounds(w,0,getWidth()/(arraySegment.size()+1),getHeight());
            }
            //Logger::writeToLog("Resize W " + String(w));
     //       Logger::writeToLog("Resize rate " + String(arraySegment[i-1]->getRate()));
 
        }
       


      
        // This is called when the MainComponent is resized.
        // If you add any child components, this is where you should
        // update their positions.
    }


private:
    //==============================================================================
    // Your private member variables go here...
    String strName {"Name "};
    bool boolModeHold = false;
    bool boolR4Enable = false;
    bool boolEasyDraw = false;
    int intR4Rate;
        int intRelease = 0;
  
    OwnedArray<Segment> arraySegment;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SADSR)
};

