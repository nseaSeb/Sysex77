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
        -h|--help) sed -n '2,30p' "$0"; exit 0 ;;
        *) echo "Argument inconnu : $arg" >&2; exit 1 ;;
    esac
done

if ! command -v cmake >/dev/null 2>&1; then
    echo "Erreur : cmake introuvable. Installez CMake (>= 3.22)." >&2
    exit 1
fi

# Auto-bump du dernier segment de version AVANT chaque build, pour que la version
# affichée (barre de titre / onglet Setting) corresponde toujours au binaire compilé
# -> on parle toujours du même build (1.2.0-dev -> 1.2.1-dev -> ...). Cf. ROADMAP.
VERSION_FILE="Source/Version.h"
if [[ -f "$VERSION_FILE" ]] && command -v perl >/dev/null 2>&1; then
    perl -i -pe 's{(kVersion\s*=\s*"\d+\.\d+\.)(\d+)(.*?")}{ $1 . ($2 + 1) . $3 }e' "$VERSION_FILE"
    NEWVER="$(perl -ne 'print $1 if /kVersion\s*=\s*"([^"]+)"/' "$VERSION_FILE")"
    echo "==> Version : ${NEWVER:-?} (auto-bump)"
fi

CMAKE_ARGS=(-B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE")
if [[ -n "${JUCE_DIR:-}" ]]; then
    CMAKE_ARGS+=(-DJUCE_DIR="$JUCE_DIR")
fi

echo "==> Configuration ($BUILD_TYPE)…"
cmake "${CMAKE_ARGS[@]}"

echo "==> Compilation…"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

# Localise le binaire produit (chemin différent selon plateforme/générateur).
APP_BIN="$(find "$BUILD_DIR" -type f \( -name Sysex77 -o -name Sysex77.exe \) \
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
