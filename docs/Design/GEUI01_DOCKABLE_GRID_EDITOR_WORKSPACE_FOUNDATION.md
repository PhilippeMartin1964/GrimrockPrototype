# GEUI01 — Dockable Grid Editor Workspace Foundation

**Date:** 28 August 2026  
**Status:** implemented on master — UE5.5.4 build and visual validation pending

## 1. Objective

GEUI01 creates the low-risk docking foundation for the Grimrock Grid Editor ergonomic redesign.

The milestone deliberately does not remove or rewrite the current inline toolkit. Its purpose is to prove that the existing Slate panels can live in independent Unreal Editor Nomad tabs before the larger UI is migrated.

The authoritative editing model remains unchanged:

~~~text
UGridDungeonAsset
    -> UGridLevelAsset
        -> Cells / Objects / Links / Lua / Quest references
~~~

No alternate level representation is introduced.

## 2. New dockable windows

Four Nomad tabs are registered by GrimrockPrototypeEditor:

~~~text
Dungeon Levels
PlayTest & Validation
Tools & Palette
Selected Object
~~~

The existing tab remains unchanged:

~~~text
Grimrock Lua Scripts
~~~

All five entries are exposed through the Unreal Editor Window menu and use FGlobalTabmanager / SDockTab with ETabRole::NomadTab.

They can therefore be docked, undocked, floated and closed using Unreal's native workspace behavior.

## 3. Reuse-first composition

GEUI01 intentionally reuses the existing Slate widgets.

### Dungeon Levels

Hosts:

~~~text
SGridEditorOverviewMapPanel
~~~

The level list and level-management actions still live in FGridLevelEdModeToolkit until GEUI02. A migration notice in the tab makes that temporary boundary explicit.

### PlayTest & Validation

Hosts:

~~~text
SGridEditorValidationPanel
~~~

The existing PlayTest controls still live in FGridLevelEdModeToolkit until GEUI03.

The validation rules are not duplicated. The panel continues to call the existing authoritative GridEditorLuaService::ValidateCurrentLevelWithLua path.

### Tools & Palette

Hosts the existing:

~~~text
SGridEditorToolPalettePanel
~~~

Tool selection and palette mutations still target AGridLevelEditorActor and UGridObjectPaletteAsset exactly as before.

### Selected Object

Composes the existing:

~~~text
SGridEditorObjectInspectorPanel
SGridEditorLinksPanel
~~~

No object property or connector mutation logic is copied into the workspace host.

## 4. Workspace host

New widget:

~~~text
SGridEditorWorkspaceTab
~~~

Location:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorWorkspaceTab.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorWorkspaceTab.cpp
~~~

Responsibilities are intentionally narrow:

- find the current AGridLevelEditorActor in the editor world;
- compose the existing widget corresponding to the requested workspace tab;
- own presentation-only Tool Palette and Validation panel state for that Nomad tab;
- rebuild the detached tab when lightweight editor context changes.

It does not own dungeon, level, object, connector or runtime data.

## 5. Context synchronization

The old inline Toolkit receives explicit refresh calls from FGridLevelEdMode.

A detached Nomad tab does not automatically receive those calls, so GEUI01 observes a small set of existing editor context values:

- current editor actor;
- DungeonAsset / LevelAsset / ObjectPalette;
- CurrentDungeonLevelId;
- selected cell and edge;
- selected object;
- active tool;
- selected palette entry;
- object/link counts;
- monster patrol-route edit selection.

When this context changes, only the open workspace host rebuilds its Slate content.

This is intentionally not a new subsystem, event bus or editor data model.

A future GEUI09 may replace this conservative observation with more targeted notifications after the new workspace is proven stable.

## 6. Validation state note

The PlayTest & Validation Nomad tab owns its current presentation state (message filters and last displayed results), while the old inline Validation panel keeps its own current presentation state.

Both execute the same authoritative validation logic.

This temporary UI-state duplication is accepted for GEUI01 because the inline toolkit remains the compatibility fallback. GEUI03 will decide the single canonical presentation state when PlayTest and Validation are actually migrated.

## 7. Main toolkit remains intact

GEUI01 does not remove:

~~~text
DUNGEON LEVELS
PLAYTEST
TOOLS / PALETTE
OVERVIEW MAP
SELECTED OBJECT
CONNECTORS
VALIDATION
~~~

from FGridLevelEdModeToolkit.

This is deliberate. The new windows are introduced first, validated in UE5.5.4, then the old inline sections will be migrated one responsibility at a time.

## 8. Files changed

New:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorWorkspaceTab.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorWorkspaceTab.cpp
docs/Design/GEUI01_DOCKABLE_GRID_EDITOR_WORKSPACE_FOUNDATION.md
~~~

Modified:

~~~text
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.cpp
~~~

No runtime source, .uasset or .umap is modified.

## 9. Required UE5.5.4 validation

Build:

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5
~~~

At minimum confirm that GrimrockPrototypeEditor builds successfully.

Visual validation in Unreal Editor:

1. Open L_GrimrockEditor.
2. Activate Grimrock Grid Editor.
3. Open Window and verify exactly one entry for each:
   - Dungeon Levels
   - PlayTest & Validation
   - Tools & Palette
   - Selected Object
   - Grimrock Lua Scripts
4. Open all four new Grid Editor tabs.
5. Dock, undock and float each tab.
6. Select cells and objects in the viewport and verify the detached panels follow the selection.
7. Change tools and palette entries from Tools & Palette.
8. Verify Selected Object edits still update the same LevelAsset.
9. Create/remove a connector from the detached Selected Object window.
10. Run validation from PlayTest & Validation.
11. Confirm the original inline Toolkit still works unchanged.
12. Start PIE once to confirm the editor module registration changes did not affect PIE preparation.

## 10. Explicit non-goals

GEUI01 does not:

- extract Dungeon Levels controls from FGridLevelEdModeToolkit;
- extract PlayTest controls from FGridLevelEdModeToolkit;
- redesign palette categories/search/favorites/recent items;
- redesign Selected Object into final internal tabs;
- merge validation presentation state with the old inline panel;
- create a Grid Editor subsystem;
- move AGridLevelEditorActor;
- move UGridLevelAsset or UGridDungeonAsset;
- create a plugin;
- modify runtime behavior;
- modify .uasset or .umap files;
- open MON21.4.

## 11. Next migration steps

After UE5.5.4 validation:

~~~text
GEUI02 — Dungeon Levels Window
    extract the level list/actions and compose them with Overview Map

GEUI03 — PlayTest & Validation Window
    migrate PlayTest controls and establish the canonical validation presentation
~~~

The old inline sections should only be removed after their corresponding dockable window is functionally validated.
