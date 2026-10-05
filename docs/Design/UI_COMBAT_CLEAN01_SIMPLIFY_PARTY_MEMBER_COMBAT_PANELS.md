# UI-COMBAT-CLEAN01 — Simplify Party Member Combat Panels

> **ARCHITECTURE CONSERVÉE, AUTHORING UMG SUPERSEDED PAR UI-COMBAT-UNIFY01 (05.10.2026).**  
> La séparation HUD propriétaire / panneaux de présentation reste valide. Les noms, hiérarchies et réglages manuels actuels sont désormais centralisés dans `docs/Design/UI_COMBAT_WIDGETS_CURRENT.md`.

Date : **27 septembre 2026**

## Objectif

Supprimer la double autorité de projection entre `UGridCombatHudWidget` et
`UGridCombatActionPanelWidget` sans fusionner les deux widgets.

Le HUD de combat reste le propriétaire de la lecture runtime. Les quatre panneaux
de personnages restent des composants visuels répétés.

## Architecture retenue

```text
Inventory / TurnManager / StatusEffectLifecycle
                |
                v
        UGridCombatHudWidget
                |
                v
   FGridCombatHudPartyMemberView[4]
                |
                +--> UGridCombatActionPanelWidget #0
                +--> UGridCombatActionPanelWidget #1
                +--> UGridCombatActionPanelWidget #2
                +--> UGridCombatActionPanelWidget #3
```

`FGridCombatActionPanelView` est supprimé.

`UGridCombatActionPanelWidget` ne conserve plus :

```text
ConfiguredCharacterIndex
PartyPawn
InventoryComponent
TurnManagerComponent
InitializeCombatActionPanel(...)
RefreshFromSources()
```

Il reçoit uniquement le snapshot déjà construit via `SetView()`.

Les informations de statut et le feedback MON16.6 sont ajoutés à
`FGridCombatHudPartyMemberView` afin de conserver une projection unique.

## WBP_GridCombatHud — hiérarchie constatée

La hiérarchie Designer actuelle validée visuellement est :

```text
WBP_GridCombatHud
└── CanvasPanel_Root
    └── Panel_CombatHud
        ├── HorizontalBox_InitiativeArea
        │   └── Panel_Initiative
        ├── Panel_PartyMembers
        └── VerticalBox_EndTurnArea
            ├── Text_MobilityActionPoints
            ├── Button_EndTurn
            │   └── Text ("Fin du tour")
            └── Text_EndTurnDisabledReason
```

Correction manuelle requise : renommer uniquement
`VerticalBox_EndTurnArea` en `Panel_CombatBottomRight`.

Le type reste `VerticalBox`. Aucun reparenting n'est nécessaire.

`HorizontalBox_InitiativeArea`, `Panel_Initiative` et
`Panel_PartyMembers` sont conservés.

Aucun `Panel_Targeting` n'est ajouté dans ce ticket : les bindings de ciblage
sont optionnels dans le C++ et ce panneau n'existe pas dans le WBP actuel.
L'ajout éventuel d'une présentation textuelle du ciblage relève d'un ticket UI
séparé.

## Non-objectifs

- pas de fusion de `WBP_GridCombatActionPanel` avec `WBP_GridCombatHud` ;
- pas de renommage d'asset `.uasset` à l'aveugle ;
- pas de nouvelle couche ViewModel ;
- pas de modification du gameplay combat ;
- pas de modification de la barre d'actions persistante.

## Validation

Après le renommage manuel du conteneur dans `WBP_GridCombatHud` :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON12.CombatActionPanel"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON12.CombatHUD"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.MON16.6"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.GlobalHud01"
```

PIE : portraits, PV, mana, PA, état actif/désactivé, effets de statut,
initiative, PAM et Fin du tour doivent rester inchangés.
