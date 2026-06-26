#!/usr/bin/env bash
# =============================================================================
# Sysex77 — publication d'une release GitHub (asset .app zippé) pour la MAJ in-app.
#
# Usage :  ./release.sh [patch|minor|major|X.Y.Z]  [ -- notes de release libres ]
#   défaut = patch (+0.0.1, correctifs). « minor » (+0.1.0) si une feature est ajoutée.
#   La version est calculée depuis la DERNIÈRE release GitHub (pas depuis la version -dev locale).
#
# Étapes :
#   1) fige Source/Version.h sur la version donnée (release, sans -dev) ;
#   2) build Release (sans auto-bump) + tests ;
#   3) copie dans dist/Sysex77.app + signature ad-hoc (arm64) ;
#   4) zip préservant le bundle (ditto) ;
#   5) commit (Version.h + dist) + tag v<version> + push ;
#   6) gh release create v<version> avec l'asset zippé ;
#   7) re-bump Version.h en <patch+1>-dev (reprise du dev) + commit + push.
#
# Prérequis : gh CLI authentifié (gh auth status), macOS (arm64), perl, ditto.
# =============================================================================
set -euo pipefail
cd "$(dirname "$0")"

# Choix de la version :
#   ./release.sh            -> patch +0.0.1 (correctifs)        [défaut]
#   ./release.sh minor      -> mineur +0.1.0 (feature ajoutée), patch remis à 0
#   ./release.sh major      -> majeur +1.0.0
#   ./release.sh X.Y.Z      -> version explicite (gros jalon)
#   ... [ -- "notes libres" ]
BUMP="patch"
VERSION=""
case "${1:-}" in
    patch|minor|major)        BUMP="$1"; shift ;;
    [0-9]*.[0-9]*.[0-9]*)     VERSION="$1"; shift ;;
esac
[[ "${1:-}" == "--" ]] && shift || true
NOTES="${*:-}"

if [[ -z "$VERSION" ]]; then
    LAST="$(gh release view --json tagName -q .tagName 2>/dev/null | sed 's/^v//')"
    [[ "$LAST" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]] || { LAST="0.0.0"; [[ "0.0.0" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; }
    MA="${BASH_REMATCH[1]}"; MI="${BASH_REMATCH[2]}"; PA="${BASH_REMATCH[3]}"
    case "$BUMP" in
        patch) PA=$((PA + 1)) ;;
        minor) MI=$((MI + 1)); PA=0 ;;
        major) MA=$((MA + 1)); MI=0; PA=0 ;;
    esac
    VERSION="$MA.$MI.$PA"
fi

if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Version invalide. Usage : ./release.sh [patch|minor|major|X.Y.Z] [-- notes]" >&2
    exit 1
fi
echo "==> Version cible : $VERSION (bump: ${BUMP})"
TAG="v$VERSION"
VERSION_FILE="Source/Version.h"
APP="build/Sysex77_artefacts/Release/Sysex77.app"
ZIP="/tmp/Sysex77-$VERSION.app.zip"

command -v gh   >/dev/null || { echo "gh CLI requis (brew install gh)"     >&2; exit 1; }
gh auth status  >/dev/null 2>&1 || { echo "gh non authentifié (gh auth login)" >&2; exit 1; }

if [[ -n "$(git status --porcelain)" ]]; then
    echo "Arbre git non propre — committez/stashez d'abord." >&2; exit 1
fi
if gh release view "$TAG" >/dev/null 2>&1; then
    echo "La release $TAG existe déjà." >&2; exit 1
fi

echo "==> 1) Version $VERSION (figée)"
perl -i -pe "s{(kVersion\\s*=\\s*\")[^\"]+(\")}{\${1}$VERSION\${2}}" "$VERSION_FILE"

echo "==> 2) Build Release + tests (sans auto-bump)"
./build.sh --no-bump --test

echo "==> 3) dist + signature ad-hoc"
rm -rf dist/Sysex77.app
cp -R "$APP" dist/Sysex77.app
codesign --force --deep -s - dist/Sysex77.app
codesign --verify --deep --strict dist/Sysex77.app

echo "==> 4) zip (ditto)"
rm -f "$ZIP"
ditto -c -k --keepParent dist/Sysex77.app "$ZIP"

echo "==> 5) commit + tag + push"
git add "$VERSION_FILE" dist/
git commit -q -m "release : $TAG"
git tag -a "$TAG" -m "Sysex77 $TAG"
git push origin master
git push origin "$TAG"   # push explicite (— follow-tags ne pousse pas les tags légers)

echo "==> 6) GitHub release + asset"
if [[ -n "$NOTES" ]]; then
    gh release create "$TAG" "$ZIP" --title "Sysex77 $TAG" --notes "$NOTES"
else
    gh release create "$TAG" "$ZIP" --title "Sysex77 $TAG" --generate-notes
fi

echo "==> 7) reprise du dev (patch+1 -dev)"
NEXT="$(perl -e '$ARGV[0]=~/^(\d+)\.(\d+)\.(\d+)$/; print "$1.$2.".($3+1)."-dev"' "$VERSION")"
perl -i -pe "s{(kVersion\\s*=\\s*\")[^\"]+(\")}{\${1}$NEXT\${2}}" "$VERSION_FILE"
git add "$VERSION_FILE"
git commit -q -m "reprise dev : $NEXT"
git push origin master

echo "==> Release $TAG publiée. Version de dev : $NEXT"
