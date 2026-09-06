# WORLDOBJ-MIG07 — Placements de niveau typés

Statut : MIG07-A — fondation de schéma C++.

## Objectif

`FGridLevelObjectData` est devenu un conteneur historique trop large : il transporte en même temps des données de WorldObject, item, monstre, logique, lecture, patrol, encounter et comportement d'instance.

MIG07 remplace progressivement ce monolithe par les cinq familles prévues dans l'architecture cible :

```text
UGridLevelAsset
└── Placements
    ├── WorldObjectInstances
    ├── LooseItemInstances
    ├── MonsterSpawns
    ├── ItemSpawns
    └── LogicObjects
```

La séparation est sémantique : un objet ne transporte plus les champs appartenant aux autres familles.

## Structures introduites

### `FGridWorldObjectInstance`

Représente une instance d'une définition réutilisable du monde : porte, bouton, levier, plaque de pression, pit, téléporteur, trigger, réceptacle, décoration, lumière, etc.

```text
FGridWorldObjectInstance
├── InstanceId
├── WorldObjectDefinitionId
├── Type                   [pont jusqu'à MIG10]
├── CellX / CellY
├── WallSide
├── LocalTransformOverride optionnel
├── bInitiallyEnabled
├── bInitiallyActive
├── LogicId
├── Tag / Notes / PaletteEntryId
├── ReadableTextOverride
└── InstanceConfig
```

`InstanceConfig` ne contient que les données naturellement locales au niveau :

```text
FGridWorldObjectInstanceConfig
├── Teleporter
├── Transition
├── Pit
├── ReceptacleInitialContent
└── bStartsUnlocked
```

Les règles générales de plaque, serrure, réceptacle, chaîne de porte, animation, spatialité, audio, etc. restent dans la définition conformément à MIG06.

### `FGridLooseItemInstance`

Un item réellement présent dans le niveau :

```text
FGridLooseItemInstance
├── InstanceId
├── ItemDefinition
├── Quantity
├── CellX / CellY
├── SurfaceSide
├── LocalOffset / LocalYaw
├── Readable overrides
└── état/metadata d'instance
```

La référence d'item est directe : aucun `WorldObjectDefinition` compagnon n'est réintroduit.

### `FGridMonsterSpawnInstance`

```text
FGridMonsterSpawnInstance
├── SpawnId
├── MonsterDefinition
├── CellX / CellY
├── Facing
├── InitialMonsterState
├── PatrolMode / PatrolWaypoints
├── EncounterGroupId
├── EncounterWaveIndex
└── état/metadata d'instance
```

Le `SpawnId` correspond au rôle historique de `ObjectId` pour la persistance MON13.

### `FGridItemSpawnInstance`

```text
FGridItemSpawnInstance
├── SpawnId
├── ItemDefinition
├── Quantity
├── CellX / CellY
└── règles/metadata du générateur
```

Invariant important :

```text
LooseItemInstance != ItemSpawnInstance
```

Le premier est un item présent. Le second est un générateur.

### `FGridLogicObjectInstance`

Regroupe les cibles data-only : logique, recrutement narratif et autres cibles sans acteur runtime.

```text
FGridLogicObjectInstance
├── InstanceId
├── LogicId
├── Type
├── CellX / CellY
├── InitialState
├── Logic
├── StoryCompanionDefinition
└── Tag / Notes / PaletteEntryId
```

## MIG07-A : projection depuis le schéma historique

Le runtime et le Grid Editor utilisent encore `UGridLevelAsset::Objects` comme autorité durant cette première tranche.

`UGridLevelAsset::RebuildTypedPlacementProjectionFromLegacy()` permet de projeter explicitement le monolithe vers le modèle cible :

```text
FGridLevelObjectData.Type
        │
        ├─ Item          -> LooseItemInstances
        ├─ MonsterSpawn  -> MonsterSpawns
        ├─ ItemSpawn     -> ItemSpawns
        ├─ Logic/Story   -> LogicObjects
        └─ autres        -> WorldObjectInstances
```

La projection n'est volontairement **pas exécutée automatiquement dans `PostLoad()`**. Les `.uasset` réels ne doivent pas être migrés implicitement avant MIG08.

Les cinq nouvelles collections sont donc `VisibleAnywhere` pendant MIG07-A : elles décrivent et testent le schéma cible, mais ne constituent pas encore une seconde interface d'authoring concurrente.

## Pourquoi conserver temporairement `Objects`

MIG08 est l'étape explicitement réservée à la conversion et au réenregistrement des Data Assets Unreal réels. Supprimer `Objects` avant cette migration rendrait les assets existants illisibles ou imposerait un fallback caché.

La séquence voulue est :

```text
MIG07-A  nouvelles structures + projection + tests
MIG07-B  runtime/editor consomment les structures typées
MIG08    conversion/réenregistrement des .uasset réels
MIG09    suppression physique de FGridLevelObjectData/Objects et des ponts
```

## Garde-fous

Les tests `Grimrock.WorldObjects.MIG07` vérifient :

- l'existence des cinq collections dans `UGridLevelAsset` ;
- la classification indépendante `LooseItem` / `ItemSpawn` ;
- la projection de l'identité stable ;
- la référence directe ItemDefinition/MonsterDefinition ;
- le maintien du patrol et de l'encounter dans `MonsterSpawns` ;
- le maintien de `Transition`, `Pit`, contenu initial de réceptacle et état initial de serrure dans `InstanceConfig` ;
- l'absence de mélange entre les cinq familles.

## Suite MIG07-B

La seconde tranche doit faire basculer progressivement les consommateurs vers les collections typées et construire, uniquement lorsque nécessaire pour la compatibilité MIG08, une projection legacy temporaire vers `FGridLevelObjectData`.

Critère de fermeture MIG07 : le runtime et le Grid Editor n'ont plus besoin de considérer `FGridLevelObjectData` comme leur modèle conceptuel principal. `Objects` ne subsiste alors que comme mécanisme de migration des anciens assets jusqu'à MIG09.
