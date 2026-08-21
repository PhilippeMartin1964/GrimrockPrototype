# UI01.3 — Current WBP_GrimrockMenu Architecture

## Purpose

This document records the actual current state of the player menu UI after source-code and Blueprint audit.

The key point is that `WBP_GrimrockMenu` is already a functional multi-page shell whose navigation ownership is implemented in C++ by `UGrimrockMenuWidget`.

---

# Blueprint structure

`WBP_GrimrockMenu` is a Blueprint derived from the C++ class `UGrimrockMenuWidget`.

Its Blueprint Event Graph is intentionally empty: navigation logic is not implemented in Blueprint.

Current widget hierarchy:

```
WBP_GrimrockMenu
|
+-- Button_TabInventory
+-- Button_TabSkills
+-- Button_TabJournal
+-- Button_TabMap
+-- Button_TabRecipes
+-- Button_TabCodex
|
+-- WidgetSwitcher_MainContent
    |
    +-- Page_Inventory  -> WBP_GridInventory
    +-- Page_Skills     -> WBP_GridSkills
    +-- Page_Journal    -> WBP_GridJournal
    +-- Page_Map        -> WBP_GridMap
    +-- Page_Recipes    -> WBP_GridRecipes
    +-- Page_Codex      -> WBP_GridCodex
```

All of these widgets are bound to the native C++ parent through `BindWidget` properties.

---

# Navigation ownership

Navigation ownership already belongs to `UGrimrockMenuWidget`.

Relevant files:

```
Source/GrimrockPrototype/Public/UI/GrimrockMenuWidget.h
Source/GrimrockPrototype/Private/UI/GrimrockMenuWidget.cpp
Source/GrimrockPrototype/Public/UI/GridInventoryUiTypes.h
```

`NativeConstruct()` calls `BindTopTabButtons()` and activates the Inventory tab on first initialization.

`BindTopTabButtons()` binds each top-tab button to a native handler:

- `HandleInventoryTopTabClicked()`
- `HandleSkillsTopTabClicked()`
- `HandleJournalTopTabClicked()`
- `HandleMapTopTabClicked()`
- `HandleRecipesTopTabClicked()`
- `HandleCodexTopTabClicked()`

Each handler calls `SetActiveTopTab()`.

`SetActiveTopTab()`:

1. resolves the target page with `GetTopTabPage()`;
2. updates `CurrentTopTab`;
3. calls `WidgetSwitcher_MainContent->SetActiveWidget(TargetPage)`;
4. refreshes the selected-tab visual style through `UpdateTopTabButtonStyles()`.

Therefore no navigation migration from `WBP_GridInventory` to `WBP_GrimrockMenu` is required: that ownership is already correctly implemented in the native menu class.

---

# Current top-tab model

The current enum is:

```cpp
UENUM(BlueprintType)
enum class EInventoryTopTab : uint8
{
    Inventory = 0,
    Skills = 1,
    Journal = 2,
    Map = 3,
    Recipes = 4,
    Codex = 5
};
```

The name `EInventoryTopTab` reflects the historical origin of the menu but its actual responsibility is now broader than inventory.

No rename is required for UI01 because that would be an unnecessary refactor and could affect serialized Blueprint references.

---

# Inventory responsibilities

`WBP_GridInventory` remains responsible for inventory-specific presentation and interactions, including:

- party member widget registration;
- equipment slot registration;
- inventory refresh;
- inventory slot rebuilding;
- item context menus;
- item reading/examination panels;
- equipment display updates.

These responsibilities are separate from the top-level menu navigation owned by `UGrimrockMenuWidget`.

Current flow:

```
Player opens menu
        |
        v
WBP_GrimrockMenu / UGrimrockMenuWidget
        |
        +-- native top-tab navigation
        |
        v
WidgetSwitcher_MainContent
        |
        v
Page_Inventory / WBP_GridInventory
        |
        +-- inventory presentation and interactions
```

---

# UI01.3 conclusion

The navigation architecture required by UI01.3 already exists and is correctly owned by `UGrimrockMenuWidget`.

UI01.3 therefore does not require a new navigation system, a new `WidgetSwitcher`, an additional page enum, or Blueprint Graph navigation logic.

The next functional delta is Spellbook integration into the existing architecture.

Target extension:

```
WBP_GrimrockMenu
|
+-- Button_TabSpellbook
|
+-- WidgetSwitcher_MainContent
    |
    +-- Page_Inventory
    +-- Page_Skills
    +-- Page_Spellbook   <- new
    +-- Page_Journal
    +-- Page_Map
    +-- Page_Recipes
    +-- Page_Codex
```

The corresponding native work will extend the existing C++ navigation contract rather than introduce a parallel system.
