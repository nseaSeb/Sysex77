# Sysex77 — Roadmap & carnet de bord

Éditeur MIDI pour Yamaha SY77/TG77, inspiré de **SynthWorks (Atari)**.
Document de pilotage tenu par le « chef de projet ». Voir aussi `MAP.md` (couverture
paramètre par paramètre) et `Source/SysexUtils.h` (logique pure + conventions).

## Équipe d'agents (`.claude/agents/`)
- **sysex-specialist** — format Sysex, décodeur de dump, carte d'offsets, tests purs.
- **midi-specialist** — transport (SysexBus, I/O réel), câblage envoi↔réception, device number.
- **ui-specialist** — vues JUCE, fidélité SynthWorks + ergonomie, LookAndFeel/thèmes.
- **architecte** — plans des gros chantiers (lecture seule, ne code pas).
- **Chef de projet** = la session principale : découpe, délègue, recolle, vérifie la cohérence.

## Contrat partagé (source unique de vérité)
- `MAP.md` = état vérifié (✅ hardware / 🟢 dump / 🟡 câblé non vérifié / ❓ douteux / ⬜ absent).
- Format filaire param-change = `SyVoice::paramBytes/paramMessage` (device via `deviceByte`).
- Toute logique pure → `SysexUtils.h` + test dans `Tests.h`. Build/test : `./build.sh --debug --test`.
- **Fiabilité d'abord** : ne jamais exposer/charger un paramètre dont l'adresse n'est pas prouvée.

---

## État des lieux (audit 2026-06-24)

### Constats transverses (les 3 agents convergent)
1. ~~**`MAP.md` est PÉRIMÉ.**~~ **RESYNC FAIT (0.1).** Coarse/Waveform 🟢 (oracle SteelStrng),
   Detune/SENSIT/SCALING/VEL-SW 🟡 (chargés sans oracle), section AWM ajoutée (waveform + amp-EG
   🟢 oracle TARKUSCYMB), éléments 2-4 = même statut que él. 1 (addrHi près).
2. **Le bug EG group-byte 0x00 est RÉSOLU** (WaveEg 0x06, PitchEG 0x05, Filter 0x09).
   **Filet de test posé (0.2)** : `SyVoice::egGroupFor` (table pure) consommée par les 4 éditeurs
   + test anti-régression. Verrouille le **group byte** émis (pas encore les offsets de param).
3. **Format filaire dupliqué.** Les ~205 widgets `Midi*` n'utilisent PAS le builder testé
   `paramBytes` ; ils fabriquent un `sysexData[9]` brut (MidiObjects.h, Hook.h, MidiDemo.h).
   Le device est correct (réécrit au choke-point) mais par deux mécanismes différents.

### Sysex (décodeur / offsets)
- Solide : AFM ops (19 params/op), ALGNUM, pitch-EG rates, filtres FCTOF/FMODE/rates. Validé
  offline sur 811 voix (0 hors plage). Tests sur 2 vrais dumps oracles (SteelStrng, TARKUSCYMB).
- Trous : ~~pitch-EG **levels** (o/b)~~ **RÉSOLU (2.2, recoupé lua)**, Main/Sub-LFO, FTYPE (bulk≠enum), PHASE/SYNC packés,
  AWM fine/fixed/filtre, effets (chargement), **drum type 10** (~413 voix non couvertes).
- Tests : aucun test d'**encodage d'envoi** (le côté TX des widgets) ; 2/4 AFM testés sur blobs
  zéro-remplis seulement.

### MIDI (transport / câblage)
- Transport solide : SysexBus in-process, ports in/out multi-sélectionnables + persistés,
  device number 1..16 + omni centralisés.
- Trous : **pas de Dump Request émis** (capture 100 % manuelle côté synthé, sans checksum validé) ;
  envoi de banque en un seul `MidiMessage` (risque saturation). ~~**Hook.h émet sans écouter**
  (TX sans RX)~~ **RÉSOLU (0.3)** ; ~~`boolStopReceive` non thread-safe et redondant~~ **RETIRÉ (0.4)**.

### UI (vues / ergonomie)
- Abouti : écran Voice (grille 4 éléments + routage dessiné), grille AFM 6 opérateurs façon
  SynthWorks + zoom par op, filtres, AWM, effets. Édition souris (cutoff/réso, nœuds EG),
  rendu FM approximé, ModernLookAndFeel + 5 thèmes + Theme Builder.
- Trous : ~~topologie des 45 algos non dessinée~~ **FAIT (3.1)** — liens modulateur→porteuse tracés
  dans `AlgoDraw` (vue AFM) + glyphe compact par élément AFM dans la carte ALGO/ROUTAGE de Voice ;
  pas d'éditeur **LFO** ; pas de **drum set** ; voice-common incomplet (microtuning, portamento…) ;
  sliders verticaux étroits illisibles (pan/volume 16-24 px) ; code mort (`SADSR`,
  `sendChangeMessage()` dans `paint()`) ; couleurs en dur échappant au thème.

---

## Backlog priorisé

### Vague 0 — Stabiliser les fondations (avant toute feature)
| # | Tâche | Agent | Effort | Critère de fini |
|---|---|---|---|---|
| 0.1 | ~~Resynchroniser `MAP.md` avec le code réel~~ **FAIT** | sysex | bas | MAP.md resync : Coarse/Waveform 🟢 (oracle), Detune/SENSIT/SCALING 🟡, AWM section ajoutée |
| 0.2 | ~~Tests d'encodage d'envoi (régression EG)~~ **FAIT (group byte)** | sysex | bas | `egGroupFor` pure (SysexUtils.h) consommée par les 4 éditeurs + test `egGroupFor locks…` (118/118). NB : verrouille le **group**, pas encore les offsets de param |
| 0.3 | ~~Brancher `Hook.h` sur `valueSysexIn` (RX)~~ **FAIT** | midi | bas | `Segment` écoute `valueSysexIn` (Hook.h) ; match des DEUX adresses (rate via `sysexRate`, level via `sysexLevel`) et route vers le bon axe via `setValue()` (sans renvoi). Les EG via Hook se resynchronisent depuis le synthé (118/118) |
| 0.4 | ~~Supprimer `boolStopReceive`~~ **FAIT puis RÉVISÉ (2026-06-25)** | midi | bas | Flag non thread-safe retiré. `dontSendNotification` seul s'est révélé INSUFFISANT sur hardware (boucle d'écho réelle) : `ComboBox::valueChanged` re-publie, et la notif RX async sortait du garde. Remplacé par un garde ATOMIQUE/RÉENTRANT `ScopedEchoSuppress`+`SysexBus::suppressSend` autour de l'application RX (rendue synchrone) — cf. Journal 2026-06-25 #1. Tests `EchoLoop` (698/698) |

