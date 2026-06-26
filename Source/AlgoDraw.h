/*
  ==============================================================================

    AlgoDraw.h — schéma de TOPOLOGIE des 45 algorithmes AFM du SY77.

    Style ORTHOGONAL (façon widget « ALGORITHM » de l'écran AFM ELEMENT du SY77,
    cf. docs/MT_91_01_geerdes_sy77_full.jpg) : opérateurs en boîtes numérotées,
    modulateur EMPILÉ au-dessus de sa cible, connecteurs uniquement HORIZONTAUX ou
    VERTICAUX (coudes à angle droit) — jamais de diagonale, jamais de trait qui
    traverse une boîte. Porteuses (-> sortie audio) sur la rangée du bas, reliées à
    une barre de sortie. Toutes les couleurs proviennent des rôles de la palette
    de thème (jamais de couleur en dur).

  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>
#include "EnvelopeDraw.h"   // SyDraw::afmTopology / kAlgo (topologie exacte AFM)

//==============================================================================
class AlgoDraw   : public Component
{
public:
    AlgoDraw()                         { setInterceptsMouseClicks (false, false); }
    ~AlgoDraw() override               {}

    // Preset 1..45 (efface l'algo custom éventuel).
    void setAlgo (int number)          { intAlgoNumber = number; useCustom = false; repaint(); }
    // Algorithme LIBRE / custom : prioritaire sur le numéro de preset tant qu'il est posé.
    void setCustomAlgo (const SyDraw::AlgoDef& d) { customAlgo = d; useCustom = true; repaint(); }

    //==============================================================================
    void paint (Graphics& g) override
    {
        g.fillAll (SYPal.background);                 // fond opaque (rôle thème)
        g.setColour (SYPal.panelBorder);
        g.drawRect (0, 0, getWidth(), getHeight());   // cadre discret (rôle thème)

        auto area = getLocalBounds().toFloat().reduced (8.0f);
        if (useCustom) drawAlgo (g, customAlgo,    area, /*labels*/ true);
        else           drawAlgo (g, intAlgoNumber, area, /*labels*/ true);
    }

    //==============================================================================
    // Rendu COMPACT (vue Voice / carte ALGO-ROUTAGE) : même moteur, boîtes plus
    // petites, numéros si la place le permet. 'area' = zone allouée au glyphe.
    static void drawAlgoGlyph (Graphics& g, int algoNum, Rectangle<float> area)
    {
        drawAlgo (g, algoNum, area, /*labels*/ area.getHeight() > 44.0f);
    }

    //==============================================================================
    // Moteur de dessin partagé (grand schéma et glyphe). Calcule un placement en
    // grille (rangée = profondeur, colonne centrée sur les cibles) puis route les
    // liens en orthogonal strict via des couloirs entre rangées.
    static void drawAlgo (Graphics& g, int algoNum, Rectangle<float> area, bool labels)
    {
        bool edge[6][6], fb[6], car[6];
        SyDraw::afmTopology (algoNum, edge, fb, car);
        drawTopo (g, edge, fb, car, area, labels);
    }

    // Surcharge CUSTOM : dessine un AlgoDef quelconque (algorithme LIBRE / free), pas
    // seulement un preset 1..45. Même moteur (topologie calculée puis dessinée).
    static void drawAlgo (Graphics& g, const SyDraw::AlgoDef& def, Rectangle<float> area, bool labels)
    {
        bool edge[6][6], fb[6], car[6];
        SyDraw::afmTopology (def, edge, fb, car);
        drawTopo (g, edge, fb, car, area, labels, &def);   // &def -> tags AWM/Noise par op
    }

    // Cœur de dessin partagé : à partir de la topologie DÉJÀ calculée (edge/fb/carrier).
    // srcDef (optionnel) : permet de SYMBOLISER les entrées externes AWM/Noise d'un op.
    static void drawTopo (Graphics& g, bool edge[6][6], bool fb[6], bool car[6],
                          Rectangle<float> area, bool labels, const SyDraw::AlgoDef* srcDef = nullptr)
    {
        int   row[6];        // 0 = porteuse (bas) ; profondeur croissante vers le haut
        float colF[6];       // colonne fractionnaire (centroïde) avant normalisation
        layout (edge, car, row, colF);

        // --- Normalise les colonnes sur une grille entière compacte (sans trous). ---
        // Tri des valeurs de colonne distinctes -> index 0..C-1 (ordre gauche->droite).
        float uniq[6]; int nUniq = 0;
        for (int o = 0; o < 6; ++o)
        {
            bool found = false;
            for (int k = 0; k < nUniq; ++k) if (std::abs (uniq[k] - colF[o]) < 0.001f) { found = true; break; }
            if (! found) uniq[nUniq++] = colF[o];
        }
        for (int a = 0; a < nUniq; ++a) for (int b = a + 1; b < nUniq; ++b) if (uniq[b] < uniq[a]) std::swap (uniq[a], uniq[b]);
        int col[6];
        for (int o = 0; o < 6; ++o)
            for (int k = 0; k < nUniq; ++k) if (std::abs (uniq[k] - colF[o]) < 0.001f) { col[o] = k; break; }

        int maxRow = 0, cols = nUniq;
        for (int o = 0; o < 6; ++o) maxRow = jmax (maxRow, row[o]);
        const int rows = maxRow + 1;

        // --- Géométrie : cellule = boîte + gouttière. Réserve une rangée sous les
        //     porteuses pour la barre de sortie. ---
        const float cellW = area.getWidth()  / (float) jmax (1, cols);
        const float cellH = area.getHeight() / (float) (rows + 1);     // +1 = sortie
        const float boxS  = jmin (cellW, cellH) * 0.62f;
        // Pas régulier = boîte + gouttière, mais PLAFONNÉ au pas de cellule alloué : sinon, pour
        // les algos profonds (ex. ALG 1 = pile de 6 ops), rows*rowH dépassait la hauteur et la
        // boîte du haut était tronquée. jmin(cell…) garantit que (rows+1) rangées tiennent.
        const float colW  = jmin (cellW, boxS + jmax (boxS * 0.6f, 8.0f));   // pas horizontal
        const float rowH  = jmin (cellH, boxS + jmax (boxS * 0.9f, 10.0f));  // pas vertical (gouttière)

        const float gridW = colW * cols;
        const float xL    = area.getX() + (area.getWidth() - gridW) * 0.5f + colW * 0.5f;
        const float yBot  = area.getBottom() - rowH;                   // centre des porteuses (rangée 0)

        auto cx = [&] (int o) { return xL + col[o] * colW; };
        auto cy = [&] (int o) { return yBot - row[o] * rowH; };
        auto boxOf = [&] (int o) { return Rectangle<float> (0, 0, boxS, boxS).withCentre ({ cx (o), cy (o) }); };

        const Colour modCol = SYPal.textMuted;
        const Colour carCol = SYPal.accent;
        const float  lw     = jmax (1.0f, boxS * 0.07f);

        // --- 1) Liens modulateur -> cible, EN ORTHOGONAL. ---
        // Couloir horizontal = milieu de la gouttière SOUS la rangée du modulateur
        // (toujours vide de boîtes). Stub vertical depuis le bas du modulateur, segment
        // horizontal dans le couloir, stub vertical dans le haut de la cible.
        g.setColour (modCol);
        for (int dst = 0; dst < 6; ++dst)
            for (int src = 0; src < 6; ++src)
                if (edge[dst][src])
                    routeOrtho (g, boxOf (src), boxOf (dst), modCol, lw);

        // --- 2) Feedback : petite boucle orthogonale sur le coin sup. droit. ---
        for (int o = 0; o < 6; ++o)
            if (fb[o]) drawFeedback (g, boxOf (o), modCol, lw);

        // --- 3) Barre de sortie + descente verticale de chaque porteuse. ---
        const float outY = area.getBottom() - rowH * 0.30f;
        g.setColour (carCol);
        g.drawLine (area.getX() + 2.0f, outY, area.getRight() - 2.0f, outY, lw + 0.4f);
        for (int o = 0; o < 6; ++o)
            if (car[o])
            {
                auto b = boxOf (o);
                g.drawLine (b.getCentreX(), b.getBottom(), b.getCentreX(), outY, lw + 0.2f);
            }
        if (labels)
        {
            g.setColour (SYPal.textMuted);
            g.setFont (Font (jmin (11.0f, rowH * 0.30f)));
            g.drawText ("OUT", area.removeFromRight (30).withTop (outY - 13.0f).withHeight (12.0f),
                        Justification::topRight);
        }

        // --- 4) Boîtes d'opérateurs PAR-DESSUS les traits. ---
        g.setFont (Font (jmax (8.0f, boxS * 0.52f), Font::bold));
        for (int o = 0; o < 6; ++o)
        {
            auto b = boxOf (o);
            const Colour eC = car[o] ? carCol : modCol;
            g.setColour (car[o] ? carCol.withAlpha (SYPal.dark ? 0.20f : 0.14f) : SYPal.surfaceAlt);
            g.fillRoundedRectangle (b, 2.5f);
            g.setColour (eC);
            g.drawRoundedRectangle (b, 2.5f, car[o] ? lw + 0.5f : lw);
            if (labels)
            {
                g.setColour (car[o] ? carCol : SYPal.textPrimary);
                g.drawText (String (o + 1), b, Justification::centred);
            }
        }

        // --- 5) Entrées EXTERNES (AWM / Noise) symbolisées à GAUCHE de l'op concerné. ---
        // Codes d'entrée (cf. AlgoModel.h) : 2 = AWM, 10 = Noise. Petit badge + connecteur
        // dans le bord gauche de la boîte (zone libre : les stubs modulateur viennent du haut/bas).
        if (srcDef != nullptr)
        {
            const float fs = jmax (6.0f, boxS * 0.34f);
            g.setFont (Font (fs, Font::bold));
            for (int o = 0; o < 6; ++o)
            {
                const bool awm = (srcDef->in0[o] == 2  || srcDef->in1[o] == 2);
                const bool nz  = (srcDef->in0[o] == 10 || srcDef->in1[o] == 10);
                if (! awm && ! nz) continue;

                String tag = awm ? String ("AWM") : String();
                if (nz) tag = tag.isEmpty() ? String ("NZ") : tag + "+NZ";

                auto b = boxOf (o);
                const float tw = jmax (boxS * 0.9f, fs * (float) tag.length() * 0.72f + 6.0f);
                const float th = jmax (fs + 3.0f, boxS * 0.5f);
                Rectangle<float> tagR (b.getX() - 6.0f - tw, b.getCentreY() - th * 0.5f, tw, th);

                g.setColour (SYPal.surfaceAlt);
                g.fillRoundedRectangle (tagR, 2.0f);
                g.setColour (SYPal.accent);
                g.drawRoundedRectangle (tagR, 2.0f, jmax (0.8f, lw * 0.8f));
                g.drawLine (tagR.getRight(), b.getCentreY(), b.getX(), b.getCentreY(), lw * 0.8f);   // connecteur
                g.drawText (tag, tagR, Justification::centred);
            }
        }
    }

