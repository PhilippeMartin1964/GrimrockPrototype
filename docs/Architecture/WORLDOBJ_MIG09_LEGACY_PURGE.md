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

Les chemins MonsterSpawn suivants sont maintenant sur l'autorité typée :

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

Les opérations de patrouille modifient directement `PatrolMode` et `PatrolWaypoints` sans :

```text
LevelAsset->Objects
FGridLevelObjectData snapshot
AddObject(snapshot)
CommitPatrolEdit()
RefreshLegacyObjectMirrorFromTyped()
GetSelectedObjectData()
```

Le runtime `ApplySpawnPlacementConfiguration()` résout également son placement directement dans `MonsterSpawns`.

Le fixture MON14.3.1 crée et relit exclusivement un `FGridMonsterSpawnInstance`. `LastSelectedObjectId` reste un état de sélection transitoire, sans reconstruction du miroir `Objects`.

### 5.3. Validation locale du bloc MonsterSpawn E2C

Validation locale UE5.5.4 du 2026-09-08 après suppression du dernier refresh legacy de patrouille :

```text
Grimrock.Editor.MON14.3.1
Succeeded              : 2
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

### 5.4. Lecteurs Editor sans état mutable

Les chemins suivants ne lisent plus le cache `Objects` :

```text
GridEditorLinkService
SGridEditorWorkspaceTab
GridLevelEditorActor_InteractionViewport
```

Contrat transitoire E2C :

- `SGridEditorWorkspaceTab` observe `GetTypedPlacementCount()` ;
- `GridEditorLinkService` construit un snapshot local avec `BuildCompatibilityObjectProjectionFromTyped()` pour les politiques de liens encore exprimées en `FGridLevelObjectData` ;
- le picking/hover viewport construit lui aussi un snapshot local depuis les cinq collections typées ;
- aucun de ces chemins ne lit ni ne rafraîchit `UGridLevelAsset::Objects`.

Le fixture MON19.2 `PolicyAndTyping` a lui aussi été migré de `Objects` vers deux `FGridWorldObjectInstance` natifs (`Button` et `Door`).

Validation locale UE5.5.4 du 2026-09-08 :

```text
Grimrock.MON19.2.Editor
Succeeded              : 8
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

### 5.5. Sélection, orientation et déplacement hors cache

Le sous-bloc suivant retire les lectures directes de `Objects` des helpers centraux de sélection et des contrôles d'occupation associés :

```text
AGridLevelEditorActor::FindObjectAtSelection()
AGridLevelEditorActor::FindObjectById()
AGridLevelEditorActor::SetSelectedObjectOrientation()
AGridLevelEditorActor::MoveSelectedObjectToCurrentSelection()
```

Les deux helpers de lookup construisent désormais une vue temporaire en valeur depuis `BuildCompatibilityObjectProjectionFromTyped()` ; ils ne retournent plus un pointeur vers un élément du cache `UGridLevelAsset::Objects`.

Les opérations d'orientation et de déplacement :

1. lisent le snapshot depuis l'autorité typée ;
2. vérifient les conflits sur une projection locale construite depuis les cinq collections typées ;
3. écrivent via `ApplyGridEditorObjectSnapshotToAuthority()` ;
4. ne modifient jamais le cache `Objects` directement.

`FindSelectedObjectMutable()` n'a plus d'implémentation active dans ce chemin et sera supprimé de l'API privée lors du nettoyage final de l'en-tête E2C.

Cette étape reste transitoire : `FGridLevelObjectData` sert encore de vue DTO locale. La suppression du DTO lui-même intervient seulement lorsque les panneaux Inspector/Links/Overview et les setters restants consommeront leurs structures natives.

Validation locale UE5.5.4 du 2026-09-08 après correction des appels à `BuildCompatibilityObjectProjectionFromTyped()` :

```text
Grimrock.WorldObjects
Succeeded              : 34
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Le filtre couvre notamment `Grimrock.WorldObjects.MIG07.EditorTypedWriteThrough`, qui vérifie les écritures typées du Grid Editor, dont le déplacement d'item.

### 5.6. Bandeau EdMode hors API de sélection legacy

`FGridLevelEdModeToolkit::GetSelectedObjectStatusText()` ne passe plus par `AGridLevelEditorActor::GetSelectedObjectData()`.

Le bandeau résout désormais l'objet sélectionné à partir de :

```text
LastSelectedObjectId
+ LevelAsset->BuildCompatibilityObjectProjectionFromTyped()
```

La projection est locale à l'appel Slate ; aucun pointeur n'est conservé et aucun cache `Objects` n'est lu ou rafraîchi.

Ce retrait prépare la suppression physique de `GetSelectedObjectData()` lorsque les panneaux Inspector/Links/Overview/Lua auront été migrés.

Validation locale requise après ce sous-bloc :

```text
Grimrock.WorldObjects
```

### 5.7. Reste E2C

Il reste à migrer :

- suppression de l'API pointeur `GetSelectedObjectData()` au profit de vues typées/natives ;
- `SGridEditorObjectInspectorPanel`, `SGridEditorLinksPanel`, `SGridEditorOverviewMapPanel` et `GridLevelEdMode` encore partiellement exprimés en DTO ;
- `GridEditorLuaService` / panneaux Lua / Validation ;
- setters Item/WorldObject/Logic encore construisant des snapshots DTO ;
- fixtures de tests legacy restantes ;
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

Régression principale :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

MonsterSpawn/patrouille :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Editor.MON14.3.1"
```

Connecteurs/variables Editor :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19.2.Editor"
```
