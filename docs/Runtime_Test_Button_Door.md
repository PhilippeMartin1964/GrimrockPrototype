# Test runtime manuel : Button -> Door

Statut : test manuel de non-régression runtime.

## Objectif du test

Valider la chaîne gameplay complète `Button -> Door` dans `GrimrockPrototype` :

- placement d'une porte sur un edge de grille ;
- placement d'un bouton mural ;
- lien logique du bouton vers la porte ;
- commande `Open` ou `Toggle` ;
- blocage du déplacement avant activation ;
- déblocage du passage après activation et animation complète de la porte.

Ce document sert de test manuel de non-régression pour le runtime jouable.

## Prérequis

- Le projet compile en Editor.
- La map runtime s'ouvre sans erreur.
- Le pawn joueur possède une action `Use` configurée.
- Le niveau runtime utilise un `AGridLevelRuntimeActor` ou `BP_GridLevelRuntimeActor`.
- Le `BP_GridLevelRuntimeActor` référence le bon `UGridLevelAsset`.
- Les archetypes nécessaires sont présents dans `ObjectArchetypes`.

## Asset concerné

Asset de niveau :

```text
/Game/GrimrockPrototype/Core/DataAssets/DA_GridLevelAsset
```

Assets d'archetypes utiles :

```text
/Game/GrimrockPrototype/Core/DataAssets/DA_Door_Stone
/Game/GrimrockPrototype/Core/DataAssets/DA_Button_Normal
```

Archetype IDs attendus :

```text
Door_Stone
Button_Normal
```

## Map concernée

Map de test runtime :

```text
/Game/GrimrockPrototype/Maps/L_GrimrockRuntime
```

## Configuration des cellules

Choisir deux cellules voisines qui serviront de passage de test.

Exemple recommandé :

```text
Cellule A : X=10, Y=10
Cellule B : X=10, Y=11
Passage  : A North <-> B South
```

Configurer les deux cellules :

```text
Cellule A
CellType = Floor
NorthWall = None
bHasCeiling = true
bBlocksOccupancy = false

Cellule B
CellType = Floor
SouthWall = None
bHasCeiling = true
bBlocksOccupancy = false
```

Pour faciliter le placement du bouton, prévoir un mur sur un edge accessible de la cellule A, par exemple :

```text
Cellule A
WestWall = Solid
```

## Configuration de la porte

Ajouter une entrée dans le tableau `Objects` de `DA_GridLevelAsset`.

Exemple :

```text
ObjectId = GUID unique, par exemple DoorGuid
Type = Door
CellX = 10
CellY = 10
Edge = North
ArchetypeId = Door_Stone
bInitiallyEnabled = true
bInitiallyActive = false
Tag = None
PaletteEntryId = None
bOverrideBehavior = false
```

Interprétation :

- `Edge = North` place la porte entre `(10,10)` et `(10,11)`.
- `bInitiallyActive = false` signifie que la porte démarre fermée.
- La porte doit bloquer le passage au début du test.

## Configuration du bouton

Ajouter une autre entrée dans le tableau `Objects`.

Exemple :

```text
ObjectId = GUID unique, par exemple ButtonGuid
Type = Button
CellX = 10
CellY = 10
Edge = West
ArchetypeId = Button_Normal
bInitiallyEnabled = true
bInitiallyActive = false
Tag = None
PaletteEntryId = None
bOverrideBehavior = false
```

Interprétation :

- `Edge = West` place le bouton sur le mur ouest de la cellule `(10,10)`.
- Le joueur doit pouvoir regarder vers `West` depuis la cellule A et appuyer sur `Use`.
- Le bouton doit jouer son animation de pression.

## Configuration du lien Button -> Door

Ajouter une entrée dans le tableau `Links` de `DA_GridLevelAsset`.

Version recommandée pour un premier test :

```text
SourceObjectId = ButtonGuid
TargetObjectId = DoorGuid
SourceEvent = Activated
Command = Open
```

Version utile pour tester une bascule :

```text
SourceObjectId = ButtonGuid
TargetObjectId = DoorGuid
SourceEvent = Activated
Command = Toggle
```

Notes :

- `SourceObjectId` doit correspondre exactement au `ObjectId` du bouton.
- `TargetObjectId` doit correspondre exactement au `ObjectId` de la porte.
- Le connecteur est défini par `Source Object = Button_Normal`, `Source Event = Activated`, `Target Object = Door_Stone`, puis `Command = Open` ou `Toggle`.
- Le runtime exécute uniquement les connecteurs correspondant au `SourceEvent` exact.

## Vérification des archetypes dans BP_GridLevelRuntimeActor

Dans `L_GrimrockRuntime`, sélectionner l'acteur `BP_GridLevelRuntimeActor`.

Vérifier :

```text
LevelAsset = DA_GridLevelAsset
ObjectArchetypes contient DA_Door_Stone
ObjectArchetypes contient DA_Button_Normal
```

Sans ces archetypes :

- la porte peut ne pas trouver sa classe runtime ;
- le bouton peut ne pas spawner ;
- les meshes ou matériaux peuvent être absents ;
- le lien peut exister dans les données mais ne produire aucun effet visible.

## Position initiale du pawn

Configurer ou placer le pawn pour commencer sur la cellule A.

Exemple :

```text
CurrentCellX = 10
CurrentCellY = 10
Facing = North
```

