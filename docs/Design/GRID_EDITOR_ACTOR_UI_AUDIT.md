# Grid Editor Actor UI Audit

## Objective

This audit reviews every user-facing property and exposed function on `AGridLevelEditorActor`.

The goal is to reduce noise in the `BP_GridLevelEditorActor` Details panel without changing behavior yet. This document identifies what should remain on the actor, what should move to the Grimrock Grid Editor Mode, what should become advanced/debug-only, and what may be obsolete or duplicated.

No code should be removed or renamed as part of this audit step.

## Current State

`BP_GridLevelEditorActor` currently acts as:

- technical configuration holder for the editor map;
- daily editing state container;
- Slate toolkit command target;
- diagnostic command surface;
- PIE preparation coordinator;
- legacy CallInEditor button host.

That mix was useful while the editor was being built, but it now makes Details too crowded. The intended direction is:

- `BP_GridLevelEditorActor`: technical configuration and recoverable debug controls.
- Grimrock Grid Editor Mode: daily editing interface.
- Runtime actors/assets: gameplay execution and source-of-truth data.

## Property Audit

| Property | Current Category | Role | Usage | Keep Visible on Actor | Control from Editor Mode | Advanced/Debug Candidate | Risk if Hidden |
|---|---|---|---|---|---|---|---|
| `LevelAsset` | `Editor` | Current level being edited. Also mono-level fallback. | Core config, diagnostic. | Yes, preferably read-only/visible once dungeon workflow is stable. | Yes, indirectly through `DUNGEON LEVELS`. | No for now. | Medium: hiding too early makes asset sync harder to inspect. |
| `DungeonAsset` | `Editor|Dungeon` | Multi-level source list for editor workflow. | Core config. | Yes. | Yes, display and level list. | No. | High: required once per editor map. |
| `CurrentDungeonLevelId` | `Editor|Dungeon` | Current selected dungeon level. | Daily workflow, diagnostics. | Yes, but eventually mostly status. | Yes, primary control in `DUNGEON LEVELS`. | No. | Medium: useful for recovery if toolkit is unavailable. |
| `PreviewRuntimeActor` | `Editor` | Runtime preview/sync target. | Core technical reference. | Yes. | No direct daily control. | Maybe later. | High: misconfiguration breaks preview/PIE preparation. |
| `SceneRoot` | `Editor` | Actor component root. | Internal component. | No. | No. | Yes or hidden via component visibility only. | Low. |
| `SelectedCellX`, `SelectedCellY`, `SelectedEdge` | `Selection` | Current editor selection. | Daily state. | No long-term. | Yes, overview/viewport/header. | No, but should be Details-hidden eventually. | Low: toolkit shows selection better. |
| `HoveredCellX`, `HoveredCellY`, `HoveredEdge`, `HoveredObjectId` | `Selection` | Viewport hover state. | Internal/editor feedback. | No. | Yes, internal to viewport/toolkit. | Yes. | Low. |
| `ObjectHoverPickRadius` | `Selection` | Object hover tuning. | Advanced editor tuning. | No for daily use. | Maybe in debug/settings panel. | Yes. | Low/Medium: useful if picking feels wrong. |
| `ViewportPickTraceDistance` | `Viewport Picking` | Viewport trace tuning. | Advanced editor tuning. | No for daily use. | Maybe in debug/settings panel. | Yes. | Low. |
| `PaintCellType` | `Cell Paint` | Cell paint brush. | Daily editing. | No long-term. | Yes, `TOOLS / PALETTE`. | No. | Low if toolkit is complete. |
| `bPaintCellHasCeiling`, `bPaintCellBlocksOccupancy` | `Cell Paint` | Cell paint brush flags. | Daily editing. | No long-term. | Yes. | No. | Medium if toolkit does not expose them clearly. |
| `PaintWallType` | `Wall Paint` | Wall paint brush. | Daily editing. | No long-term. | Yes. | No. | Low. |
| `PaintObjectType` | `Object Paint` | Legacy object type brush. | Mostly superseded by palette/archetypes. | No long-term. | Yes, via palette/tool panel if needed. | Maybe. | Low/Medium until all objects use palette. |
| `bObjectInitiallyEnabled`, `bObjectInitiallyActive` | `Object Paint` | New object initial state. | Daily/advanced object placement. | No long-term. | Yes, object inspector/tool palette. | Maybe if rarely used. | Medium if hidden before inspector coverage is complete. |
| `ObjectArchetypeId` | `Object Paint` | Selected archetype for object placement. | Daily through palette. | No long-term. | Yes, palette. | No. | Low if palette remains stable. |
| `ObjectTag`, `ObjectNotes` | `Object Paint` | Metadata for new/selected object. | Object editing. | No long-term. | Yes, object inspector. | No. | Medium if inspector editing is incomplete. |
| `PlacementPolicy` | `Object Paint` | Conflict handling while placing objects. | Daily/advanced editing. | No long-term. | Yes, tool panel. | Maybe. | Medium. |
| `LastSelectedObjectId` | `Selection` | Selection memory. | Internal state. | No. | Yes, internal. | Yes. | Low. |
| `ActiveTool` | `Tool` | Current editor tool. | Daily editing. | No long-term. | Yes, primary toolkit control. | No. | Low. |
| `LinkSourceEvent`, `LinkCommand` | `Link` | Link creation settings. | Daily link editing. | No long-term. | Yes, links panel. | No. | Low if links panel remains complete. |
| `PendingLinkSourceObjectId`, `bHasPendingLinkSource` | `Link` | Link operation state. | Internal state. | No. | Yes, internal/link panel display. | Yes. | Low. |
| `ObjectPalette` | `Palette` | Palette asset for archetypes. | Core config. | Yes. | Yes, display/use in palette panel. | No. | High: if missing, editor loses palette workflow. |
| `SelectedPaletteEntryId`, `SelectedArchetypeId` | `Palette` | Palette selection state. | Daily via toolkit. | No long-term. | Yes. | Maybe. | Low. |
| `ObjectBehavior` | `Object Paint` | New/selected object behavior payload. | Used by inspector and object placement. | No long-term. | Yes, object inspector. | Maybe advanced raw struct only. | Medium/High: powerful but noisy raw data. |
| `LastValidationMessages` | `Validation` | Latest validation results. | Panel state/debug. | No. | Yes, validation panel. | Yes. | Low. |
| `CoordinateGridPlane` | `Editor Grid` | Preview helper component. | Internal component. | No. | No direct. | Yes. | Low. |
| `CoordinateGridPlaneMesh`, `CoordinateGridMaterial` | `Editor Grid` | Grid display technical assets. | Technical config. | Maybe, advanced. | No daily control. | Yes. | Medium if visuals need repair. |
| `bShowCoordinateGrid`, `bShowCoordinateLabels` | `Editor Grid` | Editor display toggles. | Daily/optional display. | Maybe until moved. | Yes, display panel/header. | No. | Low once toolkit has controls. |
| `CoordinateGridZOffset`, `CoordinateLabelWorldSize` | `Editor Grid` | Display tuning. | Advanced display tuning. | No daily. | Maybe debug/settings panel. | Yes. | Low. |
| `bShowOutgoingConnectors`, `bShowIncomingConnectors`, `bShowConnectorLabels` | `Editor|Connectors` | Connector overlay toggles. | Daily display. | No long-term. | Yes, header/display panel. | No. | Low: already present in header. |
| `bHideEditorActorDuringPIE` | `Editor|PIE` | PIE cleanup/display behavior. | Technical PIE config. | Yes. | Optional status only. | Maybe. | Medium. |
| `bAutoPreparePIE` | `Editor|PIE` | Automatic PIE preparation switch. | Core editor workflow config. | Yes. | Yes, status in `DUNGEON LEVELS`; maybe toggle later. | No. | Medium. |
| `bAbortPIEOnPreparationError` | `Editor|PIE` | Error handling policy for auto PIE. | Technical workflow guard. | Yes. | Maybe status/toggle in Playtest panel. | No/Maybe. | Medium. |

