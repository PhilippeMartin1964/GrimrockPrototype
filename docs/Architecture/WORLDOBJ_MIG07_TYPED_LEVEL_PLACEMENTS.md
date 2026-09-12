# WORLDOBJ-MIG07 — Placements de niveau typés

Statut : **migration achevée ; contrat relu après la refonte des états initiaux sémantiques**, 2026-09-12.

## Objectif

MIG07 a remplacé l'ancien placement monolithique par cinq familles natives dans `UGridLevelAsset` :

```text
UGridLevelAsset
└── Placements
    ├── WorldObjectInstances
    ├── LooseItemInstances
    ├── MonsterSpawns
    ├── ItemSpawns
    └── LogicObjects
```

La séparation est sémantique : un objet ne transporte que les champs appartenant à sa famille. Les anciennes projections et collections de compatibilité ont ensuite été supprimées par MIG09.

## Structures actuelles

### `FGridWorldObjectInstance`

Représente une instance d'une définition réutilisable du monde : porte, bouton, levier, plaque de pression, pit, téléporteur, trigger, réceptacle, décoration, lumière, etc.

```text
FGridWorldObjectInstance
├── InstanceId
├── WorldObjectDefinitionId
├── Type
├── CellX / CellY
├── WallSide
├── LocalTransformOverride optionnel
├── LogicId
├── Tag / Notes / PaletteEntryId
├── ReadableTextOverride
└── InstanceConfig
```

`InstanceConfig` contient les données naturellement locales au niveau et les rares overrides autorisés :

```text
FGridWorldObjectInstanceConfig
├── bDoorInitiallyOpen
├── bTeleporterInitiallyEnabled
├── Teleporter
├── Transition
├── Pit
├── ReceptacleInitialContent
├── InteractionOverrides
├── MovingPartOverrides
├── DoorChainMode / ChainPullDuration override
└── bStartsUnlocked
```

Un world object présent dans `WorldObjectInstances` existe dans le niveau. Il n'utilise plus de booléen générique pour exprimer son existence ou son état actif. Les états initiaux authorés sont propres au type : porte ouverte/fermée, téléporteur activé/désactivé, pit ouvert/fermé, serrure verrouillée/déverrouillée. Un levier démarre au repos/Off. Une plaque de pression démarre relâchée et son état effectif est dérivé au runtime de l'occupation et du poids.

Les règles générales de plaque, serrure, réceptacle, chaîne de porte, motion, spatialité et audio restent dans la définition, sauf les overrides d'instance explicitement prévus.

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
└── metadata d'instance
```

La référence d'item est directe : aucun `WorldObjectDefinition` compagnon n'est réintroduit. Sa présence dans `LooseItemInstances` signifie que l'item est placé dans le niveau.

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
├── bSpawnAtStart
└── metadata d'instance
```

`SpawnId` est l'identité stable utilisée pour la persistance. `bSpawnAtStart=false` signifie que le placement existe mais que son Actor n'est pas créé au démarrage ; il pourra apparaître ensuite via les commandes de spawn/encounter.

`InitialMonsterState=Dormant` a une autre signification : le monstre est créé au démarrage lorsque `bSpawnAtStart=true`, mais commence dans l'état `Dormant`.

### `FGridItemSpawnInstance`

```text
FGridItemSpawnInstance
├── SpawnId
├── ItemDefinition
├── Quantity
├── CellX / CellY
├── bSpawnAtStart
└── règles/metadata du générateur
```

Invariant important :

```text
LooseItemInstance != ItemSpawnInstance
```

Le premier est un item présent. Le second est un générateur ; `bSpawnAtStart` contrôle uniquement la génération initiale du second.

### `FGridLogicObjectInstance`

Regroupe les cibles data-only : logique, recrutement narratif et autres cibles sans acteur runtime.

```text
FGridLogicObjectInstance
├── InstanceId
├── LogicId
├── Type
├── CellX / CellY
├── Logic
├── StoryCompanionDefinition
└── Tag / Notes / PaletteEntryId
```

La présence dans `LogicObjects` suffit à définir l'existence de la cible logique. Son état mutable éventuel appartient à sa configuration logique ou au runtime persistant, pas à un booléen générique de placement.

## Autorité Definition / Instance

`UGridWorldObjectDefinitionAsset` ne fournit plus de defaults génériques d'existence ou d'activité. La définition porte le concept partagé : présentation, motion, audio, placement, comportement par défaut et classe runtime.

Le niveau porte seulement ce qui varie réellement par placement. Ce découpage évite qu'un même booléen signifie successivement « existe », « actif », « ouvert », « pressé », « On » ou « doit spawner » selon le type.

## Garde-fous

Les tests `Grimrock.WorldObjects.MIG07` et `Grimrock.WorldObjects.InitialState.SemanticContract` vérifient notamment :

- l'existence des cinq collections dans `UGridLevelAsset` ;
- la classification indépendante `LooseItem` / `ItemSpawn` ;
- l'identité stable de chaque famille ;
- les références directes `ItemDefinition` / `MonsterDefinition` ;
- le maintien du patrol et de l'encounter dans `MonsterSpawns` ;
- le maintien de `Transition`, `Pit`, contenu initial de réceptacle et état initial de serrure dans `InstanceConfig` ;
- `bSpawnAtStart` sur les générateurs de monstres et d'items ;
- l'absence de propriétés génériques d'état initial sur les placements persistés ;
- l'absence de mélange entre les cinq familles.

## Héritage de migration

La séquence historique a été :

```text
MIG07-A  nouvelles structures + projection + tests
MIG07-B  runtime/editor basculent vers les structures typées
MIG08    conversion/réenregistrement des .uasset réels
MIG09    suppression physique du modèle monolithique et des ponts
MIG10    consolidation des définitions world-object
```

Ce document décrit désormais le résultat de cette migration plutôt que les ponts temporaires utilisés pendant son exécution.