### Vague 1 — Fiabiliser l'aller-retour synthé
| # | Tâche | Agent | Effort | Critère de fini |
|---|---|---|---|---|
| 1.1 | ~~Émettre un vrai **Dump Request** + valider checksum à la réception~~ **FAIT** | midi+sysex | moyen | `SyVoice::voiceDumpRequest` (format VCED `F0 43 2n 7A "LM  8101VC" … Memory_type Memory# F7`, identifiant OCR octets 14/15=V,C + confirmé hors-repo) émis par le bouton RECEIVE (user-triggered, boucle 64 mémoires internes + throttle 10 ms). Checksum vérifié au save via `SyVoice::verifyYamahaBulkChecksum` (pure + test) ; blocs invalides signalés (log + info-line rouge), capture brute conservée (152/152). NB : framing documenté ≤32 o., **non vérifié hardware** |
| 1.2 | ~~Découper l'envoi de banque en messages F0…F7 + throttle~~ **FAIT** | midi | bas | `SyVoice::splitSysexMessages` (pure + test) consommée par `sendBankThrottled` (MidiSysex.h) : la banque part message-par-message avec délai inter-message ~10 ms (calque l'outil RE), plus d'unique gros `MidiMessage`. Fallback `sendRaw` si pas de cadrage F0…F7 (152/152) |
| 1.3 | ~~Unifier les widgets sur `paramBytes`/`paramMessage`~~ **FAIT** | midi+sysex | moyen | Le format filaire d'envoi est centralisé : nouveau `SysexBusSender::sendParam9(addr, uint8[9])` (SysexBus.h) consommé par les 4 widgets Midi* (MidiSlider — broadcast multi-groupes inclus —, MidiButton, MidiRadio, MidiCombo, + leurs outils RE) et par `Hook.h` ; plus aucun `send(addr, b0..b8)` recopié à la main. Les littéraux `{0x43, deviceByte, 0x34, …}` de MidiDemo.h (btBulk/comboFoot/comboMod) et la table OSC de MidiSysex.h passent désormais par le builder testé `SyVoice::paramBytes`. Device inchangé : le choke-point (`oscMessageReceived`/`sendSysex`) reste l'unique autorité qui réécrit l'octet [1]. Preuve : test anti-régression `paramBytes is byte-identical to the legacy inline format` (Tests.h) compare octet-pour-octet l'ancien `{0x43,0x10,0x34,…}` inline au builder sur 7 cas (215/215). NB : per-widget `setMidiSysex(int[9])` des éditeurs conserve son tableau opaque (placeholder device 0x10) — non concerné, byte-identique. |

### Vague 2 — Combler les offsets manquants (chacun avec preuve)
| # | Tâche | Agent | Preuve |
|---|---|---|---|
| 2.1 | FTYPE (lever bulk↔enum) | sysex | aller-retour hardware (un point HPF) |
| 2.2 | ~~Niveaux EG en offset-binary (pitch-EG levels, filtre FL0-4/FRL1-2)~~ **FAIT (recoupé lua)** | sysex | `egLevelToDisplay`/`egLevelToWire` (SysexUtils.h, o/b offset 64) recoupés au codec lua INDÉPENDANT (main.lua l.22 + TG77_Voice.json PEG/FEG L : display{-64,63} message{0,127} default 64). Chargés dans `voiceBlobToParams` (FPL0-3+FPRL1 group 0x05 ; FL0-4+FRL1-2 group 0x09 des 2 filtres). Éditeurs alignés sur la plage filaire réelle 0..127 (centre 64) — PitchEG/Filter1/Filter2 : était 0..64 (atteignait ½ plage) → corrige aussi l'ENVOI ; affichage signé -64..+63 via `applyEgLevelDisplay`. Tests : inversibilité full-range + valeurs-ancres lua + oracle SteelStrng (FL0==64=centre) (635/635). **FOS (2 octets) NON traité** (layout bulk non confirmé). NB validation 811-voix NON reproductible (corpus hors-repo) — preuve = lua + oracles. |
| 2.3 | ~~PHASE/SYNC bit-split~~ + ~~collision 0x18~~ + ~~detune s/m~~ + ~~coarse~~ **FAIT (2026-06-25, ENVOI)** | sysex | Bugs hardware réels remontés (test SY77). **(1) Detune** : slider passé à `setRangeAndRound(-15,15)` → chemin s/m (boolNegative) ; pures `fpdDetuneToWire/Display` + test (half=15/signbit=16, main.lua l.45 + OCR l.438). **(2) Coarse** : retrait de `setMidiValueOffset(-1)` (display 0 → wire 127) ; octet brut 0..127 comme l'éditeur TG77 vérifié. **(3) Collision 0x18** (btFix=FPM b0, sensPm=FMLPMS b4~2, pegSw=PES b1) : auto-send OFF + recomposition de l'octet complet `Oscillator::send18` lisant l'état des 3 widgets (OCR l.431-433). **(4) PHASE/SYNC 0x19** : KOE/sync (V1 b0) + phase (V2) recomposés en un frame `send19` (OCR l.435-436). **(5) PEG SW grille** : était FYPSW d'élément (group 0x05/0x0C, prop partagée → 6 boutons/1 état/aucun effet) ; c'est en fait PES par-op (0x18 b1, OCR l.432) → propriété distincte `ELEMENT<e>PEGSW<o>`. Copier-coller corrigés (`sliderFine4`→`sliderPhase4`, `labelFine2`/`labelPhase2`). `setAutoSend` ajouté à MidiSlider/MidiButton. Build/test **691/691**. **Chargement dump des octets packés NON branché** (loader ne décode pas 0x18/0x19). |
| 2.4 | Drum (type 10) | sysex | Table 2 + validation offline + spot-check |

