/*
  ==============================================================================

    Element.h
    Created: 24 Nov 2018 1:05:52pm
    Author:  Sébastien Portrait

  ==============================================================================
*/
#include "Pitch.h"
#pragma once
#include "Oscillator.h"
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "EnvelopeDraw.h"

#include "WaveVue.h"
#include "FilterVue.h"
#include "AfmVue.h"
#include "AlgoDraw.h"



//==============================================================================
/** Vue de la forme d'onde FM (approximée) d'un élément AFM, avec CACHE :
    les échantillons (normalisés -1..1) ne sont recalculés qu'au changement d'algo
    (ni à chaque paint, ni au redimensionnement). Prépare le terrain pour un calcul
    FM exact (plus coûteux). */
class FmWaveView   : public Component
{
public:
    void setAlgo (int a)
    {
        if (a == algo && ! samples.isEmpty())
            return;
        algo = a;
        recompute();   // -> remplit le cache
        repaint();
    }

    // Index des waveforms des 6 opérateurs (AFMELEMENTxOSC1-6).
    void setWaves (const int w[6])
    {
        for (int i = 0; i < 6; ++i) waves[i] = w[i];
        recompute();
        repaint();
    }

    // Ratio de fréquence des 6 opérateurs (AFMELEMENTxOSCFINE1-6, déjà converti).
    void setRatios (const float r[6])
    {
        for (int i = 0; i < 6; ++i) ratios[i] = r[i];
        recompute();
        repaint();
    }

    // Amplitude 0..1 des 6 opérateurs (AFMELEMENTxLEVEL1-6, déjà normalisé).
    void setLevels (const float l[6])
    {
        for (int i = 0; i < 6; ++i) levels[i] = l[i];
        recompute();
        repaint();
    }

