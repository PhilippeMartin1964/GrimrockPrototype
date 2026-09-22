# GrimrockMenu — Current Technical Reference

Date : **22 septembre 2026**  
Statut : **CURRENT — UI-CLEAN01 + UI-ASSET-ORG01**

## Rôle actuel

WBP_GrimrockMenu / UGrimrockMenuWidget est uniquement un shell temporaire pour les pages qui n'ont pas encore été migrées vers des fenêtres autonomes.

Il ne porte plus l'Inventaire.

~~~text
WBP_GrimrockMenu
└── WidgetSwitcher_MainContent
    ├── Page_Skills
    ├── Page_Spellbook
    ├── Page_Journal
    ├── Page_Map
    ├── Page_Recipes
    └── Page_Codex
~~~

L'Inventaire est exclusivement :

~~~text
WBP_CharacterSheet
+
WBP_InventoryBag
~~~

Référence : docs/Design/UI_CLEAN01_REMOVE_MONOLITHIC_INVENTORY.md.

## Navigation

La navigation visible appartient à WBP_GridCombatHud :

~~~text
ESC / I / K / G / M / J / H
~~~

Le shell ne possède plus de barre d'onglets C++.

Les touches routent vers AGrimrockPartyPawn :

~~~text
I -> ToggleInventoryWidget() -> CharacterSheet + InventoryBag
K -> ToggleSkillsWidget()    -> Page_Skills
G -> ToggleCraftingWidget()  -> Page_Recipes
M -> ToggleMapWidget()       -> Page_Map
J -> ToggleJournalWidget()   -> Page_Journal
H -> ToggleHelpWidget()      -> Page_Codex
~~~

Spellbook reste une page du shell et conserve son accès fonctionnel existant.

## Contrat C++

UGrimrockMenuWidget expose uniquement :

~~~text
InitializeMenuWidget(...)
SetActiveTopTab(...)
RefreshSkills()
RefreshSpellbook()
GetSkillsWidget()
GetSpellbookWidget()
~~~

Bindings UMG :

~~~text
WidgetSwitcher_MainContent
Page_Skills
Page_Spellbook
Page_Journal
Page_Map
Page_Recipes
Page_Codex
~~~

Il n'existe plus côté C++ :

~~~text
Page_Inventory
Button_TabInventory
OpenInventoryWorkspace
RefreshInventory
GetInventoryWidget
~~~

## Inventaire

Le shell ne doit jamais être utilisé comme fallback Inventory.

Chemin unique :

~~~text
AGrimrockPartyPawn
├── CharacterSheetWidgetClass -> WBP_CharacterSheet
└── InventoryBagWidgetClass   -> WBP_InventoryBag
~~~

Si l'une de ces classes n'est pas configurée, l'ouverture de l'Inventaire échoue explicitement ; elle ne revient pas à un ancien écran.

## Pages restantes

Le shell subsiste uniquement parce que les pages Skills, Spellbook, Journal, Map, Recipes et Codex sont encore réellement utilisées.

À mesure que ces pages deviennent autonomes, leurs bindings doivent être retirés du shell. Lorsque la dernière page aura migré, WBP_GrimrockMenu / UGrimrockMenuWidget devra être supprimé.

## Règle d'architecture

Aucun nouveau code Inventory ne doit être ajouté à UGrimrockMenuWidget.

Aucun nouvel onglet visuel supérieur ne doit être ajouté au shell.

La barre basse persistante reste l'unique navigation globale visible.


## Audit assets UI — 22 septembre 2026

L'inventaire complet des assets sous `Content/GrimrockPrototype/Blueprints/UI`, leur statut et le plan de rangement/nettoyage sont documentés dans :

```text
docs/Design/UI_ASSET_AUDIT_2026_09_22.md
```

Le shell `WBP_GrimrockMenu` reste temporairement nécessaire. Il ne doit pas être supprimé tant que Skills, Spellbook, Journal, Map, Recipes et Codex n'ont pas tous quitté ce shell.
