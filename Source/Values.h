/*
  ==============================================================================

    Values.h
    Created: 18 Dec 2018 6:38:29pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
namespace IDs
{
#define DECLARE_ID(name) const juce::Identifier name (#name);
    
    DECLARE_ID (VOICEMODE)
    DECLARE_ID (COMMONVOLUME)
    DECLARE_ID (COMMONFOOT)
    DECLARE_ID (COMMONMOD)
    
    DECLARE_ID (ELEMENT1WAVEFORM)
    DECLARE_ID (ELEMENT1PITCH)
    DECLARE_ID (ELEMENT1FINE)
    DECLARE_ID (ELEMENT1FIXED)
    DECLARE_ID (ELEMENT1VOLUME)
    DECLARE_ID (ELEMENT1FQ1)
    DECLARE_ID (ELEMENT1FQ2)
    DECLARE_ID (ELEMENT1MODEFILTRE1)
    DECLARE_ID (ELEMENT1MODEFILTRE2)
    DECLARE_ID (ELEMENT1RESONNANCEFILTRE)
    DECLARE_ID (ELEMENT1VELOCITYFILTRE)
    DECLARE_ID (ELEMENT1LFOFILTRE)
    DECLARE_ID (ELEMENT1SLOPEFILTRE)
        DECLARE_ID (ELEMENT1SLOPEFILTRE2)
    DECLARE_ID (ELEMENT1CONTROLEURFILTRE1)
    DECLARE_ID (ELEMENT1CONTROLEURFILTRE2)
    DECLARE_ID (ELEMENT1EGFILTRELEVEL0)
    DECLARE_ID (ELEMENT1EGFILTRELEVEL1)
    DECLARE_ID (ELEMENT1EGFILTRELEVEL2)
    DECLARE_ID (ELEMENT1EGFILTRELEVEL3)
    DECLARE_ID (ELEMENT1EGFILTRELEVEL4)
    DECLARE_ID (ELEMENT1EGFILTRERL1)
    DECLARE_ID (ELEMENT1EGFILTRERL2)
    DECLARE_ID (ELEMENT1EGFILTRER1)
    DECLARE_ID (ELEMENT1EGFILTRER2)
    DECLARE_ID (ELEMENT1EGFILTRER3)
    DECLARE_ID (ELEMENT1EGFILTRER4)
    DECLARE_ID (ELEMENT1EGFILTRERR1)
    DECLARE_ID (ELEMENT1EGFILTRERR2)
    
    DECLARE_ID (ELEMENT1EGFILTRE2LEVEL0)
    DECLARE_ID (ELEMENT1EGFILTRE2LEVEL1)
    DECLARE_ID (ELEMENT1EGFILTRE2LEVEL2)
    DECLARE_ID (ELEMENT1EGFILTRE2LEVEL3)
    DECLARE_ID (ELEMENT1EGFILTRE2LEVEL4)
    DECLARE_ID (ELEMENT1EGFILTRE2RL1)
    DECLARE_ID (ELEMENT1EGFILTRE2RL2)
    DECLARE_ID (ELEMENT1EGFILTRE2R1)
    DECLARE_ID (ELEMENT1EGFILTRE2R2)
    DECLARE_ID (ELEMENT1EGFILTRE2R3)
    DECLARE_ID (ELEMENT1EGFILTRE2R4)
    DECLARE_ID (ELEMENT1EGFILTRE2RR1)
    DECLARE_ID (ELEMENT1EGFILTRE2RR2)
    
    DECLARE_ID (ELEMENT2WAVEFORM)
    DECLARE_ID (ELEMENT2PITCH)
    DECLARE_ID (ELEMENT2FINE)
    DECLARE_ID (ELEMENT2FIXED)
    DECLARE_ID (ELEMENT2VOLUME)
    DECLARE_ID (ELEMENT2FQ1)
    DECLARE_ID (ELEMENT2FQ2)
    DECLARE_ID (ELEMENT2MODEFILTRE1)
    DECLARE_ID (ELEMENT2MODEFILTRE2)
    DECLARE_ID (ELEMENT2RESONNANCEFILTRE)
    DECLARE_ID (ELEMENT2VELOCITYFILTRE)
    DECLARE_ID (ELEMENT2LFOFILTRE)
    DECLARE_ID (ELEMENT2SLOPEFILTRE)
        DECLARE_ID (ELEMENT2SLOPEFILTRE2)
    DECLARE_ID (ELEMENT2CONTROLEURFILTRE1)
    DECLARE_ID (ELEMENT2CONTROLEURFILTRE2)
    DECLARE_ID (ELEMENT2EGFILTRELEVEL0)
    DECLARE_ID (ELEMENT2EGFILTRELEVEL1)
    DECLARE_ID (ELEMENT2EGFILTRELEVEL2)
    DECLARE_ID (ELEMENT2EGFILTRELEVEL3)
    DECLARE_ID (ELEMENT2EGFILTRELEVEL4)
    DECLARE_ID (ELEMENT2EGFILTRERL1)
    DECLARE_ID (ELEMENT2EGFILTRERL2)
    DECLARE_ID (ELEMENT2EGFILTRER1)
    DECLARE_ID (ELEMENT2EGFILTRER2)
    DECLARE_ID (ELEMENT2EGFILTRER3)
    DECLARE_ID (ELEMENT2EGFILTRER4)
    DECLARE_ID (ELEMENT2EGFILTRERR1)
    DECLARE_ID (ELEMENT2EGFILTRERR2)
    
    DECLARE_ID (ELEMENT2EGFILTRE2LEVEL0)
    DECLARE_ID (ELEMENT2EGFILTRE2LEVEL1)
    DECLARE_ID (ELEMENT2EGFILTRE2LEVEL2)
    DECLARE_ID (ELEMENT2EGFILTRE2LEVEL3)
    DECLARE_ID (ELEMENT2EGFILTRE2LEVEL4)
    DECLARE_ID (ELEMENT2EGFILTRE2RL1)
    DECLARE_ID (ELEMENT2EGFILTRE2RL2)
    DECLARE_ID (ELEMENT2EGFILTRE2R1)
    DECLARE_ID (ELEMENT2EGFILTRE2R2)
    DECLARE_ID (ELEMENT2EGFILTRE2R3)
    DECLARE_ID (ELEMENT2EGFILTRE2R4)
    DECLARE_ID (ELEMENT2EGFILTRE2RR1)
    DECLARE_ID (ELEMENT2EGFILTRE2RR2)
    
    DECLARE_ID (ELEMENT3WAVEFORM)
    DECLARE_ID (ELEMENT3PITCH)
    DECLARE_ID (ELEMENT3FINE)
    DECLARE_ID (ELEMENT3FIXED)
    DECLARE_ID (ELEMENT3VOLUME)
    DECLARE_ID (ELEMENT3FQ1)
    DECLARE_ID (ELEMENT3FQ2)
    DECLARE_ID (ELEMENT3MODEFILTRE1)
    DECLARE_ID (ELEMENT3MODEFILTRE2)
    DECLARE_ID (ELEMENT3RESONNANCEFILTRE)
    DECLARE_ID (ELEMENT3VELOCITYFILTRE)
    DECLARE_ID (ELEMENT3LFOFILTRE)
    DECLARE_ID (ELEMENT3SLOPEFILTRE)
        DECLARE_ID (ELEMENT3SLOPEFILTRE2)
    DECLARE_ID (ELEMENT3CONTROLEURFILTRE1)
    DECLARE_ID (ELEMENT3CONTROLEURFILTRE2)
    DECLARE_ID (ELEMENT3EGFILTRELEVEL0)
    DECLARE_ID (ELEMENT3EGFILTRELEVEL1)
    DECLARE_ID (ELEMENT3EGFILTRELEVEL2)
    DECLARE_ID (ELEMENT3EGFILTRELEVEL3)
    DECLARE_ID (ELEMENT3EGFILTRELEVEL4)
    DECLARE_ID (ELEMENT3EGFILTRERL1)
    DECLARE_ID (ELEMENT3EGFILTRERL2)
    DECLARE_ID (ELEMENT3EGFILTRER1)
    DECLARE_ID (ELEMENT3EGFILTRER2)
    DECLARE_ID (ELEMENT3EGFILTRER3)
    DECLARE_ID (ELEMENT3EGFILTRER4)
    DECLARE_ID (ELEMENT3EGFILTRERR1)
    DECLARE_ID (ELEMENT3EGFILTRERR2)
    
    DECLARE_ID (ELEMENT3EGFILTRE2LEVEL0)
    DECLARE_ID (ELEMENT3EGFILTRE2LEVEL1)
    DECLARE_ID (ELEMENT3EGFILTRE2LEVEL2)
    DECLARE_ID (ELEMENT3EGFILTRE2LEVEL3)
    DECLARE_ID (ELEMENT3EGFILTRE2LEVEL4)
    DECLARE_ID (ELEMENT3EGFILTRE2RL1)
    DECLARE_ID (ELEMENT3EGFILTRE2RL2)
    DECLARE_ID (ELEMENT3EGFILTRE2R1)
    DECLARE_ID (ELEMENT3EGFILTRE2R2)
    DECLARE_ID (ELEMENT3EGFILTRE2R3)
    DECLARE_ID (ELEMENT3EGFILTRE2R4)
    DECLARE_ID (ELEMENT3EGFILTRE2RR1)
    DECLARE_ID (ELEMENT3EGFILTRE2RR2)
    
    DECLARE_ID (ELEMENT4WAVEFORM)
    DECLARE_ID (ELEMENT4PITCH)
    DECLARE_ID (ELEMENT4FINE)
    DECLARE_ID (ELEMENT4FIXED)
    DECLARE_ID (ELEMENT4VOLUME)
    DECLARE_ID (ELEMENT4FQ1)
    DECLARE_ID (ELEMENT4FQ2)
    DECLARE_ID (ELEMENT4MODEFILTRE1)
    DECLARE_ID (ELEMENT4MODEFILTRE2)
    DECLARE_ID (ELEMENT4RESONNANCEFILTRE)
    DECLARE_ID (ELEMENT4VELOCITYFILTRE)
    DECLARE_ID (ELEMENT4LFOFILTRE)
    DECLARE_ID (ELEMENT4SLOPEFILTRE)
        DECLARE_ID (ELEMENT4SLOPEFILTRE2)
    DECLARE_ID (ELEMENT4CONTROLEURFILTRE1)
    DECLARE_ID (ELEMENT4CONTROLEURFILTRE2)
    DECLARE_ID (ELEMENT4EGFILTRELEVEL0)
    DECLARE_ID (ELEMENT4EGFILTRELEVEL1)
    DECLARE_ID (ELEMENT4EGFILTRELEVEL2)
    DECLARE_ID (ELEMENT4EGFILTRELEVEL3)
    DECLARE_ID (ELEMENT4EGFILTRELEVEL4)
    DECLARE_ID (ELEMENT4EGFILTRERL1)
    DECLARE_ID (ELEMENT4EGFILTRERL2)
    DECLARE_ID (ELEMENT4EGFILTRER1)
    DECLARE_ID (ELEMENT4EGFILTRER2)
    DECLARE_ID (ELEMENT4EGFILTRER3)
    DECLARE_ID (ELEMENT4EGFILTRER4)
    DECLARE_ID (ELEMENT4EGFILTRERR1)
    DECLARE_ID (ELEMENT4EGFILTRERR2)
    
    DECLARE_ID (ELEMENT4EGFILTRE2LEVEL0)
    DECLARE_ID (ELEMENT4EGFILTRE2LEVEL1)
    DECLARE_ID (ELEMENT4EGFILTRE2LEVEL2)
    DECLARE_ID (ELEMENT4EGFILTRE2LEVEL3)
    DECLARE_ID (ELEMENT4EGFILTRE2LEVEL4)
    DECLARE_ID (ELEMENT4EGFILTRE2RL1)
    DECLARE_ID (ELEMENT4EGFILTRE2RL2)
    DECLARE_ID (ELEMENT4EGFILTRE2R1)
    DECLARE_ID (ELEMENT4EGFILTRE2R2)
    DECLARE_ID (ELEMENT4EGFILTRE2R3)
    DECLARE_ID (ELEMENT4EGFILTRE2R4)
    DECLARE_ID (ELEMENT4EGFILTRE2RR1)
    DECLARE_ID (ELEMENT4EGFILTRE2RR2)
    
    
    
#undef DECLARE_ID
}
/*
Value valueVoiceMode; // mode 1AFM, 2AFM, ...
//Filter Values
Value valueFilter1FreqElement1;
Value valueFilter1FreqElement2;
Value valueFilter1FreqElement3;
Value valueFilter1FreqElement4;

Value valueFilter2FreqElement1;
Value valueFilter2FreqElement2;
Value valueFilter2FreqElement3;
Value valueFilter2FreqElement4;

Value valueFilterResonnanceElement1;
Value valueFilterResonnanceElement2;
Value valueFilterResonnanceElement3;
Value valueFilterResonnanceElement4;

Value valueFilterLfoElement1;
Value valueFilterLfoElement2;
Value valueFilterLfoElement3;
Value valueFilterLfoElement4;

Value valueFilterVelocityElement1;
Value valueFilterVelocityElement2;
Value valueFilterVelocityElement3;
Value valueFilterVelocityElement4;

Value valueFilterSlopeElement1;
Value valueFilterSlopeElement2;
Value valueFilterSlopeElement3;
Value valueFilterSlopeElement4;

Value valueFilter1ModeElement1;
Value valueFilter1ModeElement2;
Value valueFilter1ModeElement3;
Value valueFilter1ModeElement4;

Value valueFilter2ModeElement1;
Value valueFilter2ModeElement2;
Value valueFilter2ModeElement3;
Value valueFilter2ModeElement4;

Value valueFilter1ControlElement1;
Value valueFilter1ControlElement2;
Value valueFilter1ControlElement3;
Value valueFilter1ControlElement4;

Value valueFilter2ControlElement1;
Value valueFilter2ControlElement2;
Value valueFilter2ControlElement3;
Value valueFilter2ControlElement4;

Value valueFilterLevel0Element1;
Value valueFilterLevel0Element2;
Value valueFilterLevel0Element3;
Value valueFilterLevel0Element4;

Value valueFilterLevel1Element1;
Value valueFilterLevel1Element2;
Value valueFilterLevel1Element3;
Value valueFilterLevel1Element4;

Value valueFilterLevel2Element1;
Value valueFilterLevel2Element2;
Value valueFilterLevel2Element3;
Value valueFilterLevel2Element4;

Value valueFilterLevel3Element1;
Value valueFilterLevel3Element2;
Value valueFilterLevel3Element3;
Value valueFilterLevel3Element4;

Value valueFilterLevel4Element1;
Value valueFilterLevel4Element2;
Value valueFilterLevel4Element3;
Value valueFilterLevel4Element4;

Value valueFilterRate0Element1;
Value valueFilterRate0Element2;
Value valueFilterRate0Element3;
Value valueFilterRate0Element4;

Value valueFilterRate1Element1;
Value valueFilterRate1Element2;
Value valueFilterRate1Element3;
Value valueFilterRate1Element4;
*/
ValueTree   valueTreeVoice ("Sysex77");
