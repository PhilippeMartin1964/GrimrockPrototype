# WORLDOBJ-MIG09-E2B — Runtime Typed Consumers

Statut : **E2A validé ; E2B en cours** — 2026-09-07.

## Validations acquises

### E2A — Runtime World Object Boundary

```text
Build                  : OK
Succeeded              : 33
Succeeded with warnings: 1
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-174259
```

### E2B — DoorSystem typed

```text
Build                  : OK
Succeeded              : 33
Succeeded with warnings: 1
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-175225
```

### E2B — MonsterEncounter + Runtime Preview typed

```text
Build                  : OK
Succeeded              : 33
Succeeded with warnings: 1
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-180920
```

## Règle E2B

Le runtime ne doit plus consulter `UGridLevelAsset::Objects`. Chaque domaine lit sa collection typée native :

```text
World objects   -> WorldObjectInstances
Items au sol    -> LooseItemInstances
Monstres        -> MonsterSpawns
Item generators -> ItemSpawns
Logique         -> LogicObjects
```

`FGridRuntimeWorldObjectData` reste un payload runtime-only légitime pour initialiser les acteurs world-object. Il n'est jamais sérialisé dans `UGridLevelAsset`.

## Bloc DoorSystem — validé

`UGridDoorSystemComponent` indexe désormais directement `UGridLevelAsset::WorldObjectInstances`.

Les opérations suivantes ne lisent plus le cache `Objects` :

- détection d'une porte sur une arête ;
- détection d'une porte secrète par `WorldObjectDefinitionId` ;
- capture/restauration de l'état d'une porte ;
- reconstruction de l'index des portes.

Un wrapper `RegisterDoorObject(FGridLevelObjectData, ...)` reste temporairement uniquement parce que `AGridLevelRuntimeActor` n'a pas encore basculé son orchestration de spawn vers les instances typées.

## Bloc MonsterEncounter — validé

`UGridMonsterEncounterComponent` lit désormais directement `UGridLevelAsset::MonsterSpawns`.

Les opérations suivantes ne parcourent plus `LevelAsset->Objects` :

- résolution d'un `SpawnId` ;
- sélection de la prochaine vague ;
- détection d'une vague vaincue ;
- constitution et tri atomique des membres d'une vague ;
- résolution de l'ancre d'encounter ;
- notification de mort d'un membre.

Pendant cette sous-tranche uniquement, l'appel effectif au spawn et à la capture d'état construit encore un snapshot transitoire via :

```text
GridLevelPlacementCompatibility::ToLegacyMonsterSpawn(...)
```

Ce snapshot n'est jamais stocké dans `UGridLevelAsset`. Il existe uniquement parce que `AGridLevelRuntimeActor::AddMonsterSpawnActor()` et `StoreMonsterPlacementState()` utilisent encore `FGridLevelObjectData`. Ces signatures disparaissent dans la prochaine tranche runtime monstre.

## Bloc Runtime Preview — validé

`UGridEditorPreviewComponent::RebuildPreviewObjects()` ne parcourt plus `LevelAsset->Objects`.

Il lit explicitement :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

Puis il construit seulement le snapshot transitoire attendu par les fonctions de preview historiques. Le snapshot n'est ni sauvegardé ni réinjecté dans `UGridLevelAsset`.

Cette étape garantit déjà que preview et runtime partent de la même autorité typée, avant suppression physique des wrappers `FGridLevelObjectData` en E2C.

## Bloc Diagnostics + Dungeon transitions — candidat courant

Les diagnostics runtime et les diagnostics de transitions du donjon lisent maintenant les collections typées natives.

`GridLevelRuntimeActorDiagnostics` :

```text
TransitionObjects -> WorldObjectInstances[].InstanceConfig.Transition
Placement count   -> UGridLevelAsset::GetTypedPlacementCount()
```

`GridDungeonAsset` :

```text
Transition/Pit diagnostics -> WorldObjectInstances
ObjectId                   -> InstanceId
ArchetypeId                 -> WorldObjectDefinitionId
Edge                        -> WallSide
Transition                  -> InstanceConfig.Transition
Pit                         -> InstanceConfig.Pit
```

Ces deux lecteurs ne dépendent plus de `UGridLevelAsset::Objects`.

## Suite E2B

La suite migre :

```text
AGridLevelRuntimeActor / GridLevelRuntimeActorMonsters
GridLevelRuntimeActorPersistence
UGridActivationComponent
```

Puis `Receptacle`, `WallLock` et `PitTrapdoor` sont alignés sur la frontière runtime native si leurs derniers appels dépendent encore du DTO legacy.

## Sortie E2B

```text
[ ] aucune lecture runtime directe de LevelAsset->Objects
[x] DoorSystem sur WorldObjectInstances
[x] MonsterEncounter sur MonsterSpawns
[x] Runtime Preview lit les cinq collections typées
[x] Runtime diagnostics hors Objects
[x] Dungeon transition diagnostics hors Objects
[ ] LevelRuntimeActor sur collections typées
[ ] LooseItemInstances utilisé par les items monde runtime
[ ] MonsterSpawns utilisé par runtime monstre/persistence
[ ] Activation hors DTO legacy
[ ] 0 Failed sur Grimrock.WorldObjects
```
