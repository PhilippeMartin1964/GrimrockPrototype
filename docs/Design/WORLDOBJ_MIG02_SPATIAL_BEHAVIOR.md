# WORLDOBJ-MIG02 — Comportement spatial minimal

## Objectif

Réduire les règles spatiales éditables d'une définition d'objet du monde à trois concepts seulement :

```text
Blocks Cell Movement
Occupies Boundary
Suppress Base Wall
```

La migration reste en mode prototype : aucune compatibilité arrière n'est recherchée pour les anciennes règles de partage.

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

`FGridBoundaryKey` normalise les deux descriptions possibles d'une même frontière :

```text
North(X,Y) == South(X,Y+1)
East(X,Y)  == West(X+1,Y)
```

Cette clé est l'autorité pour les conflits topologiques de placement et la validation de niveau.

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

## 4. Règle finale de partage — WORLDOBJ-MIG02.1

Les anciennes propriétés `Can Share Cell` et `Can Share Anchor` ont été supprimées du schéma C++.

La règle finale est :

- plusieurs objets peuvent partager une cellule ;
- plusieurs objets peuvent partager une surface murale ;
- un objet ne revendique une frontière que si `Occupies Boundary = true` ;
- deux objets ayant `Occupies Boundary = true` ne peuvent pas posséder la même `FGridBoundaryKey` canonique.

La validation de niveau détecte les conflits topologiques à partir de `FGridBoundaryKey`. Il n'existe plus de bridge transient ni de validation fondée sur une notion d'« anchor sharing ».

## 5. Contrats de validation

- `Occupies Boundary = true` exige `Placement Surface = Wall`.
- `Suppress Base Wall = true` exige `Placement Surface = Wall` et `Occupies Boundary = true`.
- une `Door` doit utiliser `Placement Surface = Wall` et `Occupies Boundary = true`.
- le partage de cellule, à lui seul, n'est jamais une erreur ni un warning.

Le placement éditeur et la validation utilisent donc le même modèle topologique.

## Tests

```text
Grimrock.WorldObjects.MIG02.SpatialBehaviorSchema
Grimrock.WorldObjects.MIG02.BoundaryKey
Grimrock.WorldObjects.MIG02.1.DefinitionValidation
```

Ils vérifient notamment :

- exactement trois paramètres éditables dans `Spatial Behavior` ;
- l'absence totale des anciennes propriétés de partage dans le schéma réfléchi ;
- les trois sémantiques et leurs valeurs par défaut ;
- la canonicalisation Nord/Sud et Est/Ouest des frontières ;
- le hash canonique des frontières ;
- le contrat `Suppress Base Wall -> Occupies Boundary` ;
- l'absence des anciens warnings de partage dans la validation de définition.