    void paint (Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        SyDraw::drawPanel (g, area, SYColSelected);

        if (samples.size() > 1)
        {
            Path p;
            const int last = samples.size() - 1;
            for (int i = 0; i <= last; ++i)
            {
                const float x = area.getX() + (float) i / (float) last * area.getWidth();
                const float y = area.getCentreY() - samples[i] * area.getHeight() * 0.40f;
                if (i == 0) p.startNewSubPath (x, y);
                else        p.lineTo (x, y);
            }
            g.setColour (SYColSelected);
            g.strokePath (p, PathStrokeType (1.4f, PathStrokeType::curved));
        }

        // Numéro d'algorithme en overlay (petit fond contrasté pour la lisibilité).
        g.setFont (Font (11.0f, Font::bold));
        Rectangle<int> tb (4, 4, 48, 16);
        g.setColour (SYColBackground.withAlpha (0.65f));
        g.fillRect (tb);
        g.setColour (SYColBackground.contrasting());
        g.drawText ("ALG " + String (algo), tb.reduced (4, 0), Justification::left, false);
    }

private:
    // Moteur FM (Étape 1) : chaîne sérielle des 6 opérateurs avec leurs vraies waveforms.
    void recompute()
    {
        samples.clearQuick();
        const int   n     = 256;
        const float index = 3.0f;                          // profondeur de modulation FM
        float fb[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

        // 1er passage = chauffe pour stabiliser les feedbacks (algos 19/36/40/41/43/44).
        for (int i = 0; i <= n; ++i)
            SyDraw::fmEvalAlgo (waves, ratios, levels, algo,
                                (float) i / (float) n * MathConstants<float>::twoPi, index, fb);

        float maxAbs = 1.0e-6f;
        for (int i = 0; i <= n; ++i)
        {
            const float p = (float) i / (float) n * MathConstants<float>::twoPi;
            const float v = SyDraw::fmEvalAlgo (waves, ratios, levels, algo, p, index, fb);
            samples.add (v);
            maxAbs = jmax (maxAbs, std::abs (v));
        }
        // Normalise : les algos multi-porteuses peuvent dépasser ±1.
        const float g = 1.0f / maxAbs;
        for (auto& s : samples) s *= g;
    }

    Array<float> samples;        // cache (normalisé -1..1)
    int algo = 1;
    int waves[6] = { 0, 0, 0, 0, 0, 0 };
    float ratios[6] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    float levels[6] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
};

//==============================================================================
// FilterGraphView et EgGraphView (génériques) sont définis dans EnvelopeDraw.h,
// partagés avec les éditeurs plein-onglet (CommonFilter, WaveEg, …).

//==============================================================================
/*
*/
class Element    : public Component, TextButton::Listener, public ChangeBroadcaster, public Slider::Listener, public Value::Listener,public ChangeListener, public ValueTree::Listener
{
public:
    Element()
    {
     
        addAndMakeSlider(sliderVolume);
        sliderVolume.setColour(Slider::ColourIds::thumbColourId, SYColSelected);
        sliderVolume.setRange(0, 100);
        sliderVolume.setPopupDisplayEnabled(true, true, this);
        sliderVolume.setNumDecimalPlacesToDisplay(0);
        sliderVolume.addListener(this);
        addAndMakeSlider(sliderPan);
        sliderPan.setLookAndFeel(nullptr);   // suit le LnF du thème (plat en Light)
        sliderPan.setPopupDisplayEnabled(true, true,this);
       
        addAndMakeVisible(pitch);
        sliderPan.setRange(-64, 64);
         sliderPan.setNumDecimalPlacesToDisplay(0);
        sliderPan.addMouseListener(this, false);
        
        // Groupe de sortie de l'élément : 2 bascules (G1/G2). Both = les deux, Off = aucune.
        // Rendu/état seul pour l'instant (pas d'envoi sysex).
        addAndMakeVisible(btGroup1);
        addAndMakeVisible(btGroup2);
        btGroup1.setClickingTogglesState (true);
        btGroup2.setClickingTogglesState (true);
        btGroup1.setTooltip ("Sortie : groupe 1");
        btGroup2.setTooltip ("Sortie : groupe 2");
        btGroup1.setColour (TextButton::ColourIds::buttonOnColourId, SYColSelected);
        btGroup2.setColour (TextButton::ColourIds::buttonOnColourId, SYColSelected);

        addAndMakeVisible(groupWave);
        addAndMakeVisible(groupFilter);
        addAndMakeVisible(groupVolume);
        addAndMakeVisible(groupPan);
        addAndMakeVisible(btWave);
        // Mini-vue de l'algo FM par-dessus le bouton WAVE (n'intercepte pas la souris :
        // le bouton dessous reste cliquable pour ouvrir l'éditeur d'opérateurs).
        addAndMakeVisible(elementFmWave);
        elementFmWave.setInterceptsMouseClicks(false, false);
        elementFmWave.setVisible(false);
        // Nom de la wave (mode AWM), par-dessus le bouton WAVE, non-cliquable.
        addAndMakeVisible(waveNameLabel);
        waveNameLabel.setJustificationType(Justification::centred);
        waveNameLabel.setFont(Font(14.0f, Font::bold));
        // Contraste maximal : fond inversé OPAQUE + texte couleur du fond.
        waveNameLabel.setColour(Label::textColourId, SYColBackground);
        waveNameLabel.setColour(Label::backgroundColourId, SYColBackground.contrasting());
        waveNameLabel.setInterceptsMouseClicks(false, false);
        waveNameLabel.setVisible(false);
        // Repeint l'élément (filtre + enveloppe de volume) dès qu'une de ses valeurs change.
        valueTreeVoice.addListener(this);
        /*
        Path shape;
        shape.lineTo(0.0f, 1.0f);
        shape.lineTo(0.2f,0.1f);
        shape.lineTo(0.6f,0.1f);
        shape.lineTo(1.0f,1.0f);
        shape.lineTo(0.0f,1.0f);
        pathFilter1.addPath(shape);
*/
        addAndMakeVisible(btFilter);
        
        addAndMakeVisible(btVCA);

        btWave.addListener(this);
        btVCA.addListener(this);
        btFilter.addListener(this);
    
        
  

        imgAudio =  ImageFileFormat::loadFrom(BinaryData::Audio_png, (size_t) BinaryData::Audio_pngSize);
        imgAFM = ImageFileFormat::loadFrom(BinaryData::AFM_png,(size_t) BinaryData::AFM_pngSize);
        // (Filter/VCA : la réponse du filtre et l'enveloppe sont DESSINÉES dans paint() —
        //  plus d'images chargées ici, cf. nettoyage des ressources.)
        // L'emplacement Filter affiche désormais une vignette (réponse du filtre)
        // dessinée dans paint(). Le bouton reste cliquable (seuil hit-test = 0).
        btFilter.setImages (false, true, true,
                          Image(), 0.0f, Colours::transparentBlack,
                          Image(), 0.0f, Colours::transparentBlack,
                          Image(), 0.0f, Colours::transparentBlack,
                          0.0f);

        // Édition à la souris du filtre : un overlay capte le glissement (cutoff/résonance)
        // et le clic simple (ouvre l'éditeur). btFilter ne capte plus la souris.
        btFilter.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (filterGraph);
        filterGraph.onOpenEditor = [this] { elementValue.setValue (commande::FilterEdit); };
        filterGraph.onEdit = [this] (int cut, int res)
        {
            valueTreeVoice.setProperty (Identifier ("ELEMENT" + String (operatorID) + "FQ1"), cut, nullptr);
            valueTreeVoice.setProperty (Identifier ("ELEMENT" + String (operatorID) + "RESONNANCEFILTRE"), res, nullptr);
        };

        // Édition à la souris de l'EG de volume : overlay sur la cellule VOLUME.
        btVCA.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (egGraph);
        egGraph.getData = [this] (juce::Array<float>& l, juce::Array<float>& w) { getVolEg (l, w); };
        egGraph.onOpenEditor = [this]
        {
            elementValue.setValue (operatorMode == mode::AWM ? (int) commande::VolumeEdit
                                                             : (int) commande::VolumeAFM);
        };
        egGraph.maxLevel = 127.0f;                       // vignette : niveaux EGVOL 0..127
        egGraph.onEditNode = [this] (int node, float levelF, float segWeightF)
        {
            const int level = roundToInt (levelF);
            const int rateWeight = (segWeightF < 0.0f) ? -1 : roundToInt (segWeightF);
            if (node < 0 || node > 4) return;
            auto setP = [this] (const String& suffix, int v)
            { valueTreeVoice.setProperty (Identifier ("ELEMENT" + String (operatorID) + suffix), v, nullptr); };

            const char* lvlIds[5] = { "EGVOLLEVEL0", "EGVOLLEVEL1", "EGVOLLEVEL2", "EGVOLLEVEL3", "EGVOLLEVEL4" };
            const char* rIds[4]   = { "EGVOLR1", "EGVOLR2", "EGVOLR3", "EGVOLR4" };

            // EG jamais réglé (tous niveaux à 0) -> on "fige" d'abord la forme par défaut
            // pour ne pas faire s'effondrer les autres nœuds au 1er glissement.
            bool unset = true;
            for (int i = 0; i < 5; ++i)
                if ((int) valueTreeVoice.getProperty (Identifier ("ELEMENT" + String (operatorID) + lvlIds[i]), 0) != 0)
                    unset = false;
            if (unset)
            {
                juce::Array<float> l, w;
                getVolEg (l, w);
                for (int i = 0; i < 5; ++i) setP (lvlIds[i], roundToInt (l[i]));
                for (int i = 0; i < 4; ++i) setP (rIds[i],   roundToInt (w[i]));
            }
            setP (lvlIds[node], level);
            if (rateWeight >= 0 && node >= 1)            // rate du segment entrant (nœuds 1..4)
                setP (rIds[node - 1], rateWeight);
        };
   
    
  
   // btVCA reste cliquable (ouvre l'éditeur d'EG de volume) mais sans image :
   // l'enveloppe de volume est dessinée dans paint() (cf. SyDraw::drawEnvelope).

        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.

    }

