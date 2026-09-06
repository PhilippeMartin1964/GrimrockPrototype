# WORLDOBJ-MIG07-B — Autorité des placements typés et miroir de compatibilité

Statut : candidat de validation UE5.5.4 — 2026-09-06.

## 1. But

MIG07-A a introduit les cinq collections cibles :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

MIG07-B introduit la notion d'**autorité de stockage**. Le but est que le niveau puisse déclarer ces collections typées comme source de vérité sans obliger tous les consommateurs historiques à abandonner immédiatement `FGridLevelObjectData`.

Le principe est :

```text
Typed collections = autorité persistante
        |
        v
compatibility projection
        |
        v
FGridLevelObjectData = vue temporaire legacy
```

`FGridLevelObjectData` n'est donc plus conceptuellement une seconde source lorsque le mode typé est actif.

## 2. Marqueur d'autorité

`UGridLevelAsset` possède désormais :

```cpp
bool bTypedPlacementStorageAuthoritative = false;
```

La valeur `false` est volontairement la valeur par défaut afin que tous les `.uasset` existants restent des assets legacy jusqu'à MIG08.

Aucune migration automatique des assets existants n'est effectuée par MIG07-B.

## 3. Bascule explicite

Le helper :

```cpp
EnableTypedPlacementStorageFromLegacy()
```

réalise une bascule explicite en trois étapes :

1. projection de `Objects` vers les cinq collections typées ;
2. activation du marqueur d'autorité ;
3. reconstruction de `Objects` comme miroir de compatibilité.

Cette méthode est destinée aux tests MIG07 puis à la procédure de conversion MIG08. Elle n'est pas appelée implicitement sur les assets réels pendant MIG07.

## 4. Projection inverse

Le nouveau header :

```text
Core/GridLevelPlacementCompatibility.h
```

contient les projections typées vers l'ancien contrat :

```text
FGridWorldObjectInstance   -> FGridLevelObjectData
FGridLooseItemInstance     -> FGridLevelObjectData
FGridMonsterSpawnInstance  -> FGridLevelObjectData
FGridItemSpawnInstance     -> FGridLevelObjectData
FGridLogicObjectInstance   -> FGridLevelObjectData
```

Ces conversions n'introduisent aucune nouvelle donnée de définition dans l'instance.

Pour un WorldObject, le `Behavior` de compatibilité ne contient que les valeurs d'instance validées par MIG06 :

```text
Teleporter
Transition
Pit
Receptacle.InitialContent
Lock.bStartsUnlocked
```

Les règles partagées continuent de provenir de `WorldObjectDefinition.DefaultBehavior` via le resolver MIG06.

## 5. Miroir `Objects`

Le helper :

```cpp
RefreshLegacyObjectMirrorFromTyped()
```

reconstruit `Objects` uniquement depuis les collections typées lorsque le marqueur d'autorité est actif.

Le helper :

```cpp
GetObjectCompatibilityView()
```

rafraîchit ce miroir avant de l'exposer aux consommateurs historiques.

Conséquence : si un ancien consommateur continue temporairement à manipuler un `FGridLevelObjectData`, il peut recevoir une représentation issue de la vraie structure typée sans que cette représentation redevienne une autorité d'authoring.

## 6. Sparse Behavior

En mode typé, `UsesSparseBehaviorOverrides(ObjectId)` ne dépend plus de l'ancien `SparseBehaviorOverrideObjectIds` pour les WorldObjects. Il considère directement qu'un `FGridWorldObjectInstance` est sparse par construction.

Le miroir reconstruit néanmoins `SparseBehaviorOverrideObjectIds` afin de préserver les consommateurs MIG06 historiques qui le consultent encore directement.

## 7. Cas particuliers

### Loose Item

Le collectible conserve une référence directe à `UGridItemDefinitionAsset`, conformément à MIG05.

### Monster Spawn

Le `SpawnId`, la définition, le facing, l'état initial, la patrouille et l'encounter sont reconstruits dans la vue legacy. `LocalYaw` n'est qu'un miroir dérivé du facing cardinal.

### Item Spawn

`LooseItemInstance` et `ItemSpawnInstance` restent séparés. Le vieux monolithe ne possède pas encore un contrat capable d'exprimer toutes les futures règles de spawn typées, notamment une quantité supérieure à 1. Ces règles doivent être consommées directement depuis `FGridItemSpawnInstance` avant la purge MIG09.

### LocalOffset de LooseItem

Le schéma typé prévoit `LocalOffset`, mais l'ancien `FGridLevelObjectData` ne possède pas de champ équivalent générique. La vue legacy ne peut donc pas transporter ce futur offset. Le runtime direct typé devra le consommer avant l'utilisation de valeurs non nulles dans les assets réels.

## 8. Tests

MIG07-B ajoute :

```text
Grimrock.WorldObjects.MIG07.TypedAuthorityBridge
Grimrock.WorldObjects.MIG07.TypedAuthoritySchema
```

Le test principal vérifie notamment :

- bascule explicite vers l'autorité typée ;
- cinq collections comme source de vérité ;
- `WorldObjectInstance` reconnu comme sparse ;
- destruction volontaire du vieux `Objects` puis reconstruction complète depuis le typé ;
- conservation d'une transition, d'un état initial de serrure, d'un ItemDefinition et d'un MonsterSpawn ;
- modification d'une donnée typée suivie immédiatement par la vue de compatibilité.

## 9. Limite volontaire de MIG07-B

MIG07-B ne prétend pas encore avoir supprimé tous les accès directs à `LevelAsset->Objects`.

La tranche suivante doit brancher le lifecycle runtime et le Grid Editor sur le miroir/les collections typées de manière systématique :

```text
MIG07-C
├── PostLoad / runtime compatibility refresh
├── Add / Remove / edit write-through
├── Grid Editor selection / inspector write-through
├── runtime/indexes centraux
└── tests de niveau typé sans autorité legacy
```

MIG08 pourra ensuite convertir les `.uasset` réels en toute sécurité.
