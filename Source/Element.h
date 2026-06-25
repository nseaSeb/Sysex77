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
/** Cellule WAVE d'un élément AFM : affiche le SCHÉMA D'ALGORITHME compact de
    l'élément (topologie des 6 opérateurs) au lieu d'une forme d'onde FM calculée.
    Demande utilisateur : « l'algo devrait être à la place de la forme d'onde
    calculée qui est une fausse bonne idée ». Le rendu se fait via le moteur partagé
    AlgoDraw::drawAlgoGlyph (même que le grand schéma / les mini-schémas de routage).
    N'est visible qu'en mode AFM (masqué en AWM, cf. Element::setWaveMode). */
class FmWaveView   : public Component
{
public:
    void setAlgo (int a)
    {
        if (a == algo)
            return;
        algo = a;
        repaint();
    }

    // Mode AWM : la cellule WAVE dessine une forme d'onde « échantillon » thémée (au lieu du
    // schéma d'algo AFM). `seed` = index de wave -> tracé varié et stable. Ce composant est
    // TOUJOURS au-dessus du bouton WAVE -> couvre l'ancienne image PNG (qui ne suivait pas le thème).
    void setAwm (bool isAwm, int waveSeed)
    {
        if (awm == isAwm && seed == waveSeed) return;
        awm = isAwm; seed = waveSeed;
        repaint();
    }

    // Conservés pour compat. d'API (appelés depuis updateFmWave) : le schéma
    // d'algorithme ne dépend que du numéro d'algo, ces données ne sont plus utilisées.
    void setWaves  (const int[6])   {}
    void setRatios (const float[6]) {}
    void setLevels (const float[6]) {}

    void paint (Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();

        // AWM : forme d'onde échantillon thémée (suit l'accent du thème).
        if (awm)
        {
            SyDraw::drawSampleWave (g, area, SYColSelected, seed);
            return;
        }

        SyDraw::drawPanel (g, area, SYColSelected);

        // Schéma d'algorithme de l'élément (couleurs via rôles de thème dans AlgoDraw).
        auto gly = area.reduced (3.0f);
        if (gly.getWidth() > 8.0f && gly.getHeight() > 8.0f && algo >= 1 && algo <= 45)
            AlgoDraw::drawAlgoGlyph (g, algo, gly);

        // Numéro d'algorithme en overlay (petit fond contrasté pour la lisibilité).
        g.setFont (Font (11.0f, Font::bold));
        Rectangle<int> tb (4, 4, 48, 16);
        g.setColour (SYColBackground.withAlpha (0.65f));
        g.fillRect (tb);
        g.setColour (SYColBackground.contrasting());
        g.drawText ("ALG " + String (algo), tb.reduced (4, 0), Justification::left, false);
    }

private:
    int  algo = 1;
    bool awm  = false;
    int  seed = 0;
};

//==============================================================================
// FilterGraphView et EgGraphView (génériques) sont définis dans EnvelopeDraw.h,
// partagés avec les éditeurs plein-onglet (CommonFilter, WaveEg, …).

//==============================================================================
/** Libellé d'une table de pan (PANNM 0..95). Les 8 premiers noms reprennent la liste
    « PAN JOBS » de SynthWorks (docs/synthwork/pan-table.jpg : P1 Center … P8 Full Left)
    comme AMORCE PROVISOIRE — la correspondance index<->nom n'est PAS dans le spec et
    reste à confirmer hardware ; les autres tables s'affichent « Pan NN ». */
