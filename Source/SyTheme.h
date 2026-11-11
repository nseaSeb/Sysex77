// SyTheme.h - palette a roles, metriques et registre de couleurs par role (sans LookAndFeel).
// Partie de LookAndFeel.h, qui l'inclut. Verbatim.
#pragma once
#include <JuceHeader.h>

//==============================================================================
// Palette à rôles nommés : décrit un thème complet (couleurs + métadonnées).
// Les 4 globales legacy (SYColBackground/Alt/Label/Selected) en sont dérivées,
// pour ne casser aucun des ~153 usages existants.
struct SyPalette
{
    Colour background, surface, surfaceAlt, panelBorder;
    Colour textPrimary, textMuted;
    Colour accent, accentSecondary;
    Colour knobTrack, knobFill, knobBody, shadow, glow;
    bool   dark = true;          // pilote contrastes/ombres (alpha)
    String name;                 // nom affiché
    File   folder;               // dossier du thème (résolution des assets relatifs ; vide = intégré)
    File   knobImage;            // skin optionnel du potard (sprite vertical, façon OscAfm)
    File   backgroundImage;      // image de fond optionnelle (rendu : hook à venir)
    String buttonStyle;          // "square" | "round" | "flat" | "fun"  (recettes codées dans ModernLookAndFeel)
    String panelStyle;           // "square" | "round" | "flat" | "studio" | "fun"
                                 // ("studio" = "flat" + titres/onglets en capitales, grilles pointillees...)
    String sliderStyle = "bar";  // "bar" | "led" | "rail" | "groove" | "field" (rendu des barres/sliders linéaires)
    String knobStyle   = "arc";  // "arc" | "dot" | "vintage"           (rendu des potards rotatifs)
    // Couleur d'identite des 6 operateurs FM (tuiles d'algo, onglets OP). Ignoree tant que
    // hasOpColours = false : syOpColour() rend alors l'accent (rendu historique inchange).
    Colour opColour[6] { Colour (0xffe8333f), Colour (0xffd6337a), Colour (0xffb03ad0),
                         Colour (0xff5a3ae0), Colour (0xff2e7be0), Colour (0xff2fd6e0) };
    bool   hasOpColours = false;
};

// Images du thème actif (vides = rendu procédural / fond uni). Rafraîchies à chaque applySyPalette.
static Image SYKnobImage;
static Image SYBackgroundImage;   // fond du thème (PNG/JPG) ; dessiné en mode cover dans MidiDemo::paint()

// Gabarits partagés par tous les overrides du ModernLookAndFeel (fini les nombres magiques).
namespace SyMetrics
{
    constexpr float corner = 6.0f, cornerLg = 10.0f, stroke = 1.0f;
    constexpr int   pad = 8, panelHeaderH = 18, shadowRadius = 8;
}

// Couleurs globales legacy (conservées : Config.h les lit/écrit, ~153 usages).
// Valeurs initiales = thème historique « Dark Orange » (identiques à l'ancien défaut).
static Colour SYColBackground { Colours::black };
static Colour   SYColAlt      { Colour (0xff202020) };
//static Colour   SYColText;
static Colour   SYColLabel    { Colours::grey };
static Colour   SYColSelected { Colours::darkorange };

// Thème de l interface : 0 = Dark (sombre, accent orange), 1 = Light (clair, monochrome, plat)
static int SYTheme = 0;

//==============================================================================
// Constructeurs des palettes intégrées.

// « Dark Orange » : l'identité historique (noir + orange), assainie par le ModernLookAndFeel.
inline SyPalette syPaletteDarkOrange()
{
    SyPalette p;
    p.name            = "Dark Orange";
    p.dark            = true;
    p.background      = Colours::black;
    p.surface         = Colour (0xff202020);
    p.surfaceAlt      = Colour (0xff2a2a2a);
    p.panelBorder     = Colour (0xff3a3a3a);
    p.textPrimary     = Colour (0xffe2e2e2);
    p.textMuted       = Colour (0xff909090);
    p.accent          = Colours::darkorange;
    p.accentSecondary = Colour (0xff3dd6c4);
    p.knobTrack       = Colour (0xff333333);
    p.knobFill        = Colours::darkorange;
    p.knobBody        = Colour (0xff262626);
    p.shadow          = Colour (0x66000000);
    p.glow            = Colours::darkorange.withAlpha (0.35f);
    p.buttonStyle     = "square";
    p.panelStyle      = "square";
    p.sliderStyle     = "bar";
    p.knobStyle       = "arc";
    return p;
}

