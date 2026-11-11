// MixtureView.h - fenetre "Quadratic Mixture" (facon SynthWorks, docs/synthwork/quadratic-mixture*.jpg).
// 4 voix aux coins, pad carre : la position donne les poids (SysexMix.h). Rien n'est envoye en
// deplacant le curseur : TEST ouvre la voix melangee comme une audition de librairie (edit
// buffer, meme chemin, meme confirmation si edits non sauves) ; STORE l'ajoute a la banque
// "Mixtures.syx" de la librairie (jamais a une memoire du synthe).
#pragma once
#include "SysexMix.h"
#include "Templates.h"
#include <map>

class MixturePad : public juce::Component
{
public:
    std::function<void()> onMove;
    juce::Point<double> pos { 0.5, 0.5 };   // [0,1]^2, y vers le bas

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (SYPal.surfaceAlt.darker (0.6f));
        g.fillRect (r);
        g.setColour (SYPal.panelBorder);
        g.drawRect (r, 1.0f);
        // Arcs : quart de cercle centre sur chaque coin, rayon = cote (lignes poids nul).
        g.setColour (SYPal.textMuted.withAlpha (0.8f));
        const juce::Point<float> corners[4] { r.getTopLeft(), r.getTopRight(), r.getBottomLeft(), r.getBottomRight() };
        for (auto c : corners)
        {
            juce::Path p;
            p.addEllipse (c.x - r.getWidth(), c.y - r.getHeight(), 2 * r.getWidth(), 2 * r.getHeight());
            g.saveState(); g.reduceClipRegion (r.toNearestInt()); g.strokePath (p, juce::PathStrokeType (1.2f)); g.restoreState();
        }
        // Curseur en croix.
        const float px = r.getX() + (float) pos.x * r.getWidth(), py = r.getY() + (float) pos.y * r.getHeight();
        g.setColour (SYPal.accent);
        g.drawLine (px - 9, py, px + 9, py, 2.0f);
        g.drawLine (px, py - 9, px, py + 9, 2.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override { moveTo (e.position); }
    void mouseDrag (const juce::MouseEvent& e) override { moveTo (e.position); }

private:
    void moveTo (juce::Point<float> p)
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        pos = { juce::jlimit (0.0, 1.0, (double) ((p.x - r.getX()) / r.getWidth())),
                juce::jlimit (0.0, 1.0, (double) ((p.y - r.getY()) / r.getHeight())) };
        repaint();
        if (onMove) onMove();
    }
};

class MixtureView : public juce::Component
{
public:
    /** Fenetre unique (non modale) ; corner >= 0 pose d'abord ce coin. */
    static void show (int corner = -1, const juce::MemoryBlock& voice = {}, const juce::String& label = {})
    {
        auto& inst = instance();
        if (inst == nullptr)
        {
            auto* v = new MixtureView();
            v->setSize (640, 600);
            new Window (v);   // NON modale : la librairie reste utilisable (coins par clic droit)
            inst = v;
        }
        if (corner >= 0) inst->setCorner (corner, voice, label);
        if (auto* top = inst->getTopLevelComponent()) top->toFront (true);
    }

    /** Ferme la fenetre (fin de l'application : evite fuite + reference au LookAndFeel). */
    static void closeAll()
    {
        if (auto* v = instance().getComponent())
            if (auto* w = v->findParentComponentOfClass<Window>()) delete w;
    }

    void setCorner (int i, const juce::MemoryBlock& voice, const juce::String& label)
    {
        if (i < 0 || i > 3) return;
        corners[(size_t) i] = voice;
        cornerBt[i].setButtonText (label.isNotEmpty() ? label : TRANS ("(choose a voice)"));
        mapValid = false;   // carte + decodage des coins recalcules une fois (pas a chaque geste)
        refresh();
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (12);
        title.setBounds (r.removeFromTop (28));
        r.removeFromTop (6);
        auto bottom = r.removeFromBottom (30);
        status.setBounds (r.removeFromBottom (22));
        r.removeFromBottom (4);
        auto top = r.removeFromTop (28), low = r.removeFromBottom (28);
        const int bw = juce::jmin (220, (top.getWidth() - 10) / 2);
        cornerBt[0].setBounds (top.removeFromLeft (bw));  cornerBt[1].setBounds (top.removeFromRight (bw));
        cornerBt[2].setBounds (low.removeFromLeft (bw));  cornerBt[3].setBounds (low.removeFromRight (bw));
        r.reduce (0, 6);
        const int side = juce::jmin (r.getWidth(), r.getHeight());
        pad.setBounds (r.withSizeKeepingCentre (side, side));
        const int n = 3, gap = 8, w = (bottom.getWidth() - n * gap - 110) / n;
        nameEd.setBounds (bottom.removeFromLeft (110)); bottom.removeFromLeft (gap);
        for (auto* b : { &testBt, &storeBt, &exitBt }) { b->setBounds (bottom.removeFromLeft (w)); bottom.removeFromLeft (gap); }
    }