Cette position permet de tester immédiatement le blocage par la porte.

Pour tester le bouton :

```text
Facing = West
```

Le pawn peut aussi commencer face au bouton si l'objectif est de valider d'abord `Use`.

## Procédure de test dans Play

1. Ouvrir `L_GrimrockRuntime`.
2. Lancer `Play`.
3. Vérifier que le pawn démarre sur la cellule `(10,10)`.
4. Faire face au `North`.
5. Essayer d'avancer.
6. Vérifier que le déplacement est bloqué par la porte fermée.
7. Se tourner vers `West`.
8. Appuyer sur `Use`.
9. Vérifier que le bouton s'anime.
10. Vérifier que la porte commence à s'ouvrir.
11. Attendre la fin de l'animation de porte.
12. Se tourner vers `North`.
13. Avancer.
14. Vérifier que le pawn passe de `(10,10)` à `(10,11)`.

Pour `Command = Toggle` :

1. Revenir ou rester dans une position permettant de regarder le bouton.
2. Appuyer une deuxième fois sur `Use`.
3. Vérifier que la porte se referme.
4. Attendre la fin de l'animation.
5. Essayer de traverser à nouveau.
6. Vérifier que le passage est bloqué.

## Résultats attendus

Avant activation :

- la porte est visible ;
- la porte est fermée ;
- le passage est bloqué ;
- le pawn ne peut pas avancer à travers l'edge de porte.

Pendant activation :

- `Use` sur le bouton déclenche l'animation du bouton ;
- le lien `Button -> Door` est exécuté ;
- la porte démarre son animation d'ouverture.

Après animation complète :

- la porte est ouverte ;
- `UGridDoorSystemComponent` ne bloque plus l'edge ;
- le pawn peut traverser vers la cellule voisine.

Avec `Command = Toggle` :

- une nouvelle activation referme la porte ;
- le passage redevient bloqué après l'animation de fermeture.

## Diagnostic si le test échoue

### Le bouton n'apparaît pas

Vérifier :

```text
Object Type = Button
ArchetypeId = Button_Normal
bInitiallyEnabled = true
Edge != None
BP_GridLevelRuntimeActor.ObjectArchetypes contient DA_Button_Normal
```

### La porte n'apparaît pas

Vérifier :

```text
Object Type = Door
ArchetypeId = Door_Stone
bInitiallyEnabled = true
Edge != None
BP_GridLevelRuntimeActor.ObjectArchetypes contient DA_Door_Stone
```

### Le bouton s'anime mais la porte ne bouge pas

Vérifier :

```text
Links contient une entrée ButtonGuid -> DoorGuid
SourceObjectId = ObjectId exact du bouton
TargetObjectId = ObjectId exact de la porte
SourceEvent = Activated
Command = Open ou Toggle
DoorGuid est valide
ButtonGuid est valide
```

Vérifier aussi que la porte a bien été spawnée comme acteur runtime.

### La porte s'ouvre mais le pawn ne passe pas

Vérifier le piège `Wall=None`.

Le passage de grille doit être ouvert dans les données de cellule :

```text
Cellule A NorthWall = None
Cellule B SouthWall = None
```

Si l'edge reste `Solid` ou `Secret`, `AGridLevelRuntimeActor::CanMove` bloque encore le déplacement même si la porte est ouverte.

### Le bouton ne répond pas à Use

Vérifier :

```text
Le pawn regarde le bon edge
Le bouton est indexé sur CellX/CellY/Edge corrects
UseAction est bien configurée
Le pawn est bien associé au LevelRuntimeActor
```

Tester aussi l'edge opposé grâce à la résolution bidirectionnelle.

## Notes sur Wall=None pour les portes

Une porte runtime n'est pas un mur de cellule. Elle est un objet placé sur un edge.

Le déplacement est autorisé seulement si :

```text
Cellule source walkable
Cellule destination walkable
WallOnEdge = None
DoorSystemComponent ne bloque pas le passage
```

Donc une porte fermée doit bloquer par le système de porte, pas par `NorthWall = Solid`.

Configuration correcte :

```text
NorthWall = None
Door object sur North
Door bInitiallyActive = false
```

Configuration incorrecte pour ce test :

```text
NorthWall = Solid
Door object sur North
```

Dans ce cas, ouvrir la porte ne suffit pas, car le mur logique reste bloquant.

## Notes sur l'interaction edge bidirectionnelle

Le runtime résout maintenant les interactions edge-based dans les deux sens :

1. cellule courante + edge regardé ;
2. cellule voisine + edge opposé.

Exemple :

```text
Depuis (10,10), Facing North :
Recherche directe : (10,10, North)
Recherche fallback : (10,11, South)
```

Cela permet de tester deux configurations équivalentes :

```text
Porte placée sur (10,10, North)
Porte placée sur (10,11, South)
```

Même logique pour un bouton ou autre interactable edge-based :

```text
Bouton placé sur cellule courante + edge regardé
Bouton placé sur cellule voisine + edge opposé
```

La priorité reste la cellule courante. Le fallback voisin n'est utilisé que si rien n'est trouvé sur l'edge direct.

Pour les tests plus larges de cohérence UI/runtime, voir `docs/Design/10_GRID_EDITOR_UI_CONSISTENCY_CHECKLIST.md`.
