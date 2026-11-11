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
#include <map>
#include <set>

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

    //--------------------------------------------------------------------------
    // ETAPE 1 : lot "transplant". On rejoue sur la voix S (slot de test) les params decodes
    // d'une voix reelle T du meme type -> valeurs legales par construction. Apres STORE + dump
    // INT, le decodeur doit relire les valeurs de T. Prouve adresse + offset + encodage DU
    // DECODEUR (pas le cablage des widgets : voir le croisement widgets/decodeur).
    using VP = SyVoice::VoiceParam;

    inline juce::int64 keyOf (int g, int hi, int lo, int p)
    {
        return ((juce::int64) (g & 0xFF) << 24) | ((hi & 0xFF) << 16) | ((lo & 0xFF) << 8) | (p & 0xFF);
    }
    inline juce::int64 keyOf (const VP& v) { return keyOf (v.group, v.addrHi, v.addrLo, v.param); }

    inline std::map<juce::int64, VP> paramMap (const juce::MemoryBlock& b)
    {
        std::map<juce::int64, VP> m;
        for (auto& v : SyVoice::voiceBlobToParams ((const juce::uint8*) b.getData(), (int) b.getSize()))
            m[keyOf (v)] = v;   // derniere occurrence gagne (comme une relecture par l'editeur)
        return m;
    }

    inline bool sameValue (const VP& a, const VP& b) { return a.value == b.value && a.v1 == b.v1; }

    struct LotPlan
    {
        juce::Array<VP> toSend;   // params de T qui different de S (valeurs de T)
        int uncovered = 0;        // params identiques dans S et T : non testes par ce lot
        bool compatible = false;  // meme type @32 et meme taille
    };

    inline LotPlan planLot (const juce::MemoryBlock& s, const juce::MemoryBlock& t)
    {
        LotPlan r;
        if (s.getSize() != t.getSize() || s.getSize() < 34
            || ((const juce::uint8*) s.getData())[32] != ((const juce::uint8*) t.getData())[32])
            return r;
        r.compatible = true;
        const auto ms = paramMap (s), mt = paramMap (t);
        for (auto& [k, vt] : mt)
        {
            auto it = ms.find (k);
            if (it == ms.end()) continue;
            if (sameValue (it->second, vt)) ++r.uncovered;
            else r.toSend.add (vt);
        }
        return r;
    }

    enum class LotVerdict { Ok, Ignored, OtherValue, Missing };

    struct LotLine { VP sent; VP base; VP got; LotVerdict verdict = LotVerdict::Missing; };

    struct LotReport
    {
        juce::Array<LotLine> lines;   // un par param envoye
        juce::Array<VP> sideEffects;  // params NON envoyes dont la valeur a change (relu)
        int ok = 0, ignored = 0, other = 0, missing = 0;
    };

    /** S = avant (base), plan = envoyes, after = relu apres STORE. */
    inline LotReport checkLot (const juce::MemoryBlock& s, const LotPlan& plan, const juce::MemoryBlock& after)
    {
        LotReport r;
        const auto ms = paramMap (s), ma = paramMap (after);
        std::set<juce::int64> sent;
        for (auto& v : plan.toSend)
        {
            LotLine l; l.sent = v; sent.insert (keyOf (v));
            auto b = ms.find (keyOf (v)); if (b != ms.end()) l.base = b->second;
            auto a = ma.find (keyOf (v));
            if (a == ma.end())                  { l.verdict = LotVerdict::Missing;    ++r.missing; }
            else
            {
                l.got = a->second;
                if (sameValue (l.got, v))         { l.verdict = LotVerdict::Ok;         ++r.ok; }
                else if (sameValue (l.got, l.base)) { l.verdict = LotVerdict::Ignored;  ++r.ignored; }
                else                              { l.verdict = LotVerdict::OtherValue; ++r.other; }
            }
            r.lines.add (l);
        }
        for (auto& [k, va] : ma)
            if (! sent.count (k))
                if (auto b = ms.find (k); b != ms.end() && ! sameValue (b->second, va))
                    r.sideEffects.add (va);
        return r;
    }

    inline juce::String addrToString (const VP& v)
    {
        auto hx = [] (int x) { return juce::String::toHexString (x & 0xFF).paddedLeft ('0', 2).toUpperCase(); };
        return "G" + hx (v.group) + " AH" + hx (v.addrHi) + " AL" + hx (v.addrLo) + " P" + hx (v.param);
    }
    inline juce::String valToString (const VP& v)
    {
        return v.v1 != 0 ? juce::String (v.v1) + ":" + juce::String (v.value) : juce::String (v.value);
    }

    //--------------------------------------------------------------------------
    // Multi : couverture minimale (lecture seule). Pour chaque code VMEM, etat du switch
    // (bit 6) et bit 2 non documente, choisir peu de (multi, partie) a relever a l'ecran.
    struct MultiPick { int multi, part; juce::String why; };

    inline juce::Array<MultiPick> multiPicks (const juce::Array<SyMulti::Multi>& ms)
    {
        juce::Array<MultiPick> out;
        std::set<juce::String> seen;
        for (int m = 0; m < ms.size(); ++m)
        {
            if (! ms[m].loaded) continue;
            for (int p = 0; p < SyMulti::kParts; ++p)
            {
                const auto& raw = ms.getReference (m).part[p].raw;
                juce::StringArray why;
                const juce::String kv = "VMEM=" + juce::String ((int) raw[1]);
                const juce::String ks = juce::String ("bit6=") + ((raw[0] & 0x40) ? "1" : "0");
                const juce::String kb = juce::String ("bit2=") + ((raw[0] & 0x04) ? "1" : "0");
                for (auto& k : { kv, ks, kb })
                    if (seen.insert (k).second) why.add (k);
                if (! why.isEmpty()) out.add ({ m, p, why.joinIntoString (", ") });
            }
        }
        return out;
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
        juce::File                                      logFile;    // copie du rapport (append), vide = aucune
        juce::File                                      libraryDir; // banques .syx (voix de reference)
        // Croisement : adresses (g, hi, lo, p) de tous les widgets cables de l'editeur (lecture seule).
        std::function<std::vector<std::array<int, 4>>()> widgetAddresses;
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

        title2.setText ("Etape 1-2 :", juce::dontSendNotification);
        addAndMakeVisible (title2);
        for (int i = 0; i < 64; ++i) voiceSlot.addItem ("Slot " + SyBank::slotName (i), i + 1);
        voiceSlot.setSelectedId (64, juce::dontSendNotification);   // D16 par defaut
        voiceSlot.setTooltip ("Slot INT de voix sacrifiable pour LOT (sauvegarde sur disque + restauration)");
        addAndMakeVisible (voiceSlot);
        for (auto* b : { &btLot, &btCross, &btMulti }) addAndMakeVisible (b);
        btLot.setTooltip   ("Rejoue sur la voix du slot les params d'une voix reelle du meme type, tu fais STORE vers ce slot, l'app relit et donne un verdict par param, puis restaure le slot.");
        btCross.setTooltip ("Hors synthe : compare les adresses envoyees par les widgets aux adresses du decodeur de dump. N'envoie rien.");
        btMulti.setTooltip ("Lit les 16 multis INT (lecture seule) et liste les parties a relever sur l'ecran pour trancher l'ordre VMEM et le switch.");
        btLot.onClick   = [this]
        {
            const int slot = voiceSlot.getSelectedId() - 1;
            confirmThen ("Lot voix (ECRIT le slot " + SyBank::slotName (slot) + ")",
                "1. Sauvegarde du slot INT " + SyBank::slotName (slot) + " sur disque.\n"
                "2. Chargement de cette voix dans l'edit buffer, puis envoi d'un lot de param-changes.\n"
                "3. Tu fais STORE vers INT " + SyBank::slotName (slot) + " sur le SY77.\n"
                "4. Relecture, verdict, puis RESTAURATION du slot par bulk.\n\n"
                "Bulk Protect OFF dans l'app ET protection memoire OFF sur le SY77.",
                [this, slot] { runLot (slot); });
        };
        btCross.onClick = [this] { runCrossCheck(); };
        btMulti.onClick = [this] { runMultiRead(); };

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
        auto row2 = r.removeFromTop (26);
        title2.setBounds (row2.removeFromLeft (80));
        voiceSlot.setBounds (row2.removeFromLeft (110));
        row2.removeFromLeft (gap);
        const int w2 = (row2.getWidth() - gap * 2) / 3;
        for (auto* b : { &btLot, &btCross, &btMulti })
        {
            b->setBounds (row2.removeFromLeft (w2));
            row2.removeFromLeft (gap);
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
    juce::Label      title2;
    juce::ComboBox   voiceSlot, multiSlot;
    juce::TextButton btLot { "LOT  Voix (slot INT)" }, btCross { "X  Croiser widgets/decodeur" },
                     btMulti { "M  Lire multis INT 1-16" };
    juce::TextEditor log;

    //--------------------------------------------------------------------------
    void say (const juce::String& s)
    {
        log.moveCaretToEnd();
        log.insertTextAtCaret (s + "\n");
        if (ports.logFile != juce::File())
            ports.logFile.appendText (s + "\n", false, false, "\n");
    }

    void refreshButtons()
    {
        for (juce::Button* b : { (juce::Button*) &btA, (juce::Button*) &btB, (juce::Button*) &btCal,
                                 (juce::Button*) &btC, (juce::Button*) &btLot, (juce::Button*) &btCross,
                                 (juce::Button*) &btMulti })
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
        startTimer (15);   // ~1 param-change par tick pendant un lot (cf. sendParamsStep)
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

    //--------------------------------------------------------------------------
    // Outils etape 1-2
    juce::File outDir() const
    {
        auto d = ports.logFile.getParentDirectory().getChildFile ("bench");
        d.createDirectory();
        return d;
    }
    juce::File saveBlock (const juce::MemoryBlock& b, const juce::String& tag)
    {
        auto f = outDir().getChildFile (juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S_") + tag + ".syx");
        f.replaceWithData (b.getData(), b.getSize());
        return f;
    }

    bool answered = false, answerOk = false;
    Step askStep (const juce::String& t, const juce::String& msg, const juce::String& okLabel)
    {
        return { [this, t, msg, okLabel]
        {
            answered = answerOk = false;
            juce::Component::SafePointer<TestBenchPanel> self (this);
            juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, t, msg, okLabel, "Annuler", nullptr,
                juce::ModalCallbackFunction::create ([self] (int r)
                {
                    if (self == nullptr) return;
                    self->answerOk = (r == 1); self->answered = true;
                }));
        }, 3600000, [this] { return answered; } };
    }

    juce::Array<SyBench::VP> pending;   // params a envoyer (pace 1 par tick)
    int pendingIdx = 0;
    Step sendParamsStep()
    {
        return { [this] { pendingIdx = 0; }, 600000, [this]
        {
            if (pendingIdx >= pending.size()) return true;
            const auto& v = pending.getReference (pendingIdx++);
            const auto b = SyVoice::paramChangeBytes (dev(), v.group, v.addrHi, v.addrLo, v.param, v.v1, v.value);
            send (juce::MidiMessage::createSysExMessage (b.data(), (int) b.size()));
            return false;
        } };
    }

    /** Voix de la librairie (meme type et taille que s), celle qui couvre le plus de params. */
    juce::MemoryBlock pickReference (const juce::MemoryBlock& s, int& covered, int& scanned)
    {
        covered = scanned = 0;
        juce::MemoryBlock best;
        if (ports.libraryDir == juce::File()) return best;
        for (auto& f : ports.libraryDir.findChildFiles (juce::File::findFiles, true, "*.syx"))
        {
            juce::MemoryBlock data;
            if (f.getSize() > 4 * 1024 * 1024 || ! f.loadFileAsData (data)) continue;
            for (auto& v : SyBank::Bank::parse (data.getData(), data.getSize()).voices)
            {
                if (v.getSize() != s.getSize() || SyBank::typeOf (v) != SyBank::typeOf (s)) continue;
                if (++scanned > 4000) return best;
                const auto plan = SyBench::planLot (s, v);
                if (plan.toSend.size() > covered) { covered = plan.toSend.size(); best = v; }
            }
        }
        return best;
    }

    //--------------------------------------------------------------------------
    // LOT : transplant d'une voix de reference sur la voix du slot, STORE manuel, relecture.
    juce::MemoryBlock lotBackup, lotRef, lotAfter;
    SyBench::LotPlan lotPlan;
    int lotSlot = 63;

    void runLot (int slot)
    {
        if (! begin ("LOT  voix, slot INT " + SyBank::slotName (slot))) return;
        lotSlot = slot;
        lotBackup = lotRef = lotAfter = {};
        steps.push_back (requestStep (SyVoice::voiceDumpRequest (dev(), 0x00, (juce::uint8) slot), false));
        steps.push_back ({ [this]
        {
            lotBackup = lastVoiceBulk();
            if (lotBackup.getSize() == 0) { say ("LOT KO : slot illisible."); finish(); return; }
            say ("LOT slot " + SyBank::slotName (lotSlot) + " : " + voiceInfo (lotBackup));
            say ("LOT sauvegarde : " + saveBlock (lotBackup, "backup_" + SyBank::slotName (lotSlot)).getFullPathName());
            say ("    (en cas de souci : ouvrir ce .syx dans la librairie et l'envoyer au slot)");
            int covered = 0, scanned = 0;
            lotRef = pickReference (lotBackup, covered, scanned);
            if (lotRef.getSize() == 0) { say ("LOT KO : aucune voix de meme type dans la librairie (" + juce::String (scanned) + " examinees)."); finish(); return; }
            lotPlan = SyBench::planLot (lotBackup, lotRef);
            say ("LOT reference : " + voiceInfo (lotRef) + " (" + juce::String (scanned) + " candidates)");
            say ("LOT " + juce::String (lotPlan.toSend.size()) + " params a tester, " + juce::String (lotPlan.uncovered)
                 + " non couverts (meme valeur dans les 2 voix).");
            saveBlock (lotRef, "reference");
            auto eb = lotBackup;
            SyVoice::retargetVoiceToEditBuffer ((juce::uint8*) eb.getData(), (int) eb.getSize());
            SyVoice::setBulkDevice ((juce::uint8*) eb.getData(), (int) eb.getSize(), dev());
            send (juce::MidiMessage (eb.getData(), (int) eb.getSize()));
            pending = lotPlan.toSend;
        }, 900, nullptr });
        steps.push_back (sendParamsStep());
        steps.push_back ({ [this] { say ("LOT lot envoye (" + juce::String (pending.size()) + " param-changes)."); }, 300, nullptr });
        steps.push_back (askStep ("STORE sur le SY77",
            "Sur le SY77 : STORE de la voix en cours vers INT " + SyBank::slotName (lotSlot) + ", puis clique OK.",
            "STORE fait"));
        steps.push_back ({ [this]
        {
            if (! answerOk) { say ("LOT annule avant STORE : rien n'a ete ecrit (re-selectionner la voix sur le synthe)."); finish(); return; }
            rx.clear(); lastRxMs = 0;
            send (SyVoice::voiceDumpRequest (dev(), 0x00, (juce::uint8) lotSlot));
        }, 2500, [this] { return lastVoiceBulk().getSize() > 0 && juce::Time::getMillisecondCounter() - lastRxMs > 250; } });
        steps.push_back ({ [this]
        {
            lotAfter = lastVoiceBulk();
            if (lotAfter.getSize() == 0) { say ("LOT KO : relecture impossible. Slot NON restaure, la sauvegarde est sur disque."); finish(); return; }
            saveBlock (lotAfter, "after_" + SyBank::slotName (lotSlot));
            if (SyBench::diffOffsets (lotAfter, lotBackup).isEmpty())
            {
                say ("LOT ATTENTION : le slot n'a pas change. Le STORE est-il alle dans un AUTRE slot ? (verifier ; rien n'est restaure)");
                finish(); return;
            }
            reportLot (SyBench::checkLot (lotBackup, lotPlan, lotAfter));
            SyVoice::setBulkDevice ((juce::uint8*) lotBackup.getData(), (int) lotBackup.getSize(), dev());
            send (juce::MidiMessage (lotBackup.getData(), (int) lotBackup.getSize()));   // restauration (octet 30/31 = INT slot)
        }, 900, nullptr });
        steps.push_back (requestStep (SyVoice::voiceDumpRequest (dev(), 0x00, (juce::uint8) slot), false));
        steps.push_back ({ [this]
        {
            const auto back = lastVoiceBulk();
            if (back.getSize() > 0 && SyBench::diffOffsets (back, lotBackup).isEmpty())
                say ("LOT restauration OK : slot " + SyBank::slotName (lotSlot) + " identique a la sauvegarde.");
            else
                say ("LOT ATTENTION : restauration NON confirmee. Renvoyer la sauvegarde du dossier bench au slot.");
        }, 0, nullptr });
        go();
    }

    void reportLot (const SyBench::LotReport& r)
    {
        say ("LOT resultat : " + juce::String (r.ok) + " OK, " + juce::String (r.ignored) + " ignores, "
             + juce::String (r.other) + " autre valeur, " + juce::String (r.missing) + " absents ; "
             + juce::String (r.sideEffects.size()) + " effets de bord.");
        juce::String csv ("verdict,group,addrHi,addrLo,param,sent,base,got\n");
        for (auto& l : r.lines)
        {
            const char* v = l.verdict == SyBench::LotVerdict::Ok ? "OK"
                          : l.verdict == SyBench::LotVerdict::Ignored ? "IGNORE"
                          : l.verdict == SyBench::LotVerdict::OtherValue ? "AUTRE" : "ABSENT";
            csv << v << "," << l.sent.group << "," << l.sent.addrHi << "," << l.sent.addrLo << "," << l.sent.param << ","
                << SyBench::valToString (l.sent) << "," << SyBench::valToString (l.base) << "," << SyBench::valToString (l.got) << "\n";
            if (l.verdict != SyBench::LotVerdict::Ok)
                say ("  " + juce::String (v) + "  " + SyBench::addrToString (l.sent) + "  envoye " + SyBench::valToString (l.sent)
                     + "  avant " + SyBench::valToString (l.base) + "  relu " + SyBench::valToString (l.got));
        }
        for (auto& e : r.sideEffects)
        {
            csv << "EFFET," << e.group << "," << e.addrHi << "," << e.addrLo << "," << e.param << ",," << "," << SyBench::valToString (e) << "\n";
            say ("  EFFET  " + SyBench::addrToString (e) + "  relu " + SyBench::valToString (e) + " (non envoye)");
        }
        auto f = outDir().getChildFile (juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S_lot.csv"));
        f.replaceWithText (csv);
        say ("LOT detail : " + f.getFullPathName());
    }

    //--------------------------------------------------------------------------
    // X : croisement widgets <-> decodeur, hors synthe.
    void runCrossCheck()
    {
        if (running) return;
        say (""); say ("=== X  croisement widgets / decodeur");
        if (! ports.widgetAddresses) { say ("X  indisponible."); return; }
        std::set<juce::int64> wid, dec;
        for (auto& a : ports.widgetAddresses())
            if (a[0] != 0x7F)   // groupe sentinelle 0x7F = widget a envoi manuel (ex. EGOS 2 octets)
                wid.insert (SyBench::keyOf (a[0], a[1], a[2], a[3]));
        // Decodeur : union sur une voix reelle de chaque type (0..10) de la librairie.
        std::set<int> typesSeen;
        if (ports.libraryDir != juce::File())
            for (auto& f : ports.libraryDir.findChildFiles (juce::File::findFiles, true, "*.syx"))
            {
                if (typesSeen.size() >= 11) break;
                juce::MemoryBlock data;
                if (f.getSize() > 4 * 1024 * 1024 || ! f.loadFileAsData (data)) continue;
                for (auto& v : SyBank::Bank::parse (data.getData(), data.getSize()).voices)
                    if (typesSeen.insert (SyBank::typeOf (v)).second)
                        for (auto& [k, p] : SyBench::paramMap (v)) dec.insert (k);
            }
        juce::StringArray types; for (int t : typesSeen) types.add (juce::String (t));
        int both = 0;
        juce::String csv ("side,group,addrHi,addrLo,param\n");
        juce::Array<juce::int64> wOnly, dOnly;
        for (auto k : wid) { if (dec.count (k)) ++both; else wOnly.add (k); }
        for (auto k : dec) if (! wid.count (k)) dOnly.add (k);
        auto row = [] (const char* side, juce::int64 k)
        {
            return juce::String (side) + "," + juce::String ((int) (k >> 24) & 0xFF) + "," + juce::String ((int) (k >> 16) & 0xFF) + ","
                 + juce::String ((int) (k >> 8) & 0xFF) + "," + juce::String ((int) k & 0xFF) + "\n";
        };
        for (auto k : wOnly) csv << row ("widget_seul", k);
        for (auto k : dOnly) csv << row ("decodeur_seul", k);
        auto f = outDir().getChildFile ("crosscheck.csv");
        f.replaceWithText (csv);
        say ("X  widgets : " + juce::String ((int) wid.size()) + " adresses ; decodeur : " + juce::String ((int) dec.size())
             + " (types " + types.joinIntoString (",") + ")");
        say ("X  communes " + juce::String (both) + " ; widget seul " + juce::String (wOnly.size())
             + " ; decodeur seul " + juce::String (dOnly.size()) + " -> " + f.getFullPathName());
    }

    //--------------------------------------------------------------------------
    // M : lecture des 16 multis INT (aucune ecriture) + parties a relever a l'ecran.
    juce::Array<SyMulti::Multi> multis;
    void runMultiRead()
    {
        if (! begin ("M  lecture multis INT 1-16")) return;
        multis.clearQuick();
        for (int n = 0; n < 16; ++n)
        {
            steps.push_back (requestStep (SyMulti::dumpRequest (dev(), 0x00, n), true));
            steps.push_back ({ [this, n]
            {
                const auto b = lastMultiBulk();
                auto m = SyMulti::parse ((const juce::uint8*) b.getData(), (int) b.getSize());
                if (m.loaded) saveBlock (b, "multi_INT" + juce::String (n + 1).paddedLeft ('0', 2));
                say ("M  INT " + juce::String (n + 1) + (m.loaded ? " \"" + m.name + "\"" : juce::String (" : pas de reponse")));
                if (m.loaded)
                {
                    juce::String line ("     parts (VMEM/VNUM/packe) :");
                    for (int p = 0; p < SyMulti::kParts; ++p)
                        line << " " << (int) m.part[p].raw[1] << "/" << (int) m.part[p].raw[2]
                             << "/" << juce::String::toHexString ((int) m.part[p].raw[0]);
                    say (line);
                }
                multis.add (m);
            }, 0, nullptr });
        }
        steps.push_back ({ [this]
        {
            const auto picks = SyBench::multiPicks (multis);
            say ("M  A RELEVER sur l'ecran du SY77 (voix de la partie : banque + numero ; partie active ou non) :");
            for (auto& pk : picks)
                say ("   Multi INT " + juce::String (pk.multi + 1) + " partie " + juce::String (pk.part + 1)
                     + "   (" + SyMulti::voiceLabel (multis.getReference (pk.multi).part[pk.part]) + " ; " + pk.why + ")");
            say ("M  Donne ces releves a Claude : il en deduit l'ordre VMEM et la polarite du switch.");
        }, 0, nullptr });
        go();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestBenchPanel)
};