    void paint (juce::Graphics& g) override { g.fillAll (SYPal.background); }

    ~MixtureView() override { if (instance() == this) instance() = nullptr; }

private:
    MixtureView()
    {
        title.setText ("QUADRATIC MIXTURE", juce::dontSendNotification);
        title.setJustificationType (juce::Justification::centred);
        title.setFont (juce::FontOptions (16.0f, juce::Font::bold));
        addAndMakeVisible (title);
        for (int i = 0; i < 4; ++i)
        {
            cornerBt[i].setButtonText (TRANS ("(choose a voice)"));
            cornerBt[i].setTooltip (TRANS ("Choose this corner's voice from the current library bank"));
            cornerBt[i].onClick = [this, i] { chooseCorner (i); };
            addAndMakeVisible (cornerBt[i]);
        }
        pad.onMove = [this] { refresh(); };
        addAndMakeVisible (pad);
        status.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (status);
        nameEd.setText ("Mixture", false);
        juce::String printable;   // nom de voix SY77 = ASCII 32..126 (pas d'accents)
        for (char c = 32; c < 127; ++c) printable += juce::String::charToString ((juce::juce_wchar) c);
        nameEd.setInputRestrictions (10, printable);
        nameEd.setTooltip (TRANS ("Name of the mixed voice"));
        addAndMakeVisible (nameEd);
        testBt.setTooltip (TRANS ("Open the mixed voice in the editor and the SY77 edit buffer (like a library audition)"));
        storeBt.setTooltip (TRANS ("Add the mixed voice to the Mixtures.syx bank of the library (nothing is written to the synth)"));
        testBt.onClick  = [this] { test(); };
        storeBt.onClick = [this] { store(); };
        exitBt.onClick  = [this] { if (auto* w = findParentComponentOfClass<Window>()) w->closeButtonPressed(); };
        for (auto* b : { &testBt, &storeBt, &exitBt }) addAndMakeVisible (b);
        refresh();
    }

    static juce::Component::SafePointer<MixtureView>& instance() { static juce::Component::SafePointer<MixtureView> p; return p; }

    /** Fenetre non modale ; se detruit (avec son contenu) a la fermeture. */
    struct Window : public juce::DocumentWindow
    {
        explicit Window (MixtureView* content)
            : juce::DocumentWindow ("Quadratic Mixture", SYPal.background, juce::DocumentWindow::closeButton)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (content, true);
            setResizable (true, false);
            setResizeLimits (420, 420, 2000, 2000);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }
        void closeButtonPressed() override
        {
            setVisible (false);
            juce::MessageManager::callAsync ([w = juce::Component::SafePointer<Window> (this)] { delete w.getComponent(); });
        }
    };

public:
    /** Ajout d'une voix a une banque de la librairie : fourni par LibrairiePage (meme code que
        "Copier vers une banque" : ecriture sure, reindexation, rafraichissement). */
    struct AppendResult { juce::String error; int pos = -1; int padded = 0; };
    using Appender = std::function<AppendResult (const juce::File&, const juce::MemoryBlock&, bool padBankD)>;
    static Appender& bankAppender() { static Appender f; return f; }
