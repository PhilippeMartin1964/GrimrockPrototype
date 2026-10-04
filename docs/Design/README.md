# GrimrockPrototype Design Docs

Ce dossier conserve les décisions, contrats, jalons et validations du projet. Les documents datés restent historiques ; l’état courant est défini par les documents de synthèse ci-dessous.

## Ordre de lecture actif — 4 octobre 2026

1. `00_PROJECT_OVERVIEW.md` — état courant et baseline validée.
2. `PROJECT_COMPLETION_ROADMAP.md` — backlog fonctionnel actif.
3. `../Architecture/PROJECT_SYNTHESIS.md` — synthèse transversale.
4. `../Architecture/ARCHITECTURE_INDEX.md` — index des contrats courants.
5. `../Architecture/Maps/GRIMROCK_PROJECT_MAP.md` — carte détaillée rebaselinée.
6. `../Architecture/Maps/GRIMROCK_PROJECT_MAP_MERMAID.md` — vues visuelles.
7. `DOC_CLOSURE01_REBASELINE_DOCUMENTATION.md` — clôture documentaire de la baseline du 4 octobre.
8. `99_DECISIONS_LOG.md` — décisions durables.

## Baseline validée

```text
Runtime/content commit : 9045ef2db75c09997db4fc65dbf99d4598f4df5c
Global Automation      : 1026 / 1026
Warnings               : 0
Failed                 : 0
Not run                : 0
Shipping               : VALIDÉ
```

Le commit DOC-CLOSURE01 suivant ne modifie que la documentation.

## Jalons courants

| Famille | Statut |
|---|---|
| MON13–MON20 | **Clos / validés** |
| MON21.1 | **Clos** |
| MON21.2 | **Validé** |
| MON21.3 | **Validé** |
| MON21.4 | **En attente** |
| MON21.5 | **À faire** |
| MON21.6 Map | **Validé / clos** |
| MAP-THEME01 | **Validé / clos** |
| MON21.7 | **À faire** |
| MON21.8 | **À faire** |
| MON22 Vertical Slice | **À faire** |

Clôtures techniques récentes : `CPP-CLEAN01`, `RUNTIME-TRANSFORM-DIAG01`, `FINAL-MASTER-CLOSURE`.

## Règle de priorité documentaire

1. baseline courante et document de clôture le plus récent ;
2. `PROJECT_COMPLETION_ROADMAP.md` ;
3. `00_PROJECT_OVERVIEW.md` ;
4. `PROJECT_SYNTHESIS.md` / `ARCHITECTURE_INDEX.md` ;
5. tickets historiques datés.

Git conserve l’historique ; un document ancien n’est pas réécrit sauf lorsqu’il prétend encore représenter l’état courant.
