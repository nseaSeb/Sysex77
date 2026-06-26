---
name: release
description: Publish a Sysex77 GitHub release (README/ROADMAP up to date, build+tests, dist, tag, asset upload, push) via release.sh. Use when asked to release, publish a version, cut a release, or ship a new version of Sysex77.
---

# Publier une release Sysex77

Rituel complet de publication d'une version : on s'assure que la **doc est à jour**, on **build +
teste**, puis `release.sh` fait dist + tag + GitHub Release (asset `.app` zippé) + push, et la MAJ
in-app la voit (`Source/Updater.h` lit `…/releases/latest`).

⚠️ **Action publique et difficile à défaire** (release visible sur GitHub). Demander/confirmer le
numéro de version avant de lancer si l'utilisateur ne l'a pas donné explicitement.

## Argument
- **Version** `X.Y.Z` (SemVer figé, ex. `1.4.0`). Si absent : demander. Choisir un numéro
  **supérieur à la dernière release** (`gh release view --json tagName`), pas à la version `-dev`
  locale (le dev grimpe vite et dépasse normalement la release — c'est attendu).

## Pré-vol
1. Être sur `master`, arbre git **propre** (committer/push le travail en cours d'abord).
2. `gh auth status` OK (compte `nseaSeb`).
3. La release `vX.Y.Z` ne doit pas déjà exister (`gh release view vX.Y.Z`).

## Étape 1 — README / ROADMAP à jour
1. Lister ce qui a changé depuis la dernière release :
   `git log --oneline "$(git describe --tags --abbrev=0)"..HEAD` (ou tout l'historique si pas de tag).
2. Si des **fonctionnalités visibles** ont été ajoutées/modifiées, mettre à jour **`README.md`**
   (section « Fonctionnalités » FR **et** le résumé « English ») pour refléter l'état réel.
3. Si l'**UI a notablement changé**, rafraîchir la capture : lancer l'app, aller sur l'écran
   concerné, `screencapture -x -R <x,y,w,h>` (bounds de la fenêtre via System Events) vers
   `divers/<nom>.png`, et vérifier le rendu en la relisant (cf. mémoire « self-verify screencapture »).
4. Ajouter une entrée datée dans **`ROADMAP.md`** (carnet de bord).
5. Committer ces mises à jour de doc **avant** de lancer la release (l'arbre doit être propre pour
   `release.sh`). Pousser.

## Étape 2 — Publier
Lancer (depuis la racine du repo) :
```bash
./release.sh X.Y.Z            # notes auto (gh --generate-notes)
# ou : ./release.sh X.Y.Z -- "Notes de release libres (FR)"
```
`release.sh` enchaîne : fige `Source/Version.h`, `build.sh --no-bump --test` (build Release + tests),
copie dans `dist/Sysex77.app` + signature ad-hoc (arm64), zip `ditto -c -k --keepParent`,
commit + tag annoté + push (tag poussé explicitement), `gh release create` avec l'asset, puis
re-bump `Version.h` en `-dev` + commit + push. **Ne pas refaire ces étapes à la main.**

Si `release.sh` s'arrête en cours (ex. `gh` échoue) : finir manuellement le reste
(`git push origin vX.Y.Z`, `gh release create …`, re-bump `-dev`) — ne pas relancer le script
tel quel (le tag/commit existent déjà).

## Étape 3 — Vérifier et rapporter
- `gh release view vX.Y.Z --json tagName,assets -q '.tagName + " | " + .assets[0].name'`
  → confirmer le tag + l'asset `Sysex77-X.Y.Z.app.zip`.
- Optionnel : `curl -s -H "User-Agent: Sysex77" https://api.github.com/repos/nseaSeb/Sysex77/releases/latest`
  → vérifier `tag_name` + l'asset (ce que l'updater lit).
- Donner l'URL de la release et confirmer que `Version.h` est revenu en `-dev`.

## Notes
- Prérequis : macOS (arm64), `gh` authentifié, `perl`, `ditto`. Voir mémoire « in-app updater & releases ».
- Pour tester la détection in-app : build temporaire en version < release, remettre
  `~/Library/SYSEX77/Sysex77.settings` `LastUpdateCheck` à 0 (throttle 24 h), lancer → le dialogue
  « MAJ disponible » doit apparaître. Restaurer `Version.h` ensuite.
