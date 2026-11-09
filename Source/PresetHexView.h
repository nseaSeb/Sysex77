/*
  ==============================================================================

    PresetHexView.h
    Outil developpeur (mode Developer) : affiche le contenu brut d'un preset de la
    librairie (bloc bulk voix F0..F7) en hexa + ASCII, puis octet par octet avec
    NOTRE interpretation (ce que le loader voiceBlobToParams en tire).

    L'interpretation n'est PAS recopiee depuis le loader : elle est obtenue par sondage.
    Pour chaque octet, on inverse un bit, on re-decode, et on note quels parametres ont
    change. La vue reste donc toujours synchrone avec le vrai chargement ; un octet sans
    parametre = "non decode" (piste de retro-ingenierie).

    Lecture seule : n'envoie RIEN au synthe.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SysexUtils.h"
#include "SyParamTable.h"
#include "LookAndFeel.h"

namespace PresetHex
{
    inline juce::String hex2 (int v) { return juce::String::toHexString (v & 0xFF).paddedLeft ('0', 2).toUpperCase(); }

    /** Libelle lisible d'un parametre decode : nom du dictionnaire (si connu) + adresse brute. */
    inline juce::String paramLabel (const SyVoice::VoiceParam& p)
    {
        juce::String name;
        for (int i = 0; i < SyParam::kNumParams && name.isEmpty(); ++i)
        {
            const auto& e = SyParam::kParams[i];
            const auto a = SyVoice::syTranslate (e.ui, 0, 0);
            if (a.ok && a.group == p.group && a.n2 == p.param) name = e.name;
        }
        if (name.isEmpty()) name = (p.group == 0x07 ? "AWM" : "?");

        juce::String s = name;
        if (p.group != 0x02 && p.group != 0x08) s << " E" << ((p.addrHi >> 5) + 1);
        if (p.group == 0x09 && p.param < 0x32)  s << " F" << ((p.addrHi & 0x07) + 1);
        s << "  [g" << hex2 (p.group)
          << " N2 " << hex2 (p.param) << "]";
        return s;
    }

    /** Disposition des elements d'apres le type @32 (meme regle que voiceBlobToParams). */
    struct Layout { int nElem = 0; bool afm[4] { true, true, true, true }; int firstBase = 0; };

    inline Layout layoutFor (int type)
    {
        Layout l;
        switch (type)
        {
            case 0x00: case 0x03: l.nElem = 1; break;
            case 0x01: case 0x04: l.nElem = 2; break;
            case 0x02:            l.nElem = 4; break;
            case 0x05:            l.nElem = 1; l.afm[0] = false; break;
            case 0x06:            l.nElem = 2; l.afm[0] = l.afm[1] = false; break;
            case 0x07:            l.nElem = 4; for (auto& a : l.afm) a = false; break;
            case 0x08:            l.nElem = 2; l.afm[1] = false; break;
            case 0x09:            l.nElem = 4; l.afm[2] = l.afm[3] = false; break;
            default:              return l;   // drum (10) ou inconnu
        }
        l.firstBase = 107 + 9 * (l.nElem - 1);
        return l;
    }

    /** Zone structurelle de l'octet o (en-tete, nom, commun, element/operateur + offset relatif). */
    inline juce::String regionOf (int o, int size, const Layout& l)
    {
        if (o == 0)          return "F0 debut";
        if (o == 1)          return "fabricant (Yamaha)";
        if (o == 2)          return "device";
        if (o == 3)          return "format bulk";
        if (o == 4 || o == 5) return "nombre d'octets";
        if (o < 32)          return "en-tete";
        if (o == 32)         return "type de voix";
        if (o <= 42)         return "nom [" + juce::String (o - 33) + "]";
        if (o == size - 1)   return "F7 fin";
        if (o == size - 2)   return "checksum";
        if (l.nElem == 0 || o < l.firstBase) return "commun +" + juce::String (o - 32);

        int base = l.firstBase;
        for (int e = 0; e < l.nElem; ++e)
        {
            const int len = l.afm[e] ? 357 : 112;
            if (o < base + len)
            {
                const int rel = o - base;
                juce::String s = "E" + juce::String (e + 1) + (l.afm[e] ? " AFM +" : " AWM +") + juce::String (rel);
                if (l.afm[e] && rel < 270)
                    s << " (OP" << (6 - rel / 45) << " +" << (rel % 45) << ")";
                return s;
            }
            base += len;
        }
        return "hors structure";
    }

    /** Pour chaque octet : les parametres qu'il alimente (sondage par inversion de bit). */
    inline std::vector<juce::StringArray> probe (const juce::MemoryBlock& block)
    {
        const int n = (int) block.getSize();
        std::vector<juce::StringArray> notes ((size_t) juce::jmax (0, n));
        if (n < 43) return notes;

        const auto* d = (const juce::uint8*) block.getData();
        const auto ref = SyVoice::voiceBlobToParams (d, n);
        if (ref.isEmpty()) return notes;

        juce::MemoryBlock copy (block);
        auto* m = (juce::uint8*) copy.getData();
        for (int o = 33; o < n - 2; ++o)            // ni en-tete, ni type (change le layout), ni checksum/F7
        {
            m[o] ^= 0x01;
            const auto alt = SyVoice::voiceBlobToParams (m, n);
            m[o] ^= 0x01;
            if (alt.size() != ref.size()) continue;
            for (int i = 0; i < ref.size(); ++i)
            {
                const auto& a = ref.getReference (i);
                const auto& b = alt.getReference (i);
                if (a.value != b.value)  notes[(size_t) o].add (paramLabel (a) + (a.v1 != 0 || b.v1 != 0 ? " (V2)" : ""));
                else if (a.v1 != b.v1)   notes[(size_t) o].add (paramLabel (a) + " (V1)");
            }
        }
        return notes;
    }

    /** Rapport texte complet (hexdump + table d'interpretation). */
    inline juce::String report (const juce::MemoryBlock& block, const juce::String& title)
    {
        const int n = (int) block.getSize();
        const auto* d = (const juce::uint8*) block.getData();
        juce::String s;
        s << title << "\n";
        if (n < 43) { s << "Bloc trop court (" << n << " octets)."; return s; }

        const int type = d[32];
        const auto l = layoutFor (type);
        const auto notes = probe (block);
        const bool csOk = SyVoice::verifyYamahaBulkChecksum (d, n);

        int decoded = 0, body = 0;
        for (int o = 43; o < n - 2; ++o) { ++body; if (! notes[(size_t) o].isEmpty()) ++decoded; }

        s << "Type @32 : 0x" << hex2 (type) << "  " << SyVoice::voiceTypeLabel (type)
          << "    Taille : " << n << " octets    Checksum : " << (csOk ? "OK" : "INVALIDE") << "\n";
        if (l.nElem == 0)
            s << "Layout non decode (drum ou type inconnu) : hexa brut seulement.\n";
        else
        {
            s << "Structure : commun 33.." << (l.firstBase - 1);
            int base = l.firstBase;
            for (int e = 0; e < l.nElem; ++e)
            {
                const int len = l.afm[e] ? 357 : 112;
                s << "  |  E" << (e + 1) << (l.afm[e] ? " AFM " : " AWM ") << base << ".." << (base + len - 1);
                base += len;
            }
            s << "\nInterpretes : " << decoded << " / " << body << " octets (hors en-tete, nom, checksum)\n";
        }

        s << "\n== HEXA ==\n";
        for (int row = 0; row < n; row += 16)
        {
            s << juce::String (row).paddedLeft (' ', 5) << "  ";
            juce::String ascii;
            for (int k = 0; k < 16; ++k)
            {
                if (row + k < n)
                {
                    const int v = d[row + k];
                    s << hex2 (v) << (k == 7 ? "  " : " ");
                    ascii << (v >= 32 && v < 127 ? juce::String::charToString ((juce::juce_wchar) v) : ".");
                }
                else s << (k == 7 ? "    " : "   ");
            }
            s << " " << ascii << "\n";
        }

        s << "\n== INTERPRETATION ==\n";
        s << "  off  hex  dec  zone                              parametre(s)\n";
        for (int o = 0; o < n; ++o)
        {
            const int v = d[o];
            juce::String what;
            if (o >= 33 && o <= 42)      what = "'" + juce::String::charToString ((juce::juce_wchar) (v >= 32 && v < 127 ? v : '.')) + "'";
            else if (o == 32)            what = SyVoice::voiceTypeLabel (v);
            else if (o == n - 2)         what = csOk ? "OK" : "INVALIDE";
            else if (o < 32 || o == n - 1) what = "";
            else if (! notes[(size_t) o].isEmpty())
            {
                what = notes[(size_t) o].joinIntoString (" ; ");
                // Valeur lisible des enums ambigus (le codage bulk differe de l'ordre imprime).
                if (what.startsWith ("TYPE E") && what.contains ("[g09 N2 00]"))
                    what << "  = " << (v == 0 ? "THRU" : v == 1 ? "LPF" : v == 2 ? "HPF" : "?");
            }
            else                         what = "-- non decode";

            s << juce::String (o).paddedLeft (' ', 5) << "   " << hex2 (v) << "  "
              << juce::String (v).paddedLeft (' ', 3) << "  "
              << regionOf (o, n, l).paddedRight (' ', 32) << "  " << what << "\n";
        }
        return s;
    }
}

