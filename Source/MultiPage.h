/*
  MultiPage.h — onglet MULTI, ETAPE 1 : LECTURE SEULE.

  Affiche un Multi SY77 (bloc bulk « LM  8101MU ») : nom, effect mode, 16 parties (voix,
  volume, tune, shift, pan, sorties, switch). Sources : un Multi d'une banque de la librairie
  (menu), ou un Multi memorise demande au SY77 (dump request = LECTURE, sur clic). Les
  param-change Multi recus (groupes 0x00/0x01) arrivent sur valueMultiIn (jamais sur
  valueSysexIn) et mettent l'etat a jour. AUCUN param-change ni bulk n'est envoye.
  Inclus textuellement par MidiDemo.h (contexte : appDirPath, valueMultiIn, valueMultiBulk, SYPal).
*/
#pragma once

class MultiPage : public Component, private Value::Listener, private TableListBoxModel
{
public:
    enum Col { cPart = 1, cVoice, cVol, cTune, cShift, cPan, cOut, cSw };

    MultiPage()
    {
        addAndMakeVisible (libraryBt);
        libraryBt.onClick = [this] { showLibraryMenu(); };
        addAndMakeVisible (slotCombo);
        for (int i = 0; i < 16; ++i) slotCombo.addItem ("INT " + String (i + 1), i + 1);
        slotCombo.setSelectedId (1, dontSendNotification);
        addAndMakeVisible (requestBt);
        requestBt.setTooltip (TRANS ("Ask the SY77 to send this stored multi (read only, nothing is written to the synth)."));
        requestBt.onClick = [this] { sender.send (adresseOscRequestMulti, slotCombo.getSelectedId() - 1); };

        addAndMakeVisible (table);
        table.setModel (this);
        table.setRowHeight (24);
        table.setHeaderHeight (24);
        table.setColour (ListBox::backgroundColourId, Colours::transparentBlack);
        auto& h = table.getHeader();
        const int f = TableHeaderComponent::visible | TableHeaderComponent::resizable;
        h.addColumn (TRANS ("Part"),   cPart,  50,  36, 80,  f);
        h.addColumn (TRANS ("Voice"),  cVoice, 160, 80, 300, f);
        h.addColumn (TRANS ("Volume"), cVol,   140, 56, 260, f);
        h.addColumn (TRANS ("Tune"),   cTune,  110, 56, 220, f);
        h.addColumn (TRANS ("Shift"),  cShift, 110, 56, 220, f);
        h.addColumn (TRANS ("Pan"),    cPan,   110, 56, 220, f);
        h.addColumn (TRANS ("Out"),    cOut,   52,  34, 80,  f);
        h.addColumn ("Sw",             cSw,    44,  30, 80,  f);
        h.setStretchToFitActive (true);

        valueMultiIn.addListener (this);
        valueMultiBulk.addListener (this);
    }
    ~MultiPage() override
    {
        valueMultiIn.removeListener (this);
        valueMultiBulk.removeListener (this);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (SYColBackground);
        auto top = getLocalBounds().reduced (10, 0).removeFromTop (headerH);
        g.setColour (SYPal.textPrimary);
        g.setFont (Font (FontOptions (18.0f)).boldened());
        g.drawText (multi.loaded ? (multi.name.isEmpty() ? String ("(no name)") : multi.name) : TRANS ("No multi loaded"),
                    top.removeFromTop (30).withTrimmedRight (440), Justification::centredLeft);
        g.setColour (SYPal.textMuted);
        g.setFont (Font (FontOptions (12.5f)));
        String info = TRANS ("Read only - nothing is sent to the synth (editing comes next).");
        if (multi.loaded)
            info = source + "   |   " + TRANS ("Effect mode") + " " + String ((int) multi.fx[0]) + "   |   " + info;
        g.drawText (info, top, Justification::centredLeft);
        if (! multi.loaded)
            g.drawText (TRANS ("Open a multi from the library, or request a stored multi from the SY77."),
                        getLocalBounds().withTrimmedTop (headerH + 40), Justification::centredTop);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10, 6);
        auto top = r.removeFromTop (headerH).removeFromTop (30);
        requestBt.setBounds (top.removeFromRight (150).reduced (2));
        slotCombo.setBounds (top.removeFromRight (100).reduced (2));
        top.removeFromRight (10);
        libraryBt.setBounds (top.removeFromRight (170).reduced (2));
        table.setBounds (r);
        table.getViewport()->setScrollBarsShown (true, getWidth() < 520);
    }

