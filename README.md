# Sysex77 — éditeur MIDI pour Yamaha SY77 / SY99 (EN / FR)

![Sysex77](Ressources/icon.png)

Éditeur de sysex (presets & banques) pour les Yamaha **SY77 / SY99 / TG77**, écrit en C++/JUCE.
Inspiré, côté fonctionnalités, de l'éditeur *SynthWorks SY77*.

> ⚠️ **Projet en cours de développement** — à utiliser avec précaution.
> L'envoi de banques peut **écraser les patchs de votre synthé** : faites une sauvegarde et
> désactivez le *bulk protect* au préalable.

**Contributions bienvenues** — quiconque souhaite contribuer, voire reprendre le projet, est le bienvenu.

---

## Build

Le projet se compile de deux façons. **Pas besoin de Projucer pour la voie CMake** (recommandée,
notamment hors macOS).

### Option A — CMake / `build.sh` (macOS, Linux) — recommandé

```bash
./build.sh            # build Release dans ./build
./build.sh --debug    # build Debug
./build.sh --test     # build puis lance les tests unitaires
```

Ou directement en CMake (toutes plateformes, y compris Windows) :

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

**JUCE** est résolu automatiquement :
1. `-DJUCE_DIR=/chemin/vers/JUCE` (ou variable d'env `JUCE_DIR`) si vous avez une copie locale,
2. sinon `/Applications/JUCE` (install Projucer macOS),
3. sinon **téléchargé automatiquement** (JUCE 8.0.7) — aucune installation manuelle requise.

**Dépendances Linux** (Debian/Ubuntu) :

```bash
sudo apt install build-essential cmake libasound2-dev libjack-jackd2-dev \
  libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
  libxinerama-dev libxrandr-dev libxrender-dev libwebkit2gtk-4.1-dev \
  libglu1-mesa-dev libcurl4-openssl-dev
```

### Option B — Projucer / Xcode (macOS, historique)

Ouvrir `Sysex77.jucer` dans Projucer, exporter, puis compiler sous Xcode
(`Builds/MacOSX/Sysex77.xcodeproj`).

### Tests

Les tests unitaires (logique sysex « pure ») tournent via l'argument `--test` du binaire :

```bash
./build.sh --test
# ou
build/Sysex77_artefacts/Sysex77.app/Contents/MacOS/Sysex77 --test
```

---

## Utilisation

À l'ouverture (après l'image d'accueil) : sélectionnez votre interface MIDI, puis le **device
number (canal)** dans *Setting*. Vous pouvez y activer/désactiver le *bulk protect* (ou le faire
sur le synthé) pour autoriser les échanges de banques.

- **SY99** : choisissez *SY 99* dans les paramètres, fermez et rouvrez l'application (la liste des
  waves correspondra à votre synthé).
- Cliquez en haut de la page (ou dans une zone noire) pour faire apparaître le menu.
- La **Librairie** permet de dumper des banques. On en trouve beaucoup au format `.SYX`, par ex.
  http://bobbyblues.recup.ch/yamaha_sy77/sy77_patches.html
  *(Pensez à sauvegarder vos banques et à désactiver le bulk protect avant.)*

![AfmOsc](divers/AfmOsc.png)

---

## English (summary)

Sysex (presets & banks) editor for the Yamaha SY77/SY99/TG77, C++/JUCE.
**Not finalized — use at your own risk; sending banks can erase your synth's patches, back up first!**

Build (no Projucer needed): `./build.sh` (macOS/Linux) or `cmake -B build && cmake --build build`
(Windows incl.). JUCE is auto-resolved (local `JUCE_DIR`, `/Applications/JUCE`, or auto-downloaded).
Run unit tests with `--test`.

On launch: pick your MIDI interface, then the device number (channel) in *Setting*; toggle bulk
protect to allow bank exchanges. The Library dumps sound banks (`.SYX`).

---

## Documentation de référence

- `SY77_PARAMETERS.md` — référence sysex (format, groupes, adresses) croisée code + spec officielle.
- La spec Yamaha *SY77 MIDI Data Format* et les captures SynthWorks sont sous `docs/` (non versionné).

## Soutien & remerciements

Cette application est et restera **libre**. Pour m'encourager : écoutez un de mes titres
(*Sebastien Portrait* sur Spotify/Deezer — ex. *Ocean Beach Club*).
https://open.spotify.com/artist/3qPhbBJMJSF3AYbvzll5Zm

Un grand merci au **forum JUCE**, en particulier : Daniel, Jules, McMartin, matkatmusic, Xenakios, dqthebt.

*Ps : aucun animal n'a été blessé durant le développement — au contraire, un labrador a partagé
de nombreuses interrogations, et l'agitation de son train arrière n'a laissé aucun doute : « suis content » !*

— Seb
