# GrimrockMenu — Current Technical Reference

Date : **7 octobre 2026**  
Statut : **CURRENT — UI-RPG06.3B ; Skills autonome et clos**

`WBP_GrimrockMenu / UGrimrockMenuWidget` est un shell temporaire pour les seules pages qui ne sont pas encore autonomes.

## Shell restant

```text
WBP_GrimrockMenu
└── WidgetSwitcher_MainContent
    ├── Page_Spellbook
    ├── Page_Journal
    ├── Page_Recipes
    └── Page_Codex
```

## Surfaces autonomes

```text
I -> WBP_CharacterSheet + WBP_InventoryBag
K -> WBP_GridSkills
M -> WBP_GridMap
```

## Contrat C++

`UGrimrockMenuWidget` ne possède plus :

```text
Page_Inventory
Page_Map
Page_Skills
RefreshInventory
RefreshMap
RefreshSkills
GetInventoryWidget
GetMapWidget
GetSkillsWidget
```

Il conserve uniquement les responsabilités des pages legacy restantes.

Aucune surface déjà autonome ne doit réintégrer le shell. Lorsque Spellbook, Journal, Recipes et Codex auront été extraits, `WBP_GrimrockMenu` pourra être supprimé.