// « FM Dark » : bleu ardoise profond + ambre chaud + teal secondaire (esprit FM synth moderne).
inline SyPalette syPaletteFmDark()
{
    SyPalette p;
    p.name            = "FM Dark";
    p.dark            = true;
    p.background      = Colour (0xff0d1117);   // bleu-noir profond (distinct du noir pur)
    p.surface         = Colour (0xff161b22);
    p.surfaceAlt      = Colour (0xff21262d);
    p.panelBorder     = Colour (0xff2d3340);   // légère teinte bleue
    p.textPrimary     = Colour (0xffe6e8ec);
    p.textMuted       = Colour (0xff7d8590);
    p.accent          = Colour (0xffff8a3d);
    p.accentSecondary = Colour (0xff3dd6c4);
    p.knobTrack       = Colour (0xff252d38);
    p.knobFill        = Colour (0xffff8a3d);
    p.knobBody        = Colour (0xff1c2028);
    p.shadow          = Colour (0x80000000);
    p.glow            = Colour (0x55ff8a3d);
    p.buttonStyle     = "flat";
    p.panelStyle      = "flat";
    p.sliderStyle     = "led";
    p.knobStyle       = "arc";
    return p;
}

// « Light » : panneaux clairs froids + accent bleu, ombres légères (mode clair de jour).
inline SyPalette syPaletteLight()
{
    SyPalette p;
    p.name            = "Light";
    p.dark            = false;
    p.background      = Colour (0xffe9ebef);
    p.surface         = Colour (0xfff5f6f8);
    p.surfaceAlt      = Colour (0xffffffff);
    p.panelBorder     = Colour (0xffd2d6dd);
    p.textPrimary     = Colour (0xff1c2026);
    p.textMuted       = Colour (0xff6b727e);
    p.accent          = Colour (0xff2f7df6);
    p.accentSecondary = Colour (0xfff5a623);
    p.knobTrack       = Colour (0xffd8dce3);
    p.knobFill        = Colour (0xff2f7df6);
    p.knobBody        = Colour (0xffffffff);
    p.shadow          = Colour (0x22000000);
    p.glow            = Colour (0x332f7df6);
    p.buttonStyle     = "round";   // boutons on/off arrondis…
    p.panelStyle      = "flat";    // …mais cards plates (pas de cards arrondies/circulaires)
    p.sliderStyle     = "bar";
    p.knobStyle       = "dot";     // potards minimalistes (anneau + point)
    return p;
}

// « Crimson » : noir teinté rouge + rouge-sang + accents ambrés.
inline SyPalette syPaletteCrimson()
{
    SyPalette p;
    p.name            = "Crimson";
    p.dark            = true;
    p.background      = Colour (0xff110707);   // noir chaud teinté rouge
    p.surface         = Colour (0xff1e1010);
    p.surfaceAlt      = Colour (0xff291616);
    p.panelBorder     = Colour (0xff3d2222);   // teinte rouge marquée
    p.textPrimary     = Colour (0xfff0e8e8);   // blanc légèrement chaud
    p.textMuted       = Colour (0xff9a8080);
    p.accent          = Colour (0xffff5a4d);
    p.accentSecondary = Colour (0xfff5a623);
    p.knobTrack       = Colour (0xff3a2020);
    p.knobFill        = Colour (0xffff5a4d);
    p.knobBody        = Colour (0xff231414);
    p.shadow          = Colour (0x80000000);
    p.glow            = Colour (0x55ff5a4d);
    p.buttonStyle     = "fun";
    p.panelStyle      = "round";
    p.sliderStyle     = "groove";
    p.knobStyle       = "vintage";
    return p;
}

// « Tangerine » : charbon chaud ambré + orange brûlé + secondaire bleu.
inline SyPalette syPaletteTangerine()
{
    SyPalette p;
    p.name            = "Tangerine";
    p.dark            = true;
    p.background      = Colour (0xff18140a);   // noir teinté ambre/bois
    p.surface         = Colour (0xff251f12);
    p.surfaceAlt      = Colour (0xff302819);
    p.panelBorder     = Colour (0xff42361f);   // teinte bois chaude
    p.textPrimary     = Colour (0xfff0ece8);   // blanc crème
    p.textMuted       = Colour (0xff9a9080);
    p.accent          = Colour (0xffff6a1a);
    p.accentSecondary = Colour (0xff37a6ff);
    p.knobTrack       = Colour (0xff3a2e1a);
    p.knobFill        = Colour (0xffff6a1a);
    p.knobBody        = Colour (0xff261e10);
    p.shadow          = Colour (0x80000000);
    p.glow            = Colour (0x55ff6a1a);
    p.buttonStyle     = "round";
    p.panelStyle      = "square";
    p.sliderStyle     = "rail";
    p.knobStyle       = "arc";
    return p;
}

