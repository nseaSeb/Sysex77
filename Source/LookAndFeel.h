/*
  ==============================================================================

    LookAndFeel.h
    Created: 18 Nov 2018 8:34:30pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "SyTheme.h"   // palette, metriques, couleurs par role


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

    // Un contrôle est « bipolaire » (remplissage depuis le centre / 12 h, point neutre) si
    // l'appli l'a marqué explicitement (propriété "bipolar" — pour les plages 0..N centrées,
    // ex. pan EG 0..63 centre 32) OU, à défaut, si sa plage est symétrique autour de 0
    // (detune ±, slope ±). Source unique honorée par TOUS les rendus (arc, helpers, barres).
    static bool isBipolar (Slider& s)
    {
        if (s.getProperties().contains ("bipolar"))
            return (bool) s.getProperties()["bipolar"];
        return (s.getMinimum() < -0.01 && s.getMaximum() > 0.01);
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
        if (SYPal.knobStyle == "tick")    { drawKnobTick    (g, cx, cy, radius, startA, endA, angle, s); return; }
        if (SYPal.knobStyle == "neon")    { drawKnobNeon    (g, cx, cy, radius, startA, endA, angle, s); return; }
        if (SYPal.knobStyle == "arctick") { drawKnobArcTick (g, cx, cy, radius, startA, endA, angle, s); return; }
        if (SYPal.knobStyle == "thin")    { drawKnobThin    (g, cx, cy, radius, startA, endA, angle, s); return; }

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
        const bool  bipolar = isBipolar (s);
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
        const bool bipolar = isBipolar (s);

        if (style == Slider::LinearVertical || style == Slider::LinearHorizontal)
        {
            // Les sliders linéaires suivent aussi le sliderStyle du thème (bar/led/rail/groove).
            const bool horiz = (style == Slider::LinearHorizontal);
            const float pos01 = (s.getMaximum() > s.getMinimum())
                ? (float) ((s.getValue() - s.getMinimum()) / (s.getMaximum() - s.getMinimum())) : 0.0f;
            drawValueBar (g, Rectangle<float> ((float) x, (float) y, (float) w, (float) h),
                          pos01, horiz, bipolar, s.isEnabled(), s.isMouseOverOrDragging());
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
        const bool enabled = b.isEnabled();
        // Desactive (tous styles) : jamais d'etat ON/survol affiche, fond attenue (cf. buttonFill)
        // -> un bouton grise ne doit pas sembler cliquable (setEnabled ne touche pas l'alpha).
        isOver = isOver && enabled;  isDown = isDown && enabled;
        auto bg  = buttonFill (b, isOver, isDown);
        const bool on = b.getToggleState() && enabled;
        const auto& style = SYPal.buttonStyle;
        const Colour onC = onColour (b);   // accent, ou couleur d'operateur (propriete syOpIndex)

        if (style == "flat")
        {
            if (! enabled)
            {
                g.setColour (bg);
                g.fillRoundedRectangle (r, 4.0f);
                g.setColour (SYPal.panelBorder.withAlpha (0.5f));
                g.drawRoundedRectangle (r, 4.0f, SyMetrics::stroke);
            }
            else if (on)
            {
                g.setColour (onC.withAlpha (0.18f));
                g.fillRoundedRectangle (r, 4.0f);
                g.setColour (onC);
                g.fillRect (r.getX() + 3.0f, r.getBottom() - 2.5f, r.getWidth() - 6.0f, 2.5f);
            }
            else
            {
                g.setColour (bg);   // fond plein (etait 45 % : libelles OFF trop pales)
                g.fillRoundedRectangle (r, 4.0f);
            }
        }
        else if (style == "round")
        {
            const float cr = r.getHeight() * 0.5f;
            g.setColour (bg);
            g.fillRoundedRectangle (r, cr);
            g.setColour (on ? onC : SYPal.panelBorder.withAlpha (0.6f));
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
            g.setColour (on ? onC : SYPal.panelBorder);
            g.drawRoundedRectangle (r, cr, on ? 2.0f : SyMetrics::stroke);
        }
        else if (style == "outline")
        {
            // « ghost » : fond transparent, contour accent ; rempli léger quand ON/survol.
            const float cr = 5.0f;
            if (on)           { g.setColour (onC.withAlpha (0.16f)); g.fillRoundedRectangle (r, cr); }
            else if (isOver)  { g.setColour (onC.withAlpha (0.07f)); g.fillRoundedRectangle (r, cr); }
            g.setColour (on ? onC : SYPal.panelBorder.withAlpha (0.8f));
            g.drawRoundedRectangle (r, cr, on ? 1.6f : SyMetrics::stroke);
        }
        else   // "square" (défaut + legacy Dark Orange)
        {
            const float cr = 2.0f;
            g.setGradientFill (ColourGradient (bg.brighter (SYPal.dark ? 0.06f : 0.02f), r.getX(), r.getY(),
                                               bg.darker   (SYPal.dark ? 0.10f : 0.04f), r.getX(), r.getBottom(), false));
            g.fillRoundedRectangle (r, cr);
            g.setColour (on ? onC : SYPal.panelBorder);
            g.drawRoundedRectangle (r, cr, SyMetrics::stroke);
        }
    }

    void drawButtonText (Graphics& g, TextButton& b, bool isOver, bool isDown) override
    {
        const bool enabled = b.isEnabled();
        isOver = isOver && enabled;  isDown = isDown && enabled;
        auto bg  = buttonFill (b, isOver, isDown);
        const bool ghost = (SYPal.buttonStyle == "flat" || SYPal.buttonStyle == "outline");
        const bool flat  = (SYPal.buttonStyle == "flat");
        Colour ink;
        if (! enabled)                   // desactive (tous styles) : texte attenue, lisible mais inerte
            ink = SYPal.textMuted.withAlpha (0.45f);
        else if (flat && b.getToggleState())
        {
            // Fond reel du bouton ON : teinte 18 % de la couleur ON posee sur la surface.
            const Colour eff = SYPal.surface.overlaidWith (onColour (b).withAlpha (0.18f));
            auto readable = [&] (Colour c) { return std::abs (c.getPerceivedBrightness()
                                                              - eff.getPerceivedBrightness()) >= 0.4f; };
            ink = b.getProperties().contains ("syOpIndex") ? onColour (b).brighter (0.25f)
                                                           : SYPal.knobFill;   // plus clair que l'accent
            if (! readable (ink))   // meme garde-fou que les autres styles : repli accent puis texte
                ink = readable (SYPal.accent) ? SYPal.accent : SYPal.textPrimary;
        }
        else if (ghost)                  // flat/outline : fond (semi-)transparent -> encre choisie à la main.
            ink = b.getToggleState() ? onColour (b) : SYPal.textPrimary;
        else if (b.getToggleState())     // ON : texte sur le fond ACCENT -> contraste garanti.
            ink = bg.contrasting();
        else
        {
            // OFF : préférer la couleur de TEXTE du thème (cohérence avec labels/combos) ; ne basculer
            // sur le contraste garanti (noir/blanc) que si elle serait illisible sur le fond du bouton.
            ink = SYPal.textPrimary;
            if (std::abs (ink.getPerceivedBrightness() - bg.getPerceivedBrightness()) < 0.4f)
                ink = bg.contrasting();
        }
        g.setColour (ink);
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
        const bool studio = syPanelStudio();

        if (syPanelFlat())
        {
            // Pas de contour de pourtour — séparateur horizontal sous le titre uniquement
            if (text.isNotEmpty())
            {
                const float inset = studio ? 2.0f : 6.0f;
                g.setColour (SYPal.panelBorder.withAlpha (0.6f));
                g.drawLine (r.getX() + inset, r.getY() + (float) SyMetrics::panelHeaderH,
                            r.getRight() - inset, r.getY() + (float) SyMetrics::panelHeaderH, 1.0f);
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
            if (studio)
            {
                // Titre de section en capitales espacees (transformees au dessin : les
                // traductions restent intactes), gris clair, a gauche.
                g.setColour (SYPal.textPrimary.withAlpha (0.78f));
                g.setFont (Font (FontOptions (12.0f)).boldened().withExtraKerningFactor (0.08f));
                g.drawText (text.toUpperCase(), 8, 2, w - 16, SyMetrics::panelHeaderH, Justification::left, true);
            }
            else
            {
                g.setColour (style == "flat" ? SYPal.textMuted : SYPal.textPrimary);
                g.setFont (Font (FontOptions (13.0f)).boldened());
                g.drawText (text, 12, 3, w - 24, SyMetrics::panelHeaderH, Justification::left, true);
            }
        }
    }

    //========================================================= Onglets
    void drawTabButton (TabBarButton& button, Graphics& g, bool isOver, bool) override
    {
        auto area = button.getActiveArea().toFloat();
        const bool front = button.isFrontTab();
        g.setColour (front ? SYPal.surface : (isOver ? SYPal.surfaceAlt : SYPal.background));
        g.fillRect (area);
        // Liseré accent CÔTÉ CONTENU : en bas pour une barre en haut, en haut pour une barre en bas.
        const bool barAtTop = button.getTabbedButtonBar().getOrientation() == TabbedButtonBar::TabsAtTop;
        auto stripe = [&] (float th)
        {
            g.fillRect (area.getX(), barAtTop ? area.getBottom() - th : area.getY(), area.getWidth(), th);
        };
        if (front)
        {
            g.setColour (SYPal.accent);
            stripe (2.5f);
        }
        if (syPanelStudio())
        {
            // Style "studio" : onglet actif teinte accent + soulignement, libelles en capitales.
            if (front)
            {
                g.setColour (SYPal.accent.withAlpha (0.16f));
                g.fillRect (area);
                g.setColour (SYPal.accent);
                stripe (2.0f);
            }
            // Survol : rapprochement vers le texte principal (marche en clair comme en sombre).
            g.setColour (front ? SYPal.textPrimary
                               : (isOver ? SYPal.textMuted.interpolatedWith (SYPal.textPrimary, 0.5f)
                                         : SYPal.textMuted));
        }
        else
            g.setColour (front ? SYPal.textPrimary : SYPal.textMuted);
        g.setFont (tabFont());
        g.drawFittedText (tabLabel (button.getButtonText()), area.reduced (8, 0).toNearestInt(),
                          Justification::centred, 1);
    }

    void drawTabAreaBehindFrontButton (TabbedButtonBar&, Graphics&, int, int) override {}

    // Police + libelle d'onglet, partages par drawTabButton et getTabButtonBestWidth (la largeur
    // calculee doit correspondre exactement au texte dessine).
    static Font tabFont()
    {
        return syPanelStudio() ? Font (FontOptions (12.5f)).boldened().withExtraKerningFactor (0.06f)
                               : Font (FontOptions (14.0f));
    }
    static String tabLabel (const String& name)
    {
        return syPanelStudio() ? name.trim().toUpperCase() : name.trim();
    }

    // Barre d'onglets PRINCIPALE (propriete "syEqualTabs") : onglets de largeur EGALE, calee sur
    // le libelle le plus long (meme police que drawTabButton), etiree jusqu'a 180 px si la place
    // le permet. Si la barre est trop etroite, TabbedButtonBar retrecit (echelle) les onglets.
    // Les autres barres (onglets imbriques) gardent le calcul JUCE standard.
    int getTabButtonBestWidth (TabBarButton& button, int tabDepth) override
    {
        auto& bar = button.getTabbedButtonBar();
        if (! (bool) bar.getProperties().getWithDefault ("syEqualTabs", false))
            return LookAndFeel_V4::getTabButtonBestWidth (button, tabDepth);

        const Font f = tabFont();
        float widest = 0.0f;
        for (auto& name : bar.getTabNames())
            widest = jmax (widest, GlyphArrangement::getStringWidth (f, tabLabel (name)));
        const int textW = roundToInt (widest) + 32;   // 2 x 8 px de marge (drawTabButton) + respiration
        const int n     = jmax (1, bar.getNumTabs());
        const int fill  = jmin (180, bar.getWidth() / n);
        return jmax (textW, fill);
    }

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
    // Couleur de l'etat ON d'un bouton : accent, sauf bouton d'operateur FM marque par la
    // propriete "syOpIndex" (0..5) -> couleur de cet operateur (= accent si le theme n'en a pas).
    static Colour onColour (const Button& b)
    {
        const auto& pr = b.getProperties();
        return pr.contains ("syOpIndex") ? syOpColour ((int) pr["syOpIndex"]) : SYPal.accent;
    }

    // Fond effectif d'un bouton selon son état (toggle ON = accent ; sinon couleur de base + survol/appui).
    Colour buttonFill (Button& b, bool isOver, bool isDown) const
    {
        auto base = b.findColour (TextButton::buttonColourId);
        if (! b.isEnabled())     return base.withMultipliedAlpha (0.5f);   // desactive : fond attenue
        if (b.getToggleState())  return onColour (b);
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

        // Position (pixel) de la valeur le long de l'axe + origine du remplissage
        // (centre si bipolaire). Vertical : valeur haute = vers le haut.
        const float vpos = horiz ? (r.getX() + pos01 * r.getWidth())
                                 : (r.getBottom() - pos01 * r.getHeight());
        const float vorg = bipolar ? (horiz ? r.getCentreX() : r.getCentreY())
                                   : (horiz ? r.getX() : r.getBottom());
        auto fillRect = [&]
        {
            return horiz ? Rectangle<float> (jmin (vorg, vpos), r.getY(), std::abs (vpos - vorg), r.getHeight())
                         : Rectangle<float> (r.getX(), jmin (vorg, vpos), r.getWidth(), std::abs (vpos - vorg));
        };

        if (SYPal.sliderStyle == "field")
        {
            // Champ plat (style "studio") : bloc surfaceAlt sans bordure, remplissage accent
            // discret proportionnel a la valeur (depuis le centre si bipolaire). Reste une BARRE :
            // les lignes se comparent d'un coup d'oeil, la valeur ecrite reste lisible dessus.
            const float cr = jmin (3.0f, jmin (r.getWidth(), r.getHeight()) * 0.25f);
            g.setColour (SYPal.surfaceAlt.brighter (hot && enabled ? 0.06f : 0.0f));
            g.fillRoundedRectangle (r, cr);
            const float a = enabled ? (hot ? 0.50f : 0.35f) : 0.15f;
            auto fr = fillRect();
            if (! fr.isEmpty())
            {
                g.setColour (SYPal.accent.withAlpha (a));
                g.fillRoundedRectangle (fr, cr);
                // Repere fin a la position de la valeur (lecture precise sans bordure).
                g.setColour (SYPal.accent.withAlpha (enabled ? 0.9f : 0.3f));
                if (horiz) g.fillRect (jlimit (r.getX(), r.getRight() - 2.0f, vpos - 1.0f), r.getY(), 2.0f, r.getHeight());
                else       g.fillRect (r.getX(), jlimit (r.getY(), r.getBottom() - 2.0f, vpos - 1.0f), r.getWidth(), 2.0f);
            }
            if (bipolar)   // trait de zero discret
            {
                g.setColour (SYPal.textMuted.withAlpha (0.35f));
                if (horiz) g.fillRect (r.getCentreX() - 0.5f, r.getY(), 1.0f, r.getHeight());
                else       g.fillRect (r.getX(), r.getCentreY() - 0.5f, r.getWidth(), 1.0f);
            }
            return;
        }
        if (SYPal.sliderStyle == "led")
        {
            g.setColour (SYPal.knobTrack);
            g.fillRect (r);
            const float lenAxis = horiz ? r.getWidth() : r.getHeight();
            const int n = jmax (6, (int) (lenAxis / 9.0f));
            const int lit = (int) std::round (pos01 * (float) n);
            // Bipolaire (pan) : segments allumés ENTRE le centre et la valeur (pas depuis le bord).
            const int mid = bipolar ? (int) std::round (0.5f * (float) n) : 0;
            const int loC = jmin (mid, lit), hiC = jmax (mid, lit);
            const float cell = lenAxis / (float) n;
            for (int i = 0; i < n; ++i)
            {
                auto seg = horiz
                    ? Rectangle<float> (r.getX() + i * cell, r.getY(), cell, r.getHeight()).reduced (cell * 0.16f, r.getHeight() * 0.18f)
                    : Rectangle<float> (r.getX(), r.getBottom() - (i + 1) * cell, r.getWidth(), cell).reduced (r.getWidth() * 0.18f, cell * 0.16f);
                const bool on = bipolar ? (i >= loC && i < hiC) : (i < lit);
                g.setColour (on ? acc : SYPal.knobTrack.contrasting (0.08f));
                g.fillRoundedRectangle (seg, 1.5f);
            }
            g.setColour (SYPal.panelBorder);
            g.drawRect (r, SyMetrics::stroke);
            return;
        }
        if (SYPal.sliderStyle == "rail")
        {
            if (horiz)
            {
                const float cy = r.getCentreY(), railH = jmin (5.0f, r.getHeight() * 0.35f);
                g.setColour (SYPal.knobTrack);
                g.fillRoundedRectangle (r.getX(), cy - railH * 0.5f, r.getWidth(), railH, railH * 0.5f);
                g.setColour (acc);
                g.fillRoundedRectangle (jmin (vorg, vpos), cy - railH * 0.5f, std::abs (vpos - vorg), railH, railH * 0.5f);
                const float tr = jmax (5.0f, r.getHeight() * 0.30f);
                drawSliderThumb (g, Rectangle<float> (vpos - tr, cy - tr, tr * 2.0f, tr * 2.0f));
            }
            else
            {
                const float cx = r.getCentreX(), railW = jmin (5.0f, r.getWidth() * 0.35f);
                g.setColour (SYPal.knobTrack);
                g.fillRoundedRectangle (cx - railW * 0.5f, r.getY(), railW, r.getHeight(), railW * 0.5f);
                g.setColour (acc);
                g.fillRoundedRectangle (cx - railW * 0.5f, jmin (vorg, vpos), railW, std::abs (vpos - vorg), railW * 0.5f);
                const float tr = jmax (5.0f, r.getWidth() * 0.30f);
                drawSliderThumb (g, Rectangle<float> (cx - tr, vpos - tr, tr * 2.0f, tr * 2.0f));
            }
            return;
        }
        if (SYPal.sliderStyle == "groove")
        {
            const float cr = jmin (r.getHeight(), r.getWidth()) * 0.35f;
            g.setColour (SYPal.background.contrasting (SYPal.dark ? 0.0f : 0.06f).withAlpha (SYPal.dark ? 0.55f : 0.25f));
            g.fillRoundedRectangle (r, cr);                                   // canal creusé
            g.setColour (acc);
            g.fillRoundedRectangle (fillRect().reduced (1.5f), cr);
            g.setColour (SYPal.panelBorder);
            g.drawRoundedRectangle (r.reduced (0.5f), cr, SyMetrics::stroke);
            return;
        }
        if (SYPal.sliderStyle == "notch")
        {
            g.setColour (SYPal.knobTrack);
            g.fillRect (r);
            g.setColour (acc);
            g.fillRect (fillRect());
            g.setColour (SYPal.background.withAlpha (0.45f));                 // crans (règle dentée)
            const float lenAxis = horiz ? r.getWidth() : r.getHeight();
            const int n = jmax (4, (int) (lenAxis / 14.0f));
            for (int i = 1; i < n; ++i)
            {
                const float t = (float) i / (float) n;
                if (horiz) { const float xx = r.getX() + t * r.getWidth();
                             g.drawLine (xx, r.getY() + r.getHeight() * 0.25f, xx, r.getBottom() - r.getHeight() * 0.25f, 1.0f); }
                else       { const float yy = r.getY() + t * r.getHeight();
                             g.drawLine (r.getX() + r.getWidth() * 0.25f, yy, r.getRight() - r.getWidth() * 0.25f, yy, 1.0f); }
            }
            g.setColour (hot ? SYPal.accent : SYPal.panelBorder);
            g.drawRect (r, SyMetrics::stroke);
            return;
        }
        if (SYPal.sliderStyle == "dots")
        {
            g.setColour (SYPal.knobTrack);
            g.fillRect (r);
            const float lenAxis   = horiz ? r.getWidth()  : r.getHeight();
            const float thickAxis = horiz ? r.getHeight() : r.getWidth();
            const float d = jmin (thickAxis * 0.52f, 10.0f);
            const int n = jmax (4, (int) (lenAxis / (d + 4.0f)));
            const int lit = (int) std::round (pos01 * (float) n);
            // Bipolaire (pan) : pastilles allumées ENTRE le centre et la valeur (pas depuis le bord).
            const int mid = bipolar ? (int) std::round (0.5f * (float) n) : 0;
            const int loC = jmin (mid, lit), hiC = jmax (mid, lit);
            const float gap = lenAxis / (float) n;
            for (int i = 0; i < n; ++i)
            {
                const float c  = (i + 0.5f) * gap;
                const float dx = horiz ? r.getX() + c : r.getCentreX();
                const float dy = horiz ? r.getCentreY() : r.getBottom() - c;
                const bool on = bipolar ? (i >= loC && i < hiC) : (i < lit);
                g.setColour (on ? acc : SYPal.knobTrack.contrasting (0.10f));
                g.fillEllipse (dx - d * 0.5f, dy - d * 0.5f, d, d);
            }
            g.setColour (SYPal.panelBorder);
            g.drawRect (r, SyMetrics::stroke);
            return;
        }
        // "bar" (défaut) : rectangle plat rempli
        g.setColour (SYPal.knobTrack);
        g.fillRect (r);
        g.setColour (acc);
        g.fillRect (fillRect());
        g.setColour (hot ? SYPal.accent : SYPal.panelBorder);
        g.drawRect (r, SyMetrics::stroke);
    }

    //========================================================= Recettes de potard
    // « dot » : corps plat + anneau de valeur fin + point indicateur (minimaliste).
    // "thin" : arc de valeur fin sur piste discrete, corps mat sans repere ; la valeur est
    // ecrite au centre seulement si le slider n'a pas deja sa propre text-box (pas de doublon).
    void drawKnobThin (Graphics& g, float cx, float cy, float radius,
                       float startA, float endA, float angle, Slider& s)
    {
        const float thick = jlimit (2.0f, 3.5f, radius * 0.10f);
        const float ringR = radius - thick * 0.5f - 1.0f;
        if (ringR <= 2.0f) return;

        Path track;
        track.addCentredArc (cx, cy, ringR, ringR, 0.0f, startA, endA, true);
        g.setColour (SYPal.knobTrack);
        g.strokePath (track, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));

        if (s.isEnabled())
        {
            const float fillA = isBipolar (s) ? (startA + endA) * 0.5f : startA;
            Path val;
            val.addCentredArc (cx, cy, ringR, ringR, 0.0f, jmin (fillA, angle), jmax (fillA, angle), true);
            if (s.isMouseOverOrDragging())
            {
                g.setColour (SYPal.glow);
                g.strokePath (val, PathStrokeType (thick + 3.0f, PathStrokeType::curved, PathStrokeType::rounded));
            }
            g.setColour (SYPal.knobFill);
            g.strokePath (val, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
        }

        const float bodyR = ringR - thick * 0.5f - jmax (2.0f, radius * 0.10f);
        if (bodyR <= 1.0f) return;
        Rectangle<float> body (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        Path bodyPath; bodyPath.addEllipse (body);
        DropShadow (SYPal.shadow, 6, { 0, 2 }).drawForPath (g, bodyPath);
        g.setColour (SYPal.knobBody);
        g.fillEllipse (body);
        g.setColour (SYPal.textPrimary.withAlpha (0.06f));
        g.drawEllipse (body.reduced (0.5f), 1.0f);

        if (s.getTextBoxPosition() == Slider::NoTextBox && bodyR >= 11.0f)
        {
            g.setColour (s.isEnabled() ? SYPal.textPrimary : SYPal.textMuted);
            g.setFont (Font (FontOptions (jlimit (10.0f, 15.0f, bodyR * 0.62f))).boldened());
            g.drawFittedText (s.getTextFromValue (s.getValue()), body.reduced (2.0f).toNearestInt(),
                              Justification::centred, 1, 0.8f);
        }
    }

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
            // Bipolaire (pan) : l'arc de valeur part du centre (12 h) vers la valeur.
            const float fillA = isBipolar (s) ? (startA + endA) * 0.5f : startA;
            Path val; val.addCentredArc (cx, cy, ringR, ringR, 0.0f, jmin (fillA, angle), jmax (fillA, angle), true);
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

    // « tick » : couronne de graduations (allumées jusqu'à la valeur) + aiguille centrale.
    void drawKnobTick (Graphics& g, float cx, float cy, float radius,
                       float startA, float endA, float angle, Slider& s)
    {
        const float r = radius - 2.0f;
        if (r <= 1.0f) return;
        g.setColour (SYPal.knobBody);
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour (SYPal.panelBorder);
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);

        const int n = 11;
        const float ro = r - 1.5f, ri = r - jmax (4.0f, r * 0.24f);
        const float tw = jmax (1.5f, r * 0.07f);
        // Bipolaire (pan) : graduations allumées entre le centre (12 h) et la valeur.
        const float fillA = isBipolar (s) ? (startA + endA) * 0.5f : startA;
        const float litLo = jmin (fillA, angle), litHi = jmax (fillA, angle);
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) i / (float) (n - 1);
            const float a = startA + t * (endA - startA);
            const bool  lit = s.isEnabled() && a >= litLo - 0.001f && a <= litHi + 0.001f;
            g.setColour (lit ? SYPal.accent : SYPal.knobTrack);
            g.drawLine (cx + std::sin (a) * ri, cy - std::cos (a) * ri,
                        cx + std::sin (a) * ro, cy - std::cos (a) * ro, tw);
        }
        Path ptr;
        const float pw = jmax (2.0f, r * 0.11f);
        ptr.addRoundedRectangle (-pw * 0.5f, -ri + 2.0f, pw, ri * 0.9f, pw * 0.5f);
        ptr.applyTransform (AffineTransform::rotation (angle).translated (cx, cy));
        g.setColour (s.isEnabled() ? SYPal.accent : SYPal.textMuted);
        g.fillPath (ptr);
    }

    // « neon » : corps sombre + anneau d'accent lumineux (halo) + point brillant (synthwave).
    void drawKnobNeon (Graphics& g, float cx, float cy, float radius,
                       float startA, float endA, float angle, Slider& s)
    {
        const float r = radius - 2.0f;
        if (r <= 1.0f) return;
        g.setColour (SYPal.knobBody.darker (SYPal.dark ? 0.35f : 0.0f));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

        const float ringR = r - 3.0f;
        const float thick = jmax (2.0f, r * 0.13f);
        if (ringR > 1.0f)
        {
            Path track; track.addCentredArc (cx, cy, ringR, ringR, 0.0f, startA, endA, true);
            g.setColour (SYPal.knobTrack);
            g.strokePath (track, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
            if (s.isEnabled())
            {
                // Bipolaire (pan) : l'arc lumineux part du centre (12 h) vers la valeur.
                const float fillA = isBipolar (s) ? (startA + endA) * 0.5f : startA;
                Path val; val.addCentredArc (cx, cy, ringR, ringR, 0.0f, jmin (fillA, angle), jmax (fillA, angle), true);
                g.setColour (SYPal.accent.withAlpha (0.35f));
                g.strokePath (val, PathStrokeType (thick + 5.0f, PathStrokeType::curved, PathStrokeType::rounded));
                g.setColour (SYPal.accent);
                g.strokePath (val, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
                const float dx = cx + std::sin (angle) * ringR, dy = cy - std::cos (angle) * ringR;
                const float dr = jmax (2.0f, r * 0.14f);
                g.setColour (SYPal.accent.withAlpha (0.4f));
                g.fillEllipse (dx - dr * 1.7f, dy - dr * 1.7f, dr * 3.4f, dr * 3.4f);
                g.setColour (SYPal.accent);
                g.fillEllipse (dx - dr, dy - dr, dr * 2.0f, dr * 2.0f);
            }
        }
        g.setColour (SYPal.panelBorder);
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);
    }

    // « arctick » : encodeur façon Nord — couronne de LED logée dans la goulotte
    //               AUTOUR d'un cap central, allumées jusqu'à la valeur (avec halo).
    void drawKnobArcTick (Graphics& g, float cx, float cy, float radius,
                          float startA, float endA, float angle, Slider& s)
    {
        const float r = radius - 1.0f;
        const float chThick = jmax (4.0f, r * 0.26f);
        const float chMid   = r - chThick * 0.5f;          // rayon de la couronne de LED
        if (chMid <= 1.0f) return;

        // Goulotte sombre en anneau (le logement des LED).
        Path channel; channel.addCentredArc (cx, cy, chMid, chMid, 0.0f, startA, endA, true);
        g.setColour (SYPal.background.contrasting (SYPal.dark ? 0.0f : 0.05f).withAlpha (SYPal.dark ? 0.55f : 0.25f));
        g.strokePath (channel, PathStrokeType (chThick, PathStrokeType::curved, PathStrokeType::butt));

        // Couronne de LED, allumées jusqu'à la valeur (depuis le centre si bipolaire — pan).
        const int n = 21;
        const float dr = jmax (1.5f, chThick * 0.30f);
        const float fillA = isBipolar (s) ? (startA + endA) * 0.5f : startA;
        const float litLo = jmin (fillA, angle), litHi = jmax (fillA, angle);
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) i / (float) (n - 1);
            const float a = startA + t * (endA - startA);
            const bool  lit = s.isEnabled() && a >= litLo - 0.001f && a <= litHi + 0.001f;
            const float dx = cx + std::sin (a) * chMid, dy = cy - std::cos (a) * chMid;
            if (lit)
            {
                g.setColour (SYPal.accent.withAlpha (0.35f));
                g.fillEllipse (dx - dr * 1.8f, dy - dr * 1.8f, dr * 3.6f, dr * 3.6f);   // halo
                g.setColour (SYPal.accent);
            }
            else
                g.setColour (SYPal.knobTrack);
            g.fillEllipse (dx - dr, dy - dr, dr * 2.0f, dr * 2.0f);
        }

        // Cap central + repère.
        const float bodyR = chMid - chThick * 0.5f - 2.0f;
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
            ptr.addRoundedRectangle (-pw * 0.5f, -bodyR + 2.0f, pw, bodyR * 0.5f, pw * 0.5f);
            ptr.applyTransform (AffineTransform::rotation (angle).translated (cx, cy));
            g.setColour (SYPal.accent);
            g.fillPath (ptr);
        }
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
