/*
  ==============================================================================

    WaveCards.h
    Noms des ondes AWM de CARTE (WSOURCE != 0) : tables editables par l'utilisateur.
    Une carte = nom + liste ordonnee de noms d'ondes ; l'onde n de la carte (numero brut
    du dump, AWMWAVE) = waves[n]. Stockage : <userAppData>/SYSEX77/WaveCards/<nom>.json.
    Pre-remplies : cartes Yamaha W77xx (listes publiees sur bobbyblues.recup.ch, patchlists
    S77xx). Hypothese a confirmer sur hardware : numero brut 0 = 1re onde de la carte.
    Lecture/ecriture de fichiers locaux uniquement (rien n'est envoye au synthe).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "AppSettings.h"

namespace WaveCards
{
    struct Card { juce::String name; juce::StringArray waves; };

    inline juce::File folder()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("SYSEX77").getChildFile ("WaveCards");
    }

    inline std::vector<Card> builtins()
    {
        auto c = [] (const char* n, const char* list) { return Card { n, juce::StringArray::fromTokens (list, "|", "") }; };
        return {
            // W7701 : le site numerote 00, 01, 03, 04 (pas de 02, probable coquille) -> 4 ondes dans l'ordre.
            c ("W7701 Sax 1", "TSaxSoft|TSaxHard|ASaxSoft|ASaxHard"),
            c ("W7702 Drums 1", "Kick|SD 1 Dry|SD 1 Rim|SD 2|SD 3|Tom 1|Tom 2|HH Light|HH Mid|HH Heavy|HH Open|HH Pedal"),
            c ("W7704 Brass Section", "Trumpets|Tenor|Baritone|Blast|Roll|Sax Pad"),
            c ("W7705 String Section", "Strings|Pizz"),
            c ("W7731 Syn Wave 1", "Syn Brs1|AnlgSaw3|Pan Pipe|NewroStr|GlassPhon|TwinBell|Slap E.P|W Steel|MetalSaw|Keen|Bell Pad|Or.Click|ProcesBD|ProcesSD|ProcessHH|TuttiHit"),
            c ("W7732 Syn Wave 2", "AnaBass1|AnaBass2|Ana Str1|Ana Str2|Ana Str3|CS80 Brs|AnaBrs 2|AnaLead|NoiseMix"),
            c ("W7751 Rock & Pop", "Power BD|Tight BD|MediumSD|Light SD|Power SD|PowerTom|Syn Tom|HHclosed|HH open|Claps|Clave|Belltree|PickBass|SlapBass|SynBass|Gtr Pluk|Clavi Wv|EP Wave|SynOrgan|MiniSolo|MiniSaw|MatrixWv|Quack Wv|Brth Flt|Trumbone|BrassWv1|BrassWv2|Typist"),
            c ("W7752 House & Latin", "TronixBD|RebelBD|BoomBD|MaddySN|SplashSN|RimhitSN|TrashSN|CheapSN|AnaCow|AnaClap|AnaTom|AnaClave|AnaConga|AnaHHCL|AnaHHOP|NoizeHit|RockMel|RollMel|Haaaay!|Yeeeah!|Synbass1|Synbass2|GtrPluck|CongaSlp|CongaLow|CongaHi|CongaSl|CongaHI|CongaCmp|Timbale|Shaker|Cabasa|Whistle|Tambo|Agogo|Bongo|Tabla")
        };
    }

    inline juce::File fileFor (const juce::String& name)
    {
        return folder().getChildFile (juce::File::createLegalFileName (name) + ".json");
    }

    inline void save (const Card& c)
    {
        folder().createDirectory();
        auto* o = new juce::DynamicObject();
        o->setProperty ("name", c.name);
        juce::Array<juce::var> w; for (auto& s : c.waves) w.add (s);
        o->setProperty ("waves", w);
        fileFor (c.name).replaceWithText (juce::JSON::toString (juce::var (o)));
    }

    /** Toutes les cartes (fichiers utilisateur ; les cartes W77xx sont ecrites au 1er appel). */
    inline std::vector<Card> all()
    {
        if (! folder().isDirectory())
            for (auto& c : builtins()) save (c);
        std::vector<Card> out;
        for (auto& f : folder().findChildFiles (juce::File::findFiles, false, "*.json"))
        {
            const auto v = juce::JSON::parse (f);
            Card c { v["name"].toString(), {} };
            if (auto* a = v["waves"].getArray()) for (auto& s : *a) c.waves.add (s.toString());
            if (c.name.isNotEmpty()) out.push_back (c);
        }
        std::sort (out.begin(), out.end(), [] (const Card& a, const Card& b) { return a.name < b.name; });
        return out;
    }

    inline juce::String activeName()            { return getAppSettings()->getValue ("WaveCard"); }
    inline void setActive (const juce::String& n) { getAppSettings()->setValue ("WaveCard", n); }

    /** Nom de l'onde n de la carte active, sinon « Card #n ». */
    inline juce::String waveName (int n)
    {
        const auto act = activeName();
        if (act.isNotEmpty())
            for (auto& c : all())
                if (c.name == act && n >= 0 && n < c.waves.size() && c.waves[n].isNotEmpty())
                    return c.waves[n];
        return "Card #" + juce::String (n + 1);
    }

    //==========================================================================
    /** Editeur : choix de la carte active, creation, renommage implicite, noms d'ondes (1 par ligne). */
    class Editor : public juce::Component
    {
    public:
        Editor()
        {
            for (juce::Component* c : std::initializer_list<juce::Component*> { &combo, &waves, &btNew, &btDelete, &btSave, &btActive, &hint })
                addAndMakeVisible (c);
            waves.setMultiLine (true); waves.setReturnKeyStartsNewLine (true);
            hint.setText ("Une onde par ligne, dans l'ordre de la carte (ligne 1 = onde 0 du dump).",
                          juce::dontSendNotification);
            combo.onChange = [this] { showCard (combo.getText()); };
            btNew.onClick = [this]
            {
                auto* w = new juce::AlertWindow ("Nouvelle carte", "Nom de la carte :", juce::AlertWindow::QuestionIcon);
                w->addTextEditor ("n", "Ma carte");
                w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
                w->addButton ("Annuler", 0, juce::KeyPress (juce::KeyPress::escapeKey));
                w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
                {
                    const auto n = w->getTextEditorContents ("n").trim();
                    if (r == 1 && n.isNotEmpty()) { save ({ n, {} }); reload (n); }
                }), true);
            };
            btSave.onClick = [this]
            {
                if (combo.getText().isEmpty()) return;
                save ({ combo.getText(), juce::StringArray::fromLines (waves.getText().trimEnd()) });
            };
            btDelete.onClick = [this] { fileFor (combo.getText()).deleteFile(); reload ({}); };
            btActive.onClick = [this] { setActive (combo.getText()); updateActiveLabel(); };
            reload (activeName());
            setSize (460, 420);
        }
        void resized() override
        {
            auto r = getLocalBounds().reduced (8);
            auto top = r.removeFromTop (26);
            btNew.setBounds (top.removeFromRight (80)); top.removeFromRight (4);
            btDelete.setBounds (top.removeFromRight (80)); top.removeFromRight (4);
            combo.setBounds (top);
            r.removeFromTop (4);
            hint.setBounds (r.removeFromTop (20));
            auto bottom = r.removeFromBottom (26); r.removeFromBottom (6);
            btSave.setBounds (bottom.removeFromRight (110)); bottom.removeFromRight (6);
            btActive.setBounds (bottom.removeFromLeft (200));
            waves.setBounds (r);
        }
    private:
        void reload (const juce::String& select)
        {
            combo.clear (juce::dontSendNotification);
            int id = 1; for (auto& c : all()) combo.addItem (c.name, id++);
            int idx = 0;
            for (int i = 0; i < combo.getNumItems(); ++i) if (combo.getItemText (i) == select) idx = i;
            combo.setSelectedItemIndex (idx, juce::sendNotificationSync);
            updateActiveLabel();
        }
        void showCard (const juce::String& n)
        {
            for (auto& c : all()) if (c.name == n) { waves.setText (c.waves.joinIntoString ("\n")); break; }
            updateActiveLabel();
        }
        void updateActiveLabel()
        {
            const bool act = combo.getText().isNotEmpty() && combo.getText() == activeName();
            btActive.setButtonText (act ? "Carte active" : "Utiliser comme carte active");
            btActive.setEnabled (! act);
        }
        juce::ComboBox combo;
        juce::TextEditor waves;
        juce::Label hint;
        juce::TextButton btNew { "Nouvelle" }, btDelete { "Supprimer" }, btSave { "Enregistrer" }, btActive;
    };

    inline void showEditor()
    {
        juce::DialogWindow::LaunchOptions o;
        o.content.setOwned (new Editor());
        o.dialogTitle = "Cartes d'ondes AWM";
        o.escapeKeyTriggersCloseButton = true;
        o.useNativeTitleBar = true;
        o.resizable = true;
        o.launchAsync();
    }
}
