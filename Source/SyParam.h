/*
  ==============================================================================

    SyParam.h
    Façade du dictionnaire de paramètres déclaratif (écrite à la main ; la table
    de données `SyParamTable.h` est GÉNÉRÉE, ne pas l'éditer). Réunit les 3 piliers
    purs et testés :
      1. SyParamTable.h  — la table {ui, nom, zone, op, N2, SyEnc, encA/B, plage}.
      2. SyVoice::syTranslate — ui (+élément/filtre) -> (group, T2, N2).   [adresse]
      3. SyVoice::SyEnc + primitives smToWire/obToWire/ob2*  — affichage -> filaire. [encodage]
    Et expose la liaison `bytesFor()` : un `ui` + une valeur AFFICHÉE -> le message
    param-change 9 octets prêt à émettre. C'est l'API que les widgets consommeront
    (Phase 4), en remplacement des `{0x43,…,group,addrHi,…,N2,…}` recopiés à la main.

  ==============================================================================
*/
#pragma once
#include "SyParamTable.h"   // table générée + SysexUtils (SyVoice)

namespace SyParam
{
    /** Entrée de la table pour un `ui` (nullptr si inconnu). Recherche linéaire (table figée). */
    inline const Entry* entry (int ui)
    {
        for (const auto& e : kParams) if (e.ui == ui) return &e;
        return nullptr;
    }

    struct Msg { std::array<juce::uint8, 9> bytes; bool ok; };

    /** Bout-en-bout : ui (SyParamTable) + valeur AFFICHÉE (signée éventuellement) ->
        message param-change 9 octets. Enchaîne table (encodage+plage) -> syTranslate
        (adresse) -> paramBytes. `ok=false` si ui inconnu/non transmissible.
        - plain / signMag / offBin1 : octet de valeur dans V2 (octet [8]).
        - offBin2 (break-point offsets) : V1 (octet [7]) = bit7, V2 = low7. */
    inline Msg bytesFor (int ui, int display, int element, int filterTarget, int deviceNumber)
    {
        const Entry* e = entry (ui);
        const SyVoice::SyAddr a = SyVoice::syTranslate (ui, element, filterTarget);
        if (e == nullptr || ! a.ok) return { {}, false };

        display = juce::jlimit (e->dispMin, e->dispMax, display);
        auto m = SyVoice::paramBytes (deviceNumber, a.group, a.t2, 0x00, a.n2, 0);
        switch (e->enc)
        {
            case SyVoice::SyEnc::plain:   m[8] = (juce::uint8) display;                                  break;
            case SyVoice::SyEnc::signMag: m[8] = (juce::uint8) SyVoice::smToWire (display, e->encA, e->encB); break;
            case SyVoice::SyEnc::offBin1: m[8] = (juce::uint8) SyVoice::obToWire (display, e->encA, e->encB); break;
            case SyVoice::SyEnc::offBin2: m[7] = SyVoice::ob2V1 (display); m[8] = SyVoice::ob2V2 (display);    break;
        }
        return { m, true };
    }
}