## Function Audit

| Function | Exposure | Role | Called from Toolkit/Slate | Daily Workflow | Keep CallInEditor | Advanced/Debug | Remove from Details Later |
|---|---|---|---|---|---|---|---|
| `EnsureLevelReady` | `CallInEditor`, `BlueprintCallable` | Repair/initialize asset cells. | Not primary toolkit action today. | No. | Temporarily. | Yes. | Yes, eventually debug only. |
| `RebuildPreview` | `CallInEditor`, `BlueprintCallable` | Force preview rebuild. | Indirectly used by many edit operations. | Rare. | Temporarily. | Yes. | Yes. |
| `ApplyCurrentDungeonLevel` | `CallInEditor`, returns bool | Core helper and legacy button. | Yes, `DUNGEON LEVELS`. | Yes, through toolkit. | No long-term because bool CallInEditor is not ideal as a user button. | Maybe. | Yes, keep callable/helper. |
| `ApplyCurrentDungeonLevelInEditor` | `CallInEditor`, void wrapper | Visible Details button for current level. | Equivalent behavior now in toolkit. | No longer primary. | Temporarily. | Maybe. | Likely yes after toolkit validation. |
| `LoadDefaultDungeonLevelInEditor` | `CallInEditor` | Load default dungeon level. | Yes, `DUNGEON LEVELS`. | Yes, through toolkit. | Temporarily. | Maybe. | Likely yes. |
| `LogDungeonDiagnostics` | `CallInEditor` | Dungeon summary diagnostics. | Yes, `DUNGEON LEVELS`. | Debug. | Temporarily. | Yes. | Yes. |
| `LogDungeonTransitionDiagnostics` | `CallInEditor` | Transition diagnostics. | Yes, `DUNGEON LEVELS`. | Debug/validation. | Temporarily. | Yes. | Yes. |
| `GetDungeonDiagnostics` | `BlueprintCallable` | Text generator. | Indirect diagnostic support. | No direct button. | N/A. | Yes. | Keep callable. |
| `LogEditorRuntimeAssetConsistency` | `CallInEditor` | Editor/runtime sync diagnostic. | Not primary UI. | Debug. | Temporarily. | Yes. | Yes. |
| `GetEditorRuntimeAssetConsistencyDiagnostics` | `BlueprintCallable` | Text generator. | Diagnostic support. | No. | N/A. | Yes. | Keep callable. |
| `SyncPreviewRuntimeLevelAsset` | `CallInEditor` | Force preview asset sync. | Used internally by apply/prepare. | No. | Temporarily. | Yes. | Yes, should become internal/debug callable. |
| `PreparePIETestFromStart` | `CallInEditor` | Manual PIE prep. | Superseded by auto PIE prepare. | No normal use. | Temporarily for debug. | Yes. | Yes. |
| `PreparePIETestFromStartInternal` | `BlueprintCallable` | Reusable auto PIE implementation. | Editor module calls it. | Internal. | N/A. | Yes. | Keep callable only if needed by automation/tests. |
| `SetStartFromSelection` | `CallInEditor` | Set LevelAsset start from selected cell. | Should move to Playtest panel. | Yes. | Temporarily. | No. | Yes once Playtest panel has it. |
| `ClearSelectedCell`, `PaintSelectedWall`, `ClearSelectedWall` | `BlueprintCallable` | Tool actions. | Called via tool flow. | Yes, through tools. | N/A. | No. | Keep callable, not Details buttons. |
| `PlaceSelectedObject`, `RemoveObjectsAtSelection` | `CallInEditor`, `BlueprintCallable` | Object edit actions. | Tool/object UI uses related flows. | Yes, through toolkit. | Temporarily. | Maybe. | Yes once toolkit covers them. |
| `SelectObjectAtSelection` | `BlueprintCallable` | Selection command. | Tool/viewport use. | Yes, indirect. | N/A. | No. | Keep callable. |
| `ApplyEditedSelectedObject` | `CallInEditor`, returns bool | Legacy apply button for selected object fields. | Object inspector calls it in one path. | Possibly obsolete because many inspector fields auto-save. | Temporarily. | Maybe. | Candidate for removal from Details after inspector audit. |
| `ApplyViewportHitSelection` | `BlueprintCallable` | Viewport hit-to-selection. | Editor mode/viewport. | Internal. | N/A. | No. | Keep callable/internal. |
| `IsSelectionValidForEditing`, `SelectCellFromOverview`, `ApplyGridHoverFromWorldPoint`, `CommitHoveredCellSelection`, `GetSelectionPreviewCenter` | `BlueprintCallable` | Selection/overview helpers. | Yes, overview/viewport. | Through toolkit. | N/A. | No. | Keep callable/internal. |
| `UpdateHoveredObjectFromWorldPoint`, `SelectHoveredObject`, `TryGetHoveredObjectWorldLocation` | `BlueprintCallable` | Object hover helpers. | Viewport/inspector. | Internal. | N/A. | Maybe. | Keep callable/internal. |
| `ApplyPrimaryToolAction`, `ApplySecondaryToolAction`, `EraseAtSelection` | `BlueprintCallable` | Main tool command handlers. | Yes, editor mode invokes primary action. | Through viewport. | N/A. | No. | Keep callable/internal. |
| `BeginOrCompleteLinkAtSelection`, `ClearPendingLinkSource`, `RemoveLinksAtSelection` | `BlueprintCallable` | Link workflow commands. | Link tool/panel. | Through toolkit. | N/A. | No. | Keep callable/internal. |
| `HasPendingLinkSource`, `TryGetPendingLinkSourceLocation` | `BlueprintCallable` | Link display helpers. | Link panel/viewport. | Internal. | N/A. | Maybe. | Keep callable/internal. |
| `RemoveLinkByIndexForSelectedObject`, `RemoveAllLinksForSelectedObject` | `BlueprintCallable` | Link cleanup commands. | Links panel. | Through toolkit. | N/A. | No. | Keep callable/internal. |
| `CreateLink`, `RemoveExactLink` | `BlueprintCallable` | Explicit link edits. | Yes, `SGridEditorLinksPanel`. | Through links panel. | N/A. | No. | Keep callable/internal. |
| `TryGetSelectedObjectWorldLocation`, `SelectObjectById`, `TryGetObjectWorldLocationById`, `FocusSelectedObject` | `BlueprintCallable` / `CallInEditor` | Selection/focus helpers. | Object/links panels. | Through toolkit. | Maybe only `FocusSelectedObject` was visible. | Maybe. | Hide Details button later; keep helper. |
| `ApplyPaletteEntry`, `ApplySelectedPaletteEntry` | `BlueprintCallable` / `CallInEditor` | Palette selection. | Tool palette panel. | Through toolkit. | `ApplySelectedPaletteEntry` likely not long-term. | Maybe. | Yes for Details button. |
| `GetSelectedObjectData`, `FindObjectArchetypeById`, `GetObjectEditorWorldCenter`, `SetSelectedObjectOrientation` | C++ public helpers | Slate support. | Yes. | Internal. | N/A. | No. | Keep C++ helpers. |
| `ApplyBehaviorToSelectedObject`, `ResetSelectedObjectBehaviorFromArchetype`, setters for archetype/tag/notes/readable/enabled/active/orientation | `BlueprintCallable` | Object inspector auto-save/editing. | Yes, object inspector. | Through toolkit. | N/A. | No. | Keep callable/internal. |
| `MoveSelectedObjectToCurrentSelection` | `CallInEditor`, `BlueprintCallable` | Move selected object to selected cell. | Object inspector has action. | Through toolkit. | Temporarily. | Maybe. | Yes once inspector action is enough. |
| `ValidateCurrentLevel` | `CallInEditor`, `BlueprintCallable` | Level validation. | Yes, validation panel. | Yes through toolkit. | Temporarily. | No. | Yes once validation panel is canonical. |

