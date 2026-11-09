/*
  ==============================================================================

    Themes.h
    Système de thèmes « façon neovim » : palettes intégrées + dossier de fichiers
    thème (un sous-dossier par thème, theme.xml + assets optionnels), découverte,
    sérialisation (export des modèles) et application par nom.

  ==============================================================================
*/

#pragma once
#include <vector>
#include "LookAndFeel.h"

//==============================================================================
// Liste des palettes intégrées (ordre d'affichage dans le sélecteur).
inline std::vector<SyPalette> syBuiltinPalettes()
{
    return { syPaletteDarkOrange(), syPaletteFmDark(), syPaletteLight(),
             syPaletteCrimson(), syPaletteTangerine(), syPaletteIndigoNight() };
}

// Dossier utilisateur des thèmes : <appData>/Application Support/Sysex77/Themes
inline juce::File syThemesDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
              .getChildFile ("Application Support/Sysex77/Themes");
}

// Lit une couleur ARGB depuis un attribut ("0xff8a3d", "#ff8a3d", "ff8a3d" ou décimal).
// 6 chiffres hex -> alpha plein ; absent -> défaut.
inline juce::Colour syReadColour (const juce::XmlElement& e, juce::StringRef attr, juce::Colour def)
{
    if (! e.hasAttribute (attr)) return def;
    auto s = e.getStringAttribute (attr).trim().toLowerCase().replace ("0x", "").replace ("#", "");
    if (s.isEmpty()) return def;
    if (s.length() <= 6) s = "ff" + s;
    return juce::Colour ((juce::uint32) s.getHexValue64());
}

// Parse un <THEME> en SyPalette (défauts = FM Dark, surchargés par les attributs présents).
// base (optionnel) : palette de depart a la place de FM Dark -- pour un theme integre exporte
// sur disque, les champs ajoutes depuis l'export (ex. op1..op6) heritent de la version integree.
inline SyPalette syParseTheme (const juce::XmlElement& root, const juce::File& folder,
                               const SyPalette* base = nullptr)
{
    SyPalette p = base != nullptr ? *base : syPaletteFmDark();
    p.folder = folder;
    p.name   = root.getStringAttribute ("name", folder.getFileNameWithoutExtension());
    p.dark   = root.getBoolAttribute  ("dark", p.dark);

    if (auto* c = root.getChildByName ("COLORS"))
    {
        p.background      = syReadColour (*c, "background",      p.background);
        p.surface         = syReadColour (*c, "surface",         p.surface);
        p.surfaceAlt      = syReadColour (*c, "surfaceAlt",      p.surfaceAlt);
        p.panelBorder     = syReadColour (*c, "panelBorder",     p.panelBorder);
        p.textPrimary     = syReadColour (*c, "textPrimary",     p.textPrimary);
        p.textMuted       = syReadColour (*c, "textMuted",       p.textMuted);
        p.accent          = syReadColour (*c, "accent",          p.accent);
        p.accentSecondary = syReadColour (*c, "accentSecondary", p.accentSecondary);
        p.knobTrack       = syReadColour (*c, "knobTrack",       p.knobTrack);
        p.knobFill        = syReadColour (*c, "knobFill",        p.accent);            // défaut = accent
        p.knobBody        = syReadColour (*c, "knobBody",        p.knobBody);
        p.shadow          = syReadColour (*c, "shadow",          p.shadow);
        p.glow            = syReadColour (*c, "glow",            p.accent.withAlpha (0.45f));

        // Couleurs d'operateurs FM (op1..op6) : un seul attribut present suffit a les activer.
        for (int i = 0; i < 6; ++i)
        {
            const juce::String key = "op" + juce::String (i + 1);
            if (c->hasAttribute (key))
            {
                p.opColour[i]  = syReadColour (*c, key, p.opColour[i]);
                p.hasOpColours = true;
            }
        }
    }

    if (auto* a = root.getChildByName ("ASSETS"))
    {
        auto knob = a->getStringAttribute ("knob").trim();
        auto bg   = a->getStringAttribute ("background").trim();
        if (knob.isNotEmpty()) p.knobImage       = folder.getChildFile (knob);
        if (bg.isNotEmpty())   p.backgroundImage = folder.getChildFile (bg);
    }

    if (auto* s = root.getChildByName ("STYLE"))
    {
        auto bs = s->getStringAttribute ("buttons").trim().toLowerCase();
        auto ps = s->getStringAttribute ("panels").trim().toLowerCase();
        auto ss = s->getStringAttribute ("sliders").trim().toLowerCase();
        auto ks = s->getStringAttribute ("knobs").trim().toLowerCase();
        if (bs.isNotEmpty()) p.buttonStyle = bs;
        if (ps.isNotEmpty()) p.panelStyle  = ps;
        if (ss.isNotEmpty()) p.sliderStyle = ss;
        if (ks.isNotEmpty()) p.knobStyle   = ks;
    }
    return p;
}

