#!/usr/bin/env bash
# =============================================================================
# Sysex77 — script de build (macOS / Linux)
#
# Compile l'application via CMake. Pas besoin de Projucer.
# JUCE est résolu automatiquement :
#   1) variable JUCE_DIR si fournie, sinon
#   2) /Applications/JUCE (macOS), sinon
#   3) téléchargé automatiquement (FetchContent) — aucune install requise.
#
# Usage :
#   ./build.sh              # build Release dans ./build
#   ./build.sh --debug      # build Debug
#   ./build.sh --test       # build puis lance les tests unitaires
#   JUCE_DIR=/path ./build.sh   # utiliser une copie locale de JUCE
#
# Linux : installer au préalable les dépendances de dev JUCE, ex. (Debian/Ubuntu)
#   sudo apt install build-essential cmake libasound2-dev libjack-jackd2-dev \
#     libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
#     libxinerama-dev libxrandr-dev libxrender-dev libwebkit2gtk-4.1-dev \
#     libglu1-mesa-dev libcurl4-openssl-dev
# =============================================================================
set -euo pipefail

BUILD_TYPE="Release"
RUN_TESTS=0
BUILD_DIR="build"

for arg in "$@"; do
    case "$arg" in
        --debug)   BUILD_TYPE="Debug" ;;
        --release) BUILD_TYPE="Release" ;;
        --test)    RUN_TESTS=1 ;;
        --no-bump) ;;                  # accepté (compat release.sh) — l'auto-bump n'existe plus
        -h|--help) sed -n '2,30p' "$0"; exit 0 ;;
        *) echo "Argument inconnu : $arg" >&2; exit 1 ;;
    esac
done

if ! command -v cmake >/dev/null 2>&1; then
    echo "Erreur : cmake introuvable. Installez CMake (>= 3.22)." >&2
    exit 1
fi

# Plus d'auto-bump par build : la version (Source/Version.h) ne change QU'AUX RELEASES
# (release.sh). Chaque build reste identifiable par le timestamp de versionString()
# (« build <date heure> »). Ainsi le numéro de dev (X.Y.Z-dev = prochaine release en cours)
# ne dépasse plus la dernière release, et les releases avancent proprement de +0.0.1 / +0.1.0.

CMAKE_ARGS=(-B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE")
if [[ -n "${JUCE_DIR:-}" ]]; then
    CMAKE_ARGS+=(-DJUCE_DIR="$JUCE_DIR")
fi

echo "==> Configuration ($BUILD_TYPE)…"
cmake "${CMAKE_ARGS[@]}"

echo "==> Compilation…"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

# Localise le binaire produit (chemin différent selon plateforme/générateur).
# IMPORTANT : préférer l'artefact du BUILD_TYPE courant — sinon `find … | head -1` peut
# renvoyer un binaire d'un AUTRE type (ex. Release périmé après une release) et les tests
# tourneraient sur du code obsolète (faux « N/N passed »).
APP_BIN="$(find "$BUILD_DIR" -type f \( -name Sysex77 -o -name Sysex77.exe \) \
            -path "*artefacts/$BUILD_TYPE/*" \( -path '*MacOS*' -o -path '*artefacts*' \) 2>/dev/null | head -1 || true)"
[[ -z "$APP_BIN" ]] && APP_BIN="$(find "$BUILD_DIR" -type f \( -name Sysex77 -o -name Sysex77.exe \) \
            \( -path '*MacOS*' -o -path '*artefacts*' \) 2>/dev/null | head -1 || true)"
[[ -z "$APP_BIN" ]] && APP_BIN="$(find "$BUILD_DIR" -type f -name 'Sysex77*' -perm -u+x 2>/dev/null | head -1 || true)"

echo "==> Build terminé."
[[ -n "$APP_BIN" ]] && echo "    Binaire : $APP_BIN"

if [[ "$RUN_TESTS" -eq 1 ]]; then
    if [[ -z "$APP_BIN" ]]; then
        echo "Erreur : binaire introuvable pour les tests." >&2
        exit 1
    fi
    echo "==> Tests unitaires…"
    "$APP_BIN" --test
fi