## Classification

### A. Configuration Essential On `BP_GridLevelEditorActor`

These should remain visible for now:

- `DungeonAsset`
- `CurrentDungeonLevelId`
- `LevelAsset` as current level/status and mono-level fallback
- `PreviewRuntimeActor`
- `ObjectPalette`
- `bAutoPreparePIE`
- `bAbortPIEOnPreparationError`
- `bHideEditorActorDuringPIE`

Possible later refinement:

- Move to `Editor|Core`: `DungeonAsset`, `CurrentDungeonLevelId`, `LevelAsset`, `PreviewRuntimeActor`, `ObjectPalette`.
- Keep `LevelAsset` visible but treat it as current state rather than the primary multi-level selector.

### B. Daily Interface To Centralize In Grimrock Grid Editor Mode

These belong in toolkit panels, not Details:

- dungeon level selection;
- `Load Default`;
- `Reload Current`;
- `SetStartFromSelection`;
- active tool selection;
- cell/wall/object paint brush state;
- palette entry selection;
- selected object inspector;
- connector display toggles;
- link creation/removal;
- validation run and validation result display;
- dungeon/transition logs as compact toolkit actions.

Suggested panels:

- `DUNGEON LEVELS`: level list, `Load Default`, `Reload Current`, `Log Dungeon`, `Log Transitions`.
- `TOOLS / PALETTE`: `Select`, `Paint Cell`, `Paint Wall`, `Paint Object`, `Erase`, `Link`, palette entries.
- `PLAYTEST`: `Auto PIE Prepare` status, `Set Start From Selection`, current StartCell display, collapsed debug prepare action.
- `VALIDATION`: run validation, show warnings/errors.
- `DEBUG / ADVANCED`: sync/rebuild/diagnostics.