### Vague 3 — Rapprocher de SynthWorks + ergonomie
| # | Tâche | Agent | Effort |
|---|---|---|---|
| 3.1 | ~~Dessiner la topologie des 45 algos (depuis `kAlgo`) + l'afficher dans Voice~~ **FAIT** | ui | moyen | Topologie décodée depuis `kAlgo` (table partagée avec le rendu son) via `SyDraw::afmTopology` : edges modulateur→porteuse (thru/registres/feedback), **porteuse = puits du graphe** (ne module aucun op → sortie) — donne les vrais algos SY77 (algo 1 = 1 porteuse OP1 ; 45 = 6 // ; 44 = OP6→{1..5}+fb). `AlgoDraw` (vue AFM, Operator.h) trace désormais les liens : flèches modulateur→cible, boucle de feedback, porteuses (accent thème) descendant vers une barre OUT ; modulateurs sourds (textMuted). `AlgoDraw::drawAlgoGlyph` rend une version compacte par élément AFM dans la carte ALGO/ROUTAGE de la vue Voice (`paintOverChildren`). Couleurs = rôles de palette (jamais en dur). Code mort `FMOperator` (sliders inutilisés) remplacé par une boîte d'opérateur propre. Preuve : snapshots PNG (`createComponentSnapshot` + glyph) relus pour algos 1/5/8/21/42/44/45 — liens conformes à `kAlgo` ; grand schéma confirmé en vrai dans l'app (vue AFM). RENDU ACCEPTÉ EN L'ÉTAT (2026-06-25), perfectible — artefacts de tracé connus (stub de feedback sur les modulateurs ; algos 19/36/40/41/43), routage pas pleinement orthogonal ; polissage visuel différé. La logique/topologie est bonne, c'est le tracé à fignoler. Build 635/635. |
| 3.2 | Unifier rotatif vs barre ; remplacer les sliders verticaux illisibles — **REFONTE MASTER-DÉTAIL FAITE (2026-06-25)** | ui | moyen | Vue AFM 6 opérateurs (`Oscillator.h`) refondue en **master-détail** (s'éloigne assumé de la grille pleine SynthWorks, jugée « riquiqui sans labels »). **En-tête** rappelant OP / LEVEL / WAVE ; **6 rangées compactes** (résumé : n° OP + barre `sliderLevel*` + roue `sliderOsc*`), rangée sélectionnée surlignée (accent thème + barre gauche) ; **panneau détail** sous les rangées affichant TOUS les contrôles labellisés et au large de l'op sélectionné (`layoutDetail` reprend la disposition de l'ex-`layoutZoom` : potards LEVEL/COARSE/FINE/DET/PHASE + SENSIT VEL/AM/PM + VEL SW/PEG SW + SCALING BP/Offset + SYNC/MODE). Sélection : clic sur une rangée ou flèches `<`/`>` ; `selOp` par défaut 0 (détail jamais vide). **Aucun recâblage sysex** : réutilise les widgets existants via `op(i)`/tableaux et leurs `referTo`/`setMidiSysex` posés dans `setElementNumber` ; seules la disposition (`layoutMaster`/`layoutDetail`) et la visibilité (`updateSelVisibility`) changent. Colonne droite (algo + LFO) inchangée, toujours visible. Couleurs via `SYPal.*`. Build/test **695/695, 0 régression**. **AFFINEMENT (2026-06-25, retour user « beaucoup mieux » mais en-tête+rangées trop hauts)** : en-tête (OP/LEVEL/WAVE) ET 6 rangées résumé (+ flèches `<`/`>` + clic-rangée) SUPPRIMÉS, remplacés par UNE seule **barre d'onglets fine** « OP1 | … | OP6 » en haut de la colonne gauche (6 `TextButton` radioGroup 1001, `setConnectedEdges` segmented, on-colour = `SYPal.accent`, hauteur ~26-34 px) ; clic onglet → `setSelOp`. Tout l'espace libéré va au panneau détail (la roue WAVE `sliderOsc*` y est désormais affichée, à côté du titre). `op(i)` widgets tous masqués sauf l'op sélectionné. Toujours aucun recâblage sysex. Build/test **695/695, 0 régression**. **ON/OFF OPÉRATEURS (2026-06-25, retour user « beaucoup beaucoup mieux ») : barre du haut = GAUCHE onglets de SÉLECTION (édition) + DROITE 6 interrupteurs ON/OFF (activer/désactiver) sur la même ligne fine** (mini-libellés `EDIT`/`ACTIF` pour distinguer). Les onglets sont rétrécis pour laisser place aux 6 carrés `opOn[6]` à droite (allumé=ON accent thème, atténué=OFF). Wiring : **PAS de paramètre on/off dédié dans la spec AFM** (Table 1-7, OCR l.418-444 — comme DX7) → **MUTE éditeur via TL** (param 0x1B déjà ✅) : OFF mémorise le niveau (`savedLevel[i]`) puis force la Value de `sliderLevel*` à 0 (envoi TL=0 par le wiring existant, le potard Level reflète l'état) ; ON restaure. Aucune adresse nouvelle ; mute LOCAL (rechargement de voix → re-cliquer). Cf. MAP.md « OP on/off ». Build/test **695/695, 0 régression**. **POLISH LISIBILITÉ SENSIT/SCALING (2026-06-25)** : dans le panneau détail, SENSIT (Vel 0..15 / AM 0..7 / PM 0..7) et SCALING (4 break-points + 4 offsets, 0..127) s'affichaient en `LinearVertical` NoTextBox étirés = gros pavés gris vides. Repassés en **barres horizontales `LinearBar`** : remplissage proportionnel + **valeur numérique lisible DANS la barre** (rendu déjà fourni par `ModernLookAndFeel::drawLinearSlider`), hauteur compacte (~22-24 px) sous la rangée de potards. Labels : SENSIT garde Vel/AM/PM ; SCALING reçoit des étiquettes **BP1..BP4 / OF1..OF4** dessinées au-dessus de chaque barre (`g_scalingTags`, peintes dans `paint()`). Style/taille/labels seulement — **aucun recâblage sysex** (mêmes `sensVel/sensAm/sensPm`, `bpScl/offScl`). Couleurs via `SYPal.*`. Build/test **695/695, 0 régression**. À juger en test. |
| 3.3 | Éditeur LFO (Main + Sub) — **EN COURS (2026-06-25)** | ui+sysex+midi | moyen | Panneau `AfmLfo.h` (Main LFO : Wave/Speed/Delay/Phase/PMD/AMD/FMD ; Sub LFO : Wave/Speed/Time/PMD + Mode delay/decay), réutilisant MidiSlider/MidiCombo/MidiButton. Intégré dans la **colonne droite** de la vue AFM (`Oscillator.h`), sous l'ALGORITHM — fidèle à l'écran SynthWorks. Câblé pour les 4 éléments : group **0x05**, addrHi élément<<5, params N2 Table 1-6 (Main 0x0D-0x13, Sub 0x15-0x19 ; OCR l.357-371, recoupé `project-sy77-addresses.md`). Statut MAP **🟡** (envoi câblé spec-correct, **non vérifié hardware**) ; chargement depuis dump **⬜** (offsets bulk non vérifiés). Build/test **635/635, 0 régression**. À FAIRE : vérif hardware de l'envoi ; brancher le chargement (.syx) une fois les offsets bulk confirmés ; fignolage layout/wave-glyph. |
| 3.3b | Corrections écrans d'édition après test hardware (C/D/E) — **FAIT (2026-06-25)** | ui+sysex | bas | **(C) Numéro d'algo affiché (vue AFM)** : `Operator.h` ajoute un label `labelAlgoNum` (« ALG n », 1..45, couleur `SYColSelected`) au-dessus de la barre algo à droite, mis à jour dans `setAlgorythm()` depuis la valeur ALGNUM déjà câblée — aucun recâblage sysex. **(D) Boutons Vel SW / PEG SW compacts (panneau détail AFM)** : `Oscillator.h` `layoutDetail` lambda `putBtn` — au lieu de `cell.reduced(6,4)` (énorme rectangle remplissant la rangée), bouton toggle de gabarit normal (~26 px haut, ≤96 px large, `withSizeKeepingCentre`) centré dans la cellule. Câblage sysex inchangé (velSw=RVSW 0x24, pegSw=PES via `send18`). **(E) AWM PNOTE (hauteur de note en mode Fixed)** : `AWMVue.h` ajoute `sliderNote` câblé group **0x07** N2 **0x03** (PNOTE 0~127, octet brut), affichage du NOM de note C-2..G8 (`sy77NoteName`), propriété `ELEMENT<n>PNOTE`, grisé hors mode Fixed (observe `btFixed` via `Value::Listener`). Adresse DOCUMENTÉE (OCR l.488) → MAP **🟡** (envoi non vérifié hardware). PPF/encodage fine inchangé. Build/test **695/695, 0 régression**. À juger en test hardware. |
| 3.3c | Écran Voice : on/off des ÉLÉMENTS + algo dans la cellule WAVE (A/B) — **FAIT (2026-06-25)** | ui+sysex | bas | **(A) On/off des éléments 1-4** : les boutons « Operateurs On-Off » (Voice.h, en réalité les 4 ÉLÉMENTS) étaient INOPÉRANTS. Désormais toggles → `Element::setElementMuted`. **MUTE éditeur via ELVL** : pas de vrai param on/off élément dans la spec (seul ELVL « Element Level 0-127 » existe, group 0x03 / addrHi=(él-1)<<5 / param 0x00, OCR l.281, déjà ✅ chargé/câblé). OFF = mémorise l'ELVL puis force 0 via `sliderVolume.setValue` (qui émet le vrai param-change ELVL) ; ON = restaure. `op1` (ex-`MidiButton` sur adresse bidon `0x0d`) reconverti en `TextButton`. Boutons des éléments **hors mode courant grisés/désactivés** (source de vérité = `nombreElements`, comme les rangées grisées à alpha 0.35). Envoi au CLIC seulement ; mute LOCAL (recharger la voix → re-cliquer). **(B) Cellule WAVE AFM = schéma d'algo** (retour user : « l'algo devrait être à la place de la forme d'onde calculée, fausse bonne idée »). `FmWaveView::paint` (Element.h) ne calcule plus la forme d'onde FM : il dessine le **schéma d'algorithme compact** via `AlgoDraw::drawAlgoGlyph(algo)` (+ overlay « ALG n »). Uniquement en mode AFM (masqué en AWM, le nom de wave échantillonnée reste inchangé). Moteur FM `recompute()` retiré ; `setWaves/setRatios/setLevels` = no-ops (compat API). Couleurs via rôles de thème. Build/test **695/695, 0 régression**. À juger en test. |
| 3.3d | Écran Voice : panneau droit = ROUTAGE seul (suite du switch algo) — **FAIT (2026-06-25)** | ui | bas | Suite de 3.3c/B : l'algo étant désormais dans les cellules WAVE, le panneau de droite dessinait ENCORE un mini-schéma d'algo par élément (`drawAlgoGlyph` dans `paintOverChildren`) → redondant, et avec 2+ éléments AFM les glyphes s'empilaient en chevauchant les lignes de reverb. **Retiré** le bloc `drawAlgoGlyph` (Voice.h `paintOverChildren`). **Gardé** uniquement le routage : sorties éléments (groupes G1/G2) → REVERB HALL/ROOM → L/R + OUT. Boîtes de reverb recentrées (`boxL` 0.34→0.20, `boxR` 0.74→0.62) pour réutiliser l'espace libéré et raccourcir les lignes. **Renommé** le libellé du panneau « ALGORITHME / ROUTAGE » → « ROUTAGE ». Couleurs via rôles de thème. Build/test **695/695, 0 régression**. À juger en test. |
| 3.3e | Libellé de forme d'onde dans le panneau détail AFM — **FAIT (2026-06-25)** | ui | bas | Le glyphe de forme d'onde de l'op sélectionné (roue `sliderOsc*`/`waveLook`, cadre haut-droite du détail) n'affichait pas le nom/numéro. Ajout d'un label `lblWave` (Oscillator.h) **sous le glyphe** (bande de 16 px réservée dans `waveCell`, `layoutDetail`), couleur `SYPal.accent`, Inter bold 12px. Texte = **« Wave N »** (N = 1..16) : **aucune liste de noms des 16 formes AFM** trouvée dans le projet/spec (recherché `LookAndFeel.h`/`AfmWaveLookAndFeel` = sprite `OscAfm_png` sans noms ; OCR l.429 = « PWAVE 0~15 Waveform of oscilator » sans libellés ; TG77_Voice.json = « Wave 1..6 » pour les LFO seulement) → numéro 1-indexé cohérent avec `setOscSliderStyle`. Mise à jour live : Oscillator devient `Value::Listener`, abonné une fois aux 6 `sliderOsc.getValueObject()` (referTo conserve la liste d'écouteurs, cf. `Operator::sliderAlgo`) → `valueChanged`/`updateWaveLabel` rafraîchit au drag ET au chargement de voix ; `setSelOp`/`updateSelVisibility` resynchronisent le texte sur l'op affiché. **Aucun recâblage sysex.** Build/test **695/695, 0 régression**. À juger en test. |
| 3.4 | Nettoyer le code mort + couleurs en dur → rôles de thème | ui | bas |

---

## Journal
- **2026-06-30 (architecture — dico déclaratif, Phase 1)** — Décision : passer d'une RE paramètre
  par paramètre à un **dictionnaire de paramètres déclaratif** (source de vérité unique), inspiré du
  modèle Electra One / lua. AUDIT DE RÉCONCILIATION d'abord (`docs/reconciliation_audit.md`, script
  scratchpad `reconcile.py`) : croise le **JSON Electra** (241 contrôles) × `translate()` du **codec
  lua** (`docs/TG-77 Voice lua and json/main.lua`, bench-verified) → **table master de 221 params**
  (nom/zone/group/N2/encodage/plage). Encodages : 194 plain / 23 s/m / 4 o/b2. Constat clé : notre
  couverture n'est centralisée NULLE PART de façon machine-lisible (adresses calculées à l'exécution,
  MAP.md en prose) — d'où le dico. PHASE 1 livrée (`SysexUtils.h`) : **codec d'encodage générique**
  `enum SyEnc` + primitives pures `smToWire/Display(half,signbit)`, `obToWire/Display(offset)`,
  `ob2Combined/V1/V2/ToDisplay` ; `egLevel`/`panLevel`/`fpdDetune` refactorés en wrappers (équivalence
  prouvée par test). L'**o/b2** (4 break-point offsets, jusque-là non implémenté) est désormais codé +
  testé. Tests `codec déclaratif` (s/m ±7/±12/±15, o/b1 64/32, o/b2 inversible) → **1599/1599**.
  Outil : `build.sh` corrigé (lançait un binaire Release périmé via `find|head -1` → faux 911 ; préfère
  maintenant l'artefact du BUILD_TYPE).
- **2026-06-30 (dico déclaratif, Phase 2)** — Table générée `Source/SyParamTable.h` (221 entrées
  `{ui, nom, zone, op, N2, SyEnc, encA, encB, dispMin, dispMax}`) depuis le JSON Electra × lua, via
  `tools/gen_param_table.py` (+ `tools/reconcile_audit.py`). Classification d'encodage CORRIGÉE vs
  l'audit : 181 plain / 23 s/m / **13 offBin1** (les niveaux EG PEG/FEG, display −64..+63 — l'audit
  les comptait à tort « plain ») / 4 o/b2. Test `SyParamTable` : 221 entrées, ui uniques, aller-retour
  codec sur TOUTE la plage de chaque entrée + ancrages (DETUNE=s/m 15/16, FEG L0=o/b1 64, BP1=o/b2,
  R1=plain) → **4960/4960**. À FAIRE : Phase 3 resolver `syTranslate(ui,el,filtre)→(G,T2,N2)` (port
  `translate()` lua) ; Phase 4 migration progressive des widgets vers le dico.
