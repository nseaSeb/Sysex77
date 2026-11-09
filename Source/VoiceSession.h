/*
  ==============================================================================

    VoiceSession.h
    Session d'edition de la voix ouverte : historique (undo/redo), comparaison A/B,
    revert, et flux sysex du chargement d'une voix dans l'edit buffer.

    Decoupage MidiDemo, ETAPE 1 : copie FIDELE de la logique de MidiDemo.h
    (editHistory, originalVoiceBlock, applyingHistory, showingA, loadSettling,
    bulkBusyUntilMs, historyGen, settleAfterLoad, sendOriginalBulk, noteBulkSent,
    toKV, onVoiceLoaded, replayToUi, sendParams, applyKV, undo/redo, setShowingA,
    revertToOriginal, unknownStatus, publishHistoryState, coeur d'openVoiceBlock,
    bloc d'enregistrement de sendToOutputs, noteRx de handleAsyncUpdate).
    PAS ENCORE BRANCHEE dans MidiDemo : seulement compilee et caracterisee par Tests.h.

    Toutes les dependances sont injectees (Ports) pour etre testable sans boucle de
    messages (en --test elle ne tourne pas) :
      - send          = MidiDemo::sendToOutputs (qui rappelle observeOutgoing AVANT son
                        gate « Out », puis coupe si Out OFF). Le gate n'est PAS ici.
      - replayParam   = application SYNCHRONE d'un param aux widgets (valueSysexIn +
                        sendChangeMessage(true)) ; toujours appele sous ScopedEchoSuppress.
      - post          = MessageManager::callAsync (FIFO). settleAfterLoad en DEPEND (deux
                        sauts FIFO pour passer apres les notifications des widgets).
      - schedule      = Timer::callAfterDelay (envoi differe de B ; ms >= 1).
      - historyState  = publication de l'etat (EditHist::Hooks).
    Thread message uniquement. observeOutgoing : l'appelant doit avoir verifie
    MessageManager::existsAndIsCurrentThread() (sendToOutputs est aussi appele par les
    threads sequenceur / envoi de banque).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <memory>
#include "SysexUtils.h"
#include "SysexBus.h"
#include "EditHistory.h"

class VoiceSession
{
public:
    struct HistoryState
    {
        bool canUndo = false, canRedo = false, loaded = false, showingA = false;
        juce::String status;
    };

    struct Ports
    {
        std::function<void (const juce::MidiMessage&)> send;
        std::function<void (int g, int hi, int lo, int p, int v1, int v2)> replayParam;
        std::function<void (bool)> lockEditing;
        std::function<void()> editHappened;
        std::function<double()> nowMs;
        std::function<void (std::function<void()>)> post;              // callAsync (FIFO)
        std::function<void (int ms, std::function<void()>)> schedule;  // callAfterDelay
        std::function<bool()> sendEnabled;                             // interrupteur Out
        std::function<int()> device;                                   // device number 1..16
        std::function<void (const HistoryState&)> historyState;
    };

    explicit VoiceSession (Ports p) : ports (std::move (p)) {}
    ~VoiceSession() { *alive = false; }

    //==========================================================================
    /** Coeur d'openVoiceBlock. Ordre du flux (fige) :
        stamp device + retarget $7F -> settle -> envoi bulk -> beforeReplay (props UI :
        VOICEMODE/VOICENAME/titre) -> rejeu des params decodes -> onVoiceLoaded.
        Renvoie false si sz <= 42 : le bulk est envoye (et settle pose) mais ni rejeu ni
        historique (l'historique et l'original PRECEDENTS restent en place).
        typeOut = octet 32 brut (le shell ne pose VOICEMODE que pour 0..9), nameOut = 33..42. */
    bool loadBlock (juce::MemoryBlock block, int& typeOut, juce::String& nameOut,
                    std::function<void (int type, const juce::String& name)> beforeReplay = nullptr)
    {
        typeOut = -1;
        nameOut = {};
        SyVoice::prepareEditBufferBulk ((juce::uint8*) block.getData(), (int) block.getSize(), ports.device());
        settleAfterLoad();   // widget echoes of this load are neither sent nor recorded
        sendRaw (block.getData(), (long) block.getSize());

        const juce::uint8* d = (const juce::uint8*) block.getData();
        const int sz = (int) block.getSize();
        if (sz <= 42) return false;

        typeOut = d[32];
        juce::String name;
        for (int i = 33; i < 43; ++i)
            name += juce::String::charToString ((juce::juce_wchar) d[i]);
        nameOut = name.trimEnd();
        if (beforeReplay != nullptr) beforeReplay (typeOut, nameOut);

        {
            // Rejeu VERBATIM (ordre et champs bruts de voiceBlobToParams, pas via toKV).
            const ScopedEchoSuppress noEcho;
            for (auto& vp : SyVoice::voiceBlobToParams (d, sz))
                ports.replayParam (vp.group, vp.addrHi, vp.addrLo, vp.param, vp.v1, vp.value);
        }
        onVoiceLoaded (block);
        return true;
    }

    /** Bloc d'enregistrement de sendToOutputs (AVANT le gate Out). raw = getRawData(). */
    void observeOutgoing (const juce::uint8* raw, int n, bool mouseDown)
    {
        if (SyVoice::isRecordableVoiceParamChange (raw, n)
            && ! applyingHistory && ! showingA && ! loadSettling && editHistory.isLoaded())
        {
            editHistory.recordEdit (EditHist::keyOf (raw[4], raw[5], raw[6], raw[7]),
                                    EditHist::valOf (raw[8], raw[9]), ports.nowMs(), mouseDown);
            ++historyGen;   // any pending delayed B send is now stale
            publishHistoryState ({});
        }
    }

    /** noteRx de handleAsyncUpdate. d = { g, hi, lo, p, v1, v2 } (octets 3..8 du param recu,
        deja filtre 43 1n 34 + device accepte). Pas de garde isLoaded/loadSettling (fidele). */
    void observeIncomingParam (const juce::uint8 d[6])
    {
        if (d[0] != 0x0F && ! showingA)
            editHistory.noteRx (EditHist::keyOf (d[0], d[1], d[2], d[3]), EditHist::valOf (d[4], d[5]));
    }

    void undo()
    {
        if (showingA) setShowingA (false);
        ++historyGen;
        int unknown = 0;
        applyKV (editHistory.undo (unknown));
        publishHistoryState (unknownStatus (unknown));
    }

    void redo()
    {
        if (showingA) setShowingA (false);
        ++historyGen;
        int unknown = 0;
        applyKV (editHistory.redo (unknown));
        publishHistoryState (unknownStatus (unknown));
    }

    void setShowingA (bool a)
    {
        if (a == showingA || ! editHistory.isLoaded() || originalVoiceBlock.getSize() < 2) return;
        if (a)
        {
            // A: the voice as loaded, sent whole to the edit buffer (covers every param).
            ++historyGen;
            sendOriginalBulk();
            replayToUi (editHistory.originalValues());
            ports.editHappened();   // a bulk is not a param-change: restart explicitly
        }
        else
        {
            // B: only what was edited. UI now; the sends wait until the SY77 has absorbed
            // the A bulk (fast A/B toggling would otherwise lose param-changes).
            auto diff = editHistory.diffFromOriginal();
            replayToUi (diff);
            const double wait = bulkBusyUntilMs - ports.nowMs();
            if (wait <= 0) sendParams (diff);
            else
            {
                auto token = alive;
                const int gen = historyGen;   // cancelled by any later edit/undo/load/revert/A
                ports.schedule ((int) wait + 1, [this, token, diff, gen]
                {
                    if (*token && ! showingA && historyGen == gen)
                        sendParams (diff);
                });
            }
        }
        showingA = a;
        ports.lockEditing (a);
        publishHistoryState (a ? TRANS ("Listening to the original - editing locked") : juce::String());
    }

    /** revertToOriginal (la confirmation askRevert reste dans l'UI). */
    void revert()
    {
        if (showingA) { showingA = false; ports.lockEditing (false); }
        ++historyGen;
        sendOriginalBulk();
        replayToUi (editHistory.originalValues());
        editHistory.makeRevertStep (ports.nowMs());
        ports.editHappened();
        publishHistoryState ({});
    }

    /** Condition des confirmations « edits non sauves » (askNewVoice, navigation). */
    bool hasUnsavedEdits() const   { return editHistory.isLoaded() && ! editHistory.diffFromOriginal().empty(); }
    bool isShowingA() const        { return showingA; }
    /** Condition d'askRevert. */
    bool canRevert() const         { return editHistory.isLoaded() && originalVoiceBlock.getSize() >= 2; }
    const EditHist::History& history() const { return editHistory; }

    static std::vector<EditHist::KV> toKV (const juce::Array<SyVoice::VoiceParam>& ps)
    {
        std::vector<EditHist::KV> out;
        for (auto& vp : ps)
            out.push_back ({ EditHist::keyOf (vp.group, vp.addrHi, vp.addrLo, vp.param),
                             EditHist::valOf (vp.v1, vp.value) });
        return out;
    }

    static juce::String unknownStatus (int n)
    {
        return n > 0 ? juce::String (n) + " " + TRANS ("value(s) not restorable (not read from the dump)")
                     : juce::String();
    }