### C. Debug / Advanced

These are useful but should not be part of the daily Details surface:

- `EnsureLevelReady`
- `RebuildPreview`
- `SyncPreviewRuntimeLevelAsset`
- `LogEditorRuntimeAssetConsistency`
- `GetEditorRuntimeAssetConsistencyDiagnostics`
- `PreparePIETestFromStart`
- `PreparePIETestFromStartInternal`
- `LogDungeonDiagnostics`
- `LogDungeonTransitionDiagnostics`
- `CoordinateGridPlaneMesh`
- `CoordinateGridMaterial`
- `CoordinateGridZOffset`
- `CoordinateLabelWorldSize`
- `ObjectHoverPickRadius`
- `ViewportPickTraceDistance`
- `LastValidationMessages`
- hover and pending link state fields.

### D. Internal / Called By Slate / Not User Buttons

These should not appear as user-facing Details buttons, but should remain callable or public while Slate/editor mode depends on them:

- `ApplyPrimaryToolAction`
- `ApplySecondaryToolAction`
- `ApplyViewportHitSelection`
- `ApplyGridHoverFromWorldPoint`
- `CommitHoveredCellSelection`
- `UpdateHoveredObjectFromWorldPoint`
- `SelectHoveredObject`
- `CreateLink`
- `RemoveExactLink`
- `ApplyBehaviorToSelectedObject`
- `ResetSelectedObjectBehaviorFromArchetype`
- object field setters
- object/link/world-location helpers
- `TryGetPendingLinkSourceLocation`
- `TryGetSelectedObjectWorldLocation`
- `TryGetObjectWorldLocationById`

