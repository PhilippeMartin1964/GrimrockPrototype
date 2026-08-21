# UI01.3.1 — Current WBP_GrimrockMenu Architecture

## Purpose

This document records the real current state of the player menu UI before any UI01.3 evolution.

The objective is to avoid redesigning an already existing system and to document the migration path toward a unified RPG menu architecture.

---

# Current architecture

## WBP_GrimrockMenu

`WBP_GrimrockMenu` already exists.

It is not a new shell to create. It is the current visual container for the player interface.

Current hierarchy:

```
WBP_GrimrockMenu
|
+-- WidgetSwitcher_MainContent
    |
    +-- Page_Inventory
    +-- Page_Skills
    +-- Page_Journal
    +-- Page_Map
    +-- Page_Recipes
    +-- Page_Codex
```

The presence of `WidgetSwitcher_MainContent` shows that multi-page navigation was already anticipated.

---

# Current responsibilities

## WBP_GrimrockMenu

Currently responsible mainly for:

- visual frame;
- layout;
- page containers.

It does not currently contain the inventory gameplay presentation logic.

---

## WBP_GridInventory

`WBP_GridInventory` currently contains the active inventory UI logic.

Responsibilities observed:

- party member widget registration;
- equipment slot registration;
- inventory refresh;
- slot rebuilding;
- context menu creation;
- item examination panels;
- equipment display updates.

This means that the current implementation is functional but contains responsibilities that will eventually belong to different UI layers.

---

# Current flow

Current conceptual flow:

```
Player opens menu
        |
        v
WBP_GrimrockMenu
        |
        v
Page_Inventory
        |
        v
WBP_GridInventory
        |
        +-- refresh data
        +-- display items
        +-- handle inventory interactions
```

---

# Architecture observation

The project already has part of the target architecture:

```
WBP_GrimrockMenu
        |
        +-- WidgetSwitcher
              |
              +-- RPG pages
```

The next evolution should therefore be incremental.

The goal is not to replace the menu, but to move navigation responsibilities upward while preserving existing inventory behaviour.

---

# Migration strategy

Future target:

```
WBP_GrimrockMenu
|
+-- Navigation
|
+-- WidgetSwitcher_MainContent
    |
    +-- WBP_GridInventory
    +-- WBP_GridSkills
    +-- WBP_GridSpellbook
    +-- WBP_GridJournal
    +-- WBP_GridMap
    +-- WBP_GridCodex
    +-- WBP_GridRecipes
```

Principles:

- no inventory rewrite;
- no gameplay logic in navigation widgets;
- preserve existing validated systems;
- add Spellbook as another page, not as a separate screen.

---

# UI01.3 next steps

1. Add navigation ownership to `WBP_GrimrockMenu`.
2. Keep `WBP_GridInventory` focused on inventory presentation.
3. Integrate `WBP_GridSpellbook` through the existing page system.
