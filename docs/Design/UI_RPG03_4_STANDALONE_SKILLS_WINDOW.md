# UI-RPG03.4 — Standalone Skills / Talents Window

Date : **7 octobre 2026**  
État : **03.4A SOURCE PRÊTE — validation locale puis migration UMG manuelle**

## Décision

`WBP_GridSkills` quitte `WBP_GrimrockMenu`.

Architecture cible :

```text
I -> WBP_CharacterSheet + WBP_InventoryBag
K -> WBP_GridSkills
M -> WBP_GridMap
```

## Avant

```text
K
 -> ToggleSkillsWidget()
 -> ToggleMenuPage(Skills)
 -> WBP_GrimrockMenu
 -> WidgetSwitcher_MainContent
 -> Page_Skills
 -> WBP_GridSkills
```

## Après

```text
K
 -> ToggleSkillsWidget()
 -> ShowSkillsWidget()
 -> SkillsWidgetClass
 -> WBP_GridSkills
 -> AddToViewport(100)
```

Le focus GameAndUI cible directement `SkillsWidgetInstance`.

## Nouveau contrat Pawn

```text
SkillsWidgetClass
SkillsWidgetInstance
ToggleSkillsWidget()
ShowSkillsWidget()
HideSkillsWidget()
IsSkillsWidgetVisible()
```

## UGrimrockMenuWidget

Supprimé côté C++ :

```text
Page_Skills
RefreshSkills()
GetSkillsWidget()
```

Le shell ne conserve plus que :

```text
Page_Spellbook
Page_Journal
Page_Recipes
Page_Codex
```

## Persistent HUD

La sélection K utilise désormais :

```text
PartyPawn->IsSkillsWidgetVisible()
```

et non plus `MenuWidgetInstance->CurrentTopTab == Skills`.

## Migration UE manuelle après validation source

### BP_GrimrockPartyPawn

Class Defaults :

```text
RPG | Skills | UI
Skills Widget Class = WBP_GridSkills
```

### WBP_GrimrockMenu

Dans `WidgetSwitcher_MainContent`, supprimer :

```text
Page_Skills / WBP_GridSkills
```

Compiler et sauvegarder.

## Validation

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.RPG03.StandaloneSkills"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.Navigation01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.GlobalHud01.NavigationSelection"
```

PIE :

```text
K ouvre directement WBP_GridSkills
WBP_GrimrockMenu reste caché
COMPÉTENCES Hovered/Pressed + index 0
TALENTS Hovered/Pressed + index 1
K ferme Skills
ESC ferme Skills
I/M/G/J/H ferment Skills avant d'ouvrir leur surface
```
