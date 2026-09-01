# PIT02 — World Items Falling Through Pits

Date : 01.09.2026

## Objectif

PIT02 généralise la fosse aux objets physiques du monde.

Un objet qui doit être déposé comme World Item sur une cellule contenant une fosse ouverte ne reste pas sur le niveau source. Il est transféré dans l'état runtime du niveau cible, puis devient un World Item ordinaire lorsque ce niveau est actif.

La logique est volontairement placée dans `AGridLevelRuntimeActor::TryDropItemInstanceAtCell()` et non dans `AGridThrownItemActor`.

Conséquence : les mêmes règles s'appliquent à :

- une pierre lancée ;
- un objet déposé depuis l'inventaire ;
- un loot déposé sur une fosse ;
- toute future mécanique produisant un World Item par le pipeline normal.

## Flux

```text
Projectile / dépôt / loot
        |
        v
TryDropItemInstanceAtCell()
        |
        +-- cellule normale --> World Item sur le niveau courant
        |
        +-- fosse ouverte
                |
                v
       TryRouteWorldItemThroughOpenPit()
                |
                v
       PendingInboundItems du niveau cible
                |
          niveau cible activé
                |
                v
       ApplyPendingInboundItemsForCurrentLevel()
                |
                v
          World Item normal
```

## Persistance multi-niveaux

`FGridLevelRuntimeState` contient désormais :

```cpp
TMap<FGuid, FGridPendingWorldItemState> PendingInboundItems;
```

Un item en chute conserve :

- RuntimeObjectId ;
- ItemDefinitionId ;
- quantité ;
- cellule cible ;
- offset horizontal dans la cellule ;
- état des lumières ;
- données de lecture ;
- référence au `UGridItemDefinitionAsset`.

La référence au Data Asset permet de matérialiser l'objet même si le LevelAsset inférieur ne référence pas encore ce type d'item.

Le simple fait qu'un objet tombe sur un niveau inférieur **ne marque pas ce niveau comme visité**.

Lorsque le niveau devient actif, la file Pending est consommée. Lors d'une capture runtime ultérieure, l'objet est enregistré dans `Items` comme n'importe quel World Item.

## Interaction avec les plaques de pression

PIT02 ne contient aucune logique « projectile active plaque ».

Après arrivée sur le niveau inférieur, l'objet passe par `TryDropItemInstanceAtCell()`, rejoint `SpawnedItemEntries` et participe donc au calcul existant :

`GetWorldItemWeightAtCell()`

Une plaque de pression située sur la cellule d'arrivée peut ainsi réagir au poids normal de l'objet.

## Fosse fermée

Si `Pit.bInitiallyOpen=false`, le pipeline ne route pas l'objet : il est déposé normalement sur la cellule.

Ce contrat prépare PIT03 : une future trappe fermée se comportera comme un sol ; ouverte, elle laissera tomber les World Items.

## Chute en cascade

PIT02 conserve la restriction PIT01 : une destination contenant une autre fosse ouverte est refusée pour l'instant.

Les puits multi-étages seront traités explicitement dans un jalon ultérieur plutôt que par récursion implicite.

## Présentation

PIT02 n'ajoute pas une seconde simulation physique verticale entre les niveaux.

Pour un projectile lancé, la trajectoire visible existe déjà jusqu'à l'impact. Lorsqu'il atteint la fosse, il disparaît du niveau supérieur et est mis en attente sur le niveau cible.

Ce choix garde l'autorité sur la grille et évite de maintenir des acteurs physiques dans plusieurs LevelAssets simultanément.

## Test automatisé

Filtre :

`Grimrock.Pit.PIT02`

Le test vérifie :

- routage d'un World Item normal sur une fosse ouverte ;
- absence d'item sur la cellule supérieure ;
- création de `PendingInboundItems` sur le niveau inférieur ;
- niveau inférieur non marqué visité par le simple transfert ;
- matérialisation lors de la transition vers le niveau inférieur ;
- conservation du poids normal ;
- consommation de la file Pending ;
- persistance finale dans `FGridLevelRuntimeState::Items`.