    ~Element()
    {
        btWave.removeListener(this);
        btVCA.removeListener(this);
        btFilter.removeListener(this);
        sliderPan.setLookAndFeel(nullptr);
        sliderPan.removeMouseListener(this);
        sliderVolume.removeListener(this);
        valueTreeVoice.removeListener(this);
    }

    // Repeinture live des vignettes (filtre, enveloppe de volume) à chaque édition ;
    // si une waveform/algo d'opérateur de CET élément change, on recalcule la forme FM.
    void valueTreePropertyChanged (ValueTree&, const Identifier& property) override
    {
        const auto p = property.toString();
        if (p.startsWith ("AFMELEMENT" + String (operatorID))
            || p == ("AFMALGOELEMENT" + String (operatorID)))
            updateFmWave();
        repaint();
    }

    // Pousse les 6 waveforms d'opérateur (+ ratios + algo) de cet élément vers la vue FM.
    void updateFmWave()
    {
        int   w[6];
        float r[6];
        float l[6];
        int   lvlSum = 0;
        for (int i = 0; i < 6; ++i)
        {
            w[i] = (int) valueTreeVoice.getProperty (Identifier ("AFMELEMENT" + String (operatorID)
                                                                 + "OSC" + String (i + 1)), 0);
            const int fine = (int) valueTreeVoice.getProperty (Identifier ("AFMELEMENT" + String (operatorID)
                                                                 + "OSCFINE" + String (i + 1)), 0);
            r[i] = (float) jmax (1, roundToInt (fine / 8.0)); // 0-127 -> ratio harmonique ~1..16

            const int lvl = (int) valueTreeVoice.getProperty (Identifier ("AFMELEMENT" + String (operatorID)
                                                                 + "LEVEL" + String (i + 1)), 0);
            l[i] = (float) lvl / 127.0f;                       // amplitude normalisée
            lvlSum += lvl;
        }
        // Aucun niveau stocké (preset sans données de niveau) -> tout plein, rendu visible.
        if (lvlSum == 0)
            for (int i = 0; i < 6; ++i) l[i] = 1.0f;

        elementFmWave.setWaves (w);
        elementFmWave.setRatios (r);
        elementFmWave.setLevels (l);
        elementFmWave.setAlgo (jmax (1, (int) algoValue.getValue()));
    }
    void addAndMakeSlider (Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setRange(0, 127);
        
    }
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        
        
