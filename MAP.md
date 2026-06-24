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
mono+poly, tous les éléments **+ voix mixtes AFM+AWM** (type 8 & 9). Pattern Table 2 : base
élément 1 = 107 + 9×(N−1) ; bloc = 357 o. (AFM) / 112 o. (AWM) ; addrHi = élément<<5.
Couvert par dump (`SysexUtils.h:139-252`) : par opérateur AFM — EG (R/L/RR/RL/L0), TL, **Coarse,
Waveform, Detune, Fine**, **SENSIT (VEL/AM)**, **SCALING (BP1-4 + offset-levels) + VEL SW** ;
par élément — niveau, algo, pitch-EG rates + PEG-switch, filtres (mode/cutoff/réso/rates) ;
**AWM — waveform + amp-EG (group 0x07 @50-56)** ; volume de voix commun.

Preuve par symbole (politique « fiabilité d'abord ») : 🟢 = oracle de test sur vrai dump
(`Tests.h` : **SteelStrng** 1AFM, **TARKUSCYMB** 1AWM) ou diff single-param documenté ; 🟡 =
décodeur le lit mais sans oracle/diff ; ❓ = encodage encore douteux ; ⬜ = non chargé.

Le **filtre AWM** est désormais **câblé** (édition) : les éditeurs de filtre basculent en fN 3/4/5
quand l'élément est AWM (à spot-checker hardware). Son **chargement** depuis dump n'est pas encore
ajouté (offsets AWM filtre non vérifiés).

Les **voix mixtes AFM+AWM** (1AFM_1AWM type 8, 2AFM_2AWM type 9) sont gérées (chargement) : moteur
par élément, blocs cumulatifs (AFM 357 o. / AWM 112 o.) — testé sur type 8 (`Tests.h`).

> **Validation offline (2026-06-24)** : décodeur rejoué sur **811 voix réelles** (14 banques, types
> 0-9 dont 171 mixtes type-8 + 74 type-9) — **0 valeur hors plage**. Forte confiance.
> Sur 1224 voix, ~413 restantes = surtout **drum (type 10)**.

Restant : **drum** (type 10) ; AWM filtre (chargement) + fine/fixed (encodage à lever) ;
niveaux EG en o/b ; PHASE/SYNC packé ; coarse/detune AFM négatif/packé (s/m) ; effets ; LFO.

---

## Voice Common

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| Nom de voix | 🟡 | 🟢 | offsets 33-42 ; envoi spec-correct, hardware à confirmer |
| Type / mode de voix | 🟡 | 🟢 | offset 32 |
| **Volume voix** | 🟡 | 🟢 | dump @95 ; group 0x02 / param 0x3F ; oracle SteelStrng `val(0x02,0x3F)==60` (Tests.h:238) |
| **Niveau élément (ELVL)** | 🟡 | 🟢 | dump @98+9·e ; group 0x03 / param 0x00 ; oracle SteelStrng `val(0x03,0x00)==1` (Tests.h:239) |
| Pan, niveaux d'envoi (rev/cho/var) | 🟡 | ⬜ | — |

## AFM — Élément commun (élément 1)

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| **Algorithme (ALGNUM)** | 🟡 | 🟢 | dump **@377 confirmé** (diff single-param 16→1) ; oracle `val(0x05,0x00)==23` (Tests.h:234) |
| **Pitch EG rates (FPR1-3, FPRR1)** | 🟡 | 🟢 | bloc N2-ordonné @base+271+ ; oracle SteelStrng `val(0x05,0x02)==16`, `val(0x05,0x03)==13` (Tests.h:249-250) |
| **PEG switch (FYPSW)** | 🟡 | 🟡 | chargé param 0x0C (SysexUtils:213) ; pas d'oracle |
| Pitch EG levels (o/b) | 🟡 | ⬜ | repr. éditeur 0..64 à aligner sur o/b ; NON chargé |
| LFO 1 / LFO 2 | ⬜ | ⬜ | ni chargé ni câblé éditeur |

## AFM — Opérateurs (élément 1, OP1→OP6)

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| EG rates R1-R4 | ✅ | 🟢 | aller-retour fingerprint confirmé |
| EG rates RR1, RR2 | ✅ | 🟢 | |
| EG levels L0-L4 | ✅ | 🟢 | |
| EG levels RL1, RL2 | ✅ | 🟢 | |
| **Niveau de sortie (TL)** | ✅ | 🟢 | Track A + aller-retour (6/6) |
| Fine | ✅ | 🟢 | aller-retour @interne 44 (6/6) ; oracle `val(0x56,0x26)==46` (Tests.h) |
| **Coarse (FPC)** | 🟡 | 🟢 | chargé interne 43 / param 0x25 (SysexUtils:179) ; **oracle SteelStrng** `val(0x56,0x25)==1` (Tests.h:228) |
| **Waveform (PWAVE)** | 🟡 | 🟢 | chargé interne 24 / param 0x17 (SysexUtils:180) ; **oracle SteelStrng** `val(0x56,0x17)==15` (Tests.h:229) |
| **Detune (FPD)** | 🟡 | 🟡 | chargé interne 28 / param 0x1A (SysexUtils:181) ; **pas d'oracle** (encodage négatif s/m non levé) |
| **SENSIT (VEL/AM)** | 🟡 | 🟡 | chargé param 0x11/0x10 (SysexUtils:184), offsets bulk ESTIMÉS ; pas d'oracle |
| **SCALING (BP1-4 + offset-levels)** | 🟡 | 🟡 | chargé param 0x1C-1F / 0x20-23 (SysexUtils:186-187) ; pas d'oracle |
| **VEL SW (RVSW)** | 🟡 | 🟡 | chargé param 0x24 (SysexUtils:188) ; pas d'oracle |
| RS / Slope / HT | ❓ | ⬜ | interne 15 = constante dans le dump → offset à revoir |
| Freq MODE (Fixed/Ratio, PM 0x18) | ⬜ | ⬜ | NON chargé : packé avec SENSIT → bit-split à faire (SysexUtils:184) |
| PHASE / SYNC (0x19) | ⬜ | ⬜ | NON chargé : octet packé phase+sync → extraction de bits à faire (SysexUtils:189) |

## Filtres (filtre 1 & 2, élément AFM)

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| Type filtre 1 (FTYPE : LPF/HPF/Thru) | 🟡 | ❓ | envoi câblé ; dump @403 (offset OK) mais encodage bulk ambigu (Thru→LPF = 0→1, ≠ enum) — besoin d'un point HPF. NON chargé |
| **Mode de contrôle (FMODE) f1 & f2** | 🟡 | 🟢 | dump @base+298/+327 ; oracle SteelStrng `val(0x09,0x02)==2` (Tests.h:251) |
| **EG filtre 1 & 2 : rates (FR1-4, FRR1-2)** | 🟡 | 🟢 | dump @base+296/+325+N2 ; oracle SteelStrng `val(0x09,0x03)==11` (Tests.h:252) |
| EG filtre : niveaux (FL0-4, FRL1-2, o/b) | 🟡 | ⬜ | repr. éditeur à aligner sur o/b ; NON chargé |
| **Cutoff filtre 1** | 🟡 | 🟢 | dump **@404 confirmé** (diff single-param 127→0) ; oracle `val(0x09,0x01)==27` (Tests.h:236) |
| **Cutoff filtre 2** | 🟡 | 🟢 | dump @433 ; oracle SteelStrng `valH(0x09,1,0x01)==50` (Tests.h:246) |
| **Résonance** | 🟡 | 🟢 | dump @461 ; oracle SteelStrng `valH(0x09,2,0x32)==59` (Tests.h:247) |
| Filter EG : niveaux/slope (R1-4, RR1, L0-4, RL1-2) | 🟡 | ⬜ | EG câblé group 0x09 (Filter1/2.h) ; niveaux NON chargés (o/b) |

## Effets

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| Effect mode, Chorus 1/2, Reverb 1/2 | ❓ | ⬜ | adresses « plausibles » (group 0x08), non vérifiées |

## Éléments 2-4 (AFM) — adressage par élément

Le loader traite **tous** les éléments (1-4), AFM **et** AWM, addrHi = élément<<5 (SysexUtils:195-244).
Les paramètres par élément ont donc le même statut que pour l'élément 1 (cf. tableaux ci-dessus) ;
seul l'addrHi change. Vérifié par oracle sur 2/4 AFM (`Tests.h` : addrHi 0x20 & 0x60 produits) et
sur le type mixte 8.

## AWM (éléments AWM, group 0x07)

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| **Waveform (AWMWAVE)** | 🟡 | 🟢 | dump @base+2 (low7) ; **oracle TARKUSCYMB** `val(0x07,0,0x01)==105` (Tests.h:313) |
| **Amp-EG (PAR1-4, PARR1, PAL2-3)** | 🟡 | 🟢 | dump @base+89.. param 0x50-0x56 ; **oracle TARKUSCYMB** (Tests.h:317-323) ; câblé WaveEg mode AWM |
| Niveau élément (ELVL) | 🟡 | 🟢 | commun, cf. Voice Common ; oracle TARKUSCYMB `val(0x03,0,0x00)==127` (Tests.h:314) |
| Fine / Fixed / PARS (s/m) / filtre AWM | 🟡 | ⬜ | encodage à lever → NON chargé (filtre AWM câblé édition seulement) |

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
