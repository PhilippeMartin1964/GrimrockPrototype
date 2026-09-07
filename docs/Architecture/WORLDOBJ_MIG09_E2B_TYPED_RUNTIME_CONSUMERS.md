# WORLDOBJ-MIG09-E2B — Runtime Typed Consumers

Statut : **E2A validé ; E2B en cours** — 2026-09-07.

## Validation E2A

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

## Bloc DoorSystem

`UGridDoorSystemComponent` indexe désormais directement :

```text
UGridLevelAsset::WorldObjectInstances
```

Les opérations suivantes ne lisent plus le cache `Objects` :

- détection d'une porte sur une arête ;
- détection d'une porte secrète par `WorldObjectDefinitionId` ;
- capture/restauration de l'état d'une porte ;
- reconstruction de l'index des portes.

La map `DoorIndexByEdge` contient désormais des indices dans `WorldObjectInstances`.

Un wrapper `RegisterDoorObject(FGridLevelObjectData, ...)` reste temporairement uniquement parce que `AGridLevelRuntimeActor` n'a pas encore basculé son orchestration de spawn vers les instances typées. Il délègue immédiatement à `FGridRuntimeWorldObjectData` et sera supprimé avec les wrappers E2.

## Suite E2B

La prochaine macro-tranche migre ensemble :

```text
AGridLevelRuntimeActor
UGridActivationComponent
UGridMonsterEncounterComponent
GridLevelRuntimeActorPersistence
GridLevelRuntimeActorDiagnostics
GridEditorPreviewComponent (runtime preview)
```

Puis `Receptacle`, `WallLock` et `PitTrapdoor` sont alignés sur la frontière runtime native si leurs derniers appels dépendent encore du DTO legacy.

## Sortie E2B

```text
[ ] aucune lecture runtime directe de LevelAsset->Objects
[ ] WorldObjectInstances utilisé par le runtime world-object
[ ] LooseItemInstances utilisé par les items monde
[ ] MonsterSpawns utilisé par monstres/encounters/persistence
[ ] Activation et diagnostics hors DTO legacy
[ ] 0 Failed sur Grimrock.WorldObjects
```