        Logger::writeToLog("Element: changeListener");
        //    Logger::writeToLog(source);
        //    test.setVisible(true);
        
    }
    
    void mouseUp (const MouseEvent& e) override
    {
        
        if(e.getNumberOfClicks()>1) // double clic
        {
            if(e.eventComponent == &sliderPan)
                sliderPan.setValue(0);
        }
            
    }
    void valueChanged(Value & value) override
    {
        Logger::writeToLog("Element value change");
        if (value.refersToSameSourceAs (algoValue))
            updateFmWave();
        if (value.refersToSameSourceAs (waveNameValue))
            waveNameLabel.setText (awmWaveName ((int) waveNameValue.getValue(), SYModel == 3), dontSendNotification);
/*        if(operatorID == 1)
            sliderVolume.setValue(intVolumeOP1);
  
        if(operatorID == 2)
                  sliderVolume.setValue(intVolumeOP2);
        if(operatorID == 3)
                 sliderVolume.setValue(intVolumeOP3);
        if(operatorID == 4)
             sliderVolume.setValue(intVolumeOP4);
  */
    }
    void sliderValueChanged (Slider* slider) override
    {
 
    }
    void buttonClicked (Button* button) override
    {
    
        if(button == &btWave)
        {
            Logger::writeToLog("Bouton wave clicked");
           elementValue.setValue(0);
            if(operatorMode == mode::AWM)
            elementValue.setValue(commande::WaveEdit) ;
            
            else
            {
                elementValue.setValue(commande::AfmEdit);
            }
        }
        if(button == &btFilter)
            elementValue.setValue(commande::FilterEdit);
        
        if (button == &btVCA)
        {
            if(operatorMode == mode::AWM)
            {
            elementValue.setValue(commande::VolumeEdit);
            }
            else
            {
            elementValue.setValue(commande::VolumeAFM);
            }
        }
    
  //      sendChangeMessage();
  
    }
    void setOpNumber (int operatorNumber, UndoManager& undoManager)
    {
        Logger::writeToLog(String(operatorNumber));
        operatorID = operatorNumber;
        pitch.setOperator(operatorNumber, undoManager);

        // Lien vers l'algo FM de cet élément -> mini-vue (mise à jour live).
        algoValue = valueTreeVoice.getPropertyAsValue (Identifier ("AFMALGOELEMENT" + String (operatorID)), &undoManager);
        algoValue.addListener (this);
        updateFmWave();

        // Wave AWM choisie pour cet élément : on suit l'INDEX (WAVEFORM) et on affiche
        // le NOM via le lookup XML (vrai nom même par défaut, pas seulement après sélection).
        waveNameValue = valueTreeVoice.getPropertyAsValue (Identifier ("ELEMENT" + String (operatorID) + "WAVEFORM"), &undoManager);
        waveNameValue.addListener (this);
        waveNameLabel.setText (awmWaveName ((int) waveNameValue.getValue(), SYModel == 3), dontSendNotification);
        if(operatorID == 1)
        {
//            btFilter.setShape(pathFilter1, false, false, true);
            Value valueElement1Volume = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1VOLUME.toString(), &undoManager);
            sliderVolume.getValueObject().referTo(valueElement1Volume);
            int data[9] = { 0x43, 0X10, 0x34, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 };
            sliderVolume.setMidiSysex(data);
        
        }
        if(operatorID == 2)
        {
//            btFilter.setShape(pathFilter2, false, false, true);
            Value valueElement2Volume = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2VOLUME.toString(), &undoManager);
            sliderVolume.getValueObject().referTo(valueElement2Volume);
            int data[9] = { 0x43, 0X10, 0x34, 0x03, 0x20, 0x00, 0x00, 0x00, 0x00 };
            sliderVolume.setMidiSysex(data);
        }
        if(operatorID == 3)
        {
//            btFilter.setShape(pathFilter3, true, false, true);
            Value valueElement3Volume = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3VOLUME.toString(), &undoManager);
            sliderVolume.getValueObject().referTo(valueElement3Volume);
            int data[9] = { 0x43, 0X10, 0x34, 0x03, 0x40, 0x00, 0x00, 0x00, 0x00 };
            sliderVolume.setMidiSysex(data);
        }
        if(operatorID == 4)
        {
 //           btFilter.setShape(pathFilter4, false, false, true);
            Value valueElement4Volume = valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4VOLUME.toString(), &undoManager);
            sliderVolume.getValueObject().referTo(valueElement4Volume);
            int data[9] = { 0x43, 0X10, 0x34, 0x03, 0x60, 0x00, 0x00, 0x00, 0x00 };
            sliderVolume.setMidiSysex(data);
        }

        // Groupe de sortie (rendu/état seul) : lié par élément à valueTreeVoice.
        const Identifier g1id ("ELEMENT" + String (operatorID) + "GROUP1");
        const Identifier g2id ("ELEMENT" + String (operatorID) + "GROUP2");
        // Défaut sensé (et schéma de routage non vide) : sortie vers le groupe 1 si rien n'est réglé.
        if (! valueTreeVoice.hasProperty (g1id) && ! valueTreeVoice.hasProperty (g2id))
            valueTreeVoice.setProperty (g1id, true, nullptr);
        btGroup1.getToggleStateValue().referTo (valueTreeVoice.getPropertyAsValue (g1id, &undoManager));
        btGroup2.getToggleStateValue().referTo (valueTreeVoice.getPropertyAsValue (g2id, &undoManager));
    }

   int getOpNumber ()
    {
        return operatorID;
    }
    void setOpMode (int mode)
    {
        operatorMode = mode;
    }
    int getOpMode ()
    {
        return operatorMode;
    }
    void setWaveMode()
    {
                btWave.setButtonText("Wave");
 
        btWave.setImages (false, true, true,
                         imgAudio, 0.7f, Colours::transparentBlack,
                         imgAudio, 1.0f, Colours::transparentBlack,
                          imgAudio, 0.6f, Colours::transparentBlack,
                         0.0f);
        elementFmWave.setVisible (false); // AWM : pas de forme d'onde FM
        waveNameLabel.setVisible (true);  // AWM : montre le nom de la wave
        waveNameLabel.setText (awmWaveName ((int) waveNameValue.getValue(), SYModel == 3), dontSendNotification);
        repaint();
    }
    void setAfmMode()
    {
        btWave.setButtonText("AFM");
        btWave.setButtonText("AFM");

        btWave.setImages (false, true, true,
                          imgAFM, 0.7f, Colours::transparentBlack,
                          imgAFM, 1.0f, Colours::transparentBlack,
                          imgAFM, 0.6f, Colours::transparentBlack,
                          0.0f);
        elementFmWave.setVisible (true);  // AFM : forme d'onde FM (+ n° algo en overlay)
        updateFmWave();
        waveNameLabel.setVisible (false); // AFM : pas de nom de wave
        repaint();
    }
    // Niveaux (5) + poids de segment (4) EFFECTIFS de l'EG de volume de cet élément,
    // avec repli sur une forme par défaut si l'EG n'est pas réglé. Partagé par paint()
    // (tracé) et EgGraphView (test de proximité) pour rester cohérents.
    void getVolEg (juce::Array<float>& levels, juce::Array<float>& weights) const
    {
        auto id = [this] (const char* s) { return Identifier (String ("ELEMENT") + String (operatorID) + s); };
        auto eg = [&] (const char* s) { return (float) (int) valueTreeVoice.getProperty (id (s), 0); };
        levels  = { eg ("EGVOLLEVEL0"), eg ("EGVOLLEVEL1"), eg ("EGVOLLEVEL2"), eg ("EGVOLLEVEL3"), eg ("EGVOLLEVEL4") };
        weights = { jmax (1.0f, eg ("EGVOLR1")), jmax (1.0f, eg ("EGVOLR2")),
                    jmax (1.0f, eg ("EGVOLR3")), jmax (1.0f, eg ("EGVOLR4")) };
        float maxL = 0.0f;
        for (auto l : levels) maxL = jmax (maxL, l);
        if (maxL <= 0.0f) { levels = { 0.0f, 120.0f, 84.0f, 84.0f, 0.0f }; weights = { 1.0f, 2.0f, 4.0f, 3.0f }; }
    }

    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
           draws some placeholder text to get you started.

           You should replace everything in this method with your own
           drawing code..
        */

