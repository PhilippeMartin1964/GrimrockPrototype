# WORLDOBJ-MIG09-E2C — Autorité typée des helpers d’édition

Statut : **candidat à validation locale UE5.5.4**

Date : 2026-09-08

## Objectif

Cette tranche retire de nouveaux accès directs au cache transitoire `UGridLevelAsset::Objects` dans les helpers centraux du Grid Editor.

L’autorité persistante reste exclusivement :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

`FGridLevelObjectData` reste temporairement un DTO de compatibilité en valeur jusqu’à la fin de MIG09-E2C. Il ne doit plus être utilisé comme stockage persistant ni comme cible d’écriture.

## Modifications de cette tranche

### Suppression et conflits de placement

`RemoveObjectsAtSelectionInternal()` et `RemoveObjectsConflictingWithPlacementInternal()` ne parcourent plus `LevelAsset->Objects`.

Ils construisent une projection locale avec :

```cpp
LevelAsset->BuildCompatibilityObjectProjectionFromTyped()
```

Les suppressions continuent ensuite à passer par `RemoveObjectById()`, qui atteint l’autorité typée.

### ApplyEditedSelectedObject

`ApplyEditedSelectedObject()` ne modifie plus un élément mutable du cache `Objects`.

Le flux devient :

```text
projection locale depuis autorité typée
-> copie du snapshot sélectionné
-> application des champs édités
-> ApplyGridEditorObjectSnapshotToAuthority()
-> MarkPackageDirty()
-> RebuildPreview()
```

Il n’existe donc plus de write-back direct vers `Objects` dans ce chemin.

### Création d’un nouveau niveau

`CreateAndAddDungeonLevel()` ne réinitialise plus `NewLevelAsset->Objects`.

Un niveau neuf initialise uniquement les cinq collections typées, les cellules, les liens et les données de départ.

## Baseline déjà validée avant cette tranche

```text
Grimrock.WorldObjects
Succeeded              : 34
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

```text
Grimrock.Editor.MON14.3.1
Succeeded              : 2
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

## Validation requise

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Une fois ce filtre vert, la suite E2C porte sur les derniers consommateurs DTO/cache : Inspector, Lua/Validation, fixtures résiduelles et suppression physique finale de `Objects`, `FGridLevelObjectData` et des conversions de compatibilité.
