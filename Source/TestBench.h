/*
  ==============================================================================
    TestBench.h  (MODE DEVELOPPEUR uniquement, onglet "Midi Setting")

    Banc de test hardware : valider sur le SY77 ce qui reste "non verifie" dans MAP.md, en
    utilisant le synthe lui-meme comme oracle (et plus l'oeil).

    Etape 0 = sondes de capacite. Elles disent quelles methodes de verification sont possibles :
      A   lecture de l'edit buffer voix ($7F) + stabilite (2 dumps identiques)  [aucune ecriture]
      A2  lecture de l'edit buffer Multi ($7F, interdit par la spec)             [aucune ecriture]
      B   ecoute : le SY77 emet-il des param-changes quand on edite en facade ?  [aucune ecriture]
      CAL boucle fermee sur un param deja verifie (ELVL elem 1 @98) : dump -> envoi -> dump ->
          diff -> restauration. Valide la METHODE avant de l'appliquer aux params douteux.
                                                    [ecrit l'edit buffer, restaure, confirmation]
      C   reception d'un bulk Multi vers l'edit buffer $7F (question oui/non a l'ecran)
                                                    [ecrit l'edit buffer, confirmation]

    Envois via Ports::send, qui CONTOURNE l'historique d'edition voix (sinon undo/A-B
    rejouerait les valeurs de test vers le synthe).
    Litteraux ASCII uniquement (cf. String(const char*)).
  ==============================================================================
*/

#pragma once

#include <deque>

//==============================================================================
/** Logique pure du banc (testee dans Tests.h). */
namespace SyBench
{
    /** Bulk voix SY77 complet : F0 43 0n 7A .. "LM  8101VC" .. F7. */
    inline bool isVoiceBulk (const juce::uint8* d, int n)
    {
        return d != nullptr && n > 34 && d[0] == 0xF0 && d[1] == 0x43 && (d[2] & 0xF0) == 0x00
            && d[3] == 0x7A && d[n - 1] == 0xF7 && std::memcmp (d + 6, "LM  8101VC", 10) == 0;
    }

    /** Offsets qui different entre deux blocs de meme taille, checksum (n-2) exclu.
        Tailles differentes -> { -1 }. */
    inline juce::Array<int> diffOffsets (const juce::MemoryBlock& a, const juce::MemoryBlock& b)
    {
        juce::Array<int> out;
        if (a.getSize() != b.getSize()) { out.add (-1); return out; }
        const auto* x = (const juce::uint8*) a.getData();
        const auto* y = (const juce::uint8*) b.getData();
        const int n = (int) a.getSize();
        for (int i = 0; i < n; ++i)
            if (i != n - 2 && x[i] != y[i]) out.add (i);
        return out;
    }

    enum class Verdict { Ok, OkWithSideEffects, NoChange, WrongOffset, WrongValue, SizeMismatch };

    struct Result
    {
        Verdict verdict = Verdict::NoChange;
        juce::Array<int> changed;   // offsets modifies (checksum exclu)
    };

    /** Verdict d'un cycle boucle fermee : on attendait `expected` a l'offset `offset`. */
    inline Result classify (const juce::MemoryBlock& before, const juce::MemoryBlock& after,
                            int offset, int expected)
    {
        Result r;
        r.changed = diffOffsets (before, after);
        if (r.changed.contains (-1))  { r.verdict = Verdict::SizeMismatch; return r; }
        if (r.changed.isEmpty())      { r.verdict = Verdict::NoChange;     return r; }
        if (! r.changed.contains (offset)) { r.verdict = Verdict::WrongOffset; return r; }
        const int got = ((const juce::uint8*) after.getData())[offset];
        if (got != (expected & 0x7F)) { r.verdict = Verdict::WrongValue; return r; }
        r.verdict = r.changed.size() == 1 ? Verdict::Ok : Verdict::OkWithSideEffects;
        return r;
    }

    inline juce::String offsetsToString (const juce::Array<int>& offs, int max = 12)
    {
        juce::StringArray s;
        for (int i = 0; i < offs.size() && i < max; ++i) s.add ("@" + juce::String (offs[i]));
        if (offs.size() > max) s.add ("... (" + juce::String (offs.size()) + ")");
        return s.joinIntoString (" ");
    }
}

//==============================================================================
/** Panneau du banc de test (embarque dans MidiSettingsPage). Machine a etapes sequentielle
    pilotee par un Timer : chaque etape = action, puis attente (fin anticipee via `done`). */
