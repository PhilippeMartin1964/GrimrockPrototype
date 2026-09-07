# WORLDOBJ-MIG09-E2B — Runtime Typed Consumers

Statut : **E2B-FINAL validé localement** — 2026-09-07.

Commit validé :

```text
2f817837e93df9848bf3d96485858f407c6a933e
WORLDOBJ-MIG09 finish E2B typed runtime consumers
```

## But de la tranche

E2B retire le runtime du cache transitoire `UGridLevelAsset::Objects` sans supprimer encore le DTO `FGridLevelObjectData`. La suppression physique du DTO, du cache `Objects` et des projections de compatibilité appartient à E2C avec le Grid Editor et les anciens tests.

L'autorité persistante reste exclusivement constituée des cinq collections typées :

```text
World objects   -> WorldObjectInstances
Items au sol    -> LooseItemInstances
Monstres        -> MonsterSpawns
Item generators -> ItemSpawns
Logique         -> LogicObjects
```

## Résultat E2B-FINAL

Le runtime ne dépend plus de la mise à jour préalable du miroir `Objects` :

- `AGridLevelRuntimeActor::RebuildLevel()` ne rafraîchit plus le cache legacy pour alimenter le runtime ;
- `UGridActivationComponent` construit son index local depuis une projection en valeur créée directement à partir des collections typées ;
- la géométrie spécialisée, les wall locks, transitions et pits sont résolus depuis cette projection typée temporaire ;
- les transferts d'items à travers les pits ne consultent plus `LevelAsset->Objects` ;
- l'identité de spawn d'un monstre est vérifiée directement dans `MonsterSpawns` ;
- `DoorSystem`, `MonsterEncounter`, diagnostics et preview monstre restent sur leurs contrats typés déjà migrés.

`UGridLevelAsset::BuildCompatibilityObjectProjectionFromTyped()` est volontairement temporaire. Il retourne un tableau de DTO construit depuis l'autorité typée mais ne lit ni ne modifie le cache `Objects`. E2C supprimera ce helper en même temps que `FGridLevelObjectData`.

## Frontière E2B / E2C

E2B-FINAL signifie :

```text
Runtime -> aucune dépendance fonctionnelle au miroir LevelAsset->Objects
LevelAsset persistent -> collections typées uniquement
DTO legacy -> encore disponible seulement comme pont de valeur temporaire
```

E2C reste responsable de :

```text
Grid Editor sur placements typés
fixtures/tests legacy restants
suppression physique de UGridLevelAsset::Objects
suppression de FGridLevelObjectData
suppression de GridLevelPlacementCompatibility.h
suppression de GridLevelPlacementConversion::To*
suppression des derniers wrappers DTO temporaires
```

## Sortie E2B-FINAL

```text
[x] DoorSystem sur WorldObjectInstances
[x] MonsterEncounter sur MonsterSpawns
[x] Runtime Preview lit les cinq collections typées
[x] Preview monstre utilise FGridMonsterSpawnInstance
[x] Runtime diagnostics hors Objects
[x] Dungeon transition diagnostics hors Objects
[x] LevelRuntimeActor ne dépend plus du miroir Objects
[x] LooseItemInstances utilisé par l'orchestration runtime des items au sol
[x] MonsterSpawns utilisé par runtime monstre/persistence
[x] Activation indexée depuis l'autorité typée
[x] RebuildLevel ne rafraîchit plus Objects pour les consommateurs runtime
[x] build UE5.5.4 du candidat E2B-FINAL validé localement
[x] Grimrock.WorldObjects validé localement sans échec ni warning Automation
```

## Validation locale du 2026-09-07

### `Grimrock.WorldObjects`

```text
Build                  : OK
Succeeded              : 34
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-221214
```

### `Grimrock.Monsters.MON13.3.LifecyclePersistence`

```text
Build                  : OK
Succeeded              : 0
Succeeded with warnings: 1
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-221251
```

Le warning de `LifecyclePersistence` n'est pas détaillé dans la sortie concise fournie. Il n'est donc pas interprété ni masqué ici. Le test est néanmoins validé par le harness : aucun échec et code processus 0.

Le warning UBT `Visual Studio 2022 compiler is not a preferred version` reste distinct des résultats Automation et n'affecte pas la validation E2B.

## Conclusion

**WORLDOBJ-MIG09-E2B est validé.**

La tranche suivante est `WORLDOBJ-MIG09-E2C` : migration du Grid Editor et des fixtures/tests restants, puis suppression physique de `Objects`, `FGridLevelObjectData` et des projections legacy.
