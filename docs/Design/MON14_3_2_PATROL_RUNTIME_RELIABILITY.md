# MON14.3.2 — Patrol Runtime Reliability

## Problème

MON14.3 savait exécuter une route, mais le premier traitement d'un monstre dépendait principalement d'une évaluation de perception. Les tests historiques appelaient directement `ProcessMonsterNow()`, donc ils ne validaient pas le bootstrap réel d'un `MonsterSpawn` authored.

## Contrat

```text
SpawnAtStart = true
InitialMonsterState = Idle
PatrolMode = Loop ou PingPong
PatrolWaypoints >= 2
        ↓
runtime/party prêts
        ↓
BootstrapRuntimeExploration
        ↓
déplacement case par case MON3/MON4
```

Aucune entrée joueur n'est nécessaire.

## Architecture

- `UGridMonsterPatrolSubsystem` reste l'orchestrateur.
- `UGridMonsterMovementComponent` reste l'autorité du mouvement et de l'occupation.
- `UGridMonsterBehaviorComponent` reste l'autorité du pathfinding et de la perception.
- aucun Tick IA permanent ;
- aucune modification du LevelAsset ;
- aucune modification de Blueprint/`.uasset`.

Deux événements peuvent amorcer le même runtime de manière idempotente :

1. application/restauration de l'état runtime ;
2. fin du `BeginPlay` du Party Pawn.

Le second couvre l'ordre de démarrage où le RuntimeActor est construit avant que le Party Pawn soit prêt.

## Route bloquée

Si aucun chemin n'existe vers le waypoint courant, le monstre ne triche pas : il reste sur place et réessaie toutes les 0,25 s.

Une ligne de diagnostic unique identifie alors la cible :

```text
[MON14.3.2] Patrol path unavailable Monster=... Cell=(x,y) TargetIndex=... TargetCell=(x,y) Action=Retry
```

## Tests

```text
Grimrock.Monsters.MON14.3.RuntimeBootstrap
Grimrock.Monsters.MON14.3.CursorRules
```

Le premier interdit un retour à un démarrage manuel via `ProcessMonsterNow()`. Le second verrouille la séquence PingPong à quatre points.


## Correction complémentaire — autorité LevelAsset de la route

Le diagnostic PIE `Patrol bootstrap ... Eligible=0` a révélé qu'un Actor runtime pouvait ne plus porter la copie de route attendue alors que le `MonsterSpawn` de l'éditeur contenait encore `PingPong` et ses waypoints.

Le contrat est désormais explicite :

- `FGridMonsterSpawnInstance::PatrolMode` et `PatrolWaypoints` sont l'autorité authored ;
- ces données ne sont pas persistées dans `FGridRuntimeMonsterState` ;
- `AGridLevelRuntimeActor::ApplyMonsterPlacementMetadata()` recopie toujours `EncounterGroupId`, `PatrolMode` et `PatrolWaypoints` depuis le LevelAsset ;
- le bootstrap MON14.3.2 resynchronise ces métadonnées avant toute décision d'exploration.

Le log de bootstrap distingue maintenant la présence d'une route de son activité runtime :

```text
[MON14.3.2] Patrol candidate Monster=... State=... Mode=PingPong Waypoints=4 Cell=(...)
[MON14.3.2] Patrol bootstrap ... Authored=1 Active=1
```

Le test `Grimrock.Monsters.MON14.3.AuthoredRouteResync` efface volontairement la copie de route de l'Actor puis vérifie que le bootstrap la recharge depuis le LevelAsset et démarre le mouvement.
