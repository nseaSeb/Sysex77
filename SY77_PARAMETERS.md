# SY77 / SY99 — Référence sysex (source de vérité)

Backbone des messages sysex de l'éditeur, **croisé entre le code existant (valeurs
validées car l'app fonctionne) et la spec officielle** Yamaha *« SY77 MIDI Data Format »*
(`docs/sy77midi.pdf`, hors-repo). Sert de référence unique pour :
centraliser la construction sysex (un seul *builder*), valider/compléter chaque contrôle,
et lever proprement les FIXME (ex. voice-name).

> Périmètre : **presets (édition de voix) + banques**. Hors périmètre : séquenceur, multi/
> performance (groupes 00/01 Multi, 04 Drum, 0E Sequencer ignorés sauf besoin).

---

## 1. Format des messages

### 1.1 Parameter change (modèle `0x34`)
```
F0  43  1n  34  G  AH  AL  P  DH  DL  F7
```
| Octet | Rôle |
|------|------|
| `F0` / `F7` | début / fin sysex |
| `43` | fabricant Yamaha |
| `1n` | **device number** : `n = deviceNumber - 1` (1..16). Octet = `0x10 \| (n & 0x0F)`. Voir `SyVoice::deviceByte`. |
| `34` | modèle / format « parameter change » SY77 |
| `G` | **groupe** (table §2) |
| `AH AL` | adresse (encodage selon groupe, §2) |
| `P` | numéro de paramètre |
| `DH DL` | valeur : 1 octet (`DH=0`, `DL=valeur`) ou 2 octets (`DH=MSB`, `DL=LS7bits`) selon le paramètre |

> ⚠️ L'octet `1n` est désormais imposé à l'émission par le device global
> (`sysexDeviceNumber`) aux 2 *choke points* `MidiSysex.h` (`/SYSEX` + `sendSysex`) et aux
> envois directs. Ne pas re-coder le canal en dur ailleurs.

### 1.2 Bulk dump (banques)
```
F0  43  0n  7A  <count MSB>  <count LSB>  "LM  8101xx"...  <data...>  <checksum>  F7
```
9 types de bulk voix : 1/2/4-AFM, 1/2/4-AWM, combos AFM+AWM, Drum-set.
`Memory_type` : `7F` = edit buffer, `00` = INTERNAL, `02/03` = PRESET1/2.
Checksum = complément à 128 de la somme des octets de data → `SyVoice::yamahaChecksum`.

---

## 2. Groupes et encodage d'adresse

`*` = dans le périmètre éditeur de voix. `ee` = numéro d'élément (0..3) encodé `0ee00000`
en `AH` (soit `0x00, 0x20, 0x40, 0x60`).

| `G` | Données | `AH` / `AL` | Statut |
|----:|---------|-------------|:------:|
| `00` | Multi common (nom multi, ascii) | `00 00` | hors |
| `01` | Multi channel | `00`, n° canal | hors |
| `02` | **Voice common** | `00 00` | ✅* |
| `03` | **Voice element** | `AH = ee<<5`, `AL = 00` | ✅* |
| `04` | Drum set | n° note MIDI | hors |
| `05` | **AFM element common** / **operator enable** | `AH = ee`, `AL = 00` (enable : `7F 7F`) | ✅* |
| `(op<<4)\|06` | **AFM operator** (op 0..5 = **OP6..OP1**) → `06 16 26 36 46 56` | `AH = ee`, `AL = 00` | ✅* |
| `07` | **AWM element** | `AH = ee`, `AL = 00` (valeur 2 octets MSB/LS7) | ✅* |
| `08` | **Effector** (effets) | `00 00` | ✅* |
| `09` | **Filter** | `AH = 0ee00fff` (fff = n° filtre) | ✅* |
| `0A` | **Pan** (dynamique) | `0A`, n° mémoire | ✅* |
| `0B` | Micro tuning | n° mémoire | (option) |
| `0D` | Switch Remote (simulate appui façade) — **réception seule** | — | ✅ (utilisé) |
| `0F` | **System setup** (foot/mod, bulk protect…) | `00 00` | ✅* |
| `04` + `P=40` | Master tuning | — | (option) |

---

## 3. Inventaire actuellement implémenté (récolté dans le code)

Adresses **validées** (présentes dans une app fonctionnelle). Beaucoup de fichiers
construisent un gabarit puis fixent `AH/AL/P` à l'exécution (par élément / opérateur /
rang d'EG) — voir le fichier indiqué pour la logique de calcul.

| Groupe | Paramètre (P) | Sens | Fichier(s) |
|-------|--------------|------|-----------|
| `02` | `3F` | Volume total de la voix | Voice.h |
| `02` | `01` | (voice common, ex. nom — cf. FIXME voice-name) | Voice.h |
| `03` | `00` groupe / `01` fine / `02` pitch / `08` … (par élément `ee<<5`) | Pitch/pan/groupe d'élément | Element.h, Pitch.h, MidiObjects.h, MidiSysex.h |
| `05` | … | AFM element common | Operator.h |
| `46`,`56` (`(op<<4)\|06`) | `26`, `17` … | AFM operator (OSC) | Oscillator.h |
| `07` | `01`, `50` … | AWM element (waveform, volume…) | AWMVue.h, Volume.h |
| `09` | `09`, `00` … (filtre, par élément) | Filtre | Filter1.h, Filter2.h, CommonFilter.h |
| `0D` | `32` | Switch remote (appui façade simulé) | Voice.h |
| `0F` | `2c` mod / `2d` foot / `34` bulk protect | System setup | MidiDemo.h, MidiSysex.h |
| `00` (EG) | gabarits EG (rang fixé via `sysexdata[6]`) | Enveloppes (pitch/wave/filtre) | PitchEG.h, WaveEg.h, Filter*.h |

> Les valeurs détaillées par paramètre (plages, n° P exacts par contrôle) se trouvent dans
> les **tables 1-x de `sy77midi.pdf`** (pages 6+). Elles seront transcrites **au fil de l'eau**
> lors de la centralisation (Étape 2) et de la complétude (Étape 4), chaque ajout étant
> vérifié par build + comportement — on ne transcrit pas en masse pour éviter toute erreur
> de format vers le matériel.

---

## 4. Manques pour une édition de voix complète (gap vs spec — pilote l'Étape 4)
- AFM : 6 opérateurs × (les non-op `05` + operator `*6` + enable) pour les 4 éléments.
- AWM (`07`) : jeu de paramètres complet par élément.
- Filtre (`09`) : les deux filtres + EG de filtre par élément.
- Effets (`08`) : algorithmes + paramètres.
- Pan dynamique (`0A`), routage / algorithme (matrice façade SynthWorks).
- Voir captures `docs/synthwork sy77 voices.jpg` pour l'agencement cible.
