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

The transition inspector saves values directly into the selected object's `Behavior` inside the `LevelAsset`. There is no separate `Apply` button for transition data.

Text fields such as `Target Level Id` use `OnTextCommitted`, so press Enter or leave the field to commit the value.

## Diagnostic Workflow

1. Select the transition object.
2. Check `Is Transition`.
3. Fill `Target Level Id`.
4. Press Enter or leave the field.
5. Fill `Target Cell X` and `Target Cell Y`.
6. Choose `Target Facing`.
7. Save the edited `UGridLevelAsset`.
8. Select `BP_GridLevelEditorActor`.
9. Click `Log Dungeon Transition Diagnostics`.

Expected log shape:

```text
TransitionObjects=1
[0] SourceLevelId=Into_The_Dark SourceDisplayName=Into The Dark SourceLevelAsset=/Game/.../DA_GridLevel_00 ObjectId=... ArchetypeId=Floor_Bones Type=Decoration Cell=(28,25) Edge=None TargetLevelId=Old_Tunnels TargetCell=(28,26) TargetFacing=North bRequireUseAction=false Status=OK
Status=OK
```

Runtime travel is intentionally not implemented yet. Runtime diagnostics only report how many transition objects exist in the loaded `LevelAsset`.