private:
    Ports ports;
    std::shared_ptr<bool> alive = std::make_shared<bool> (true);   // remplace SafePointer

    EditHist::History editHistory;
    juce::MemoryBlock originalVoiceBlock;   // edit-buffer copy of the loaded voice (A side)
    bool applyingHistory = false;           // our own sends: not recorded
    bool showingA = false;
    bool loadSettling = false;              // a voice load's async widget echoes are still pending
    double bulkBusyUntilMs = 0;             // SY77 still absorbing the last voice bulk (A / Original)
    int historyGen = 0;                     // bumped by every edit/undo/redo/load/revert/A

    /** Cf. MidiDemo::settleAfterLoad. Le decrement de suppressSend a lieu MEME si la session
        est detruite entre-temps (sinon l'envoi resterait coupe a vie). post est copie dans
        la lambda : il ne faut pas toucher `ports` d'un objet mort. */
    void settleAfterLoad()
    {
        SysexBus::get().suppressSend.fetch_add (1, std::memory_order_acq_rel);
        loadSettling = true;
        auto token = alive;
        auto post  = ports.post;
        post ([this, token, post]
        {
            post ([this, token]
            {
                SysexBus::get().suppressSend.fetch_sub (1, std::memory_order_acq_rel);
                if (*token) loadSettling = false;
            });
        });
    }

    /** Copie de MidiSysex.h::sendRaw. */
    void sendRaw (const void* data, long size)
    {
        if (size > 1)
        {
            juce::Logger::writeToLog (juce::String (size));
            juce::MidiMessage m (data, (int) size);
            m.setTimeStamp (ports.nowMs() * 0.001);
            ports.send (m);
        }
        else
            juce::Logger::writeToLog ("Error Send RAW");
    }

    /** Sends the original voice to the edit buffer, stamped with the CURRENT device number. */
    void sendOriginalBulk()
    {
        SyVoice::setBulkDevice ((juce::uint8*) originalVoiceBlock.getData(), (int) originalVoiceBlock.getSize(),
                                ports.device());
        sendRaw (originalVoiceBlock.getData(), (long) originalVoiceBlock.getSize());
        noteBulkSent (originalVoiceBlock);
    }

    /** Time the SY77 needs for a voice bulk: wire time (~size/3 ms) + processing margin. */
    void noteBulkSent (const juce::MemoryBlock& b)
    {
        if (! ports.sendEnabled()) return;   // nothing went out
        bulkBusyUntilMs = ports.nowMs() + (double) b.getSize() / 3.0 + 60.0;
    }

    void onVoiceLoaded (const juce::MemoryBlock& editBufferBlock)
    {
        if (showingA) { showingA = false; ports.lockEditing (false); }
        originalVoiceBlock = editBufferBlock;
        ++historyGen;
        noteBulkSent (editBufferBlock);
        editHistory.load (toKV (SyVoice::voiceBlobToParams ((const juce::uint8*) editBufferBlock.getData(),
                                                            (int) editBufferBlock.getSize())));
        publishHistoryState ({});
    }

    void replayToUi (const std::vector<EditHist::KV>& kvs)
    {
        const ScopedEchoSuppress noEcho;   // UI follows, nothing is re-emitted by the widgets
        for (auto& kv : kvs)
            ports.replayParam (EditHist::gOf (kv.first), EditHist::hiOf (kv.first),
                               EditHist::loOf (kv.first), EditHist::pOf (kv.first),
                               EditHist::v1Of (kv.second), EditHist::v2Of (kv.second));
    }

    void sendParams (const std::vector<EditHist::KV>& kvs)
    {
        const juce::ScopedValueSetter<bool> svs (applyingHistory, true);
        for (auto& kv : kvs)
        {
            const auto d = SyVoice::paramChangeBytes (ports.device(),
                                 EditHist::gOf (kv.first), EditHist::hiOf (kv.first),
                                 EditHist::loOf (kv.first), EditHist::pOf (kv.first),
                                 EditHist::v1Of (kv.second), EditHist::v2Of (kv.second));
            auto m = juce::MidiMessage::createSysExMessage (d.data(), 9);
            m.setTimeStamp (ports.nowMs() * 0.001);
            ports.send (m);
        }
    }

    void applyKV (const std::vector<EditHist::KV>& kvs) { replayToUi (kvs); sendParams (kvs); }

    void publishHistoryState (const juce::String& status)
    {
        if (ports.historyState == nullptr) return;
        HistoryState h;
        h.canUndo  = editHistory.canUndo();
        h.canRedo  = editHistory.canRedo();
        h.loaded   = editHistory.isLoaded() && originalVoiceBlock.getSize() > 1;
        h.showingA = showingA;
        h.status   = status;
        ports.historyState (h);
    }

    JUCE_DECLARE_NON_COPYABLE (VoiceSession)
};
