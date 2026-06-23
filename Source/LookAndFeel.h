/*
  ==============================================================================

    LookAndFeel.h
    Created: 18 Nov 2018 8:34:30pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

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
    String panelStyle;           // "square" | "round" | "flat" | "fun"
    String sliderStyle = "bar";  // "bar" | "led" | "rail" | "groove"   (rendu des barres/sliders linéaires)
    String knobStyle   = "arc";  // "arc" | "dot" | "vintage"           (rendu des potards rotatifs)
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

// Palette active (initialisée au thème historique pour rester identique avant tout applySyTheme).
static SyPalette SYPal = syPaletteDarkOrange();

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
    lf.setColour (TextButton::textColourOffId,     ink);
    lf.setColour (TextButton::textColourOnId,      SYPal.accent.contrasting());

    lf.setColour (ComboBox::backgroundColourId,    SYPal.surfaceAlt);
    lf.setColour (ComboBox::textColourId,          ink);
    lf.setColour (ComboBox::arrowColourId,         SYPal.textMuted);
    lf.setColour (ComboBox::outlineColourId,       SYPal.panelBorder);

    // listes / tables
    lf.setColour (ListBox::backgroundColourId,     SYPal.surfaceAlt);
    lf.setColour (ListBox::textColourId,           ink);

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
inline void applySyPalette (const SyPalette& p)
{
    SYPal   = p;
    SYTheme = p.dark ? 0 : 1;   // compat : 0 = sombre, 1 = clair
    deriveLegacyAliases();
    refreshThemeAssets();
    selectSyLookAndFeel();
}

// Compat historique : 0 = Dark (Dark Orange), 1 = Light.
inline void applySyTheme (int theme)
{
    applySyPalette (theme == 1 ? syPaletteLight() : syPaletteDarkOrange());
}


//==============================================================================
// Forme d'onde d'un opérateur AFM : on REND la vraie image SY77 (donc les formes
// EXACTES du synthé : pas de carré/triangle/dent-de-scie inventés), mais TEINTÉE à
// la couleur d'accent du thème actif — le sprite d'origine est figé en orange. Le
// drapeau fillAlphaChannelWithCurrentBrush utilise l'alpha de l'image comme masque
// et la remplit avec le pinceau courant. Bonus : un léger halo, sans nouvel asset.
class AfmWaveLookAndFeel : public LookAndFeel_V4
{
public:
    AfmWaveLookAndFeel()
    {
        sprite = ImageCache::getFromMemory (BinaryData::OscAfm_png, BinaryData::OscAfm_pngSize);
    }

    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float /*pos*/,
                           float /*startA*/, float /*endA*/, Slider& s) override
    {
        if (! sprite.isValid())
            return;

        const int frameW = sprite.getWidth();
        const int frames  = jmax (1, sprite.getHeight() / frameW);
        const int fid     = jlimit (0, frames - 1, roundToInt (s.getValue()));
        auto      frame   = sprite.getClippedImage ({ 0, fid * frameW, frameW, frameW });

        const float radius = jmin (w, h) * 0.5f;
        const float cx = x + w * 0.5f, cy = y + h * 0.5f;
        const float scale = (radius * 2.0f) / (float) frameW;
        const auto  base  = AffineTransform::scale (scale).translated (cx - radius, cy - radius);

        const Colour accent = SYPal.knobFill;
        g.setImageResamplingQuality (Graphics::highResamplingQuality);

        // Halo doux : passe légèrement agrandie, faible alpha.
        g.setColour (accent.withAlpha (0.20f));
        g.drawImageTransformed (frame,
            AffineTransform::scale (scale * 1.06f).translated (cx - radius * 1.06f, cy - radius * 1.06f),
            true);

        // Forme nette teintée à l'accent du thème.
        g.setColour (accent);
        g.drawImageTransformed (frame, base, true);
    }

private:
    Image sprite;
};

//==============================================================================
// LookAndFeel moderne unique, piloté par la palette SYPal (lue au paint).
// Rend les DEUX esthétiques (clair/sombre) sans sous-classe : potards à arc +
// dégradé + ombre, sliders arrondis, boutons/onglets « cartes », encre calculée
// depuis le fond réel (corrige les états sélectionnés illisibles).
class ModernLookAndFeel : public LookAndFeel_V4
{
public:
    ModernLookAndFeel()
    {
        interRegular  = Typeface::createSystemTypefaceFor (BinaryData::InterRegular_ttf,  BinaryData::InterRegular_ttfSize);
        interSemiBold = Typeface::createSystemTypefaceFor (BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize);
    }