//==============================================================================
/** Contenu de la fenetre : rapport en police monospace + bouton Copier. */
class PresetHexView : public juce::Component
{
public:
    PresetHexView (const juce::MemoryBlock& block, const juce::String& title)
    {
        addAndMakeVisible (text);
        text.setMultiLine (true);
        text.setReadOnly (true);
        text.setScrollbarsShown (true);
        text.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain)));
        text.setText (PresetHex::report (block, title), false);
        text.moveCaretToTop (false);

        addAndMakeVisible (btCopy);
        btCopy.onClick = [this] { juce::SystemClipboard::copyTextToClipboard (text.getText()); };
        setSize (900, 640);
    }

    void paint (juce::Graphics& g) override { g.fillAll (SYColBackground); }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8);
        btCopy.setBounds (r.removeFromBottom (26).removeFromRight (120));
        r.removeFromBottom (6);
        text.setBounds (r);
    }

    /** Ouvre la vue dans une fenetre non modale redimensionnable. */
    static void show (const juce::MemoryBlock& block, const juce::String& title)
    {
        juce::DialogWindow::LaunchOptions o;
        o.content.setOwned (new PresetHexView (block, title));
        o.dialogTitle = "Preset hex - " + title;
        o.dialogBackgroundColour = SYColBackground;
        o.escapeKeyTriggersCloseButton = true;
        o.useNativeTitleBar = true;
        o.resizable = true;
        o.launchAsync();
    }

private:
    juce::TextEditor text;
    juce::TextButton btCopy { "Copier" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetHexView)
};
