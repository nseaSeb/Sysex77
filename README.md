# Sysex77 — éditeur MIDI pour Yamaha SY77 / SY99 (EN / FR)

![Sysex77](Ressources/icon.png)

Éditeur de sysex (presets & banques) pour les Yamaha **SY77 / SY99 / TG77**, écrit en C++/JUCE.
Inspiré, côté fonctionnalités, de l'éditeur *SynthWorks SY77*.

> ⚠️ **Projet en cours de développement** — à utiliser avec précaution.
> L'envoi de banques peut **écraser les patchs de votre synthé** : faites une sauvegarde et
> désactivez le *bulk protect* au préalable.

**Contributions bienvenues** — quiconque souhaite contribuer, voire reprendre le projet, est le bienvenu.

![Éditeur de voix Sysex77](divers/editor.png)

---

## Fonctionnalités

- **Éditeur de voix** AFM / AWM : opérateurs, oscillateurs, filtres, enveloppes, pan.
- **Rendu de la forme d'onde FM** reproduisant la topologie réelle des **45 algorithmes** du SY77.
- **Édition à la souris des graphes** : glisser sur la réponse du **filtre** (cutoff/résonance) et
  sur les nœuds des **enveloppes** (niveau + rate) — vignettes d'élément et éditeurs complets.
- **Onglet Effets** : Effect Mode + 2 unités Modulation + 2 unités Reverb.
- **Librairie de banques** (`.SYX`) moderne : **envoi/réception** avec barre de progression,
  **suppression** de banque, **recherche** (banques et presets), **tri**, **métadonnées par preset**
  (tags, notes, favoris), **recherche globale + filtre par tag** transversale à toutes les banques,
  **filtre par synthé** (si plusieurs présents) et **copie d'un preset** vers une banque perso.
  Double-clic sur une voix = envoi au synthé (audition dans l'edit buffer) + ouverture dans l'éditeur.
- **Persistance** des réglages : interfaces MIDI, device number, taille de fenêtre, onglet,
  clavier, et **couleurs personnalisées** (thème custom).
- Synchronisation **tout-MIDI** bidirectionnelle (un seul *device number* global).
- **Mise à jour intégrée** : l'app vérifie les *GitHub Releases* au démarrage et propose de
  télécharger + installer la nouvelle version automatiquement (macOS ; désactivable dans *Setting*).

> ⚠️ Certaines éditions (effets, groupes, enveloppes…) sont pour l'instant en **rendu/état seul**
> côté éditeur ; l'envoi sysex complet vers le synthé est en cours de fiabilisation. Tous n'est pas implémenté.

📋 **État de validation paramètre par paramètre** (envoi → synthé / ouverture dump → éditeur) :
voir **[MAP.md](MAP.md)**.

---

## Lancer l'app pré-compilée (macOS)

L'application macOS pré-compilée est fournie dans **`dist/Sysex77.app`** (Apple Silicon / arm64).
Elle n'est **pas signée ni notariée** par Apple : au **premier lancement**, macOS la bloque
(« développeur non identifié »). Pour l'autoriser :

1. **Clic droit** (ou Ctrl-clic) sur `dist/Sysex77.app` → **Ouvrir** → puis, dans la boîte de
   dialogue, cliquez encore sur **Ouvrir**. *(Une seule fois ; ensuite, double-clic normal.)*
2. **Sans Terminal** (macOS récent) : double-cliquez l'app une fois (elle est bloquée), puis allez
   dans **Réglages Système → Confidentialité et sécurité** ; tout en bas, un message « Sysex77 a
   été bloqué… » propose **« Ouvrir quand même »** → cliquez, puis **rouvrez l'app** et confirmez.
3. *(Dernier recours, Terminal)* si l'app est dite « endommagée » après **téléchargement**
   (quarantaine), retirez l'attribut :
   ```bash
   xattr -dr com.apple.quarantine dist/Sysex77.app
   ```

> ⚠️ Binaire **Apple Silicon (arm64)** pour l'instant. Sur Mac **Intel** (ou en cas de doute),
> compilez depuis les sources (ci-dessous) — c'est tout aussi simple.

---

## Build (sans Projucer)

Compilation via CMake — **pas besoin de Projucer**, et **JUCE est téléchargé
automatiquement** s'il est absent. Idéal pour les utilisateurs **non-Mac** (l'app pré-compilée
n'est fournie que pour macOS).

```bash
./build.sh --test     # macOS / Linux
build.bat  --test     # Windows
```

📖 **Guide complet** (prérequis par OS, options, dépannage) : voir **[BUILD.md](BUILD.md)**.

---

## Utilisation

À l'ouverture (après l'image d'accueil) : sélectionnez votre interface MIDI, puis le **device
number (canal)** dans *Setting*. Vous pouvez y activer/désactiver le *bulk protect* (ou le faire
sur le synthé) pour autoriser les échanges de banques.

- **SY99** : choisissez *SY 99* dans les paramètres, fermez et rouvrez l'application (la liste des
  waves correspondra à votre synthé).
