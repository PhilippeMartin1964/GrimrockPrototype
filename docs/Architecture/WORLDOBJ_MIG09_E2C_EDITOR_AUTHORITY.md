# WORLDOBJ-MIG09-E2C — Autorité typée des helpers d’édition

Statut : **candidat à validation locale UE5.5.4**

Date : 2026-09-08

## Objectif

Cette tranche retire progressivement les accès directs au cache transitoire `UGridLevelAsset::Objects` et les dépendances aux pointeurs de DTO temporaires dans les helpers centraux du Grid Editor.

L’autorité persistante reste exclusivement :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

`FGridLevelObjectData` reste temporairement un DTO de compatibilité en valeur jusqu’à la fin de MIG09-E2C. Il ne doit plus être utilisé comme stockage persistant ni comme cible d’écriture.

## 1. Bloc déjà validé — mutations Editor hors cache

`RemoveObjectsAtSelectionInternal()`, `RemoveObjectsConflictingWithPlacementInternal()` et `ApplyEditedSelectedObject()` ne lisent/écrivent plus directement `LevelAsset->Objects`.

`CreateAndAddDungeonLevel()` ne réinitialise plus `NewLevelAsset->Objects`.

Validation fournie le 8 septembre 2026 :

```text
Grimrock.WorldObjects
Succeeded              : 34
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

Le build Development Editor UE5.5.4 est également passé.

## 2. Bloc courant — sélection et Lua par valeur

### SelectObjectAtSelection

`SelectObjectAtSelection()` ne parcourt plus `LevelAsset->Objects`.

Il construit une projection locale à partir des collections typées :

```cpp
const TArray<FGridLevelObjectData> CompatibilityObjects =
    LevelAsset->BuildCompatibilityObjectProjectionFromTyped();
```

### Lecture de l’objet sélectionné

Une API par valeur est introduite :

```cpp
bool TryGetSelectedObjectData(FGridLevelObjectData& OutObject) const;
```

Elle construit une vue résolue Definition + Instance depuis la projection typée, sans exposer au consommateur un pointeur vers une projection temporaire.

L’ancienne API pointeur :

```cpp
const FGridLevelObjectData* GetSelectedObjectData() const;
```

reste temporairement comme wrapper de compatibilité pour le seul gros consommateur UI restant : `SGridEditorObjectInspectorPanel`.

### Panneau Lua

`SGridEditorLuaScriptsPanel` n’utilise plus directement l’API pointeur comme source de ses bindings. Les chemins suivants consomment désormais un snapshot local par valeur :

```text
RebuildBindingOptions()
BuildBindingsSection()
OnCreateBindingClicked()
```

## 3. Baseline déjà validée avant le bloc courant

```text
Grimrock.WorldObjects       : 34 / 0 warning / 0 échec
Grimrock.Editor.MON14.3.1  : 2  / 0 warning / 0 échec
Grimrock.MON19.2.Editor     : 8  / 0 warning / 0 échec
```

## 4. Validation requise pour le bloc courant

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Puis :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19.2.Editor"
```

## 5. Reste de WORLDOBJ-MIG09-E2C

Après validation de ce bloc :

1. migrer `SGridEditorObjectInspectorPanel` vers `TryGetSelectedObjectData()` puis supprimer `GetSelectedObjectData()` ;
2. sortir `GridEditorLuaService` et `GridLevelEditorActor_Validation.inl` des derniers accès directs à `Objects` ;
3. migrer les fixtures/tests résiduels ;
4. supprimer physiquement `Objects`, `FGridLevelObjectData`, `GridLevelPlacementCompatibility` et les conversions/projections transitoires devenues inutiles.
