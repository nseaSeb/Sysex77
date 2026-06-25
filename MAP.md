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
**FOS filtre (o/b 2 octets, layout bulk à confirmer)** ; effets ; LFO (chargement dump).
**Résolus 2026-06-25** : detune AFM s/m (envoi+affichage), coarse (offset retiré), octets packés
0x18 (MODE/PES/PM recomposés) et 0x19 (SYNC/PHASE recomposés). Niveaux EG pitch/filtre o/b : résolus (lua).

> **Boucle d'écho hardware (CORRIGÉE 2026-06-25)** : sur SY77 réel, l'app ré-émettait en
> boucle les param-changes ÉCHOÉS par le synthé (oscillation A↔B incluant des params non
> touchés). **Cause** : (1) le `valueSysexIn` reçu était notifié en ASYNCHRONE — l'application
> aux widgets se faisait HORS de tout garde ; (2) `ComboBox::valueChanged` (JUCE) re-publie via
> `setSelectedId(... sendNotification)` quand son currentId lié change. **Fix** : garde anti-écho
> ATOMIQUE et RÉENTRANT (`ScopedEchoSuppress`, `SysexBus::suppressSend`) posé autour de
> l'APPLICATION RX (notification rendue SYNCHRONE dans `MidiDemo::handleAsyncUpdate`), qui
> SUPPRIME tout envoi widget→bus pendant l'application ; le replay de voice-load est aussi sous
> garde (plus de flot non sollicité vers le synthé). Remplace l'ancien `boolStopReceive` global/
> non thread-safe. Couvert par `Tests.h` (suite **EchoLoop**, 3 tests). Choke-point unique :
> `SysexBus::publish`.

---