- Cliquez en haut de la page (ou dans une zone noire) pour faire apparaître le menu.
- La **Librairie** gère vos banques `.SYX`. On en trouve beaucoup en ligne, par ex.
  http://bobbyblues.recup.ch/yamaha_sy77/sy77_patches.html
  *(Pensez à sauvegarder vos banques et à désactiver le bulk protect avant tout envoi.)*
  - **Recevoir / Envoyer** une banque (barre de progression, débit adapté au SY77), **supprimer**
    une banque, **rechercher** et **trier** la liste.
  - **Métadonnées par preset** : sélectionnez une voix → panneau d'inspection (nom · synthé · type)
    pour saisir **tags**, **notes** et **favori** ; un repère ★/• apparaît dans la liste.
    Tout est stocké dans `~/Library/Application Support/Sysex77/library.json`.
  - **Recherche globale de presets** (par nom ou tag, avec filtre **★ favoris**) : le panneau de
    droite bascule en liste de résultats **transversale à toutes les banques**.
  - **Copier un preset** vers une banque perso ; **filtre par synthé** lorsque plusieurs familles
    sont présentes (l'envoi/édition reste réservé au SY77).
- **Double-cliquer une voix** l'envoie au synthé (audition dans l'edit buffer, sans écraser la
  mémoire) **et l'ouvre dans l'éditeur**.

![Librairie Sysex77](divers/librairie.png)

### Thèmes

L'onglet *Setting* propose un sélecteur de thème. Cinq palettes sont intégrées (*Dark Orange*,
*FM Dark*, *Light*, *Crimson*, *Tangerine*). Au premier lancement, elles sont exportées en tant
que fichiers éditables dans :

```
~/Library/Application Support/Sysex77/Themes/
```

Chaque thème est un sous-dossier contenant un fichier `theme.xml`. Vous pouvez :
- **Modifier** un thème intégré en éditant le `theme.xml` correspondant.
- **Créer un thème custom** : dupliquez un sous-dossier, renommez-le, éditez le `theme.xml`.
- Cliquer sur **Reload** dans *Setting* pour prendre en compte vos modifications sans relancer l'app.

![AfmOsc](divers/AfmOsc.png)

---

## English (summary)

Sysex (presets & banks) editor for the Yamaha SY77/SY99/TG77, C++/JUCE.
**Not finalized — use at your own risk; sending banks can erase your synth's patches, back up first!**

Prebuilt macOS app: `dist/Sysex77.app` (Apple Silicon, **unsigned**). First launch: right-click →
Open → Open, or (recent macOS) System Settings → Privacy & Security → **Open Anyway** then reopen.
If "damaged" after download: `xattr -dr com.apple.quarantine dist/Sysex77.app`.

Build (no Projucer needed): `./build.sh` (macOS/Linux) or `cmake -B build && cmake --build build`
(Windows incl.). JUCE is auto-resolved (local `JUCE_DIR`, `/Applications/JUCE`, or auto-downloaded).
Run unit tests with `--test`.

On launch: pick your MIDI interface, then the device number (channel) in *Setting*; toggle bulk
protect to allow bank exchanges. The Library sends/receives sound banks (`.SYX`) with a progress
bar; double-clicking a voice auditions it on the synth (edit buffer) and opens it in the editor.

Highlights: AFM/AWM voice editor, FM-waveform render of the real 45 algorithms, mouse-editable
filter & envelope graphs, output groups + routing, an Effects tab, and persistence of settings &
custom colours. **Modern library**: bank send/receive with progress, delete, search & sort,
**per-preset metadata** (tags, notes, favourites) in `library.json`, **global preset search +
tag filter** across all banks, per-synth filter, and copy-a-preset-to-a-user-bank (send/edit
stay SY77-only). **Built-in updater**: checks GitHub Releases on launch and can download + install
the new version automatically (macOS; opt-out in *Setting*). Some editor edits are state-only for
now (full sysex send is being firmed up).

**Themes:** five built-in palettes (Dark Orange, FM Dark, Light, Crimson, Tangerine), selectable
in *Setting*. On first launch, editable `theme.xml` files are exported to
`~/Library/Application Support/Sysex77/Themes/`. Add a subfolder with your own `theme.xml` to
create a custom theme; hit **Reload** in *Setting* to pick it up without restarting.

---

## Documentation de référence

- `SY77_PARAMETERS.md` — référence sysex (format, groupes, adresses) croisée code + spec officielle.
- La spec Yamaha *SY77 MIDI Data Format* et les captures SynthWorks sont sous `docs/` (non versionné).

## Soutien & remerciements

Un grand merci au **forum JUCE**, en particulier : Daniel, Jules, McMartin, matkatmusic, Xenakios, dqthebt.

*Ps : aucun animal n'a été blessé durant le développement — au contraire, un labrador a partagé
de nombreuses interrogations, et l'agitation de son train arrière n'a laissé aucun doute : « suis content » !*

— Seb
