# Sysex77 — Carte de couverture des paramètres

État de ce que l'éditeur sait faire **de façon vérifiée**, paramètre par paramètre, sur deux axes :

- **Envoi → synthé** : l'éditeur émet le bon message *parameter-change* et le synthé applique le bon paramètre.
- **Ouverture dump → éditeur** : à l'ouverture d'un `.syx`, l'éditeur lit le bon octet du blob et affiche la bonne valeur.

> Objectif : informer honnêtement de ce qui est **fiable** ou non. Tant qu'une case n'est pas ✅/🟢,
> ne pas s'y fier pour de l'édition critique. Politique « fiabilité d'abord » : on n'active/charge
> jamais un paramètre tant que son adresse/offset n'est pas confirmé contre du vrai matériel.

## Légende

| Symbole | Signification |
|---|---|
| ✅ | **Vérifié hardware** (testé sur un SY77 réel : édition en direct ou aller-retour dump) |
| 🟢 | **Chargé & vérifié** (ouverture dump : octet confirmé par diff single-param ou oracle de test) |
| 🟡 | **Câblé, non vérifié** (émis / spec-correct, mais pas encore confirmé sur le synthé) |
| ❓ | **Douteux** (adresse ou offset incertain — à revérifier) |
| ⬜ | **Non implémenté / non chargé** |

Couverture du chargement : **AFM 1/2/4** (profond) **+ AWM 1/2/4** (waveform + niveau + amp-EG),
mono+poly, tous les éléments. Pattern Table 2 : base élément 1 = 107 + 9×(N−1) ; bloc = 357 o.
(AFM) / 112 o. (AWM) ; addrHi = élément<<5. AFM diff-vérifié ; AWM waveform + amp-EG confirmés
(TARKUSCYMB). L'amp-EG AWM est désormais câblé dans l'éditeur (WaveEg mode AWM → group 0x07 @50-56).

Le **filtre AWM** est désormais **câblé** (édition) : les éditeurs de filtre basculent en fN 3/4/5
quand l'élément est AWM (à spot-checker hardware). Son **chargement** depuis dump n'est pas encore
ajouté (offsets AWM filtre non vérifiés).

Les **voix mixtes AFM+AWM** (1AFM_1AWM type 8, 2AFM_2AWM type 9) sont gérées : moteur par élément,
blocs cumulatifs (AFM 357 o. / AWM 112 o.).

> **Validation offline (2026-06-24)** : décodeur rejoué sur **811 voix réelles** (14 banques, types
> 0-9 dont 171 mixtes type-8 + 74 type-9) — **0 valeur hors plage**. Forte confiance.
> Sur 1224 voix, ~413 restantes = surtout **drum (type 10)**.

Restant : **drum** (type 10) ; AWM filtre (chargement) + fine/fixed (encodage à lever) ;
niveaux EG en o/b ; effets ; LFO.

---

## Voice Common

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| Nom de voix | 🟡 | 🟢 | offsets 33-42 ; envoi spec-correct, hardware à confirmer |
| Type / mode de voix | 🟡 | 🟢 | offset 32 |
| **Volume voix** | 🟡 | 🟢 | dump @95 (Table 2) ; group 0x02 / param 0x3F — à spot-checker |
| Niveau élément 1 (ELVL) | 🟡 | 🟢 | dump @98 (Table 2) ; group 0x03 / param 0x00 — à spot-checker |
| Pan, niveaux d'envoi (rev/cho/var) | 🟡 | ⬜ | — |

## AFM — Élément commun (élément 1)

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| **Algorithme (ALGNUM)** | 🟡 | 🟢 | dump **@377 confirmé** (diff single-param 16→1) ; envoi via l'app pas encore testé |
| Pitch EG rates (FPR1-3, FPRR1) | 🟡 | 🟢 | bloc N2-ordonné @base+270+N2 ; validé 566 voix |
| Pitch EG levels (o/b) | 🟡 | ⬜ | repr. éditeur 0..64 à aligner sur o/b |
| LFO 1 / LFO 2 | 🟡 | ⬜ | — |

