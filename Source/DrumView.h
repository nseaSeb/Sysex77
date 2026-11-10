/*
  DrumView.h — vue dediee aux voix DRUM SET (type 10), ETAPE 1 : LECTURE SEULE.

  Tableau des 61 notes (C1..C6) : onde, volume, tune, note shift, pan, sorties, groupe alterne.
  Aucun envoi au synthe. L'etat vit dans SyDrum::Kit (membre de cette vue, toujours vivante dans
  VoicePage) alimente par valueSysexIn : chargement d'un dump (rejeu des params group 0x04) ET
  param-change recus du SY77. Pas d'editeur unique « retargete » (piege des vues uniques).
  Textuellement inclus par Voice.h (contexte : valueSysexIn, SYPal, SYModel, awmWaveName).
*/
#pragma once

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
    }
    ~DrumView() override { valueSysexIn.removeListener (this); }

    const SyDrum::Kit& kit() const { return drumKit; }
    void clearKit() { drumKit = {}; refresh(); }

    void paint (Graphics& g) override
    {
        g.fillAll (SYColBackground);
        g.setColour (SYPal.textMuted);
        g.setFont (Font (FontOptions (12.5f)));
        g.drawText (TRANS ("DRUM SET - read only (editing comes next)"),
                    getLocalBounds().removeFromTop (titleH).reduced (6, 0), Justification::centredLeft);
        if (! drumKit.loaded)
            g.drawText (TRANS ("Open a drum voice from the library to see its 61 keys."),
                        getLocalBounds().withTrimmedTop (titleH + 30), Justification::centredTop);
    }
    void resized() override
    {
        table.setBounds (getLocalBounds().withTrimmedTop (titleH));
        // Colonnes etirees ; sous la somme des minimums (~460 px) on defile plutot que couper.
        table.getViewport()->setScrollBarsShown (true, getWidth() < minTableW);
    }

private:
    static constexpr int titleH = 24, minTableW = 36 + 100 + 4 * 56 + 34 + 32 + 20;

    void refresh()
    {
        for (int k = 0; k < SyDrum::numNotes; ++k)   // noms mis en cache (pas de recherche par paint)
            waveNames[k] = awmWaveName (SyDrum::wave (drumKit.raw[k]), SYModel == 3, SyDrum::source (drumKit.raw[k]));
        table.updateContent();
        table.repaint();
        repaint();
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
                g.drawText ((src == 0 ? "P" : "C") + String (wv).paddedLeft ('0', 3),
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
    String waveNames[SyDrum::numNotes];
    TableListBox table;
    bool pendingRepaint = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumView)
};