private:
    //==============================================================================
    // Placement type « arbre centré » (façon widget algo du SY77) :
    //   rangée  = profondeur (0 = porteuse en bas, croissant vers le haut) ;
    //   colonne = parcours postfixe depuis chaque PORTEUSE en remontant ses modulateurs.
    // Les feuilles (modulateurs sans modulateur) prennent des colonnes successives dans
    // l'ordre de visite ; chaque op interne se centre sur le barycentre de SES modulateurs.
    // Modulateurs visités par n° d'op croissant -> rangées du haut lisibles (2,4,5,6…),
    // porteuses centrées sous leur grappe. Pas de croisement pour les algos arborescents.
    static void layout (const bool edge[6][6], const bool car[6], int row[6], float colF[6])
    {
        // Profondeur = plus long chemin op -> porteuse (le long des arêtes mod->cible).
        for (int o = 0; o < 6; ++o) { row[o] = 0; colF[o] = -1.0f; }
        for (int pass = 0; pass < 6; ++pass)
            for (int dst = 0; dst < 6; ++dst)
                for (int src = 0; src < 6; ++src)
                    if (edge[dst][src]) row[src] = jmax (row[src], row[dst] + 1);

        int maxR = 0; for (int o = 0; o < 6; ++o) maxR = jmax (maxR, row[o]);

        // (1) PORTEUSES (rangée 0) : colonnes distinctes, gauche->droite par n° d'op.
        //     Distinctes par construction -> jamais de chevauchement sur la barre de sortie.
        float nextCol = 0.0f;
        for (int o = 0; o < 6; ++o) if (car[o] && row[o] == 0) colF[o] = nextCol++;

        // (2) MODULATEURS, rangée par rangée du bas vers le haut : chaque op VISE le
        //     barycentre des cibles qu'il module (déjà placées, rangées inférieures).
        //     Puis, dans la rangée, on RÉSOUT LES COLLISIONS par un balayage gauche->droite
        //     imposant un écart minimal de 1.0 (façon Reingold-Tilford) : les ops sont triés
        //     par colonne désirée (puis n° d'op), poussés vers la droite pour ne jamais se
        //     chevaucher, puis le bloc est recentré sur le barycentre des désirs. Garantit
        //     ZÉRO chevauchement de boîtes — corrige les cas (36/40/41/19/43…) où deux
        //     grappes voisines (carriers à distance 1, chacun avec 2 modulateurs) se
        //     télescopaient sur une colonne commune.
        for (int r = 1; r <= maxR; ++r)
        {
            float want[6]; int ops[6]; int m = 0;
            for (int o = 0; o < 6; ++o)
            {
                if (row[o] != r || colF[o] >= 0.0f) continue;
                float sum = 0.0f; int n = 0;
                for (int dst = 0; dst < 6; ++dst)
                    if (edge[dst][o] && colF[dst] >= 0.0f) { sum += colF[dst]; ++n; }
                want[o] = (n > 0) ? sum / (float) n : nextCol++;   // sans cible placée : colonne neuve
                ops[m++] = o;
            }
            if (m == 0) continue;

            // Tri par (colonne désirée croissante, puis n° d'op) — ordre stable type oracle.
            for (int a = 0; a < m; ++a) for (int b = a + 1; b < m; ++b)
                if (want[ops[b]] < want[ops[a]] - 0.001f
                    || (std::abs (want[ops[b]] - want[ops[a]]) <= 0.001f && ops[b] < ops[a]))
                    std::swap (ops[a], ops[b]);

            // Balayage : pos[k] >= pos[k-1] + 1, et >= want.
            float pos[6];
            pos[0] = want[ops[0]];
            for (int k = 1; k < m; ++k)
                pos[k] = jmax (want[ops[k]], pos[k - 1] + 1.0f);

            // Recentre le bloc pour minimiser le décalage moyen / désir (garde l'alignement).
            float shift = 0.0f;
            for (int k = 0; k < m; ++k) shift += (want[ops[k]] - pos[k]);
            shift /= (float) m;
            for (int k = 0; k < m; ++k) colF[ops[k]] = pos[k] + shift;

            // Met à jour nextCol pour les rangées suivantes (évite de réutiliser ces colonnes).
            for (int k = 0; k < m; ++k) nextCol = jmax (nextCol, colF[ops[k]] + 1.0f);
        }

        // (3) Filet de sécurité : ops jamais placés (cycles purs) -> colonne neuve.
        for (int o = 0; o < 6; ++o) if (colF[o] < 0.0f) colF[o] = nextCol++;
    }

    //==============================================================================
    // Route un lien src(haut) -> dst(bas) en ORTHOGONAL strict (jamais en diagonale,
    // jamais à travers une boîte). Si même colonne : trait vertical direct. Sinon :
    // descente verticale jusqu'au couloir de la gouttière sous la rangée du modulateur,
    // segment horizontal, puis descente verticale dans le haut de la cible.
    static void routeOrtho (Graphics& g, Rectangle<float> bs, Rectangle<float> bd, Colour c, float lw)
    {
        g.setColour (c);
        const float sx = bs.getCentreX(), sb = bs.getBottom();
        const float dx = bd.getCentreX(), dt = bd.getY();

        if (std::abs (sx - dx) < 0.5f)                      // même colonne -> vertical pur
        {
            g.drawLine (sx, sb, dx, dt, lw);
            return;
        }
        // Couloir horizontal : à mi-chemin entre le bas du modulateur et le haut de la
        // cible (cet espace est une gouttière sans boîte par construction du placement).
        const float chY = (sb + dt) * 0.5f;
        g.drawLine (sx, sb, sx, chY, lw);                  // descente verticale (modulateur)
        g.drawLine (sx, chY, dx, chY, lw);                 // segment horizontal (couloir)
        g.drawLine (dx, chY, dx, dt, lw);                  // descente verticale (cible)
    }

    // Indicateur de FEEDBACK (auto-modulation) : petite BOUCLE FERMÉE, à angle droit,
    // qui sort du HAUT de la boîte, contourne le coin supérieur droit et REVIENT dans le
    // haut de la même boîte — symbole de rebouclage sortie->entrée, entièrement rattaché
    // à l'opérateur (aucun trait flottant dans le vide). Inspiré du repère de feedback du
    // widget algo SynthWorks, version orthogonale propre.
    static void drawFeedback (Graphics& g, Rectangle<float> b, Colour c, float lw)
    {
        g.setColour (c);
        const float s   = jmin (b.getWidth(), b.getHeight()) * 0.34f;  // taille de la boucle
        const float yT  = b.getY();                       // bord haut de la boîte
        const float xOut = b.getCentreX() + b.getWidth() * 0.18f;      // départ (sortie rebouclée)
        const float xIn  = b.getCentreX() - b.getWidth() * 0.18f;      // arrivée (entrée)
        const float top  = yT - s;                        // sommet de la boucle

        Path p;
        p.startNewSubPath (xOut, yT);     // part du haut de la boîte (sortie)
        p.lineTo (xOut, top);             // monte
        p.lineTo (xIn,  top);             // traverse vers la gauche (au-dessus de la boîte)
        p.lineTo (xIn,  yT);              // redescend dans le haut de la boîte (entrée)
        g.strokePath (p, PathStrokeType (lw, PathStrokeType::mitered, PathStrokeType::square));

        // Petite pointe de flèche à l'arrivée -> indique le sens (rebouclage vers l'entrée).
        const float ah = jmax (2.0f, s * 0.30f);
        Path tri;
        tri.addTriangle (xIn, yT,
                         xIn - ah * 0.6f, yT - ah,
                         xIn + ah * 0.6f, yT - ah);
        g.fillPath (tri);
    }

    int intAlgoNumber = 1;
    SyDraw::AlgoDef customAlgo {};   // algo libre/custom (utilisé si useCustom)
    bool useCustom = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AlgoDraw)
};