    // Police moderne embarquée (Inter, OFL) pour le texte sans-serif ; conserve le monospace (diff).
    Typeface::Ptr getTypefaceForFont (const Font& f) override
    {
        const auto name = f.getTypefaceName();
        if (name == Font::getDefaultSansSerifFontName() || name.isEmpty())
        {
            if (f.isBold() && interSemiBold != nullptr) return interSemiBold;
            if (interRegular != nullptr)                return interRegular;
        }
        return LookAndFeel_V4::getTypefaceForFont (f);
    }

    //========================================================= Potards
    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, Slider& s) override
    {
        // Skin optionnel : sprite vertical fourni par le thème (sinon rendu procédural ci-dessous).
        if (SYKnobImage.isValid())
        {
            const int frames  = jmax (1, SYKnobImage.getHeight() / jmax (1, SYKnobImage.getWidth()));
            const int frameId = jlimit (0, frames - 1, (int) std::lround (pos * (frames - 1)));
            const float radius = jmin (w, h) * 0.5f;
            const float cx = x + w * 0.5f, cy = y + h * 0.5f;
            g.drawImage (SYKnobImage, (int) (cx - radius), (int) (cy - radius),
                         (int) (radius * 2), (int) (radius * 2),
                         0, frameId * SYKnobImage.getWidth(), SYKnobImage.getWidth(), SYKnobImage.getWidth());
            return;
        }

        auto bounds = Rectangle<int> (x, y, w, h).toFloat().reduced (2.0f);
        const float cx = bounds.getCentreX(), cy = bounds.getCentreY();
        const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const float angle  = startA + pos * (endA - startA);

        // Recettes de potard pilotées par le thème (sinon « arc » par défaut ci-dessous).
        if (SYPal.knobStyle == "dot")     { drawKnobDot     (g, cx, cy, radius, startA, endA, angle, s); return; }
        if (SYPal.knobStyle == "vintage") { drawKnobVintage (g, cx, cy, radius, angle, s);              return; }

        const float ringThick = jmax (3.0f, radius * 0.30f);
        const float ringR      = radius - ringThick * 0.5f - 1.0f;
        if (ringR <= 1.0f) return;

        // Piste (arc complet)
        Path track;
        track.addCentredArc (cx, cy, ringR, ringR, 0.0f, startA, endA, true);
        g.setColour (SYPal.knobTrack);
        g.strokePath (track, PathStrokeType (ringThick, PathStrokeType::curved, PathStrokeType::rounded));

        // Arc de valeur (dégradé accent) + glow si actif
        // Contrôle bipolaire (ex. pan) : l'arc part du centre (12h) vers la gauche ou la droite.
        const float centreA = (startA + endA) * 0.5f;
        const bool  bipolar = (s.getMinimum() < -0.01 && s.getMaximum() > 0.01);
        if (s.isEnabled())
        {
            Path val;
            if (bipolar)
                val.addCentredArc (cx, cy, ringR, ringR, 0.0f,
                                   jmin (centreA, angle), jmax (centreA, angle), true);
            else
                val.addCentredArc (cx, cy, ringR, ringR, 0.0f, startA, angle, true);
            // Halo de l'arc : permanent (discret) + renforcé au survol.
            const bool hot = s.isMouseOverOrDragging();
            g.setColour (SYPal.glow.withMultipliedAlpha (hot ? 1.0f : 0.45f));
            g.strokePath (val, PathStrokeType (ringThick + (hot ? 3.5f : 1.5f),
                                               PathStrokeType::curved, PathStrokeType::rounded));
            g.setGradientFill (ColourGradient (SYPal.knobFill.brighter (0.25f), cx, bounds.getY(),
                                               SYPal.knobFill,                  cx, bounds.getBottom(), false));
            g.strokePath (val, PathStrokeType (ringThick, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Corps bombé + ombre douce + repère
        const float bodyR = ringR - ringThick * 0.5f - 2.0f;
        if (bodyR > 1.0f)
        {
            Rectangle<float> body (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
            Path bodyPath; bodyPath.addEllipse (body);
            DropShadow (SYPal.shadow, SyMetrics::shadowRadius, { 0, 2 }).drawForPath (g, bodyPath);
            g.setGradientFill (ColourGradient (SYPal.knobBody.brighter (SYPal.dark ? 0.18f : 0.04f), cx, body.getY(),
                                               SYPal.knobBody.darker   (SYPal.dark ? 0.20f : 0.06f), cx, body.getBottom(), false));
            g.fillEllipse (body);
            g.setColour (SYPal.panelBorder);
            g.drawEllipse (body, 1.0f);

            Path ptr;
            const float pw = jmax (2.0f, bodyR * 0.16f);
            ptr.addRoundedRectangle (-pw * 0.5f, -bodyR + 2.0f, pw, bodyR * 0.55f, pw * 0.5f);
            ptr.applyTransform (AffineTransform::rotation (angle).translated (cx, cy));
            g.setColour (SYPal.accent);
            g.fillPath (ptr);
        }
    }

    //========================================================= Sliders linéaires
    void drawLinearSlider (Graphics& g, int x, int y, int w, int h,
                           float sliderPos, float minPos, float maxPos,
                           Slider::SliderStyle style, Slider& s) override
    {
        // Contrôle bipolaire (ex. pan, detune, accord ±) : la portion remplie part du
        // centre (point neutre) au lieu d'une extrémité — vrai pour TOUS les styles.
        const bool bipolar = (s.getMinimum() < -0.01 && s.getMaximum() > 0.01);

        if (style == Slider::LinearVertical)
        {
            const float cx = x + w * 0.5f;
            const float railW = jmin (5.0f, (float) w * 0.4f);
            g.setColour (SYPal.knobTrack);
            g.fillRoundedRectangle (cx - railW * 0.5f, (float) y, railW, (float) h, railW * 0.5f);
            g.setColour (SYPal.accent);
            const float originY = bipolar ? (y + h * 0.5f) : (float) (y + h);   // centre si bipolaire
            g.fillRoundedRectangle (cx - railW * 0.5f, jmin (originY, sliderPos),
                                    railW, std::abs (sliderPos - originY), railW * 0.5f);
            drawSliderThumb (g, Rectangle<float> ((float) x + 1.0f, sliderPos - 4.0f, (float) w - 2.0f, 8.0f));
        }
        else if (style == Slider::LinearHorizontal)
        {
            const float cy = y + h * 0.5f;
            const float railH = jmin (5.0f, (float) h * 0.4f);
            g.setColour (SYPal.knobTrack);
            g.fillRoundedRectangle ((float) x, cy - railH * 0.5f, (float) w, railH, railH * 0.5f);
            g.setColour (SYPal.accent);
            const float originX = bipolar ? (x + w * 0.5f) : (float) x;          // centre si bipolaire
            g.fillRoundedRectangle (jmin (originX, sliderPos), cy - railH * 0.5f,
                                    std::abs (sliderPos - originX), railH, railH * 0.5f);
            drawSliderThumb (g, Rectangle<float> (sliderPos - 4.0f, (float) y + 1.0f, 8.0f, (float) h - 2.0f));
        }
        else if (style == Slider::LinearBar || style == Slider::LinearBarVertical)
        {
            // Barre de valeur : rendu piloté par le thème (bar/led/rail/groove).
            Rectangle<float> r ((float) x, (float) y, (float) w, (float) h);
            const bool horiz = (style == Slider::LinearBar);
            const float pos01 = (s.getMaximum() > s.getMinimum())
                ? (float) ((s.getValue() - s.getMinimum()) / (s.getMaximum() - s.getMinimum())) : 0.0f;
            drawValueBar (g, r, pos01, horiz, bipolar, s.isEnabled(), s.isMouseOverOrDragging());

            // Valeur numérique lisible dans la barre (lecture directe, façon éditeur SY77).
            if (horiz && h >= 12)
            {
                g.setColour (s.isEnabled() ? SYPal.textPrimary : SYPal.textMuted);
                g.setFont (Font (FontOptions (jmin (13.0f, (float) h * 0.7f))));
                g.drawText (String (roundToInt (s.getValue())),
                            r.reduced (5.0f, 0.0f).toNearestInt(), Justification::centredRight, false);
            }
        }
        else
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, minPos, maxPos, style, s);
    }

    //========================================================= Boutons
    void drawButtonBackground (Graphics& g, Button& b, const Colour&,
                               bool isOver, bool isDown) override
    {
        auto r   = b.getLocalBounds().toFloat().reduced (0.5f);
        auto bg  = buttonFill (b, isOver, isDown);
        const bool on = b.getToggleState();
        const auto& style = SYPal.buttonStyle;

        if (style == "flat")
        {
            if (on)
            {
                g.setColour (SYPal.accent.withAlpha (0.14f));
                g.fillRoundedRectangle (r, 4.0f);
                g.setColour (SYPal.accent);
                g.fillRect (r.getX() + 3.0f, r.getBottom() - 2.5f, r.getWidth() - 6.0f, 2.5f);
            }
            else
            {
                g.setColour (bg.withAlpha (isOver ? 0.75f : 0.45f));
                g.fillRoundedRectangle (r, 4.0f);
            }
        }
        else if (style == "round")
        {
            const float cr = r.getHeight() * 0.5f;
            g.setColour (bg);
            g.fillRoundedRectangle (r, cr);
            g.setColour (on ? SYPal.accent : SYPal.panelBorder.withAlpha (0.6f));
            g.drawRoundedRectangle (r, cr, on ? 1.5f : SyMetrics::stroke);
        }
        else if (style == "fun")
        {
            const float cr = 8.0f;
            Path bp; bp.addRoundedRectangle (r, cr);
            DropShadow (SYPal.shadow, 5, { 1, 2 }).drawForPath (g, bp);
            g.setGradientFill (ColourGradient (bg.brighter (0.18f), r.getX(), r.getY(),
                                               bg.darker   (0.18f), r.getRight(), r.getBottom(), false));
            g.fillPath (bp);
            g.setColour (on ? SYPal.accent : SYPal.panelBorder);
            g.drawRoundedRectangle (r, cr, on ? 2.0f : SyMetrics::stroke);
        }
        else   // "square" (défaut + legacy Dark Orange)
        {
            const float cr = 2.0f;
            g.setGradientFill (ColourGradient (bg.brighter (SYPal.dark ? 0.06f : 0.02f), r.getX(), r.getY(),
                                               bg.darker   (SYPal.dark ? 0.10f : 0.04f), r.getX(), r.getBottom(), false));
            g.fillRoundedRectangle (r, cr);
            g.setColour (on ? SYPal.accent : SYPal.panelBorder);
            g.drawRoundedRectangle (r, cr, SyMetrics::stroke);
        }
    }

    void drawButtonText (Graphics& g, TextButton& b, bool isOver, bool isDown) override
    {
        auto bg  = buttonFill (b, isOver, isDown);
        // flat ON : texte couleur accent (fond est semi-transparent, contrasting() serait faux)
        Colour ink = (SYPal.buttonStyle == "flat" && b.getToggleState())
                         ? SYPal.accent
                         : bg.contrasting();
        g.setColour (b.isEnabled() ? ink : ink.withAlpha (0.5f));
        g.setFont (getTextButtonFont (b, b.getHeight()));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (SyMetrics::pad, 0),
                          Justification::centred, 2);
    }

    //========================================================= ComboBox
    void drawComboBox (Graphics& g, int w, int h, bool,
                       int, int, int, int, ComboBox& box) override
    {
        Rectangle<float> r (0.5f, 0.5f, (float) w - 1.0f, (float) h - 1.0f);
        g.setColour (SYPal.surfaceAlt);
        g.fillRoundedRectangle (r, SyMetrics::corner);
        const bool active = box.isPopupActive();
        g.setColour (active ? SYPal.accent : SYPal.panelBorder);
        g.drawRoundedRectangle (r, SyMetrics::corner, active ? 1.5f : SyMetrics::stroke);

        Rectangle<float> arrow ((float) w - 18.0f, 0.0f, 14.0f, (float) h);
        arrow = arrow.reduced (3.0f, (float) h * 0.42f);
        Path p;
        p.addTriangle (arrow.getX(), arrow.getY(),
                       arrow.getRight(), arrow.getY(),
                       arrow.getCentreX(), arrow.getBottom());
        g.setColour (SYPal.textMuted);
        g.fillPath (p);
    }

    //========================================================= Panneaux « cartes »
    // SANS remplissage opaque : le contenu peint derrière (routage, courbes, EG) reste visible.
    void drawGroupComponentOutline (Graphics& g, int w, int h, const String& text,
                                    const Justification&, GroupComponent&) override
    {
        Rectangle<float> r (1.0f, 1.0f, (float) w - 2.0f, (float) h - 2.0f);
        const auto& style = SYPal.panelStyle;

        if (style == "flat")
        {
            // Pas de contour de pourtour — séparateur horizontal sous le titre uniquement
            if (text.isNotEmpty())
            {
                g.setColour (SYPal.panelBorder.withAlpha (0.6f));
                g.drawLine (r.getX() + 6.0f, r.getY() + (float) SyMetrics::panelHeaderH,
                            r.getRight() - 6.0f, r.getY() + (float) SyMetrics::panelHeaderH, 1.0f);
            }
        }
        else if (style == "round")
        {
            // Coins arrondis, mais PLAFONNÉS : le style "round" est pensé pour des boutons
            // ronds, pas pour des cards circulaires (sinon les bords passent sous le contenu).
            const float cr = jmin (jmin (r.getWidth(), r.getHeight()) * 0.25f, 12.0f);
            g.setColour (SYPal.panelBorder);
            g.drawRoundedRectangle (r, cr, SyMetrics::stroke);
        }
        else if (style == "fun")
        {
            // Léger remplissage translucide + bord accent atténué + liseré « neon »
            const float cr = 12.0f;
            g.setColour (SYPal.surface.withAlpha (SYPal.dark ? 0.45f : 0.30f));
            g.fillRoundedRectangle (r, cr);
            g.setColour (SYPal.accent.withAlpha (0.30f));
            g.drawRoundedRectangle (r, cr, 1.5f);
            g.setColour (SYPal.accent.withAlpha (0.18f));
            g.drawLine (r.getX() + cr, r.getY() + 1.0f, r.getRight() - cr, r.getY() + 1.0f, 2.0f);
        }
        else   // "square" (défaut + legacy)
        {
            g.setColour (SYPal.panelBorder);
            g.drawRoundedRectangle (r, 4.0f, SyMetrics::stroke);
            g.setColour (SYPal.textPrimary.withAlpha (SYPal.dark ? 0.05f : 0.06f));
            g.drawLine (r.getX() + 4.0f, r.getY() + 1.0f, r.getRight() - 4.0f, r.getY() + 1.0f, 1.0f);
        }

        if (text.isNotEmpty())
        {
            g.setColour (style == "flat" ? SYPal.textMuted : SYPal.textPrimary);
            g.setFont (Font (FontOptions (13.0f)).boldened());
            g.drawText (text, 12, 3, w - 24, SyMetrics::panelHeaderH, Justification::left, true);
        }
    }

    //========================================================= Onglets
    void drawTabButton (TabBarButton& button, Graphics& g, bool isOver, bool) override
    {
        auto area = button.getActiveArea().toFloat();
        const bool front = button.isFrontTab();
        g.setColour (front ? SYPal.surface : (isOver ? SYPal.surfaceAlt : SYPal.background));
        g.fillRect (area);
        if (front)
        {
            g.setColour (SYPal.accent);
            g.fillRect (area.getX(), area.getY(), area.getWidth(), 2.5f); // liseré accent côté contenu
        }
        g.setColour (front ? SYPal.textPrimary : SYPal.textMuted);
        g.setFont (Font (FontOptions (14.0f)));
        g.drawFittedText (button.getButtonText().trim(), area.reduced (8, 0).toNearestInt(),
                          Justification::centred, 1);
    }

    void drawTabAreaBehindFrontButton (TabbedButtonBar&, Graphics&, int, int) override {}

    //========================================================= Cases à cocher / switches
    void drawToggleButton (Graphics& g, ToggleButton& b, bool, bool) override
    {
        auto bounds = b.getLocalBounds().toFloat();
        const float boxSize = jmin (18.0f, bounds.getHeight() - 2.0f);
        Rectangle<float> box (bounds.getX() + 1.0f, bounds.getCentreY() - boxSize * 0.5f, boxSize, boxSize);
        const bool on = b.getToggleState();
        g.setColour (on ? SYPal.accent : SYPal.surfaceAlt);
        g.fillRoundedRectangle (box, 4.0f);
        g.setColour (on ? SYPal.accent : SYPal.panelBorder);
        g.drawRoundedRectangle (box, 4.0f, SyMetrics::stroke);
        if (on)
        {
            Path tick;
            tick.startNewSubPath (box.getX() + boxSize * 0.26f, box.getCentreY());
            tick.lineTo (box.getCentreX() - boxSize * 0.04f, box.getBottom() - boxSize * 0.28f);
            tick.lineTo (box.getRight() - boxSize * 0.22f, box.getY() + boxSize * 0.28f);
            g.setColour (SYPal.accent.contrasting());
            g.strokePath (tick, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (SYPal.textPrimary);
        g.setFont (Font (FontOptions ((float) jmin (15, b.getHeight()))));
        g.drawFittedText (b.getButtonText(),
                          b.getLocalBounds().withTrimmedLeft ((int) boxSize + 8),
                          Justification::centredLeft, 2);
    }

private:
    // Fond effectif d'un bouton selon son état (toggle ON = accent ; sinon couleur de base + survol/appui).
    Colour buttonFill (Button& b, bool isOver, bool isDown) const
    {
        if (b.getToggleState())  return SYPal.accent;
        auto base = b.findColour (TextButton::buttonColourId);
        if (isDown)  return base.darker   (SYPal.dark ? 0.25f : 0.12f);
        if (isOver)  return base.brighter (SYPal.dark ? 0.10f : 0.04f);
        return base;
    }

    void drawSliderThumb (Graphics& g, Rectangle<float> th)
    {
        Path p; p.addRoundedRectangle (th, jmin (th.getWidth(), th.getHeight()) * 0.5f);
        DropShadow (SYPal.shadow, 4, { 0, 1 }).drawForPath (g, p);
        g.setColour (SYPal.surfaceAlt);
        g.fillPath (p);
        g.setColour (SYPal.panelBorder);
        g.strokePath (p, PathStrokeType (SyMetrics::stroke));
    }

    //========================================================= Recettes de barre de valeur
    // pos01 = 0..1 ; horiz = LinearBar (h) vs LinearBarVertical (v) ; bipolar = remplit depuis le centre.
    void drawValueBar (Graphics& g, Rectangle<float> r, float pos01, bool horiz,
                       bool bipolar, bool enabled, bool hot)
    {
        pos01 = jlimit (0.0f, 1.0f, pos01);
        const Colour acc = SYPal.accent.withMultipliedAlpha (enabled ? (hot ? 1.0f : 0.9f) : 0.4f);
        const float  px  = r.getX() + pos01 * r.getWidth();
        const float  ox  = bipolar ? r.getCentreX() : r.getX();   // origine du remplissage (h)

        if (SYPal.sliderStyle == "led")
        {
            g.setColour (SYPal.knobTrack);
            g.fillRect (r);
            const int n = jmax (6, (int) ((horiz ? r.getWidth() : r.getHeight()) / 9.0f));
            const int lit = (int) std::round (pos01 * (float) n);
            const float cell = (horiz ? r.getWidth() : r.getHeight()) / (float) n;
            for (int i = 0; i < n; ++i)
            {
                auto seg = horiz
                    ? Rectangle<float> (r.getX() + i * cell, r.getY(), cell, r.getHeight()).reduced (cell * 0.16f, r.getHeight() * 0.18f)
                    : Rectangle<float> (r.getX(), r.getBottom() - (i + 1) * cell, r.getWidth(), cell).reduced (r.getWidth() * 0.18f, cell * 0.16f);
                g.setColour (i < lit ? acc : SYPal.knobTrack.contrasting (0.08f));
                g.fillRoundedRectangle (seg, 1.5f);
            }
            g.setColour (SYPal.panelBorder);
            g.drawRect (r, SyMetrics::stroke);
            return;
        }
        if (SYPal.sliderStyle == "rail")
        {
            const float cy = r.getCentreY();
            const float railH = jmin (5.0f, r.getHeight() * 0.35f);
            g.setColour (SYPal.knobTrack);
            g.fillRoundedRectangle (r.getX(), cy - railH * 0.5f, r.getWidth(), railH, railH * 0.5f);
            g.setColour (acc);
            g.fillRoundedRectangle (jmin (ox, px), cy - railH * 0.5f, std::abs (px - ox), railH, railH * 0.5f);
            const float tr = jmax (5.0f, r.getHeight() * 0.30f);
            drawSliderThumb (g, Rectangle<float> (px - tr, cy - tr, tr * 2.0f, tr * 2.0f));
            return;
        }
        if (SYPal.sliderStyle == "groove")
        {
            const float cr = jmin (r.getHeight(), r.getWidth()) * 0.5f * 0.7f;
            g.setColour (SYPal.background.contrasting (SYPal.dark ? 0.0f : 0.06f).withAlpha (SYPal.dark ? 0.55f : 0.25f));
            g.fillRoundedRectangle (r, cr);                                   // canal creusé
            g.setColour (acc);
            g.fillRoundedRectangle (Rectangle<float> (jmin (ox, px), r.getY(), std::abs (px - ox), r.getHeight()).reduced (1.5f), cr);
            g.setColour (SYPal.panelBorder);
            g.drawRoundedRectangle (r.reduced (0.5f), cr, SyMetrics::stroke);
            return;
        }
        // "bar" (défaut) : rectangle plat rempli
        g.setColour (SYPal.knobTrack);
        g.fillRect (r);
        g.setColour (acc);
        if (horiz)
            g.fillRect (Rectangle<float> (jmin (ox, px), r.getY(), std::abs (px - ox), r.getHeight()));
        else
            g.fillRect (r.withTop (r.getBottom() - pos01 * r.getHeight()));
        g.setColour (hot ? SYPal.accent : SYPal.panelBorder);
        g.drawRect (r, SyMetrics::stroke);
    }

    //========================================================= Recettes de potard
    // « dot » : corps plat + anneau de valeur fin + point indicateur (minimaliste).
    void drawKnobDot (Graphics& g, float cx, float cy, float radius,
                      float startA, float endA, float angle, Slider& s)
    {
        const float r = radius - 2.0f;
        if (r <= 1.0f) return;
        g.setColour (SYPal.knobBody);
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour (SYPal.panelBorder);
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);

        const float ringR = r - 3.0f;
        if (ringR > 1.0f && s.isEnabled())
        {
            Path val; val.addCentredArc (cx, cy, ringR, ringR, 0.0f, startA, angle, true);
            g.setColour (SYPal.accent);
            g.strokePath (val, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        const float dr   = jmax (1.0f, ringR - 1.0f);
        const float dotR = jmax (2.0f, r * 0.14f);
        const float dx = cx + std::sin (angle) * dr;
        const float dy = cy - std::cos (angle) * dr;
        g.setColour (s.isEnabled() ? SYPal.accent : SYPal.textMuted);
        g.fillEllipse (dx - dotR, dy - dotR, dotR * 2.0f, dotR * 2.0f);
    }

    // « vintage » : corps glossy/métal (dégradé + reflet) + aiguille accent épaisse.
    void drawKnobVintage (Graphics& g, float cx, float cy, float radius, float angle, Slider& s)
    {
        const float r = radius - 2.0f;
        if (r <= 1.0f) return;
        Rectangle<float> body (cx - r, cy - r, r * 2.0f, r * 2.0f);
        Path bp; bp.addEllipse (body);
        DropShadow (SYPal.shadow, SyMetrics::shadowRadius, { 0, 3 }).drawForPath (g, bp);
        g.setGradientFill (ColourGradient (SYPal.knobBody.brighter (SYPal.dark ? 0.55f : 0.20f), cx, body.getY(),
                                           SYPal.knobBody.darker   (SYPal.dark ? 0.40f : 0.12f), cx, body.getBottom(), false));
        g.fillEllipse (body);
        g.setColour (SYPal.panelBorder);
        g.drawEllipse (body, jmax (1.5f, r * 0.08f));
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (SYPal.dark ? 0.22f : 0.30f), cx, body.getY(),
                                           Colours::transparentWhite, cx, cy, false));
        g.fillEllipse (body.reduced (r * 0.16f).removeFromTop (r));

        Path ptr;
        const float pw = jmax (2.5f, r * 0.15f);
        ptr.addRoundedRectangle (-pw * 0.5f, -r + 3.0f, pw, r * 0.62f, pw * 0.5f);
        ptr.applyTransform (AffineTransform::rotation (angle).translated (cx, cy));
        g.setColour (s.isEnabled() ? SYPal.accent : SYPal.textMuted);
        g.fillPath (ptr);
    }

    Typeface::Ptr interRegular, interSemiBold;
};

// Active le LookAndFeel moderne (unique pour les deux thèmes) puis applique les couleurs de la palette.
inline void selectSyLookAndFeel()
{
    static ModernLookAndFeel modernLF;
    LookAndFeel::setDefaultLookAndFeel (&modernLF);
    syncSyLookAndFeel();
}
