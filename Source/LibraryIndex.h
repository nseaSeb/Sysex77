/*
 ==============================================================================

 LibraryIndex.h

 Index de la librairie (métadonnées + recherche), persistant dans
 ~/Library/Application Support/Sysex77/library.json (via juce::JSON).

 100 % LOCAL : ne lit/écrit que des fichiers ; n'émet JAMAIS vers le synthé.

 - Clé preset stable = "<chemin relatif de la banque>#<slot positionnel 0..N-1>".
   Le nom n'est PAS une clé (éditable, non unique).
 - Par banque : famille de synthé détectée (SyVoice::detectSynthKind) + size/mtime
   (pour la réconciliation) + nombre de slots.
 - Par preset : nom (cache pour la recherche globale) + tags + notes + favori.

 Sérialisation en TABLEAUX d'objets (et non en objets-dictionnaires) pour éviter
 les Identifier JUCE invalides sur des clés contenant espaces/slashs/#.

 ==============================================================================
 */

#pragma once
#include <JuceHeader.h>
#include "SysexUtils.h"

class LibraryIndex
{
public:
    struct PresetMeta
    {
        juce::String      name;     // cache du nom (pour recherche), 10 car. SY77
        juce::StringArray tags;
        juce::String      notes;
        bool              fav = false;

        bool hasUserData() const { return ! tags.isEmpty() || notes.isNotEmpty() || fav; }
    };

    struct Hit
    {
        juce::String relPath;
        int          slot = 0;
        juce::String name;
        juce::String synth;
        juce::StringArray tags;
        bool         fav = false;
    };

    static LibraryIndex& get() { static LibraryIndex instance; return instance; }

    // Notifié (sur le thread message) après load()/reconcile() — l'UI s'y rebranche pour
    // rafraîchir le filtre synthé et les indicateurs. Mis à null par le consommateur en fin de vie.
    std::function<void()> onChanged;

    void setRoot (const juce::File& dir)
    {
        root = dir;
        jsonFile = dir.getChildFile ("library.json");
    }

    //==============================================================================
    // Clés / chemins
    static juce::String keyFor (const juce::String& relPath, int slot)
    {
        return relPath + "#" + juce::String (slot);
    }

    juce::String relPathOf (const juce::File& f) const
    {
        return f.getRelativePathFrom (root).replaceCharacter ('\\', '/');
    }

    //==============================================================================
    // Lecture / écriture
    void load()
    {
        const juce::ScopedLock sl (lock);
        banks.clear();
        presets.clear();

        auto data = juce::JSON::parse (jsonFile);
        if (auto* banksArr = data["banks"].getArray())
            for (auto& bv : *banksArr)
            {
                BankEntry b;
                b.synth = bv["synth"].toString();
                b.size  = (juce::int64) bv["size"];
                b.mtime = (juce::int64) bv["mtime"];
                b.slots = (int) bv["slots"];
                banks[bv["path"].toString()] = b;
            }
        if (auto* presetArr = data["presets"].getArray())
            for (auto& pv : *presetArr)
            {
                PresetMeta m;
                m.name  = pv["name"].toString();
                m.notes = pv["notes"].toString();
                m.fav   = (bool) pv["fav"];
                if (auto* t = pv["tags"].getArray())
                    for (auto& tag : *t) m.tags.add (tag.toString());
                presets[pv["key"].toString()] = m;
            }

        notifyChanged();
    }

    void save()
    {
        const juce::ScopedLock sl (lock);

        juce::var banksArr;
        for (auto& kv : banks)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("path",  kv.first);
            o->setProperty ("synth", kv.second.synth);
            o->setProperty ("size",  kv.second.size);
            o->setProperty ("mtime", kv.second.mtime);
            o->setProperty ("slots", kv.second.slots);
            banksArr.append (juce::var (o));
        }

        juce::var presetArr;
        for (auto& kv : presets)
        {
            // On ne sérialise que ce qui sert : nom (cache recherche) toujours,
            // tags/notes/fav s'ils existent.
            auto* o = new juce::DynamicObject();
            o->setProperty ("key",   kv.first);
            o->setProperty ("name",  kv.second.name);
            if (! kv.second.tags.isEmpty())
            {
                juce::var t;
                for (auto& tag : kv.second.tags) t.append (tag);
                o->setProperty ("tags", t);
            }
            if (kv.second.notes.isNotEmpty()) o->setProperty ("notes", kv.second.notes);
            if (kv.second.fav)                o->setProperty ("fav",   true);
            presetArr.append (juce::var (o));
        }