### E. Obsolete / Duplicate / To Confirm

Do not remove these yet, but they deserve follow-up:

- `ApplyCurrentDungeonLevel` vs `ApplyCurrentDungeonLevelInEditor`: the void wrapper exists mainly to force a visible Details button. Toolkit now has the cleaner action.
- `PreparePIETestFromStart`: now a manual debug fallback because auto PIE prepare handles the normal path.
- `ApplyEditedSelectedObject`: likely legacy if the object inspector has moved to auto-save for all fields; confirm remaining path before removing Details exposure.
- `ApplySelectedPaletteEntry`: likely legacy if palette Slate selection is canonical.
- `PlaceSelectedObject` / `RemoveObjectsAtSelection`: useful commands, but daily usage should be through tool actions or object inspector.
- `ValidateCurrentLevel`: should remain callable, but the Details button can become redundant once validation panel is canonical.

## Proposed Future Details Organization

### `Editor|Core`

- `DungeonAsset`
- `CurrentDungeonLevelId`
- `LevelAsset` visible/read-only if possible
- `PreviewRuntimeActor`
- `ObjectPalette`

### `Editor|Display`

- `bShowCoordinateGrid`
- `bShowCoordinateLabels`
- `bShowOutgoingConnectors`
- `bShowIncomingConnectors`
- `bShowConnectorLabels`

The connector toggles are already in the toolkit header; once stable, Details exposure can be reduced.