- **2026-06-30 (nettoyage legacy)** — Suppression du vieil éditeur d'enveloppe mort + démo JUCE.
  `Source/ADSR.h` (classe `SADSR`) et `Source/Hook.h` (classes `Hook`/`Segment`) = ancien éditeur EG
  graphique, remplacé par `EnvelopeDraw.h` + vues Wave/Pitch-EG/filtres → **jamais instancié**.
  `ADSR.h` était `#include` par `FilterVue.h`/`Volume.h` sans usage d'aucun symbole → includes retirés,
  fichiers supprimés (`git rm`), entrées `<FILE>` retirées du `.jucer`. Classe démo `CustomTabButton`
  (étoile verte, dans une ligne commentée) retirée de `MidiDemo.h`. Build unity CMake (via `Main.cpp`)
  ne référençait pas ces fichiers → 0 impact. Le `pbxproj` Xcode garde des entrées orphelines de
  headers `compile="0"` (régénérées au prochain save Projucer, sans effet sur le build). Build/test 911/911.
- **2026-06-30 (UI/LookAndFeel)** — Rendu BIPOLAIRE (pan) uniforme + aperçu Theme Builder.
  **(1) Pans qui ne partaient pas du centre.** Le remplissage « depuis le centre » (12 h) des
  contrôles bipolaires n'était implémenté QUE dans le potard « arc » par défaut et la barre de
  valeur (`drawValueBar`) ; les 4 recettes de potards thématiques (`drawKnobDot/Tick/Neon/ArcTick`,
  `LookAndFeel.h`) remplissaient/allumaient TOUJOURS depuis le début de course → pan faux dans les
  thèmes « dot/tick/neon/arctick ». De plus la détection (`min<0 && max>0`) ratait les pans EG
  (plage 0..63 centrée sur 32, jamais symétriques autour de 0). FIX : helper unique
  `ModernLookAndFeel::isBipolar(s)` = propriété explicite `"bipolar"` OU plage symétrique ;
  honoré par TOUS les rendus (arc + 4 helpers : arc de valeur / graduations / LED allumés entre le
  CENTRE et la valeur ; `Vintage` = aiguille seule, inchangé). Niveaux pan EG (L0..L4, RL1, RL2)
  marqués `bipolar` dans `PanEg.h` (`applyAddresses`). Aucun recâblage sysex. Build/test 911/911.
  COMPLÉMENT : les sous-styles de BARRE `led` et `dots` (`drawValueBar`) allumaient aussi les
  cellules depuis le bord (`i < lit`) en ignorant `bipolar` (≠ rail/groove/notch/bar qui passent
  par `fillRect()` déjà centré) → corrigés : segments/pastilles allumés ENTRE le centre et la
  valeur (centre = vide). Pastilles `dots` aussi rétrécies (0.7→0.52, plafond 14→10 px). 911/911.