// "Indigo Night" : gris anthracite (pas de noir pur), accent indigo unique reserve aux
// valeurs/etats, panneaux sans cadre (titre + filet), potards a arc fin. Style "studio" sobre.
inline SyPalette syPaletteIndigoNight()
{
    SyPalette p;
    p.name            = "Indigo Night";
    p.dark            = true;
    p.background      = Colour (0xff1a1a1c);
    p.surface         = Colour (0xff222224);
    p.surfaceAlt      = Colour (0xff2e2e30);
    p.panelBorder     = Colour (0xff38383c);
    p.textPrimary     = Colour (0xffe8e8ea);
    p.textMuted       = Colour (0xff8a8a90);
    p.accent          = Colour (0xff8b5cf0);
    p.accentSecondary = Colour (0xff2fd6e0);
    p.knobTrack       = Colour (0xff2c2c30);
    p.knobFill        = Colour (0xffa98af0);
    p.knobBody        = Colour (0xff27272a);
    p.shadow          = Colour (0x70000000);
    p.glow            = Colour (0x338b5cf0);
    p.buttonStyle     = "flat";
    p.panelStyle      = "studio";
    p.sliderStyle     = "field";
    p.knobStyle       = "thin";
    p.hasOpColours    = true;    // palette op1..op6 par defaut (rouge -> cyan)
    return p;
}

// Palette active (initialisée au thème historique pour rester identique avant tout applySyTheme).
static SyPalette SYPal = syPaletteDarkOrange();

// Panneaux sans cadre : "flat" et "studio" (studio = flat + habillage studio, cf. syPanelStudio).
inline bool syPanelFlat()   { return SYPal.panelStyle == "flat" || SYPal.panelStyle == "studio"; }
// Habillage "studio" : onglets/titres en capitales, grilles pointillees, poignees creuses,
// filet ROUTAGE, cartes a filet discret. Reserve au panelStyle "studio" (Indigo Night).
inline bool syPanelStudio() { return SYPal.panelStyle == "studio"; }

// Bordure des cartes dessinees a la main (ROUTAGE, PAN, ecrans d'EG) : accent vif par defaut,
// filet discret pour le style "studio" (l'accent reste reserve aux valeurs).
inline Colour syCardBorder()
{
    return syPanelStudio() ? SYPal.panelBorder : SYColSelected;
}

// Couleur d'identite de l'operateur FM opIndex0 (0..5) ; accent si le theme n'en definit pas.
inline Colour syOpColour (int opIndex0)
{
    if (! SYPal.hasOpColours || opIndex0 < 0 || opIndex0 > 5) return SYPal.accent;
    return SYPal.opColour[opIndex0];
}

// Dérive les 4 alias legacy depuis la palette active.
inline void deriveLegacyAliases()
{
    SYColBackground = SYPal.background;
    SYColAlt        = SYPal.surface;
    SYColLabel      = SYPal.textPrimary;
    SYColSelected   = SYPal.accent;
}

inline void syncSyLookAndFeel();   // déclaration anticipée

// Pousse les 4 alias (modifiés par les ColourSelectors) dans les rôles de la palette active,
// puis re-synchronise le LookAndFeel. Garde palette et alias cohérents pour un thème « Custom ».
inline void pushAliasesToPalette()
{
    SYPal.background  = SYColBackground;
    SYPal.surface     = SYColAlt;
    SYPal.textPrimary = SYColLabel;
    SYPal.accent      = SYColSelected;
    SYPal.knobFill    = SYColSelected;
    SYPal.name        = "Custom";
    syncSyLookAndFeel();
}

// Choisit le LookAndFeel selon SYTheme (défini en fin de fichier).
inline void selectSyLookAndFeel();

