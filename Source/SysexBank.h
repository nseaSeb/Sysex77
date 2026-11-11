// SysexBank.h - SyBank : banques .syx (slots, reorganisation, checksum).
// Partie de SysexUtils.h (en-tete parapluie). Logique pure, testee dans tests/.
#pragma once
#include <JuceHeader.h>
#include "SysexVoice.h"

//==============================================================================
/** ORGANISATION DE BANQUE (.syx) — operations pures sur les blocs voix, hors synthe.
    Faits verifies (corpus docs/, 235 banques / 14 914 voix) :
    - octet 30 = type de memoire (0 = interne), octet 31 = numero de slot 0..63 = position du
      bloc parmi les voix « LM  8101VC ». L'envoi de banque transmet les blocs BRUTS : c'est
      l'octet 31 qui decide quel slot du SY77 est ecrase -> renumerotation + checksum a chaque
      ecriture (somme Yamaha sur [6, size-2)).
    - Voix a 4 elements (types 0x02 4AFM mono, 0x07 4AWM, 0x09 2AFM+2AWM) : UNIQUEMENT en
      banque D (slots 48..63), 1 012 voix sans exception -> toute operation est refusee sinon.
    Les blocs non-voix (multi, pan, systeme) restent identiques et a leur place. */
namespace SyBank
{
    constexpr int kMaxVoices = 64, kFirstD = 48;

    inline bool isVoiceBlock (const juce::MemoryBlock& b)
    {
        // MEME classifieur que l'UI (extractVoiceNames/getVoiceBlock) : sinon les numeros de slot
        // de l'UI et de SyBank divergeraient sur un bloc court/non termine.
        auto* d = (const juce::uint8*) b.getData();
        return b.getSize() > 0 && d[0] == 0xF0 && SyVoice::isVoiceMessageAt (d, b.getSize(), 0);
    }
    inline int  typeOf (const juce::MemoryBlock& b) { return b.getSize() > 32 ? ((const juce::uint8*) b.getData())[32] : -1; }
    inline bool needsBankD (int type) { return type == 0x02 || type == 0x07 || type == 0x09; }
    /** « A1 » .. « D16 ». */
    inline juce::String slotName (int slot) { return juce::String::charToString ((juce::juce_wchar) ('A' + slot / 16)) + juce::String (slot % 16 + 1); }

    struct Bank
    {
        juce::Array<juce::MemoryBlock> blocks;   // tous les messages, ordre du fichier
        juce::Array<juce::MemoryBlock> voices;   // sous-suite des voix (edition)

        static Bank parse (const void* data, size_t size)
        {
            Bank b;
            b.blocks = SyVoice::splitSysexMessages ((const juce::uint8*) data, size);
            for (auto& m : b.blocks) if (isVoiceBlock (m)) b.voices.add (m);
            return b;
        }
        int numVoices() const { return voices.size(); }

        /** Premiere violation (texte) ou vide si la banque est placable telle quelle. */
        juce::String problem() const
        {
            if (voices.size() > kMaxVoices) return TRANS ("A bank holds at most 64 voices.");
            for (int i = 0; i < voices.size(); ++i)
                if (needsBankD (typeOf (voices[i])) && i < kFirstD)
                    return TRANS ("4-element voices can only be in bank D (D1-D16): ") + slotName (i);
            return {};
        }

        /** Fichier : voix remises a leur place (les nouvelles en fin), octet 30 = 0 (interne),
            octet 31 = slot, checksum recalcule. Non-voix inchanges. */
        juce::MemoryBlock serialize() const
        {
            juce::MemoryBlock out;
            int v = 0;
            auto put = [&out, &v] (juce::MemoryBlock m)
            {
                auto* d = (juce::uint8*) m.getData();
                const int n = (int) m.getSize();
                if (n >= 34 && d[n - 1] == 0xF7)   // bloc mal forme : garde tel quel (slot compte)
                {
                    d[30] = 0x00;
                    d[31] = (juce::uint8) v;
                    d[n - 2] = SyVoice::yamahaChecksum (d + 6, n - 8);
                }
                ++v;
                out.append (m.getData(), m.getSize());
            };
            for (auto& m : blocks)
            {
                if (! isVoiceBlock (m)) { out.append (m.getData(), m.getSize()); continue; }
                if (v < voices.size()) put (voices[v]);   // (une voix retiree n'est jamais : on remplace)
            }
            while (v < voices.size()) put (voices[v]);
            return out;
        }
    };

    /** Resultat d'operation : erreur (vide = ok) + nouvelle position de chaque ancienne voix
        (pour deplacer tags/notes/favoris ; -1 = metadonnees abandonnees). */
    struct Result { juce::String error; std::vector<int> newPosOfOld; };