private:
    static constexpr int headerH = 56;

    void setMulti (const SyMulti::Multi& m, const String& from)
    {
        multi = m;
        source = from;
        table.updateContent();
        table.repaint();
        repaint();
    }

    void valueChanged (Value& v) override
    {
        if (v.refersToSameSourceAs (valueMultiBulk))
        {
            const var data = valueMultiBulk.getValue();   // garder le var : getBinaryData() pointe DEDANS
            if (auto* mb = data.getBinaryData())
            {
                auto m = SyMulti::parse ((const uint8*) mb->getData(), (int) mb->getSize());
                if (m.loaded)
                    setMulti (m, TRANS ("Received from the SY77") + " ("
                                 + (m.memType == 0x7F ? String ("EDIT") : (m.memType == 0 ? "INT " : "PRE ") + String (m.memNumber + 1)) + ")");
            }
            return;
        }
        if (v.refersToSameSourceAs (valueMultiIn))
        {
            const var a = v.getValue();
            if (a.isArray() && a.size() >= 6
                && multi.apply ((int) a[0], (int) a[1], (int) a[2], (int) a[3], (int) a[4], (int) a[5]))
            {
                table.repaint();
                repaint();
            }
        }
    }

    /** Multis de toutes les banques de la librairie (scan a la demande), groupes par banque. */
    void showLibraryMenu()
    {
        libMultis.clear();
        libLabels.clear();
        Array<File> files;
        appDirPath.findChildFiles (files, File::findFiles, true, "*.syx");
        files.sort();
        PopupMenu m;
        for (auto& f : files)
        {
            MemoryBlock mb;
            if (! f.loadFileAsData (mb)) continue;
            const auto blocks = SyMulti::multisIn ((const uint8*) mb.getData(), mb.getSize());
            if (blocks.isEmpty()) continue;
            PopupMenu sub;
            for (auto& b : blocks)
            {
                const auto mu = SyMulti::parse ((const uint8*) b.getData(), (int) b.getSize());
                const int idx = libMultis.size();
                libMultis.add (b);
                libLabels.add (f.getFileNameWithoutExtension() + " / " + mu.name);
                sub.addItem ((mu.memType == 0 ? "INT " : "PRE ") + String (mu.memNumber + 1) + "  " + mu.name,
                             [this, idx]
                             {
                                 const auto& blk = libMultis.getReference (idx);
                                 setMulti (SyMulti::parse ((const uint8*) blk.getData(), (int) blk.getSize()), libLabels[idx]);
                             });
            }
            m.addSubMenu (f.getFileNameWithoutExtension(), sub);
        }
        if (m.getNumItems() == 0) m.addItem (TRANS ("No multi in the library"), false, false, [] {});
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&libraryBt));
    }

    // --- TableListBoxModel
    int getNumRows() override { return multi.loaded ? SyMulti::kParts : 0; }

    void paintRowBackground (Graphics& g, int row, int, int, bool selected) override
    {
        if (selected)     g.fillAll (SYPal.accent.withAlpha (0.18f));
        else if (row & 1) g.fillAll (SYPal.surface.withAlpha (0.35f));
    }

    void paintCell (Graphics& g, int row, int col, int w, int h, bool) override
    {
        if (row < 0 || row >= SyMulti::kParts) return;
        const auto& p = multi.part[row];
        const Rectangle<int> cell (0, 0, w, h);
        g.setFont (Font (FontOptions (13.0f)));
        g.setColour (SYPal.textPrimary);
        switch (col)
        {
            case cPart:  g.drawText (String (row + 1), cell.reduced (6, 0), Justification::centredLeft); break;
            case cVoice: g.drawText (SyMulti::voiceLabel (p), cell.reduced (6, 0), Justification::centredLeft); break;
            case cVol:   bar (g, cell, SyMulti::volume (p), 0, 127, false); break;
            case cTune:  bar (g, cell, SyMulti::tune (p), -64, 63, true); break;
            case cShift: bar (g, cell, SyMulti::shift (p), -64, 63, true); break;
            case cPan:
                if (SyMulti::panFromVoice (p))
                {
                    g.setColour (SYPal.textMuted);
                    g.drawText (TRANS ("voice"), cell, Justification::centred);
                }
                else bar (g, cell, SyMulti::pan (p), -31, 31, true);
                break;
            case cOut: g.drawText (SyDrum::outputs (p.raw), cell, Justification::centred); break;   // meme codage b0/b1
            case cSw: g.drawText (SyMulti::ofvcsw (p) ? "1" : "0", cell, Justification::centred); break;
            default: break;
        }
    }

    static void bar (Graphics& g, Rectangle<int> cell, int v, int lo, int hi, bool bipolar)
    {
        auto r = cell.reduced (4, 4).toFloat();
        g.setColour (SYPal.surfaceAlt);
        g.fillRoundedRectangle (r, 3.0f);
        auto xOf = [&] (int x) { return r.getX() + r.getWidth() * (float) (jlimit (lo, hi, x) - lo) / (float) (hi - lo); };
        const float x0 = bipolar ? xOf (0) : r.getX(), x1 = xOf (v);
        g.setColour (SYPal.accent.withAlpha (0.75f));
        g.fillRect (Rectangle<float> (jmin (x0, x1), r.getY(), std::abs (x1 - x0), r.getHeight()));
        if (bipolar) { g.setColour (SYPal.panelBorder); g.drawVerticalLine ((int) x0, r.getY(), r.getBottom()); }
        g.setColour (SYPal.textPrimary);
        g.setFont (Font (FontOptions (11.5f)));
        g.drawText (bipolar && v > 0 ? "+" + String (v) : String (v), r.toNearestInt(), Justification::centred);
    }

    SyMulti::Multi multi;
    String source;
    TableListBox table;
    TextButton libraryBt { TRANS ("Library multis...") }, requestBt { TRANS ("Request from SY77") };
    ComboBox slotCombo;
    Array<MemoryBlock> libMultis;
    StringArray libLabels;
    SysexBusSender sender;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiPage)
};
