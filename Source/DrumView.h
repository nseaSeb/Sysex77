/*
  DrumView.h — vue dediee aux voix DRUM SET (type 10).

  Tableau des 61 notes (C1..C6) : onde, volume, tune, note shift, pan, sorties, groupe alterne
  (lecture, comparaison). ETAPE 2 : panneau detail EDITABLE pour la note selectionnee, envoi des
  param-change group 0x04 (Table 1-5, T2 = note) via le bus /SYSEX -> enregistres par l'historique
  (undo/A-B) comme tout widget. Envoi seulement sur geste utilisateur. L'etat vit dans SyDrum::Kit (membre de cette vue, toujours vivante dans
  VoicePage) alimente par valueSysexIn : chargement d'un dump (rejeu des params group 0x04) ET
  param-change recus du SY77. Pas d'editeur unique « retargete » (piege des vues uniques).
  Textuellement inclus par Voice.h (contexte : valueSysexIn, SYPal, SYModel, awmWaveName).
*/
#pragma once

/** Ecoute d'une note (bouton Play) : branche par MidiDemo sur son clavier a l'ecran (meme canal
    de jeu, meme sortie, touche allumee). Vide = pas d'ecoute. */
inline std::function<void (int note, bool on)>& drumAuditionHook()
{
    static std::function<void (int, bool)> f;
    return f;
}

class DrumView : public Component, private Value::Listener, private TableListBoxModel
{
public:
    enum Col { cNote = 1, cWave, cVol, cTune, cShift, cPan, cOut, cAlt };

    DrumView()
    {
        addAndMakeVisible (table);
        table.setModel (this);
        table.setRowHeight (22);
        table.setHeaderHeight (24);
        table.setColour (ListBox::backgroundColourId, Colours::transparentBlack);
        auto& h = table.getHeader();
        const int f = TableHeaderComponent::visible | TableHeaderComponent::resizable;
        h.addColumn (TRANS ("Note"),   cNote,  56,  36, 90,  f);
        h.addColumn (TRANS ("Wave"),   cWave,  210, 100, 400, f);
        h.addColumn (TRANS ("Volume"), cVol,   130, 56, 260,  f);
        h.addColumn (TRANS ("Tune"),   cTune,  110, 56, 220,  f);
        h.addColumn (TRANS ("Shift"),  cShift, 110, 56, 220,  f);
        h.addColumn (TRANS ("Pan"),    cPan,   110, 56, 220,  f);
        h.addColumn (TRANS ("Out"),    cOut,   48,  34, 80,   f);
        h.addColumn (TRANS ("Alt"),    cAlt,   44,  32, 80,   f);
        h.setStretchToFitActive (true);
        valueSysexIn.addListener (this);
        buildDetail();
    }
    ~DrumView() override { valueSysexIn.removeListener (this); }

    const SyDrum::Kit& kit() const { return drumKit; }
    void clearKit() { drumKit = {}; refresh(); }
    /** A/B (A affiche) : edition bloquee, tableau toujours defilable. */
    void setEditLocked (bool locked) { detail.setEnabled (! locked); }

    void paint (Graphics& g) override
    {
        g.fillAll (SYColBackground);
        g.setColour (SYPal.textMuted);
        g.setFont (Font (FontOptions (12.5f)));
        g.drawText (TRANS ("DRUM SET - select a key to edit it"),
                    getLocalBounds().removeFromTop (titleH).reduced (6, 0), Justification::centredLeft);
        if (! drumKit.loaded)
            g.drawText (TRANS ("Open a drum voice from the library to see its 61 keys."),
                        getLocalBounds().withTrimmedTop (titleH + 30), Justification::centredTop);
    }
    void resized() override
    {
        auto r = getLocalBounds().withTrimmedTop (titleH);
        detail.setBounds (r.removeFromBottom (detailH));
        layoutDetail();
        table.setBounds (r.withTrimmedBottom (4));
        // Colonnes etirees ; sous la somme des minimums (~460 px) on defile plutot que couper.
        table.getViewport()->setScrollBarsShown (true, getWidth() < minTableW);
    }

private:
    static constexpr int titleH = 24, detailH = 104, minTableW = 36 + 100 + 4 * 56 + 34 + 32 + 20;

    void refresh()
    {
        for (int k = 0; k < SyDrum::numNotes; ++k)   // noms mis en cache (pas de recherche par paint)
            waveNames[k] = awmWaveName (SyDrum::wave (drumKit.raw[k]), SYModel == 3, SyDrum::source (drumKit.raw[k]));
        table.updateContent();
        table.repaint();
        detail.setVisible (drumKit.loaded);
        if (drumKit.loaded && table.getSelectedRow() < 0)
            table.selectRow (selRow);   // -> selectedRowsChanged -> syncDetail
        syncDetail();
        repaint();
    }

