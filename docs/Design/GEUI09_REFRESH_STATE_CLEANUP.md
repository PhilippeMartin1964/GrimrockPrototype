# GEUI09 — Refresh and State Cleanup

**Date:** 28 August 2026  
**Status:** implemented on master — UE5.5.4 build and visual validation pending

## 1. Objective

GEUI01 introduced lightweight `Tick()` observation so detached Nomad tabs could follow Grid Editor context without adding a parallel event system.

That was intentionally temporary.

By GEUI08 the workspace set had become stable enough to refine the observation policy.

GEUI09 does two things:

1. scope observed context to the workspace that actually needs it;
2. preserve editor-session UI state across workspace destruction/recreation.

GEUI09 deliberately does **not** introduce a global editor event bus or subsystem.

## 2. Problem before GEUI09

Every `SGridEditorWorkspaceTab` observed the same broad context:

~~~text
DungeonAsset
LevelAsset
ObjectPalette
CurrentDungeonLevelId
SelectedPaletteEntryId
SelectedObjectId
SelectedCell
SelectedEdge
ActiveTool
ObjectCount
LinkCount
Patrol edit state
Patrol waypoint
~~~

Any difference rebuilt the complete content of **every open workspace**.

Examples of unnecessary work:

- selecting another object rebuilt Tools & Palette;
- selecting another cell rebuilt PlayTest & Validation;
- changing the active paint tool could rebuild unrelated windows;
- connector count changes could rebuild windows that never display connectors.

This also increased the risk of destroying transient Slate interaction state.

## 3. Workspace-specific observation

GEUI09 keeps one lightweight context poll, but evaluates only relevant fields for each workspace.

### Dungeon Levels

Observed:

~~~text
DungeonAsset
LevelAsset
CurrentDungeonLevelId
SelectedCell
SelectedEdge
ObjectCount
~~~

This keeps the level list and overview selection/current geometry synchronized.

### PlayTest & Validation

Observed:

~~~text
DungeonAsset
LevelAsset
CurrentDungeonLevelId
~~~

Object/cell selection no longer rebuilds the validation search/filter interface.

Actions inside PlayTest and Validation already request their own refresh when needed.

### Tools & Palette

Observed:

~~~text
ObjectPalette
SelectedPaletteEntryId
ActiveTool
~~~

Viewport cell/object selection no longer destroys and recreates the palette workspace.

### Selected Object

Observed:

~~~text
DungeonAsset
LevelAsset
CurrentDungeonLevelId
ObjectPalette
SelectedObjectId
SelectedCell
SelectedEdge
ObjectCount
LinkCount
Patrol edit state
Selected patrol waypoint
~~~

This workspace remains the most selection-sensitive by design.

## 4. Actor polling cleanup

The observed editor actor weak pointer is reused while valid.

`FindEditorActor()` is therefore no longer required simply to rediscover the same actor on every context check.

A dedicated:

~~~text
bObservedHasEditorActor
~~~

flag also lets the workspace detect actor disappearance/reappearance cleanly instead of leaving stale detached content when the editor actor is destroyed.

## 5. Session UI state

GEUI09 adds one editor-session presentation state owned inside the editor module implementation.

It preserves:

~~~text
ToolPaletteState
ValidationState
SelectedObjectPage
ValidationLevelAsset
~~~

Consequences:

- Palette search/view/favorites/recent state survives closing and reopening Tools & Palette in the same UE session;
- Validation filters/search/results survive temporary workspace destruction;
- Properties vs Connectors survives Selected Object tab recreation;
- mode exit/re-entry no longer resets those presentation choices.

This is **not gameplay data** and is not stored in `.uasset` or `.umap`.

Favorites/Recent keep their existing per-user config persistence from GEUI07.

## 6. Validation safety on level change

Validation messages belong to one level.

Therefore, when the PlayTest & Validation workspace detects a different `UGridLevelAsset` than the one associated with the session validation state, GEUI09 clears:

~~~text
ValidationMessages
bValidationHasRun
~~~

The user's search and severity-filter preferences remain intact.

This prevents stale errors from a previous dungeon level appearing as if they belonged to the newly selected level.

## 7. No global event bus yet

GEUI09 intentionally does not add:

~~~text
UGridEditorSubsystem
FGridEditorEventBus
global multicast delegates
new UObject notification models
~~~

The current editor has one authoring actor and a small number of dockable workspaces.

A global event architecture would currently add more lifecycle complexity than value.

If future player-facing/runtime level authoring requires broader decoupling, that concern belongs to the later plugin/runtime readiness work.

## 8. Files changed

Modified:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorWorkspaceTab.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorWorkspaceTab.cpp
~~~

New:

~~~text
docs/Design/GEUI09_REFRESH_STATE_CLEANUP.md
~~~

No runtime source, `.uasset` or `.umap` is modified.

## 9. Required UE5.5.4 validation

Build:

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Visual/behavior validation:

1. Open all four Grid Editor workspaces.
2. In Validation, enter a multi-character search.
3. Change selected object/cell in the viewport.
4. Confirm the Validation search text remains intact and focused interaction is not unexpectedly reset.
5. In Tools & Palette, choose a category/search.
6. Change viewport cell/object selection.
7. Confirm palette UI state remains unchanged.
8. Switch Selected Object to Connectors.
9. Leave Grid Editor and return.
10. Confirm Selected Object returns to Connectors.
11. Confirm Tools & Palette retains its session view/search.
12. Confirm Validation retains its search/filter state.
13. Change to a different dungeon level.
14. Confirm old validation messages are cleared and Validation reports that no validation has run for the new level.
15. Return to the previous level and run validation again.
16. Confirm no runtime/PIE behavior changes.

## 10. Explicit non-goals

GEUI09 does not:

- change authoring data;
- change runtime behavior;
- change connector semantics;
- change validation rules;
- add a global editor subsystem;
- persist every UI detail between separate UE launches;
- modify `.uasset` or `.umap`;
- open MON21.4.

## 11. Next step

After validation:

~~~text
GEUI10 — Plugin / Player Level Editor readiness audit
~~~

GEUI10 should be an architectural audit/documentation milestone, not a refactor. It will classify the current Grid Editor code into editor-only, reusable, runtime-compatible and future extraction candidates before returning focus to real dungeon content authoring.