## Voice Common

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| Nom de voix | 🟡 | 🟢 | offsets 33-42 ; envoi spec-correct, hardware à confirmer |
| Type / mode de voix | 🟡 | 🟢 | offset 32 |
| **Volume voix** | 🟡 | 🟢 | dump @95 ; group 0x02 / param 0x3F ; oracle SteelStrng `val(0x02,0x3F)==60` (Tests.h:238) |
| **Niveau élément (ELVL)** | 🟡 | 🟢 | dump @98+9·e ; group 0x03 / param 0x00 ; oracle SteelStrng `val(0x03,0x00)==1` (Tests.h:239) |
| **On/off élément (1 2 3 4)** | 🟡 | — | **CÂBLÉ (A)** : boutons « Operateurs On-Off » (Voice.h) = on/off des 4 ÉLÉMENTS via **MUTE éditeur** sur ELVL (pas de vrai param on/off dans la spec — seul ELVL existe, OCR l.281). OFF = mémorise l'ELVL courant puis force 0 (émis par sliderVolume = group 0x03 / param 0x00) ; ON = restaure. `Element::setElementMuted`. Boutons des éléments hors mode courant grisés/désactivés (source = `nombreElements`). Envoi au CLIC seulement ; non vérifié hardware |
| **Pan — sélecteur de table (PANNM, 0x07)** | 🟡 | ⬜ | **CÂBLÉ (#P)** : colonne PAN = sélecteur de table de pan (façon SynthWorks « PAN JOBS »), édité dans l'overlay `PanEg`. Envoi group 0x03 / param 0x07 / T2=(el-1)<<5, valeur 0..95. Propriété `ELEMENT<n>PANNM` ; la carte (Element.h) n'affiche que la table (clic→éditeur). Spec OCR l.290. Envoi au CLIC ; **non vérifié hardware**. Libellés 0-7 = amorce SynthWorks (mapping index↔nom **à confirmer**) ; offset bulk PANNM **inconnu** (chargement dump non branché) |
| **Pan — EG de table (group 0x0A)** | 🟡 | ⬜ | **CÂBLÉ (#P)** : éditeur `PanEg`/`PanVue` (overlay, 1/élément). group 0x0A / T2=table PANNM / N2 : PNSCSEL 0x00, PNSCDPT 0x01, PNDT(hold) 0x02, PNR1-4 0x03-06, PNRR1-2 0x07-08, PNL0-4 0x09-0D, PNRL1-2 0x0E-0F, PNSLP 0x10, nom PNNAM0-9 0x11-1A. Niveaux o/b **offset 32** (`panLevelTo*`, Tests.h, centre octet 32 — distinct du 64 des EG pitch/filtre). Spec OCR l.543-584. **T2 (=mémoire de table), centre o/b, longueur nom (10 vs 11), codes PNSCSEL, sens L/R : NON vérifiés HW** (à lever par diff single-param). Table = ressource GLOBALE (INT 1..32) présentée par élément ; pas dans le dump de voix (dump Pan séparé non parsé) |
| **Groupe de sortie (OUTSEL, 0x08 b1/b2)** | 🟡 | ⬜ | **CÂBLÉ (#2)** : btGroup1/btGroup2 (Element.h) émettent l'octet PACKÉ group 0x03 / param 0x08 par élément (addrHi=(el-1)<<5). b1=OUTSEL0 (grp1), b2=OUTSEL1 (grp2), b0=MCTEN laissé 0. Avant : « rendu/état seul » → aucun envoi (les boutons ne faisaient rien). Spec OCR l.292-294 + carte `out sel@08`. Envoi au CLIC seulement ; non vérifié hardware |

## AFM — Élément commun (élément 1)

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| **Algorithme (ALGNUM)** | 🟡 | 🟢 | dump **@377 confirmé** (diff single-param 16→1) ; oracle `val(0x05,0x00)==23` (Tests.h:234) |
| **Pitch EG rates (FPR1-3, FPRR1)** | 🟡 | 🟢 | bloc N2-ordonné @base+271+ ; oracle SteelStrng `val(0x05,0x02)==16`, `val(0x05,0x03)==13` (Tests.h:249-250) |
| **PEG vel switch (FYPSW, élément)** | 🟡 | 🟡 | chargé param 0x05/0x0C ; **= « Velocity Switch » du pitch-EG d'élément** (1 par élément, spec OCR l.356), PAS le « PEG SW » par-op de la grille. Pas d'oracle |
| **Pitch EG levels (FPL0-3, FPRL1, o/b)** | 🟡 | 🟢 | **chargés** group 0x05 N2 0x05-0x09 (SysexUtils, `voiceBlobToParams`). Encodage o/b offset 64 (`egLevelToDisplay`/`egLevelToWire`) **recoupé codec lua indépendant** (main.lua l.22 + TG77_Voice.json pNum 7005-7008 : display{-64,63}/message{0,127}/default 64). Éditeur aligné 0..64→0..127 (PitchEG.h) → corrige l'**envoi** (atteignait ½ plage). Oracle SteelStrng `val(0x05,0x05)==55` (Tests.h). NON vérifié hardware |
| **Main LFO** (Speed/Delay/PMD/AMD/FMD/Wave/Phase) | 🟡 | ⬜ | éditeur câblé (`AfmLfo.h`) ; group 0x05 N2 0x0D-0x13 (Table 1-6, OCR l.357-364) ; non vérifié hardware ; chargement dump non branché |
| **Sub LFO** (Wave/Speed/Mode/Time/PMD) | 🟡 | ⬜ | éditeur câblé (`AfmLfo.h`) ; group 0x05 N2 0x15-0x19 (Table 1-6, OCR l.366-371) ; Mode = delay/decay (bouton 2 états) ; non vérifié hardware ; chargement dump non branché |

## AFM — Opérateurs (élément 1, OP1→OP6)

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| EG rates R1-R4 | ✅ | 🟢 | aller-retour fingerprint confirmé |
| EG rates RR1, RR2 | ✅ | 🟢 | |
| EG levels L0-L4 | ✅ | 🟢 | |
| EG levels RL1, RL2 | ✅ | 🟢 | |
| **Niveau de sortie (TL)** | ✅ | 🟢 | Track A + aller-retour (6/6) |
| **OP on/off (mute éditeur)** | ✅* | n/a | *Pas de paramètre « operator on/off » dans la spec AFM (Table 1-7, OCR l.418-444) — comme sur DX7. Implémenté comme **MUTE éditeur via TL** (param 0x1B, déjà ✅) : interrupteurs `opOn[6]` dans `Oscillator.h` — OFF mémorise le niveau (`savedLevel[i]`) puis force la Value de `sliderLevel*` à 0 (envoi TL=0 via le wiring existant) ; ON restaure. Aucune adresse nouvelle. Mute LOCAL (un rechargement de voix remet les niveaux réels → re-cliquer pour re-muter) |
| Fine (FPF, 0x26) | 🟢 | 🟢 | **CORRIGÉ plage** : colonne FINE (`sliderFreqFine*`) bornée **0..127** (octet brut, display==wire) au lieu de 0..99 — la carte TG77 vérifiée hardware (`FINE OP*`, pNum *038=0x26) borne 0..127. L'ancien 0..99 rendait 100..127 inatteignables et faussait la barre (#3). Chargement : oracle dump `val(0x56,0x26)==46` (Tests.h) |
| **Coarse (FPC)** | 🟢 | 🟢 | **CORRIGÉ** : envoi octet brut 0..127, AUCUN offset (retrait du `setMidiValueOffset(-1)` qui rendait display 0 → wire 127). Aligné sur l'éditeur TG77 vérifié hardware (`COARSE OP*` display==message 0..127). Oracle dump `val(0x56,0x25)==1` |
| **Waveform (PWAVE)** | 🟡 | 🟢 | chargé interne 24 / param 0x17 ; **oracle SteelStrng** `val(0x56,0x17)==15` |
| **Detune (FPD)** | 🟢 | 🟢 | **CORRIGÉ** : encodage s/m activé (slider `setRangeAndRound(-15,15)` → chemin boolNegative). Pures `fpdDetuneToWire/Display` (SysexUtils) + test (Tests.h, half=15 signbit=16). Provenance hardware : main.lua l.45 `SM[*+0x1A]={15,16}` + OCR l.438. La trace SY77 (wire 29/30) = display -13/-14, désormais affichée/envoyée correctement |
| **SENSIT (VEL/AM)** | 🟡 | 🟡 | chargé param 0x11/0x10, offsets bulk ESTIMÉS ; pas d'oracle |
| **SENSIT PM (FMLPMS, 0x18 b4~2)** | 🟢 | ⬜ | **CORRIGÉ** : recompose l'octet packé 0x18 (auto-send OFF, plage 0..7) au lieu d'envoyer 0..31 brut qui écrasait MODE/PES. Spec OCR l.431 |
| **SCALING (BP1-4 + offset-levels)** | 🟡 | 🟡 | chargé param 0x1C-1F / 0x20-23 ; pas d'oracle |
| **VEL SW (RVSW)** | 🟡 | 🟡 | chargé param 0x24 ; pas d'oracle |
| RS / Slope / HT | ❓ | ⬜ | interne 15 = constante dans le dump → offset à revoir |
| **Freq MODE (Fixed/Ratio, FPM 0x18 b0)** | 🟢 | ⬜ | **CORRIGÉ collision 0x18** : MODE/PES/PM recomposent désormais l'octet 0x18 complet (`Oscillator::send18`, lit l'état des 3 widgets → robuste). Spec OCR l.433. Chargement dump non branché (octet packé) |
| **PHASE / SYNC (0x19)** | 🟢 | ⬜ | **CORRIGÉ** : V1 b0 = KOE/SYNC (btPhase), V2 = PHASE 0..127 (sliderPhase) recomposés en un frame (`Oscillator::send19`). Spec OCR l.435-436. Auparavant sync et phase s'écrasaient (et copier-coller `sliderFine4` corrigé). Chargement dump non branché |
| **PEG SW par-op (PES, 0x18 b1)** | 🟢 | ⬜ | **CORRIGÉ** : la grille « PEG SW » (6 boutons) = Pitch-EG Switch PAR OPÉRATEUR (spec OCR l.432), recompose l'octet 0x18 b1 (`Oscillator::send18`). Avant : câblé sur FYPSW (group 0x05/0x0C, switch d'ÉLÉMENT) avec une propriété PARTAGÉE → 6 boutons/1 état/aucun effet. Maintenant 6 états distincts, propriété `ELEMENT<e>PEGSW<o>` |

## Filtres (filtre 1 & 2, élément AFM)

| Paramètre | Envoi → synthé | Ouverture dump | Notes |
|---|:---:|:---:|---|
| Type filtre 1 (FTYPE : LPF/HPF/Thru) | 🟡 | ❓ | envoi câblé ; dump @403 (offset OK) mais encodage bulk ambigu (Thru→LPF = 0→1, ≠ enum) — besoin d'un point HPF. NON chargé |
| **Mode de contrôle (FMODE) f1 & f2** | 🟡 | 🟢 | dump @base+298/+327 ; oracle SteelStrng `val(0x09,0x02)==2` (Tests.h:251) |
| **EG filtre 1 & 2 : rates (FR1-4, FRR1-2)** | 🟡 | 🟢 | dump @base+296/+325+N2 ; oracle SteelStrng `val(0x09,0x03)==11` (Tests.h:252) |
| **EG filtre 1 & 2 : niveaux (FL0-4, FRL1-2, o/b)** | 🟡 | 🟢 | **chargés** group 0x09 N2 0x09-0x0F des 2 filtres (SysexUtils). Encodage o/b offset 64 recoupé codec lua (main.lua l.22 + TG77_Voice.json pNum 7309-7315). Éditeur aligné 0..64→0..127 (Filter1/2.h) → corrige l'**envoi**. Oracle SteelStrng : FL0 filtre 1 == octet **64 == centre o/b par défaut** (`val(0x09,0x09)==64`, Tests.h). NON vérifié hardware |
| EG filtre : FOS (break-point offsets, o/b 2 octets) | 🟡 | ⬜ | encodage param-change connu (lua OB2 combined=msg+1, V1=bit7/V2=low7) mais **layout BULK non confirmé** (octets @424-427 = 1,0,1,0, MSB/interleave ambigu) → NON chargé |
| **Cutoff filtre 1** | 🟡 | 🟢 | dump **@404 confirmé** (diff single-param 127→0) ; oracle `val(0x09,0x01)==27` (Tests.h:236) |
| **Cutoff filtre 2** | 🟡 | 🟢 | dump @433 ; oracle SteelStrng `valH(0x09,1,0x01)==50` (Tests.h:246) |
| **Résonance** | 🟡 | 🟢 | dump @461 ; oracle SteelStrng `valH(0x09,2,0x32)==59` (Tests.h:247) |
| Filter EG : rates/slope (R1-4, RR1-2, slope) | 🟡 | 🟢 | EG câblé group 0x09 (Filter1/2.h) ; rates chargés (cf. ligne rates ci-dessus) ; niveaux désormais o/b chargés (ligne dédiée) |

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
| **PPM (Fixed mode, 0x02)** | 🟡 | ⬜ | câblé `btFixed` (AWMVue.h) group 0x07 N2 0x02 ; spec OCR l.487 ; non vérifié hardware |
| **PNOTE (fixed note#, 0x03)** | 🟡 | ⬜ | **CÂBLÉ (#E)** : `sliderNote` (AWMVue.h) group 0x07 N2 0x03, plage 0~127, octet brut (display = nom de note C-2..G8 via `sy77NoteName`). Grisé hors mode Fixed (observe `btFixed`). Propriété `ELEMENT<n>PNOTE`. Adresse DOCUMENTÉE (OCR l.488) ; **envoi non vérifié hardware** |
| **PPF (fine, 0x04, o/b)** | ⚠️ | ⬜ | câblé `sliderFine` (AWMVue.h) N2 0x04, mais via le chemin **boolNegative/boolInvert** (s/m-like) alors que la spec dit **offset-binary** (`-64~+63 (o/b)`, OCR l.489) → encodage de l'envoi à CONFIRMER hardware (#4). Non modifié (pas de devinette) |

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