inline juce::String panTableName (int idx)
{
    static const char* const known[8] = {
        "Center", "Full Right", "Right 90%", "Right 75%",
        "Right 50%", "Right 25%", "Right 10%", "Full Left"
    };
    if (idx >= 0 && idx < 8) return juce::String (idx) + " " + known[idx];
    return "Pan " + juce::String (idx);
}

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

        addAndMakeVisible(pitch);

        // PAN : la colonne PAN est une CARTE (panneau + nom de table + mini-EG), DESSINÉE dans
        // paint() comme les cartes FILTER/VOLUME. Le clic n'importe où dans la carte ouvre
        // l'éditeur (PanEg) où se fait TOUTE l'édition du pan (table + nom + EG). Cf. mouseUp.

        // Libellé de la barre verticale de niveau de sortie de l'élément (ELVL), jusque-là
        // sans légende (demande utilisateur).
        addAndMakeVisible (labelLevel);
        labelLevel.setJustificationType (Justification::centred);
        labelLevel.setColour (Label::textColourId, SYColLabel);
        labelLevel.setFont (Font (9.0f, Font::bold));
        labelLevel.setInterceptsMouseClicks (false, false);

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
        // Le cadre « Pan » ne doit PAS capter la souris : sinon il avale le clic destiné à la
        // carte PAN (-> l'éditeur ne s'ouvrait pas). Les clics traversent jusqu'au mouseUp.
        groupPan.setInterceptsMouseClicks (false, false);
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
        // Clic dans la carte PAN (zone sans enfant) -> ouvre l'éditeur de pan (table + EG).
        if (e.eventComponent == this && e.mouseWasClicked()
            && getHeight() >= 56 && panCardArea.contains (e.getPosition()))
            elementValue.setValue (commande::PanEdit);
    }

    // Nom d'affichage de la table de pan : nom personnalisé (PNNAM) si défini, sinon « Pan NN ».
    String panDisplayName() const
    {
        const String custom = valueTreeVoice.getProperty (
            Identifier (String ("ELEMENT") + String (operatorID) + "PANNAME")).toString();
        const int nm = (int) valueTreeVoice.getProperty (
            Identifier (String ("ELEMENT") + String (operatorID) + "PANNM"), 0);
        return custom.isNotEmpty() ? custom : panTableName (nm);
    }

    // Niveaux/poids du mini-EG de pan (affichage carte). Défaut = centre (octet o/b 32) si non
    // réglé -> la carte montre une ligne centrale plutôt qu'un « tout à gauche » trompeur.
    void getPanEg (juce::Array<float>& levels, juce::Array<float>& weights) const
    {
        auto id = [this] (const char* s) { return Identifier (String ("ELEMENT") + String (operatorID) + s); };
        auto lv = [&] (const char* s) { return (float) (int) valueTreeVoice.getProperty (id (s), 32); };
        auto rt = [&] (const char* s) { return (float) (64 - (int) valueTreeVoice.getProperty (id (s), 0)); };
        levels  = { lv ("PANL0"), lv ("PANL1"), lv ("PANL2"), lv ("PANL3"), lv ("PANL4"), lv ("PANRL1"), lv ("PANRL2") };
        weights = { rt ("PANR1"), rt ("PANR2"), rt ("PANR3"), rt ("PANR4"), rt ("PANRR1"), rt ("PANRR2") };
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

        // --- Routage de sortie (OUTSEL) -> sysex group 0x03, OCTET PACKÉ 0x08 ---------------
        // Spec Table 1-4 (sy77midi_ocr.txt l.292-294) + carte project-sy77-addresses.md :
        //   group 0x03, param 0x08 = MCTEN (b0, micro-tuning) | OUTSEL0 (b1) | OUTSEL1 (b2).
        // Auparavant les 2 bascules étaient « rendu/état seul » (aucun envoi) -> elles ne
        // faisaient RIEN sur le synthé (problème #2). On compose l'octet complet à partir de
        // l'état des deux boutons (comme send18/19 des opérateurs AFM) et on émet par élément.
        // addrHi = (élément-1)<<5 (0x00/0x20/0x40/0x60).
        outSelAddrHi = (operatorID - 1) << 5;
        btGroup1.onClick = [this] { sendOutSel(); };
        btGroup2.onClick = [this] { sendOutSel(); };
    }

    // Émet l'octet OUTSEL (group 0x03, param 0x08, addrHi=(él-1)<<5). VÉRIFIÉ HW (dump SY77) :
    // off=0, grp1=2, grp2=4, both=6 = (outsel1<<2)|(outsel0<<1) (b0 MCTEN laissé 0).
    void sendOutSelValue (int v)
    {
        juce::uint8 b[9] = { 0x43, 0x10, 0x34, 0x03, (juce::uint8) outSelAddrHi,
                             0x00, 0x08, 0x00, (juce::uint8) (v & 0x7F) };
        outSelSender.sendParam9 ("/SYSEX", b);
    }

    // Recompose OUTSEL depuis les 2 bascules G1/G2. Si l'élément est MUTÉ, on force OUTSEL=off
    // (le mute EST l'output off, cf. setElementMuted) : changer G1/G2 quand muté met à jour le
    // routage « voulu » mais l'élément reste silencieux jusqu'au unmute. Émis au CLIC seulement.
    void sendOutSel()
    {
        if (elementMuted) { sendOutSelValue (0); return; }
        const int outsel0 = btGroup1.getToggleState() ? 1 : 0;   // b1
        const int outsel1 = btGroup2.getToggleState() ? 1 : 0;   // b2
        sendOutSelValue ((outsel1 << 2) | (outsel0 << 1));
    }

   int getOpNumber ()
    {
        return operatorID;
    }

    // MUTE de l'élément = OUTPUT SELECT à OFF (modèle réel du SY77, identifié par l'utilisateur :
    // « le mute vient de l'output off »). OFF -> OUTSEL=0 (group 0x03 / param 0x08) ; ON ->
    // restaure le routage depuis les bascules G1/G2 (sendOutSel). Le routage « voulu » reste
    // mémorisé dans les bascules (et ELEMENT<n>GROUP1/2), donc rien à sauvegarder. L'émission ne
    // part qu'au CLIC utilisateur (sécurité hardware). VÉRIFIÉ HW (off=0).
    void setElementMuted (bool muted)
    {
        if (muted == elementMuted) return;
        elementMuted = muted;
        sendOutSel();   // muté -> OUTSEL=0 (off) ; non muté -> valeur des bascules G1/G2
    }
    bool isElementMuted() const { return elementMuted; }
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

        // Bouton transparent : la forme d'onde AWM est DESSINÉE dans paint() (SYColSelected,
        // suit le thème) au lieu de l'ancienne image PNG orange figée. Reste cliquable.
        btWave.setImages (false, true, true,
                         Image(), 0.0f, Colours::transparentBlack,
                         Image(), 0.0f, Colours::transparentBlack,
                         Image(), 0.0f, Colours::transparentBlack,
                         0.0f);
        elementFmWave.setAwm (true, (int) waveNameValue.getValue()); // AWM : waveform échantillon thémée
        elementFmWave.setVisible (true);  // dessinée AU-DESSUS du bouton (couvre l'ancienne image)
        waveNameLabel.setVisible (true);  // AWM : montre le nom de la wave (par-dessus)
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
        elementFmWave.setAwm (false, 0);  // AFM : schéma d'algorithme
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

        // Carte PAN (façon FILTER/VOLUME) : panneau + nom de table + mini-EG de pan, CLIQUABLE
        // (le clic ouvre l'éditeur, cf. mouseUp). Puis les 2 sorties L/R (départ du routage vers
        // la matrice). Rangées ACTIVES uniquement (les inactives ~30 px sont épurées).
        if (getHeight() >= 56 && ! panCardArea.isEmpty())
        {
            auto pc = panCardArea.toFloat();
            juce::Array<float> lv, w;
            getPanEg (lv, w);
            SyDraw::drawEnvelope (g, pc, lv, w, 63.0f, SYColSelected, {}, false);

            // Bordure accent nette (comme les cartes FILTER/VOLUME).
            g.setColour (SYColSelected);
            g.drawRoundedRectangle (pc.reduced (0.5f), 6.0f, 1.5f);

            // Ligne centrale (pan au milieu).
            g.setColour (SYColLabel.withAlpha (0.35f));
            const float cy = pc.getBottom() - (32.0f / 63.0f) * pc.getHeight();
            g.drawHorizontalLine ((int) cy, pc.getX(), pc.getRight());

            // Nom de la table (bandeau haut lisible).
            auto nameBar = pc.reduced (3.0f).withHeight (15.0f);
            g.setColour (SYColBackground.withAlpha (0.6f));
            g.fillRect (nameBar);
            g.setColour (SYColBackground.contrasting());
            g.setFont (Font (12.0f, Font::bold));
            g.drawText (panDisplayName(), nameBar.toNearestInt(), Justification::centred, true);

            // Sorties L (bas) / R (haut) vers le bord droit de l'élément.
            const float ox = (float) getWidth() - 4.0f;
            const float rY = pc.getY() + pc.getHeight() * 0.30f;
            const float lY = pc.getY() + pc.getHeight() * 0.70f;
            g.setColour (SYColSelected);
            g.fillEllipse (ox - 3.0f, rY - 3.0f, 6.0f, 6.0f);
            g.fillEllipse (ox - 3.0f, lY - 3.0f, 6.0f, 6.0f);
            g.drawLine (pc.getRight(), rY, ox - 3.0f, rY, 1.0f);
            g.drawLine (pc.getRight(), lY, ox - 3.0f, lY, 1.0f);
            g.setColour (SYColLabel);
            g.setFont (10.0f);
            g.drawText ("R", (int) ox - 30, (int) rY - 7, 14, 12, Justification::right);
            g.drawText ("L", (int) ox - 30, (int) lY - 7, 14, 12, Justification::right);
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

        // VOLUME : carte d'EG ALIGNÉE sur FILTER (même inset vertical y=16, h=H-32) — avant elle
        // débordait en haut (y=4). La barre de niveau (ELVL) est à droite avec un ÉCART net.
        const int volW = xPan - xVol;
        groupVolume.setBounds (xVol, 0, volW, H);
        btVCA.setBounds (xVol + 4, 16, volW - 40, H - 32);
        egGraph.setBounds (btVCA.getBounds().reduced (2)); // overlay d'édition = zone dessinée
        sliderVolume.setBounds (xPan - 24, 16, 18, H - 32);
        labelLevel.setVisible (H >= 56);
        labelLevel.setBounds (xPan - 30, 2, 28, 12);

        const int panW = xEnd - xPan;
        groupPan.setBounds (xPan, 0, panW, H);
        // PAN : la carte (cliquable -> éditeur, cf. mouseUp/paint) occupe le haut de la colonne ;
        // les 2 bascules de groupe de sortie (G1/G2) sont sous la carte. Rangées INACTIVES
        // (~30 px) : carte épurée (panCardArea vide) pour ne pas brouiller les mini-graphes.
        const bool compact = H < 56;
        btGroup1.setVisible (! compact);
        btGroup2.setVisible (! compact);
        if (! compact)
        {
            const int px = xPan + 4, pw = jmax (40, panW - 8);
            panCardArea = { px, 16, pw, jmax (24, H - 16 - 26) };
            btGroup1.setBounds (px, panCardArea.getBottom() + 4, 24, 18);
            btGroup2.setBounds (px + 28, panCardArea.getBottom() + 4, 24, 18);
        }
        else
        {
            panCardArea = {};
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
    bool elementMuted = false;  // MUTE = OUTPUT SELECT off (OUTSEL=0) ; cf. setElementMuted
    
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
    // PAN : carte cliquable (panneau + nom + mini-EG) dessinée dans paint() ; clic -> éditeur.
    Rectangle<int> panCardArea;    // zone cliquable de la carte PAN (cf. mouseUp/paint/resized)
    Label       labelLevel {"","Level"};  // légende de la barre verticale ELVL (niveau élément)
    TextButton  btGroup1 {"1"};
    TextButton btGroup2 {"2"};
    SysexBusSender outSelSender;   // envoi OUTSEL (group 0x03, octet packé 0x08) — cf. sendOutSel
    int outSelAddrHi = 0;          // (élément-1)<<5, posé dans setOpNumber

    GroupComponent groupWave{"","Wave"};
    GroupComponent groupFilter{"",TRANS("Filter")};
    GroupComponent groupVolume{"","Volume"};
    GroupComponent groupPan{"","Pan"};
    float fAngle = -90 * (juce::MathConstants<float>::pi  / 180.0);
   
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Element)
};
