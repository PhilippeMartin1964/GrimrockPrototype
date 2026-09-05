# WORLDOBJ-MIG02 — Comportement spatial minimal

## Objectif

Réduire les règles spatiales éditables d'une définition d'objet du monde à trois concepts seulement :

```text
Blocks Cell Movement
Occupies Boundary
Suppress Base Wall
```

La migration reste en mode prototype : les anciennes règles de partage ne sont plus des paramètres d'auteur et aucune compatibilité de Data Asset n'est recherchée.

## 1. Blocks Cell Movement

Indique qu'un objet constitue intrinsèquement un obstacle de cellule pour le déplacement sur grille.

Ce paramètre ne doit pas être utilisé pour l'état dynamique d'une porte. Une porte ouverte/fermée bloque ou libère un passage via son système runtime de porte.

Exemples :

- grosse statue occupant toute la cellule : `true` ;
- bouton mural : `false` ;
- porte : `false` ;
- fosse : `false`.

## 2. Occupies Boundary

Indique que l'objet possède sémantiquement la frontière physique entre deux cellules.

Ce paramètre est distinct de `PlacementSurface = Wall` :

- un bouton mural est placé sur un mur mais ne possède pas la frontière ;
- une porte, une grille ou une porte secrète possède la frontière.

WORLDOBJ-MIG02 ajoute `FGridBoundaryKey`, qui normalise les deux descriptions possibles d'une même frontière :

```text
North(X,Y) == South(X,Y+1)
East(X,Y)  == West(X+1,Y)
```

Cette clé servira de base unique pour les conflits topologiques, la validation et la suppression de mur structurel.

## 3. Suppress Base Wall

Indique que le mesh du mur structurel généré sur la frontière doit être supprimé afin de laisser la géométrie de l'objet le remplacer.

Ce paramètre ne signifie ni « bloque le passage », ni « est fermé ».

Exemples :

```text
Bouton mural
Blocks Cell Movement = false
Occupies Boundary    = false
Suppress Base Wall   = false

Porte
Blocks Cell Movement = false
Occupies Boundary    = true
Suppress Base Wall   = true

Portail placé dans une ouverture existante
Blocks Cell Movement = false
Occupies Boundary    = true
Suppress Base Wall   = false
```

## Suppression des anciennes règles de partage

`Can Share Cell` et `Can Share Anchor` ne sont plus exposés comme paramètres d'auteur.

La règle cible est :

- plusieurs objets peuvent partager une cellule ;
- plusieurs objets peuvent partager une surface murale ;
- deux objets ayant `Occupies Boundary = true` ne doivent normalement pas posséder la même `FGridBoundaryKey` canonique.

Les membres C++ historiques de partage subsistent temporairement uniquement en `Transient` pour que les consommateurs éditeur encore non migrés continuent à compiler. Ils ne sont ni éditables ni sérialisés et ne constituent pas une compatibilité arrière des Data Assets.

## Portée de cette étape

WORLDOBJ-MIG02 établit le schéma minimal et la représentation canonique des frontières.

Le câblage exhaustif de `FGridBoundaryKey` dans les conflits de placement éditeur, la validation de niveau et les derniers consommateurs historiques est la phase de nettoyage immédiatement suivante de MIG02, avant la migration visuelle WORLDOBJ-MIG03.

## Tests

```text
Grimrock.WorldObjects.MIG02.SpatialBehaviorSchema
Grimrock.WorldObjects.MIG02.BoundaryKey
```

Ils vérifient :

- exactement trois paramètres éditables dans `Spatial Behavior` ;
- les anciennes règles de partage ne sont plus éditables ni sérialisées ;
- les trois sémantiques ont les valeurs par défaut attendues ;
- la canonicalisation Nord/Sud et Est/Ouest des frontières ;
- le hash canonique des frontières.
