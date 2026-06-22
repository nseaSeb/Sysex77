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

// SyDraw : helpers de dessin partagés (enveloppes, réponse de filtre, panneaux).
namespace SyDraw
{
    //==============================================================================
    /** Fond « écran » commun (panneau arrondi sombre + liseré coloré). */
    inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour accent)
    {
        g.setColour (SYColBackground.contrasting (0.10f));
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (accent.withAlpha (0.40f));
        g.drawRoundedRectangle (area, 6.0f, 1.2f);
    }

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
                              const juce::String& caption = {},
                              bool showValues = true)
    {
        drawPanel (g, area, colour);
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

        // points (noeuds) + valeurs
        g.setFont (juce::Font (juce::FontOptions (10.0f)));
        for (int i = 0; i < nodes.size(); ++i)
        {
            auto n = nodes[i];
            g.setColour (colour);
            g.fillEllipse (n.x - 2.5f, n.y - 2.5f, 5.0f, 5.0f);
            g.setColour (SYColBackground.contrasting (0.5f).withAlpha (0.7f));
            g.drawEllipse (n.x - 2.5f, n.y - 2.5f, 5.0f, 5.0f, 1.0f);

            if (showValues)
            {
                g.setColour (SYColBackground.contrasting (0.75f));
                // au-dessus du noeud, sauf s'il touche le haut -> en dessous
                float ty = (n.y - 16.0f < area.getY()) ? n.y + 4.0f : n.y - 15.0f;
                g.drawText (juce::String (juce::roundToInt (levels[i])),
                            juce::Rectangle<float> (n.x - 14.0f, ty, 28.0f, 12.0f).toNearestInt(),
                            juce::Justification::centred, false);
            }
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
        drawPanel (g, area, colour);
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

    //==============================================================================
    /** Forme d'onde FM *approximée* (modèle 2 opérateurs : porteuse modulée en phase
        par un modulateur) : out(t) = sin(t + I·sin(R·t)). Ce n'est PAS le rendu exact
        du SY77 (Advanced FM, 6 op, 45 algos, feedback, EG…), mais un signal FM réel,
        dont la richesse varie avec l'algorithme. À affiner si on câble les vrais
        paramètres d'opérateurs + les matrices d'algorithme. */
    inline void drawFmWave (juce::Graphics& g, juce::Rectangle<float> area,
                            int algo, juce::Colour colour)
    {
        drawPanel (g, area, colour);

        const float ratio = 1.0f + (float) (juce::jmax (1, algo) % 4);        // 1..4
        const float index = 0.8f + (float) (juce::jmax (1, algo) % 6) * 0.7f; // profondeur de modulation

        const int   n = juce::jmax (24, (int) area.getWidth());
        juce::Path  p;
        for (int i = 0; i <= n; ++i)
        {
            const float ph = (float) i / (float) n;                      // un cycle
            const float t  = ph * juce::MathConstants<float>::twoPi;
            const float y  = std::sin (t + index * std::sin (ratio * t)); // -1..1
            const float x  = area.getX() + ph * area.getWidth();
            const float py = area.getCentreY() - y * area.getHeight() * 0.40f;
            if (i == 0) p.startNewSubPath (x, py);
            else        p.lineTo (x, py);
        }
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved));
    }

    //==============================================================================
    // Moteur FM (approché) — Étape 1 : utilise les vraies formes d'onde des opérateurs.
    // Les 16 formes d'onde AFM sont approximées en maths (assez pour un rendu indicatif ;
    // le SY77 a son propre jeu exact). idx 0..15, phase en radians.
    inline float afmWaveform (int idx, float p)
    {
        const float tp = juce::MathConstants<float>::twoPi;
        const float pi = juce::MathConstants<float>::pi;
        p = std::fmod (p, tp); if (p < 0) p += tp;
        const float s = std::sin (p);
        switch (((idx % 16) + 16) % 16)
        {
            case 0:  return s;                                              // sine
            case 1:  return s > 0 ? s : 0.0f;                               // half sine
            case 2:  return std::abs (s);                                   // rectified sine
            case 3:  return p < pi ? std::sin (2.0f * p) : 0.0f;            // quarter sine
            case 4:  return s > 0 ? std::sin (2.0f * p) : 0.0f;             // even sine
            case 5:  return s > 0 ? std::abs (std::sin (2.0f * p)) : 0.0f;  // even rectified
            case 6:  return s * std::abs (s);                              // sine^2 signé
            case 7:  return std::sin (p + 0.5f * std::sin (p));            // sine "feedback"
            case 8:  return s >= 0 ? 1.0f : -1.0f;                          // square
            case 9:  return 2.0f * (p / tp) - 1.0f;                         // saw montant
            case 10: return 1.0f - 2.0f * (p / tp);                        // saw descendant
            case 11: return p < pi ? (2.0f * p / pi - 1.0f) : (3.0f - 2.0f * p / pi); // triangle
            case 12: return std::sin (p) * std::cos (2.0f * p);            // produit
            case 13: return std::sin (3.0f * p);                           // 3e harmonique
            case 14: return 0.5f * (std::sin (p) + std::sin (2.0f * p));   // sine + 2e
            case 15: return std::tanh (3.0f * s);                          // sine saturé
            default: return s;
        }
    }

    /** Sortie FM d'un élément (chaîne sérielle OP6->OP1) à partir des index de waveform
        des 6 opérateurs. Approximation Étape 1 : ratios=1, index de modulation fixe.
        (Étapes suivantes : ratio/niveau réels par op + matrices d'algo exactes.) */
    inline float fmEval (const int waves[6], float phase, float index)
    {
        float mod = 0.0f;
        for (int op = 5; op >= 0; --op)            // OP6 (waves[5]) ... OP1 (waves[0])
            mod = afmWaveform (waves[op], phase + index * mod);
        return mod;                                // sortie = OP1
    }
}
