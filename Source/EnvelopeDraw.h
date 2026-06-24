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
        // Ombre portée douce : détache l'écran du fond.
        {
            juce::Path sp; sp.addRoundedRectangle (area, 6.0f);
            juce::DropShadow (SYPal.shadow, 10, { 0, 3 }).drawForPath (g, sp);
        }
        // Remplissage : léger dégradé vertical (haut un peu plus clair).
        const auto base = SYColBackground.contrasting (0.10f);
        g.setGradientFill ({ base.brighter (SYPal.dark ? 0.05f : 0.02f), area.getCentreX(), area.getY(),
                             base.darker   (SYPal.dark ? 0.07f : 0.02f), area.getCentreX(), area.getBottom(), false });
        g.fillRoundedRectangle (area, 6.0f);
        // Liseré supérieur lumineux (effet « verre »).
        g.setColour (juce::Colours::white.withAlpha (SYPal.dark ? 0.05f : 0.30f));
        g.drawLine (area.getX() + 6.0f, area.getY() + 1.0f, area.getRight() - 6.0f, area.getY() + 1.0f, 1.0f);
        // Bord accent.
        g.setColour (accent.withAlpha (0.45f));
        g.drawRoundedRectangle (area, 6.0f, 1.2f);
    }

    /** Trace une courbe (forme d'onde) avec un halo doux pour un rendu « néon » subtil.
        @param fillToBottom  remplit sous la courbe (utile pour les enveloppes, pas les
                             signaux oscillant autour du centre). */
    inline void strokeWave (juce::Graphics& g, const juce::Path& p, juce::Rectangle<float> area,
                            juce::Colour colour, float thickness = 1.6f, bool fillToBottom = false)
    {
        if (fillToBottom)
        {
            juce::Path f = p;
            f.lineTo (area.getRight(), area.getBottom());
            f.lineTo (area.getX(),     area.getBottom());
            f.closeSubPath();
            g.setGradientFill ({ colour.withAlpha (0.18f), area.getCentreX(), area.getY(),
                                 colour.withAlpha (0.0f),  area.getCentreX(), area.getBottom(), false });
            g.fillPath (f);
        }
        // Halo (trait large translucide) puis trait net par-dessus.
        g.setColour (colour.withAlpha (0.22f));
        g.strokePath (p, juce::PathStrokeType (thickness + 2.5f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
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
        strokeWave (g, p, area, colour, 1.6f);
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
    // ratios[op] = multiplicateur de fréquence ; levels[op] = amplitude 0..1 de l'opérateur.
    inline float fmEval (const int waves[6], const float ratios[6], const float levels[6],
                         float phase, float index)
    {
        float mod = 0.0f;
        for (int op = 5; op >= 0; --op)            // OP6 (waves[5]) ... OP1 (waves[0])
        {
            const float out = afmWaveform (waves[op], ratios[op] * phase + index * mod);
            mod = out * levels[op];                // le niveau pondère la contribution de l'op
        }
        return mod;                                // sortie = OP1 (déjà pondérée par son niveau)
    }

    inline float fmEval (const int waves[6], const float ratios[6], float phase, float index)
    {
        const float l[6] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        return fmEval (waves, ratios, l, phase, index);
    }

    inline float fmEval (const int waves[6], float phase, float index)
    {
        const float r[6] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        return fmEval (waves, r, phase, index);
    }

    //==============================================================================
    // Topologie exacte des 45 algorithmes AFM du SY77.
    // Données extraites de Yamaha_DX7_to_SY77_Conversion.pdf (Annexe A, modèle FM).
    // Par algorithme (1..45) : in0/in1 = sources d'entrée des opérateurs, acc0/acc1 =
    // sélecteurs d'entrée de l'accumulateur, outd = destination de sortie (registre).
    // Opérateurs ordonnés OP1..OP6. Codes d'entrée : 0=off, 1=thru (op n+1), 2=AWM,
    // 3/4/5=registre 1/2/3, 6/7/8=feedback, 9=accumulateur, 10=bruit.
    struct AlgoDef { int in0[6], in1[6], acc0[6], acc1[6], outd[6]; };
    static const AlgoDef kAlgo[45] = {
      { {1,1,1,1,1,0}, {0,0,0,0,0,0}, {1,0,0,0,0,1}, {0,1,1,1,1,0}, {0,0,0,3,2,1} }, // 1
      { {1,1,1,1,0,0}, {0,0,0,9,0,0}, {1,0,0,1,0,1}, {0,1,1,0,1,0}, {0,0,0,3,2,1} }, // 2
      { {1,1,1,1,0,0}, {0,0,9,0,0,0}, {1,0,1,0,0,1}, {0,1,0,1,1,0}, {0,0,0,3,2,1} }, // 3
      { {1,1,1,1,0,0}, {0,9,0,0,0,0}, {1,0,0,0,0,1}, {0,1,1,1,1,0}, {0,0,0,3,2,1} }, // 4
      { {1,1,1,1,0,0}, {9,0,0,0,0,0}, {1,0,0,0,0,1}, {0,1,1,1,1,0}, {0,0,0,3,2,1} }, // 5
      { {1,1,1,0,0,0}, {0,0,9,0,0,0}, {1,0,1,0,1,1}, {0,1,0,1,1,0}, {0,0,0,3,2,1} }, // 6
      { {1,1,1,0,1,0}, {0,9,0,0,0,0}, {1,0,0,0,1,0}, {0,1,1,1,0,1}, {0,0,0,3,2,1} }, // 7
      { {1,1,1,0,0,0}, {0,9,0,0,0,0}, {1,0,0,0,1,1}, {0,1,1,1,1,0}, {0,0,0,3,2,1} }, // 8
      { {1,1,0,1,0,0}, {0,9,0,9,0,0}, {1,0,0,1,0,1}, {0,1,1,0,1,0}, {0,0,3,0,2,1} }, // 9
      { {1,1,1,0,1,0}, {9,0,0,0,0,0}, {1,0,0,0,1,0}, {0,1,1,1,0,1}, {0,0,0,3,2,1} }, // 10
      { {1,1,1,0,0,0}, {9,0,0,0,0,0}, {1,0,0,0,1,1}, {0,1,1,1,1,0}, {0,0,0,3,2,1} }, // 11
      { {1,0,1,1,0,0}, {9,0,0,9,0,0}, {1,0,1,0,0,1}, {0,1,0,1,1,0}, {0,3,0,0,2,1} }, // 12
      { {1,0,1,1,0,0}, {9,0,9,0,0,0}, {1,0,1,0,0,1}, {0,1,0,1,1,0}, {0,3,0,0,2,1} }, // 13
      { {1,1,0,0,0,0}, {0,9,0,0,0,0}, {1,0,0,1,1,1}, {0,1,1,1,1,0}, {0,0,3,0,2,1} }, // 14
      { {1,0,1,0,0,0}, {9,0,9,0,0,0}, {1,0,1,0,1,1}, {0,1,0,1,1,0}, {0,3,0,2,0,1} }, // 15
      { {1,1,0,1,0,0}, {9,0,0,0,0,0}, {1,0,0,1,0,1}, {0,1,1,1,1,0}, {0,0,3,0,2,1} }, // 16
      { {1,1,0,1,0,0}, {9,0,0,9,0,0}, {1,0,0,1,0,1}, {0,1,1,0,1,0}, {0,0,3,0,2,1} }, // 17
      { {1,1,0,0,0,0}, {9,0,0,0,0,0}, {1,0,0,1,1,1}, {0,1,1,1,1,0}, {0,0,0,3,2,1} }, // 18
      { {1,1,0,0,0,0}, {9,3,0,0,0,6}, {1,0,0,1,1,0}, {0,1,1,1,0,1}, {0,0,3,0,2,1} }, // 19
      { {1,0,0,0,0,0}, {9,0,0,0,0,0}, {1,0,1,1,1,1}, {0,1,1,1,1,0}, {0,3,0,0,2,1} }, // 20
      { {0,1,1,1,1,0}, {0,0,0,0,0,0}, {1,1,0,0,0,1}, {1,0,1,1,1,0}, {0,0,0,3,2,1} }, // 21
      { {0,1,1,1,0,0}, {0,0,0,9,0,0}, {1,1,0,0,0,1}, {1,0,1,1,1,0}, {0,0,3,0,2,1} }, // 22
      { {0,1,1,1,0,0}, {0,0,9,0,0,0}, {1,1,0,0,0,1}, {1,0,1,1,1,0}, {0,0,0,3,2,1} }, // 23
      { {0,1,1,1,0,0}, {0,9,0,0,0,0}, {1,1,0,0,0,1}, {1,0,1,1,1,0}, {0,0,0,3,2,1} }, // 24
      { {0,1,1,0,0,0}, {0,0,9,0,0,0}, {1,1,0,0,1,1}, {1,0,1,1,1,0}, {0,0,0,3,2,1} }, // 25
      { {0,1,1,0,1,0}, {0,9,0,0,0,0}, {1,1,0,0,1,0}, {1,0,1,1,0,1}, {0,0,0,3,2,1} }, // 26
      { {0,1,1,0,0,0}, {0,9,0,0,0,0}, {1,1,0,0,1,1}, {1,0,1,1,1,0}, {0,0,0,3,2,1} }, // 27
      { {0,1,0,1,0,0}, {0,9,0,9,0,0}, {1,1,0,1,0,1}, {1,0,1,0,1,0}, {0,0,3,0,2,1} }, // 28
      { {0,1,0,0,0,0}, {0,9,0,0,0,0}, {1,1,0,1,1,1}, {1,0,1,1,1,0}, {0,0,3,0,2,1} }, // 29
      { {1,0,1,1,1,0}, {0,0,0,0,0,0}, {1,0,1,0,0,1}, {1,1,0,1,1,0}, {0,3,0,0,2,1} }, // 30
      { {1,0,1,1,0,0}, {0,0,0,9,0,0}, {1,0,1,0,0,1}, {1,1,0,1,1,0}, {0,3,0,0,2,1} }, // 31
      { {1,0,1,1,0,0}, {0,0,9,0,0,0}, {1,0,1,0,0,1}, {1,1,0,1,1,0}, {0,3,0,0,2,1} }, // 32
      { {1,0,1,0,0,0}, {0,0,9,0,0,0}, {1,0,1,0,1,1}, {1,1,0,1,1,0}, {0,3,0,2,0,1} }, // 33
      { {1,1,0,1,1,0}, {0,0,0,0,0,0}, {1,0,0,1,0,1}, {1,1,1,0,1,0}, {0,0,3,0,2,1} }, // 34
      { {1,1,0,1,0,0}, {0,0,0,9,0,0}, {1,0,0,1,0,1}, {1,1,1,0,1,0}, {0,0,3,0,2,1} }, // 35
      { {1,0,6,1,0,0}, {3,0,0,9,0,0}, {1,0,0,1,0,1}, {1,1,1,0,1,0}, {0,0,1,0,3,2} }, // 36
      { {0,0,1,1,1,0}, {0,0,0,0,0,0}, {1,1,1,0,0,1}, {1,1,0,1,1,0}, {0,0,0,3,2,1} }, // 37
      { {0,0,1,1,0,0}, {0,0,0,9,0,0}, {1,1,1,0,0,1}, {1,1,0,1,1,0}, {0,0,0,3,2,1} }, // 38
      { {0,0,1,1,0,0}, {0,0,9,0,0,0}, {1,1,1,0,0,1}, {1,1,0,1,1,0}, {0,0,0,3,2,1} }, // 39
      { {3,1,0,1,1,0}, {0,0,6,0,0,0}, {1,1,0,1,0,1}, {1,1,1,0,1,0}, {0,0,1,0,3,2} }, // 40
      { {3,1,0,1,0,0}, {0,0,6,9,0,0}, {1,1,0,1,0,1}, {1,1,1,0,1,0}, {0,0,1,0,3,2} }, // 41
      { {1,0,1,0,1,0}, {0,0,0,0,0,0}, {1,0,1,0,1,0}, {1,1,1,1,0,1}, {0,3,0,2,0,1} }, // 42
      { {4,4,4,1,1,6}, {3,3,3,3,7,7}, {1,1,1,1,0,1}, {1,1,1,0,1,0}, {0,0,0,0,2,1} }, // 43
      { {3,3,3,3,1,0}, {0,0,0,0,0,6}, {1,1,1,1,1,0}, {1,1,1,1,0,1}, {0,0,0,3,2,1} }, // 44
      { {0,0,0,0,0,0}, {0,0,0,0,0,0}, {1,1,1,1,1,1}, {1,1,1,1,1,0}, {0,0,0,3,2,1} }, // 45
    };

    //==============================================================================
    // Décode la TOPOLOGIE EXACTE (modulateur -> opérateur) d'un algorithme AFM depuis
    // kAlgo, en SIMULANT le pipeline du SY77 (mêmes règles que fmEvalAlgo : ops évalués
    // OP6..OP1, registres + ACCUMULATEUR). edge[dst][src]=true si op(src) module op(dst) ;
    // feedback[op]=auto-modulation ; carrier[op]=op dont la sortie atteint l'accumulateur
    // FINAL (= va vers la sortie audio).
    //
    // Point CLÉ : le code d'entrée 9 (accumulateur) EST une vraie liaison de modulation —
    // l'op qui le lit est modulé par tous les ops déjà sommés/initialisés dans l'accumulateur
    // à cet instant (ordre OP6..OP1). L'ignorer donnait de faux algos (ex. algo 18 : sans
    // l'acc, OP4/5/6 paraissaient porteuses ; en réalité ils modulent OP1 via l'acc — vérifié
    // sur la grille SynthWorks et le widget algo de l'écran AFM). Codes : 1=thru op n+1,
    // 3/4/5=registre écrit par un op, 6/7/8=feedback, 9=accumulateur.
    inline void afmTopology (int algoNum, bool edge[6][6], bool feedback[6], bool carrier[6])
    {
        for (int d = 0; d < 6; ++d) { feedback[d] = false; carrier[d] = false; for (int s = 0; s < 6; ++s) edge[d][s] = false; }

        const AlgoDef& A = kAlgo[ ((algoNum - 1) % 45 + 45) % 45 ];
        int regProd[4] = { 0, 0, 0, 0 };
        for (int i = 0; i < 6; ++i) if (A.outd[i] >= 1 && A.outd[i] <= 3) regProd[A.outd[i]] = i + 1;

        // Contenu courant de l'accumulateur (numéros d'op), mis à jour OP6..OP1.
        int acc[6]; int accN = 0;
        auto addEdge = [&] (int dstNum, int srcNum)
        {
            if (srcNum >= 1 && srcNum <= 6 && dstNum >= 1 && dstNum <= 6 && srcNum != dstNum)
                edge[dstNum - 1][srcNum - 1] = true;
        };

        for (int num = 6; num >= 1; --num)
        {
            const int i = num - 1;
            const int src[2] = { A.in0[i], A.in1[i] };
            for (int s = 0; s < 2; ++s)
            {
                switch (src[s])
                {
                    case 1:  addEdge (num, num + 1);     break;             // thru : op n+1
                    case 3:  addEdge (num, regProd[1]);  break;
                    case 4:  addEdge (num, regProd[2]);  break;
                    case 5:  addEdge (num, regProd[3]);  break;
                    case 6: case 7: case 8: feedback[i] = true; break;     // auto-modulation
                    case 9:  for (int k = 0; k < accN; ++k) addEdge (num, acc[k]); break; // acc = modulation réelle
                    default: break;                                        // 0 off, 2 AWM, 10 bruit
                }
            }
            // Mise à JOUR de l'accumulateur APRÈS lecture (comme fmEvalAlgo).
            const int a0 = A.acc0[i], a1 = A.acc1[i];
            if      (a0 == 1 && a1 == 0) { accN = 0; acc[accN++] = num; }   // init/remplace
            else if (a0 == 1 && a1 == 1) { acc[accN++] = num; }            // somme (porteuse)
            else if (a0 == 0 && a1 == 1) { /* conserve */ }
            else                         { accN = 0; }                      // reset
        }
        // Porteuses = contenu FINAL de l'accumulateur (ce qui sonne).
        for (int k = 0; k < accN; ++k) carrier[acc[k] - 1] = true;
    }

    // Émule le pipeline AFM du SY77 : 6 opérateurs calculés OP6->OP1 (thru = op n+1),
    // registres + accumulateur (= somme des porteuses = sortie). fb[6] conserve l'état de
    // feedback de l'échantillon précédent (auto-modulation approchée pour les 6 algos à
    // feedback fixe : 19/36/40/41/43/44). Renvoie la sortie (non normalisée).
    inline float fmEvalAlgo (const int waves[6], const float ratios[6], const float levels[6],
                             int algoNum, float phase, float index, float fb[6])
    {
        const AlgoDef& A = kAlgo[ ((algoNum - 1) % 45 + 45) % 45 ];
        float opOut[8] = { 0 };
        float reg[3]   = { 0.0f, 0.0f, 0.0f };
        float acc      = 0.0f;

        for (int num = 6; num >= 1; --num)
        {
            const int i = num - 1;
            float mod = 0.0f;
            const int src[2] = { A.in0[i], A.in1[i] };
            for (int s = 0; s < 2; ++s)
            {
                switch (src[s])
                {
                    case 1:  mod += opOut[num + 1];     break;   // thru (opérateur num+1)
                    case 3:  mod += reg[0];             break;
                    case 4:  mod += reg[1];             break;
                    case 5:  mod += reg[2];             break;
                    case 9:  mod += acc;                break;   // accumulateur courant
                    case 6: case 7: case 8: mod += fb[i]; break; // feedback (auto-modulation approx)
                    default: break;                              // 0 off, 2 AWM, 10 bruit
                }
            }
            const float arg = ratios[i] * phase + index * mod;
            const float o   = afmWaveform (waves[i], arg) * levels[i];
            opOut[num] = o;
            fb[i] = o;
            if (A.outd[i] >= 1 && A.outd[i] <= 3) reg[A.outd[i] - 1] = o;

            const int a0 = A.acc0[i], a1 = A.acc1[i];
            if      (a0 == 1 && a1 == 0) acc = o;          // init/remplace
            else if (a0 == 1 && a1 == 1) acc = acc + o;    // somme (porteuse)
            else if (a0 == 0 && a1 == 1) { /* conserve */ }
            else                          acc = 0.0f;       // reset
        }
        return acc;
    }
}

//==============================================================================
// Surface interactive d'édition de la réponse de filtre : glisser = cutoff (X,
// gauche->droite) + résonance (Y, bas->haut) en 0..127 ; clic simple (sans glisser)
// = appelle onOpenEditor si défini. Ne dessine rien (la réponse est peinte dessous).
// Partagée par la vignette d'élément (Element.h) et l'éditeur de filtre complet
// (CommonFilter.h).
class FilterGraphView : public juce::Component
{
public:
    std::function<void()>                  onOpenEditor;  // clic simple (optionnel)
    std::function<void (int cut, int res)> onEdit;        // glissement (valeurs 0..127)

    FilterGraphView() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void mouseDown (const juce::MouseEvent&)   override { dragged = false; }
    void mouseDrag (const juce::MouseEvent& e) override { dragged = true; apply (e); }
    void mouseUp   (const juce::MouseEvent&)   override { if (! dragged && onOpenEditor) onOpenEditor(); }

private:
    void apply (const juce::MouseEvent& e)
    {
        auto b = getLocalBounds().toFloat();
        if (b.getWidth() < 2.0f || b.getHeight() < 2.0f || ! onEdit) return;
        const float fx = juce::jlimit (0.0f, 1.0f, e.position.x / b.getWidth());
        const float fy = juce::jlimit (0.0f, 1.0f, e.position.y / b.getHeight());
        onEdit (juce::roundToInt (fx * 127.0f), juce::roundToInt ((1.0f - fy) * 127.0f));
    }
    bool dragged = false;
};

//==============================================================================
// Overlay GÉNÉRIQUE d'édition d'enveloppe (EG) : glisser un nœud = niveau (Y, 0..maxLevel)
// + poids du segment entrant (X). Le host fournit les niveaux/poids DESSINÉS via getData
// (pour que le test de proximité colle au tracé de SyDraw::drawEnvelope) et reçoit les
// éditions via onEditNode (au host de convertir poids<->rate selon son modèle). Le nœud 0
// (départ) n'a pas de segment entrant -> segWeight = -1. Partagé par la vignette d'élément
// et les éditeurs d'EG plein-onglet.
class EgGraphView : public juce::Component
{
public:
    std::function<void (juce::Array<float>&, juce::Array<float>&)> getData;       // (levels, weights)
    std::function<void (int node, float level, float segWeight)>   onEditNode;    // segWeight<0 => inchangé
    std::function<void()>                                          onOpenEditor;  // clic simple (optionnel)
    float maxLevel = 127.0f;

    EgGraphView() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void mouseDown (const juce::MouseEvent& e) override { dragged = false; selected = nearestNode (e.position.x); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        dragged = true;
        if (selected < 0 || ! onEditNode || getWidth() < 2 || getHeight() < 2) return;
        const float fy = juce::jlimit (0.0f, 1.0f, e.position.y / (float) getHeight());
        const float level = (1.0f - fy) * maxLevel;
        const float segW  = (selected >= 1) ? solveSegWeight (selected, e.position.x / (float) getWidth()) : -1.0f;
        onEditNode (selected, level, segW);
    }
    void mouseUp (const juce::MouseEvent&) override { if (! dragged && onOpenEditor) onOpenEditor(); }

    // Affordance : surligne le nœud survolé (ou en cours de glissement) -> on voit
    // qu'il est saisissable. Ne dessine que l'anneau (la courbe est peinte dessous).
    void mouseMove (const juce::MouseEvent& e) override
    {
        const int h = nearestNode (e.position.x);
        if (h != hovered) { hovered = h; repaint(); }
    }
    void mouseExit (const juce::MouseEvent&) override
    {
        if (hovered != -1) { hovered = -1; repaint(); }
    }
    void paint (juce::Graphics& g) override
    {
        const int h = (dragged && selected >= 0) ? selected : hovered;
        if (h < 0) return;
        auto pts = nodePoints();
        if (h >= pts.size()) return;
        auto p = pts[h];
        g.setColour (SYColSelected.withAlpha (0.25f));
        g.fillEllipse (p.x - 6.0f, p.y - 6.0f, 12.0f, 12.0f);
        g.setColour (SYColSelected);
        g.drawEllipse (p.x - 6.0f, p.y - 6.0f, 12.0f, 12.0f, 2.0f);
    }

private:
    void computeData (juce::Array<float>& levels, juce::Array<float>& weights)
    {
        levels.clearQuick(); weights.clearQuick();
        if (getData) getData (levels, weights);
    }
    // Positions écran des nœuds (mêmes X/Y que SyDraw::drawEnvelope).
    juce::Array<juce::Point<float>> nodePoints()
    {
        juce::Array<juce::Point<float>> pts;
        juce::Array<float> levels, weights;
        computeData (levels, weights);
        if (levels.size() < 2 || weights.size() != levels.size() - 1 || maxLevel <= 0.0f) return pts;
        float total = 0.0f;
        for (auto w : weights) total += juce::jmax (1.0f, w);
        if (total <= 0.0f) return pts;
        const float W = (float) getWidth(), H = (float) getHeight();
        auto yFor = [&] (float lv) { return H - juce::jlimit (0.0f, maxLevel, lv) / maxLevel * H; };
        float x = 0.0f;
        pts.add ({ x, yFor (levels[0]) });
        for (int i = 0; i < weights.size(); ++i)
        {
            x += W * juce::jmax (1.0f, weights[i]) / total;
            pts.add ({ x, yFor (levels[i + 1]) });
        }
        return pts;
    }
    int nearestNode (float mx)
    {
        auto pts = nodePoints();
        int best = -1; float bestD = 1.0e9f;
        for (int i = 0; i < pts.size(); ++i)
        { const float d = std::abs (pts[i].x - mx); if (d < bestD) { bestD = d; best = i; } }
        return best;
    }
    // Poids du segment entrant du nœud `node` pour qu'il atterrisse à la fraction X `fx`,
    // les autres poids fixes : fx = (before + w)/(otherSum + w)  =>  on résout w. Clamp [1,99].
    float solveSegWeight (int node, float fx)
    {
        juce::Array<float> levels, weights;
        computeData (levels, weights);
        if (node < 1 || node - 1 >= weights.size()) return -1.0f;
        float otherSum = 0.0f, before = 0.0f;
        for (int i = 0; i < weights.size(); ++i)
        {
            if (i == node - 1) continue;
            const float w = juce::jmax (1.0f, weights[i]);
            otherSum += w;
            if (i < node - 1) before += w;
        }
        fx = juce::jlimit (0.0f, 0.98f, fx);
        return juce::jlimit (1.0f, 99.0f, (fx * otherSum - before) / (1.0f - fx));
    }
    int  selected = -1;
    int  hovered  = -1;
    bool dragged  = false;
};
