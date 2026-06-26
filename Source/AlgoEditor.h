/*
  ==============================================================================

    AlgoEditor.h
    Éditeur d'ALGORITHME FM (colonne droite de la vue AFM). Remplace l'ancien
    panneau passif `Operator` : il garde la sélection de PRESET (1..45, envoi ALGNUM)
    ET ajoute un mode LIBRE (« Free ») où, pour l'opérateur sélectionné, on édite ses
    2 entrées (off/thru/AWM/registres/feedback/accumulateur/noise), sa destination et
    l'état d'accumulateur. Le schéma `AlgoDraw` est le miroir live (preset ou custom).

    Décisions : édition « codes par op » (fidèle au modèle SY77), diagramme live,
    panneau en place. Le LEVEL (TL) reste édité dans le détail d'opérateur (Oscillator).
    Le mode free est un flag SÉPARÉ `AFMELEMENT<e>ALGFREE` (le slider preste reste 1..45) ;
    l'envoi du routage free (ALGNUM=127 + ALGSRC/ALGDST…) est branché en étape 4
    (`SyVoice::sendAfmFreeAlgo`). Cf. AlgoModel.h pour le modèle custom.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "AlgoDraw.h"
#include "AlgoModel.h"

class AlgoEditorView : public ElementComponent, private Value::Listener
{
public:
    AlgoEditorView()
    {
        addAndMakeVisible (algoFm);

        // Sélecteur de PRESET 1..45 (comme l'ancien Operator) : MidiSlider -> ALGNUM (0x05/0x00).
        addAndMakeVisible (sliderAlgo);
        sliderAlgo.setSliderStyle (Slider::LinearHorizontal);
        sliderAlgo.setRangeAndRound (1, 45, 1);
        sliderAlgo.setTextBoxStyle (Slider::NoTextBox, true, 10, 10);
        sliderAlgo.setColour (Slider::thumbColourId, SYColSelected);
        sliderAlgo.setPopupDisplayEnabled (true, true, this);
        sliderAlgo.onValueChange = [this] { onPresetChanged(); };
        sliderAlgo.getValueObject().addListener (this);   // redraw aussi au chargement de voix

        addAndMakeVisible (labelAlgo);
        labelAlgo.attachToComponent (&sliderAlgo, false);
        addAndMakeVisible (labelAlgoNum);
        labelAlgoNum.setJustificationType (Justification::centredRight);
        labelAlgoNum.setColour (Label::textColourId, SYColSelected);
        labelAlgoNum.setFont (Font (FontOptions (13.0f)).boldened());

        // Toggle FREE (algorithme libre).
        addAndMakeVisible (btFree);
        btFree.setClickingTogglesState (true);
        btFree.setColour (TextButton::buttonOnColourId, SYColSelected);
        btFree.setTooltip ("Algorithme libre : editer le routage des operateurs");
        btFree.onClick = [this] { onFreeToggled(); };

        // Bouton « Dériver du preset » (amorce le custom depuis le preset courant du slider).
        addAndMakeVisible (btDerive);
        btDerive.setTooltip ("Copier l'algo preset courant dans l'algo libre, comme point de depart");
        btDerive.onClick = [this] { deriveFromPreset(); };

        // Sélecteur d'opérateur (OP1..OP6) pour l'édition free.
        for (int i = 0; i < 6; ++i)
        {
            auto* b = opTab.add (new TextButton (String (i + 1)));
            addAndMakeVisible (b);
            b->setClickingTogglesState (true);
            b->setRadioGroupId (9201);
            b->setColour (TextButton::buttonOnColourId, SYColSelected);
            b->onClick = [this, i] { selOp = i; refreshOpControls(); };
        }
        opTab[0]->setToggleState (true, dontSendNotification);

        // Combos de routage de l'op sélectionné.
        addAndMakeVisible (cbIn0);  addAndMakeVisible (cbIn1);
        for (auto* cb : { &cbIn0, &cbIn1 })
            for (auto* n : { "Off", "Thru OP+1", "AWM", "Reg 1", "Reg 2", "Reg 3",
                             "Feedback", "FB 2", "FB 3", "Accum", "Noise" })
                cb->addItem (n, cb->getNumItems() + 1);   // id = code+1
        addAndMakeVisible (cbDst);
        for (auto* n : { "-", "Reg 1", "Reg 2", "Reg 3" }) cbDst.addItem (n, cbDst.getNumItems() + 1);
        addAndMakeVisible (cbAcc);
        // Accumulateur (acc0,acc1) : Init(1,0) / Sum=porteuse(1,1) / Keep(0,1) / Reset(0,0).
        for (auto* n : { "Init", "Sum (out)", "Keep", "Reset" }) cbAcc.addItem (n, cbAcc.getNumItems() + 1);

        cbIn0.onChange = [this] { onRouteEdited ("INA", cbIn0.getSelectedId() - 1); };
        cbIn1.onChange = [this] { onRouteEdited ("INB", cbIn1.getSelectedId() - 1); };
        cbDst.onChange = [this] { onRouteEdited ("OUT", cbDst.getSelectedId() - 1); };
        cbAcc.onChange = [this] { onAccEdited(); };

        for (auto* l : { &lblIn0, &lblIn1, &lblDst, &lblAcc, &lblOp, &lblLvl })
        {
            addAndMakeVisible (*l);
            l->setColour (Label::textColourId, SYColLabel);
            l->setFont (Font (FontOptions (11.0f)));
        }

        // Titre de section : rappelle quel opérateur on édite (les combos/niveau ci-dessous).
        addAndMakeVisible (lblOpHdr);
        lblOpHdr.setColour (Label::textColourId, SYColSelected);
        lblOpHdr.setFont (Font (FontOptions (12.0f)).boldened());

        // Niveau (TL) de l'op sélectionné : barre éditable. autoSend OFF -> n'émet pas elle-même ;
        // elle partage la valeur AFMELEMENT<e>LEVEL<op> du slider Level du détail, qui émet le 0x1B.
        addAndMakeVisible (sliderLevel);
        sliderLevel.setSliderStyle (Slider::LinearBar);
        sliderLevel.setTextBoxStyle (Slider::TextBoxRight, false, 34, 16);
        sliderLevel.setRange (0, 127, 1);
        sliderLevel.setColour (Slider::trackColourId, SYColSelected);
        sliderLevel.setAutoSend (false);
        sliderLevel.setPopupDisplayEnabled (true, true, this);
    }

    void setElementNumber (int element, UndoManager& um) override
    {
        elem = jlimit (1, 4, element);
        undo = &um;

        int sx[9] = { 0x43, 0X10, 0x34, 0x05, (int) SyVoice::elementAddrHi (elem - 1), 0x00, 0x00, 0x00, 0x00 };
        sliderAlgo.setMidiSysex (sx);
        sliderAlgo.setMidiValueOffset (-1);   // synthé 0..44 <-> UI 1..45
        sliderAlgo.getValueObject().referTo (valueTreeVoice.getPropertyAsValue (
            Identifier ("AFMALGOELEMENT" + String (elem)), &um));

        freeValue.referTo (valueTreeVoice.getPropertyAsValue (
            Identifier ("AFMELEMENT" + String (elem) + "ALGFREE"), &um));
        freeValue.addListener (this);

        refreshAll();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6);
        const int line = jlimit (20, 26, r.getHeight() / 18);

        // Ligne haute : preset slider + Free.
        labelAlgoNum.setBounds (r.getX(), r.getY(), r.getWidth() - 64, 16);
        btFree.setBounds (r.getRight() - 60, r.getY(), 60, 18);
        auto top = r.removeFromTop (line + 18); top.removeFromTop (18);
        sliderAlgo.setBounds (top.removeFromLeft (top.getWidth() - 130));
        btDerive.setBounds (top.reduced (4, 0));

        // Schéma (~45 % du reste).
        algoFm.setBounds (r.removeFromTop (jmax (90, r.getHeight() * 45 / 100)));

        r.removeFromTop (4);
        // Barre OP1..6.
        { auto row = r.removeFromTop (line); const int w = row.getWidth() / 6;
          for (int i = 0; i < 6; ++i) opTab[i]->setBounds (row.removeFromLeft (w).reduced (1)); }
        r.removeFromTop (2);

        // Titre de section (« Operateur N ») : ce que les champs ci-dessous éditent.
        lblOpHdr.setBounds (r.removeFromTop (line));
        r.removeFromTop (2);

        // Champs de l'op (label gauche, contrôle droite) : entrées 1/2, sortie, accum, NIVEAU.
        auto field = [&] (Label& l, Component& c)
        {
            auto row = r.removeFromTop (line); r.removeFromTop (2);
            l.setBounds (row.removeFromLeft (52));
            c.setBounds (row);
        };
        field (lblIn0, cbIn0);
        field (lblIn1, cbIn1);
        field (lblDst, cbDst);
        field (lblAcc, cbAcc);
        field (lblLvl, sliderLevel);
    }

private:
    int  presetNum() const { return jlimit (1, 45, (int) sliderAlgo.getValue()); }
    bool isFree()    const { return (bool) freeValue.getValue(); }

    // (Re)synchronise tout l'éditeur depuis l'état (preset/free), met à jour le schéma.
    void refreshAll()
    {
        const bool free = isFree();
        // En free, le slider preset ne doit PAS auto-envoyer ALGNUM (il sert de base à dériver).
        sliderAlgo.setAutoSend (! free);
        sliderAlgo.setEnabled (! free);
        btFree.setToggleState (free, dontSendNotification);

        // Les contrôles d'édition restent TOUJOURS actifs : éditer un code bascule
        // automatiquement en mode libre (ensureFree). Plus de « gate Free » à cliquer
        // d'abord (qui rendait l'éditeur inopérant tant que Free n'était pas armé).
        for (auto* c : { &cbIn0, &cbIn1, &cbDst, &cbAcc })
            c->setEnabled (true);
        for (int i = 0; i < 6; ++i) opTab[i]->setEnabled (true);
        btDerive.setEnabled (true);

        if (free)
        {
            algoFm.setCustomAlgo (SyAlgo::buildCustomAlgo (elem));
            labelAlgoNum.setText ("ALG Free", dontSendNotification);
        }
        else
        {
            algoFm.setAlgo (presetNum());
            labelAlgoNum.setText ("ALG " + String (presetNum()), dontSendNotification);
        }
        refreshOpControls();
        repaint();
    }

    // Met les combos sur les valeurs de l'op sélectionné (free) ou du preset (lecture).
    void refreshOpControls()
    {
        SyDraw::AlgoDef def = isFree() ? SyAlgo::buildCustomAlgo (elem)
                                       : SyDraw::kAlgo[ (presetNum() - 1) % 45 ];
        cbIn0.setSelectedId (jlimit (0, 10, def.in0[selOp]) + 1, dontSendNotification);
        cbIn1.setSelectedId (jlimit (0, 10, def.in1[selOp]) + 1, dontSendNotification);
        cbDst.setSelectedId (jlimit (0, 3,  def.outd[selOp]) + 1, dontSendNotification);
        const int a0 = def.acc0[selOp], a1 = def.acc1[selOp];
        const int accItem = (a0 && ! a1) ? 1 : (a0 && a1) ? 2 : (! a0 && a1) ? 3 : 4;
        cbAcc.setSelectedId (accItem, dontSendNotification);

        lblOpHdr.setText ("Operateur " + String (selOp + 1), dontSendNotification);

        // Niveau (TL) : partage la valeur du slider Level du détail (AFMELEMENT<e>LEVEL<op>),
        // qui porte l'envoi 0x1B. Re-referTo à CHAQUE changement d'op.
        if (undo != nullptr)
            sliderLevel.getValueObject().referTo (valueTreeVoice.getPropertyAsValue (
                Identifier ("AFMELEMENT" + String (elem) + "LEVEL" + String (selOp + 1)), undo));
    }

    void onPresetChanged()
    {
        if (isFree()) return;                 // le slider ne pilote le dessin qu'en mode preset
        algoFm.setAlgo (presetNum());
        labelAlgoNum.setText ("ALG " + String (presetNum()), dontSendNotification);
        refreshOpControls();
        repaint();
    }

    void onFreeToggled()
    {
        const bool wantFree = btFree.getToggleState();
        if (wantFree && ! SyAlgo::hasCustomAlgo (elem))
            SyAlgo::seedCustomFromPreset (elem, presetNum());   // amorce depuis le preset courant
        freeValue.setValue (wantFree);        // -> valueChanged -> refreshAll
        if (wantFree) sendFreeAlgoFull();      // ALGNUM=127 + établissement des 6 op (une fois)
        else          sendPreset();            // ré-arme le preset
    }

    void deriveFromPreset()
    {
        SyAlgo::seedCustomFromPreset (elem, presetNum());
        refreshAll();
        sendFreeAlgoFull();
    }

    void onRouteEdited (const char* field, int code)
    {
        const bool entered = ensureFree();
        valueTreeVoice.setProperty (SyAlgo::algoId (elem, field, selOp + 1), jmax (0, code), nullptr);
        afterEdit (entered);
    }

    void onAccEdited()
    {
        const bool entered = ensureFree();
        const int item = cbAcc.getSelectedId();   // 1=Init 2=Sum 3=Keep 4=Reset
        const int a0 = (item == 1 || item == 2) ? 1 : 0;
        const int a1 = (item == 2 || item == 3) ? 1 : 0;
        valueTreeVoice.setProperty (SyAlgo::algoId (elem, "ACCA", selOp + 1), a0, nullptr);
        valueTreeVoice.setProperty (SyAlgo::algoId (elem, "ACCB", selOp + 1), a1, nullptr);
        afterEdit (entered);
    }

    // Bascule en free au 1er édit d'un code (en amorçant depuis le preset si vierge).
    // Retourne TRUE si on vient JUSTE de basculer (-> il faudra un établissement complet).
    bool ensureFree()
    {
        if (isFree()) return false;
        if (! SyAlgo::hasCustomAlgo (elem))
            SyAlgo::seedCustomFromPreset (elem, presetNum());
        freeValue.setValue (true);
        return true;
    }

    // Après une édition : redraw + envoi MINIMAL. À l'ENTRÉE en free -> établissement complet
    // (ALGNUM=127 + 6 op, une seule fois) ; ensuite -> SEULEMENT l'op édité (1-2 octets).
    // Évite le flood « 19 messages + ALGNUM à chaque clic » qui re-déclenchait le recalcul
    // d'algo du synthé et le faisait râler.
    void afterEdit (bool enteredFree)
    {
        algoFm.setCustomAlgo (SyAlgo::buildCustomAlgo (elem));
        repaint();
        if (enteredFree) sendFreeAlgoFull();
        else             sendOneOp (selOp);
    }

    // Envoi 9-octets (param-change SY77). V1=[7], V2=[8] (octets PACKÉS pour le free-algo).
    void send9 (int group, int addrHi, int param, int v1, int v2)
    {
        juce::uint8 b[9] = { 0x43, 0x10, 0x34, (juce::uint8) group, (juce::uint8) addrHi, 0x00,
                             (juce::uint8) param, (juce::uint8) (v1 & 0x7F), (juce::uint8) (v2 & 0x7F) };
        algoSender.sendParam9 ("/SYSEX", b);
    }

    // Groupe param-change de l'opérateur (VÉRIFIÉ HW, cf. Oscillator.h FINE/COARSE) :
    // OP1=0x56 … OP6=0x06. op = 0..5 (op 0 = OP1).
    static int opGroup (int op) { static const int g[6] = { 0x56,0x46,0x36,0x26,0x16,0x06 }; return g[op]; }

    // Sortie du mode free : ré-arme le preset (ALGNUM = preset-1, 0-indexé).
    void sendPreset()
    {
        send9 (0x05, (elem - 1) << 5, 0x00, 0x00, presetNum() - 1);
    }

    // Routage d'UN opérateur (0x13 ALGSRC + 0x14 ALGDST/OAC). Pas d'ALGNUM, pas de SHIFT :
    // c'est l'envoi par édition (minimal). Adresses/packing = spec OCR Table 1-7 (sysex-specialist) :
    //   0x13 : V1=ALGSRC1 b0 ; V2=((ALGSRC1>>1)&7)<<4 | (ALGSRC0&0xF)
    //   0x14 : V2=(OACSRC1&1)<<4 | (OACSRC0&3)<<2 | (ALGDST&3)
    // 🟡 reste à valider par dump : la légende des codes d'entrée 0..10 (cf. AlgoModel.h).
    void sendOneOp (int op)
    {
        if (! isFree()) return;
        const int aH = (elem - 1) << 5, g = opGroup (op);
        const SyDraw::AlgoDef def = SyAlgo::buildCustomAlgo (elem);
        const int in0 = def.in0[op] & 0x0F, in1 = def.in1[op] & 0x0F;
        const int dst = def.outd[op] & 0x03;
        const int oac0 = def.acc0[op] & 0x03, oac1 = def.acc1[op] & 0x01;
        send9 (g, aH, 0x13, (in1 & 1), (((in1 >> 1) & 7) << 4) | in0);
        send9 (g, aH, 0x14, 0x00, ((oac1 & 1) << 4) | ((oac0 & 3) << 2) | dst);
    }

    // Établissement COMPLET du routage libre : ALGNUM=127 (le synthé recalcule -> AVANT les
    // routages) puis 0x13/0x14/0x15 pour les 6 op. Émis UNE fois, à l'entrée en free / dérive.
    void sendFreeAlgoFull()
    {
        if (! isFree()) return;
        const int aH = (elem - 1) << 5;
        const SyDraw::AlgoDef def = SyAlgo::buildCustomAlgo (elem);

        send9 (0x05, aH, 0x00, 0x00, 127);   // ALGNUM = 127 (free)

        for (int op = 0; op < 6; ++op)        // op = 0 -> OP1
        {
            const int g   = opGroup (op);
            const int in0 = def.in0[op] & 0x0F, in1 = def.in1[op] & 0x0F;
            const int dst = def.outd[op] & 0x03;
            const int sh0 = (int) valueTreeVoice.getProperty (SyAlgo::algoId (elem, "SHA", op + 1), 0) & 7;
            const int sh1 = (int) valueTreeVoice.getProperty (SyAlgo::algoId (elem, "SHB", op + 1), 0) & 7;
            const int oac0 = def.acc0[op] & 0x03, oac1 = def.acc1[op] & 0x01;

            send9 (g, aH, 0x13, (in1 & 1), (((in1 >> 1) & 7) << 4) | in0);
            send9 (g, aH, 0x14, 0x00, ((oac1 & 1) << 4) | ((oac0 & 3) << 2) | dst);
            send9 (g, aH, 0x15, 0x00, (sh0 << 3) | sh1);
        }
    }

    void valueChanged (Value& v) override
    {
        if (v.refersToSameSourceAs (freeValue)) { refreshAll(); return; }
        // sinon = valueObject du slider (chargement de voix) -> redraw preset.
        if (! isFree()) onPresetChanged();
    }

    AlgoDraw   algoFm;
    MidiSlider sliderAlgo;
    TextButton btFree { "Free" }, btDerive { "Deriver" };
    OwnedArray<TextButton> opTab;
    ComboBox   cbIn0, cbIn1, cbDst, cbAcc;
    Label labelAlgo { "", TRANS ("AFM Algorithm") }, labelAlgoNum { "", "ALG 1" };
    // Entrées FM 1-indexées (In 1 / In 2), sortie = registre cible, Accum = accumulateur, Niveau = TL.
    Label lblOp { "", "Op" }, lblIn0 { "", "In 1" }, lblIn1 { "", "In 2" },
          lblDst { "", "Sortie" }, lblAcc { "", "Accum" }, lblLvl { "", "Niveau" };
    Label lblOpHdr { "", "Operateur 1" };   // titre de section « ce que vous editez »
    MidiSlider sliderLevel;                 // niveau (TL) de l'op selectionne ; autoSend OFF
                                            // (l'envoi 0x1B passe par le slider Level du detail).
    Value freeValue;
    SysexBusSender algoSender;   // envoi ALGNUM + routage free (cf. send9/sendFreeAlgo)
    UndoManager* undo = nullptr; // pour referTo du niveau (sliderLevel) au changement d'op
    int  elem = 1;
    int  selOp = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AlgoEditorView)
};