//        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background

        // Vignette : réponse du filtre de cet élément (lue depuis valueTreeVoice)
        {
            auto idFor = [this] (const char* suffix)
            { return Identifier (String ("ELEMENT") + String (operatorID) + suffix); };

            int   fMode = (int) valueTreeVoice.getProperty (idFor ("MODEFILTRE1"), 0);
            float fCut  = (float) (int) valueTreeVoice.getProperty (idFor ("FQ1"), 0) / 127.0f;
            float fRes  = (float) (int) valueTreeVoice.getProperty (idFor ("RESONNANCEFILTRE"), 0) / 127.0f;

            auto fb = groupFilter.getBounds().toFloat().reduced (5.0f, 14.0f);
            if (fb.getWidth() > 12.0f && fb.getHeight() > 12.0f)
                SyDraw::drawFilterResponse (g, fb, fMode, fCut, fRes, SYColSelected);

            // Enveloppe de VOLUME (data-driven) : niveaux/rates effectifs (cf. getVolEg),
            // partagés avec EgGraphView pour l'édition à la souris.
            auto vb = btVCA.getBounds().toFloat().reduced (2.0f);
            if (vb.getWidth() > 12.0f && vb.getHeight() > 12.0f)
            {
                juce::Array<float> levels, weights;
                getVolEg (levels, weights);
                SyDraw::drawEnvelope (g, vb, levels, weights, 127.0f, SYColSelected);
            }
        }

        // Deux sorties du pan (L / R) sur le bord droit de l'élément : point de départ
        // du routage vers la matrice (à connecter ensuite).
        {
            auto pb = groupPan.getBounds().toFloat();
            const float ox = (float) getWidth() - 4.0f;
            const float lY = pb.getY() + pb.getHeight() * 0.32f;
            const float rY = pb.getY() + pb.getHeight() * 0.68f;
            g.setColour (SYColSelected);
            g.fillEllipse (ox - 3.0f, lY - 3.0f, 6.0f, 6.0f);
            g.fillEllipse (ox - 3.0f, rY - 3.0f, 6.0f, 6.0f);
            g.drawLine (sliderPan.getRight(), lY, ox - 3.0f, lY, 1.0f);
            g.drawLine (sliderPan.getRight(), rY, ox - 3.0f, rY, 1.0f);
            g.setColour (SYColLabel);
            g.setFont (10.0f);
            g.drawText ("L", (int) ox - 30, (int) lY - 7, 14, 12, Justification::right);
            g.drawText ("R", (int) ox - 30, (int) rY - 7, 14, 12, Justification::right);
        }

        g.setColour (SYColBackground.contrasting());
        g.addTransform(AffineTransform::rotation(fAngle));
        String str = "OP" + String(operatorID) + " ";
        if(operatorMode == 1)
        {
            str = str + "AFM Mono";
            setAfmMode();
        }
        if(operatorMode == 2)
        {
             str = str + "AFM Poly";
            setAfmMode();
        }
        if(operatorMode == 3)
        {
            str = str + "AWM";
            setWaveMode();
        }
        if(operatorMode == 4)
        {
            str = str + "DRUM Set";
        }
        g.drawText( str, -getHeight(), 0, getHeight(), 24, Justification::centred);
        
