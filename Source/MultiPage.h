/*
  MultiPage.h — onglet MULTI. Etape 1 : affichage ; ETAPE 2 : edition de la partie
  selectionnee (panneau sous le tableau) -> param-change groupe 0x01 / T2 = partie (Table 1-2),
  envoye UNIQUEMENT sur geste utilisateur via le bus /SYSEX (jamais dans l'historique voix :
  groupes 0x00/0x01 exclus, cf. SyVoice::isVoiceGroup). Octet packe N2=0 recompose en gardant
  les bits non edites (bit 2 non documente). Ancien en-tete :

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

        addChildComponent (nameEd);
        nameEd.setInputRestrictions (20);
        nameEd.setFont (Font (FontOptions (17.0f)).boldened());
        nameEd.setTooltip (TRANS ("Multi name - press Return to send it to the SY77"));
        nameEd.onReturnKey = [this] { sendName(); };
        addChildComponent (sendBt);
        sendBt.setTooltip (TRANS ("Send the whole multi to the SY77 edit buffer (no memory is written)"));
        sendBt.onClick = [this] { confirmSendMulti(); };

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
        valueMultiOpen.addListener (this);
        buildDetail();
    }
    ~MultiPage() override
    {
        valueMultiIn.removeListener (this);
        valueMultiBulk.removeListener (this);
        valueMultiOpen.removeListener (this);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (SYColBackground);
        auto top = getLocalBounds().reduced (10, 0).removeFromTop (headerH);
        g.setColour (SYPal.textPrimary);
        g.setFont (Font (FontOptions (18.0f)).boldened());
        if (! multi.loaded)
            g.drawText (TRANS ("No multi loaded"), top.withHeight (30), Justification::centredLeft);
        top.removeFromTop (30);
        g.setColour (SYPal.textMuted);
        g.setFont (Font (FontOptions (12.5f)));
        String info = TRANS ("Select a part to edit it - each change is sent to the SY77.");
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
        top.removeFromRight (10);
        sendBt.setBounds (top.removeFromRight (150).reduced (2));
        nameEd.setBounds (top.removeFromLeft (jmin (260, top.getWidth())).reduced (0, 2));
        detail.setBounds (r.removeFromBottom (detailH));
        layoutDetail();
        table.setBounds (r.withTrimmedBottom (4));
        table.getViewport()->setScrollBarsShown (true, getWidth() < 520);
    }

private:
    static constexpr int headerH = 56, detailH = 100;

    void setMulti (const SyMulti::Multi& m, const String& from, const MemoryBlock& block)
    {
        multi = m;
        source = from;
        baseBlock = block;
        nameEd.setVisible (multi.loaded);
        sendBt.setVisible (multi.loaded);
        nameEd.setText (multi.name, dontSendNotification);
        detail.setVisible (multi.loaded);
        table.updateContent();   // AVANT selectRow (sinon la ligne n'existe pas encore)
        if (multi.loaded && table.getSelectedRow() < 0) table.selectRow (sel);
        syncDetail();
        table.repaint();
        repaint();
    }

    void valueChanged (Value& v) override
    {
        if (v.refersToSameSourceAs (valueMultiOpen))
        {
            const var a = valueMultiOpen.getValue();
            if (a.isArray() && a.size() == 2)
                if (auto* mb = a[1].getBinaryData())
                {
                    auto m = SyMulti::parse ((const uint8*) mb->getData(), (int) mb->getSize());
                    if (m.loaded) setMulti (m, a[0].toString(), *mb);
                }
            return;
        }
        if (v.refersToSameSourceAs (valueMultiBulk))
        {
            const var data = valueMultiBulk.getValue();   // garder le var : getBinaryData() pointe DEDANS
            if (auto* mb = data.getBinaryData())
            {
                auto m = SyMulti::parse ((const uint8*) mb->getData(), (int) mb->getSize());
                if (m.loaded)
                    setMulti (m, TRANS ("Received from the SY77") + " ("
                                 + (m.memType == 0x7F ? String ("EDIT") : (m.memType == 0 ? "INT " : "PRE ") + String (m.memNumber + 1)) + ")", *mb);
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
                syncDetail();
                if (! nameEd.hasKeyboardFocus (true)) nameEd.setText (multi.name, dontSendNotification);
                repaint();
            }
        }
    }

    //==========================================================================
    // Panneau d'edition de la partie selectionnee
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
            for (auto& t : tags) g.drawText (t.first, t.second, Justification::centredLeft);
        }
        Array<std::pair<String, Rectangle<int>>> tags;
    };

    void buildDetail()
    {
        addChildComponent (detail);
        partLabel.setFont (Font (FontOptions (18.0f)).boldened());
        partLabel.setColour (Label::textColourId, SYPal.accent);
        for (auto* c : std::initializer_list<Component*> { &partLabel, &memCombo, &numCombo, &volS, &tuneS, &shiftS,
                                                           &panS, &panVoiceBt, &out1Bt, &out2Bt, &swBt })
            detail.addAndMakeVisible (c);
        for (int i = 0; i < 4; ++i) memCombo.addItem ("M" + String (i), i + 1);   // libelle de banque non confirme
        memCombo.setTooltip (TRANS ("Voice memory code (bank meaning not confirmed yet)"));
        memCombo.onChange = [this] { send (0x01, memCombo.getSelectedId() - 1); };
        for (int n = 0; n < 64; ++n) numCombo.addItem (SyBank::slotName (n), n + 1);
        numCombo.onChange = [this] { send (0x02, numCombo.getSelectedId() - 1); };
        auto bar = [this] (Slider& sl, int lo, int hi, int n2, int off, bool sign)
        {
            sl.setSliderStyle (Slider::LinearBar);
            sl.setRange (lo, hi, 1);
            if (sign) sl.textFromValueFunction = [] (double v) { return v > 0 ? "+" + String ((int) v) : String ((int) v); };
            sl.setDoubleClickReturnValue (true, sign ? 0.0 : 100.0);
            sl.onValueChange = [this, &sl, n2, off] { send (n2, jlimit (0, 127, (int) sl.getValue() + off)); };
        };
        bar (volS, 0, 127, 0x03, 0, false);
        bar (tuneS, -64, 63, 0x04, 64, true);
        bar (shiftS, -64, 63, 0x05, 64, true);
        bar (panS, -31, 31, 0x06, 32, true);
        panVoiceBt.setClickingTogglesState (true);
        panVoiceBt.setTooltip (TRANS ("Use the voice's own pan"));
        panVoiceBt.onClick = [this]
        {
            send (0x06, panVoiceBt.getToggleState() ? 0 : jlimit (1, 63, (int) panS.getValue() + 32));
            panS.setEnabled (! panVoiceBt.getToggleState());
        };
        for (auto* b : { &out1Bt, &out2Bt, &swBt })
        {
            b->setClickingTogglesState (true);
            b->onClick = [this] { sendPacked(); };
        }
        out1Bt.setTooltip (TRANS ("Output 1"));
        out2Bt.setTooltip (TRANS ("Output 2"));
        swBt.setTooltip (TRANS ("Part switch (bit 6, meaning not confirmed: set on the parts in use)"));
    }

    void layoutDetail()
    {
        auto r = detail.getLocalBounds().reduced (8, 6);
        detail.tags.clear();
        auto row1 = r.removeFromTop (28);
        partLabel.setBounds (row1.removeFromLeft (90));
        memCombo.setBounds (row1.removeFromLeft (80).reduced (2));
        numCombo.setBounds (row1.removeFromLeft (90).reduced (2));
        row1.removeFromLeft (10);
        const int bw = jmax (28, row1.getWidth() / 3);
        out1Bt.setBounds (row1.removeFromLeft (jmin (44, bw)).reduced (2));
        out2Bt.setBounds (row1.removeFromLeft (jmin (44, bw)).reduced (2));
        swBt.setBounds (row1.removeFromLeft (jmin (56, bw)).reduced (2));
        r.removeFromTop (4);
        Slider* bars[4] = { &volS, &tuneS, &shiftS, &panS };
        const char* names[4] = { "Volume", "Tune", "Note shift", "Pan" };
        auto row2 = r;
        auto voiceArea = row2.removeFromRight (70);
        const int w = row2.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto c = row2.removeFromLeft (w).reduced (3, 0);
            detail.tags.add ({ TRANS (names[i]), c.removeFromTop (16) });
            bars[i]->setBounds (c.withSizeKeepingCentre (c.getWidth(), jmin (26, c.getHeight())));
        }
        voiceArea.removeFromTop (16);
        panVoiceBt.setBounds (voiceArea.withSizeKeepingCentre (voiceArea.getWidth() - 4, jmin (26, voiceArea.getHeight())));
        detail.repaint();
    }

    /** Nom (Return) : 20 param-change groupe 0x00 / N2 0..19 (MNAM, Table 1-1), sur geste. */
    void sendName()
    {
        if (! multi.loaded) return;
        multi.name = nameEd.getText().substring (0, 20);
        const auto nm = multi.name.paddedRight (' ', 20);
        for (int k = 0; k < 20; ++k)
        {
            const uint8 b[9] = { 0x43, 0x10, 0x34, 0x00, 0x00, 0x00, (uint8) k, 0x00, (uint8) jlimit (32, 126, (int) nm[k]) };
            sender.sendParam9 ("/SYSEX", b);
        }
        repaint();
    }

    /** Envoi du Multi complet vers l'EDIT BUFFER, apres confirmation (jamais automatique). */
    void confirmSendMulti()
    {
        const auto blk = SyMulti::encodeForEditBuffer (multi, baseBlock);
        if (blk.getSize() == 0) return;
        SafePointer<MultiPage> self (this);
        AlertWindow::showOkCancelBox (MessageBoxIconType::QuestionIcon, TRANS ("Send multi"),
            TRANS ("Send this multi to the SY77 edit buffer? It replaces the multi being edited on the synth; no stored multi is overwritten. Bulk Protect must be off.")
                + "\n" + TRANS ("(Not yet checked on hardware.)"),
            TRANS ("Send"), TRANS ("Cancel"), nullptr,
            ModalCallbackFunction::create ([self, blk] (int r)
            {
                if (r == 1 && self != nullptr) self->sender.send (adresseOscSendMulti, blk);
            }));
    }

    /** Partie selectionnee -> controles, SANS envoi. */
    void syncDetail()
    {
        if (! multi.loaded) return;
        const auto& p = multi.part[sel];
        partLabel.setText (TRANS ("Part") + " " + String (sel + 1), dontSendNotification);
        memCombo.setSelectedId (jlimit (0, 3, SyMulti::voiceMem (p)) + 1, dontSendNotification);
        numCombo.setSelectedId (jlimit (0, 63, SyMulti::voiceNum (p)) + 1, dontSendNotification);
        volS.setValue   (SyMulti::volume (p), dontSendNotification);
        tuneS.setValue  (SyMulti::tune (p),   dontSendNotification);
        shiftS.setValue (SyMulti::shift (p),  dontSendNotification);
        panVoiceBt.setToggleState (SyMulti::panFromVoice (p), dontSendNotification);
        panS.setEnabled (! SyMulti::panFromVoice (p));
        if (! SyMulti::panFromVoice (p)) panS.setValue (SyMulti::pan (p), dontSendNotification);
        out1Bt.setToggleState (SyMulti::out1 (p), dontSendNotification);
        out2Bt.setToggleState (SyMulti::out2 (p), dontSendNotification);
        swBt.setToggleState (SyMulti::ofvcsw (p), dontSendNotification);
    }

    /** Param-change d'une partie (geste utilisateur) : bus /SYSEX + etat local. */
    void send (int n2, int v)
    {
        if (! multi.loaded) return;
        const uint8 b[9] = { 0x43, 0x10, 0x34, 0x01, (uint8) sel, 0x00, (uint8) n2, 0x00, (uint8) (v & 0x7F) };
        sender.sendParam9 ("/SYSEX", b);   // device reecrit au choke-point (MidiSysex.h)
        multi.part[sel].raw[n2] = (uint8) (v & 0x7F);
        table.repaintRow (sel);
    }

    void sendPacked()
    {
        send (0x00, SyMulti::packPart (multi.part[sel].raw[0], out1Bt.getToggleState(),
                                       out2Bt.getToggleState(), swBt.getToggleState()));
    }

    void selectedRowsChanged (int row) override
    {
        if (row < 0 || row >= SyMulti::kParts) return;
        sel = row;
        syncDetail();
    }

    /** Multis de toutes les banques de la librairie (scan a la demande), groupes par banque. */
    void showLibraryMenu()
    {
        Array<File> files;
        appDirPath.findChildFiles (files, File::findFiles, true, "*.syx");
        files.sort();
        // Cache : on ne relit les fichiers que si la liste / tailles / dates ont change.
        int64 key = files.size();
        for (auto& f : files) key = key * 31 + f.getSize() + f.getLastModificationTime().toMilliseconds();
        if (key != libKey)
        {
            libKey = key;
            libCache.clear();
            for (auto& f : files)
            {
                MemoryBlock mb;
                if (! f.loadFileAsData (mb)) continue;
                auto blocks = SyMulti::multisIn ((const uint8*) mb.getData(), mb.getSize());
                if (! blocks.isEmpty()) libCache.push_back ({ f.getFileNameWithoutExtension(), blocks });
            }
        }
        libMultis.clear();
        libLabels.clear();
        PopupMenu m;
        for (auto& entry : libCache)
        {
            const auto& blocks = entry.second;
            const String fileName = entry.first;
            if (blocks.isEmpty()) continue;
            PopupMenu sub;
            for (auto& b : blocks)
            {
                const auto mu = SyMulti::parse ((const uint8*) b.getData(), (int) b.getSize());
                const int idx = libMultis.size();
                libMultis.add (b);
                libLabels.add (fileName + " / " + mu.name);
                sub.addItem ((mu.memType == 0 ? "INT " : "PRE ") + String (mu.memNumber + 1) + "  " + mu.name,
                             [this, idx]
                             {
                                 const auto& blk = libMultis.getReference (idx);
                                 setMulti (SyMulti::parse ((const uint8*) blk.getData(), (int) blk.getSize()), libLabels[idx], blk);
                             });
            }
            m.addSubMenu (fileName, sub);
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
    MemoryBlock baseBlock;   // bloc d'origine (effets/reserve conserves a l'envoi)
    TextEditor nameEd;
    TextButton sendBt { TRANS ("Send to SY77") };
    int64 libKey = -1;
    std::vector<std::pair<String, Array<MemoryBlock>>> libCache;
    Detail detail;
    Label partLabel;
    ComboBox memCombo, numCombo;
    Slider volS, tuneS, shiftS, panS;
    TextButton panVoiceBt { TRANS ("voice") }, out1Bt { "1" }, out2Bt { "2" }, swBt { "Sw" };
    int sel = 0;
    String source;
    TableListBox table;
    TextButton libraryBt { TRANS ("Library multis...") }, requestBt { TRANS ("Request from SY77") };
    ComboBox slotCombo;
    Array<MemoryBlock> libMultis;
    StringArray libLabels;
    SysexBusSender sender;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiPage)
};
