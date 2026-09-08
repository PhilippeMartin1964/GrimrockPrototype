# WORLDOBJ-MIG09-E2C — Autorité typée des helpers d’édition

Statut : **fixtures Editor validées ; premier lot Runtime migré — validation locale requise**

Date : 2026-09-08

## Objectif

WORLDOBJ-MIG09-E2C retire les derniers lecteurs et writers du Grid Editor et des fixtures encore dépendants du cache transitoire `UGridLevelAsset::Objects`.

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

Fixtures Editor migrées et validées :

```text
Grimrock.Editor.MonsterSpawn.InspectorAuthoringContract : 1 / 0 / 0
Grimrock.TechnicalDebt.TD03_2.ObjectInspectorDetails    : 1 / 0 / 0
Grimrock.TechnicalDebt.TD03_3.ObjectInspectorDetails    : 1 / 0 / 0
Grimrock.MON20.4.RecruitmentUI.PalettePlacement         : 1 / 0 / 0
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

Les objets `Logic` data-only avec `ArchetypeId=None` sont acceptés structurellement.

## 3. Fixtures Editor migrées vers les structures natives

Les fixtures Editor suivantes utilisent maintenant directement les collections typées :

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

## 4. Premier lot de fixtures Runtime migré

Le lot courant retire le stockage persistant `Objects` de trois familles Runtime :

```text
Grimrock.MON19.4.LuaBridge
Grimrock.MON19.7.1.LuaAuthoring.LogicIdCommand
Grimrock.Monsters.Perception.AcousticHearing
```

### MON19.4 LuaBridge

- Trigger source → `FGridWorldObjectInstance` ;
- nœuds Relay / AddInt / SetBool → `FGridLogicObjectInstance` ;
- aucune construction de chaîne logique via `Objects`.

### MON19.7.1 LogicId runtime

- Trigger source et Lever cible → `WorldObjectInstances` ;
- `LogicId` du Lever est stocké directement sur `FGridWorldObjectInstance`.

### AcousticHearing

- monstre dormant → `MonsterSpawns` ;
- portes normales/secrètes → `WorldObjectInstances` ;
- `FGridLevelObjectData` n’est conservé que comme DTO local pour les helpers de test `GridDoorTestUtils` / `RegisterDoorObject`, jamais comme stockage de niveau.

## 5. Validation requise pour ce lot Runtime

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19.4.LuaBridge"
```

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19.7.1.LuaAuthoring.LogicIdCommand"
```

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.Perception.AcousticHearing"
```

Puis la régression générale :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

## 6. Reste de WORLDOBJ-MIG09-E2C

1. migrer les autres fixtures Runtime résiduelles encore branchées explicitement sur `Objects` ;
2. traiter les tests historiques MIG07 qui vérifient encore volontairement les conversions legacy ;
3. migrer `ValidateMonsterSpawns()` et les derniers helpers Core/Runtime encore exprimés en DTO lorsque leur frontière peut devenir native ;
4. supprimer physiquement :

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

5. renforcer les tests anti-régression empêchant toute réintroduction du monolithe.

MIG10 ne commence qu’après suppression physique de ces compatibilités.