//        Rect localRect = getLocalBounds();   // draw an outline around the component
  //      g.fillRoundedRectangle (0, 0, getWidth(),getHeight(), 20);
     
    }

    void resized() override
    {
        // Colonnes homogènes (mêmes fractions que les en-têtes de VoicePage) :
        // PITCH .18 | WAVE .18 | FILTER .26 | VOLUME .26 | PAN .12
        const float W = (float) getWidth();
        const int   H = getHeight();
        auto col = [W] (float f) { return (int) (W * f); };
        const int xPitch = col (0.0f),  xWave = col (0.18f), xFilter = col (0.36f),
                  xVol   = col (0.62f), xPan  = col (0.88f), xEnd   = col (1.0f);

        const int labelMargin = 22; // place pour le libellé vertical "OPx ..." à gauche
        pitch.setBounds (xPitch + labelMargin, 0, (xWave - xPitch) - labelMargin - 2, H);

        groupWave.setBounds (xWave, 0, xFilter - xWave, H);
        btWave.setBounds (groupWave.getBounds().reduced (4, 16));
        {
            auto wb = btWave.getBounds().reduced (2);
            elementFmWave.setBounds (wb);                         // AFM : forme d'onde FM plein-cellule
            waveNameLabel.setBounds (wb.getX(), wb.getCentreY() - 13, wb.getWidth(), 26); // AWM : nom en overlay
        }

        groupFilter.setBounds (xFilter, 0, xVol - xFilter, H);
        btFilter.setBounds (groupFilter.getBounds().reduced (4, 16));
        // L'overlay d'édition couvre exactement la zone où la réponse est dessinée (paint).
        filterGraph.setBounds (groupFilter.getBounds().reduced (5, 14));

        const int volW = xPan - xVol;
        groupVolume.setBounds (xVol, 0, volW, H);
        btVCA.setBounds (xVol + 4, 4, volW - 34, H - 8);
        egGraph.setBounds (btVCA.getBounds().reduced (2)); // overlay d'édition = zone dessinée
        sliderVolume.setBounds (xPan - 30, 16, 24, H - 24);

        const int panW = xEnd - xPan;
        groupPan.setBounds (xPan, 0, panW, H);
        // Pan vertical (façon SynthWorks) ; les 2 sorties L/R sont dessinées à droite (paint).
        sliderPan.setBounds (xPan + 6, 16, 16, H - 28);
        // Bascules de groupe de sortie (G1/G2), empilées à droite du pan. Masquées sur les
        // rangées d'éléments inactifs (trop courtes, ~30 px) pour éviter le débordement.
        const bool compact = H < 56;
        btGroup1.setVisible (! compact);
        btGroup2.setVisible (! compact);
        if (! compact)
        {
            const int gx = sliderPan.getRight() + 4;
            btGroup1.setBounds (gx, 16, 22, 18);
            btGroup2.setBounds (gx, 36, 22, 18);
        }
    }
