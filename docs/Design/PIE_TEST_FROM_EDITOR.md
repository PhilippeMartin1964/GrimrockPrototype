# Testing a Grid Level from L_GrimrockEditor

## 1. Purpose

This workflow allows quick testing of a `UGridLevelAsset` directly from `L_GrimrockEditor`.

The editor map can be used to edit the dungeon, preview the dungeon, start a playable PIE test, and then return to editing without opening `L_GrimrockRuntime`.

## 2. Required Actors

`L_GrimrockEditor` should contain:

- `BP_GridLevelEditorActor`
- `BP_GridLevelRuntimeActor`
- `BP_GrimrockPartyPawn`, or a GameMode that spawns a compatible pawn
- a valid `UGridLevelAsset`

`BP_GridLevelEditorActor.LevelAsset` and `BP_GridLevelRuntimeActor.LevelAsset` should point to the same `UGridLevelAsset`.

## 3. Workflow

1. Open `L_GrimrockEditor`.
2. Select a playable cell.
3. Run `SetStartFromSelection` on `BP_GridLevelEditorActor`.
4. Save the `UGridLevelAsset`.
5. Run `PreparePIETestFromStart` on `BP_GridLevelEditorActor`.
6. Run `LogPIEReadinessDiagnostics` on `BP_GridLevelRuntimeActor` if more detail is needed.
7. Click Play.

`PreparePIETestFromStart` does not start PIE automatically. It only synchronizes the preview runtime actor, enables LevelAsset start application on begin play, rebuilds the runtime preview, and logs readiness diagnostics.

## 4. Diagnostics

Useful diagnostics:

- `LogEditorRuntimeAssetConsistency`
- `LogLevelAssetDiagnostics`
- `LogPIEReadinessDiagnostics`

Use `LogEditorRuntimeAssetConsistency` first when the editor actor and preview runtime actor may not reference the same `UGridLevelAsset`.

Use `LogLevelAssetDiagnostics` to inspect the runtime actor's assigned asset, grid size, object counts, links, and start cell.

Use `LogPIEReadinessDiagnostics` before Play to check the runtime actor, start cell, meshes, components, archetypes, and pawn availability in the current world.

## 5. Difference with L_GrimrockRuntime

`L_GrimrockEditor` is for editing and fast local PIE tests.

`L_GrimrockRuntime`, or a future `L_GrimrockGame`, remains the cleaner gameplay test map. It should be used to validate game flow without editor helper actors, editor grid overlays, or temporary editing setup.