struct TestBenchPanel : public juce::Component, private juce::Timer
{
    struct Ports
    {
        std::function<void (const juce::MidiMessage&)> send;   // hors historique d'edition
        std::function<int()>                            device;
        std::function<juce::String()>                   preflight;  // "" = OK, sinon raison
    };
    Ports ports;

    TestBenchPanel()
    {
        title.setText ("Banc de test hardware (etape 0 : sondes)", juce::dontSendNotification);
        title.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        addAndMakeVisible (title);

        for (auto* b : { &btA, &btB, &btCal, &btC, &btStop, &btCopy })
            addAndMakeVisible (b);
        btA.setTooltip   ("Demande 2 dumps de l'edit buffer voix ($7F) et les compare. Puis tente le $7F Multi. N'ecrit rien.");
        btB.setTooltip   ("Ecoute 20 s : touche un parametre sur le SY77, l'app verifie qu'elle recoit le param-change. N'ecrit rien.");
        btCal.setTooltip ("Boucle fermee sur ELVL element 1 (deja verifie) : change la valeur dans l'edit buffer, relit, compare, restaure.");
        btC.setTooltip   ("Envoie le Multi INT 1 renomme BENCH TEST vers l'edit buffer $7F, puis demande ce qu'affiche le SY77.");

        btA.onClick    = [this] { runProbeA(); };
        btB.onClick    = [this] { runProbeB(); };
        btCal.onClick  = [this] { confirmThen ("Calibration boucle fermee",
            "Modifie le niveau de l'element 1 (ELVL) dans l'EDIT BUFFER du SY77, relit, puis remet la valeur d'origine. "
            "Aucune memoire n'est ecrite. Le SY77 doit etre sur une voix (pas une drum).",
            [this] { runCalibration(); }); };
        btC.onClick    = [this] { confirmThen ("Reception Multi $7F",
            "Mets d'abord le SY77 en mode MULTI. L'app lit le Multi INT 1, le renomme BENCH TEST et l'envoie a l'EDIT BUFFER. "
            "Aucune memoire n'est ecrite (re-selectionne le multi sur le synthe pour annuler). Bulk Protect doit etre OFF.",
            [this] { runProbeC(); }); };
        btStop.onClick = [this] { if (listening) stopListen = true; else abort ("arrete"); };
        btCopy.onClick = [this] { juce::SystemClipboard::copyTextToClipboard (log.getText()); };

        log.setMultiLine (true);
        log.setReadOnly (true);
        log.setScrollbarsShown (true);
        log.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
        addAndMakeVisible (log);
        refreshButtons();
    }

    ~TestBenchPanel() override { stopTimer(); }

    bool isBusy() const noexcept { return running; }

    /** A appeler (thread message) pour CHAQUE message entrant, avant tout filtrage device. */
    void handleIncoming (const juce::MidiMessage& m)
    {
        if (! running || ! m.isSysEx()) return;
        rx.add (juce::MemoryBlock (m.getRawData(), (size_t) m.getRawDataSize()));
        lastRxMs = juce::Time::getMillisecondCounter();

        if (listening && m.getSysExDataSize() == 9)
        {
            const auto* d = m.getSysExData();
            if (d[0] != 0x43 || d[2] != 0x34) return;
            const int dev = ports.device ? ports.device() : 1;
            const bool devOk = (d[1] & 0xF0) == 0x10 && (d[1] & 0x0F) == ((dev - 1) & 0x0F);
            if (devOk) ++listenOk; else ++listenBadDev;
            auto hx = [] (int v) { return juce::String::toHexString (v).paddedLeft ('0', 2).toUpperCase(); };
            say ("  recu  G=" + hx (d[3]) + " AH=" + hx (d[4]) + " AL=" + hx (d[5]) + " P=" + hx (d[6])
                 + "  val=" + juce::String ((int) d[8]) + (d[7] ? " (hi " + juce::String ((int) d[7]) + ")" : juce::String())
                 + (devOk ? "" : "   [device " + juce::String ((d[1] & 0x0F) + 1) + " != " + juce::String (dev) + "]"));
        }
    }

    /** Interrompt toute sonde en cours (ex. mode developpeur desactive). */
    void abort (const juce::String& why)
    {
        if (! running) return;
        steps.clear();
        stopTimer();
        running = listening = false;
        say ("-- interrompu (" + why + ")");
        refreshButtons();
    }

    void resized() override
    {
        auto r = getLocalBounds();
        title.setBounds (r.removeFromTop (22));
        r.removeFromTop (4);
        auto row = r.removeFromTop (26);
        const int gap = 6, n = 6;
        const int w = (row.getWidth() - gap * (n - 1)) / n;
        for (auto* b : { &btA, &btB, &btCal, &btC, &btStop, &btCopy })
        {
            b->setBounds (row.removeFromLeft (w));
            row.removeFromLeft (gap);
        }
        r.removeFromTop (6);
        log.setBounds (r);
    }

private:
    struct Step
    {
        std::function<void()> action;
        int maxWaitMs = 0;
        std::function<bool()> done;   // fin anticipee (nullptr = attendre maxWaitMs)
    };