### `Editor|PIE`

- `bAutoPreparePIE`
- `bAbortPIEOnPreparationError`
- `bHideEditorActorDuringPIE`

### `Editor|Advanced Debug`

- `EnsureLevelReady`
- `RebuildPreview`
- `SyncPreviewRuntimeLevelAsset`
- `LogEditorRuntimeAssetConsistency`
- `PreparePIETestFromStart`
- raw grid material/mesh tuning
- picking radius/distance tuning

### `Editor|Deprecated / Legacy`

Temporary only, if the team wants an explicit holding area before removing Details buttons:

- `ApplyCurrentDungeonLevelInEditor`
- `ApplySelectedPaletteEntry`
- `ApplyEditedSelectedObject`

## Recommended Grid Editor Mode Organization

### `DUNGEON LEVELS`

- level list;
- `Load Default`;
- `Reload Current`;
- `Log Dungeon`;
- `Log Transitions`;
- current level asset/status.

### `TOOLS / PALETTE`

- tool mode;
- cell paint settings;
- wall paint settings;
- object paint settings;
- object palette selection.

### `PLAYTEST`

- `Auto PIE Prepare` status;
- `Set Start From Selection`;
- current StartCell/StartFacing display;
- debug-only manual `PreparePIETestFromStart`, collapsed by default.

### `VALIDATION`

- `Run Validation`;
- warning/error summary;
- clickable messages if possible later.

### `DEBUG / ADVANCED`

- preview sync;
- rebuild preview;
- level asset diagnostics;
- editor/runtime consistency diagnostics;
- picking/display tuning if needed.

## Safe Changes

These are low-risk because they do not alter runtime/editor behavior:

- Add `AdvancedDisplay` metadata to hover state, pending link state, validation messages, grid tuning values, and picking tuning values.
- Move existing Details buttons into an `Editor|Advanced Debug` category.
- Add a `PLAYTEST` panel to the toolkit with `SetStartFromSelection` and StartCell display.
- Keep `BlueprintCallable` while removing `CallInEditor` later from functions that should not be Details buttons.
- Make toolkit actions the canonical daily interface while leaving actor controls as fallback.

## Risky Changes

These require validation before implementation:

- Hiding `LevelAsset` completely: this can obscure mono-level fallback and sync debugging.
- Removing `CallInEditor` from commands still used manually during recovery.
- Hiding `ObjectBehavior` before every behavior field is fully supported in the object inspector.
- Removing `ApplyEditedSelectedObject` before confirming no inspector field still depends on manual apply.
- Collapsing `ApplyCurrentDungeonLevel` and `ApplyCurrentDungeonLevelInEditor` before checking Blueprint references.
- Making `DungeonAsset` mandatory: mono-LevelAsset editing must remain supported.

## Migration Plan

### Step 1: Metadata And Toolkit Parity

- Do not remove anything.
- Add `AdvancedDisplay` to technical/debug fields.
- Move visible debug buttons into `Editor|Advanced Debug`.
- Add `PLAYTEST` panel to the toolkit.
- Confirm toolkit covers all daily actions from Details.

### Step 2: Remove Details Buttons, Keep APIs

- Remove `CallInEditor` from functions that should no longer be user buttons.
- Keep `BlueprintCallable` and C++ public APIs for Slate panels and editor mode.
- Keep debug commands available through a collapsed toolkit panel.
- Validate no Blueprint graph depends on button-only behavior.

### Step 3: Merge Or Delete True Duplicates

- Merge `ApplyCurrentDungeonLevelInEditor` back into a single canonical toolkit command if no longer needed.
- Remove or replace `ApplySelectedPaletteEntry` if palette Slate selection is complete.
- Remove `ApplyEditedSelectedObject` only if object inspector auto-save covers all edited fields.
- Revisit raw object paint fields after StairsUp/StairsDown and transition archetypes are in place.

## Recommended Next Step

Implement Step 1 only:

- add metadata/categories for debug and technical fields;
- add the `PLAYTEST` panel;
- keep every existing function and property intact.

This gives immediate UI cleanup while preserving recovery paths and avoiding behavior regressions before StairsUp/StairsDown are added.
