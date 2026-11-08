/*
  ==============================================================================

    ControllersPage.h
    Ecran Controleurs (Voice Common, group 0x02, Table 1-3 N2 0x28..0x3D).
    Regroupement = celui du SY77 (Controller Set F1 Pitch Bend / F2 Modulation /
    F3 Pan / F4 Other) + bloc Voice (micro tuning, random pitch, portamento).
    Adresses + plages : dictionnaire SyParamTable (ui 7140..7161). Chargement :
    voiceBlobToParams (d[32 + N2]). Envoi uniquement sur geste utilisateur.

  ==============================================================================
*/

#pragma once

class ControllersPage : public juce::Component
{
public:
    ControllersPage()
    {
        addAndMakeVisible (viewport);
        viewport.setViewedComponent (&content, false);
        viewport.setScrollBarsShown (true, false);

        // --- Voice
        auto& v = addCard ("Voice");
        addSlider (v, "Random Pitch", 7159);
        addSlider (v, "Micro Tuning", 7158);
        // PORM 0/1 : Fingered / Full time en mono (manuel Swayed) ; ordre filaire A CONFIRMER.
        addCombo  (v, "Portamento", 7160, { "Fingered", "Full time" }, 0);
        addSlider (v, "Porta Time", 7161);
        v.note = TRANS ("Portamento: mono modes only");

        // --- Pitch Bend
        auto& pb = addCard ("Pitch Bend");
        addSlider (pb, "Wheel", 7140);
        addSlider (pb, "After Touch", 7141, 16);   // ATPBR s/m bit 16

        // --- Modulation (assign = n° de CC, 121 = aftertouch)
        auto& m = addCard ("Modulation");
        addAssign (m, "Pitch Mod",  7142, 7143);
        addAssign (m, "Amp Mod",    7144, 7145);
        addAssign (m, "Filter Mod", 7146, 7147);

        // --- Pan (Multi seulement, Table 1-3)
        auto& p = addCard ("Pan");
        addAssign (p, "Pan LFO",  7148, 7149);
        addAssign (p, "Pan Bias", 7152, 7153);
        p.note = TRANS ("Valid only in Multi mode");

        // --- Other
        auto& o = addCard ("Other");
        addAssign (o, "Cutoff Bias", 7150, 7151);
        addAssign (o, "EG Bias",     7154, 7155);
        addAssign (o, "Vol Low Lim", 7156, 7157);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        viewport.setBounds (getLocalBounds());
        layout (viewport.getMaximumVisibleWidth());
    }

private:
    struct Row  { juce::Label* label; juce::Component* a; juce::Component* b; };
    struct Card { juce::GroupComponent* grp; juce::Array<Row> rows; juce::String note; juce::Label* noteLabel = nullptr; };

    Card& addCard (const juce::String& title)
    {
        auto* g = groups.add (new juce::GroupComponent ({}, TRANS (title)));
        content.addAndMakeVisible (g);
        cards.add (new Card { g, {}, {} });
        return *cards.getLast();
    }
    juce::Label* rowLabel (const juce::String& text)
    {
        auto* l = labels.add (new juce::Label ({}, TRANS (text)));
        l->setFont (juce::FontOptions (13.0f));
        content.addAndMakeVisible (l);
        return l;
    }
    template <typename W> static void wire (W& w, int ui)
    {
        const auto a = SyVoice::syTranslate (ui, 0, 0);
        jassert (a.ok);
        int sx[9] = { 0x43, 0x10, 0x34, a.group, a.t2, 0x00, a.n2, 0x00, 0x00 };
        w.setMidiSysex (sx);
    }
    static juce::Identifier propFor (int ui) { return juce::Identifier ("VC" + juce::String (ui)); }

