# Illustrations du chapitre 20 — Architecture noyau Donjon / Niveau / Grille

Ce dossier contient les illustrations produites pour le chapitre 20 du document :

```text
docs/Architecture/CORE_DUNGEON_LEVEL_GRID.md
```

Les illustrations sont volontairement au format **SVG** afin de rester lisibles, légères et facilement modifiables.

---

## 20.1. Le donjon comme classeur

![Le donjon comme classeur](core_20_1_dungeon_binder.svg)

But : montrer que `UGridDungeonAsset` organise les niveaux mais ne stocke pas directement la grille.

Fichier :

```text
docs/Images/core_20_1_dungeon_binder.svg
```

---

## 20.2. Le niveau comme carte quadrillée

![Le niveau comme carte quadrillée](core_20_2_level_grid_map.svg)

But : montrer que `UGridLevelAsset` stocke cellules, murs, position de départ, objets et liens.

Fichier :

```text
docs/Images/core_20_2_level_grid_map.svg
```

---

## 20.3. Une cellule et ses quatre murs

![Une cellule et ses quatre murs](core_20_3_cell_four_walls.svg)

But : expliquer la différence entre `CellType`, les quatre murs, le plafond et le blocage d’occupation.

Fichier :

```text
docs/Images/core_20_3_cell_four_walls.svg
```

---

## 20.4. Flux éditeur

![Flux éditeur](core_20_4_editor_flow.svg)

But : montrer que l’utilisateur agit sur `Grimrock Grid Editor Mode`, lequel pilote `AGridLevelEditorActor`, modifie le `UGridLevelAsset` et reconstruit un aperçu.

Fichier :

```text
docs/Images/core_20_4_editor_flow.svg
```

---

## 20.5. Flux runtime

![Flux runtime](core_20_5_runtime_flow.svg)

But : montrer que `AGridLevelRuntimeActor` lit le `UGridLevelAsset`, génère `FloorISM`, `WallISM`, `CeilingISM`, puis produit le niveau jouable.

Fichier :

```text
docs/Images/core_20_5_runtime_flow.svg
```

---

## Bloc Markdown à intégrer dans le chapitre 20

Le bloc suivant peut remplacer le chapitre 20 de `CORE_DUNGEON_LEVEL_GRID.md` :

```markdown
## 20. Illustrations du noyau

Les illustrations suivantes accompagnent le noyau Donjon / Niveau / Grille.

### 20.1. Le donjon comme classeur

![Le donjon comme classeur](../Images/core_20_1_dungeon_binder.svg)

Cette illustration montre que `UGridDungeonAsset` organise les niveaux du donjon, mais ne stocke pas directement la grille.

### 20.2. Le niveau comme carte quadrillée

![Le niveau comme carte quadrillée](../Images/core_20_2_level_grid_map.svg)

Cette illustration montre que `UGridLevelAsset` stocke les cellules, les murs, la position de départ, les objets et les liens.

### 20.3. Une cellule et ses quatre murs

![Une cellule et ses quatre murs](../Images/core_20_3_cell_four_walls.svg)

Cette illustration montre qu’une cellule `FGridLevelCellData` contient `CellType`, `NorthWall`, `EastWall`, `SouthWall`, `WestWall`, `bHasCeiling` et `bBlocksOccupancy`.

### 20.4. Flux éditeur

![Flux éditeur](../Images/core_20_4_editor_flow.svg)

Cette illustration montre le chemin de l’action utilisateur : `Grimrock Grid Editor Mode` pilote `AGridLevelEditorActor`, qui modifie le `UGridLevelAsset` puis reconstruit l’aperçu.

### 20.5. Flux runtime

![Flux runtime](../Images/core_20_5_runtime_flow.svg)

Cette illustration montre que le runtime lit le `UGridLevelAsset`, génère les composants `FloorISM`, `WallISM`, `CeilingISM`, puis produit le niveau jouable.
```
