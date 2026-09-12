# MON13.1 — Modèle persistant `MonsterSpawn`

Statut : **contrat courant après WORLDOBJ-MIG10 et refonte des états initiaux sémantiques**, 2026-09-12.

## Objectif

MON13.1 définit la représentation persistante d'un générateur de monstre dans `UGridLevelAsset` :

```text
FGridMonsterSpawnInstance
    → UGridMonsterDefinitionAsset
    → AGridMonsterActor au runtime
```

Le jalon historique a précédé les migrations typées. Le contrat actuel n'utilise plus `FGridLevelObjectData`, `UGridLevelAsset::Objects`, `bInitiallyEnabled`, `bInitiallyActive`, `MonsterDefinitionId`, `Edge` ni `LocalYaw` pour un `MonsterSpawn`.

## Autorité persistante actuelle

Les générateurs de monstres sont stockés exclusivement dans :

```text
UGridLevelAsset::MonsterSpawns
```

Chaque entrée est un `FGridMonsterSpawnInstance` :

```text
SpawnId
MonsterDefinition
CellX / CellY
Facing
InitialMonsterState
PatrolMode / PatrolWaypoints
EncounterGroupId / EncounterWaveIndex
bSpawnAtStart
LogicId
Notes / PaletteEntryId
```

`SpawnId` est l'identité persistante stable du placement. Il est réutilisé par le runtime, la persistance, les liens et `ResolvePersistenceId()` ; aucun second GUID n'est créé.

La définition est une référence directe `MonsterDefinition`. Il n'existe plus de miroir `MonsterDefinitionId` dans le placement.

## État initial sémantique

Le modèle distingue explicitement deux notions :

- `bSpawnAtStart` : le générateur crée ou non son monstre lors de l'entrée initiale dans le niveau ;
- `InitialMonsterState` : état du monstre lorsqu'il est créé, actuellement `Idle` ou `Dormant` pour l'authoring initial validé.

Ainsi :

```text
Spawn at Start = false
    => aucun Actor au démarrage

Spawn at Start = true + InitialMonsterState = Idle
    => Actor présent et actif

Spawn at Start = true + InitialMonsterState = Dormant
    => Actor présent mais dormant
```

Il n'existe plus de `Enabled at Start` ou `Active at Start` générique pour `MonsterSpawn`.

## Placement et orientation

Un spawn est centré sur une cellule. Sa position gameplay est décrite uniquement par `CellX`, `CellY` et `Facing`.

`Facing` doit être cardinal :

```text
North / East / South / West
```

Aucun `Edge` mural ni `LocalYaw` persistant n'appartient au placement typé.

## Définition du monstre

`UGridMonsterDefinitionAsset` porte les données réutilisables du monstre, notamment `MonsterId`, les statistiques, la présentation et `MonsterActorClass`.

L'identité de l'instance n'appartient jamais à la définition : elle reste exclusivement `SpawnId` dans le niveau.

Pour un monstre de production, `MonsterActorClass` doit désigner la classe gameplay réellement équipée des composants attendus par le projet, par exemple `BP_MON_RatGiant` pour le Rat géant.

## Palette et Grid Editor

Une entrée de palette destinée à placer un monstre doit fournir `DefaultMonsterDefinition`. Le Grid Editor crée alors directement un `FGridMonsterSpawnInstance` et conserve la référence à la définition de monstre.

Les contrôles d'instance pertinents sont désormais :

- `SpawnId` en lecture seule ;
- `Monster Definition` ;
- cellule ;
- orientation cardinale ;
- `Spawn at Start` ;
- `Initial Monster State` ;
- groupe et vague de rencontre ;
- patrol lorsque configuré.

Les anciens contrôles `MonsterDefinitionId`, `Enabled at Start` et `Active at Start` ne font plus partie du contrat.

## Validation

`UGridLevelAsset::ValidateMonsterSpawns()` vérifie notamment :

- `SpawnId` valide et unique à travers toutes les collections typées du niveau ;
- cellule dans les limites, non vide et autorisant l'occupation ;
- absence de deux monstres `bSpawnAtStart=true` sur la même cellule ;
- `Facing` cardinal ;
- `InitialMonsterState` initial valide ;
- `MonsterDefinition` présente et valide ;
- patrol cohérent et waypoints valides ;
- `EncounterWaveIndex >= 0` ;
- une vague future nécessite un `EncounterGroupId` et ne doit pas être `bSpawnAtStart=true` ;
- absence de deux placements d'une même vague sur la même cellule.

Les identités des autres familles (`WorldObjectInstances`, `LooseItemInstances`, `ItemSpawns`, `LogicObjects`) participent au même espace d'identifiants : un `SpawnId` de monstre ne peut pas entrer en collision avec elles.

## Tests Automation

Le socle MON13.1 est couvert notamment par :

```text
Grimrock.Monsters.MON13.1.PersistentModel
Grimrock.Monsters.MON13.1.Validation
Grimrock.Monsters.MON13.1.TypedAuthority
Grimrock.Monsters.MON13.1.PaletteContract
```

Le contrat sémantique global est également verrouillé par :

```text
Grimrock.WorldObjects.InitialState.SemanticContract
```

Ce test vérifie notamment que `FGridMonsterSpawnInstance` expose `bSpawnAtStart` et ne réintroduit pas `bInitiallyEnabled`.

Validation locale recommandée :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON13.1"
```

## Checklist éditeur

1. Ouvrir la définition du monstre et vérifier `MonsterId`, `MonsterActorClass`, présentation et validation.
2. Vérifier que l'entrée de palette référence la bonne `DefaultMonsterDefinition`.
3. Placer deux monstres sur deux cellules libres différentes.
4. Vérifier que leurs `SpawnId` sont valides, différents et stables après sauvegarde/rechargement.
5. Tester les quatre orientations cardinales.
6. Tester `Spawn at Start` sur un des deux placements.
7. Tester `Initial Monster State = Idle` puis `Dormant`.
8. Vérifier `EncounterGroupId`, `EncounterWaveIndex` et les règles de vague.
9. Lancer `Refresh Validation` et confirmer qu'aucune erreur MON13.1 n'est produite pour un contenu valide.

### Cas négatifs utiles

Sur une copie de niveau uniquement :

- supprimer `MonsterDefinition` ;
- dupliquer un `SpawnId` ;
- utiliser un `Facing` non cardinal ;
- placer le spawn hors grille ou sur une cellule bloquée ;
- placer deux spawns `Spawn at Start` sur la même cellule ;
- définir une vague future avec `Spawn at Start=true`.

Chaque cas doit produire une validation explicite sans mutation silencieuse du niveau.

## Suite

MON13.2 décrit le pipeline d'aperçu et d'instanciation runtime. MON13.3 décrit les commandes `Spawn`, `Despawn` et `Teleport` ainsi que leur persistance.