    MidiSlider* makeSlider (int ui, int smBit = 0)
    {
        auto* s = sliders.add (new MidiSlider());
        const auto* e = SyParam::entry (ui);
        jassert (e != nullptr);
        s->setRangeAndRound (e->dispMin, e->dispMax, 0);
        s->setSliderStyle (juce::Slider::LinearHorizontal);   // min<0 force Rotary
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 18);
        if (smBit > 0) s->setSignMagBit (smBit);
        wire (*s, ui);
        s->getValueObject().referTo (valueTreeVoice.getPropertyAsValue (propFor (ui), nullptr));
        content.addAndMakeVisible (s);
        return s;
    }
    MidiCombo* makeCombo (int ui, const juce::StringArray& items, int firstValue)
    {
        auto* c = combos.add (new MidiCombo());
        for (int i = 0; i < items.size(); ++i)
            c->addItem (items[i], firstValue + i + 1);     // id = valeur filaire + 1 (MidiCombo emet id-1)
        wire (*c, ui);
        c->getSelectedIdAsValue().referTo (valueTreeVoice.getPropertyAsValue (propFor (ui), nullptr));
        // Pas de valeur par defaut forcee : vide tant qu'aucune voix n'est chargee (sinon on
        // ecrirait "CC 0" dans la voix sans que le synthe l'ait).
        content.addAndMakeVisible (c);
        return c;
    }
    void addSlider (Card& card, const juce::String& name, int ui, int smBit = 0)
    { card.rows.add ({ rowLabel (name), makeSlider (ui, smBit), nullptr }); }
    void addCombo (Card& card, const juce::String& name, int ui, const juce::StringArray& items, int first)
    { card.rows.add ({ rowLabel (name), makeCombo (ui, items, first), nullptr }); }
    void addAssign (Card& card, const juce::String& name, int uiAssign, int uiRange)
    {
        juce::StringArray items;
        for (int v = 0; v <= 121; ++v) items.add (assignName (v));
        card.rows.add ({ rowLabel (name), makeCombo (uiAssign, items, 0), makeSlider (uiRange) });
    }
    // Device assign 0..120 = n° de CC MIDI, 121 = aftertouch (note Table 1-3).
    static juce::String assignName (int v)
    {
        if (v == 121) return "After Touch";
        juce::String n = "CC " + juce::String (v);
        switch (v)
        {
            case 1:  return n + " Mod Wheel";
            case 2:  return n + " Breath";
            case 4:  return n + " Foot";
            case 7:  return n + " Volume";
            case 11: return n + " Expression";
            case 64: return n + " Sustain";
            default: return n;
        }
    }

    // Grille responsive : 1 a 3 colonnes selon la largeur, hauteur scrollable (Viewport).
    void layout (int width)
    {
        const int pad = 12, gap = 10, rowH = 30, head = 26;
        const int cols = width >= 1100 ? 3 : (width >= 700 ? 2 : 1);
        const int cw = (width - 2 * pad - (cols - 1) * gap) / cols;
        juce::Array<int> colY;
        for (int i = 0; i < cols; ++i) colY.add (pad);
        for (auto* c : cards)
        {
            int col = 0;                                       // colonne la moins haute
            for (int i = 1; i < cols; ++i) if (colY[i] < colY[col]) col = i;
            const int nNote = c->note.isNotEmpty() ? 1 : 0;
            const int h = head + c->rows.size() * rowH + nNote * 20 + 12;
            const int x = pad + col * (cw + gap), y = colY[col];
            c->grp->setBounds (x, y, cw, h);
            auto inner = juce::Rectangle<int> (x + 10, y + head, cw - 20, h - head - 8);
            const int lw = juce::jlimit (70, 110, inner.getWidth() / 4);
            for (auto& r : c->rows)
            {
                auto line = inner.removeFromTop (rowH).reduced (0, 3);
                r.label->setBounds (line.removeFromLeft (lw));
                if (r.b != nullptr)
                {
                    const int aw = juce::jlimit (90, 170, line.getWidth() / 2);
                    r.a->setBounds (line.removeFromLeft (aw));
                    line.removeFromLeft (6);
                    r.b->setBounds (line);
                }
                else
                    r.a->setBounds (line.removeFromLeft (juce::jmin (line.getWidth(), 260)));
            }
            if (nNote)
            {
                if (c->noteLabel == nullptr)
                {
                    c->noteLabel = labels.add (new juce::Label ({}, c->note));
                    c->noteLabel->setFont (juce::FontOptions (11.0f).withStyle ("Italic"));
                    c->noteLabel->setColour (juce::Label::textColourId,
                                             findColour (juce::Label::textColourId).withAlpha (0.6f));
                    content.addAndMakeVisible (c->noteLabel);
                }
                c->noteLabel->setBounds (inner.removeFromTop (20));
            }
            colY.set (col, y + h + gap);
        }
        int maxY = 0;
        for (auto y : colY) maxY = juce::jmax (maxY, y);
        content.setSize (width, maxY + pad);
    }

    juce::Viewport  viewport;
    juce::Component content;
    juce::OwnedArray<Card> cards;
    juce::OwnedArray<juce::GroupComponent> groups;
    juce::OwnedArray<juce::Label> labels;
    juce::OwnedArray<MidiSlider> sliders;
    juce::OwnedArray<MidiCombo> combos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllersPage)
};
