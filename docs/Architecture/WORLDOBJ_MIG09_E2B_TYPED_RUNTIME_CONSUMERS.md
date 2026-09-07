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

### E2B — MonsterEncounter + lecture Runtime Preview typée

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

### E2B — Diagnostics runtime / dungeon typed

Après correction syntaxique minimale du namespace diagnostics :

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
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-182641
```

### E2B — Monster Runtime typed

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
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-184544
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

L'activation d'une vague appelle maintenant directement les signatures runtime `FGridMonsterSpawnInstance`. Le pont `GridLevelPlacementCompatibility::ToLegacyMonsterSpawn(...)` a disparu de `GridMonsterEncounterComponent`.

## Bloc Monster Runtime — validé

Le pipeline natif existe désormais pour :

```text
ResolveMonsterSpawn(FGridMonsterSpawnInstance)
GetMonsterSpawnTransform(FGridMonsterSpawnInstance)
AddMonsterSpawnActor(FGridMonsterSpawnInstance)
StoreMonsterPlacementState(FGridMonsterSpawnInstance)
DespawnMonsterSpawnActor(FGridMonsterSpawnInstance)
```

Les données utilisées sont directement :

```text
SpawnId
MonsterDefinition
CellX / CellY
Facing
EncounterGroupId
```

Les anciennes surcharges `FGridLevelObjectData` restent temporairement uniquement pour les derniers appels de l'orchestrateur principal et du preview historique. Elles doivent disparaître avant la clôture E2B/E2C.

## Bloc Runtime Preview — candidat courant

`UGridEditorPreviewComponent::RebuildPreviewObjects()` lit déjà explicitement :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

Le chemin `MonsterSpawns` est maintenant migré plus loin : il appelle directement `ResolveMonsterSpawn(FGridMonsterSpawnInstance)` et `GetMonsterSpawnTransform(FGridMonsterSpawnInstance)`, puis initialise le preview monstre avec `SpawnId` sans construire de `FGridLevelObjectData`.

Les quatre autres catégories conservent encore leur projection de preview historique ; cette projection sera retirée avec le DTO legacy en E2C.

## Bloc Diagnostics + Dungeon transitions — validé

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

## Suite E2B

La suite migre l'orchestration de `AGridLevelRuntimeActor` et la persistence vers les structures typées natives, puis élimine les wrappers monstre legacy. Ensuite :

```text
UGridActivationComponent
Receptacle / WallLock / PitTrapdoor
LooseItemInstances runtime
```

## Sortie E2B

```text
[ ] aucune lecture runtime directe de LevelAsset->Objects
[x] DoorSystem sur WorldObjectInstances
[x] MonsterEncounter sur MonsterSpawns
[x] Runtime Preview lit les cinq collections typées
[x] Preview monstre utilise directement FGridMonsterSpawnInstance
[x] Runtime diagnostics hors Objects
[x] Dungeon transition diagnostics hors Objects
[ ] LevelRuntimeActor sur collections typées
[ ] LooseItemInstances utilisé par les items monde runtime
[~] MonsterSpawns utilisé par runtime monstre/persistence ; orchestration principale encore à migrer
[ ] Activation hors DTO legacy
[ ] 0 Failed sur Grimrock.WorldObjects pour le candidat courant
```