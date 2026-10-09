# GrimrockPrototype Design Docs

Ce dossier conserve décisions, contrats, jalons, validations et historiques du
projet. Les documents datés restent valides pour **leur époque** ; l'état
courant est défini par les références de synthèse ci-dessous.

## Ordre de lecture actif — 9 octobre 2026

1. `00_PROJECT_OVERVIEW.md` — état courant C++ / fonctionnalités.
2. `PROJECT_COMPLETION_ROADMAP.md` — backlog fonctionnel actif.
3. `UI_ARCHITECTURE_CURRENT.md` — architecture UI courante.
4. `../Architecture/PROJECT_SYNTHESIS.md` — synthèse transversale.
5. `../Architecture/ARCHITECTURE_INDEX.md` — index des contrats courants.
6. `../Architecture/Maps/GRIMROCK_PROJECT_MAP.md` — carte détaillée du projet.
7. `99_DECISIONS_LOG.md` — décisions durables.
8. `DOC_AUDIT02_DESIGN_CPP_COHERENCE.md` — audit docs/Design ↔ C++ du 9 octobre.
9. `DOC_CLOSURE01_REBASELINE_DOCUMENTATION.md` — baseline historique du 4 octobre.

## Code courant et dernière baseline globale

Baseline C++ auditée par DOC-AUDIT02 :

```text
6f98a0ef5599f37d3d544529aa5a790d647e8251
```

DOC-AUDIT02 est documentation-only et avance ensuite le HEAD Git sans modifier
la baseline C++ auditée.

La dernière campagne **globale + Shipping** reste celle du 4 octobre :

```text
Runtime/content commit : 9045ef2db75c09997db4fc65dbf99d4598f4df5c
Global Automation      : 1026 / 1026
Warnings               : 0
Failed                 : 0
Not run                : 0
Shipping               : VALIDÉ
```

Les tickets postérieurs ont leurs propres validations ciblées ; aucune nouvelle
campagne globale/Shipping n'est inventée par la documentation.

## Jalons courants

| Famille | Statut |
|---|---|
| MON13–MON20 | **Clos / validés** |
| MON21.1 | **Clos** |
| MON21.2 | **Validé** |
| MON21.3 | **Validé** |
| MON21.4 | **En attente — runtime Quest non persisté** |
| MON21.5 | **À faire** |
| MON21.6 Map | **Validé / clos** |
| MAP-THEME01 | **Validé / clos** |
| MON21.7 | **À faire** |
| MON21.8 | **À faire** |
| MON22 Vertical Slice | **À faire** |
| RPG-SKILL01 | **Validé / clos** |
| RPG-LEVELUX01 | **Validé / clos** |
| RPG-ATTR01 | **Validé / clos** |
| UI-RPG-DESC01 | **Validé / clos** |
| UI-RPG-CODE-AUDIT01 | **Validé / clos** |

SaveGame courant : **v24 exact-match**.

## Règle de priorité documentaire

1. références courantes/rebaselinées ;
2. document de clôture le plus récent d'un système ;
3. roadmap active ;
4. tickets historiques datés.

Un document ancien n'est pas réécrit simplement parce que le projet a évolué.
Lorsqu'un nom comme `CURRENT`, `FINAL` ou `CLOSURE` pourrait induire en
erreur après une évolution d'API, DOC-AUDIT02 ajoute un bandeau
`HISTORIQUE / SUPERSEDED` plutôt que de falsifier le contenu original.
