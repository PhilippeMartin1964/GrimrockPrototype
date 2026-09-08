# WORLDOBJ-MIG09-E2C — Autorité typée des helpers d’édition

Statut : **fixtures Editor migrées vers l’autorité typée — validation locale requise**

Date : 2026-09-08

## Objectif

WORLDOBJ-MIG09-E2C retire les derniers lecteurs et writers du Grid Editor encore dépendants du cache transitoire `UGridLevelAsset::Objects`.

L’autorité persistante de placement est exclusivement :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

`FGridLevelObjectData` reste temporairement un DTO **par valeur** pour certaines politiques Editor/Runtime. Il ne doit plus être utilisé comme stockage persistant, comme cache d’autorité ni comme cible d’écriture.

## 1. Blocs E2C déjà validés

Les chemins suivants sont hors du cache `Objects` :

- mutations centrales du Grid Editor ;
- sélection par cellule et par identifiant ;
- patrol route MonsterSpawn ;
- `GridEditorLinkService` ;
- `SGridEditorWorkspaceTab` ;
- viewport/picking ;
- `SGridEditorLinksPanel` ;
- `FGridLevelEdMode` ;
- `SGridEditorOverviewMapPanel` ;
- `SGridEditorObjectInspectorPanel` ;
- API de sélection par pointeur `GetSelectedObjectData()` supprimée ;
- `GridEditorLuaService` ;
- `AGridLevelEditorActor::ValidateCurrentLevel()`.

Validations locales fournies le 8 septembre 2026 :

```text
Grimrock.WorldObjects
Succeeded              : 34
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

```text
Grimrock.MON19.2.Editor
Succeeded              : 8
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

```text
Grimrock.MON19.6.Editor
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

```text
Grimrock.MON19.7.1.Editor
Succeeded              : 2
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

Le build Development Editor UE5.5.4 est également passé.

## 2. LuaService + Validation

### 2.1 Lookup Lua par valeur

`GridEditorLuaService` ne lit plus directement `LevelAsset->Objects`.

La résolution d’un objet passe par :

```cpp
bool UGridLevelAsset::TryGetCompatibilityObjectSnapshot(
    const FGuid& ObjectId,
    FGridLevelObjectData& OutObject) const;
```

Cette fonction construit sa vue depuis les cinq collections typées. Aucun pointeur vers une projection temporaire n’est exposé au consommateur.

### 2.2 Écriture LogicId dans l’autorité typée

`SetSelectedObjectLogicId()` écrit directement dans le placement typé via :

```cpp
bool UGridLevelAsset::SetTypedPlacementLogicId(
    const FGuid& ObjectId,
    FName NewLogicId);
```

`FGridLooseItemInstance` possède également un `LogicId` persistant afin que l’adressage Lua soit cohérent pour tous les placements authorables.

### 2.3 Validation générale hors cache

`AGridLevelEditorActor::ValidateCurrentLevel()` construit une projection DTO locale depuis :

```cpp
LevelAsset->BuildCompatibilityObjectProjectionFromTyped();
```

Cette projection locale sert uniquement à la validation ; elle n’est ni stockée ni écrite dans `Objects`.

Les objets `Logic` data-only avec `ArchetypeId=None` sont acceptés structurellement ; ils ne dépendent plus d’un filtre Lua basé sur un texte de diagnostic historique.

## 3. Fixtures Editor migrées vers les structures natives

Le bloc courant retire les écritures/lectures explicites de `Objects` dans les fixtures Editor suivantes :

```text
Grimrock.Editor.MonsterSpawn.InspectorAuthoringContract
Grimrock.TechnicalDebt.TD03_2.ObjectInspectorDetails.MoveToCurrentCellContract
Grimrock.TechnicalDebt.TD03_3.ObjectInspectorDetails.DefinitionSyncContract
Grimrock.WorldObjects.MIG07.EditorTypedWriteThrough
Grimrock.MON20.4.RecruitmentUI.PalettePlacement
```

Principes appliqués :

- MonsterSpawn → `FGridMonsterSpawnInstance` / `MonsterSpawns` ;
- objets interactifs réutilisables → `FGridWorldObjectInstance` / `WorldObjectInstances` ;
- items présents physiquement → `FGridLooseItemInstance` / `LooseItemInstances` ;
- StoryCompanion → `FGridLogicObjectInstance` / `LogicObjects` ;
- les assertions relisent l’autorité typée directement.

Le test TD03.3 ne tente plus de fabriquer artificiellement des identifiants redondants `ItemDefinitionId` / `MonsterDefinitionId` qui n’existent plus dans le modèle typé cible. Il vérifie désormais le contrat durable : les références d’assets sont canoniques et les placements créés par la palette arrivent dans la bonne collection typée.

## 4. Validation requise pour le bloc fixtures Editor

Régression principale :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Puis les contrats directement touchés :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Editor.MonsterSpawn.InspectorAuthoringContract"
```

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD03_2.ObjectInspectorDetails"
```

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD03_3.ObjectInspectorDetails"
```

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON20.4.RecruitmentUI.PalettePlacement"
```

## 5. Reste de WORLDOBJ-MIG09-E2C

1. migrer les fixtures Runtime résiduelles encore branchées explicitement sur `Objects` ou sur les conversions legacy ;
2. migrer `ValidateMonsterSpawns()` et les derniers helpers Core/Runtime encore exprimés en DTO lorsque leur frontière peut devenir native ;
3. supprimer physiquement :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
BuildCompatibilityObjectProjectionFromTyped()
TryGetCompatibilityObjectSnapshot()
RebuildTypedPlacementProjectionFromLegacy()
EnableTypedPlacementStorageFromLegacy()
GridLevelPlacementCompatibility.h
GridLevelPlacementConversion::To*
```

4. renforcer les tests anti-régression empêchant toute réintroduction du monolithe.

MIG10 ne commence qu’après suppression physique de ces compatibilités.
