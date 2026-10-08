# GrimrockPrototype — Vue d’ensemble du projet

## Objectif

GrimrockPrototype est un dungeon crawler Unreal Engine 5.5.4 en C++ inspiré de *Legend of Grimrock 2* : vue subjective case par case, Grid Editor, mécanismes data-driven, Logic/Lua, IA de monstres, combat tactique, groupe RPG, progression, magie et, à terme, création de niveaux par les joueurs.

## État courant — 4 octobre 2026

Baseline runtime/content canonique validée :

```text
master == origin/master
9045ef2db75c09997db4fc65dbf99d4598f4df5c
```

Validation finale :

```text
Grimrock
Succeeded               : 1026
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code        : 0

Win64 Shipping
Build + Cook + Stage + Package + Pak + Archive : OK
Cook : 0 error / 0 warning
AutomationTool ExitCode : 0
```

Le commit `DOC-CLOSURE01` qui suit cette baseline est **documentation-only** : il ne modifie ni C++, ni Blueprint, ni DataAsset, ni map, ni contenu runtime.

## Jalons majeurs

```text
MON13–MON20                    CLOS / VALIDÉS
MON21.1                        CLOS
MON21.2 Quest Runtime          VALIDÉ
MON21.3 Quest Event->Command   VALIDÉ
MON21.4 Quest Persistence      EN ATTENTE
MON21.5 Journal                À FAIRE
MON21.6 Map                    VALIDÉ — CLOS
MAP-THEME01                    VALIDÉ — CLOS
MON21.7 Codex                  À FAIRE
MON21.8 Cross-System Closure   À FAIRE
MON22 Vertical Slice           À FAIRE
```

Les clôtures techniques récentes sont également acquises : `CPP-CLEAN01`, `RUNTIME-TRANSFORM-DIAG01` et `FINAL-MASTER-CLOSURE`.

## Architecture de référence

```text
GrimrockLua
    ↓
GrimrockPrototype
    ↓
GrimrockPrototypeEditor
```

Principes :

- DataAssets et grille = autorités de conception/logique ;
- Actors runtime = reconstruction transitoire ;
- Event -> Command = bus gameplay ;
- Logic et Lua orchestrent sans créer une voie parallèle ;
- C++ porte logique, calculs, invariants et read models ;
- Blueprint/UMG porte composition, configuration et présentation ;
- aucune compatibilité arrière Save/DataAsset/Blueprint exigée pendant le prototype ;
- SaveGame courant : **v24 exact-match**.

## Map

MON21.6 est clos. La Map possède exploration persistante, secrets filtrés, projection multi-dalles/multi-étages, symboles, navigation étage, zoom/pan/recenter et rendu final texturé data-driven via `UGridMapVisualThemeAsset`, avec fallback procédural sans seconde autorité.

Références : `MON21_6_13_MAP_CLOSURE.md` et `MAP_THEME01_TEXTURED_MAP_RENDERING.md`.

## Documentation courante

Ordre recommandé :

1. `docs/Design/00_PROJECT_OVERVIEW.md`
2. `docs/Design/PROJECT_COMPLETION_ROADMAP.md`
3. `docs/Architecture/PROJECT_SYNTHESIS.md`
4. `docs/Architecture/ARCHITECTURE_INDEX.md`
5. `docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.md`
6. `docs/Architecture/Maps/GRIMROCK_PROJECT_MAP_MERMAID.md`
7. `docs/Design/99_DECISIONS_LOG.md`
8. `docs/Design/DOC_CLOSURE01_REBASELINE_DOCUMENTATION.md`

Les tickets datés plus anciens restent historiques et ne priment pas sur cette baseline lorsqu’un statut a évolué.

## Règle de travail

- travail sur `master` ;
- un ticket = un commit atomique ;
- pas de force-push ;
- tests déclarés verts uniquement avec sortie UE fournie par l’utilisateur ;
- assets binaires modifiés uniquement via Unreal Editor ;
- mise à jour documentaire durable à la clôture d’un jalon.