- **2026-06-30 (UI/thème)** — Labels qui ne suivaient pas le changement de thème. CAUSE :
  `Label::setColour(textColourId, …)` FIGE la couleur à l'appel (construction) ; au changement de
  thème, seul le DÉFAUT du LookAndFeel (`Label::textColourId = textPrimary`, lu en direct) suit,
  pas les overrides. DEUX classes : (a) overrides REDONDANTS `= SYColLabel` (== le défaut vivant) —
  purs snapshots inutiles → **supprimés** (8 sites : AlgoEditor, Config ×4, Effects, Element,
  Voice) ; les labels héritent du défaut thématisé en direct. (b) overrides de RÔLE (accent/muted/
  fond) qui doivent rester non-défaut → nouveau registre `SyRoleLabels` (`LookAndFeel.h`) :
  `setRoleLabel`/`setRoleLabelColour (label, []{ return SYPal.accent; })` enregistre (label via
  `Component::SafePointer`, applicateur) et `applySyPalette` ré-applique → suivent le thème.
  Convertis : AfmLfo (titres Main/Sub accent + labels muted), AlgoEditor (ALG n + entête op),
  Operator (ALG n), Oscillator (nom de wave), Element (waveNameLabel texte+fond), Config (statut
  de connexion, état mémorisé `connStatusOk`). Statuts orange/rouge (Librairie) = indicateurs
  intentionnels, inchangés. Build/test 911/911.
  SUITE (boutons) — même piège pour les BOUTONS (« Bulk Protect », toggles Settings, groupes,
  onglets…) : `setColour(buttonOnColourId/tickColourId, SYColSelected)` figeait l'état ON. FIX :
  défaut GLOBAL posé dans `syncSyLookAndFeel` (`TextButton::buttonOnColourId = accent`,
  `ToggleButton::tickColourId = accent`, `textColourId = ink`, `tickDisabledColourId = textMuted`)
  → lu en direct par `findColour`. Les 18 overrides par-composant redondants (== accent) SUPPRIMÉS
  (AlgoEditor, Config ×3, Element ×2, MidiDemo ×4, Oscillator ×2, MidiObjects ×2, Pitch, Voice ×2).
  Registre `SyRoleLabels` généralisé en `SyRoleColours` (n'importe quel Component via
  `registerThemeColour`) ; les interrupteurs d'op AFM (texte ON=fond / OFF=muted, non-défaut) y
  sont enregistrés. Build/test 911/911.
  VRAI BUG « Bulk Protect » (vu APRÈS capture d'écran — thème DX7 vert/noir) : le TEXTE des boutons
  pleins (square/round/fun) était `bg.contrasting()` = BLANC, alors que labels/combos/titres
  prenaient `textPrimary` (vert) → boutons « hors thème ». FIX `drawButtonText` : état OFF utilise
  désormais `SYPal.textPrimary` (cohérent), repli sur `bg.contrasting()` seulement si l'écart de
  `getPerceivedBrightness` < 0.4 (illisible) ; état ON garde `accent.contrasting()` (texte sur
  fond accent). Build/test 911/911. Leçon : pour un bug VISUEL, capturer d'abord (cf. mémoire).
- **2026-06-30 (UI/audit)** — Audit « artefacts hors-thème » des vues (grep + agent architecte).
  2 artefacts RÉELLEMENT visibles corrigés : (1) **table AWM** (`AWMVue.h`) sélection en
  `Colours::lightblue`/`darkblue` codés en dur → `SYColSelected` + `.contrasting()` (aligné sur
  Bank/Voices tables) ; (2) **libellés d'octave** du clavier `MidiKeyDraw` (`MidiObjects.h`) peints
  `Colours::white` → `SYColLabel` (illisibles sur thème clair ; touches blanc/noir gardées =
  métaphore piano assumée). Le RESTE de l'audit = code mort / inerte, PAS des artefacts : `Hook`/
  `Segment`/`ADSR` (legacy EG jamais instancié), étoile verte `CustomTabButton` (démo JUCE
  commentée), `setColour` sur colour-ids ignorés par ModernLookAndFeel (Slider track/thumb,
  TextButton on/text — dont mes lignes Oscillator.h:196-197), `addLabelAndSetStyle` qui pose
  `TextEditor::textColourId` sur un Label (mauvais id → inerte). Nettoyage de ce code mort = backlog
  optionnel. Build/test 911/911.
- **2026-06-30 (UI/audit suite)** — 2 vues signalées par l'user (capture à l'appui). (1) **Listes
  MIDI In/Out** (`MidiDeviceListBox::paintListBoxItem`) : ligne sélectionnée (device activé) peinte
  en blend gris texte/fond 50 % → passée à `SYColSelected` + `.contrasting()` (accent, comme
  Bank/Voices/AWM). (2) **St Mix** (Effects) : déjà couvert par le fix `drawButtonText` (TextButton
  → texte = couleur du thème) — confirmé vert en DX7. NETTOYAGE CODE MORT (approuvé) : retrait des
  `setColour` INERTES (colour-ids ignorés par ModernLookAndFeel) — 12 lignes Slider thumb/track +
  TextButton on (= SYColSelected/red) sur 9 fichiers, le bloc `registerThemeColour` des
  interrupteurs d'op (Oscillator), 2 lignes `TextEditor::textColourId=black` sur un Label (MidiDemo).
  RESTE EN BACKLOG (passe dédiée, risqué — inclus par fichiers vivants) : fichiers legacy `ADSR.h`/
  `Hook.h` + classe `Segment` (EG jamais instancié) et `CustomTabButton` (démo JUCE). Build/test 911/911.