## AFM — Opérateurs (élément 1, OP1→OP6)

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| EG rates R1-R4 | ✅ | 🟢 | aller-retour fingerprint confirmé |
| EG rates RR1, RR2 | ✅ | 🟢 | |
| EG levels L0-L4 | ✅ | 🟢 | |
| EG levels RL1, RL2 | ✅ | 🟢 | |
| **Niveau de sortie (TL)** | ✅ | 🟢 | Track A + aller-retour (6/6) |
| Fine | ✅ | 🟢 | aller-retour @interne 44 (6/6) |
| Coarse | ❓ | ⬜ | n'a pas matché à l'aller-retour (packé ?) |
| Detune | ❓ | ⬜ | encodage négatif non résolu |
| RS / Slope / HT | ❓ | ⬜ | interne 15 = constante dans le dump → offset à revoir |
| Break points / scaling | ⬜ | ⬜ | — |
| Waveform | ⬜ | ⬜ | pas de sélecteur dans l'éditeur |

## Filtres (filtre 1 & 2, élément AFM)

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| Type filtre 1 (FTYPE : LPF/HPF/Thru) | ✅ | ❓ | envoi Track A ; dump @403 (offset OK) mais encodage bulk ambigu (Thru→LPF = 0→1, ≠ enum) — besoin d'un point HPF |
| Mode de contrôle (FMODE) f1 & f2 | ✅ | 🟢 | dump @base+298/+327 ; validé 566 voix |
| EG filtre 1 & 2 : rates (FR1-4, FRR1-2) | 🟡 | 🟢 | dump @base+296/+325+N2 ; validé 566 voix |
| EG filtre : niveaux (FL0-4, FRL1-2, o/b) | 🟡 | ⬜ | repr. éditeur à aligner sur o/b |
| **Cutoff filtre 1** | 🟡 | 🟢 | dump **@404 confirmé** (diff single-param 127→0) ; group 0x09/param 0x01 |
| Cutoff filtre 2 | 🟡 | 🟢 | dump @433 (Table 2) ; group 0x09 addrHi 1 / param 0x01 — à spot-checker |
| Résonance | 🟡 | 🟢 | dump @461 (Table 2) ; group 0x09 addrHi 2 / param 0x32 — à spot-checker |
| Filter EG : RR2, RL1, RL2 | ✅ | ⬜ | Track A |
| Filter EG : R1-4, RR1, L0-4, slope | 🟡 | ⬜ | câblé group 0x09 |

## Effets

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| Effect mode, Chorus 1/2, Reverb 1/2 | ❓ | ⬜ | adresses « plausibles » (group 0x08), non vérifiées |

## Éléments 2-4 / AWM

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| Tous | 🟡 | ⬜ | le loader ne traite que l'élément 1 AFM ; AWM non géré |

---

## Méthode de validation (référence)

La méthode fiable pour cartographier un offset de dump, sans ambiguïté :

1. **Dump** la voix (`.syx`).
2. Change **UN seul** paramètre sur le synthé (valeur bien distincte).
3. **Re-dump**.
4. **Diff** des deux blocs (`SyVoice::diffVoiceBlocks`) → le seul octet qui bouge (hors checksum)
   est l'offset du paramètre.

Le « fingerprint global » (bouton RE) sert de **test de vie** (ça réagit) mais est trop ambigu
pour une carte 1:1 (563 contrôles, valeurs cycliques sur 60 → ~9 collisions par valeur).

## Mettre à jour cette carte

Quand un paramètre est confirmé (envoi testé sur le synthé, ou offset de dump prouvé par diff),
passer sa case à ✅/🟢 ici. Le décodeur de chargement est `SyVoice::voiceBlobToParams()`
(`Source/SysexUtils.h`), testé dans `Source/Tests.h`. Détails d'offsets de travail :
`docs/bulk_offset_map_WIP.md` (non versionné).
