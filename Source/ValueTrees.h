/*
  ==============================================================================

    ValueTrees.h
    Created: 13 Dec 2018 11:22:09am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
// ValueTree for the Voice
//==============================================================================



bool keyPressed (const KeyPress& key,
                 Component* originatingComponent) override
{
    if(key.getKeyCode() == 90)
    {
        if(key.getModifiers() == ModifierKeys::commandModifier)

            undoManager.undo();
        
        if(key.getModifiers() == ModifierKeys::shiftModifier + ModifierKeys::commandModifier)
            undoManager.redo();
     //   repaint();
    }
    
    return 0;
}




void valueTreePropertyChanged (ValueTree& tree, const Identifier& treeId) override
{
    Logger::writeToLog("Value Tree property Changed");

    repaint();
}
void valueTreeChildAdded (ValueTree& parentTree,
                          ValueTree& childWhichHasBeenAdded) override
{
}


void valueTreeChildRemoved (ValueTree& parentTree,
                            ValueTree& childWhichHasBeenRemoved,
                            int indexFromWhichChildWasRemoved) override
{
    
}

virtual void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved,
                                         int oldIndex, int newIndex) override
{
    
}

void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged) override
{
    
}
void timerCallback() override
{
    undoManager.beginNewTransaction();
}
/*
void initValuesToValueTree()
{
 //   = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT, &undoManager);
    valueFilter1FreqElement1= valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1FQ1 , &undoManager);
    valueFilter1FreqElement2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2FQ1, &undoManager);
    valueFilter1FreqElement3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3FQ1, &undoManager);
    valueFilter1FreqElement4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4FQ1, &undoManager);
    
    valueFilter2FreqElement1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1FQ1, &undoManager);
    valueFilter2FreqElement2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2FQ1, &undoManager);
     valueFilter2FreqElement3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3FQ1, &undoManager);
     valueFilter2FreqElement4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4FQ1, &undoManager);
    
     valueFilterResonnanceElement1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1RESONNANCEFILTRE, &undoManager);
     valueFilterResonnanceElement2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2RESONNANCEFILTRE, &undoManager);
     valueFilterResonnanceElement3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3RESONNANCEFILTRE, &undoManager);
     valueFilterResonnanceElement4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4RESONNANCEFILTRE, &undoManager);
    
     valueFilterLfoElement1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1LFOFILTRE, &undoManager);
     valueFilterLfoElement2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2LFOFILTRE, &undoManager);
     valueFilterLfoElement3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3LFOFILTRE, &undoManager);
     valueFilterLfoElement4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4LFOFILTRE, &undoManager);
    
     valueFilterVelocityElement1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1VELOCITYFILTRE, &undoManager);
     valueFilterVelocityElement2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2VELOCITYFILTRE, &undoManager);
     valueFilterVelocityElement3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3VELOCITYFILTRE, &undoManager);
     valueFilterVelocityElement4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4VELOCITYFILTRE, &undoManager);
    
     valueFilterSlopeElement1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1SLOPEFILTRE, &undoManager);
     valueFilterSlopeElement2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2SLOPEFILTRE, &undoManager);
     valueFilterSlopeElement3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3SLOPEFILTRE, &undoManager);
     valueFilterSlopeElement4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4SLOPEFILTRE, &undoManager);
    
     valueFilter1ModeElement1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1MODEFILTRE1, &undoManager);
     valueFilter1ModeElement2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2MODEFILTRE1, &undoManager);
     valueFilter1ModeElement3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3MODEFILTRE1, &undoManager);
     valueFilter1ModeElement4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4MODEFILTRE1, &undoManager);
    
     valueFilter2ModeElement1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1MODEFILTRE2, &undoManager);
     valueFilter2ModeElement2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2MODEFILTRE2, &undoManager);
     valueFilter2ModeElement3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3MODEFILTRE2, &undoManager);
     valueFilter2ModeElement4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4MODEFILTRE2, &undoManager);
    
     valueFilter1ControlElement1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1CONTROLEURFILTRE1, &undoManager);
     valueFilter1ControlElement2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2CONTROLEURFILTRE1, &undoManager);
     valueFilter1ControlElement3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3CONTROLEURFILTRE1, &undoManager);
     valueFilter1ControlElement4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4CONTROLEURFILTRE1, &undoManager);
    
     valueFilter2ControlElement1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1CONTROLEURFILTRE2, &undoManager);
     valueFilter2ControlElement2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2CONTROLEURFILTRE2, &undoManager);
     valueFilter2ControlElement3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3CONTROLEURFILTRE2, &undoManager);
     valueFilter2ControlElement4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4CONTROLEURFILTRE2, &undoManager);
    
     valueFilterLevel0Element1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRELEVEL0, &undoManager);
     valueFilterLevel0Element2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRELEVEL0, &undoManager);
     valueFilterLevel0Element3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRELEVEL0, &undoManager);
     valueFilterLevel0Element4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRELEVEL0, &undoManager);
    
     valueFilterLevel1Element1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRELEVEL1, &undoManager);
     valueFilterLevel1Element2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRELEVEL1, &undoManager);
     valueFilterLevel1Element3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRELEVEL1, &undoManager);
     valueFilterLevel1Element4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRELEVEL1, &undoManager);
    
     valueFilterLevel2Element1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRELEVEL2, &undoManager);
     valueFilterLevel2Element2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRELEVEL2, &undoManager);
     valueFilterLevel2Element3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRELEVEL2, &undoManager);
     valueFilterLevel2Element4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRELEVEL2, &undoManager);
    
     valueFilterLevel3Element1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRELEVEL3, &undoManager);
     valueFilterLevel3Element2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRELEVEL3, &undoManager);
     valueFilterLevel3Element3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRELEVEL3, &undoManager);
     valueFilterLevel3Element4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRELEVEL3, &undoManager);
    
     valueFilterLevel4Element1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRELEVEL4, &undoManager);
     valueFilterLevel4Element2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRELEVEL4, &undoManager);
     valueFilterLevel4Element3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRELEVEL4, &undoManager);
     valueFilterLevel4Element4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRELEVEL4, &undoManager);
    
     valueFilterRate0Element1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT, &undoManager);
     valueFilterRate0Element2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT, &undoManager);
     valueFilterRate0Element3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT, &undoManager);
     valueFilterRate0Element4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT, &undoManager);
    
     valueFilterRate1Element1 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT, &undoManager);
     valueFilterRate1Element2 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT, &undoManager);
     valueFilterRate1Element3 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT, &undoManager);
     valueFilterRate1Element4 = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT, &undoManager);
 
}
*/
//==============================================================================
