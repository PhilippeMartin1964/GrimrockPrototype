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


## 03.4E — Persistent HUD root hit-test shield

Diagnostic runtime après extraction standalone :

```text
GridSkills Tabs Bound ... ActiveIndex=1
aucun SkillsTab HOVERED
aucun TalentsTab HOVERED
```

Le binding du WBP Skills était correct, mais la souris n'atteignait pas les boutons.

Cause structurelle traitée :

```text
WBP_GridPersistentHud : Z = 200
WBP_GridSkills        : Z = 100
```

Le `UUserWidget` Persistent HUD était déjà `SelfHitTestInvisible`, mais son root Designer
(`CanvasPanel_Root` recommandé historiquement) pouvait rester `Visible` et couvrir tout le viewport.

`UGridPersistentHudWidget::NativeConstruct()` force désormais :

```cpp
WidgetTree->RootWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
```

Conséquence :

- le root plein écran ne bloque plus les surfaces majeures situées dessous ;
- les enfants restent hit-testables ;
- les boutons ESC/I/K/G/M/J/H restent interactifs ;
- les slots d'action restent interactifs ;
- les boutons internes de `WBP_GridSkills` peuvent recevoir Hover/Click.

Validation PIE attendue avec l'instrumentation temporaire :

```text
GridSkills SkillsTab HOVERED
GridSkills SkillsTab CLICKED
GridSkills ShowSkillsTab Before=1 After=0

GridSkills TalentsTab HOVERED
GridSkills TalentsTab CLICKED
GridSkills ShowTalentsTab Before=0 After=1
```


## 03.4F — alignement hit-test avec l'Inventaire

Le diagnostic final compare directement les surfaces déjà validées.

Inventaire :

```cpp
CharacterSheetWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
InventoryBagWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
```

Skills utilisait encore :

```cpp
SkillsWidgetInstance->SetVisibility(ESlateVisibility::Visible);
```

La fenêtre Skills ne traite aucun événement souris au niveau de son root : seuls ses enfants
(`Button_SkillsTab`, `Button_TalentsTab`, puis les nœuds) doivent être interactifs.

Le contrat est donc désormais identique à l'Inventaire :

```cpp
SkillsWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
```

La Carte reste volontairement `Visible`, car sa surface traite elle-même des événements souris
(pan/zoom/interactions) au niveau widget.

Les instrumentations temporaires Hover/Click de UI-RPG03.4D sont retirées et le correctif
spéculatif du root Persistent HUD de UI-RPG03.4E est annulé. Aucun code supplémentaire
d'interaction n'est conservé.