    /** Les Multi (8101MU, 204 o.) de la MEME banque designent leurs voix internes par VNUM
        (slot) : on suit les voix deplacees (parties VMEM 0 = interne, seule valeur non ambigue),
        checksum recalcule. Une voix remplacee (-1) garde son slot -> VNUM inchange. */
    inline void remapMultis (Bank& b, const std::vector<int>& map)
    {
        for (auto& m : b.blocks)
        {
            auto* d = (juce::uint8*) m.getData();
            if (m.getSize() != 204 || d[0] != 0xF0 || std::memcmp (d + 6, "LM  8101MU", 10) != 0) continue;
            for (int ch = 0; ch < 16; ++ch)
            {
                const int base = 90 + 7 * ch;
                const int vn = d[base + 2];
                if (d[base + 1] == 0 && vn < (int) map.size() && map[(size_t) vn] >= 0)
                    d[base + 2] = (juce::uint8) map[(size_t) vn];
            }
            d[202] = SyVoice::yamahaChecksum (d + 6, 204 - 8);
        }
    }

    inline Result checked (Bank& b, const Bank& before, std::vector<int> map)
    {
        const auto e = b.problem();
        if (e.isNotEmpty()) { b = before; return { e, {} }; }
        remapMultis (b, map);
        return { {}, std::move (map) };
    }
    inline std::vector<int> identity (int n) { std::vector<int> m ((size_t) n); for (int i = 0; i < n; ++i) m[(size_t) i] = i; return m; }

    /** Deplace la voix from vers to (les voix entre les deux glissent d'un cran). */
    inline Result move (Bank& b, int from, int to)
    {
        const int n = b.numVoices();
        if (from < 0 || from >= n || to < 0 || to >= n || from == to) return { TRANS ("Invalid slot."), {} };
        const Bank before = b;
        b.voices.move (from, to);
        std::vector<int> map ((size_t) n);
        for (int i = 0; i < n; ++i)
            map[(size_t) i] = i == from ? to : (from < to ? (i > from && i <= to ? i - 1 : i)
                                                         : (i >= to && i < from ? i + 1 : i));
        return checked (b, before, std::move (map));
    }

    inline Result swap (Bank& b, int x, int y)
    {
        const int n = b.numVoices();
        if (x < 0 || x >= n || y < 0 || y >= n || x == y) return { TRANS ("Invalid slot."), {} };
        const Bank before = b;
        b.voices.swap (x, y);
        auto map = identity (n); map[(size_t) x] = y; map[(size_t) y] = x;
        return checked (b, before, std::move (map));
    }

    /** Copie de la voix i inseree juste apres (les suivantes glissent d'un cran). */
    inline Result duplicate (Bank& b, int i)
    {
        const int n = b.numVoices();
        if (i < 0 || i >= n) return { TRANS ("Invalid slot."), {} };
        if (n >= kMaxVoices) return { TRANS ("The bank is full (64 voices)."), {} };
        const Bank before = b;
        b.voices.insert (i + 1, b.voices[i]);
        auto map = identity (n);
        for (int k = i + 1; k < n; ++k) map[(size_t) k] = k + 1;
        return checked (b, before, std::move (map));
    }

    /** « Supprimer » = remplacer par une voix Init : slots fixes (une remontee ferait sortir
        des voix 4 elements de la banque D). Metadonnees de la voix effacees. */
    inline Result replace (Bank& b, int i, const juce::MemoryBlock& voice)
    {
        const int n = b.numVoices();
        if (i < 0 || i >= n || ! isVoiceBlock (voice)) return { TRANS ("Invalid slot."), {} };
        const Bank before = b;
        b.voices.set (i, voice);
        auto map = identity (n); map[(size_t) i] = -1;
        return checked (b, before, std::move (map));
    }

    /** Ajout en fin (copie depuis une autre banque). */
    inline Result append (Bank& b, const juce::MemoryBlock& voice)
    {
        if (! isVoiceBlock (voice)) return { TRANS ("Not a voice."), {} };
        if (b.numVoices() >= kMaxVoices) return { TRANS ("The bank is full (64 voices)."), {} };
        const Bank before = b;
        b.voices.add (voice);
        return checked (b, before, identity (before.numVoices()));
    }

    /** Preparation de l'envoi d'une banque (logique extraite de MidiSysex, adresseOscSendBank).
        empty = rien de charge ; notSY77 = banque d'un autre synthe (JAMAIS envoyee) ;
        raw = pas de cadrage F0..F7 reconnu -> envoi brut ; blocks = messages, device restampe. */
    struct SendPlan { enum class Kind { empty, notSY77, raw, blocks } kind = Kind::empty;
                      juce::Array<juce::MemoryBlock> blocks; };

    inline SendPlan prepareForSend (const juce::MemoryBlock& bank, int deviceNumber)
    {
        SendPlan p;
        if (bank.getSize() <= 1) return p;
        if (SyVoice::detectSynthKind ((const juce::uint8*) bank.getData(), bank.getSize()) != SyVoice::SynthKind::SY77)
        { p.kind = SendPlan::Kind::notSY77; return p; }
        p.blocks = SyVoice::splitSysexMessages ((const juce::uint8*) bank.getData(), bank.getSize());
        if (p.blocks.isEmpty()) { p.kind = SendPlan::Kind::raw; return p; }
        for (auto& b : p.blocks)   // device number of the app, not the one stored in the file
            SyVoice::setBulkDevice ((juce::uint8*) b.getData(), (int) b.getSize(), deviceNumber);
        p.kind = SendPlan::Kind::blocks;
        return p;
    }
}
