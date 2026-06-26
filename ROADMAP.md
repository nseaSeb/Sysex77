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
  RESTE (non fait) : **envoi cross-synthé gardé** — n'autoriser SEND/audition que vers le synthé du
  type détecté ([[feedback-hardware-safety]]) ; aujourd'hui l'envoi reste pensé SY77 (les banques
  d'autres synthés sont stockées/affichées/cherchables mais l'éditeur et l'audition edit-buffer
  restent SY77). À faire unité par unité, vérifié HW avant activation.