// Sérialise une palette en XML <THEME> (pour exporter des modèles éditables).
inline juce::String syPaletteToXml (const SyPalette& p)
{
    auto hex = [] (juce::Colour c) { return "0x" + c.toDisplayString (true).toLowerCase(); };

    juce::XmlElement root ("THEME");
    root.setAttribute ("name", p.name);
    root.setAttribute ("dark", p.dark ? 1 : 0);

    auto* c = new juce::XmlElement ("COLORS");
    c->setAttribute ("background",      hex (p.background));
    c->setAttribute ("surface",         hex (p.surface));
    c->setAttribute ("surfaceAlt",      hex (p.surfaceAlt));
    c->setAttribute ("panelBorder",     hex (p.panelBorder));
    c->setAttribute ("textPrimary",     hex (p.textPrimary));
    c->setAttribute ("textMuted",       hex (p.textMuted));
    c->setAttribute ("accent",          hex (p.accent));
    c->setAttribute ("accentSecondary", hex (p.accentSecondary));
    c->setAttribute ("knobTrack",       hex (p.knobTrack));
    c->setAttribute ("knobFill",        hex (p.knobFill));
    c->setAttribute ("knobBody",        hex (p.knobBody));
    c->setAttribute ("shadow",          hex (p.shadow));
    c->setAttribute ("glow",            hex (p.glow));
    if (p.hasOpColours)                 // absent = les operateurs prennent l'accent
        for (int i = 0; i < 6; ++i)
            c->setAttribute ("op" + juce::String (i + 1), hex (p.opColour[i]));
    root.addChildElement (c);

    auto* a = new juce::XmlElement ("ASSETS");   // déposez knob.png / background.png dans ce dossier
    a->setAttribute ("knob", "");
    a->setAttribute ("background", "");
    root.addChildElement (a);

    auto* s = new juce::XmlElement ("STYLE");
    s->setAttribute ("buttons", p.buttonStyle.isNotEmpty() ? p.buttonStyle : "square");
    s->setAttribute ("panels",  p.panelStyle.isNotEmpty()  ? p.panelStyle  : "square");
    s->setAttribute ("sliders", p.sliderStyle.isNotEmpty() ? p.sliderStyle : "bar");
    s->setAttribute ("knobs",   p.knobStyle.isNotEmpty()   ? p.knobStyle   : "arc");
    root.addChildElement (s);

    return root.toString();
}

// Exporte les palettes intégrées en <Themes>/<nom>/theme.xml (modèles éditables, créés si absents).
inline void syExportBuiltinThemes()
{
    auto dir = syThemesDir();
    dir.createDirectory();
    for (auto& p : syBuiltinPalettes())
    {
        auto sub = dir.getChildFile (p.name.toLowerCase().replaceCharacter (' ', '-'));
        sub.createDirectory();
        auto xml = sub.getChildFile ("theme.xml");
        if (! xml.existsAsFile())
            xml.replaceWithText (syPaletteToXml (p));
    }
}

// Construit la liste des thèmes : intégrés d'abord (version dossier si éditée), puis thèmes utilisateur.
inline std::vector<SyPalette> syDiscoverThemes()
{
    std::vector<SyPalette> folderThemes;
    const auto builtins = syBuiltinPalettes();
    auto dir = syThemesDir();
    if (dir.isDirectory())
        for (auto& sub : dir.findChildFiles (juce::File::findDirectories, false))
        {
            auto xml = sub.getChildFile ("theme.xml");
            if (xml.existsAsFile())
                if (auto doc = juce::XmlDocument::parse (xml))
                {
                    // Version disque d'un theme integre : part de l'integre (champs nouveaux herites).
                    const auto nm = doc->getStringAttribute ("name", sub.getFileNameWithoutExtension());
                    const SyPalette* base = nullptr;
                    for (auto& b : builtins) if (b.name == nm) { base = &b; break; }
                    folderThemes.push_back (syParseTheme (*doc, sub, base));
                }
        }

    auto findByName = [&] (const juce::String& n) -> const SyPalette*
    {
        for (auto& t : folderThemes) if (t.name == n) return &t;
        return nullptr;
    };

    std::vector<SyPalette> v;
    juce::StringArray emitted;
    for (auto& b : syBuiltinPalettes())                 // intégrés d'abord (version dossier prioritaire)
    {
        if (auto* f = findByName (b.name)) v.push_back (*f);
        else                               v.push_back (b);
        emitted.add (b.name);
    }
    for (auto& t : folderThemes)                         // puis thèmes utilisateur supplémentaires
        if (! emitted.contains (t.name)) { v.push_back (t); emitted.add (t.name); }
    return v;
}

// Applique le thème nommé s'il existe dans la liste ; renvoie son index (ou -1 si introuvable).
inline int syApplyThemeByName (const std::vector<SyPalette>& themes, const juce::String& name)
{
    for (int i = 0; i < (int) themes.size(); ++i)
        if (themes[(size_t) i].name == name) { applySyPalette (themes[(size_t) i]); return i; }
    return -1;
}
