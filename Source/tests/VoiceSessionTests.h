// Tests unitaires : VoiceSession (inclus par Source/Tests.h, ordre d'enregistrement conserve).
#pragma once

//==============================================================================
// Caracterisation de VoiceSession (seul proprietaire de la logique de session, branchee
// dans MidiDemo). Ports factices ; le faux `send` modelise sendToOutputs :
// observeOutgoing AVANT le gate Out, puis coupe si Out OFF (note-off exceptes), puis trace.
class VoiceSessionTests : public juce::UnitTest
{
public:
    VoiceSessionTests() : juce::UnitTest ("VoiceSession", "Sysex77") {}

    using Bytes = std::vector<juce::uint8>;

    struct Rig
    {
        std::vector<Bytes> out;                        // ce qui sort reellement (apres gate)
        std::vector<std::array<int, 6>> replay;
        std::vector<int> replayDepth;                  // suppressSend vu pendant chaque rejeu
        std::vector<bool> locks;
        std::vector<VoiceSession::HistoryState> states;
        std::vector<std::function<void()>> posted;
        struct T { double due; int ms; std::function<void()> fn; };
        std::vector<T> timers;
        int editHappened = 0;
        double now = 1000.0;
        bool outOn = true, mouse = false;
        int device = 3;
        std::unique_ptr<VoiceSession> s;
        std::function<void (const juce::MidiMessage&)> sendToOutputs;

        Rig()
        {
            VoiceSession::Ports p;
            sendToOutputs = [this] (const juce::MidiMessage& m)
            {
                if (s != nullptr) s->observeOutgoing (m.getRawData(), m.getRawDataSize(), mouse);
                if (! outOn && ! m.isNoteOff()) return;
                out.push_back (Bytes (m.getRawData(), m.getRawData() + m.getRawDataSize()));
            };
            p.send = sendToOutputs;
            p.replayParam = [this] (int g, int hi, int lo, int pp, int v1, int v2)
            {
                replay.push_back ({ { g, hi, lo, pp, v1, v2 } });
                replayDepth.push_back (SysexBus::get().suppressSend.load());
            };
            p.lockEditing  = [this] (bool l) { locks.push_back (l); };
            p.editHappened = [this] { ++editHappened; };
            p.nowMs        = [this] { return now; };
            p.post         = [this] (std::function<void()> f) { posted.push_back (std::move (f)); };
            p.schedule     = [this] (int ms, std::function<void()> f) { timers.push_back ({ now + ms, ms, std::move (f) }); };
            p.sendEnabled  = [this] { return outOn; };
            p.device       = [this] { return device; };
            p.historyState = [this] (const VoiceSession::HistoryState& h) { states.push_back (h); };
            s = std::make_unique<VoiceSession> (p);
        }

        void drain()   // boucle de messages : FIFO, les posts imbriques passent en fin de file
        {
            for (size_t i = 0; i < posted.size(); ++i) { auto f = posted[i]; f(); }
            posted.clear();
        }
        void stepOne()   // un seul message de la file (pour entrelacer les sauts FIFO)
        {
            if (posted.empty()) return;
            auto f = posted.front();
            posted.erase (posted.begin());
            f();
        }
        void advance (double ms)
        {
            now += ms;
            auto due = timers;
            timers.clear();
            for (auto& t : due) { if (t.due <= now) t.fn(); else timers.push_back (t); }
        }
        // Edit utilisateur tel qu'il arrive a sendToOutputs (widget -> bus -> choke-point).
        void userEdit (EditHist::Key k, int v)
        {
            auto b = SyVoice::paramChangeBytes (device, EditHist::gOf (k), EditHist::hiOf (k), EditHist::loOf (k),
                                                EditHist::pOf (k), EditHist::v1Of (v), EditHist::v2Of (v));
            sendToOutputs (juce::MidiMessage::createSysExMessage (b.data(), 9));
        }
    };