    //==========================================================================
    // Panneau detail (note selectionnee)
    struct Detail : Component
    {
        void paint (Graphics& g) override
        {
            g.setColour (SYPal.surface.withAlpha (0.6f));
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
            g.setColour (SYPal.panelBorder);
            g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
            g.setColour (SYPal.textMuted);
            g.setFont (Font (FontOptions (11.0f)));
            for (auto& t : tags) g.drawText (t.text, t.area, Justification::centredLeft);
        }
        struct Tag { String text; Rectangle<int> area; };
        Array<Tag> tags;
    };

    void buildDetail()
    {
        addChildComponent (detail);
        noteLabel.setFont (Font (FontOptions (20.0f)).boldened());
        noteLabel.setColour (Label::textColourId, SYPal.accent);
        for (auto* c : std::initializer_list<Component*> { &noteLabel, &playBt, &srcCombo, &waveCombo,
                                                           &volS, &tuneS, &shiftS, &panS, &out1Bt, &out2Bt, &altBt })
            detail.addAndMakeVisible (c);

        playBt.setTooltip (TRANS ("Hold to hear this key"));
        playBt.onStateChange = [this]
        {
            const bool down = playBt.isDown();
            if (down == playing) return;
            playing = down;
            if (auto& f = drumAuditionHook()) f (SyDrum::firstNote + selRow, down);
        };

        srcCombo.addItem (TRANS ("Internal"), 1);
        srcCombo.addItem (TRANS ("Card"), 2);
        srcCombo.addItem (TRANS ("Source 2"), 3);   // WSRC 0~2 (Table 1-5) ; sens de 2 non confirme
        srcCombo.onChange = [this]
        {
            send (0x01, 0, srcCombo.getSelectedId() - 1);
            fillWaveCombo();
            syncDetail();
        };
        waveCombo.onChange = [this]
        {
            const int w = waveCombo.getSelectedId() - 1;
            if (w >= 0) send (0x02, (w >> 7) & 0x7F, w & 0x7F);
        };

        auto setupBar = [this] (Slider& s, int lo, int hi, int n2, int offset, bool sign)
        {
            s.setSliderStyle (Slider::LinearBar);
            s.setRange (lo, hi, 1);
            if (sign) s.textFromValueFunction = [] (double v) { return v > 0 ? "+" + String ((int) v) : String ((int) v); };
            s.setDoubleClickReturnValue (true, sign ? 0.0 : 127.0);
            s.onValueChange = [this, &s, n2, offset] { send (n2, 0, jlimit (0, 127, (int) s.getValue() + offset)); };
        };
        setupBar (volS,   0,   127, 0x03, 0,  false);
        setupBar (tuneS,  -64, 63,  0x04, 64, true);
        setupBar (shiftS, -48, 36,  0x05, 64, true);
        setupBar (panS,   -31, 31,  0x06, 31, true);

        for (auto* b : { &out1Bt, &out2Bt, &altBt })
        {
            b->setClickingTogglesState (true);
            b->onClick = [this] { sendPacked(); };
        }
        out1Bt.setTooltip (TRANS ("Output 1"));
        out2Bt.setTooltip (TRANS ("Output 2"));
        altBt.setTooltip (TRANS ("Alternate group: keys of the group cut each other"));
    }

    void layoutDetail()
    {
        auto r = detail.getLocalBounds().reduced (8, 6);
        detail.tags.clear();
        auto row1 = r.removeFromTop (30);
        noteLabel.setBounds (row1.removeFromLeft (64));
        playBt.setBounds (row1.removeFromLeft (64).reduced (2, 2));
        row1.removeFromLeft (8);
        srcCombo.setBounds (row1.removeFromLeft (jmin (120, row1.getWidth() / 3)).reduced (2, 2));
        waveCombo.setBounds (row1.reduced (2, 2));

        r.removeFromTop (4);
        auto row2 = r.removeFromTop (r.getHeight());
        const int btW = 44, nBars = 4;
        auto btArea = row2.removeFromRight (jmin (3 * btW + 8, row2.getWidth() / 3));
        const int bw = row2.getWidth() / nBars;
        Slider* bars[nBars] = { &volS, &tuneS, &shiftS, &panS };
        const char* names[nBars] = { "Volume", "Tune", "Note shift", "Pan" };
        for (int i = 0; i < nBars; ++i)
        {
            auto c = row2.removeFromLeft (bw).reduced (3, 0);
            detail.tags.add ({ TRANS (names[i]), c.removeFromTop (16) });
            bars[i]->setBounds (c.withSizeKeepingCentre (c.getWidth(), jmin (26, c.getHeight())));
        }
        detail.tags.add ({ TRANS ("Out / Alt"), btArea.removeFromTop (16).withTrimmedLeft (4) });
        auto bc = btArea.withSizeKeepingCentre (btArea.getWidth(), jmin (26, btArea.getHeight()));
        const int w = bc.getWidth() / 3;
        out1Bt.setBounds (bc.removeFromLeft (w).reduced (2, 0));
        out2Bt.setBounds (bc.removeFromLeft (w).reduced (2, 0));
        altBt.setBounds (bc.reduced (2, 0));
        detail.repaint();
    }

