# GEUI02 — Dungeon Levels Window

**Date:** 28 August 2026  
**Status:** implemented on master — UE5.5.4 build and visual validation pending

## 1. Objective

GEUI02 turns the GEUI01 `Dungeon Levels` Nomad tab into the first complete authoring workspace.

The target is a single window that combines:

- dungeon identity and current-level status;
- dungeon-level list and level-management actions;
- the existing 32x32 Overview Map;
- selected-cell/object navigation already provided by the Overview widget.

No new dungeon or level data model is introduced.

## 2. Single level-management widget

The level-management UI previously lived directly inside:

~~~text
FGridLevelEdModeToolkit::BuildDungeonLevelsPanel
~~~

GEUI02 extracts that implementation into:

~~~text
SGridEditorDungeonLevelsPanel
~~~

Files:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorDungeonLevelsPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorDungeonLevelsPanel.cpp
~~~

This widget is now the only Slate implementation of the current level-management workflow.

It is reused by both:

~~~text
legacy inline Grid Editor Toolkit
GEUI02 Dungeon Levels Nomad tab
~~~

This preserves the GEUI01 fallback while avoiding duplicated logic.

## 3. Preserved level actions

The extracted panel preserves the existing behavior:

- Load Default;
- Reload Current;
- Log Dungeon;
- Log Transitions;
- New Level;
- select an enabled dungeon level with a valid LevelAsset.

The `Create New Dungeon Level` transient `SWindow` is preserved.

It still validates:

- non-empty Level Id;
- unique Level Id;
- unique logical position;
- fallback display name.

Creation still delegates to:

~~~text
AGridLevelEditorActor::CreateAndAddDungeonLevel
~~~

No asset-creation contract is duplicated.

## 4. New Dungeon Levels workspace layout

The Nomad tab now uses a horizontal Slate splitter:

~~~text
+------------------------------+------------------------------------------+
| DUNGEON / LEVELS             | OVERVIEW MAP                             |
|                              |                                          |
| Dungeon                      | 32x32 level overview                     |
| Default Level Id             |                                          |
| Current Level Id             | cell selection                           |
| Current LevelAsset           | object markers                           |
| Levels                       | selected-cell summary                    |
| Auto PIE Prepare             | objects at selected cell                 |
|                              |                                          |
| Load / Reload / Diagnostics  |                                          |
| New Level                    |                                          |
|                              |                                          |
| level list                   |                                          |
+------------------------------+------------------------------------------+
~~~

Initial splitter proportion:

~~~text
Level management : 36%
Overview Map      : 64%
~~~

Each side has its own scroll area, so a long dungeon level list does not force the Overview Map down the window.

## 5. Synchronization

Both halves continue to operate on the current `AGridLevelEditorActor`.

When a level is selected or created:

1. the authoritative actor is updated;
2. the existing dungeon-level application path runs;
3. the GEUI workspace rebuilds;
4. the Overview Map therefore follows the newly active `LevelAsset`;
5. editor viewports are redrawn.

The existing GEUI01 context observation remains unchanged.

## 6. Legacy Toolkit

The old `DUNGEON LEVELS` section is deliberately retained for this milestone, but it no longer contains a separate implementation.

It now instantiates:

~~~text
SGridEditorDungeonLevelsPanel
~~~

This is the compatibility fallback promised by GEUI01.

The old Toolkit section should not be removed until the complete workspace migration reaches GEUI06.

The existing inline `OVERVIEW MAP` also remains present for the same reason.

## 7. Files changed

New:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorDungeonLevelsPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorDungeonLevelsPanel.cpp
docs/Design/GEUI02_DUNGEON_LEVELS_WINDOW.md
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
3. Open `Window > Dungeon Levels`.
4. Confirm that the left side contains the dungeon metadata, actions and level list.
5. Confirm that the right side contains the Overview Map.
6. Drag the splitter between both panes.
7. Select every existing enabled level and confirm:
   - Current Level Id changes;
   - Current LevelAsset changes;
   - Overview Map changes to that level;
   - viewport preview follows the level.
8. Test `Load Default`.
9. Test `Reload Current`.
10. Test `New Level` up to opening the dialog; creating a disposable level is optional because it modifies assets.
11. Confirm the legacy inline `DUNGEON LEVELS` section still exposes the same actions.
12. Confirm the legacy inline `OVERVIEW MAP` still works.
13. Close and reopen the Nomad tab and confirm the current level is preserved.

## 9. Explicit non-goals

GEUI02 does not:

- change UGridDungeonAsset;
- change UGridLevelAsset;
- change level switching semantics;
- remove the legacy inline Dungeon Levels section;
- remove the legacy inline Overview Map section;
- add level deletion/reordering/duplication;
- redesign logical dungeon topology;
- create a subsystem;
- create a plugin;
- modify runtime behavior;
- modify .uasset or .umap files;
- open MON21.4.

## 10. Next step

After build and visual validation:

~~~text
GEUI03 — PlayTest & Validation Window
~~~

GEUI03 will perform the same reuse-first extraction for the PlayTest controls and compose them with the existing Validation panel.