// Applique les couleurs globales courantes (SYCol*) au LookAndFeel par défaut.
// Ces couleurs sont lues au moment du paint -> un repaint suffit pour les voir.
inline void syncSyLookAndFeel()
{
    auto& lf  = LookAndFeel::getDefaultLookAndFeel();
    auto  ink = SYPal.textPrimary; // encre lisible (rôle palette)

    lf.setColour (ResizableWindow::backgroundColourId, SYPal.background);
    lf.setColour (DocumentWindow::backgroundColourId,  SYPal.background);

    lf.setColour (Label::textColourId,             ink);
    lf.setColour (GroupComponent::textColourId,    ink);
    lf.setColour (GroupComponent::outlineColourId, SYPal.panelBorder);

    lf.setColour (TextButton::buttonColourId,      SYPal.surfaceAlt);
    lf.setColour (TextButton::buttonOnColourId,    SYPal.accent);   // état ON = accent (suit le thème)
    lf.setColour (TextButton::textColourOffId,     ink);
    lf.setColour (TextButton::textColourOnId,      SYPal.accent.contrasting());

    // Cases à cocher / interrupteurs : couleurs lues en direct (les widgets n'ont plus à les figer).
    lf.setColour (ToggleButton::textColourId,         ink);
    lf.setColour (ToggleButton::tickColourId,         SYPal.accent);
    lf.setColour (ToggleButton::tickDisabledColourId, SYPal.textMuted);

    lf.setColour (ComboBox::backgroundColourId,    SYPal.surfaceAlt);
    lf.setColour (ComboBox::textColourId,          ink);
    lf.setColour (ComboBox::arrowColourId,         SYPal.textMuted);
    lf.setColour (ComboBox::outlineColourId,       SYPal.panelBorder);

    // listes / tables
    lf.setColour (ListBox::backgroundColourId,     SYPal.surfaceAlt);
    lf.setColour (ListBox::textColourId,           ink);
    lf.setColour (ListBox::outlineColourId,        SYPal.panelBorder);   // contour = bordure du thème (sinon gris JUCE)

    // menus déroulants
    lf.setColour (PopupMenu::backgroundColourId,            SYPal.surfaceAlt);
    lf.setColour (PopupMenu::textColourId,                  ink);
    lf.setColour (PopupMenu::highlightedBackgroundColourId, SYPal.accent);
    lf.setColour (PopupMenu::highlightedTextColourId,       SYPal.accent.contrasting());

    // barres de défilement
    lf.setColour (ScrollBar::thumbColourId,        SYPal.textMuted);

    // éditeurs de texte (ex. moniteur MIDI)
    lf.setColour (TextEditor::backgroundColourId,  SYPal.surfaceAlt);
    lf.setColour (TextEditor::textColourId,        ink);
    lf.setColour (TextEditor::outlineColourId,     SYPal.panelBorder);
    lf.setColour (TextEditor::focusedOutlineColourId, SYPal.accent);
    lf.setColour (CaretComponent::caretColourId,   SYPal.accent);
}

// Charge les assets du thème actif depuis le disque (champs vides = Image() = rendu procédural).
inline void refreshThemeAssets()
{
    SYKnobImage = SYPal.knobImage.existsAsFile()
                    ? ImageCache::getFromFile (SYPal.knobImage)
                    : Image();
    SYBackgroundImage = SYPal.backgroundImage.existsAsFile()
                          ? ImageCache::getFromFile (SYPal.backgroundImage)
                          : Image();
}

// Applique une palette : la rend active, dérive les alias legacy, charge les assets, (re)sélectionne le LAF.
// Registre des labels à couleur de RÔLE (accent, muted, fond...). setColour() FIGE la couleur
// au moment de l'appel : un label réglé à la construction garde la couleur du thème de l'époque
// et ne suit pas un changement de thème. On enregistre (label, fonction rôle->couleur lue en
// direct sur SYPal) ; applySyPalette ré-applique => ces labels suivent le thème. Les labels à
// couleur de TEXTE standard n'ont PAS besoin de ça : ils héritent du défaut vivant du LookAndFeel.
struct SyRoleColours
{
    static SyRoleColours& instance() { static SyRoleColours s; return s; }
    std::vector<std::pair<Component::SafePointer<Component>, std::function<void()>>> items;

    void add (Component& c, std::function<void()> apply)
    {
        apply();
        items.push_back ({ Component::SafePointer<Component> (&c), std::move (apply) });
    }
    void refresh()
    {
        items.erase (std::remove_if (items.begin(), items.end(),
                        [] (auto& it) { return it.first == nullptr; }), items.end());
        for (auto& it : items)
            if (it.first != nullptr) it.second();   // SafePointer vivant -> la closure peut déréférencer
    }
};

// Enregistre un Component dont des couleurs sont des RÔLES de palette (ré-appliquées à chaque thème).
inline void registerThemeColour (Component& c, std::function<void()> apply)
{
    SyRoleColours::instance().add (c, std::move (apply));
}
// Label : couleurs arbitraires (texte + fond) en rôle.
inline void setRoleLabel (Label& l, std::function<void (Label&)> apply)
{
    registerThemeColour (l, [&l, apply = std::move (apply)] { apply (l); });
}
// Label : cas courant, seule la couleur de TEXTE est un rôle.
inline void setRoleLabelColour (Label& l, std::function<Colour()> role)
{
    registerThemeColour (l, [&l, role = std::move (role)] { l.setColour (Label::textColourId, role()); });
}

inline void applySyPalette (const SyPalette& p)
{
    SYPal   = p;
    SYTheme = p.dark ? 0 : 1;   // compat : 0 = sombre, 1 = clair
    deriveLegacyAliases();
    refreshThemeAssets();
    selectSyLookAndFeel();
    SyRoleColours::instance().refresh();  // les composants à couleur de rôle suivent le nouveau thème
}

// Compat historique : 0 = Dark (Dark Orange), 1 = Light.
inline void applySyTheme (int theme)
{
    applySyPalette (theme == 1 ? syPaletteLight() : syPaletteDarkOrange());
}
