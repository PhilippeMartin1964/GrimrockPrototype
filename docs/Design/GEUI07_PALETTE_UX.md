# GEUI07 — Palette UX

**Date:** 28 August 2026  
**Status:** implemented on master — UE5.5.4 build and visual validation pending

## 1. Objective

GEUI07 adds productivity features to the dedicated `Tools & Palette` workspace without changing gameplay assets or the palette data model.

The existing palette already provides:

- search;
- category tabs;
- compact 96x96 tiles;
- stable object selection.

GEUI07 adds:

~~~text
Favorites
Recent
~~~

as first-class palette tabs.

## 2. Tab organization

The palette tab strip is now ordered:

~~~text
All | Favorites | Recent | Doors | Mechanisms | Receptacles | ...
~~~

`Favorites` and `Recent` are presentation views, not new object categories.

The existing effective categories remain authoritative and continue to come from `UGridObjectPaletteAsset` / archetype data.

Search applies to every view:

- All;
- Favorites;
- Recent;
- a specific category.

## 3. Favorites

Every compact palette tile now exposes a small star in its upper-right corner:

~~~text
☆  not favorite
★  favorite
~~~

Clicking the star:

1. toggles the EntryId in the user's favorite set;
2. persists the updated set;
3. refreshes only the palette result area;
4. does not select or place the object.

The `Favorites` tab shows only starred entries.

If empty, it displays:

~~~text
No favorites yet. Use the star on a palette tile to add one.
~~~

Favorites are keyed by existing palette `EntryId`.

No favorite flag is written into a DataAsset.

## 4. Recently Used

A palette entry becomes recent when the user actually chooses its object tile.

GEUI07 keeps at most:

~~~text
16
~~~

recent EntryIds.

Rules:

- most recently chosen entry appears first;
- choosing an existing recent entry moves it back to first position;
- duplicates are removed;
- old entries fall off the end after 16.

The `Recent` tab preserves this recency order instead of regrouping entries by category.

If empty:

~~~text
No recently used entries yet.
~~~

## 5. Per-user persistence

Favorites and Recent are editor-user state.

They are persisted through Unreal's:

~~~text
GEditorPerProjectIni
~~~

under:

~~~text
[Grimrock.GridEditor.Palette]
Favorites=...
Recent=...
~~~

This is deliberately:

- per project;
- per editor user;
- outside `.uasset`;
- outside `.umap`;
- outside `UGridObjectPaletteAsset`;
- outside `UGridLevelAsset`.

Changing favorites therefore never dirties gameplay content.

## 6. Palette state

`FGridEditorToolPalettePanelState` now owns:

~~~text
CachedIconBrushes
SearchText
SelectedView
SelectedCategory
FavoriteEntryIds
RecentEntryIds
bUserStateLoaded
~~~

View modes:

~~~text
All
Favorites
Recent
Category
~~~

The category field is meaningful only for `Category`.

This avoids encoding pseudo-categories such as Favorites or Recent as fake `FName` categories.

## 7. Result layout

GEUI07 preserves the GEUI04.2 compact grid:

- strict 96x96 tiles;
- up to 8 columns per row;
- 2 px grid spacing;
- no `SWrapBox`;
- left/top aligned cells.

Presentation differs by view:

- `All`: entries are grouped under category headings;
- `Category`: one flat compact grid, no redundant category title;
- `Favorites`: one flat compact grid;
- `Recent`: one flat compact grid ordered by recency.

## 8. Files changed

Modified:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorToolPalettePanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorToolPalettePanel.cpp
~~~

New:

~~~text
docs/Design/GEUI07_PALETTE_UX.md
~~~

No runtime source, `.uasset` or `.umap` is modified.

## 9. Required UE5.5.4 validation

Build:

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Visual validation:

1. Open `Window > Tools & Palette`.
2. Select `Paint Object`.
3. Confirm the tab order begins:
   - All
   - Favorites
   - Recent
4. Confirm every object tile remains 96x96 and shows a small star at top-right.
5. Star several entries.
6. Open Favorites and confirm only those entries appear.
7. Unstar one entry while Favorites is active and confirm it disappears immediately.
8. Choose several object tiles in a known order.
9. Open Recent and confirm newest-first ordering.
10. Choose an older recent object again and confirm it moves to first position.
11. Search inside Favorites and Recent.
12. Close and reopen the Tools & Palette window; confirm Favorites and Recent survive.
13. Restart Unreal Editor and confirm they still survive.
14. Confirm selecting an entry still activates Paint Object and uses the same archetype.

## 10. Explicit non-goals

GEUI07 does not:

- change palette/archetype DataAssets;
- add favorite metadata to gameplay content;
- synchronize favorites between machines/users;
- add drag-and-drop reordering;
- add user-defined categories;
- add palette asset editing;
- change placement semantics;
- change runtime behavior;
- modify `.uasset` or `.umap`;
- open MON21.4.

## 11. Next step

After build and visual validation:

~~~text
GEUI08 — Validation UX
~~~

GEUI08 can focus on validation readability, filtering and navigation inside the dedicated PlayTest & Validation workspace.
