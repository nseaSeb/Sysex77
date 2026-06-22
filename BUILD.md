# Compiler Sysex77 depuis les sources

Sysex77 se compile **sans Projucer** grâce à CMake. C'est la voie recommandée, en
particulier sur **Windows / Linux** (où l'app pré-compilée n'est pas fournie).

**JUCE est résolu automatiquement** — dans l'ordre :
1. variable d'environnement / option `JUCE_DIR` (copie locale de JUCE), sinon
2. `/Applications/JUCE` (install Projucer sur macOS), sinon
3. **téléchargement automatique** de JUCE 8.0.7 (CMake FetchContent) — rien à installer à la main.

---

## Prérequis

| Plateforme | À installer |
|-----------|-------------|
| **Tous**   | CMake ≥ 3.22, Git |
| **macOS**  | Xcode + outils en ligne de commande (`xcode-select --install`) |
| **Windows**| Visual Studio (charge de travail « Développement Desktop en C++ ») |
| **Linux**  | `build-essential` + dépendances JUCE (voir ci-dessous) |

**Dépendances Linux (Debian/Ubuntu) :**
```bash
sudo apt install build-essential cmake git libasound2-dev libjack-jackd2-dev \
  libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
  libxinerama-dev libxrandr-dev libxrender-dev libwebkit2gtk-4.1-dev \
  libglu1-mesa-dev libcurl4-openssl-dev
```

---

## Compiler

### macOS / Linux
```bash
./build.sh            # Release dans ./build
./build.sh --debug    # Debug
./build.sh --test     # compile puis lance les tests unitaires
```

### Windows
```bat
build.bat
build.bat --debug
build.bat --test
```

### CMake directement (toutes plateformes)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Utiliser une copie locale de JUCE (évite le téléchargement) :
```bash
cmake -B build -DJUCE_DIR=/chemin/vers/JUCE      # ou:  JUCE_DIR=... ./build.sh
```

---

## Lancer / tester

Binaire produit :
- macOS : `build/Sysex77_artefacts/<Config>/Sysex77.app`
- Linux : `build/Sysex77_artefacts/<Config>/Sysex77`
- Windows : `build\Sysex77_artefacts\<Config>\Sysex77.exe`

Tests unitaires (logique sysex « pure ») :
```bash
<binaire> --test     # affiche "=== UnitTests: N/N assertions passed ... ==="
```

---

## Dépannage

- **« JUCE introuvable » / lenteur au 1er `configure`** : c'est le téléchargement de JUCE
  (FetchContent). Pour l'éviter, passez `-DJUCE_DIR` vers une copie locale.
- **Linux : erreurs `*.h not found` à la compilation de JUCE** : il manque une dépendance de
  dev (voir la liste ci-dessus).
- **Windows : `cmake` ou compilateur introuvable** : ouvrez un « Developer Command Prompt for
  VS » (qui met cmake/MSVC dans le PATH), puis relancez `build.bat`.
- **Build alternatif (macOS, historique)** : `Sysex77.jucer` + Xcode sous `Builds/MacOSX`.
