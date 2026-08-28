# GEUI04 — Tools & Palette Window

**Date:** 28 August 2026  
**Status:** implemented on master — UE5.5.4 build and visual validation pending

## 1. Objective

GEUI04 turns the existing dockable `Tools & Palette` tab into the primary authoring surface for tool selection and object discovery.

The milestone remains data-model neutral:

- no `UGridObjectPaletteAsset` schema change;
- no `UGridObjectArchetypeAsset` schema change;
- no new gameplay category enum;
- no Favorites/Recent persistence yet;
- no runtime behavior change.

## 2. Responsive tool strip

The existing tools are now displayed through a wrapping Slate layout rather than one fixed horizontal row.

Visible tools:

~~~text
Select
Paint Cell
Paint Wall
Paint Object
Erase
Link
~~~

`Link` already existed in `EGridEditorTool` and in the editor interaction code; GEUI04 simply exposes it alongside the other tools.

When the window narrows, tool tiles wrap instead of being clipped or forcing excessive horizontal width.

## 3. Palette search

When `Paint Object` is active, the Palette section now exposes a live search field.

Search matches against the existing data:

- effective display name;
- palette EntryId;
- effective category;
- archetype ArchetypeId;
- archetype DisplayName;
- archetype Description.

Typing updates only the palette result area, so the search field keeps focus instead of rebuilding the complete workspace on every keystroke.

Search text is presentation-only state stored in:

~~~text
FGridEditorToolPalettePanelState
~~~

It does not dirty a DataAsset.

## 4. Category filters

The Palette now exposes category buttons generated from the categories already present in `UGridObjectPaletteAsset`.

The first button is:

~~~text
All
~~~

followed by the effective categories sorted with the existing editor order:

~~~text
Doors
Mechanisms
Receptacles
Transitions
Items
Logic
Readable
Wall Decorations
Floor Decorations
Lights
Spawns
Uncategorized
...
~~~

Unknown/custom categories remain supported and sort after the known editor categories.

The selected category is also presentation-only state. GEUI04 does not create a second taxonomy.

## 5. Responsive palette results

The former fixed five-column `SUniformGridPanel` result layout is replaced by `SWrapBox` groups.

This means object tiles wrap according to the actual docked/floating window width.

Results remain grouped by category and show:

~~~text
Showing N of M palette entries
~~~

When no entry matches:

~~~text
No palette entries match the active filters.
~~~

## 6. State ownership

`FGridEditorToolPalettePanelState` now contains:

~~~text
CachedIconBrushes
SearchText
SelectedCategory
~~~

This state belongs to the editor UI only.

The workspace and the legacy inline Toolkit each retain their own presentation state while both interfaces coexist. Gameplay data remains authoritative in the existing actor/assets.

Favorites and Recently Used are deliberately deferred until the later palette UX milestone, where their per-user persistence strategy can be chosen explicitly.

## 7. Files changed

Modified:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorToolPalettePanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorToolPalettePanel.cpp
~~~

New:

~~~text
docs/Design/GEUI04_TOOLS_PALETTE_WINDOW.md
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
3. Open `Window > Tools & Palette`.
4. Resize the window narrow and wide; confirm tool tiles wrap cleanly.
5. Confirm the `Link` tool is present and activates the existing Link mode.
6. Select `Paint Object`.
7. Confirm the search field and category buttons appear.
8. Search by:
   - visible object name;
   - EntryId fragment;
   - category name;
   - archetype id/name.
9. Confirm the result count updates live.
10. Select several category filters and return to `All`.
11. Resize the window and confirm object tiles wrap rather than distort.
12. Select a filtered palette entry and confirm Paint Object still uses the same archetype.
13. Confirm the legacy inline Tools / Palette section still works.

## 9. Explicit non-goals

GEUI04 does not:

- change palette/archetype assets;
- add a new category enum;
- add Favorites;
- add Recently Used;
- persist UI preferences to gameplay assets;
- virtualize very large palettes;
- move icon assets;
- create a plugin;
- change runtime behavior;
- modify .uasset or .umap files;
- open MON21.4.

## 10. Next step

After build and visual validation:

~~~text
GEUI05 — Selected Object Workspace
~~~

GEUI05 will consolidate the object inspector and connectors into the dedicated selected-object workspace, initially with a low-risk Properties / Links organization before deeper General / Placement / Behavior / Diagnostics subdivision.
