/*
  ==============================================================================

    AlgoModel.h
    Modèle d'ALGORITHME FM CUSTOM (libre) par élément AFM, stocké dans le ValueTree
    plat global. Le SY77 permet un « free algorithm » (ALGNUM=127) où chaque
    opérateur déclare ses 2 entrées, sa destination et l'état d'accumulateur.

    On stocke les champs ALIGNÉS sur SyDraw::AlgoDef (in0/in1/outd/acc0/acc1) pour
    pouvoir DESSINER le custom gratuitement (AlgoDraw::setCustomAlgo), plus les 2
    SHIFT par op (non dessinés, transmis tels quels à l'envoi). Identifiers
    construits dynamiquement (même style que le code PAN/filtre) -> pas 168 DECLARE_ID.

    Champs/props par élément e (1..4) et op o (1..6) :
      AFMELEMENT<e>ALGINA<o>  (in0,  code 0-10)   AFMELEMENT<e>ALGINB<o>  (in1, 0-10)
      AFMELEMENT<e>ALGOUT<o>  (outd, 0-3)
      AFMELEMENT<e>ALGACCA<o> (acc0, 0-1)         AFMELEMENT<e>ALGACCB<o> (acc1, 0-1)
      AFMELEMENT<e>ALGSHA<o>  (shift0, 0-7)        AFMELEMENT<e>ALGSHB<o>  (shift1, 0-7)
    Le LEVEL (TL) reste sur AFMELEMENT<e>LEVEL<o> (NE PAS dupliquer).

    Codes d'entrée (in0/in1) : 0=Off, 1=Thru(op n+1), 2=AWM, 3/4/5=Reg1/2/3,
      6/7/8=Feedback, 9=Accumulateur, 10=Noise.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "EnvelopeDraw.h"   // SyDraw::AlgoDef + kAlgo
#include "Values.h"         // valueTreeVoice

namespace SyAlgo
{
    /** Identifier d'un champ d'algo custom (op 1..6). field non terminé par un chiffre
        -> « ...ALGINA1 » est sans ambiguïté (field=INA, op=1). */
    inline juce::Identifier algoId (int element, const char* field, int op)
    {
        return juce::Identifier ("AFMELEMENT" + juce::String (element) + "ALG" + field + juce::String (op));
    }

    /** Construit l'AlgoDef custom de l'élément depuis les props (pour dessin/envoi). */
    inline SyDraw::AlgoDef buildCustomAlgo (int element)
    {
        SyDraw::AlgoDef d {};
        for (int o = 0; o < 6; ++o)
        {
            auto get = [&] (const char* f, int def)
            { return (int) valueTreeVoice.getProperty (algoId (element, f, o + 1), def); };
            d.in0[o]  = get ("INA",  0);
            d.in1[o]  = get ("INB",  0);
            d.outd[o] = get ("OUT",  0);
            d.acc0[o] = get ("ACCA", 0);
            d.acc1[o] = get ("ACCB", 0);
            // SHA/SHB : pas dans AlgoDef (non dessinés) ; lus à l'envoi.
        }
        return d;
    }

    /** Amorce les props custom de l'élément à partir d'un preset 1..45 (« Dériver du
        preset N ») : copie kAlgo[N-1] dans INA/INB/OUT/ACCA/ACCB, shifts à 0. */
    inline void seedCustomFromPreset (int element, int presetN)
    {
        const SyDraw::AlgoDef& A = SyDraw::kAlgo[ ((presetN - 1) % 45 + 45) % 45 ];
        for (int o = 0; o < 6; ++o)
        {
            auto set = [&] (const char* f, int v)
            { valueTreeVoice.setProperty (algoId (element, f, o + 1), v, nullptr); };
            set ("INA",  A.in0[o]);
            set ("INB",  A.in1[o]);
            set ("OUT",  A.outd[o]);
            set ("ACCA", A.acc0[o]);
            set ("ACCB", A.acc1[o]);
            set ("SHA",  0);
            set ("SHB",  0);
        }
    }

    /** True si l'élément a au moins une prop custom posée (déjà amorcé). */
    inline bool hasCustomAlgo (int element)
    {
        return valueTreeVoice.hasProperty (algoId (element, "INA", 1));
    }
}
