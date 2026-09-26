# UI-GLOBALHUD01.3 — Remove Legacy Combat HUD Chrome

Date : **26 septembre 2026**  
Statut : **C++ prêt pour validation UE5.5.4 + nettoyage WBP requis**

## Décision

Le Combat HUD ne porte plus aucune surface globale persistante.

Autorités visuelles finales :

```text
WBP_GridPersistentHud
    -> navigation ESC / I / K / G / M / J / H
    -> barre générale d'actions dynamique

WBP_GridCombatHud
    -> initiative
    -> panneaux des combattants
    -> PAM
    -> Fin du tour
    -> ciblage
```

Le backend d'exécution `UGridCombatHudWidget::RequestHotbarSlot()` reste réutilisé par les widgets d'action du Persistent HUD. Il ne possède plus la présentation de la barre.

## C++ supprimé de UGridCombatHudWidget

UI-GLOBALHUD01.3 retire :

```text
ActionWidgetClass
HotbarSlotSpacing
HotbarActionWidgets
HotbarRow
Panel_Actions
Panel_GlobalNavigation
Button_NavEscape
Button_NavInventory
Button_NavSkills
Button_NavCrafting
Button_NavMap
Button_NavJournal
Button_NavHelp
EnsureActionWidgets()
RefreshActionWidgets()
BindGlobalNavigationButtons()
UnbindGlobalNavigationButtons()
HandleNavEscapeClicked()
HandleNavInventoryClicked()
HandleNavSkillsClicked()
HandleNavCraftingClicked()
HandleNavMapClicked()
HandleNavJournalClicked()
HandleNavHelpClicked()
```

Le Pawn ne possède plus `CombatHotbarConfigurationZOrder` ni le fallback qui remontait le Combat HUD lorsque le Persistent HUD n'était pas configuré.

## Persistent HUD

`UGridPersistentHudWidget::ActionWidgetClass` devient un contrat explicite. Il ne retombe plus sur une classe configurée dans le Combat HUD.

Dans `WBP_GridPersistentHud` :

```text
Action Widget Class = WBP_GridCombatHudAction
```

est obligatoire.

## Nettoyage manuel WBP_GridCombatHud

Supprimer complètement :

```text
VerticalBox_ActionArea
└── Overlay_ActionContext
    └── Panel_ActionPalette

HorizontalBox_BottomBar
├── Panel_GlobalNavigation
│   └── ESC / I / K / G / M / J / H
└── Panel_Actions
```

`Panel_ActionPalette` était déjà obsolète depuis HOTBAR01.2.

Conserver et restructurer le bloc Fin de tour :

```text
Panel_CombatHud
├── HorizontalBox_InitiativeArea
│   └── Panel_Initiative
├── Panel_PartyMembers
├── Panel_CombatBottomRight
│   ├── Text_MobilityActionPoints
│   ├── Button_EndTurn
│   └── Text_EndTurnDisabledReason
└── Panel_Targeting
    ├── Text_TargetingInstructions
    └── Text_TargetingCell
```

Le conteneur historique `VerticalBox_EndTurnArea` peut être renommé directement `Panel_CombatBottomRight` s'il contient déjà PAM, Fin du tour et le texte de refus.

`Panel_Targeting` reste indépendant et doit être conservé s'il est déjà présent. Il ne faut pas maintenir `Overlay_ActionContext` uniquement pour lui.

## Politique SaveGame prototype

Aucune compatibilité arrière de hotbar n'est maintenue.

Le schéma courant exige :

```text
CombatHotbarSlots.Num() >= 12
bindings structurellement valides
slot 1 = PrimaryAttack
```

Une ancienne sauvegarde :

```text
10 slots
hotbar absente
ancien binding supprimé
ancien schéma
```

est rejetée et peut être supprimée.

UI-GLOBALHUD01.3 retire :

```text
FGridCombatHotbarBinding::SlotCount
EnsureMinimumCombatHotbarCapacityPreservingBindings()
migration implicite d'une hotbar courte au RestorePartyInventoryState()
```

`EnsureCharacterCombatHotbarCapacity()` reste nécessaire, mais uniquement pour agrandir la hotbar **courante** en fonction de la largeur du viewport.

## Validation

Filtres prioritaires :

```text
Grimrock.UI.GlobalHud01
Grimrock.UI.Navigation01
Grimrock.Monsters.MON12.CombatHUD
Grimrock.Monsters.MON12.8.1
Grimrock.Monsters.MON12.8.2
Grimrock.TechnicalDebt.TD06_2.PartyInventoryHotbar
```

PIE :

1. la navigation et la barre d'actions proviennent uniquement de `WBP_GridPersistentHud` ;
2. le Combat HUD n'affiche aucun doublon en bas ;
3. PAM / Fin du tour restent visibles uniquement en combat et sont remontés de 56 px ;
4. initiative et panneaux combat sont inchangés ;
5. les raccourcis continuent d'exécuter via le backend Combat HUD ;
6. une ancienne sauvegarde à 10 slots n'est pas migrée.
