# WORLDOBJ-MIG09 — Purge des compatibilités legacy

Statut : **MIG09-A à MIG09-E2B validés ; MIG09-E2C en cours ; MIG10 après clôture de MIG09**.

Date de mise à jour : 2026-09-08.

## 1. Référence et règle

Référence architecturale prioritaire :

```text
docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md
```

Règle :

```text
une donnée cible existe
        -> les consommateurs sont migrés
        -> le legacy est supprimé physiquement
        -> un test empêche sa réapparition
```

Contrat cible :

```text
Definition
+ instance typée minimale
+ runtime/save delta
```

## 2. État des tranches

| Tranche | État | Résultat |
|---|---|---|
| MIG09-A | ✅ | Autorité Definition sans marqueur sparse. |
| MIG09-B* | ✅ | Identité Item legacy purgée. |
| MIG09-C | ✅ | `MovingParts[].Motion` est l'autorité visuelle des mécanismes. |
| MIG09-D1 | ✅ | `SparseBehaviorOverrideObjectIds` supprimé. |
| MIG09-D2 | ✅ | Les écritures principales du Grid Editor atteignent l'autorité typée. |
| MIG09-D3 | ✅ | `RebuildPreview()` est read-only. |
| MIG09-E1 | ✅ | Les cinq collections typées sont l'unique autorité persistante. |
| MIG09-E2A | ✅ | Frontière runtime world-object explicite. |
| MIG09-E2B | ✅ validé localement | Runtime spécialisé hors cache `Objects`. |
| MIG09-E2C | 🟨 en cours | Editor/tests puis suppression physique du DTO et des projections. |

## 3. Autorité persistante actuelle

Les seules données persistantes de placement sont :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

`UGridLevelAsset::Objects` n'est plus sérialisé. Il reste uniquement comme cache de compatibilité transitoire jusqu'à la fin d'E2C.

## 4. Validation MIG09-E2B

Validation locale UE5.5.4 du 2026-09-07 :

```text
Grimrock.WorldObjects
Succeeded              : 34
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

```text
Grimrock.Monsters.MON13.3.LifecyclePersistence
Failed                 : 0
Process exit code       : 0
Succeeded with warnings: 1
```

Le warning UBT `Visual Studio 2022 compiler is not a preferred version` est indépendant du résultat Automation.

E2B a notamment sorti du cache `Objects` :

- `AGridLevelRuntimeActor` ;
- world items ;
- lifecycle/persistence MonsterSpawn ;
- Activation ;
- DoorSystem ;
- transitions/pits/locks/diagnostics runtime.

## 5. MIG09-E2C — purge Editor/tests

Objectif final : supprimer physiquement :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
BuildCompatibilityObjectProjectionFromTyped()
RebuildTypedPlacementProjectionFromLegacy()
EnableTypedPlacementStorageFromLegacy()
GridLevelPlacementCompatibility.h
GridLevelPlacementConversion::To*
wrappers runtime FGridLevelObjectData temporaires
```

### 5.1. Ordre E2C

```text
1. Editor de production sur placements typés
2. Tests/fixtures sur structures natives
3. Core helpers encore DTO -> structures natives
4. suppression du cache Objects
5. suppression de FGridLevelObjectData et des conversions
6. tests anti-régression
```

### 5.2. MonsterSpawn authoring natif

Le premier bloc E2C migre les chemins MonsterSpawn qui conservaient encore un round-trip via `FGridLevelObjectData` :

```text
GridMonsterSpawnConfiguration
GridLevelEditorActorPatrolRoute
GridEditorMON14_3_1Tests
```

La cible est directe :

```text
UGridLevelAsset::MonsterSpawns
        -> FGridMonsterSpawnInstance
```

Les opérations de patrouille modifient maintenant directement :

```text
PatrolMode
PatrolWaypoints
```

sur le `FGridMonsterSpawnInstance` sélectionné, sans :

```text
LevelAsset->Objects
FGridLevelObjectData snapshot
AddObject(snapshot)
CommitPatrolEdit()
RefreshLegacyObjectMirrorFromTyped()
GetSelectedObjectData()
```

Le runtime `ApplySpawnPlacementConfiguration()` résout également son placement directement dans `MonsterSpawns`.

Le fixture MON14.3.1 crée et relit désormais exclusivement un `FGridMonsterSpawnInstance`. `LastSelectedObjectId` est positionné comme état de sélection transitoire du test sans reconstruire le miroir `Objects`.

### 5.3. Validation locale du bloc MonsterSpawn E2C

Validation locale UE5.5.4 du 2026-09-08 avant suppression du dernier refresh de compatibilité :

```text
Grimrock.Editor.MON14.3.1
Succeeded              : 2
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

```text
Grimrock.WorldObjects
Succeeded              : 34
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Une relance de `Grimrock.Editor.MON14.3.1` est requise après la suppression du refresh legacy dans le code de patrouille.

### 5.4. Reste E2C

Il reste à migrer :

- sélection/inspection générique du Grid Editor ;
- Overview/Links/EdMode/Lua/Validation encore basés sur le DTO ;
- setters Item/WorldObject/Logic encore construisant des snapshots ;
- fixtures de tests legacy ;
- `ValidateMonsterSpawns()` et derniers helpers Core DTO ;
- suppression finale du cache, du DTO, des conversions et wrappers.

## 6. Definition of Done MIG09

```text
[ ] aucun Objects sérialisé ou transient
[ ] aucun FGridLevelObjectData
[ ] aucune projection legacy <-> typed
[ ] runtime sur structures natives / payload runtime légitime
[ ] Editor sur placements typés
[ ] tests sans fixtures legacy actives
[ ] Grimrock.WorldObjects : 0 Failed
[ ] documentation réconciliée avec la mind map
```

MIG10 ne commence qu'après cette liste entièrement cochée.

## 7. Validation courante

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Pour la tranche MonsterSpawn E2C :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Editor.MON14.3.1"
```

et, pour le lifecycle runtime :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON13.3.LifecyclePersistence"
```
