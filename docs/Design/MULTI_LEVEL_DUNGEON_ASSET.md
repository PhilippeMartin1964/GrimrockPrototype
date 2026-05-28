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