- **2026-06-30 (UI/fixes ciblés, capture user)** — (A) **Contour card MIDI Input** blanc/gris (vs
  Output vert) : `MidiDeviceListBox` fait `setOutlineThickness(1)` sans couleur → défaut JUCE gris.
  Ajout `lf.setColour(ListBox::outlineColourId, SYPal.panelBorder)` au global (`syncSyLookAndFeel`)
  → toutes les listes ont le contour du thème. (B) **BUG « St Mix » : libellé effacé à la sélection.**
  `MidiButton::valueChanged` (RX) ET `buttonClicked` (clic) faisaient `setButtonText(strOn/strOff)` ;
  or `setupToggle`/usages sans `setTextOnOff` laissent `strOn`/`strOff` VIDES → au toggle, le libellé
  ("St Mix 1", etc.) était remplacé par "" → bouton vert plein sans texte. FIX : ne swapper le texte
  QUE si `strOn`/`strOff` non vides, sinon garder le libellé statique. Corrige tous les MidiButton
  utilisés comme toggle simple. Build/test 911/911.
  **(2) Theme Builder enrichi** (`ThemeBuilder.h`) : l'aperçu n'avait que des potards/barres
  unipolaires → impossible de juger le pan. Ajout d'un POTARD pan bipolaire (0..63 centre 32, calque
  pan EG) + passage de la barre bipolaire d'exemple en 0..63 centre 32 + légendes sous chaque
  surface (Barre / Barre pan / Potard / Pan), couleur suivant le thème. À juger en test.
