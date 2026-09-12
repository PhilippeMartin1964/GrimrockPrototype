# MON14.2 — Perception directionnelle, état initial et données de patrouille

Statut : **contrat courant sur placements typés**, 2026-09-12.

MON14.2 prolonge MON14.1 sans modifier le TurnManager ni le protocole de démarrage du combat.

Le jalon introduit trois fondations :

1. un état initial `Idle` ou `Dormant` sérialisé par `FGridMonsterSpawnInstance` ;
2. une vision axiale directionnelle qui tient compte du `Facing` courant du monstre ;
3. un modèle de données de patrouille sérialisé, exécuté ensuite par MON14.3.

## Principes conservés de MON14.1

Le raccord automatique reste :

```text
événement runtime
    -> UGridAutomaticPerceptionEngagementSubsystem
    -> évaluation différée/coalescée
    -> StartCombatFromPerception()
```

Une source directe automatique doit **voir** le groupe. L'ouïe seule peut mettre le monstre en `Alert` et mémoriser `LastKnownPartyCell`, mais ne lance pas automatiquement le combat.

Le chemin manuel/diagnostic `StartCombatFromPerception()` conserve son contrat historique vue **ou** ouïe.

Aucun `Tick` IA permanent n'est ajouté.

## 1. État initial du MonsterSpawn

`FGridMonsterSpawnInstance` contient :

```cpp
EGridMonsterState InitialMonsterState = EGridMonsterState::Idle;
bool bSpawnAtStart = true;
```

Les seules valeurs d'authoring initial validées sont :

- `Idle` ;
- `Dormant`.

Les états `Alert`, `Pursuing`, `Attacking`, `Repositioning`, `Hurt` et `Dead` sont des états runtime et ne doivent pas être utilisés comme état de départ d'un placement frais.

### Présence et dormance restent distinctes

```text
bSpawnAtStart = false
    => aucun Actor au démarrage

bSpawnAtStart = true + InitialMonsterState = Dormant
    => Actor présent mais dormant
```

`Dormant` ne doit donc jamais être simulé en supprimant le spawn initial.

Il n'existe plus de `bInitiallyEnabled` ou `bInitiallyActive` générique sur `MonsterSpawn`.

### Fresh game et Continue

Lors d'une création fraîche, le runtime applique la configuration du `FGridMonsterSpawnInstance` possédant le même `SpawnId`, notamment `InitialMonsterState`.

Lors d'un Continue, l'état runtime sauvegardé prévaut sur l'état initial du placement lorsqu'un état persistant existe déjà.

## 2. Champ de vision directionnel

MON4 utilise une géométrie de vue adaptée au dungeon crawler :

- même ligne X ou Y ;
- portée en cellules ;
- chaque edge traversé doit être praticable pour la vue ;
- murs et portes fermées bloquent la vue ;
- pas de vision autour d'un angle.

MON14.2 ajoute le `Facing` comme contrainte supplémentaire.

Pour un monstre en `(X,Y)` :

```text
North : cible sur X identique et Y supérieur
East  : cible sur Y identique et X supérieur
South : cible sur X identique et Y inférieur
West  : cible sur Y identique et X inférieur
```

Le modèle reste volontairement un **rayon cardinal**, pas un cône angulaire. Il est cohérent avec le déplacement case par case et garde les gardes lisibles sans introduire une perception physique continue.

### API pure

`FGridMonsterPerception` conserve :

```cpp
HasStraightLineOfSight(...)
```

comme contrat géométrique indépendant du Facing.

MON14.2 utilise également :

```cpp
IsTargetInFacingDirection(...)
HasDirectionalLineOfSight(...)
```

`UGridMonsterBehaviorComponent::RefreshPerception()` s'appuie sur la ligne de vue directionnelle.

### Ouïe

L'ouïe reste omnidirectionnelle et fondée sur la topologie/distance prévue par le système acoustique du projet. Elle ne déclenche pas à elle seule le combat automatique.

## 3. Données de patrouille

Le placement typé contient :

```cpp
EGridMonsterPatrolMode PatrolMode;
TArray<FGridMonsterPatrolWaypoint> PatrolWaypoints;
```

avec :

```cpp
EGridMonsterPatrolMode
{
    None,
    Loop,
    PingPong
};
```

et :

```cpp
FGridMonsterPatrolWaypoint
{
    FIntPoint Cell;
    EGridEdge Facing;
    float WaitSeconds;
};
```

### Sémantique des waypoints

- `Cell` est la cellule d'arrivée ;
- `Facing=None` signifie que le waypoint n'impose pas d'orientation finale ;
- une direction cardinale impose l'orientation d'arrivée ;
- `WaitSeconds=0` signifie aucune attente ;
- `Loop` revient du dernier waypoint au premier ;
- `PingPong` parcourt la liste dans les deux sens.

La route appartient au `LevelAsset`. Le curseur d'exécution MON14.3 reste runtime/transitoire.

## 4. Validation des MonsterSpawn

`UGridLevelAsset::ValidateMonsterSpawns()` vérifie notamment :

- `InitialMonsterState` = `Idle` ou `Dormant` ;
- `Facing` du spawn cardinal ;
- une patrouille active (`Loop` ou `PingPong`) avec au moins deux waypoints ;
- chaque waypoint dans la grille ;
- cellule non vide et autorisant l'occupation ;
- `Facing` de waypoint = `None` ou cardinal ;
- `WaitSeconds` fini et positif ou nul.

Une liste de waypoints peut rester stockée avec `PatrolMode=None`, ce qui permet de désactiver temporairement une route sans perdre son authoring.

## 5. Placement typé et compatibilité conceptuelle

Le modèle courant est :

```text
UGridLevelAsset::MonsterSpawns
    -> FGridMonsterSpawnInstance
```

Il n'existe plus de :

```text
FGridLevelObjectData MonsterSpawn
MonsterDefinitionId
Edge
LocalYaw
bInitiallyEnabled
bInitiallyActive
```

La définition de monstre est référencée directement par `MonsterDefinition`; l'orientation persistée est `Facing`; la présence initiale est `bSpawnAtStart`.

## 6. Édition

Le Grid Editor édite les données directement sur `FGridMonsterSpawnInstance` :

- `MonsterDefinition` ;
- `Facing` ;
- `Spawn at Start` ;
- `InitialMonsterState` ;
- `PatrolMode` ;
- `PatrolWaypoints` ;
- données de rencontre.

L'éditeur visuel de route MON14.3.1 fournit l'authoring de viewport pour les waypoints.

## 7. Tests automatisés

La suite :

```text
Grimrock.Monsters.MON14.2
```

couvre notamment :

- les quatre directions cardinales ;
- l'absence de vision arrière/latérale ;
- la portée et les edges bloquants ;
- le helper géométrique MON4 ;
- la validation `Idle/Dormant` ;
- la validation des routes et waypoints ;
- le transfert de l'état initial, du Facing, du PatrolMode et des waypoints vers un Actor frais ;
- l'intégration réelle du Facing dans `UGridMonsterBehaviorComponent`.

Validation locale recommandée :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON14.2"
```

## 8. Relation avec MON14.3

MON14.3 exécute la véritable exploration :

```text
Idle + PatrolMode != None
    -> chemin vers waypoint
    -> déplacement case par case
    -> orientation d'arrivée
    -> attente
    -> waypoint suivant

vue du groupe
    -> Alert
    -> abandon de patrouille
    -> engagement MON14.1
```

L'orchestration reste événementielle : timers discrets et callbacks de fin de mouvement plutôt qu'une boucle IA permanente dans `Tick`.
