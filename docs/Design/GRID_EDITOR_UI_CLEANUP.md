# Grid Editor UI Cleanup

## Principle

`BP_GridLevelEditorActor` remains the technical configuration actor for the editor map.

The Grimrock Grid Editor Mode is the daily editing interface. New workflow controls should move there when they are part of normal level editing.

## Dungeon Levels Panel

The Grid Editor Mode now includes a `DUNGEON LEVELS` panel after the header and before `TOOLS / PALETTE`.

The panel shows:

- dungeon name;
- default level id;
- current dungeon level id;
- current `LevelAsset`;
- level count;
- a clickable list of levels from the assigned `DungeonAsset`.

Example entries:

```text
(0,0,0) Into The Dark
(0,0,1) Old Tunnels
```

Clicking a valid level sets `CurrentDungeonLevelId`, calls `ApplyCurrentDungeonLevel()`, rebuilds the preview, and redraws editor viewports.

## Workflow

1. Assign `DungeonAsset` once on `BP_GridLevelEditorActor`.
2. Open Grimrock Grid Editor Mode.
3. Click a level in `DUNGEON LEVELS`.
4. Edit the selected level.
5. Use `PreparePIETestFromStart`.
6. Click Play.

## Compatibility

The existing `CallInEditor` buttons on `BP_GridLevelEditorActor` are kept for now for debugging and compatibility:

- `Apply Current Dungeon Level`
- `Load Default Dungeon Level`
- `Log Dungeon Diagnostics`
- `Log Dungeon Transition Diagnostics`

A later audit can decide which technical controls should remain visible on the actor.
