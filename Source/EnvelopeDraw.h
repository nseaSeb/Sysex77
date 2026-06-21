/*
  ==============================================================================

    EnvelopeDraw.h
    Helpers de dessin réutilisables pour les vues du SY77 :
      - SyDraw::drawEnvelope     : courbe d'enveloppe (EG) à partir des niveaux/rates
      - SyDraw::drawFilterResponse : réponse stylisée d'un filtre (Thru / LPF / HPF)

    Pensé pour être appelé depuis paint(). Aucune dépendance hors JUCE.

  ==============================================================================
*/

#pragma once

namespace SyDraw
{
    //==============================================================================
    /** Dessine une grille légère dans `area`. */
    inline void drawGrid (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour)
    {
        g.setColour (colour.withAlpha (0.10f));
        for (int i = 0; i <= 8; ++i)
        {
            auto x = area.getX() + (area.getWidth()  * i) / 8.0f;
            g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
        }
        for (int i = 0; i <= 4; ++i)
        {
            auto y = area.getY() + (area.getHeight() * i) / 4.0f;
            g.drawHorizontalLine ((int) y, area.getX(), area.getRight());
        }
    }

    //==============================================================================
    /** Dessine une enveloppe.

        @param levels       suite de niveaux (0..maxLevel), un par point
        @param segWeights   poids horizontal de chaque segment (taille = levels.size()-1).
                            Une valeur plus grande = segment plus large (temps plus long).
        @param maxLevel     valeur pleine échelle des niveaux (p.ex. 64)
    */
    inline void drawEnvelope (juce::Graphics& g, juce::Rectangle<float> area,
                              const juce::Array<float>& levels,
                              const juce::Array<float>& segWeights,
                              float maxLevel, juce::Colour colour,
                              const juce::String& caption = {})
    {
        drawGrid (g, area, colour);

        if (levels.size() < 2 || segWeights.size() != levels.size() - 1 || maxLevel <= 0.0f)
            return;

        // largeur totale (chaque segment a une largeur minimale pour rester visible)
        float total = 0.0f;
        for (auto w : segWeights)
            total += juce::jmax (1.0f, w);

        auto yFor = [&] (float level)
        {
            auto clamped = juce::jlimit (0.0f, maxLevel, level);
            return area.getBottom() - (clamped / maxLevel) * area.getHeight();
        };

        juce::Path curve;
        juce::Array<juce::Point<float>> nodes;

        float x = area.getX();
        curve.startNewSubPath (x, yFor (levels[0]));
        nodes.add ({ x, yFor (levels[0]) });

        for (int i = 0; i < segWeights.size(); ++i)
        {
            x += area.getWidth() * juce::jmax (1.0f, segWeights[i]) / total;
            auto pt = juce::Point<float> (x, yFor (levels[i + 1]));
            curve.lineTo (pt);
            nodes.add (pt);
        }

        // remplissage sous la courbe
        juce::Path fill = curve;
        fill.lineTo (x, area.getBottom());
        fill.lineTo (area.getX(), area.getBottom());
        fill.closeSubPath();

        g.setGradientFill ({ colour.withAlpha (0.28f), area.getCentreX(), area.getY(),
                             colour.withAlpha (0.03f), area.getCentreX(), area.getBottom(), false });
        g.fillPath (fill);

        g.setColour (colour);
        g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // points (noeuds)
        for (auto& n : nodes)
        {
            g.setColour (colour);
            g.fillEllipse (n.x - 2.5f, n.y - 2.5f, 5.0f, 5.0f);
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            g.drawEllipse (n.x - 2.5f, n.y - 2.5f, 5.0f, 5.0f, 1.0f);
        }

        if (caption.isNotEmpty())
        {
            g.setColour (colour.withAlpha (0.7f));
            g.setFont (juce::Font (juce::FontOptions (12.0f)));
            g.drawText (caption, area.reduced (4.0f).toNearestInt(),
                        juce::Justification::topLeft, false);
        }
    }

    //==============================================================================
    enum FilterMode { Thru = 0, LPF = 1, HPF = 2 };

    /** Réponse stylisée d'un filtre.
        @param cutoff01  position de coupure normalisée 0..1
        @param reso01    résonance normalisée 0..1
    */
    inline void drawFilterResponse (juce::Graphics& g, juce::Rectangle<float> area,
                                    int mode, float cutoff01, float reso01,
                                    juce::Colour colour, const juce::String& caption = {})
    {
        drawGrid (g, area, colour);

        auto fc   = juce::jlimit (0.02f, 0.98f, cutoff01);
        auto reso = juce::jlimit (0.0f, 1.0f, reso01);

        const int   numPoints = 96;
        const float passband  = 0.78f;            // niveau de bande passante (0..1)
        juce::Path curve;

        auto magFor = [&] (float f) -> float
        {
            float mag;

            if (mode == Thru)
            {
                mag = passband;
            }
            else
            {
                // pente de Butterworth stylisée (ordre ~2)
                float ratio = (mode == LPF) ? (f / fc) : (fc / juce::jmax (0.001f, f));
                mag = passband / std::sqrt (1.0f + std::pow (ratio, 4.0f));

                // bosse de résonance autour de la coupure
                float bump = reso * std::exp (-std::pow ((f - fc) / 0.045f, 2.0f));
                mag += bump * (1.0f - passband + 0.2f);
            }

            return juce::jlimit (0.0f, 1.0f, mag);
        };

        for (int i = 0; i <= numPoints; ++i)
        {
            float f = (float) i / (float) numPoints;
            float x = area.getX() + area.getWidth() * f;
            float y = area.getBottom() - magFor (f) * area.getHeight();

            if (i == 0) curve.startNewSubPath (x, y);
            else        curve.lineTo (x, y);
        }

        // remplissage
        juce::Path fill = curve;
        fill.lineTo (area.getRight(), area.getBottom());
        fill.lineTo (area.getX(),     area.getBottom());
        fill.closeSubPath();

        g.setGradientFill ({ colour.withAlpha (0.25f), area.getCentreX(), area.getY(),
                             colour.withAlpha (0.03f), area.getCentreX(), area.getBottom(), false });
        g.fillPath (fill);

        g.setColour (colour);
        g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // repère vertical de la coupure
        g.setColour (colour.withAlpha (0.35f));
        auto cx = area.getX() + area.getWidth() * fc;
        g.drawVerticalLine ((int) cx, area.getY(), area.getBottom());

        if (caption.isNotEmpty())
        {
            g.setColour (colour.withAlpha (0.7f));
            g.setFont (juce::Font (juce::FontOptions (12.0f)));
            g.drawText (caption, area.reduced (4.0f).toNearestInt(),
                        juce::Justification::topLeft, false);
        }
    }
}
