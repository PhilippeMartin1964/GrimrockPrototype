# MON14.3.1 — Visual Patrol Route Editor

## Statut

Implémentation C++ prête pour compilation et validation sous Unreal Engine 5.5.4.

MON14.3.1 ajoute l'édition visuelle des routes de patrouille directement dans le Grimrock Grid Editor. Le runtime MON14.3 validé n'est pas modifié : l'éditeur écrit uniquement les données `PatrolMode` et `PatrolWaypoints` déjà sérialisées dans `FGridLevelObjectData`.

## Objectifs

- sélectionner un `MonsterSpawn` existant ;
- afficher sa route dans le viewport même hors mode d'édition ;
- ajouter ou sélectionner des waypoints directement en cliquant des cellules ;
- supprimer et réordonner les waypoints ;
- éditer le `Facing` et `WaitSeconds` du waypoint sélectionné ;
- choisir `None`, `Loop` ou `PingPong` ;
- conserver Undo/Redo et le marquage dirty du `LevelAsset` ;
- ne créer aucun second modèle de route ni aucune donnée editor-only persistante.

## Architecture

MON14.3.1 réutilise trois couches existantes :

```text
GridLevelEdMode
    picking souris / raccourcis / rendu viewport + HUD
            │
            ▼
GridLevelEditorActor
    mutations transactionnelles du LevelAsset
            │
            ▼
FGridLevelObjectData
    PatrolMode + PatrolWaypoints
            │
            ▼
MON14.3 runtime inchangé
```

Les données de patrouille restent donc une propriété du `MonsterSpawn`. Le viewport n'enregistre aucune copie parallèle.

## Activation

1. passer le Grid Editor en outil `Select` ;
2. sélectionner un `MonsterSpawn` ;
3. appuyer sur `P`.

Le HUD affiche alors :

```text
PATROL ROUTE EDIT  Mode=...  Waypoints=...
Left click add/select | Delete remove | M mode | F facing |
PgUp/PgDn reorder | -/+ wait | P exit
```

`P` quitte le mode de route sans modifier la sélection du monstre.

## Création et sélection des waypoints

En mode route :

- clic gauche sur une cellule sans waypoint : ajoute un waypoint ;
- clic gauche sur une cellule contenant déjà un waypoint : sélectionne ce waypoint sans le dupliquer ;
- le waypoint sélectionné est dessiné en jaune ;
- les autres waypoints sont dessinés en cyan ;
- les numéros `#1`, `#2`, ... sont projetés dans le HUD.

Le premier waypoint est conservé avec `PatrolMode=None`. Dès que le deuxième waypoint est ajouté, une route encore à `None` devient automatiquement `Loop`.

Cette règle évite de créer artificiellement une patrouille active invalide à un seul waypoint.

## Modes de route

La touche `M` cycle :

```text
None -> Loop -> PingPong -> None
```

Un passage manuel vers `Loop` ou `PingPong` est refusé si la route possède moins de deux waypoints.

### Loop

Le viewport affiche :

```text
#1 -> #2 -> #3
 ^           |
 |-----------|
```

Le segment de fermeture dernier -> premier est dessiné en pointillés afin de distinguer le wrap de la séquence principale.

### PingPong

Les segments affichent les deux sens :

```text
#1 <-> #2 <-> #3
```

Il n'existe pas de segment #3 -> #1.

## Facing

La touche `F` cycle le `Facing` du waypoint sélectionné :

```text
None -> North -> East -> South -> West -> None
```

Lorsque le Facing est cardinal, une flèche verte est dessinée depuis le waypoint. Le waypoint sélectionné utilise une flèche dorée.

`None` conserve le contrat MON14.2 : aucune orientation d'arrivée n'est imposée.

## Attente

Le délai du waypoint sélectionné est modifié par pas de 0,5 seconde :

```text
-  : -0.5 s
+  : +0.5 s
```

La valeur est toujours clampée à `>= 0` et doit rester finie.

Le HUD affiche la valeur courante :

```text
Waypoint #2 Cell=(12,8) Facing=East Wait=1.5s
```

## Réorganisation

Le waypoint sélectionné peut être déplacé dans l'ordre logique :

```text
PageUp   -> plus tôt
PageDown -> plus tard
```

Le waypoint reste sélectionné après l'échange.

## Suppression

`Delete` ou `Backspace` retire le waypoint sélectionné.

Si la route tombe sous deux waypoints :

```text
PatrolMode -> None
```

Cela évite de laisser un `Loop` ou `PingPong` invalide dans l'asset.

La route entière peut également être effacée via la fonction editor-callable `Clear Selected Monster Patrol Route` du `GridLevelEditorActor`.

## Undo / Redo

Chaque mutation persistante utilise :

```cpp
FScopedTransaction
LevelAsset->Modify()
LevelAsset->MarkPackageDirty()
```

Sont transactionnels :

- ajout ;
- suppression ;
- clear ;
- changement de mode ;
- réorganisation ;
- Facing ;
- WaitSeconds.

La sélection visuelle d'un waypoint et l'entrée/sortie du mode route sont transitoires et ne salissent pas l'asset.

## Rendu

Lorsqu'un `MonsterSpawn` est sélectionné et que `bShowSelectedMonsterPatrolRoute=true`, la route est visible même si le mode route n'est pas actif.

Le rendu comporte :

- segments entre waypoints ;
- fermeture pointillée pour `Loop` ;
- double direction pour `PingPong` ;
- marqueurs de waypoint ;
- marqueur jaune pour le waypoint édité ;
- flèches de Facing ;
- numéros projetés dans le HUD.

Cela permet d'inspecter rapidement le niveau sans entrer en édition sur chaque garde.

## Sécurité d'édition

Le mode route n'est disponible que si l'objet sélectionné est un `MonsterSpawn`.

Pendant son activation :

- le clic gauche est capturé par l'éditeur de route ;
- les outils `PaintCell`, `PaintWall`, `PaintObject`, `Erase` et `Link` ne sont pas exécutés par erreur ;
- le clic droit reste disponible pour la navigation du viewport ;
- quitter complètement le `GridLevelEdMode` désactive le mode route.

## Tests automatisés

Deux tests editor-only sont ajoutés :

```text
Grimrock.Editor.MON14.3.1.PatrolRouteEditingModel
Grimrock.Editor.MON14.3.1.PatrolRouteGuards
```

Ils couvrent :

- activation du mode route ;
- ajout du premier et du deuxième waypoint ;
- auto-passage en `Loop` au deuxième waypoint ;
- sélection sans duplication ;
- Facing ;
- WaitSeconds ;
- réorganisation ;
- PingPong ;
- suppression ;
- retour automatique à `None` sous deux waypoints ;
- clear ;
- refus d'un mode actif avec moins de deux waypoints ;
- refus d'une cellule de hover invalide.

## Hors périmètre

MON14.3.1 ne modifie pas :

- le pathfinding MON4 ;
- le runtime MON14.3 ;
- la persistance de progression runtime de patrouille ;
- les animations spécifiques de garde ;
- les routes multi-niveaux ;
- un outil spline libre : les routes restent strictement basées sur les cellules de la grille.