    void fillWaveCombo()
    {
        const int src = srcCombo.getSelectedId() - 1;
        if (src == waveListSrc && waveCombo.getNumItems() > 0) return;
        waveListSrc = src;
        waveCombo.clear (dontSendNotification);
        const bool sy99 = SYModel == 3;
        for (int i = 0; i < 256; ++i)
        {
            const auto n = awmWaveName (i, sy99, src);
            if (src == 0 && n.isEmpty()) break;   // fin de la liste interne
            if (src != 0 && i >= 128) break;
            waveCombo.addItem ((src == 0 ? "P" : "C") + String (i + 1).paddedLeft ('0', 3) + "  " + n, i + 1);   // affichage 1-indexe (ecran SY77), fil 0-indexe
        }
    }

    /** Recopie l'etat de la note selectionnee dans les controles, SANS envoi. */
    void syncDetail()
    {
        if (! drumKit.loaded) return;
        const auto* r = drumKit.raw[selRow];
        noteLabel.setText (sy77NoteName (SyDrum::firstNote + selRow), dontSendNotification);
        srcCombo.setSelectedId (jlimit (0, 2, SyDrum::source (r)) + 1, dontSendNotification);
        fillWaveCombo();
        const int w = SyDrum::wave (r);
        if (waveCombo.indexOfItemId (w + 1) < 0)
            waveCombo.addItem ("#" + String (w + 1), w + 1);   // hors liste connue : affiche le brut
        waveCombo.setSelectedId (w + 1, dontSendNotification);
        volS.setValue   (SyDrum::volume (r), dontSendNotification);
        tuneS.setValue  (SyDrum::tune (r),   dontSendNotification);
        shiftS.setValue (SyDrum::shift (r),  dontSendNotification);
        panS.setValue   (SyDrum::pan (r),    dontSendNotification);
        out1Bt.setToggleState ((r[0] & 0x01) != 0, dontSendNotification);
        out2Bt.setToggleState ((r[0] & 0x02) != 0, dontSendNotification);
        altBt.setToggleState  (SyDrum::alternate (r), dontSendNotification);
    }

    /** Param-change d'une note (geste utilisateur) : envoi bus /SYSEX + etat local. */
    void send (int n2, int v1, int v2)
    {
        if (! drumKit.loaded) return;
        const int note = SyDrum::firstNote + selRow;
        const uint8 b[9] = { 0x43, 0x10, 0x34, 0x04, (uint8) note, 0x00, (uint8) n2, (uint8) v1, (uint8) v2 };
        sender.sendParam9 ("/SYSEX", b);   // device reecrit au choke-point (MidiSysex.h)
        drumKit.apply (0x04, note, 0, n2, v1, v2);
        if (n2 <= 0x02) waveNames[selRow] = awmWaveName (SyDrum::wave (drumKit.raw[selRow]), SYModel == 3,
                                                         SyDrum::source (drumKit.raw[selRow]));
        table.repaintRow (selRow);
    }

    /** N2 = 0 : octet PACKE (ALTGRP b6 | OUT2 b1 | OUT1 b0) recompose en entier, les autres
        bits de l'octet courant conserves. */
    void sendPacked()
    {
        send (0x00, 0, SyDrum::packOutAlt (drumKit.raw[selRow][0], out1Bt.getToggleState(),
                                           out2Bt.getToggleState(), altBt.getToggleState()));
    }

    void selectedRowsChanged (int row) override
    {
        if (row < 0 || row >= SyDrum::numNotes) return;
        if (playing) { if (auto& f = drumAuditionHook()) f (SyDrum::firstNote + selRow, false); playing = false; }
        selRow = row;
        syncDetail();
    }