    // Attendu derive du code MidiDemo actuel : bulk = copie, device byte 0n, $7F/0, checksum.
    static Bytes expectedLoadBulk (const juce::MemoryBlock& tpl, int device)
    {
        Bytes e ((const juce::uint8*) tpl.getData(), (const juce::uint8*) tpl.getData() + tpl.getSize());
        const int n = (int) e.size();
        e[2] = (juce::uint8) (device - 1);
        e[30] = 0x7F; e[31] = 0x00;
        e[(size_t) n - 2] = SyVoice::yamahaChecksum (e.data() + 6, n - 8);
        return e;
    }
    static Bytes paramMsg (int device, EditHist::Key k, int v)
    {
        auto b = SyVoice::paramChangeBytes (device, EditHist::gOf (k), EditHist::hiOf (k), EditHist::loOf (k),
                                            EditHist::pOf (k), EditHist::v1Of (v), EditHist::v2Of (v));
        Bytes m { 0xF0 }; m.insert (m.end(), b.begin(), b.end()); m.push_back (0xF7);
        return m;
    }

    void runTest() override
    {
        using namespace EditHist;
        auto& bus = SysexBus::get();
        const int depth0 = bus.suppressSend.load();
        const auto tpl = VoiceTemplates::make (0);
        const auto params = SyVoice::voiceBlobToParams ((const juce::uint8*) tpl.getData(), (int) tpl.getSize());
        // Cle editee : premier param decode (connue de l'original).
        const Key k = keyOf (params[0].group, params[0].addrHi, params[0].addrLo, params[0].param);
        const int orig = valOf (params[0].v1, params[0].value);
        const int edited = orig == 5 ? 6 : 5;

        auto loadTpl = [&] (Rig& r)
        {
            const bool ok = r.s->loadBlock (tpl) == VoiceSession::LoadResult::Loaded;
            r.drain();
            return ok;
        };

        beginTest ("load: one bulk to the edit buffer ($7F) at the current device, verbatim replay");
        {
            Rig r;
            int type = -1; juce::String name; size_t outAtHeader = 99, replayAtHeader = 99;
            const bool ok = r.s->loadBlock (tpl, [&] (int t, const juce::String& n)
                                            { type = t; name = n; outAtHeader = r.out.size(); replayAtHeader = r.replay.size(); })
                            == VoiceSession::LoadResult::Loaded;
            expect (ok);
            expectEquals (type, 3);
            expect (name == juce::String ((const char*) tpl.getData() + 33, 10).trimEnd());
            expect (name.isNotEmpty());
            expectEquals ((int) r.out.size(), 1);
            expect (r.out[0] == expectedLoadBulk (tpl, 3));
            expect (outAtHeader == 1 && replayAtHeader == 0);       // ordre : bulk -> props -> rejeu
            expectEquals ((int) r.replay.size(), params.size());
            bool verbatim = (int) r.replay.size() == params.size(), guarded = true;
            for (int i = 0; verbatim && i < params.size(); ++i)
            {
                const auto& vp = params.getReference (i);
                verbatim = r.replay[(size_t) i] == std::array<int, 6> { { vp.group, vp.addrHi, vp.addrLo, vp.param, vp.v1, vp.value } };
            }
            for (int dpt : r.replayDepth) guarded = guarded && dpt >= depth0 + 2;   // settle + ScopedEchoSuppress
            expect (verbatim);
            expect (guarded);
            expect (r.s->history().isLoaded() && ! r.s->history().canUndo());
            expect (! r.states.empty() && r.states.back().loaded && ! r.states.back().showingA);
            expect (r.locks.empty() && r.editHappened == 0 && r.timers.empty());
            expectEquals (bus.suppressSend.load(), depth0 + 1);    // settle tenu
            r.drain();
            expectEquals (bus.suppressSend.load(), depth0);        // relache apres 2 sauts FIFO
        }

        beginTest ("edits during settle are not recorded; recorded once settled");
        {
            Rig r;
            r.s->loadBlock (tpl);
            r.userEdit (k, edited);
            expect (! r.s->history().canUndo());
            r.drain();
            r.userEdit (k, edited == 5 ? 7 : 8);
            expect (r.s->history().canUndo());
            expectEquals (bus.suppressSend.load(), depth0);
        }

        beginTest ("edit -> undo/redo: sent at the CURRENT device, not recorded");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            expectEquals (r.s->history().numSteps(), 1);
            r.device = 5;
            const size_t n0 = r.out.size(), rp0 = r.replay.size();
            r.s->undo();
            expectEquals ((int) r.out.size(), (int) n0 + 1);
            expect (r.out.back() == paramMsg (5, k, orig));
            expect (r.out.back()[2] == 0x14);
            expectEquals ((int) r.replay.size(), (int) rp0 + 1);
            expect (r.replayDepth.back() > depth0);
            expect (! r.s->history().canUndo() && r.s->history().canRedo());   // undo's own send not recorded
            r.s->redo();
            expect (r.out.back() == paramMsg (5, k, edited));
            expect (r.s->history().canUndo() && ! r.s->history().canRedo());
            expectEquals (r.s->history().numSteps(), 1);
            expect (r.states.back().status.isEmpty());
        }

