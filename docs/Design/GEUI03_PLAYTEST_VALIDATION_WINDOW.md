# GEUI03 — PlayTest & Validation Window

**Date:** 28 August 2026  
**Status:** implemented on master — UE5.5.4 build and visual validation pending

## 1. Objective

GEUI03 completes the second dockable authoring workspace by merging the existing PlayTest controls with the existing Validation panel.

The milestone remains presentation-only:

- no PIE preparation rule is changed;
- no validation rule is changed;
- no Lua/Event -> Command validation path is duplicated;
- no runtime class or asset is modified.

## 2. Shared PlayTest widget

The PlayTest controls previously lived directly in:

~~~text
FGridLevelEdModeToolkit::BuildPlaytestPanel
~~~

GEUI03 extracts them into:

~~~text
SGridEditorPlaytestPanel
~~~

Files:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorPlaytestPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorPlaytestPanel.cpp
~~~

The legacy inline Toolkit and the dockable workspace both instantiate this same widget.

There is therefore one Slate implementation for PlayTest behavior.

## 3. Preserved PlayTest controls

The extracted panel preserves:

- Auto Prepare PIE;
- Abort PIE On Error;
- Current LevelAsset;
- Start Cell / Facing / Valid status;
- invalid-start warning;
- Set Start From Selection;
- Log PIE Readiness;
- Debug Prepare PIE.

The action buttons are displayed as a vertically spaced stack to remain readable in a narrow docked pane.

Existing calls remain authoritative:

~~~text
AGridLevelEditorActor::SetStartFromSelection
AGridLevelEditorActor::PreparePIETestFromStart
AGridLevelRuntimeActor::LogPIEReadinessDiagnostics
~~~

## 4. PlayTest & Validation workspace

The Nomad tab now uses a horizontal splitter:

~~~text
+--------------------------------+------------------------------------------+
| PLAYTEST                       | VALIDATION                               |
|                                |                                          |
| Auto Prepare PIE               | Refresh Validation                       |
| Abort PIE On Error             | Copy Summary                             |
|                                |                                          |
| Current LevelAsset             | Errors / Warnings / Infos                |
| Start Cell / Facing / Valid    | severity filters                         |
|                                |                                          |
| Set Start From Selection       | validation message list                  |
| Log PIE Readiness              | Select/Focus object                      |
| Debug Prepare PIE              | Select cell                              |
+--------------------------------+------------------------------------------+
~~~

Initial splitter proportion:

~~~text
PlayTest   : 34%
Validation : 66%
~~~

Each side owns its own scroll area.

## 5. Validation authority

The dockable Validation panel continues to use:

~~~text
SGridEditorValidationPanel
GridEditorLuaService::ValidateCurrentLevelWithLua
~~~

GEUI03 does not introduce a second validator.

The dockable workspace keeps its presentation state (last displayed messages and severity filters) while it is open. The legacy inline Validation section retains its own fallback presentation state until the old monolithic Toolkit is slimmed in GEUI06.

This is UI-state duplication only; validation logic and level data remain single-source.

## 6. Legacy Toolkit

The existing inline `PLAYTEST` section remains visible as fallback, but now delegates to:

~~~text
SGridEditorPlaytestPanel
~~~

The inline `VALIDATION` section is also retained until GEUI06.

This mirrors the GEUI02 migration strategy and keeps rollback risk low.

## 7. Files changed

New:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorPlaytestPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorPlaytestPanel.cpp
docs/Design/GEUI03_PLAYTEST_VALIDATION_WINDOW.md
~~~

Modified:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEdModeToolkit.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorWorkspaceTab.cpp
~~~

No runtime source, `.uasset` or `.umap` is modified.

## 8. Required UE5.5.4 validation

Build:

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Visual validation:

1. Open `L_GrimrockEditor`.
2. Activate `Grimrock Grid Editor`.
3. Open `Window > PlayTest & Validation`.
4. Confirm PlayTest appears on the left and Validation on the right.
5. Drag the splitter.
6. Toggle `Auto Prepare PIE`.
7. Toggle `Abort PIE On Error`.
8. Select a valid cell and use `Set Start From Selection`.
9. Confirm Start Cell / Facing / Valid refresh immediately.
10. Run `Log PIE Readiness`.
11. Run `Refresh Validation`.
12. Exercise Errors / Warnings / Infos filters.
13. Use at least one Select/Focus action from a validation message when available.
14. Confirm the legacy inline PLAYTEST and VALIDATION sections still work.
15. Start PIE once and confirm automatic preparation behavior is unchanged.

## 9. Explicit non-goals

GEUI03 does not:

- add a custom Start PIE command;
- change PreBeginPIE hooks;
- change bAutoPreparePIE semantics;
- change bAbortPIEOnPreparationError semantics;
- change validation rules;
- change Lua validation;
- remove legacy inline panels;
- create a subsystem;
- create a plugin;
- modify runtime behavior;
- modify .uasset or .umap files;
- open MON21.4.

## 10. Next step

After build and visual validation:

~~~text
GEUI04 — Tools & Palette Window
~~~

GEUI04 will keep the existing tool/palette behavior but prepare the dedicated authoring surface for search, category filtering and later Favorites / Recently Used.
