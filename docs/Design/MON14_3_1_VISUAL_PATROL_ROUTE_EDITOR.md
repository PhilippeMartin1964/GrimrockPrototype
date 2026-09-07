# MON14.3.1 — Visual Patrol Route Editor

## Statut

Fonctionnalité validée historiquement, puis **migrée en WORLDOBJ-MIG09-E2C vers l'autorité typée `FGridMonsterSpawnInstance`**.

Le Grid Editor édite directement les données persistantes du `MonsterSpawn` dans :

```text
UGridLevelAsset::MonsterSpawns
    -> FGridMonsterSpawnInstance::PatrolMode
    -> FGridMonsterSpawnInstance::PatrolWaypoints
```

Il n'existe plus de round-trip de patrouille via `LevelAsset->Objects`, `FGridLevelObjectData` ou `AddObject(snapshot)`.

## Objectifs

- sélectionner un `MonsterSpawn` existant ;
- afficher sa route dans le viewport même hors mode d'édition ;
- ajouter ou sélectionner des waypoints en cliquant des cellules ;
- supprimer et réordonner les waypoints ;
- éditer `Facing` et `WaitSeconds` ;
- choisir `None`, `Loop` ou `PingPong` ;
- conserver Undo/Redo et le marquage dirty du `LevelAsset` ;
- ne créer aucun second modèle de route ni aucune donnée editor-only persistante.

## Architecture actuelle

```text
GridLevelEdMode
    picking souris / raccourcis / rendu viewport + HUD
            │
            ▼
GridLevelEditorActor
    mutations transactionnelles
            │
            ▼
UGridLevelAsset::MonsterSpawns
            │
            ▼
FGridMonsterSpawnInstance
    PatrolMode + PatrolWaypoints
            │
            ▼
Runtime MonsterSpawn
```

La route est donc une propriété directe du placement typé `MonsterSpawn`. Le viewport n'enregistre aucune copie parallèle.

## Activation

1. passer le Grid Editor en outil `Select` ;
2. sélectionner un `MonsterSpawn` ;
3. appuyer sur `P`.

Le HUD affiche :

```text
PATROL ROUTE EDIT  Mode=...  Waypoints=...
Left click add/select | Delete remove | M mode | F facing |
PgUp/PgDn reorder | -/+ wait | P exit
```

## Règles d'édition

En mode route :

- clic gauche sur une cellule sans waypoint : ajoute un waypoint ;
- clic gauche sur une cellule contenant déjà un waypoint : sélectionne ce waypoint ;
- le premier waypoint reste compatible avec `PatrolMode=None` ;
- dès le deuxième waypoint, une route encore à `None` passe automatiquement à `Loop` ;
- `M` cycle `None -> Loop -> PingPong -> None` ;
- `Loop` et `PingPong` sont refusés avec moins de deux waypoints ;
- `F` cycle `None -> North -> East -> South -> West -> None` ;
- `-` / `+` modifient l'attente par pas de 0,5 s ;
- `PageUp` / `PageDown` réordonnent le waypoint sélectionné ;
- `Delete` / `Backspace` suppriment le waypoint ;
- sous deux waypoints, `PatrolMode` revient à `None`.

## Undo / Redo

Chaque mutation persistante utilise :

```cpp
FScopedTransaction
LevelAsset->Modify()
LevelAsset->MarkPackageDirty()
```

La sélection visuelle d'un waypoint et l'entrée/sortie du mode route sont transitoires.

## Rendu

Lorsqu'un `MonsterSpawn` est sélectionné et que `bShowSelectedMonsterPatrolRoute=true`, la route peut afficher :

- segments entre waypoints ;
- fermeture pointillée pour `Loop` ;
- double direction pour `PingPong` ;
- marqueurs de waypoint ;
- marqueur du waypoint édité ;
- flèches de Facing ;
- numéros projetés dans le HUD.

## Sécurité d'édition

Le mode route n'est disponible que si `LastSelectedObjectId` correspond à un `FGridMonsterSpawnInstance` présent dans `UGridLevelAsset::MonsterSpawns`.

Pendant son activation :

- le clic gauche est capturé par l'éditeur de route ;
- les autres outils de peinture ne sont pas exécutés par erreur ;
- le clic droit reste disponible pour la navigation ;
- quitter le `GridLevelEdMode` désactive le mode route.

## Tests automatisés

Les tests editor-only de référence sont :

```text
Grimrock.Editor.MON14.3.1.PatrolRouteEditingModel
Grimrock.Editor.MON14.3.1.PatrolRouteGuards
```

Validation recommandée après la migration E2C :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Editor.MON14.3.1"
```

Compléter par :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

## Hors périmètre

MON14.3.1 ne modifie pas :

- le pathfinding ;
- la persistance de progression runtime de patrouille ;
- les animations spécifiques de garde ;
- les routes multi-niveaux ;
- le principe de route strictement basée sur les cellules de la grille.