private:

    void chooseCorner (int i)
    {
        const auto bank = SyBank::Bank::parse (currentBankData.getData(), currentBankData.getSize());
        int wantType = -1;
        for (int k = 0; k < 4; ++k) if (k != i && corners[(size_t) k].getSize() > 0) { wantType = SyBank::typeOf (corners[(size_t) k]); break; }
        juce::PopupMenu m;
        m.addSectionHeader (TRANS ("Current library bank"));
        for (int v = 0; v < bank.numVoices(); ++v)
        {
            const auto& b = bank.voices.getReference (v);
            const int t = SyBank::typeOf (b);
            const juce::String nm = SyBank::slotName (v) + "  " + juce::String ((const char*) b.getData() + 33, 10).trim();
            m.addItem (nm, t != 0x0A && (wantType < 0 || t == wantType), false,
                       [this, i, b, nm] { setCorner (i, b, nm); });
        }
        if (bank.numVoices() == 0) m.addItem (TRANS ("Select a bank in the library first"), false, false, [] {});
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&cornerBt[i]));
    }

    void refresh()
    {
        const auto why = SyMix::incompatibility (corners);
        const bool ok = why.isEmpty();
        testBt.setEnabled (ok); storeBt.setEnabled (ok);
        if (! ok) { status.setText (TRANS (why), juce::dontSendNotification); result = {}; return; }
        if (! mapValid)
        {
            // La carte ne depend que de la disposition (type + taille) : calculee une fois par session.
            static std::map<std::pair<int, size_t>, SyMix::OffsetMap> cache;
            const auto key = std::make_pair (SyBank::typeOf (corners[0]), corners[0].getSize());
            auto it = cache.find (key);
            if (it == cache.end()) it = cache.emplace (key, SyMix::buildOffsetMap (corners[0])).first;
            map = it->second;
            decoded = SyMix::decodeAll (corners);
            mapValid = true;
        }
        result = SyMix::mix (corners, decoded, pad.pos.x, pad.pos.y, map, nameEd.getText());
        const auto w = SyMix::weights (pad.pos.x, pad.pos.y);
        status.setText (TRANS ("Base") + ": " + cornerBt[result.base].getButtonText() + "   |   "
                        + juce::String (result.mixed) + " " + TRANS ("params mixed") + "   |   "
                        + juce::String (juce::roundToInt (w[0] * 100)) + "/" + juce::String (juce::roundToInt (w[1] * 100)) + "/"
                        + juce::String (juce::roundToInt (w[2] * 100)) + "/" + juce::String (juce::roundToInt (w[3] * 100)) + " %",
                        juce::dontSendNotification);
    }

    void test()
    {
        refresh();
        if (result.block.getSize() == 0) return;
        sender.send (adresseOscOpenVoiceBlock, result.block);
    }

    void store()
    {
        refresh();
        if (result.block.getSize() == 0 || ! bankAppender()) return;
        const auto f = appDirPath.getChildFile ("Mixtures.syx");
        // Voix 4 elements : seulement en banque D. Si Mixtures.syx a moins de 48 voix, il faut
        // completer A1..C16 avec des voix INIT : jamais sans accord (un envoi de cette banque au
        // synthe ecraserait ces slots).
        int missing = 0;
        if (SyBank::needsBankD (SyBank::typeOf (result.block)))
        {
            juce::MemoryBlock data;
            if (f.existsAsFile()) f.loadFileAsData (data);
            missing = juce::jmax (0, SyBank::kFirstD - SyBank::Bank::parse (data.getData(), data.getSize()).numVoices());
        }
        if (missing == 0) { doStore (f, result.block, false); return; }
        auto block = result.block;
        juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, TRANS ("Store"),
            TRANS ("4-element voices can only go to bank D (D1-D16).") + "\n"
              + juce::String (missing) + " " + TRANS ("INIT voices will be added to Mixtures.syx first (up to C16).") + "\n"
              + TRANS ("Sending this bank to the synth would overwrite those slots with INIT."),
            TRANS ("Add"), TRANS ("Cancel"), nullptr,
            juce::ModalCallbackFunction::create ([self = juce::Component::SafePointer<MixtureView> (this), f, block] (int r)
            {
                if (r == 1 && self != nullptr) self->doStore (f, block, true);
            }));
    }

    void doStore (const juce::File& f, const juce::MemoryBlock& block, bool padBankD)
    {
        const auto r = bankAppender() (f, block, padBankD);
        if (r.error.isNotEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, TRANS ("Store"), r.error);
            return;
        }
        status.setText (TRANS ("Stored in") + " Mixtures.syx (" + SyBank::slotName (r.pos) + ")"
                        + (r.padded > 0 ? "  -  " + juce::String (r.padded) + " INIT" : juce::String()),
                        juce::dontSendNotification);
    }

    std::array<juce::MemoryBlock, 4> corners;
    SyMix::OffsetMap map;
    SyMix::Decoded decoded;
    bool mapValid = false;
    SyMix::Result result;
    juce::Label title, status;
    juce::TextButton cornerBt[4];
    MixturePad pad;
    juce::TextEditor nameEd;
    juce::TextButton testBt { "TEST" }, storeBt { "STORE" }, exitBt { "EXIT" };
    SysexBusSender sender;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixtureView)
};
