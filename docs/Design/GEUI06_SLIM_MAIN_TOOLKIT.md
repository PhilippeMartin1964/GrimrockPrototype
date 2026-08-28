# GEUI06 — Slim Main Toolkit

**Date:** 28 August 2026  
**Status:** implemented on master — UE5.5.4 build and visual validation pending

## 1. Objective

GEUI06 completes the first Grid Editor workspace migration by removing the large authoring panels that were still duplicated inside the editor-mode Toolkit.

The main Toolkit is no longer the place where level authoring panels are embedded.

It now acts as a compact dashboard containing:

- Dungeon Editor title;
- current editor context badges;
- connector viewport display toggles;
- workspace launch buttons.

## 2. Removed inline authoring sections

The following legacy inline sections are removed from the main Toolkit:

~~~text
DUNGEON LEVELS
PLAYTEST
TOOLS / PALETTE
OVERVIEW MAP
SELECTED OBJECT
CONNECTORS
VALIDATION
~~~

Their authoritative dockable replacements are:

~~~text
Dungeon Levels
PlayTest & Validation
Tools & Palette
Selected Object
~~~

Lua authoring remains in:

~~~text
Grimrock Lua Scripts
~~~

No feature is deleted; only the duplicate presentation is removed.

## 3. Workspace launcher

The Toolkit now exposes five explicit launch buttons:

~~~text
Dungeon Levels
PlayTest & Validation
Tools & Palette
Selected Object
Grimrock Lua Scripts
~~~

Each button invokes the already registered Nomad tab through `FGlobalTabmanager`.

If a workspace is already open, Unreal brings that tab forward rather than constructing a second independent authoring implementation.

## 4. Shared workspace tab identifiers

GEUI06 adds:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridEditorWorkspaceTabs.h
~~~

This header centralizes the canonical tab identifiers used by both:

- `FGrimrockPrototypeEditorModule` for registration/menu entries;
- `FGridLevelEdModeToolkit` for dashboard launch buttons.

This removes string/name duplication between module registration and the launcher.

Canonical identifiers remain unchanged:

~~~text
GrimrockGridDungeonLevels
GrimrockGridPlaytestValidation
GrimrockGridToolsPalette
GrimrockGridSelectedObject
GrimrockLuaEditor
~~~

## 5. Main dashboard status

The dashboard keeps lightweight context only.

Status badges:

~~~text
Tool
Cell
Edge/Facing
Object
Level
~~~

The old Validation badge is removed because its presentation state belonged to the legacy inline Validation widget and would become stale once that widget disappeared.

Validation status now lives in the dedicated `PlayTest & Validation` workspace.

## 6. Display parameters retained

These viewport-only toggles remain in the main dashboard:

~~~text
Show Outgoing Connectors
Show Incoming Connectors
Show Connector Labels
~~~

They remain useful global display settings and do not belong exclusively to the Selected Object / Connectors authoring page.

Their behavior is unchanged.

## 7. Toolkit cleanup

GEUI06 removes Toolkit-only presentation state that is no longer needed:

~~~text
FGridEditorPanelExpansionState
ToolPaletteState
ValidationState
BuildCollapsiblePanelSection
TogglePanelExpansion
ExpandValidationIfMessagesNeedAttention
GetValidationStatusText
~~~

The public method name `RefreshPalette()` is intentionally retained because `FGridLevelEdMode` already uses it as the Toolkit refresh entry point. Its implementation now refreshes the compact dashboard rather than rebuilding an inline palette.

The Toolkit display name changes from:

~~~text
Grimrock Grid Palette
~~~

to:

~~~text
Grimrock Grid Editor
~~~

## 8. Files changed

New:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridEditorWorkspaceTabs.h
docs/Design/GEUI06_SLIM_MAIN_TOOLKIT.md
~~~

Modified:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEdModeToolkit.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.cpp
~~~

No runtime source, `.uasset` or `.umap` is modified.

## 9. Required UE5.5.4 validation

Build:

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Visual validation:

1. Open `L_GrimrockEditor`.
2. Activate `Grimrock Grid Editor`.
3. Confirm the inline Toolkit no longer contains the seven legacy authoring panels.
4. Confirm the Toolkit contains:
   - DUNGEON EDITOR title;
   - Tool / Cell / Edge-Facing / Object / Level badges;
   - three connector display toggles;
   - WORKSPACE launcher.
5. Open each launcher button and confirm the expected Nomad tab appears.
6. Click a launcher for an already open window and confirm Unreal focuses/reuses it.
7. Change selected cell/object/tool and confirm dashboard badges refresh.
8. Toggle outgoing/incoming/labels and confirm viewport behavior is unchanged.
9. Confirm `Window` menu entries still open all five workspaces.
10. Run a short PIE smoke test.

## 10. Explicit non-goals

GEUI06 does not:

- remove any authoring capability;
- remove the underlying shared widgets;
- change level/runtime data;
- change connector logic;
- change validation logic;
- change PIE preparation;
- persist workspace layout;
- create a plugin;
- modify `.uasset` or `.umap`;
- open MON21.4.

## 11. Next step

After build and visual validation, the first workspace migration phase is complete.

The next roadmap item is:

~~~text
GEUI07 — Palette UX
~~~

GEUI07 can then focus purely on higher-level palette productivity features such as Favorites, Recently Used and user-oriented organization without carrying the old monolithic Toolkit constraints.


## GEUI06.1 — Scope workspace windows to Grid Editor mode

Workspace Nomad tabs are now explicitly tied to the lifetime of:

~~~text
EM_GrimrockGridLevelEdMode
~~~

Behavior:

- while Grimrock Grid Editor is active, the workspace tabs can be opened normally;
- when the mode is exited, all live Grimrock workspace tabs are requested to close;
- while the mode is inactive, their Nomad spawners reject creation through `FCanSpawnTab`;
- Unreal's Window menu therefore cannot spawn these authoring windows outside Grid Editor mode.

The scoped tabs are:

~~~text
Dungeon Levels
PlayTest & Validation
Tools & Palette
Selected Object
Grimrock Lua Scripts
~~~

`GridEditorWorkspaceTabs::All()` centralizes this list so close behavior and canonical identifiers cannot drift apart.

This is editor-only lifecycle behavior and does not affect level/runtime data.


## GEUI06.2 — Restore open workspace tabs when re-entering Grid Editor

GEUI06.1 correctly hid Grid Editor windows outside the editor mode, but it did not remember which workspace windows were open when the user left the mode.

GEUI06.2 adds session restore behavior.

### Exit behavior

Before closing workspace tabs, `FGridLevelEdMode::Exit()` now records the exact set of live Grimrock workspace `TabId` values.

Only tabs that were actually open are recorded.

If a user manually closed a workspace before leaving Grid Editor, that workspace is not restored later.

### Enter behavior

After `FGridLevelEdMode::Enter()` has activated the mode and initialized its Toolkit, every recorded workspace tab is invoked again through:

~~~text
FGlobalTabmanager::TryInvokeTab
~~~

Because the same stable TabIds are reused, Unreal's docking layout can restore each tab into its previous dock stack / floating window location rather than creating an unrelated authoring surface.

The expected workflow is therefore:

~~~text
Grid Editor active
  -> Dungeon Levels + Tools & Palette open and positioned

Leave Grid Editor
  -> those windows disappear

Return to Grid Editor
  -> Dungeon Levels + Tools & Palette reopen automatically
     in their remembered Unreal docking locations
~~~

This restore list is session-only editor presentation state. It does not touch gameplay assets or level data.
