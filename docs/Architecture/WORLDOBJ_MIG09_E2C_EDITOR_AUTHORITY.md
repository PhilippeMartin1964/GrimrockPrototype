# WORLDOBJ-MIG09-E2C — Autorité typée des helpers d’édition et fixtures

Statut : **premier lot Runtime validé ; TD01.3.2 migré vers l’autorité typée — validation locale requise**

Date : 2026-09-08

## Objectif

WORLDOBJ-MIG09-E2C retire les derniers lecteurs et writers du Grid Editor, des helpers et des fixtures encore dépendants du cache transitoire `UGridLevelAsset::Objects`.

L’autorité persistante de placement est exclusivement :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

`FGridLevelObjectData` reste temporairement un DTO **par valeur** pour certaines politiques Editor/Runtime. Il ne doit plus être utilisé comme stockage persistant, comme cache d’autorité ni comme cible d’écriture.

## 1. État validé

### 1.1 Editor

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

Validations locales déjà fournies le 8 septembre 2026 :

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

### 1.2 Premier lot Runtime — validé localement

Le commit `5be706ab` a migré les fixtures suivantes :

```text
Grimrock.MON19.4.LuaBridge
Grimrock.MON19.7.1.LuaAuthoring.LogicIdCommand
Grimrock.Monsters.Perception.AcousticHearing
```

Résultats fournis le 8 septembre 2026 :

```text
Grimrock.MON19.4.LuaBridge
Succeeded              : 4
Succeeded with warnings: 1
Failed                 : 0
Process exit code       : 0
```

```text
Grimrock.MON19.7.1.LuaAuthoring.LogicIdCommand
Succeeded              : 0
Succeeded with warnings: 1
Failed                 : 0
Process exit code       : 0
```

```text
Grimrock.Monsters.Perception.AcousticHearing
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

Régression générale après ce lot :

```text
Grimrock.WorldObjects
Succeeded              : 34
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

Les deux filtres MON19 classés `Succeeded with warnings` n’ont produit aucun échec et ont terminé avec un code de sortie `0`. Le résumé Automation fourni ne détaille pas ici la nature de ces warnings ; ils ne sont donc pas requalifiés artificiellement.

## 2. LuaService + Validation

### 2.1 Lookup Lua par valeur

`GridEditorLuaService` ne lit plus directement `LevelAsset->Objects`.

La résolution d’un objet passe encore temporairement par :

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

## 4. Fixtures Runtime migrées

### 4.1 Premier lot

`MON19.4`, `MON19.7.1` et `AcousticHearing` n’utilisent plus `Objects` comme stockage de niveau.

- Trigger/Lever/Door → `FGridWorldObjectInstance` ;
- Relay/AddInt/SetBool → `FGridLogicObjectInstance` ;
- MonsterSpawn → `FGridMonsterSpawnInstance` ;
- les DTO `FGridLevelObjectData` encore présents dans `AcousticHearing` sont uniquement locaux aux helpers de porte et ne servent pas de stockage persistant.

### 4.2 Deuxième lot — TD01.3.2 RuntimeHardening

La fixture :

```text
Grimrock.TechnicalDebt.TD01_3.EventCommandContract.RuntimeHardening
```

est maintenant construite directement sur les collections typées.

Avant :

```text
Trigger     -> Level->Objects
Teleporter -> Level->Objects
ItemSpawn  -> Level->Objects
Light      -> Level->Objects
```

Après :

```text
Trigger     -> WorldObjectInstances / FGridWorldObjectInstance
Teleporter -> WorldObjectInstances / FGridWorldObjectInstance
ItemSpawn  -> ItemSpawns / FGridItemSpawnInstance
Light      -> WorldObjectInstances / FGridWorldObjectInstance
```

Le helper de fixture `MakeTD0132Object()` basé sur `FGridLevelObjectData` est supprimé. Il est remplacé par deux constructeurs natifs :

```text
MakeTD0132WorldObject()
MakeTD0132ItemSpawn()
```

Le contrat fonctionnel du test ne change pas : il vérifie toujours que les commandes `Activate` / `Deactivate` interdites par la politique `StateOnly` sont rejetées sans modifier l’état runtime des cibles.

## 5. Validation locale requise pour le lot TD01.3.2

Exécuter d’abord le filtre directement touché :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD01_3.EventCommandContract.RuntimeHardening"
```

Puis la régression générale :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Le build Development Editor UE5.5.4 est inclus par le harness `ValidateUE.ps1`.

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

## 7. Definition of Done MIG09

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
