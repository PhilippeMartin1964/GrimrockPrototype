# GEUI08 — Validation UX

**Date:** 28 August 2026  
**Status:** implemented on master — UE5.5.4 build and visual validation pending

## 1. Objective

GEUI08 improves the dedicated `PlayTest & Validation` workspace without changing any validation rule.

The validation source remains:

~~~text
GridEditorLuaService::ValidateCurrentLevelWithLua
~~~

and the existing `FGridLevelValidationMessage` model remains authoritative.

GEUI08 is presentation, filtering and navigation only.

## 2. Validation status banner

After a validation run, the panel now exposes a prominent status banner:

~~~text
INVALID
VALID WITH WARNINGS
VALID
~~~

Rules:

- one or more errors -> `INVALID`;
- zero errors and one or more warnings -> `VALID WITH WARNINGS`;
- zero errors and zero warnings -> `VALID`.

The banner also keeps the complete numerical summary:

~~~text
Errors: N | Warnings: N | Infos: N
~~~

## 3. Search

A validation search box is added directly below the status banner.

Search is case-insensitive and matches:

- message text;
- validation category;
- optional object GUID;
- source object GUID;
- target object GUID;
- cell coordinates;
- edge name/display name.

Examples:

~~~text
door
lua
12,7
north
A1B2C3D4
~~~

Search is combined with the severity filters.

## 4. Severity filters

The existing independent filters remain available, but now include their total counts:

~~~text
Errors (3)
Warnings (5)
Infos (12)
~~~

This preserves the useful ability to display combinations such as:

~~~text
Errors + Warnings
Warnings only
Infos only
~~~

rather than replacing them with mutually exclusive tabs.

A new:

~~~text
Clear Filters
~~~

action restores:

- Errors visible;
- Warnings visible;
- Infos visible;
- empty search.

## 5. Result count

The panel now reports:

~~~text
Showing X of Y validation messages
~~~

This makes the effect of search/severity filters explicit.

If no result remains:

~~~text
No messages match the active filters or search.
~~~

## 6. Existing navigation retained

GEUI08 deliberately reuses all existing message actions:

- Select Object;
- Focus Object;
- Select Source;
- Focus Source;
- Select Target;
- Focus Target;
- Select Cell.

No second object-selection or viewport-focus implementation is introduced.

## 7. Message ordering

Messages keep the established stable ordering:

1. Error;
2. Warning;
3. Info;

then category within the same severity.

Search/filtering does not alter this deterministic ordering.

## 8. Copy Summary

`Copy Summary` remains unchanged and copies the complete validation run, not only the currently filtered subset.

This is intentional: clipboard diagnostics remain a complete technical report.

## 9. State lifetime

Search and severity filters live in:

~~~text
FGridEditorValidationPanelState
~~~

which is already owned by the dockable workspace host.

They therefore survive workspace rebuilds caused by editor selection/context changes during the current window lifetime.

No gameplay asset is dirtied.

## 10. Files changed

Modified:

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorValidationPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorValidationPanel.cpp
~~~

New:

~~~text
docs/Design/GEUI08_VALIDATION_UX.md
~~~

No runtime source, `.uasset` or `.umap` is modified.

## 11. Required UE5.5.4 validation

Build:

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Visual validation:

1. Open `PlayTest & Validation`.
2. Run `Refresh Validation`.
3. Confirm the status banner is visible and consistent with the counts.
4. Confirm the severity controls show counts.
5. Disable/enable Errors, Warnings and Infos independently.
6. Search a known category/message term.
7. Search a known cell as `X,Y`.
8. Confirm `Showing X of Y` updates.
9. Use `Clear Filters`.
10. Exercise Select/Focus on object-related messages.
11. Confirm `Copy Summary` still copies the full run.
12. Resize the window and confirm the validation side remains readable.

## 12. Explicit non-goals

GEUI08 does not:

- change validation rules;
- automatically fix validation problems;
- change Lua compilation/validation semantics;
- add a second validation data model;
- add runtime behavior;
- modify `.uasset` or `.umap`;
- open MON21.4.

## 13. Next step

After visual/build validation:

~~~text
GEUI09 — Refresh / State cleanup
~~~

GEUI09 will consolidate the temporary context-observation/rebuild mechanism introduced during the workspace migration and reduce unnecessary full-widget rebuilds.
