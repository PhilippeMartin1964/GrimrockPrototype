# WORLDOBJ-MIG09-E2C — Autorité typée des helpers d’édition

Statut : **bloc Lua/Validation candidat à validation locale UE5.5.4**

Date : 2026-09-08

## Objectif

WORLDOBJ-MIG09-E2C retire les derniers lecteurs et writers du Grid Editor encore dépendants du cache transitoire `UGridLevelAsset::Objects`.

L’autorité persistante de placement reste exclusivement :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

`FGridLevelObjectData` reste temporairement un DTO **par valeur** pour certaines politiques Editor/Runtime. Il ne doit plus être utilisé comme stockage persistant, comme cache d’autorité ni comme cible d’écriture.

## 1. Blocs E2C déjà validés

Les chemins suivants sont maintenant hors du cache `Objects` :

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
- API de sélection par pointeur `GetSelectedObjectData()` supprimée.

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

Le build Development Editor UE5.5.4 est également passé.

## 2. Bloc courant — LuaService + Validation

### 2.1. Lookup Lua par valeur

`GridEditorLuaService` ne lit plus directement `LevelAsset->Objects`.

La résolution d’un objet passe par :

```cpp
bool UGridLevelAsset::TryGetCompatibilityObjectSnapshot(
    const FGuid& ObjectId,
    FGridLevelObjectData& OutObject) const;
```

Cette fonction construit sa vue depuis les cinq collections typées. Aucun pointeur vers une projection temporaire n’est exposé au consommateur.

### 2.2. Écriture LogicId dans l’autorité typée

`SetSelectedObjectLogicId()` n’écrit plus dans un DTO/cache legacy.

Le writer :

```cpp
bool UGridLevelAsset::SetTypedPlacementLogicId(
    const FGuid& ObjectId,
    FName NewLogicId);
```

résout directement l’identifiant dans :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

`FGridLooseItemInstance` possède maintenant lui aussi un `LogicId` persistant, afin que le contrat d’adressage Lua soit cohérent pour tous les placements authorables.

### 2.3. Validation générale hors cache

`AGridLevelEditorActor::ValidateCurrentLevel()` construit une seule projection DTO locale depuis :

```cpp
LevelAsset->BuildCompatibilityObjectProjectionFromTyped();
```

Cette projection locale alimente :

- les identités et diagnostics par objet ;
- les règles de placement ;
- les contrôles MonsterSpawn ;
- les transitions/pits ;
- les conflits de partage cellule/ancre ;
- la résolution source/cible des links ;
- les diagnostics Trigger/Receptacle ;
- la localisation des messages de validation.

Le contrôle d’un pit de destination construit également sa projection directement depuis l’autorité typée du niveau cible.

Il n’existe donc plus de lecture directe de `LevelAsset->Objects` dans `GridLevelEditorActor_Validation.inl`.

### 2.4. Fixtures Lua migrées

Les fixtures suivantes utilisent maintenant les structures natives :

```text
Grimrock.MON19.6.Editor
Grimrock.MON19.7.1.Editor
```

Les boutons/portes de test sont des `FGridWorldObjectInstance` et les nœuds Logic des `FGridLogicObjectInstance`. Les assertions `LogicId` relisent directement `WorldObjectInstances`.

## 3. Validation requise pour ce bloc

Régression principale :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Lua Editor :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19.6.Editor"
```

Puis :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19.7.1.Editor"
```

Ce bloc ne sera marqué **validé** qu’après retour des résultats UE5.5.4 locaux.

## 4. Reste de WORLDOBJ-MIG09-E2C après ce bloc

1. migrer les fixtures/tests résiduels encore branchés explicitement sur `Objects` ou sur les conversions legacy ;
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

4. ajouter/renforcer les tests anti-régression empêchant toute réintroduction du monolithe.

MIG10 ne commence qu’après suppression physique de ces compatibilités.
