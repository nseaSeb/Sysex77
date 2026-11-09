/*
  ==============================================================================

    VoiceSession.h
    Session d'edition de la voix ouverte : historique (undo/redo), comparaison A/B,
    revert, et flux sysex du chargement d'une voix dans l'edit buffer.

    VoiceSession est le SEUL proprietaire de cette logique (historique, A/B, revert, settle
    apres chargement, envoi B differe, enregistrement des edits sortants, noteRx) : MidiDemo
    n'en garde qu'une coque UI (membre `session`, ports dans makeSessionPorts, props
    VOICEMODE/VOICENAME/titre dans openVoiceBlock). Caracterisee par Tests.h.

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

/** Envoi d'un message MIDI brut (sysex en memoire), commun a MidiDemo::sendRaw et
    VoiceSession : log de la taille, horodatage nowMs (ms) -> secondes, puis send.
    Nom distinct de sendRaw : les membres sendRaw (MidiDemo, VoiceSession) le masqueraient. */
inline void sendRawSysex (const void* data, long size, double nowMs,
                          const std::function<void (const juce::MidiMessage&)>& send)
{
    if (size > 1)
    {
        juce::Logger::writeToLog (juce::String (size));
        juce::MidiMessage m (data, (int) size);
        m.setTimeStamp (nowMs * 0.001);
        send (m);
    }
    else
        juce::Logger::writeToLog ("Error Send RAW");
}

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
    enum class LoadResult
    {
        Loaded,     // bulk envoye, rejoue, historique charge
        Discarded,  // bulk envoye mais trop court (sz <= 42) : historique oublie -> le shell
                    // doit effacer le nom de voix / titre (la voix precedente n'est plus la)
        Refused     // PAS un bulk voix redirigeable vers $7F : RIEN envoye, rien modifie
    };

    /** Coeur d'openVoiceBlock. Ordre du flux (fige) :
        stamp device + retarget $7F -> settle -> envoi bulk -> beforeReplay (props UI :
        VOICEMODE/VOICENAME/titre) -> rejeu des params decodes -> onVoiceLoaded.
        Refused si prepareEditBufferBulk echoue (bloc < 34, non cadre, pas 43 .. 7A) : SECURITE,
        le bloc garderait son type memoire d'origine et pourrait ecraser une voix interne ->
        aucun envoi, ni settle, ni changement d'historique.
        Discarded si sz <= 42 : le bulk est envoye (et settle pose) mais ni rejeu ni
        historique ni beforeReplay ; l'historique et l'original precedents sont oublies
        (discardLoadedVoice).
        beforeReplay recoit type = octet 32 brut (le shell ne pose VOICEMODE que pour 0..9) et
        name = octets 33..42 sans les espaces de fin (peut etre vide pour une vraie voix). */
    LoadResult loadBlock (juce::MemoryBlock block,
                          std::function<void (int type, const juce::String& name)> beforeReplay = nullptr)
    {
        if (! SyVoice::prepareEditBufferBulk ((juce::uint8*) block.getData(), (int) block.getSize(), ports.device()))
        {
            juce::Logger::writeToLog ("openVoiceBlock refuse : pas un bulk voix redirigeable vers l'edit buffer");
            return LoadResult::Refused;
        }
        settleAfterLoad();   // widget echoes of this load are neither sent nor recorded
        sendRaw (block.getData(), (long) block.getSize());

        const juce::uint8* d = (const juce::uint8*) block.getData();
        const int sz = (int) block.getSize();
        if (sz <= 42) { discardLoadedVoice(); return LoadResult::Discarded; }

        juce::String name;
        for (int i = 33; i < 43; ++i)
            name += juce::String::charToString ((juce::juce_wchar) d[i]);
        if (beforeReplay != nullptr) beforeReplay (d[32], name.trimEnd());

        const auto params = SyVoice::voiceBlobToParams (d, sz);   // decode UNE fois par chargement
        {
            // Rejeu VERBATIM (ordre et champs bruts de voiceBlobToParams, pas via toKV).
            const ScopedEchoSuppress noEcho;
            for (auto& vp : params)
                ports.replayParam (vp.group, vp.addrHi, vp.addrLo, vp.param, vp.v1, vp.value);
        }
        onVoiceLoaded (block, params);
        return LoadResult::Loaded;
    }

    /** Bloc d'enregistrement de sendToOutputs (AVANT le gate Out). raw = getRawData(). */
    void observeOutgoing (const juce::uint8* raw, int n, bool mouseDown)
    {
        if (SyVoice::isRecordableVoiceParamChange (raw, n)
            && ! applyingHistory && ! showingA && loadSettling == 0 && editHistory.isLoaded())
        {
            // Un B differe encore en attente (SY77 en train d'absorber le bulk A) : on l'AVANCE
            // ici, avant que l'edit ne sorte (l'appelant emet raw APRES ce retour) -> le synthe
            // recoit le diff B puis l'edit, et UI/synthe restent alignes. Pas une ecriture
            // nouvelle : c'est l'envoi B deja demande. sendParams le pose sous applyingHistory.
            flushPendingB();
            editHistory.recordEdit (EditHist::keyOf (raw[4], raw[5], raw[6], raw[7]),
                                    EditHist::valOf (raw[8], raw[9]), ports.nowMs(), mouseDown);
            ++historyGen;   // any pending delayed B send is now stale
            publishHistoryState ({});
        }
    }

    /** noteRx de handleAsyncUpdate. d = { g, hi, lo, p, v1, v2 } (octets 3..8 du param recu,
        deja filtre 43 1n 34 + device accepte). Le cas « aucune voix chargee » est garde par
        History::noteRx lui-meme. */
    void observeIncomingParam (const juce::uint8 d[6])
    {
        if (d[0] != 0x0F && ! showingA)
            editHistory.noteRx (EditHist::keyOf (d[0], d[1], d[2], d[3]), EditHist::valOf (d[4], d[5]));
    }

    void undo()
    {
        if (showingA) setShowingA (false);
        // Un B differe en attente (programme par ce setShowingA(false) ou un B recent) est
        // AVANCE ici, avant ++historyGen qui l'invaliderait : le synthe recoit le diff B puis
        // les valeurs du undo -> il ne reste pas sur le bulk A alors que l'UI montre B.
        flushPendingB();
        ++historyGen;
        int unknown = 0;
        applyKV (editHistory.undo (unknown));
        publishHistoryState (unknownStatus (unknown));
    }

    void redo()
    {
        if (showingA) setShowingA (false);
        // Un B differe en attente (programme par ce setShowingA(false) ou un B recent) est
        // AVANCE ici, avant ++historyGen qui l'invaliderait : le synthe recoit le diff B puis
        // les valeurs du redo -> il ne reste pas sur le bulk A alors que l'UI montre B.
        flushPendingB();
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
                const int gen = historyGen;   // cancelled by any later undo/load/revert/A;
                pendingB = diff;              // advanced (flushPendingB) by a later edit
                pendingBGen = gen;
                ports.schedule ((int) wait + 1, [this, token, gen]
                {
                    if (*token && historyGen == gen) flushPendingB();
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
        if (! canRevert()) return;   // rien de charge : pas de pas annulable depuis un original vide
        if (showingA) { showingA = false; ports.lockEditing (false); }
        ++historyGen;
        sendOriginalBulk();
        replayToUi (editHistory.originalValues());
        editHistory.makeRevertStep (ports.nowMs());
        ports.editHappened();
        publishHistoryState ({});
    }

    /** Condition des confirmations « edits non sauves » (askNewVoice, navigation). */
    bool hasUnsavedEdits() const   { return editHistory.isLoaded() && editHistory.hasDiff(); }
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
    int loadSettling = 0;                   // voice loads whose async widget echoes are still pending
    std::vector<EditHist::KV> pendingB;     // delayed B diff not yet sent...
    int pendingBGen = -1;                   // ...valid only while historyGen == pendingBGen
    double bulkBusyUntilMs = 0;             // SY77 still absorbing the last voice bulk (A / Original)
    int historyGen = 0;                     // bumped by every edit/undo/redo/load/revert/A

    /** Pose la fenetre de stabilisation d'un chargement : suppressSend + loadSettling tenus
        pendant deux sauts FIFO de post, le temps que les notifications asynchrones des widgets
        (echos du rejeu) passent sans etre envoyees ni enregistrees. Le decrement de suppressSend a lieu MEME si la session
        est detruite entre-temps (sinon l'envoi resterait coupe a vie). post est copie dans
        la lambda : il ne faut pas toucher `ports` d'un objet mort. */
    void settleAfterLoad()
    {
        SysexBus::get().suppressSend.fetch_add (1, std::memory_order_acq_rel);
        ++loadSettling;   // compteur : deux chargements en file -> le 1er settle ne libere pas le 2e
        auto token = alive;
        auto post  = ports.post;
        post ([this, token, post]
        {
            post ([this, token]
            {
                SysexBus::get().suppressSend.fetch_sub (1, std::memory_order_acq_rel);
                if (*token && loadSettling > 0) --loadSettling;
            });
        });
    }

    /** Envoie le B differe en attente s'il est encore valide (meme generation, pas en A),
        puis l'invalide. Appele par le timer ou, en avance, par le premier edit enregistre. */
    void flushPendingB()
    {
        if (pendingBGen < 0) return;
        const bool valid = pendingBGen == historyGen && ! showingA;
        auto diff = std::move (pendingB);
        pendingB.clear();
        pendingBGen = -1;
        if (valid) sendParams (diff);
    }

    void sendRaw (const void* data, long size) { sendRawSysex (data, size, ports.nowMs(), ports.send); }

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

    void onVoiceLoaded (const juce::MemoryBlock& editBufferBlock, const juce::Array<SyVoice::VoiceParam>& params)
    {
        if (showingA) { showingA = false; ports.lockEditing (false); }
        originalVoiceBlock = editBufferBlock;
        ++historyGen;
        noteBulkSent (editBufferBlock);
        editHistory.load (toKV (params));
        publishHistoryState ({});
    }

    /** Bloc trop court (sz <= 42) envoye dans l'edit buffer : l'historique et l'original de
        la voix precedente ne correspondent plus au synthe -> on les oublie (rien n'est envoye). */
    void discardLoadedVoice()
    {
        if (showingA) { showingA = false; ports.lockEditing (false); }
        originalVoiceBlock.reset();
        ++historyGen;          // any pending delayed B send is now stale
        editHistory.unload();
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