    juce::Array<juce::MemoryBlock> rx;
    std::deque<Step> steps;
    bool running = false, listening = false, stopListen = false, waiting = false;
    juce::uint32 stepDeadline = 0, lastRxMs = 0;
    int listenOk = 0, listenBadDev = 0;

    // Etat des sondes
    juce::MemoryBlock snap1, snap2, snap3;
    int calOrig = 0, calValue = 0;
    static constexpr int kOff = 98;   // ELVL elem 1 dans le bulk voix (MAP.md, 12 089 voix)

    juce::Label title;
    juce::TextButton btA { "A  Lire edit buffer" }, btB { "B  Ecouter 20 s" },
                     btCal { "CAL  Boucle fermee" }, btC { "C  Multi $7F" },
                     btStop { "Stop" }, btCopy { "Copier rapport" };
    juce::TextEditor log;

    //--------------------------------------------------------------------------
    void say (const juce::String& s)
    {
        log.moveCaretToEnd();
        log.insertTextAtCaret (s + "\n");
    }

    void refreshButtons()
    {
        for (auto* b : { &btA, &btB, &btCal, &btC })
            b->setEnabled (! running);
        btStop.setEnabled (running);
    }

    bool begin (const juce::String& name)
    {
        if (running) return false;
        const auto why = ports.preflight ? ports.preflight() : juce::String();
        if (why.isNotEmpty()) { say ("[" + name + "] impossible : " + why); return false; }
        rx.clear();
        steps.clear();
        running = true;
        say ("");
        say ("=== " + name + "  (" + juce::Time::getCurrentTime().toString (true, true) + ", device "
             + juce::String (ports.device ? ports.device() : 1) + ")");
        refreshButtons();
        return true;
    }

    void finish()
    {
        steps.clear();
        stopTimer();
        running = listening = waiting = false;
        refreshButtons();
    }

    void go()
    {
        waiting = false;
        startTimer (40);
    }

    void timerCallback() override
    {
        const auto now = juce::Time::getMillisecondCounter();
        if (waiting)
        {
            auto& s = steps.front();
            const bool early = s.done && s.done();
            if (! early && now < stepDeadline) return;
            steps.pop_front();
            waiting = false;
        }
        if (steps.empty()) { finish(); return; }

        // Copies AVANT l'appel : l'action peut vider `steps` (finish) et detruire sa lambda.
        const auto act  = steps.front().action;
        const int  wait = steps.front().maxWaitMs;
        if (act) act();
        if (! running) return;            // l'action a pu terminer/avorter la sonde
        if (steps.empty()) { finish(); return; }
        stepDeadline = now + (juce::uint32) wait;
        waiting = true;
    }

    void send (const juce::MidiMessage& m) { if (ports.send) ports.send (m); }

    /** Dernier bulk voix / Multi recu depuis le dernier clear. */
    juce::MemoryBlock lastVoiceBulk() const
    {
        for (int i = rx.size(); --i >= 0;)
            if (SyBench::isVoiceBulk ((const juce::uint8*) rx[i].getData(), (int) rx[i].getSize()))
                return rx[i];
        return {};
    }
    juce::MemoryBlock lastMultiBulk() const
    {
        for (int i = rx.size(); --i >= 0;)
            if (SyMulti::isMultiBlock ((const juce::uint8*) rx[i].getData(), (int) rx[i].getSize()))
                return rx[i];
        return {};
    }

    /** Etape : vide la reception, envoie une requete, attend un bloc (fin 250 ms apres le
        dernier octet recu, 2.5 s max). */
    Step requestStep (juce::MidiMessage req, bool multi)
    {
        return { [this, req] { rx.clear(); lastRxMs = 0; send (req); }, 2500,
                 [this, multi]
                 {
                     const bool got = multi ? lastMultiBulk().getSize() > 0 : lastVoiceBulk().getSize() > 0;
                     return got && juce::Time::getMillisecondCounter() - lastRxMs > 250;
                 } };
    }
    Step voiceEditRequest() { return requestStep (SyVoice::voiceDumpRequest (dev(), 0x7F, 0x00), false); }
    int dev() const { return ports.device ? ports.device() : 1; }

