# Multi-Level Dungeon Asset

## Purpose

`UGridDungeonAsset` groups several `UGridLevelAsset` assets into a single dungeon definition. It is the minimal data structure needed before adding stairs, passages, runtime transitions, saves, or a main menu.

The individual `UGridLevelAsset` remains the source of truth for one grid level. The dungeon asset only selects and organizes those levels.

## DA_Dungeon_01 Fields

`DA_Dungeon_01` should expose these fields:

- `Dungeon Name`
- `Author`
- `Version`
- `Default Level Id`
- `Levels`

`DefaultLevelId` is intentionally kept as the C++ property name to avoid breaking existing assets. In the editor it appears as `Default Level Id`, and it represents the default start level of the dungeon.

Do not add a second default-start field unless the architecture later needs two distinct concepts.

## Example Setup

```text
Dungeon Name = Into The Dark
Author = Philippe
Version = 0.1
Default Level Id = Into_The_Dark
```

```text
Levels:
[0]
LevelId = Into_The_Dark
DisplayName = Into The Dark
LevelAsset = DA_GridLevel_00
LogicalPosition = X 0 / Y 0 / Z 0
Enabled = true

[1]
LevelId = Old_Tunnels
DisplayName = Old Tunnels
LevelAsset = DA_GridLevel_01
LogicalPosition = X 0 / Y 0 / Z 1
Enabled = true
```

## Editor Workflow

In `BP_GridLevelEditorActor`:

- Set `DungeonAsset = DA_Dungeon_01`.
- Set `CurrentDungeonLevelId = Into_The_Dark` and click `Apply Current Dungeon Level`.
- Or click `Load Default Dungeon Level` to use `Default Level Id`, falling back to the first enabled level with a valid `LevelAsset`.
- Use `Log Dungeon Diagnostics` to verify the selected dungeon, level ids, enabled states, and missing assets.

`ApplyCurrentDungeonLevel()` remains available for Blueprint/C++ logic. The editor-facing `Apply Current Dungeon Level` button calls the same function without changing the existing behavior.

## Creating a new level from the editor

Le panneau `DUNGEON LEVELS` permet maintenant de créer un niveau directement depuis le Grimrock Grid Editor Mode.

Workflow :

1. Ouvrir Grimrock Grid Editor Mode.
2. Dans `DUNGEON LEVELS`, cliquer `New Level`.
3. Renseigner `Level Id`, `Display Name` et `Logical Position`.
4. Cliquer `Create`.
5. Le nouvel `UGridLevelAsset` est créé, ajouté au `DungeonAsset`, sélectionné et affiché automatiquement.

La création ne génère aucun Blueprint et ne nécessite aucune action manuelle dans le Content Browser.

Le nouvel asset est créé dans :

```text
/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/
```

Le nom recommandé est généré automatiquement à partir du `Level Id` :

```text
Level Id = Test_Level_02
Asset = DA_GridLevel_Test_Level_02
```

Le niveau créé est un `UGridLevelAsset` neuf. Il contient une grille 32x32 et une cellule de départ jouable à `(1,1)` orientée `North`, afin de pouvoir tester rapidement le niveau. L'utilisateur peut ensuite peindre les cellules, les murs, les objets et les transitions.
## StairsUp / StairsDown

Les transitions multi-niveaux doivent maintenant être représentées par des objets dédiés de palette :

- `Stairs Up`
- `Stairs Down`

Ces objets sont des archétypes `UGridObjectArchetypeAsset` configurés comme décorations de sol non bloquantes. Ils utilisent les meshes `SM_Stairs_Up_01` et `SM_Stairs_Down_01`, se placent au centre d'une cellule et portent `Behavior.Transition.bIsTransition = true` par défaut.

Chaque escalier doit ensuite être configuré dans l'inspecteur :

- `TargetLevelId`
- `TargetCellX`
- `TargetCellY`
- `TargetFacing`
- `bRequireUseAction`

`bRequireUseAction = false` déclenche la transition en marchant sur l'escalier. `bRequireUseAction = true` est réservé aux transitions nécessitant l'action `Use`, qui seront traitées plus tard.

Comportement de rendu de `Stairs_Down` :

- `Stairs_Down` masque le floor standard de sa cellule avec `bHideCellFloor = true`;
- la cellule reste walkable si ses données de cellule le permettent;
- ce n'est pas un changement de type de cellule;
- `Stairs_Up` conserve le floor standard pour l'instant;
- le mur ou la face sombre devant la descente sera traité dans une étape séparée.

Workflow :

1. Créer ou sélectionner un niveau cible dans `DUNGEON LEVELS`.
2. Dans le niveau source, placer `Stairs Down`.
3. Configurer `TargetLevelId`, `TargetCellX`, `TargetCellY` et `TargetFacing`.
4. Dans le niveau cible, placer `Stairs Up`.
5. Configurer la transition retour.
6. Lancer PIE.
7. Marcher sur l'escalier.
