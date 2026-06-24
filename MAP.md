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

Couverture actuelle du chargement = **voix 1 AFM POLY (type 3), élément 1 uniquement**.

---

## Voice Common

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| Nom de voix | 🟡 | 🟢 | offsets 33-42 ; envoi spec-correct, hardware à confirmer |
| Type / mode de voix | 🟡 | 🟢 | offset 32 |
| Volume voix | 🟡 | ⬜ | group 0x02 / param 0x3F |
| Pan, niveaux d'envoi (rev/cho/var) | 🟡 | ⬜ | — |

## AFM — Élément commun (élément 1)

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| **Algorithme (ALGNUM)** | 🟡 | 🟢 | dump **@377 confirmé** (diff single-param 16→1) ; envoi via l'app pas encore testé |
| Pitch EG | 🟡 | ⬜ | group 0x05 |
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
| Type (FTYPE : LPF/HPF/Thru) | ✅ | ⬜ | Track A |
| Mode de contrôle (FMODE) | ✅ | ⬜ | Track A (filtre 1 & 2) |
| Cutoff | 🟡 | ⬜ | group 0x09 |
| Résonance | 🟡 | ⬜ | |
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