    static juce::String voiceInfo (const juce::MemoryBlock& b)
    {
        const auto* d = (const juce::uint8*) b.getData();
        juce::String name;
        for (int i = 33; i < 43 && i < (int) b.getSize() - 2; ++i) name << juce::String::charToString ((juce::juce_wchar) d[i]);
        return juce::String ((int) b.getSize()) + " octets, type " + juce::String ((int) d[32])
             + ", mem " + juce::String::toHexString ((int) d[30]) + "/" + juce::String ((int) d[31])
             + ", nom \"" + name.trimEnd() + "\"";
    }

    void confirmThen (const juce::String& t, const juce::String& msg, std::function<void()> fn)
    {
        juce::Component::SafePointer<TestBenchPanel> self (this);
        juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::WarningIcon, t, msg,
            "Lancer", "Annuler", nullptr,
            juce::ModalCallbackFunction::create ([self, fn] (int r) { if (r == 1 && self != nullptr) fn(); }));
    }

    //--------------------------------------------------------------------------
    // A + A2 : lecture de l'edit buffer (voix, puis Multi). Aucune ecriture.
    void runProbeA()
    {
        if (! begin ("A  lecture edit buffer")) return;
        steps.push_back (voiceEditRequest());
        steps.push_back ({ [this] { snap1 = lastVoiceBulk(); }, 0, nullptr });
        steps.push_back (voiceEditRequest());
        steps.push_back ({ [this]
        {
            snap2 = lastVoiceBulk();
            if (snap1.getSize() == 0)
            {
                say ("A  KO : aucune reponse au dump request voix $7F (" + juce::String (rx.size()) + " sysex recus).");
                say ("   -> boucle fermee directe impossible ; plan B = slot INT sacrifiable (STORE manuel).");
            }
            else
            {
                say ("A  recu : " + voiceInfo (snap1));
                const auto d = SyBench::diffOffsets (snap1, snap2);
                if (snap2.getSize() == 0)  say ("A  KO : 2e requete sans reponse (instable).");
                else if (d.isEmpty())      say ("A  OK : edit buffer lisible, 2 dumps identiques -> boucle fermee possible.");
                else                       say ("A  KO : 2 dumps differents sans edition : " + SyBench::offsetsToString (d));
            }
        }, 0, nullptr });
        steps.push_back (requestStep (SyMulti::dumpRequest (dev(), 0x7F, 0x00), true));
        steps.push_back ({ [this]
        {
            const auto m = lastMultiBulk();
            if (m.getSize() > 0) say ("A2 recu un Multi en reponse a $7F : la spec est contredite, l'edit buffer Multi est LISIBLE.");
            else                 say ("A2 pas de reponse au $7F Multi (conforme a la spec : edit buffer Multi illisible).");
        }, 0, nullptr });
        go();
    }

    //--------------------------------------------------------------------------
    // B : ecoute des param-changes emis par le SY77. Aucune ecriture.
    void runProbeB()
    {
        if (! begin ("B  ecoute 20 s")) return;
        steps.push_back ({ [this]
        {
            listening = true; stopListen = false; listenOk = listenBadDev = 0;
            say ("B  touche quelques parametres sur le SY77 (voix, multi, effets)... [Stop pour finir]");
        }, 20000, [this] { return stopListen; } });
        steps.push_back ({ [this]
        {
            listening = false;
            if (listenOk > 0)
                say ("B  OK : " + juce::String (listenOk) + " param-changes recus -> mode ecoute guide possible.");
            else if (listenBadDev > 0)
                say ("B  KO device : " + juce::String (listenBadDev) + " param-changes recus avec un autre device number. Aligner le device (Setting) sur celui du SY77.");
            else
                say ("B  KO : rien recu. Verifier sur le SY77 : UTILITY > MIDI, transmission SysEx / param ON, puis le cablage MIDI OUT du synthe.");
        }, 0, nullptr });
        go();
    }

    //--------------------------------------------------------------------------
    // CAL : boucle fermee sur ELVL elem 1 (G 0x03 / AH 0 / P 0x00 ; dump @98).
    void runCalibration()
    {
        if (! begin ("CAL  boucle fermee ELVL elem 1")) return;
        steps.push_back (voiceEditRequest());
        steps.push_back ({ [this]
        {
            snap1 = lastVoiceBulk();
            const auto* d = (const juce::uint8*) snap1.getData();
            if (snap1.getSize() == 0)       { say ("CAL KO : edit buffer illisible (lancer A)."); finish(); return; }
            if ((int) snap1.getSize() <= kOff + 2 || d[32] == 10) { say ("CAL annule : voix drum ou bloc trop court, choisir une voix normale."); finish(); return; }
            calOrig  = d[kOff];
            calValue = (calOrig + 37) % 128;
            say ("CAL avant : ELVL = " + juce::String (calOrig) + " ; envoi " + juce::String (calValue));
            send (SyVoice::paramMessage (dev(), 0x03, 0x00, 0x00, 0x00, (juce::uint8) calValue));
        }, 250, nullptr });
        steps.push_back (voiceEditRequest());
        steps.push_back ({ [this]
        {
            snap2 = lastVoiceBulk();
            if (snap2.getSize() == 0) say ("CAL KO : pas de dump apres l'envoi.");
            else
            {
                const auto r = SyBench::classify (snap1, snap2, kOff, calValue);
                const auto ch = SyBench::offsetsToString (r.changed);
                switch (r.verdict)
                {
                    case SyBench::Verdict::Ok:                say ("CAL OK : seul @98 a change, valeur exacte -> methode validee."); break;
                    case SyBench::Verdict::OkWithSideEffects: say ("CAL OK (+ effets de bord) : @98 exact, autres octets : " + ch); break;
                    case SyBench::Verdict::NoChange:          say ("CAL KO : aucun octet n'a change (param ignore ? Bulk/MIDI protect ?)."); break;
                    case SyBench::Verdict::WrongOffset:       say ("CAL KO : @98 intact, octets modifies : " + ch); break;
                    case SyBench::Verdict::WrongValue:        say ("CAL KO : @98 = " + juce::String ((int) ((const juce::uint8*) snap2.getData())[kOff]) + " au lieu de " + juce::String (calValue)); break;
                    case SyBench::Verdict::SizeMismatch:      say ("CAL KO : taille du dump changee (" + juce::String ((int) snap1.getSize()) + " -> " + juce::String ((int) snap2.getSize()) + ")"); break;
                }
            }
            send (SyVoice::paramMessage (dev(), 0x03, 0x00, 0x00, 0x00, (juce::uint8) calOrig));
        }, 250, nullptr });
        steps.push_back (voiceEditRequest());
        steps.push_back ({ [this]
        {
            snap3 = lastVoiceBulk();
            if (snap3.getSize() > 0 && SyBench::diffOffsets (snap1, snap3).isEmpty())
                say ("CAL restauration OK (edit buffer identique a l'origine).");
            else
                say ("CAL ATTENTION : restauration non confirmee, remettre ELVL elem 1 a " + juce::String (calOrig) + " sur le synthe.");
        }, 0, nullptr });
        go();
    }

    //--------------------------------------------------------------------------
    // C : bulk Multi vers l'edit buffer $7F, verdict par l'utilisateur.
    void runProbeC()
    {
        if (! begin ("C  reception Multi $7F")) return;
        steps.push_back (requestStep (SyMulti::dumpRequest (dev(), 0x00, 0x00), true));
        steps.push_back ({ [this]
        {
            const auto base = lastMultiBulk();
            auto m = SyMulti::parse ((const juce::uint8*) base.getData(), (int) base.getSize());
            if (! m.loaded) { say ("C  KO : Multi INT 1 illisible (pas de reponse au dump request Multi)."); finish(); return; }
            say ("C  Multi INT 1 lu : \"" + m.name + "\" ; envoi renomme BENCH TEST vers $7F");
            m.name = "BENCH TEST";
            const auto blk = SyMulti::encodeForEditBuffer (m, base);
            if (blk.getSize() == 0) { say ("C  KO : encodage impossible."); finish(); return; }
            send (juce::MidiMessage (blk.getData(), (int) blk.getSize()));
        }, 600, nullptr });
        steps.push_back ({ [this]
        {
            juce::Component::SafePointer<TestBenchPanel> self (this);
            juce::AlertWindow::showYesNoCancelBox (juce::MessageBoxIconType::QuestionIcon, "Reception Multi $7F",
                "L'ecran du SY77 (mode Multi) affiche-t-il le nom BENCH TEST ?",
                "Oui", "Non", "Je ne sais pas", nullptr,
                juce::ModalCallbackFunction::create ([self] (int r)
                {
                    if (self == nullptr) return;
                    if (r == 1)      self->say ("C  OK : le SY77 accepte un bulk Multi vers l'edit buffer $7F.");
                    else if (r == 2) self->say ("C  KO : bulk Multi $7F ignore (Bulk Protect ? ou $7F non supporte en reception).");
                    else             self->say ("C  ? : resultat non observe.");
                    self->say ("   (re-selectionner le multi sur le SY77 pour annuler l'edition)");
                    self->finish();
                }));
        }, 600000, nullptr });   // fin = reponse au dialogue (finish), pas un delai
        go();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestBenchPanel)
};