        auto* rootObj = new juce::DynamicObject();
        rootObj->setProperty ("version", 1);
        rootObj->setProperty ("banks",   banksArr);
        rootObj->setProperty ("presets", presetArr);

        // Écriture atomique : tmp puis remplacement.
        auto tmp = jsonFile.getSiblingFile ("library.json.tmp");
        if (tmp.replaceWithText (juce::JSON::toString (juce::var (rootObj))))
            tmp.moveFileTo (jsonFile);
    }

    //==============================================================================
    // Accès métadonnées
    juce::String synthOfBank (const juce::String& relPath) const
    {
        const juce::ScopedLock sl (lock);
        auto it = banks.find (relPath);
        return it != banks.end() ? it->second.synth : juce::String();
    }

    PresetMeta getMeta (const juce::String& key) const
    {
        const juce::ScopedLock sl (lock);
        auto it = presets.find (key);
        return it != presets.end() ? it->second : PresetMeta();
    }

    void setTags  (const juce::String& key, const juce::StringArray& tags) { { const juce::ScopedLock sl (lock); presets[key].tags = tags; } save(); }
    void setNotes (const juce::String& key, const juce::String& notes)     { { const juce::ScopedLock sl (lock); presets[key].notes = notes; } save(); }
    void setFav   (const juce::String& key, bool fav)                      { { const juce::ScopedLock sl (lock); presets[key].fav = fav; } save(); }

    juce::StringArray allTags() const
    {
        const juce::ScopedLock sl (lock);
        juce::StringArray out;
        for (auto& kv : presets)
            for (auto& t : kv.second.tags)
                out.addIfNotAlreadyThere (t);
        out.sort (true);
        return out;
    }

    // Liste des familles de synthé présentes (pour décider d'afficher le filtre).
    juce::StringArray synthsPresent() const
    {
        const juce::ScopedLock sl (lock);
        juce::StringArray out;
        for (auto& kv : banks)
            if (kv.second.synth.isNotEmpty())
                out.addIfNotAlreadyThere (kv.second.synth);
        out.sort (true);
        return out;
    }

    //==============================================================================
    // Recherche globale (nom ou tag). Filtres optionnels (tag exact, favoris, synthé).
    juce::Array<Hit> searchPresets (const juce::String& query, const juce::String& tag,
                                    bool favOnly, const juce::String& synth) const
    {
        const juce::ScopedLock sl (lock);
        juce::Array<Hit> hits;
        const auto q = query.trim();

        for (auto& kv : presets)
        {
            const auto& m = kv.second;
            if (favOnly && ! m.fav)                                   continue;
            if (tag.isNotEmpty()   && ! m.tags.contains (tag))        continue;

            const int hash = kv.first.lastIndexOfChar ('#');
            if (hash < 0) continue;
            const juce::String rel = kv.first.substring (0, hash);
            const int slot = kv.first.substring (hash + 1).getIntValue();

            const juce::String bankSynth = synthOfBank (rel);
            if (synth.isNotEmpty() && bankSynth != synth)            continue;

            if (q.isNotEmpty()
                && ! m.name.containsIgnoreCase (q)
                && ! m.tags.joinIntoString (" ").containsIgnoreCase (q)
                && ! rel.containsIgnoreCase (q))                     continue;

            hits.add ({ rel, slot, m.name, bankSynth, m.tags, m.fav });
        }
        return hits;
    }

    //==============================================================================
    // Mutations ciblées
    void removeBank (const juce::String& relPath)
    {
        {
            const juce::ScopedLock sl (lock);
            banks.erase (relPath);
            const juce::String prefix = relPath + "#";
            for (auto it = presets.begin(); it != presets.end(); )
                it = it->first.startsWith (prefix) ? presets.erase (it) : std::next (it);
        }
        save();
    }

    // Réindexe UNE banque (après copie d'un preset / modification). Ciblé, pas de full scan.
    void refreshBank (const juce::File& f)
    {
        if (! f.existsAsFile())
            return;
        indexBank (f, relPathOf (f), f.getSize(), f.getLastModificationTime().toMilliseconds());
        save();
        notifyChanged();
    }

    void renameBank (const juce::String& oldRel, const juce::String& newRel)
    {
        {
            const juce::ScopedLock sl (lock);
            auto bit = banks.find (oldRel);
            if (bit != banks.end()) { banks[newRel] = bit->second; banks.erase (bit); }

            const juce::String oldPrefix = oldRel + "#";
            std::map<juce::String, PresetMeta> moved;
            for (auto it = presets.begin(); it != presets.end(); )
                if (it->first.startsWith (oldPrefix))
                {
                    moved[newRel + it->first.substring (oldRel.length())] = it->second;
                    it = presets.erase (it);
                }
                else ++it;
            for (auto& kv : moved) presets[kv.first] = kv.second;
        }
        save();
    }

    //==============================================================================
    // Réconciliation disque <-> index (à lancer en tâche de fond ; ne touche pas l'UI).
    // Ajoute les banques nouvelles, purge les orphelines, rafraîchit celles modifiées.
    // Renvoie true si l'index a changé (donc sauvegardé).
    bool reconcile()
    {
        if (! root.isDirectory())
            return false;

        juce::Array<juce::File> files;
        root.findChildFiles (files, juce::File::findFiles, true, "*.syx");

        juce::StringArray seen;
        bool changed = false;

        for (auto& f : files)
        {
            const auto rel  = relPathOf (f);
            seen.add (rel);
            const juce::int64 sz = f.getSize();
            const juce::int64 mt = f.getLastModificationTime().toMilliseconds();

            bool needIndex = true;
            {
                const juce::ScopedLock sl (lock);
                auto it = banks.find (rel);
                if (it != banks.end() && it->second.size == sz && it->second.mtime == mt)
                    needIndex = false;   // inchangée
            }
            if (needIndex)
            {
                indexBank (f, rel, sz, mt);
                changed = true;
            }
        }

        // Purge des banques disparues du disque (et leurs presets).
        {
            const juce::ScopedLock sl (lock);
            juce::StringArray orphans;
            for (auto& kv : banks)
                if (! seen.contains (kv.first))
                    orphans.add (kv.first);
            for (auto& rel : orphans)
            {
                banks.erase (rel);
                const juce::String prefix = rel + "#";
                for (auto it = presets.begin(); it != presets.end(); )
                    it = it->first.startsWith (prefix) ? presets.erase (it) : std::next (it);
                changed = true;
            }
        }

        if (changed)
        {
            save();
            notifyChanged();
        }
        return changed;
    }

