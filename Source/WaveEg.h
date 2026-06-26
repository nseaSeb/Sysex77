/*
  ==============================================================================

    WaveEg.h
    Created: 10 Feb 2019 11:27:36am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "SysexUtils.h"
#include "EnvelopeDraw.h"

class WaveEg    : public ElementComponent, public TextButton::Listener, Slider::Listener
{
public:
    WaveEg()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        
        addAndMakeVisible(keyDraw);
        addAndMakeVisible(samplePathLeft);
        addAndMakeVisible(samplePathRight);
        addAndMakeVisible(labelSlope);
        labelSlope.setJustificationType(Justification::centred);
        labelSlope.attachToComponent(&sliderSlope, true);
        
        sliderSlope.setRangeAndRound(-7,7,0);
        sliderSlope.setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(sliderSlope);
        sliderSlope.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
        sliderSlope.addListener(this);
        for (auto* s : { &sliderL0, &sliderL1, &sliderL2, &sliderL3, &sliderL4,
                         &sliderR1, &sliderR2, &sliderR3, &sliderR4,
                         &sliderRL1, &sliderRL2, &sliderRR1, &sliderRR2 })
            s->addListener (this);

        addAndMakeVisible(sliderL0);
        addAndMakeVisible(sliderL1);
        addAndMakeVisible(sliderL2);
        addAndMakeVisible(sliderL3);
        addAndMakeVisible(sliderL4);
        addAndMakeVisible(sliderR0);
        addAndMakeVisible(sliderR1);
        addAndMakeVisible(sliderR2);
        addAndMakeVisible(sliderR3);
        addAndMakeVisible(sliderR4);
        addAndMakeVisible(sliderRL1);
        addAndMakeVisible(sliderRL2);
        addAndMakeVisible(sliderRR1);
        addAndMakeVisible(sliderRR2);
        
        addAndMakeVisible(labelL0);
        addAndMakeVisible(labelL1);
        addAndMakeVisible(labelL2);
        addAndMakeVisible(labelL3);
        addAndMakeVisible(labelL4);
        addAndMakeVisible(labelRL1);
        addAndMakeVisible(labelRL2);
        addAndMakeVisible(labelR1);
        addAndMakeVisible(labelR2);
        addAndMakeVisible(labelR3);
        addAndMakeVisible(labelR4);
        addAndMakeVisible(labelRR1);
        addAndMakeVisible(labelRR2);
        labelL0.attachToComponent(&sliderL0, false);
        labelL1.attachToComponent(&sliderL1, false);
        labelL2.attachToComponent(&sliderL2, false);
        labelL3.attachToComponent(&sliderL3, false);
        labelL4.attachToComponent(&sliderL4, false);
        labelRL1.attachToComponent(&sliderRL1, false);
        labelRL2.attachToComponent(&sliderRL2, false);
        labelR1.attachToComponent(&sliderR1, false);
        labelR2.attachToComponent(&sliderR2, false);
        labelR3.attachToComponent(&sliderR3, false);
        labelR4.attachToComponent(&sliderR4, false);
        labelRR1.attachToComponent(&sliderRR1, false);
        labelRR2.attachToComponent(&sliderRR2, false);

        // Édition à la souris sur le grand graphe d'EG (gauche) : glisser un nœud =
        // niveau (Y, 0..64) + rate du segment entrant (X). Les sliders (droite) restent.
        addAndMakeVisible (egGraph);
        egGraph.maxLevel = 64.0f;
        egGraph.getData = [this] (juce::Array<float>& levels, juce::Array<float>& weights)
        {
            levels = { (float) sliderL0.getValue(), (float) sliderL1.getValue(), (float) sliderL2.getValue(),
                       (float) sliderL3.getValue(), (float) sliderL4.getValue(),
                       (float) sliderRL1.getValue(), (float) sliderRL2.getValue() };
            auto rw = [] (double r) { return (float) (66.0 - r); };   // même convention que paint()
            weights = { rw (sliderR1.getValue()), rw (sliderR2.getValue()), rw (sliderR3.getValue()),
                        rw (sliderR4.getValue()), rw (sliderRR1.getValue()), rw (sliderRR2.getValue()) };
        };
        egGraph.onEditNode = [this] (int node, float levelF, float segWeightF)
        {
            MidiSlider* L[7] = { &sliderL0, &sliderL1, &sliderL2, &sliderL3, &sliderL4, &sliderRL1, &sliderRL2 };
            MidiSlider* R[6] = { &sliderR1, &sliderR2, &sliderR3, &sliderR4, &sliderRR1, &sliderRR2 };
            if (node < 0 || node > 6) return;
            L[node]->setValue (jlimit (0, 64, roundToInt (levelF)));
            if (node >= 1 && segWeightF >= 0.0f)                       // rate = 66 - poids dessiné
                R[node - 1]->setValue (jlimit (0, 64, roundToInt (66.0f - segWeightF)));
        };
    }
    
    ~WaveEg()
    {
        sliderSlope.removeListener(this);
    }
    void 	sliderValueChanged (Slider *slider) override
    {
        Logger::writeToLog("Filter::Slider value changed");
        
        if(slider == &sliderSlope)
            resized();
        repaint();
    }
    // En contexte AWM (vue Wave), l'EG de volume cible l'amp-EG AWM (group 0x07, Table 1-8)
    // au lieu de diffuser aux opérateurs AFM. Activé par WaveVue. À appeler AVANT setElementNumber.
    void setAwmMode (bool b) { isAwm = b; }
    bool isAwm = false;

    void setElementNumber ( int element, UndoManager& um) override
    {
        Logger::writeToLog( "WaveEg setElement");
        // EG d'ampli AFM = EG PAR OPÉRATEUR ; le groupe [3] sera diffusé aux 6 opérateurs
        // (mode « All », cf. SY77 [F2] All). T2 [4] = (element-1)<<5 (réglé par bloc ci-dessous).
        // Group byte centralisé (cf. SyVoice::egGroupFor) : AFM amp-EG = group du 1er opérateur
        // (op 0 = OP6 -> 0x06) ; le mode « All » réécrit ensuite le group pour chaque opérateur.
        const int afmAmpGroup = (int) SyVoice::egGroupFor (SyVoice::EgKind::afmAmplitude, 0);
        int sysexdata2[9] = { 0x43, 0X10, 0x34, afmAmpGroup, 0x00, 0x00, 0x00, 0x00, 0x00 };
        int sysexdata[9]  = { 0x43, 0X10, 0x34, afmAmpGroup, 0x00, 0x00, 0x00, 0x00, 0x00 };
        
        if(element == 1)
        {
            
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1SLOPEVOL, &um));
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLLEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLLEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLLEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLLEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLLEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLRL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLRL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLR1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLR2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLR3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLR4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLRR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGVOLRR2 , &um));
            
        }
        else if (element ==2)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2SLOPEVOL, &um));
            sysexdata[4] = 0x20;
            sysexdata2[4] = 0x20;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLLEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLLEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLLEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLLEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLLEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLRL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLRL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLR1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLR2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLR3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLR4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLRR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGVOLRR2 , &um));
            
        }
        else if (element == 3)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3SLOPEVOL, &um));
            sysexdata[4] = 0x40;
            sysexdata2[4] = 0x40;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLLEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLLEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLLEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLLEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLLEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLRL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLRL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLR1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLR2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLR3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLR4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLRR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGVOLRR2 , &um));
            
        }
        else if (element == 4)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4SLOPEVOL, &um));
            sysexdata[4] = 0x60;
            sysexdata2[4] = 0x60;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLLEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLLEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLLEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLLEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLLEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLRL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLRL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLR1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLR2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLR3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLR4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLRR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGVOLRR2 , &um));
            
        }
        // --- EG d'ampli AWM (Table 1-8, group 0x07) : un seul groupe (pas de diffusion).
        // Correspondance sliders -> N2 : PAR1-4@50-53, PARR1@54, PAL2@55, PAL3@56, PARS@57.
        // addrHi = (élément-1)<<5 (déjà dans sysexdata[4]). Les sliders sans équivalent AWM
        // (RR2/L0/L1/L4/RL1/RL2) gardent leur referTo (render) mais n'émettent rien.
        if (isAwm)
        {
            int awm[9] = { 0x43, 0X10, 0x34, (int) SyVoice::egGroupFor (SyVoice::EgKind::awmAmplitude),
                           sysexdata[4], 0x00, 0x00, 0x00, 0x00 };
            auto wireAwm = [&] (MidiSlider& s, int n2, int maxVal)
            {
                awm[6] = n2;
                s.setMidiSysex (awm);
                s.setMidiBroadcastGroups ({});
                s.setRangeAndRound (0, maxVal, 0);
            };
            wireAwm (sliderR1, 0x50, 63); wireAwm (sliderR2, 0x51, 63);
            wireAwm (sliderR3, 0x52, 63); wireAwm (sliderR4, 0x53, 63);
            wireAwm (sliderRR1, 0x54, 63);
            wireAwm (sliderL2, 0x55, 63); wireAwm (sliderL3, 0x56, 63);
            wireAwm (sliderSlope, 0x57, 64);   // PARS (s/m, encodage à affiner)
            for (MidiSlider* s : { &sliderRR2, &sliderL0, &sliderL1, &sliderL4, &sliderRL1, &sliderRL2 })
                s->setMidiBroadcastGroups ({});
            return;
        }

        // --- EG d'ampli AFM = EG par OPÉRATEUR (Table 1-7). Mode « All » : chaque slider est
        // diffusé aux 6 opérateurs (groupes 0x06/0x16/0x26/0x36/0x46/0x56). Offsets opérateur :
        // R1-4=00-03, RR1=04, RR2=05, L1-4=06-09, RL1=0A, RL2=0B, L0=0E, RS(slope)=0F.
        Array<int> ops;
        for (int op = 0; op < 6; ++op)
            ops.add ((int) SyVoice::egGroupFor (SyVoice::EgKind::afmAmplitude, op));

        auto wire = [&] (MidiSlider& s, int n2, int maxVal)
        {
            sysexdata[6] = n2;
            s.setMidiSysex (sysexdata);
            s.setMidiBroadcastGroups (ops);
            s.setRangeAndRound (0, maxVal, 0);
        };

        wire (sliderR1,  0x00, 63); wire (sliderR2,  0x01, 63); wire (sliderR3, 0x02, 63); wire (sliderR4, 0x03, 63);
        wire (sliderRR1, 0x04, 63); wire (sliderRR2, 0x05, 63);
        wire (sliderL1,  0x06, 63); wire (sliderL2,  0x07, 63); wire (sliderL3, 0x08, 63); wire (sliderL4, 0x09, 63);
        wire (sliderRL1, 0x0A, 63); wire (sliderRL2, 0x0B, 63);
        wire (sliderL0,  0x0E, 63);
        wire (sliderSlope, 0x0F, 64);   // RS rate scaling (-7..+7 s/m exact à affiner, cf. encodage EG)
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        auto bounds = getLocalBounds().toFloat();
        juce::Rectangle<float> egArea (bounds.getWidth() * 0.03f, bounds.getHeight() * 0.05f,
                                       bounds.getWidth() * 0.58f, bounds.getHeight() * 0.74f);

        juce::Array<float> egLevels { (float) sliderL0.getValue(), (float) sliderL1.getValue(),
                                      (float) sliderL2.getValue(), (float) sliderL3.getValue(),
                                      (float) sliderL4.getValue(), (float) sliderRL1.getValue(),
                                      (float) sliderRL2.getValue() };

        auto rateWeight = [] (double rate) { return (float) (66.0 - rate); };
        juce::Array<float> egWeights { rateWeight (sliderR1.getValue()), rateWeight (sliderR2.getValue()),
                                       rateWeight (sliderR3.getValue()), rateWeight (sliderR4.getValue()),
                                       rateWeight (sliderRR1.getValue()), rateWeight (sliderRR2.getValue()) };

        SyDraw::drawEnvelope (g, egArea, egLevels, egWeights, 64.0f, SYColSelected, "Wave EG");
        
    }
    void buttonClicked (Button* button) override
    {
        setVisible(false);
    }
    
    void resized() override
    {
        // Overlay d'édition = même rectangle que l'EG dessiné dans paint() (moitié gauche).
        auto b = getLocalBounds().toFloat();
        egGraph.setBounds (juce::Rectangle<float> (b.getWidth() * 0.03f, b.getHeight() * 0.05f,
                                                   b.getWidth() * 0.58f, b.getHeight() * 0.74f).toNearestInt());

        keyDraw.setBoundsRelative(0.0f, 0.85f, 1.0f, 0.09f);
        float sampleWidth = 0.2f + (sliderSlope.getValue()/100);
        samplePathLeft.setBoundsRelative(0.0f, 0.93f, sampleWidth, 0.09f);
        sampleWidth = 0.2f - (sliderSlope.getValue()/100);
        samplePathRight.setBoundsRelative(1.0f - sampleWidth, 0.93f, sampleWidth, 0.09f);
        
        const int W = getWidth(), H = getHeight();
        const int kbTop = (int) (H * 0.85f);                 // haut du clavier

        // Slope : taille FIXE, placé AU-DESSUS du clavier (avant il était en y=0.92 -> chevauchait
        // le clavier). Le label « Slope » est attaché à sa gauche.
        sliderSlope.setBounds (jmax (90, (int) (W * 0.42f)), kbTop - 30, 160, 22);

        // Sliders L/R : tailles FIXES (largeur 26 px, hauteur plafonnée) -> ils n'enflent plus avec
        // la fenêtre (setBoundsRelative donnait une largeur = 0,04 × largeur fenêtre = « fat »).
        const int sw   = 26;                                  // largeur fixe
        const int sh   = jlimit (90, 200, (int) (H * 0.28f)); // hauteur fixe (cap)
        const int x0   = (int) (W * 0.63f);                   // ancrage de la 1ère colonne
        const int step = jmax (sw + 8, jmin (sw + 34, (W - x0 - 12) / 7));
        const int y1   = (int) (H * 0.11f);                   // rangée du haut (sous les labels)
        const int y2   = y1 + sh + 26;                        // rangée du bas

        MidiSlider* top[7] = { &sliderL0,&sliderL1,&sliderL2,&sliderL3,&sliderL4,&sliderRL1,&sliderRL2 };
        for (int i = 0; i < 7; ++i) top[i]->setBounds (x0 + i * step, y1, sw, sh);

        MidiSlider* bot[6] = { &sliderR1,&sliderR2,&sliderR3,&sliderR4,&sliderRR1,&sliderRR2 };
        for (int i = 0; i < 6; ++i) bot[i]->setBounds (x0 + (i + 1) * step, y2, sw, sh); // R1 sous L1
    }
    
private:
    
    Label   labelSlope {"test", "Slope"};
    MidiKeyDraw keyDraw;
    MidiSlider  sliderSlope;
    MidiPath    samplePathLeft;
    MidiPath    samplePathRight;
    
    Label labelL0 {"","L0"};
    Label labelL1 {"","L1"};
    Label labelL2 {"","L2"};
    Label labelL3 {"","L3"};
    Label labelL4 {"","L4"};
    Label labelRL1 {"","RL1"};
    Label labelRL2 {"","RL2"};
    Label labelR1 {"","R1"};
    Label labelR2 {"","R2"};
    Label labelR3 {"","R3"};
    Label labelR4 {"","R4"};
    Label labelRR1 {"","RR1"};
    Label labelRR2 {"","RR2"};
    
    MidiSlider sliderL0;
    MidiSlider sliderL1;
    MidiSlider sliderL2;
    MidiSlider sliderL3;
    MidiSlider  sliderL4;
    MidiSlider  sliderRL1;
    MidiSlider  sliderRL2;
    
    MidiSlider  sliderR0;
    MidiSlider  sliderR1;
    MidiSlider  sliderR2;
    MidiSlider  sliderR3;
    MidiSlider  sliderR4;
    MidiSlider  sliderRR1;
    MidiSlider  sliderRR2;

    EgGraphView egGraph; // édition souris des nœuds de l'EG (niveau + rate) sur le grand graphe

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveEg)
};
