# ARM-CLEAN06.4X.55 — Préflight complet de la palette native (lecture seule)

**Parent binaire :** `51d3a1e4d15d3560bf77c17ab158d4df934c87c2`. X.54B : quatre DataAssets convertis, 1 960 réussites sans avertissement, 30 avec avertissements, zéro échec. SilentTask et LocalAuto succès sur ce SHA ; recette PIE 8/8 confirmée par le propriétaire. Le périmètre X.48 comporte désormais 59 pointeurs historiques.

## Objectif

Préparer la suppression de 33 `DefaultItemDefinition` de `DA_ObjectPalette_Default`, sans toucher à son ordre, ses catégories, ses EntryIds, ses IDs d'objets, ses niveaux physiques, ses icônes de mécanismes ou à ses autres entrées.

- `FGridEditorNativePalettePlan::TryBuild` construit une proposition **transitoire** complète ; aucune sauvegarde ou modification native. Il exige exactement 33 entrées directes et 33 ou 0 pointeurs (état mixte refusé), des IDs enregistrés, des niveaux physiques valides, la cohérence des anciennes références, la validation structurelle avant/après et la parité de nom/description/icône avant/après retrait.
- Le test `Grimrock.RPG.ARM.CLEAN06_4X55.NativePalettePointerlessDryRun` charge les vrais assets, compare chaque champ réfléchi de chaque entrée et l'ordre de la palette, contrôle l'original intact, l'idempotence et les refus (N0 ARM, ID inconnu, cardinalité inattendue, conversion partielle). Ce test ne réalise aucune écriture.
- X.28 et X.44 couvrent déjà les placements pointerless en éditeur. X.14 couvre la vue canonique Slate. Pas de nouveau texte WBP, pas de changement UI.csv/ST_UI.

## Barrières avant la transaction native

Les tests historiques X.11/X.13/X.14/4W/X.2/X.4/X.48 ainsi que PALETTE_CLEAN01 et plusieurs tests ARMEMENT vérifient encore les anciens pointeurs sur le vrai asset. Il faut aligner leurs attentes sur des identités canoniques sans affaiblir la vérification de parité, et adapter les inspecteurs historiques des pipelines à la lecture des entrées pointerless, avant toute écriture. Ne pas relancer les anciens pipelines de création. Un commandlet natif protégé et un runner Windows avec sauvegarde privée, vérification en processus séparé et publication LFS bornée seront ensuite nécessaires.

**Aucun .uasset, .umap, SaveGame ou UPROPERTY retiré.** Le total reste 59 ; 26 seulement après conversion réelle des 33 entrées. ARM-CLEAN06.4 partielle ; 06.5 bloquée. Exiger les deux validations Windows du SHA de ce lot avant le suivant.