private:
    LibraryIndex() = default;

    // Déclenche onChanged sur le thread message (load/reconcile peuvent venir d'un thread de fond).
    void notifyChanged()
    {
        if (onChanged)
            juce::MessageManager::callAsync ([cb = onChanged] { if (cb) cb(); });
    }

    struct BankEntry { juce::String synth; juce::int64 size = 0, mtime = 0; int slots = 0; };

    // (Ré)indexe une banque : détecte le synthé + rafraîchit le cache des noms,
    // EN CONSERVANT les tags/notes/favoris existants (clé = relPath#slot).
    void indexBank (const juce::File& f, const juce::String& rel,
                    juce::int64 sz, juce::int64 mt)
    {
        juce::MemoryBlock mb;
        if (! f.loadFileAsData (mb))
            return;

        const auto* data = (const juce::uint8*) mb.getData();
        const auto synth = SyVoice::synthKindLabel (SyVoice::detectSynthKind (data, mb.getSize()));
        const auto names = SyVoice::extractVoiceNames (data, mb.getSize());

        const juce::ScopedLock sl (lock);
        banks[rel] = { synth, sz, mt, names.size() };

        // Met à jour/insère le nom de chaque slot, conserve les métadonnées utilisateur.
        for (int slot = 0; slot < names.size(); ++slot)
            presets[keyFor (rel, slot)].name = names[slot].trim();

        // Élague les slots disparus (banque raccourcie).
        const juce::String prefix = rel + "#";
        for (auto it = presets.begin(); it != presets.end(); )
        {
            if (it->first.startsWith (prefix)
                && it->first.substring (prefix.length()).getIntValue() >= names.size())
                it = presets.erase (it);
            else
                ++it;
        }
    }

    juce::File root, jsonFile;
    std::map<juce::String, BankEntry>  banks;
    std::map<juce::String, PresetMeta> presets;
    juce::CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LibraryIndex)
};
