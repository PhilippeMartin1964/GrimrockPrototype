# Testing a Grid Level from L_GrimrockEditor

## 1. Purpose

This workflow allows quick testing of a `UGridLevelAsset` directly from `L_GrimrockEditor`.

The editor map can be used to edit the dungeon, preview the dungeon, start a playable PIE test, and then return to editing without opening `L_GrimrockRuntime`.

## 2. Required Actors

`L_GrimrockEditor` should contain:

- `BP_GridLevelEditorActor`
- `BP_GridLevelRuntimeActor`
- the existing `BP_GrimrockGameMode`, configured as described below
- `BP_GrimrockPartyPawn` only if the map intentionally uses a placed pawn instead of GameMode spawning
- a valid `UGridLevelAsset`

`BP_GridLevelEditorActor.LevelAsset` and `BP_GridLevelRuntimeActor.LevelAsset` should point to the same `UGridLevelAsset`.

`BP_GrimrockGameMode` already exists. Do not create another Blueprint with this name.

Open the existing `BP_GrimrockGameMode` and configure it as follows:

- reparent it to `AGrimrockGameMode` if it is still parented to `GameModeBase`;
- set `Default Pawn Class` to `BP_GrimrockPartyPawn`;
- set `Player Controller Class` to `BP_GrimrockPlayerController` if that Blueprint exists.

`AGrimrockGameMode` remains useful as the C++ parent and fallback. If `GlobalDefaultGameMode` points directly to `/Script/GrimrockPrototype.GrimrockGameMode`, PIE will spawn the native C++ `AGrimrockPartyPawn` unless a Blueprint override is used. Use the existing `BP_GrimrockGameMode` as the default GameMode when the project should spawn `BP_GrimrockPartyPawn`.

## 3. Workflow

1. Open `L_GrimrockEditor`.
2. Select a playable cell.
3. Run `SetStartFromSelection` on `BP_GridLevelEditorActor`.
4. Save the `UGridLevelAsset`.
5. Click Play.

`PreparePIETestFromStart` is still available as a manual debug tool, but it is no longer part of the normal workflow. The editor module automatically runs the same preparation logic when PIE starts from a map containing `BP_GridLevelEditorActor`.

Automatic PIE preparation synchronizes the preview runtime actor, copies `LevelAsset`, `DungeonAsset`, and `CurrentDungeonLevelId`, enables LevelAsset start application on begin play, rebuilds the runtime preview, and logs readiness diagnostics.

If automatic preparation fails and `bAbortPIEOnPreparationError` is true, the editor requests PIE to stop immediately. Unreal's PIE delegate does not provide a clean veto in this path, so failure may appear as PIE starting and then immediately ending. Check the log for the exact error.

## 4. Diagnostics

Useful diagnostics:

- `LogEditorRuntimeAssetConsistency`
- `LogLevelAssetDiagnostics`
- `LogPIEReadinessDiagnostics`

Use `LogEditorRuntimeAssetConsistency` first when the editor actor and preview runtime actor may not reference the same `UGridLevelAsset`.

Use `LogLevelAssetDiagnostics` to inspect the runtime actor's assigned asset, grid size, object counts, links, and start cell.

Use `LogPIEReadinessDiagnostics` before Play to check the runtime actor, start cell, meshes, components, archetypes, and pawn availability in the current world.

In PIE, `LogPIEReadinessDiagnostics` also reports the active GameMode class, Default Pawn Class and Player Controller Class. If it reports the native `AGrimrockGameMode`, the spawned pawn is the native C++ pawn unless the active GameMode Blueprint overrides it. If it reports `BP_GrimrockGameMode`, check that Blueprint's class settings.

## 5. Difference with L_GrimrockRuntime

`L_GrimrockEditor` is for editing and fast local PIE tests.

`L_GrimrockRuntime`, or a future `L_GrimrockGame`, remains the cleaner gameplay test map. It should be used to validate game flow without editor helper actors, editor grid overlays, or temporary editing setup.
