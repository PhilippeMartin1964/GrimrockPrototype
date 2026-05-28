# Grid Level Transition Data

## Purpose

Grid transitions are stored as object behavior data in `UGridLevelAsset`. This step only records and validates transition intent; it does not execute travel at runtime.

## Supported Uses

A placed grid object can be marked as a transition and configured as:

- `StairsUp`
- `StairsDown`
- `PassageNorth`
- `PassageSouth`
- `PassageEast`
- `PassageWest`
- future `Portal`

These names should be represented by archetypes or palette entries. They should not become new `EGridLevelObjectType` values unless a later runtime requirement proves that necessary.

## Fields

`FGridObjectTransitionParams` is part of `FGridObjectBehaviorParams`:

- `bIsTransition`
- `TargetLevelId`
- `TargetCellX`
- `TargetCellY`
- `TargetFacing`
- `bRequireUseAction`

`TargetLevelId` should match an enabled entry in the current `UGridDungeonAsset`. `TargetCellX` and `TargetCellY` should point to a valid cell in the target `UGridLevelAsset`.

## Editor Workflow

In `BP_GridLevelEditorActor`:

- Select an object representing stairs, a passage, or a portal.
- In the object inspector, enable `Transition`.
- Set the target level id and target cell.
- Set target facing.
- Run level validation to catch missing target levels or invalid target cells.

Runtime travel is intentionally not implemented yet. The current runtime diagnostics only report how many transition objects exist in the loaded `LevelAsset`.