enum mode
    {
        AFMmono = 1,
        AFMPoly = 2,
        AWM = 3,
        DrumSet = 4
    };
enum commande
    {
        AfmEdit = 1,
        WaveEdit = 3,
        DrumEdit = 4,
        FilterEdit = 5,
        VolumeEdit = 6,
        PanEdit = 7,
        VolumeAFM =8
    };
public:
        Value elementValue;
private:
    int operatorID;
    int operatorMode;
    
    Image imgAudio;
    Image imgAFM;
    Pitch pitch;
    ImageButton btWave;
    ImageButton btFilter; //{"Filter",SYColSelected,Colours::white,Colours::orange };
    ImageButton btVCA;
//    Slider sliderPitch {Slider::SliderStyle::Rotary , Slider::NoTextBox};
 //   Slider sliderFine {Slider::SliderStyle::LinearBar,Slider::NoTextBox};
    MidiSlider sliderVolume;
    FmWaveView elementFmWave; // forme d'onde FM (toute la cellule WAVE, mode AFM) + n° algo en overlay
    FilterGraphView filterGraph; // édition souris du filtre (cutoff/résonance) sur la cellule FILTER
    EgGraphView egGraph;         // édition souris des niveaux de l'EG de volume (cellule VOLUME)
    Value    algoValue;     // -> AFMALGOELEMENTx
    Label    waveNameLabel; // nom de la wave (colonne WAVE, mode AWM)
    Value    waveNameValue; // -> ELEMENT<n>WAVENAME
    Slider sliderPan {Slider::SliderStyle::LinearVertical , Slider::NoTextBox};
    TextButton  btGroup1 {"1"};
    TextButton btGroup2 {"2"};

    GroupComponent groupWave{"","Wave"};
    GroupComponent groupFilter{"",TRANS("Filter")};
    GroupComponent groupVolume{"","Volume"};
    GroupComponent groupPan{"","Pan"};
    float fAngle = -90 * (juce::MathConstants<float>::pi  / 180.0);
         CustomLookAndFeel myLookAndFeel;
   
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Element)
};
