# WORLDOBJ-MIG01 — Placement Surface simplifié

> **Évolution ultérieure — ALIGN-B5.3 (2026-09-09).** Les projections transientes conservées temporairement pendant MIG01 ont ensuite été entièrement supprimées. Le contrat final et courant est `PlacementSurface + DefaultLocalPosition.U/V/N`, consommé directement par le runtime et l’éditeur. La section « Projection transitoire interne » ci-dessous décrit volontairement l’état historique de MIG01.

## Objectif

`WORLDOBJ-MIG01` simplifie le contrat de placement des définitions d'objets du monde en séparant clairement :

- la **surface de placement** de l'objet ;
- sa **position locale** sur cette surface ;
- la **topologie de frontière** portée par l'instance de niveau (`Edge`) lorsqu'un objet occupe un mur ou un passage.

Le projet est en mode prototype : **aucune compatibilité arrière des anciens Data Assets n'est maintenue pour ce contrat de placement**.

## Nouveau contrat d'auteur

Une définition d'objet expose désormais :

```text
PlacementSurface
├─ Floor
├─ Wall
└─ Ceiling

DefaultLocalPosition
├─ U
├─ V
└─ N
```

### Floor

- `U` / `V` : coordonnées dans le plan du sol ;
- `N` : hauteur au-dessus du sol.

### Wall

- `U` : déplacement latéral le long du mur ;
- `V` : hauteur ;
- `N` : profondeur / inset vers l'intérieur de la cellule.

### Ceiling

- `U` / `V` : coordonnées dans le plan du plafond ;
- `N` : distance sous le plafond.

## Ce qui disparaît du contrat de Data Asset

Les anciens paramètres suivants ne sont plus sérialisés comme paramètres d'auteur :

- `PlacementKind` ;
- `PlacementZOffset` ;
- `WallInset` ;
- `LocalOffsetAlongWall` ;
- `LocalOffsetVertical`.

`Center` n'est plus une surface : un objet centré est simplement placé sur `Floor` avec une position locale centrée.

`Edge` n'est plus une surface : la frontière reste décrite par `FGridLevelObjectData::Edge` pour les objets nécessitant une orientation ou une frontière de cellule, notamment les portes et objets muraux.

## Projection transitoire interne — état historique de MIG01

Au moment de `WORLDOBJ-MIG01`, le runtime et certaines parties de l'éditeur consommaient encore les anciens helpers de transform. MIG01 conservait donc temporairement une projection **Transient**, non éditable et non sérialisée, calculée depuis `PlacementSurface` et `DefaultLocalPosition`.

Cette projection n'était pas une voie de compatibilité Data Asset. Elle servait uniquement à éviter de mélanger dans la même étape la refonte du contrat de données et la réécriture de tous les consommateurs C++.

Depuis `WORLDOBJ-ALIGN-B5.3`, cette projection et ses helpers n'existent plus.

## Règles de validation

- `Door`, `Button`, `Lever` : `Wall` ;
- `Pit`, `PressurePlate`, `Trigger`, `Teleporter`, `MonsterSpawn`, `ItemSpawn` : `Floor` ;
- `Decoration` et `Light` : `Floor`, `Wall` ou `Ceiling` selon la définition ;
- `Receptacle` : surface libre selon la définition, avec `Wall` pour les réceptacles muraux ;
- les items encore placés via le chemin WorldObject restent temporairement sur `Floor` dans le contexte historique de MIG01.

## Tests

`WORLDOBJ-MIG00` conserve la caractérisation de l'ancien contrat :

```text
Grimrock.WorldObjects.MIG00.Characterization
```

`WORLDOBJ-MIG01` valide le nouveau schéma et la parité des transforms utiles :

```text
Grimrock.WorldObjects.MIG01
```

Les cas couverts sont :

- sol ;
- plafond ;
- mur ;
- porte sur frontière ;
- item au bord d'une cellule.

## Important pour les assets existants

Comme aucune migration arrière n'est conservée, les définitions existantes doivent être **réenregistrées avec le nouveau contrat** avant de considérer le runtime visuel comme validé. Les anciens champs sérialisés ne sont pas relus pour reconstruire automatiquement le nouveau placement.
