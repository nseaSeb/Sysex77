#!/usr/bin/env bash
# =============================================================================
# Sysex77 — publication d'une release GitHub (asset .app zippé) pour la MAJ in-app.
#
# Usage :  ./release.sh 1.3.0  [ -- notes de release libres ]
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

VERSION="${1:-}"
shift || true
NOTES="${*:-}"

if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Usage : ./release.sh X.Y.Z [notes...]   (ex. ./release.sh 1.3.0)" >&2
    exit 1
fi
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
git tag "$TAG"
git push --follow-tags origin master

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