    void valueChanged (Value& v) override
    {
        if (! v.refersToSameSourceAs (valueSysexIn)) return;
        const var a = v.getValue();
        if (! a.isArray() || a.size() < 6) return;
        if (drumKit.apply ((int) a[0], (int) a[1], (int) a[2], (int) a[3], (int) a[4], (int) a[5])
            && ! pendingRepaint)
        {
            // 427 params arrivent d'un bloc au chargement : un seul rafraichissement differe.
            pendingRepaint = true;
            SafePointer<DrumView> self (this);
            MessageManager::callAsync ([self] { if (self) { self->pendingRepaint = false; self->refresh(); } });
        }
    }

    // --- TableListBoxModel
    int getNumRows() override { return drumKit.loaded ? SyDrum::numNotes : 0; }

    void paintRowBackground (Graphics& g, int row, int, int, bool selected) override
    {
        if (selected)           g.fillAll (SYPal.accent.withAlpha (0.18f));
        else if (row % 12 == 0) g.fillAll (SYPal.surfaceAlt.withAlpha (0.55f));   // repere des C
        else if (row & 1)       g.fillAll (SYPal.surface.withAlpha (0.35f));
    }

    void paintCell (Graphics& g, int row, int col, int w, int h, bool) override
    {
        if (row < 0 || row >= SyDrum::numNotes) return;
        const auto* r = drumKit.raw[row];
        const Rectangle<int> cell (0, 0, w, h);
        g.setFont (Font (FontOptions (13.0f)));
        g.setColour (SYPal.textPrimary);
        switch (col)
        {
            case cNote:
                g.drawText (sy77NoteName (SyDrum::firstNote + row), cell.reduced (6, 0), Justification::centredLeft);
                break;
            case cWave:
            {
                const int wv = SyDrum::wave (r), src = SyDrum::source (r);
                const auto& name = waveNames[row];
                g.setColour (SYPal.textMuted);
                g.drawText ((src == 0 ? "P" : "C") + String (wv + 1).paddedLeft ('0', 3),   // SY77 : 1-indexe
                            cell.reduced (6, 0), Justification::centredLeft);
                g.setColour (SYPal.textPrimary);
                g.drawText (name.isEmpty() ? String ("?") : name,
                            cell.withTrimmedLeft (48).reduced (2, 0), Justification::centredLeft, true);
                break;
            }
            case cVol:   bar (g, cell, SyDrum::volume (r), 0, 127, false); break;
            case cTune:  bar (g, cell, SyDrum::tune (r),  -64, 63, true);  break;
            case cShift: bar (g, cell, SyDrum::shift (r), -48, 36, true);  break;
            case cPan:   bar (g, cell, SyDrum::pan (r),   -31, 31, true);  break;
            case cOut:   g.drawText (SyDrum::outputs (r), cell, Justification::centred); break;
            case cAlt:
                if (SyDrum::alternate (r))
                {
                    g.setColour (SYPal.accent);
                    g.drawText ("ALT", cell, Justification::centred);
                }
                break;
            default: break;
        }
    }

    /** Barre de valeur (lecture seule) : remplissage depuis 0 (bipolaire = depuis le zero
        signe, place a sa vraie position quand la plage est asymetrique) + valeur ecrite. */
    static void bar (Graphics& g, Rectangle<int> cell, int v, int lo, int hi, bool bipolar)
    {
        auto r = cell.reduced (4, 4).toFloat();
        g.setColour (SYPal.surfaceAlt);
        g.fillRoundedRectangle (r, 3.0f);
        auto xOf = [&] (int x) { return r.getX() + r.getWidth() * (float) (jlimit (lo, hi, x) - lo) / (float) (hi - lo); };
        const float x0 = bipolar ? xOf (0) : r.getX(), x1 = xOf (v);
        g.setColour (SYPal.accent.withAlpha (0.75f));
        g.fillRect (Rectangle<float> (jmin (x0, x1), r.getY(), std::abs (x1 - x0), r.getHeight()));
        if (bipolar)
        {
            g.setColour (SYPal.panelBorder);
            g.drawVerticalLine ((int) x0, r.getY(), r.getBottom());
        }
        g.setColour (SYPal.textPrimary);
        g.setFont (Font (FontOptions (11.5f)));
        g.drawText (bipolar && v > 0 ? "+" + String (v) : String (v), r.toNearestInt(), Justification::centred);
    }

    SyDrum::Kit drumKit;
    Detail detail;
    Label noteLabel;
    TextButton playBt { "Play" }, out1Bt { "1" }, out2Bt { "2" }, altBt { "ALT" };
    ComboBox srcCombo, waveCombo;
    Slider volS, tuneS, shiftS, panS;
    SysexBusSender sender;
    int selRow = 0, waveListSrc = -1;
    bool playing = false;
    String waveNames[SyDrum::numNotes];
    TableListBox table;
    bool pendingRepaint = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumView)
};