- **2026-06-25 (UI/MIDI)** — 2 correctifs après test hardware.
  **(1) Boutons In/Out reconçus en INTERRUPTEURS écoute/envoi** (`MidiDemo.h`). Avant : chaque
  bouton ouvrait un menu de choix de PÉRIPHÉRIQUE (méprise). Désormais 2 toggles ON/OFF rapides
  pour diagnostiquer (couper l'écho, etc.) :
  • `btMidiIn` (« In ») pilote `static bool midiListen` (persisté `MidiListen`, défaut ON) →
    gate en TÊTE de `handleIncomingMidiMessage` (`if (! midiListen) return;`) : OFF = tout MIDI
    entrant IGNORÉ (rien traité/affiché/appliqué aux widgets).
  • `btMidiOut` (« Out ») pilote `static bool midiSend` (persisté `MidiSend`, défaut ON) → gate en
    TÊTE de `sendToOutputs` (`if (! midiSend) return;`) : point de coupe UNIQUE → couvre notes,
    param-changes/sysex, bulk, dump-request ET le MIDI Thru (qui passe par `sendToOutputs`) → OFF
    = rien ne sort, Thru compris. Le CHOIX de périphérique reste dans l'onglet Midi Setting.
  `setClickingTogglesState(true)`, couleur ON `SYColSelected`, état restauré au boot depuis
  AppSettings. `refreshMidiPortButtons` ne force plus l'état ON/OFF (n'affiche que le compteur
  « In·n » dans le libellé). Fix boucle d'écho/LFO/Thru intacts. Build 695/695 tests.
  **(2) Sélection du WAVE LFO à l'ouverture** (`AfmLfo.h`). Combos WAVE (Main 6 / Sub 4) vides à
  l'ouverture tant qu'on n'y touchait pas. CAUSE : `getSelectedIdAsValue().referTo(P(...))` lie
  selectedId à une propriété ValueTree qui est VIDE (void) au démarrage — `voiceBlobToParams`
  OMET volontairement le LFO (cf. `SysexUtils.h`) → selectedId 0 → combo vide. FIX (même garde-fou
  que comboFoot/comboMod) : après le referTo, si `getSelectedItemIndex() < 0`, `setSelectedId(1,
  dontSendNotification)` → wave 0 par défaut, jamais vide ; `dontSendNotification` n'émet aucun
  sysex (encodage d'envoi inchangé, reliability-first).
- **2026-06-24** — Mise en place des 4 agents + audit initial des 3 domaines. Roadmap créée.
- **2026-06-24** — 2.2 niveaux EG (o/b) résolus. Formule offset-binary `display = wire - 64`
  (offset 64) trouvée dans le codec lua INDÉPENDANT (`docs/TG-77 Voice lua and json/main.lua`
  l.22 + `TG77_Voice.json` PEG/FEG L). Fonctions pures `egLevelToDisplay`/`egLevelToWire`
  + chargement pitch-EG/filtre levels + alignement éditeurs (0..64 → 0..127, corrige l'envoi).
  FOS (o/b 2 octets) laissé en suspens (layout bulk non confirmé). 635/635 tests.
- **2026-06-25** — Bugs grille opérateur AFM remontés sur VRAI SY77, corrigés (ENVOI) : detune
  s/m, coarse (offset retiré), collision octet 0x18 (MODE/PES/PM recomposés), PHASE/SYNC 0x18 b1
  + 0x19 V1/V2 recomposés, PEG SW grille = PES par-op (≠ FYPSW d'élément). `setAutoSend` ajouté
  aux widgets ; pures `fpdDetuneToWire/Display` + test. Copier-coller `sliderFine4`/labels corrigés.
  691/691 tests. Reste OUVERT : chargement dump des octets packés (loader ne décode pas 0x18/0x19).
- **2026-06-25** — 4 problèmes remontés sur VRAI SY77.
  **#1 BOUCLE D'ÉCHO (critique) — CORRIGÉE.** L'app ré-émettait en boucle les param-changes
  échoés par le synthé. Cause racine double : (a) le `valueSysexIn` reçu était notifié en
  ASYNCHRONE (`MidiDemo.h` handleAsyncUpdate) → l'application widget se faisait HORS de tout
  garde ; (b) `ComboBox::valueChanged` (juce_ComboBox.cpp l.308-311) RE-PUBLIE via
  `setSelectedId(... sendNotification)` quand son currentId (lié au ValueTree) change sans passer
  par setSelectedId → `MidiCombo::comboBoxChanged` → `sendParam9`. Le `dontSendNotification` des
  widgets ne suffisait donc pas (Slider/Button OK, mais ComboBox re-fire ; et l'async sortait du
  garde). Fix THREAD-SAFE et RÉENTRANT : `ScopedEchoSuppress` + `SysexBus::suppressSend` (atomique),
  posé autour de l'APPLICATION RX désormais SYNCHRONE (`sendChangeMessage(true)`), qui fait DROPper
  tout envoi widget→bus dans `SysexBus::publish` (choke-point unique). Voice-load replay aussi sous
  garde. Remplace proprement l'ancien `boolStopReceive` (global, non thread-safe, et inutile car il
  n'entourait qu'un `sendMessageNow` synchrone, pas l'écho différé). Tests `EchoLoop` (3) ajoutés.
  **#2 GROUPES DE SORTIE — CÂBLÉS.** btGroup1/2 (Element.h) n'envoyaient RIEN (« rendu/état seul »).
  Désormais : octet packé group 0x03 / param 0x08 (b1=OUTSEL0, b2=OUTSEL1 ; b0=MCTEN laissé 0),
  addrHi=(el-1)<<5, émis au clic. Spec OCR l.292-294 + carte `out sel@08`. 🟡 non vérifié hardware.
  **#3 FINE (FPF 0x26) — CORRIGÉ.** Colonne FINE bornée à tort 0..99 → **0..127** (octet brut),
  conforme à la carte TG77 hardware (`FINE OP*` pNum *038, 0..127). 100..127 étaient inatteignables.
  **#4 AWM mode fixed — INVESTIGUÉ (pas de fix deviné).** Trace `07/2n/02,03,04` = PPM (fixed sw,
  0x02), PNOTE (fixed note#, 0x03), PPF (fine, 0x04). L'éditeur n'a PAS de contrôle PNOTE, et
  `sliderFine` (PPF) passe par le chemin boolNegative/boolInvert (s/m-like) alors que la spec dit
  offset-binary (OCR l.489) → encodage de l'envoi à confirmer hardware avant correction. MAP mis à jour.
  Build 698/698 tests.
- **2026-06-25 (UI)** — Sélecteurs de ports MIDI **in/out toujours visibles** ajoutés dans le
  bandeau bas, à GAUCHE du toggle clavier (`MidiDemo.h`) : 2 petits boutons `btMidiIn`/`btMidiOut`
  (ordre [In][Out][Clavier]) ouvrant chacun un `PopupMenu` à cases cochables (multi-sélection).
  RÉUTILISE la gestion de ports existante (SOURCE DE VÉRITÉ UNIQUE de l'onglet Midi Setting) :
  `showMidiPortMenu` lit `midiInputs/midiOutputs`, bascule via `openDevice`/`closeDevice`, persiste
  via `saveEnabledDevices` (clés `MidiInDevices`/`MidiOutDevices`), puis re-synchronise la ListBox
  de l'onglet (`MidiDeviceListBox::syncSelectedItemsWithDeviceList`) → les 2 vues restent cohérentes.
  Libellé/état rafraîchis (compteur de ports ouverts + couleur ON du thème) par `refreshMidiPortButtons`,
  appelé dans `timerCallback` (hot-plug, timer existant) et après chaque bascule. Rôles de thème
  respectés (couleur ON via `SYColSelected`). Build 695/695 tests.
- **2026-06-25 (UI)** — Moniteur MIDI : 2 toggles ajoutés dans `MidiSettingsPage` (`MidiDemo.h`),
  côte à côte au-dessus du moniteur, ticks au thème (`SYColSelected`).
  **(1) Raw / Interprété** (`rawBtn`, persisté `MonitorRaw`, défaut OFF=décodé) : bascule chaque
  message entre OCTETS BRUTS hex (`midiBytesToHex`, sans dédup) et forme décodée G/AH/AL/P/val
  (dédup par adresse). Formatage factorisé dans `formatMonitorLine` (partagé flux temps réel +
  re-render). S'applique AUSSI aux lignes déjà présentes : `rebuildMonitorFromHistory` re-rend
  depuis un historique borné `monitorHistory` (≤2000 msgs ; vidé par Clear).
  **(2) MIDI Thru** (`thruBtn`, persisté `MidiThru`, **défaut OFF**, étiquette « écrit vers le
  synthé ») : dans `handleAsyncUpdate`, forwarde chaque message reçu (hors active-sense) via
  `sendToOutputs`. ANTI-BOUCLE : le forward appelle DIRECTEMENT `sendToOutputs`, PAS
  `SysexBus::publish` → le `ScopedEchoSuppress` (qui ne garde que `publish`) ne l'avale jamais ;
  et on ne forwarde QUE l'entrée reçue (jamais les propres émissions de l'app) → aucune boucle
  créée par l'app. Seul un soft-thru du synthé pourrait boucler (assumé en activant Thru, OFF par
  défaut). Build 695/695 tests.
- **2026-06-26 (LIB) — Collection de banques du web embarquée (FAIT).** Une collection de banques
  SY77 (récupérées sur le net) est **embarquée dans le binaire** : `Ressources/SY77_AllTheWeb.zip`
  ajouté à `juce_add_binary_data` (`CMakeLists.txt`) → `BinaryData::SY77_AllTheWeb_zip`. Le zip a été
  **curé** une fois (validation bloc-par-bloc : chaque message F0…F7 doit avoir `byte[3]∈{0x7A,0x7E}`
  + checksum Yamaha correct) → **231 banques retenues, 4 rejetées** (corrompues/tronquées). Les 78
  banques `DX7patch/` sont incluses : patches DX7 **déjà convertis au format SY77** (bulk 0x7A), pas
  du SysEx DX7 natif, donc jouables sur le SY77. L'import (`LibrairiePage::importWebBanksFromBinary`,
  `Librairie.h`) décompresse le zip dans `~/Library/Application Support/Sysex77/SY77 (web)/`
  (`uncompressTo(dest, overwrite=true)` → idempotent) puis rafraîchit la librairie. Déclencheur
  d'accès **volontairement non documenté ici** (surprise à préserver). N'écrit QUE des fichiers
  locaux, jamais vers le synthé ([[feedback-hardware-safety]]).
- **2026-06-26 (MIDI) — Envoi banque/preset corrigé + fenêtre de progression (FAIT).**
  **(1) Envoi de banque** : le handler `adresseOscSendBank` (`MidiSysex.h`) reconstruisait
  `appDirPath + nom de fichier` → échouait pour toute banque en SOUS-DOSSIER (banques importées).
  Il réutilise maintenant `currentBankData` (déjà chargé à la sélection, même source que l'envoi
  d'une voix). **(2) Débit** : nouvelle `BankSendProgressWindow` (`BankSendProgress.h`,
  ThreadWithProgressWindow) — l'envoi throttlé tourne sur un thread d'arrière-plan (UI fluide, barre
  + Annuler, fermeture auto). Le délai inter-message est **proportionnel à la taille** (≈ temps-fil
  31250 bauds, `taille/3 ms`) + marge `baseDelayMs` (défaut 60) : sans ça `sendMessageNow` empile
  tout et le SY77 sature. **(3) Envoi d'UN preset = audition** : `SyVoice::retargetVoiceToEditBuffer`
  (`SysexUtils.h`, pure + test) réécrit le bulk voix en **Memory type $7F (Edit Buffer)** (offset 30)
  + Memory# 0 (offset 31) + checksum recalculé → le SY77 affiche/joue la voix envoyée SANS écraser
  la mémoire interne (avant : écrit dans le slot d'origine, non auditionné). Build 900/900 tests.
- **2026-06-26 (LIB/MIDI) — Réception de banque + recherche (FAIT).**
  **(1) RECEIVE robuste** : la réception (`Librairie.h` + `BankReceiveProgress.h`) passe en pacing
  REQUÊTE→RÉPONSE (handler `adresseOscRequestDump` = 1 dump request par mémoire) : on demande la
  voix `mem`, on attend sa réponse, puis la suivante. Envoyer les 64 d'un coup saturait le buffer du
  SY77 (dump tronqué). Fenêtre de progression (barre + Annuler). Vérifié : 64/64 slots reçus, aucun
  manquant (les « trous » à l'écran = voix init/nom vide sur le synthé). **(2) Nommage** : à la fin,
  on **demande le nom** avant d'écrire (plus de `UNNAMED.syx`) ; si le nom existe → **confirmation
  d'écrasement** (sinon annulation). Checksum de chaque bloc validé à l'écriture.
  **(3) Recherche** : champ de filtre au-dessus de la liste des banques (`BankTableModel` garde une
  liste maître ; `arrayBank`/`BankFiles` = vue filtrée, mapping ligne→fichier conservé). Build 900/900.
- **2026-06-26 (LIB) — Refonte librairie « moderne » (FAIT, 3 phases).** Voir `LibraryIndex.h`.
  **Phase 1** : index `library.json` (juce::JSON ; par banque le synthé détecté + size/mtime, par
  preset clé `banque#slot` -> nom/tags/notes/favori) ; chargé au démarrage + réconcilié au disque en
  tâche de fond (ajout/purge/refresh) ; `SyVoice::detectSynthKind`/`synthKindLabel` (pur + test) ;
  **suppression de banque** fiabilisée (bug d'index sous filtre corrigé) + bouton visible +
  désindexation ciblée. **Phase 2** : **panneau d'inspection** du preset (nom·synthé·type + Tags/Notes
  éditables + Favori, persistés), **indicateur ★/•** dans la liste de voix, **filtre par synthé**
  (ComboBox visible si >1 synthé). **Phase 3** : **recherche globale de presets** (champ + filtre tag
  + bascule ★) -> le panneau droit bascule en **liste de résultats à plat** (`LibraryIndex::searchPresets`,
  nom·banque·synthé·tags ; sélection charge la banque, double-clic envoie) ; **copier un preset vers une
  banque perso** (ajout du bloc F0…F7, `refreshBank`) ; **tri** banques (nom/synthé/date).
  Réel : 247 banques / 18199 presets indexés. Build 905/905 tests.
- **2026-06-26 (LIB) — Garde SY77 + ergonomie (FAIT).** **Garde SY77** ([[feedback-hardware-safety]]) :
  l'envoi de banque (`adresseOscSendBank`) et l'envoi/audition d'une voix (`adresseOscSendVoice`) sont
  ignorés si la donnée n'est pas SY77 (`detectSynthKind`) ; côté UI, bouton SEND grisé + note quand la
  banque sélectionnée n'est pas SY77. **Détection au niveau banque corrigée** : on scanne les blocs
  (SY77 si AU MOINS un bloc voix « LM 8101VC ») — avant, une banque démarrant par un bloc système/multi
  était classée à tort non-SY77 (47 faux négatifs → SEND aurait été bloqué à tort). Bump `library.json`
  v2 force la re-détection (tags conservés). **Ergonomie** : croix × pour vider les champs de recherche
  (banque + preset) ; l'info preset n'écrase plus le header (elle est dans l'inspecteur/footer), le
  header garde l'info de la banque. Build 905/905.
  RESTE : envoi VERS d'autres synthés (Roland/Korg…) = chantier futur, unité par unité, vérifié HW.
- **2026-06-26 (APP) — Mise à jour in-app depuis GitHub (FAIT).** `Source/Updater.h` (singleton) :
  vérif au démarrage (non bloquante, fail-silent, throttle 24 h, opt-out `CheckUpdatesOnStartup`)
  de `…/releases/latest` (`juce::URL` sans curl + `juce::JSON`, header User-Agent obligatoire pour
  l'API GitHub) → `SyVoice::isNewerVersion` (pur + test) → dialogue (Installer / Notes / Plus tard /
  Ignorer cette version). Téléchargement de l'asset `.app` zippé (fenêtre de progression
  `WebInputStream`), puis **auto-install macOS** : `ditto -x` + helper `swap.sh` détaché (attend la
  fermeture, `mv` avec `.bak` réversible, `xattr` quarantaine, `open`) + `quit`. UI : carte « Mise à
  jour » dans Setting (bouton « Vérifier » + case « au démarrage »). Garde-fous : confirmation avant
  install, repli Finder si emplacement non inscriptible, plateforme non-macOS = ouvre la page release.
  **Procédure release (`release.sh X.Y.Z`)** : fige `Version.h`, build Release `--no-bump` + tests,
  dist + ad-hoc sign, zip `ditto -c -k --keepParent`, commit+tag+push, `gh release create` (asset),
  re-bump `-dev`. (`build.sh` a gagné `--no-bump`.) Build 905/905.
- **2026-06-26 (UI) — Theme Builder : pastilles de couleur visibles (FIX).** Les pastilles étaient
  des `TextButton` rendus par le LnF maison (fond non rempli en style `outline`/`flat`) → couleur
  invisible selon le thème édité. Remplacées par un composant `Swatch` qui peint directement la
  couleur cible. Skill `/release` + `release.sh` (incrément patch/minor/major depuis la dernière
  release) ajoutés. Build 911/911.
- **2026-06-26 (UI/BUILD) — Schéma de version + dessin d'algo (FIX).** **Auto-bump par build
  RETIRÉ** de `build.sh` (il faisait dépasser le dev au-delà des releases → numéros incohérents) :
  la version ne change qu'aux releases, chaque build identifié par son timestamp. **AlgoDraw** :
  le pas des rangées/colonnes est plafonné au pas de cellule (`jmin(cellW/cellH, …)`) → le schéma
  d'algo profond (ex. ALG 1, pile de 6 ops) ne déborde plus en haut de la card WAVE.
- **2026-06-26 (APP) — Auto-install de la MAJ : réparée (FIX).** L'install (download OK) ne
  remplaçait jamais l'app. Causes : (1) **CRLF** — `replaceWithText` écrit en CRLF par défaut, le
  helper `swap.sh` était illisible par bash (exit 2) → écriture forcée en **LF** (cause racine) ;
  (2) **TCC** — helper + app extraite déplacés dans `/tmp` (les jobs launchd n'accèdent pas à
  `~/Library/Caches`) ; (3) **détachement** via `launchctl submit` (un nohup/& mourait à la
  fermeture de l'app), avec **auto-retrait du job** (sinon relance en boucle ~10 s). swap.sh réécrit
  + garde App Translocation + log `/tmp/sysex77-update.log`. Vérifié bout en bout (1.3.1→1.3.2,
  swap unique). NB : les builds ≤1.3.2 ont l'updater cassé → MAJ manuelle une fois vers ≥1.3.3.
- **2026-06-26 (UI) — Éditeurs d'EG : sliders à taille fixe (FIX).** Wave EG + Pitch EG : les sliders
  L0…RR2 et Slope utilisaient `setBoundsRelative` (largeur = 0,04 × largeur fenêtre → ils enflaient) ;
  le Slope (`y=0.92`) chevauchait le clavier. Désormais largeur fixe 26 px + hauteur plafonnée, et
  Slope juste au-dessus du clavier. (Filter/Pan EG non touchés — layouts différents.)
- **2026-06-26 (APP) — MAJ : vérif à chaque démarrage (FIX).** Le throttle 24 h empêchait la
  détection au lancement (sauf bouton « Vérifier »). Retiré : `checkAsync` vérifie maintenant à
  CHAQUE démarrage (1 requête ; le dialogue n'apparaît que si une version plus récente non ignorée
  existe). Opt-out `CheckUpdatesOnStartup` conservé.
