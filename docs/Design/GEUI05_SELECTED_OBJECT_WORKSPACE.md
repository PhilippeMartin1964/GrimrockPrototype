# GEUI05 — Selected Object Workspace

**Date:** 28 August 2026  
**Status:** implemented on master — UE5.5.4 build and visual validation pending

## 1. Objective

GEUI05 turns the dockable `Selected Object` window into a focused object-authoring workspace.

Before GEUI05, the detached window vertically stacked two large panels:

~~~text
PROPERTIES
CONNECTORS
~~~

This recreated part of the original monolithic Toolkit problem inside the new window.

GEUI05 replaces that stack with two mutually exclusive workspace tabs:

~~~text
Properties | Connectors
~~~

Only the active page is displayed.

## 2. Properties page

The `Properties` page embeds the existing:

~~~text
SGridEditorObjectInspectorPanel
~~~

No object editing logic is copied or changed.

The existing inspector remains authoritative for:

- selected-object summary;
- placement and orientation;
- initially enabled / active state;
- archetype-derived classification;
- contextual component fields;
- door, button, lever and pressure-plate behavior;
- triggers and receptacles;
- teleporters and transitions;
- readable content;
- item definitions;
- monster spawn properties;
- light properties;
- advanced/debug fields;
- focus / move / reset / apply actions already exposed by the inspector.

## 3. Connectors page

The `Connectors` page embeds the existing:

~~~text
SGridEditorLinksPanel
~~~

No Event -> Command or condition logic is duplicated.

The existing connector policy/service remains authoritative for:

- outgoing connectors;
- incoming connectors;
- connector creation;
- source event;
- target command;
- conditions;
- removal and clearing;
- linked-object selection;
- broken-link presentation.

Objects that do not support connectors continue to show the existing explanatory message.

## 4. Workspace tab behavior

The selected page is presentation-only state stored by:

~~~text
SGridEditorWorkspaceTab
~~~

with:

~~~text
EGridEditorSelectedObjectPage::Properties
EGridEditorSelectedObjectPage::Connectors
~~~

The page remains selected when:

- another object is selected;
- the selected cell changes;
- the level context refreshes;
- the workspace rebuilds because of existing GEUI01 context observation.

Closing and recreating the Nomad tab still resets to `Properties`, which is appropriate for the current non-persistent editor workspace state.

## 5. Visual organization

The top of the window now provides a compact tab strip using the same selected-tab language introduced for the GEUI04 palette:

- selected tab background;
- emphasized label;
- cyan underline;
- inactive dark tabs.

The content below occupies the remaining vertical area and owns its own scroll.

This removes the duplicate outer section headers and avoids forcing Properties and Connectors to compete for vertical space.

## 6. Legacy Toolkit

The legacy inline Toolkit remains unchanged for GEUI05:

~~~text
SELECTED OBJECT
CONNECTORS
~~~

These sections are retained as compatibility fallback until:

~~~text
GEUI06 — Slim Main Toolkit
~~~

The fallback and the dockable workspace use the same existing inspector/link widgets, so there is no second object-editing implementation.

## 7. Files changed

Modified:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorWorkspaceTab.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorWorkspaceTab.cpp
~~~

New:

~~~text
docs/Design/GEUI05_SELECTED_OBJECT_WORKSPACE.md
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
3. Open `Window > Selected Object`.
4. Confirm the top row contains exactly:
   - Properties
   - Connectors
5. Confirm only one page is visible at a time.
6. Select several object types and verify the Properties page follows selection.
7. Switch to Connectors and select several objects:
   - connector-capable objects show the existing connector UI;
   - unsupported objects show the existing no-connector message.
8. Create/cancel a connector using the existing form.
9. Switch back to Properties and verify object editing still works.
10. Resize the window narrow/tall/wide and verify the active page scrolls without duplicating the second page.
11. Confirm the legacy inline SELECTED OBJECT and CONNECTORS panels still work.

## 9. Explicit non-goals

GEUI05 does not:

- split the inspector internals into new data models;
- change object behavior fields;
- change connector semantics;
- change Event -> Command;
- add connector graph visualization;
- add persistent per-user tab preferences;
- remove legacy inline panels;
- create a plugin;
- modify runtime behavior;
- modify .uasset or .umap files;
- open MON21.4.

## 10. Next step

After build and visual validation:

~~~text
GEUI06 — Slim Main Toolkit
~~~

GEUI06 will finally remove the duplicated migrated sections from the main Grid Editor Toolkit and reduce it to the compact editor dashboard/header that opens or summarizes the dedicated workspace windows.
