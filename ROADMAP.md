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
- Trous : pitch-EG **levels** (o/b), Main/Sub-LFO, FTYPE (bulk≠enum), PHASE/SYNC packés,
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
- Trous : **topologie des 45 algos non dessinée** (données `kAlgo` présentes, carrés sans liens) ;
  pas d'éditeur **LFO** ; pas de **drum set** ; voice-common incomplet (microtuning, portamento…) ;
  sliders verticaux étroits illisibles (pan/volume 16-24 px) ; code mort (`FMOperator`, `SADSR`,
  `sendChangeMessage()` dans `paint()`) ; couleurs en dur échappant au thème.

---

## Backlog priorisé

### Vague 0 — Stabiliser les fondations (avant toute feature)
| # | Tâche | Agent | Effort | Critère de fini |
|---|---|---|---|---|
| 0.1 | ~~Resynchroniser `MAP.md` avec le code réel~~ **FAIT** | sysex | bas | MAP.md resync : Coarse/Waveform 🟢 (oracle), Detune/SENSIT/SCALING 🟡, AWM section ajoutée |
| 0.2 | ~~Tests d'encodage d'envoi (régression EG)~~ **FAIT (group byte)** | sysex | bas | `egGroupFor` pure (SysexUtils.h) consommée par les 4 éditeurs + test `egGroupFor locks…` (118/118). NB : verrouille le **group**, pas encore les offsets de param |
| 0.3 | ~~Brancher `Hook.h` sur `valueSysexIn` (RX)~~ **FAIT** | midi | bas | `Segment` écoute `valueSysexIn` (Hook.h) ; match des DEUX adresses (rate via `sysexRate`, level via `sysexLevel`) et route vers le bon axe via `setValue()` (sans renvoi). Les EG via Hook se resynchronisent depuis le synthé (118/118) |
| 0.4 | ~~Supprimer `boolStopReceive` (garder `dontSendNotification`)~~ **FAIT** | midi | bas | Flag non thread-safe retiré (décl. + set true/false dans `sendToOutputs` + test RX dans `handleIncomingMidiMessage`). Anti-écho porté uniquement par `dontSendNotification` (4 widgets Midi*) + `setValue()` sans renvoi (Segment) (118/118) |

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
| 2.2 | Niveaux EG en offset-binary (pitch-EG levels, filtre FL0-4, FOS) | sysex | validation offline sur 811 banques |
| 2.3 | PHASE/SYNC bit-split | sysex | diff single-param |
| 2.4 | Drum (type 10) | sysex | Table 2 + validation offline + spot-check |

### Vague 3 — Rapprocher de SynthWorks + ergonomie
| # | Tâche | Agent | Effort |
|---|---|---|---|
| 3.1 | Dessiner la topologie des 45 algos (depuis `kAlgo`) + l'afficher dans Voice | ui | moyen |
| 3.2 | Unifier rotatif vs barre ; remplacer les sliders verticaux illisibles | ui | moyen |
| 3.3 | Éditeur LFO (Main + Sub) | ui+sysex+midi | moyen |
| 3.4 | Nettoyer le code mort + couleurs en dur → rôles de thème | ui | bas |

---

## Journal
- **2026-06-24** — Mise en place des 4 agents + audit initial des 3 domaines. Roadmap créée.