        beginTest ("A: whole original bulk (current device), editing locked, editHappened; B: diff after delay");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            r.device = 2;
            const size_t n0 = r.out.size();
            r.s->setShowingA (true);
            expectEquals ((int) r.out.size(), (int) n0 + 1);
            expect (r.out.back() == expectedLoadBulk (tpl, 2));
            expect (r.s->isShowingA() && r.locks.back() && r.editHappened == 1);
            expect (r.states.back().showingA);
            // While A is shown nothing is recorded.
            r.userEdit (k, 99);
            expectEquals (r.s->history().numSteps(), 1);
            r.out.pop_back();   // (le faux edit est sorti : en vrai l'UI est verrouillee)

            const size_t n1 = r.out.size(), rp1 = r.replay.size();
            r.s->setShowingA (false);
            expectEquals ((int) r.out.size(), (int) n1);                 // pas encore : SY77 occupe
            expectEquals ((int) r.replay.size(), (int) rp1 + 1);         // UI tout de suite (diff)
            expect (! r.s->isShowingA() && ! r.locks.back() && r.editHappened == 1);
            expectEquals ((int) r.timers.size(), 1);
            expectEquals (r.timers[0].ms, (int) ((double) tpl.getSize() / 3.0 + 60.0) + 1);
            r.advance (r.timers[0].ms);
            expectEquals ((int) r.out.size(), (int) n1 + 1);
            expect (r.out.back() == paramMsg (2, k, edited));
            expectEquals (r.s->history().numSteps(), 1);                  // B's send not recorded
        }

        beginTest ("B delayed send cancelled when historyGen changes (edit / A again / undo)");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            r.s->setShowingA (true);
            r.s->setShowingA (false);
            const Key k2 = keyOf (params[1].group, params[1].addrHi, params[1].addrLo, params[1].param);
            const int v2 = valOf (params[1].v1, params[1].value) == 3 ? 4 : 3;
            r.now += 1000;                          // pas de regroupement avec l'edit precedent
            r.userEdit (k2, v2);                    // ++historyGen
            const size_t n0 = r.out.size();   // (l'edit a avance le diff B, cf. constat 4)
            r.advance (10000);
            expectEquals ((int) r.out.size(), (int) n0);   // le timer ne renvoie rien

            r.s->setShowingA (true);
            r.s->setShowingA (false);
            r.s->setShowingA (true);                // A again: showingA + gen
            const size_t n1 = r.out.size();
            r.advance (10000);
            expectEquals ((int) r.out.size(), (int) n1);

            // Undo while A (bulk fraichement envoye) : setShowingA(false) programme le diff B
            // (UI deja sur B), puis undo l'AVANCE (flushPendingB) avant ++historyGen.
            // CORRIGE (revue, bug 1) : ce test exigeait auparavant que seules les valeurs d'undo
            // sortent (B annule) -> le synthe restait sur le bulk A alors que l'UI affichait B.
            // Attendu desormais : diff B (ordre de la map) puis la valeur d'undo, rien ensuite.
            r.s->setShowingA (false);
            r.s->setShowingA (true);
            const auto diffB = r.s->history().diffFromOriginal();
            expect (diffB.size() == 2);
            const size_t n2 = r.out.size();
            r.s->undo();
            expectEquals ((int) r.out.size(), (int) (n2 + diffB.size() + 1));
            for (size_t i = 0; i < diffB.size() && n2 + i < r.out.size(); ++i)
                expect (r.out[n2 + i] == paramMsg (3, diffB[i].first, diffB[i].second));
            expect (r.out.back() == paramMsg (3, k2, valOf (params[1].v1, params[1].value)));
            expectEquals ((int) r.timers.size(), 1);   // programme...
            r.advance (10000);                         // ...deja envoye : le timer ne renvoie rien
            expectEquals ((int) (r.out.size()), (int) (n2 + diffB.size() + 1));
        }

        beginTest ("B sends immediately once the bulk is absorbed");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            r.s->setShowingA (true);
            r.now += 5000;
            const size_t n0 = r.out.size();
            r.s->setShowingA (false);
            expect (r.timers.empty());
            expectEquals ((int) r.out.size(), (int) n0 + 1);
            expect (r.out.back() == paramMsg (3, k, edited));
        }

        beginTest ("revert: original bulk, undoable step, editHappened");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            expect (r.s->hasUnsavedEdits());
            const size_t n0 = r.out.size();
            r.s->revert();
            expectEquals ((int) r.out.size(), (int) n0 + 1);
            expect (r.out.back() == expectedLoadBulk (tpl, 3));
            expect (r.editHappened == 1 && ! r.s->hasUnsavedEdits());
            expectEquals (r.s->history().numSteps(), 2);
            r.s->undo();
            expect (r.out.back() == paramMsg (3, k, edited));
            expect (r.s->hasUnsavedEdits());
            // revert from A: unlocks first
            r.s->setShowingA (true);
            r.s->revert();
            expect (! r.s->isShowingA() && ! r.locks.back());
        }

        beginTest ("Out OFF: nothing is emitted but the edit is recorded; B not delayed");
        {
            Rig r; r.outOn = false;
            loadTpl (r);
            expect (r.out.empty() && r.s->history().isLoaded());
            r.userEdit (k, edited);
            expect (r.out.empty());
            expect (r.s->history().canUndo());
            r.s->setShowingA (true);
            r.s->setShowingA (false);
            expect (r.timers.empty());               // bulkBusyUntilMs pas mis a jour
            r.s->undo();
            r.s->revert();
            expect (r.out.empty());
            expectEquals (bus.suppressSend.load(), depth0);
        }

        beginTest ("sz <= 42: bulk still sent + settle, no replay, previous history discarded");
        {
            juce::uint8 small[40] = { 0xF0, 0x43, 0x00, 0x7A };
            small[39] = 0xF7;
            Bytes e (small, small + 40);
            e[2] = 0x02; e[30] = 0x7F; e[31] = 0x00; e[38] = SyVoice::yamahaChecksum (e.data() + 6, 32);
            {
                Rig r;
                int type = -1; juce::String name; bool called = false;
                const auto res = r.s->loadBlock (juce::MemoryBlock (small, 40),
                                                 [&] (int t, const juce::String& n) { called = true; type = t; name = n; });
                expect (res == VoiceSession::LoadResult::Discarded && ! called && type == -1 && name.isEmpty());
                expectEquals ((int) r.out.size(), 1);
                expect (r.out[0] == e);
                expect (r.replay.empty() && ! r.s->history().isLoaded());
                // L'etat publie dit « rien de charge » (HistoryBar desactivee).
                expect (r.states.size() == 1 && ! r.states.back().loaded);
                expectEquals (bus.suppressSend.load(), depth0 + 1);
                r.drain();
                expectEquals (bus.suppressSend.load(), depth0);
            }
            {
                Rig r; loadTpl (r);
                r.userEdit (k, edited);
                r.s->loadBlock (juce::MemoryBlock (small, 40));
                r.drain();
                // L'edit buffer ne contient plus la voix precedente : rien a annuler ni a
                // comparer, et A / undo / revert n'emettent plus rien.
                expect (! r.s->history().isLoaded() && ! r.s->history().canUndo());
                expect (! r.s->hasUnsavedEdits() && ! r.s->canRevert());
                const auto sent = r.out.size();
                r.s->setShowingA (true);
                r.s->undo();
                r.s->revert();
                r.drain();
                expectEquals ((int) r.out.size(), (int) sent);
                expect (! r.s->isShowingA());
            }
        }

        beginTest ("incoming params keep the state (noteRx), ignored for group 0F and while A");
        {
            Rig r; loadTpl (r);
            const int rx = edited == 5 ? 9 : 10;
            const juce::uint8 d[6] = { (juce::uint8) gOf (k), (juce::uint8) hiOf (k), (juce::uint8) loOf (k),
                                       (juce::uint8) pOf (k), (juce::uint8) v1Of (rx), (juce::uint8) v2Of (rx) };
            r.s->observeIncomingParam (d);
            expect (! r.s->history().canUndo() && r.s->hasUnsavedEdits());
            r.userEdit (k, edited);
            r.s->undo();
            expect (r.out.back() == paramMsg (3, k, rx));
            const juce::uint8 sys[6] = { 0x0F, 0, 0, 0x01, 0, 0x05 };
            const int diffBefore = (int) r.s->history().diffFromOriginal().size();
            r.s->observeIncomingParam (sys);
            expectEquals ((int) r.s->history().diffFromOriginal().size(), diffBefore);

            // While A: the received value is NOT noted -> undo restores the pre-A value.
            r.now += 1000;
            r.userEdit (k, edited);                     // state = edited (new step)
            r.s->setShowingA (true);
            const int third = edited == 5 ? 11 : 12;
            const juce::uint8 dA[6] = { d[0], d[1], d[2], d[3], (juce::uint8) v1Of (third), (juce::uint8) v2Of (third) };
            r.s->observeIncomingParam (dA);
            r.s->setShowingA (false);
            r.advance (10000);
            r.now += 1000;
            r.userEdit (k, edited == 5 ? 13 : 14);
            r.s->undo();
            expect (r.out.back() == paramMsg (3, k, edited));
        }

        beginTest ("constat 1: non-bulk or < 34-byte block is refused: nothing sent, history kept");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            const size_t n0 = r.out.size();
            juce::uint8 tiny[20] = { 0xF0, 0x43, 0x00, 0x7A };        // bulk voix, mais < 34
            tiny[19] = 0xF7;
            juce::uint8 notBulk[60] = { 0xF0, 0x43, 0x00, 0x7B };     // > 42 mais pas 7A
            notBulk[59] = 0xF7;
            for (auto mb : { juce::MemoryBlock (tiny, sizeof (tiny)), juce::MemoryBlock (notBulk, sizeof (notBulk)) })
            {
                expect (r.s->loadBlock (mb) == VoiceSession::LoadResult::Refused);
                expectEquals ((int) r.out.size(), (int) n0);                    // rien n'est emis
                expectEquals (bus.suppressSend.load(), depth0);                 // pas de settle
                expect (r.s->history().isLoaded() && r.s->history().canUndo()); // historique intact
                r.drain();
            }
            expectEquals (bus.suppressSend.load(), depth0);
        }

        beginTest ("constat 2: after unload an incoming param + revert + undo emit nothing");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            juce::uint8 small[40] = { 0xF0, 0x43, 0x00, 0x7A };
            small[39] = 0xF7;
            r.s->loadBlock (juce::MemoryBlock (small, 40));
            r.drain();
            expect (! r.s->history().isLoaded());
            const size_t n0 = r.out.size();
            const int rx = edited == 5 ? 9 : 10;
            const juce::uint8 d[6] = { (juce::uint8) gOf (k), (juce::uint8) hiOf (k), (juce::uint8) loOf (k),
                                       (juce::uint8) pOf (k), (juce::uint8) v1Of (rx), (juce::uint8) v2Of (rx) };
            r.s->observeIncomingParam (d);
            expect (! r.s->hasUnsavedEdits());
            r.s->revert();
            r.s->undo();
            r.drain();
            r.advance (10000);
            expectEquals ((int) r.out.size(), (int) n0);
            expect (! r.s->history().canUndo() && r.editHappened == 0);
        }

        beginTest ("constat 3: two queued loads: echoes of the 2nd are not recorded after the 1st settles");
        {
            Rig r;
            r.s->loadBlock (tpl);
            r.s->loadBlock (tpl);
            expectEquals ((int) r.posted.size(), 2);
            r.stepOne();            // L1 saut 1 -> poste L1 saut 2
            r.stepOne();            // L2 saut 1 -> poste L2 saut 2
            r.stepOne();            // L1 saut 2 : le 1er chargement est stabilise...
            r.userEdit (k, edited); // ...mais un echo du 2e arrive encore
            expect (! r.s->history().canUndo());
            expect (! r.s->hasUnsavedEdits());
            r.drain();              // L2 saut 2
            expectEquals (bus.suppressSend.load(), depth0);
            r.now += 1000;
            r.userEdit (k, edited == 5 ? 7 : 8);   // une fois tout stabilise : enregistre
            expect (r.s->history().canUndo());
        }

        beginTest ("constat 4: an edit during the bulk window sends the pending B diff first, then the edit");
        {
            Rig r; loadTpl (r);
            r.userEdit (k, edited);
            r.s->setShowingA (true);
            r.s->setShowingA (false);
            expectEquals ((int) r.timers.size(), 1);
            const Key k2 = keyOf (params[1].group, params[1].addrHi, params[1].addrLo, params[1].param);
            const int v2 = valOf (params[1].v1, params[1].value) == 3 ? 4 : 3;
            const size_t n0 = r.out.size();
            r.now += 10;                            // avant l'echeance du B differe
            r.userEdit (k2, v2);
            expectEquals ((int) r.out.size(), (int) n0 + 2);
            if (r.out.size() == n0 + 2)
            {
                expect (r.out[n0]     == paramMsg (3, k, edited));   // diff B d'abord
                expect (r.out[n0 + 1] == paramMsg (3, k2, v2));      // puis l'edit
            }
            r.advance (10000);
            expectEquals ((int) r.out.size(), (int) n0 + 2);         // rien apres l'echeance
        }

        beginTest ("bug 1: undo/redo during the B delay send the pending B diff first, then their values");
        {
            {
                Rig r; loadTpl (r);
                r.userEdit (k, edited);
                r.s->setShowingA (true);
                r.s->setShowingA (false);          // UI sur B, envoi B differe (SY77 occupe)
                expectEquals ((int) r.timers.size(), 1);
                const size_t n0 = r.out.size();
                r.now += 10;
                r.s->undo();
                expectEquals ((int) r.out.size(), (int) n0 + 2);
                if (r.out.size() == n0 + 2)
                {
                    expect (r.out[n0]     == paramMsg (3, k, edited));   // diff B d'abord
                    expect (r.out[n0 + 1] == paramMsg (3, k, orig));     // puis l'undo
                }
                r.advance (10000);
                expectEquals ((int) r.out.size(), (int) n0 + 2);
            }
            {
                Rig r; loadTpl (r);
                const Key k2 = keyOf (params[1].group, params[1].addrHi, params[1].addrLo, params[1].param);
                const int v2 = valOf (params[1].v1, params[1].value) == 3 ? 4 : 3;
                r.userEdit (k, edited);
                r.now += 1000;
                r.userEdit (k2, v2);
                r.s->undo();                        // diff = { k }
                r.s->setShowingA (true);
                r.s->setShowingA (false);
                expectEquals ((int) r.timers.size(), 1);
                const size_t n0 = r.out.size();
                r.now += 10;
                r.s->redo();
                expectEquals ((int) r.out.size(), (int) n0 + 2);
                if (r.out.size() == n0 + 2)
                {
                    expect (r.out[n0]     == paramMsg (3, k, edited));   // diff B d'abord
                    expect (r.out[n0 + 1] == paramMsg (3, k2, v2));      // puis le redo
                }
                r.advance (10000);
                expectEquals ((int) r.out.size(), (int) n0 + 2);
            }
        }

        beginTest ("bug 2: values received for an undecoded key (AFM common reserve) are not unsaved edits");
        {
            // Cle filtre (groupe 09, type FTYPE N2 0x00 : encodage bulk non verifie, donc NON
            // decode) absente des params decodes, mais enregistrable (g != 0F). Les effets et
            // le LFO sont desormais decodes du dump : ils ne peuvent plus servir ici.
            Key fx = 0;
            for (int p = 0; p < 128 && fx == 0; ++p)
            {
                const Key c = keyOf (0x05, 0, 0, p);   // 1er non decode = N2 0x14 (reserve)
                bool decoded = false;
                for (auto& vp : params)
                    decoded = decoded || keyOf (vp.group, vp.addrHi, vp.addrLo, vp.param) == c;
                if (! decoded) fx = c;
            }
            expect (fx != 0);
            {
                const Bytes probe = paramMsg (3, fx, 1);
                expect (SyVoice::isRecordableVoiceParamChange (probe.data(), (int) probe.size()));
            }
            auto rx = [&] (Rig& r, Key key, int v)
            {
                const juce::uint8 d[6] = { (juce::uint8) gOf (key), (juce::uint8) hiOf (key), (juce::uint8) loOf (key),
                                           (juce::uint8) pOf (key), (juce::uint8) v1Of (v), (juce::uint8) v2Of (v) };
                r.s->observeIncomingParam (d);
            };
            {
                Rig r; loadTpl (r);
                rx (r, fx, 10);
                rx (r, fx, 20);                     // flux d'un bouton tourne sur le SY77
                expect (! r.s->hasUnsavedEdits());
                expect (! r.s->history().hasDiff() && r.s->history().diffFromOriginal().empty());
                r.s->setShowingA (true);
                const size_t n0 = r.out.size();
                r.s->setShowingA (false);
                r.advance (10000);
                expectEquals ((int) r.out.size(), (int) n0);   // B ne renvoie pas la valeur recue

                // Un edit UTILISATEUR de cette cle reste compte, meme apres une valeur recue.
                r.now += 1000;
                r.userEdit (fx, 30);
                expect (r.s->hasUnsavedEdits() && r.s->history().hasDiff());
                rx (r, fx, 40);
                expect (r.s->hasUnsavedEdits() && r.s->history().hasDiff());
                // Undo -> retour a la derniere valeur recue avant l'edit : plus d'ecart.
                r.s->undo();
                expect (r.out.back() == paramMsg (3, fx, 20));
                expect (! r.s->hasUnsavedEdits());
            }
            {
                Rig r; loadTpl (r);
                r.userEdit (fx, 30);                // cle jamais recue : edit compte (inchange)
                expect (r.s->hasUnsavedEdits());
                const auto diff = r.s->history().diffFromOriginal();
                expect (diff.size() == 1 && diff[0] == KV (fx, 30));
            }
        }

        beginTest ("bug 3: blank-named real voice is Loaded with an empty name; placeholder only if unknown");
        {
            juce::MemoryBlock blank (tpl);
            for (int i = 33; i < 43; ++i) ((juce::uint8*) blank.getData())[i] = ' ';
            Rig r;
            juce::uint8 small[40] = { 0xF0, 0x43, 0x00, 0x7A };
            small[39] = 0xF7;
            bool called = false;
            expect (r.s->loadBlock (juce::MemoryBlock (small, 40), [&] (int, const juce::String&) { called = true; })
                    == VoiceSession::LoadResult::Discarded);
            expect (! called);
            juce::String name = "x";
            expect (r.s->loadBlock (blank, [&] (int, const juce::String& n) { called = true; name = n; })
                    == VoiceSession::LoadResult::Loaded);
            expect (called && name.isEmpty());
            // Le champ nom : texte neutre pour une voix INCONNUE seulement, rien pour un nom vide.
            expect (SyVoice::voiceNameFieldPlaceholder (false).isEmpty());
            expect (SyVoice::voiceNameFieldPlaceholder (true) == TRANS ("(unknown voice)"));
            r.drain();
        }

        beginTest ("lifetime: settle decrement and B cancellation survive session destruction");
        {
            Rig r;
            r.s->loadBlock (tpl);
            expectEquals (bus.suppressSend.load(), depth0 + 1);
            r.s.reset();
            r.drain();
            expectEquals (bus.suppressSend.load(), depth0);

            Rig r2; loadTpl (r2);
            r2.userEdit (k, edited);
            r2.s->setShowingA (true);
            r2.s->setShowingA (false);
            expectEquals ((int) r2.timers.size(), 1);
            const size_t n0 = r2.out.size();
            r2.s.reset();
            r2.advance (10000);
            expectEquals ((int) r2.out.size(), (int) n0);
        }

        bus.suppressSend.store (depth0);
    }
};

static VoiceSessionTests voiceSessionTests;
