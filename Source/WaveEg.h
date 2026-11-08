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

class WaveEg    : public ElementComponent, public TextButton::Listener, Slider::Listener, public Value::Listener
{
public:
    WaveEg()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        
        // EG PAR OPERATEUR : barre d'onglets ALL | OP1..OP6 (calque le SY77 [F2] All / [F1] op).
        // ALL = diffusion aux 6 ops (proprietes partagees ELEMENT<e>EGVOL*), OPn = un seul groupe
        // (proprietes ELEMENT<e>OP<n>EGVOL*). Cf. applyOpBinding().
        for (int i = 0; i < 7; ++i)
        {
            opTab[i].setButtonText (i == 0 ? "ALL" : "OP" + String (i));
            opTab[i].setRadioGroupId (1002);
            opTab[i].setClickingTogglesState (true);
            opTab[i].setConnectedEdges ((i > 0 ? Button::ConnectedOnLeft : 0) | (i < 6 ? Button::ConnectedOnRight : 0));
            opTab[i].onClick = [this, i] { if (selOp != i) { selOp = i; applyOpBinding(); } };
            addAndMakeVisible (opTab[i]);
        }
        opTab[0].setToggleState (true, dontSendNotification);
        valueSysexIn.addListener (this);   // stocke les EG par op recus (synthe / chargement dump)
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
        // Loop point (SLP 0x0C, 0~3) + Hold time (HT 0x0D, 0~63, affiche 63~0 sur le synthe),
        // Table 1-7 OCR l.409-410. SLP : boucle de L4 (sustain) vers L1..L3 tant que la note est
        // tenue ; 3 = L4 = pas de boucle (semantique : manuel Swayed p.~23). Non verifies hardware.
        for (auto* sl : { &sliderLoop, &sliderHold })
        {
            sl->setPopupDisplayEnabled (true, true, this);
            sl->setSliderStyle (Slider::SliderStyle::LinearHorizontal);
            sl->addListener (this);
            addAndMakeVisible (*sl);
        }
        sliderLoop.textFromValueFunction = [] (double v)
        { const int i = roundToInt (v); return i >= 3 ? String ("Off") : "L" + String (i + 1); };
        sliderHold.textFromValueFunction = [] (double v) { return String (63 - roundToInt (v)); };
        labelLoop.attachToComponent (&sliderLoop, true);
        labelHold.attachToComponent (&sliderHold, true);
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
        valueSysexIn.removeListener (this);
        sliderSlope.removeListener(this);
    }
    void 	sliderValueChanged (Slider *slider) override
    {
        Logger::writeToLog("Filter::Slider value changed");
        
        if(slider == &sliderSlope)
            resized();
        if (slider == &sliderLoop)
            updateLoopMarker();
        // Mode ALL : un geste UTILISATEUR recopie la valeur dans les 6 proprietes par op (sinon
        // l'onglet OPn afficherait une valeur perimee). PAS en contexte RX/chargement
        // (suppressSend > 0) : la valeur recue ne concerne qu'un op, deja stocke par valueChanged().
        if (! isAwm && selOp == 0 && curUm != nullptr && SysexBus::get().suppressSend.load() == 0)
            for (auto& r : rows())
                if (r.s == slider)
                    for (int n = 1; n <= 6; ++n)
                        // MEME UndoManager que la prop partagee : sinon un Cmd-Z ne defait que
                        // la valeur ALL et les 6 copies par op restent en avance (desync).
                        valueTreeVoice.setProperty (propId (curElement, n, r.suffix), (int) slider->getValue(), curUm);
        repaint();
    }

    // --- EG par operateur : table sliders <-> N2 (Table 1-7) <-> suffixe de propriete.
    struct Row { MidiSlider* s; int n2; const char* suffix; };
    std::vector<Row> rows()
    {
        return { { &sliderR1, 0x00, "EGVOLR1" }, { &sliderR2, 0x01, "EGVOLR2" }, { &sliderR3, 0x02, "EGVOLR3" },
                 { &sliderR4, 0x03, "EGVOLR4" }, { &sliderRR1, 0x04, "EGVOLRR1" }, { &sliderRR2, 0x05, "EGVOLRR2" },
                 { &sliderL1, 0x06, "EGVOLLEVEL1" }, { &sliderL2, 0x07, "EGVOLLEVEL2" }, { &sliderL3, 0x08, "EGVOLLEVEL3" },
                 { &sliderL4, 0x09, "EGVOLLEVEL4" }, { &sliderRL1, 0x0A, "EGVOLRL1" }, { &sliderRL2, 0x0B, "EGVOLRL2" },
                 { &sliderL0, 0x0E, "EGVOLLEVEL0" }, { &sliderSlope, 0x0F, "SLOPEVOL" },
                 { &sliderLoop, 0x0C, "EGVOLSLP" }, { &sliderHold, 0x0D, "EGVOLHT" } };
    }
    // op 0 = partage (ALL, noms historiques ELEMENT<e>EGVOL* lus aussi par la vignette Element.h) ;
    // op 1..6 = ELEMENT<e>OP<n>EGVOL*.
    static Identifier propId (int element, int op, const char* suffix)
    {
        return Identifier ("ELEMENT" + String (element) + (op > 0 ? "OP" + String (op) : String()) + suffix);
    }
    // Groupe sysex de l'op UI n (1..6) : OP1 = 0x56 ... OP6 = 0x06 (afmOperatorGroup indexe OP6..OP1).
    static int groupForOp (int n) { return (int) SyVoice::afmOperatorGroup (6 - n); }

    // Relie les 14 sliders a l'op selectionne : adresse (group), diffusion, propriete, plage.
    // Sous ScopedEchoSuppress : le referTo re-publie la valeur liee (Slider::valueChanged ->
    // setValue(sendNotificationSync)) -> sans garde, chaque changement d'onglet enverrait 14
    // param-changes au synthe ([[feedback-hardware-safety]]).
    void applyOpBinding()
    {
        if (curUm == nullptr || isAwm) return;
        const ScopedEchoSuppress noSend;
        const int aH = (curElement - 1) << 5;
        Array<int> ops;
        for (int op = 0; op < 6; ++op)
            ops.add ((int) SyVoice::egGroupFor (SyVoice::EgKind::afmAmplitude, op));
        const int group = (selOp == 0) ? ops[0] : groupForOp (selOp);
        for (auto& r : rows())
        {
            const Identifier id = propId (curElement, selOp, r.suffix);
            // 1er passage sur OPn : amorce depuis la valeur partagee (sinon slider a 0).
            if (selOp > 0 && valueTreeVoice.getProperty (id).isVoid())
                valueTreeVoice.setProperty (id, valueTreeVoice.getProperty (propId (curElement, 0, r.suffix)), nullptr);
            // SLP jamais recu/regle (absent du decodage de dump) : affiche "Off" (3), pas L1.
            // Affichage seul, rien n'est emis (ScopedEchoSuppress).
            if (r.s == &sliderLoop && valueTreeVoice.getProperty (id).isVoid())
                valueTreeVoice.setProperty (id, 3, nullptr);
            int sx[9] = { 0x43, 0X10, 0x34, group, aH, 0x00, r.n2, 0x00, 0x00 };
            r.s->setMidiSysex (sx);
            r.s->setMidiBroadcastGroups (selOp == 0 ? ops : Array<int>());
            if (r.s == &sliderSlope)
            {
                // RS (rate scaling, 0x0F) = s/m -7..+7 (OCR l.412, table signMag {7,8}) ; le chemin
                // s/m du widget (~d + max+2) == smToWire(d,7,8) (test). Style horizontal re-pose
                // (setRangeAndRound(min<0) force Rotary).
                r.s->setRangeAndRound (-7, 7, 0);
                r.s->setSliderStyle (Slider::SliderStyle::LinearHorizontal);
            }
            else if (r.s == &sliderLoop)
                r.s->setRangeAndRound (0, 3, 3);
            else
                r.s->setRangeAndRound (0, 63, 0);
            r.s->getValueObject().referTo (valueTreeVoice.getPropertyAsValue (id, curUm));
        }
        for (int i = 0; i < 7; ++i)
            opTab[i].setToggleState (i == selOp, dontSendNotification);
        updateLoopMarker();
        resized();
        repaint();
    }

    // RX (synthe ou replay de chargement, toujours sous ScopedEchoSuppress) : tout param d'EG
    // d'ampli AFM recu pour CET element est stocke dans la propriete de SON op -> les 6 EG
    // distincts d'un dump survivent (avant : un seul jeu de sliders, dernier op replay gagnant).
    void valueChanged (Value& v) override
    {
        if (isAwm || curUm == nullptr) return;
        const var arr = v.getValue();
        if (! arr.isArray() || arr.size() < 6) return;
        const int group = (int) arr[0], aH = (int) arr[1], param = (int) arr[3], val = (int) arr[5];
        if ((group & 0x0F) != 0x06) return;               // pas un groupe d'operateur AFM
        const int opIdx = (group >> 4) & 0x0F;            // 0 = OP6 ... 5 = OP1
        if (opIdx > 5 || ((aH >> 5) + 1) != curElement) return;
        const int opN = 6 - opIdx;
        for (auto& r : rows())
            if (r.n2 == param)
            {
                const int disp = (param == 0x0F) ? SyVoice::smToDisplay (val, 7, 8) : val;
                valueTreeVoice.setProperty (propId (curElement, opN, r.suffix), disp, nullptr);
                // ET la prop PARTAGEE (dernier op recu gagne = comportement d'avant les onglets) :
                // sans ca, apres un chargement fait depuis l'onglet OPn, l'onglet ALL garderait les
                // valeurs de la voix PRECEDENTE et les diffuserait aux 6 ops au 1er geste ; la
                // vignette d'element (Element.h, props partagees) resterait elle aussi perimee.
                valueTreeVoice.setProperty (propId (curElement, 0, r.suffix), disp, nullptr);
            }
    }
    // En contexte AWM (vue Wave), l'EG de volume cible l'amp-EG AWM (group 0x07, Table 1-8)
    // au lieu de diffuser aux opérateurs AFM. Activé par WaveVue. À appeler AVANT setElementNumber.
    void setAwmMode (bool b) { isAwm = b; }
    bool isAwm = false;

    void setElementNumber ( int element, UndoManager& um) override
    {
        Logger::writeToLog( "WaveEg setElement");
        curElement = element;
        curUm      = &um;
        const int aH = (element - 1) << 5;   // T2 = (element-1)<<5

        // --- EG d'ampli AWM (Table 1-8, group 0x07) : un seul groupe (pas de diffusion), pas
        // d'onglets d'op. Correspondance sliders -> N2 : PAR1-4@50-53, PARR1@54, PAL2@55, PAL3@56,
        // PARS@57. Les sliders sans equivalent AWM (RR2/L0/L1/L4/RL1/RL2) gardent leur referTo
        // (rendu) mais n'emettent rien.
        if (isAwm)
        {
            for (int i = 0; i < 7; ++i) opTab[i].setVisible (false);
            sliderLoop.setVisible (false);   // amp-EG AWM : pas de SLP/HT (mode HOLD = PAEGMD, a part)
            sliderHold.setVisible (false);
            egGraph.loopNode = -1;
            const ScopedEchoSuppress noSend;   // le referTo re-publie la valeur liee
            for (auto& r : rows())
                r.s->getValueObject().referTo (valueTreeVoice.getPropertyAsValue (propId (element, 0, r.suffix), &um));

            int awm[9] = { 0x43, 0X10, 0x34, (int) SyVoice::egGroupFor (SyVoice::EgKind::awmAmplitude),
                           aH, 0x00, 0x00, 0x00, 0x00 };
            auto wireAwm = [&] (MidiSlider& sl, int n2, int maxVal)
            {
                awm[6] = n2;
                sl.setMidiSysex (awm);
                sl.setMidiBroadcastGroups ({});
                sl.setRangeAndRound (0, maxVal, 0);
            };
            wireAwm (sliderR1, 0x50, 63); wireAwm (sliderR2, 0x51, 63);
            wireAwm (sliderR3, 0x52, 63); wireAwm (sliderR4, 0x53, 63);
            wireAwm (sliderRR1, 0x54, 63);
            wireAwm (sliderL2, 0x55, 63); wireAwm (sliderL3, 0x56, 63);
            // PARS (rate scaling AWM, N2 0x57) = s/m -7..+7 comme PAVSON (OCR l.523/532) ;
            // etait plain 0..64. Non verifie hardware (MAP jaune).
            awm[6] = 0x57;
            sliderSlope.setMidiSysex (awm);
            sliderSlope.setMidiBroadcastGroups ({});
            sliderSlope.setRangeAndRound (-7, 7, 0);
            sliderSlope.setSliderStyle (Slider::SliderStyle::LinearHorizontal);
            for (MidiSlider* sl : { &sliderRR2, &sliderL0, &sliderL1, &sliderL4, &sliderRL1, &sliderRL2 })
            {
                sl->setMidiBroadcastGroups ({});
                sl->setRangeAndRound (0, 63, 0);
            }
            return;
        }

        // --- EG d'ampli AFM = EG PAR OPERATEUR (Table 1-7). Onglet ALL = diffusion aux 6 ops
        // (groupes 0x06..0x56), OPn = un seul groupe. Offsets : R1-4=00-03, RR1=04, RR2=05,
        // L1-4=06-09, RL1=0A, RL2=0B, L0=0E, RS(slope)=0F.
        for (int i = 0; i < 7; ++i) opTab[i].setVisible (true);
        sliderLoop.setVisible (true);
        sliderHold.setVisible (true);
        applyOpBinding();
    }
    // Rectangle du grand graphe. Partage par paint() et resized() ET decale sous la barre
    // d'onglets AFM : le graphe est ajoute APRES les boutons (donc au-dessus) et les recouvrait
    // en fenetre basse (hauteur mini 480 -> 0,05H = 24 px < bas des onglets).
    static constexpr int kTabY = 2, kTabH = 20, kTabGap = 4;
    juce::Rectangle<float> egBounds() const
    {
        auto b = getLocalBounds().toFloat();
        const float bottom = b.getHeight() * 0.79f;                       // 0,05H + 0,74H
        const float top    = jmax (b.getHeight() * 0.05f,
                                   isAwm ? 0.0f : (float) (kTabY + kTabH + kTabGap));
        return { b.getWidth() * 0.03f, top, b.getWidth() * 0.58f, jmax (40.0f, bottom - top) };
    }

    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        auto egArea = egBounds();

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
        {   // onglets ALL | OP1..OP6 au-dessus du graphe (AFM seulement)
            const int tx = (int) (b.getWidth() * 0.03f), tw = (int) (b.getWidth() * 0.58f);
            const int cw = jmax (1, tw / 7);
            for (int i = 0; i < 7; ++i)
                opTab[i].setBounds (tx + i * cw, kTabY, cw, kTabH);
        }
        egGraph.setBounds (egBounds().toNearestInt());

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
        // Loop a gauche du Slope, Hold a droite (meme rangee, au-dessus du clavier).
        sliderLoop.setBounds ((int) (W * 0.03f) + 44, kbTop - 30,
                              jmax (60, jmin (130, sliderSlope.getX() - 50 - ((int) (W * 0.03f) + 44))), 22);
        sliderHold.setBounds (sliderSlope.getRight() + 48, kbTop - 30,
                              jmax (60, jmin (160, W - (sliderSlope.getRight() + 48) - 12)), 22);

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
    Label   labelLoop  {"", "Loop"};
    Label   labelHold  {"", "Hold"};
    MidiSlider sliderLoop;   // SLP 0x0C (0..3, 3 = pas de boucle)
    MidiSlider sliderHold;   // HT  0x0D (0..63, affiche 63..0)
    // SLP v -> noeud v+1 (L1..L3) ; 3 = L4 = sustain = pas de boucle.
    void updateLoopMarker()
    {
        const int v = roundToInt (sliderLoop.getValue());
        egGraph.loopNode = (! isAwm && v >= 0 && v < 3) ? v + 1 : -1;
        egGraph.repaint();
    }
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

    TextButton   opTab[7];          // ALL | OP1..OP6
    int          selOp      = 0;    // 0 = ALL (diffusion), 1..6 = un operateur
    int          curElement = 1;
    UndoManager* curUm      = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveEg)
};
