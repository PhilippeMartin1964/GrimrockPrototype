# UI-COMBAT-UNIFY01 — Combat Widgets Current Contract

Date : **5 octobre 2026**  
Statut : **C++ prêt ; migration UMG manuelle et validation UE 5.5.4 requises**

Ce document est la référence actuelle pour :

```text
WBP_GridCombatActionPanel
WBP_GridCombatHud
WBP_GridCombatHudInitiativeSlot
```

Les procédures UMG détaillées de MON12.1, MON12.7, UI-GLOBALHUD01.3 et
UI-COMBAT-CLEAN01 sont historiques lorsqu'elles contredisent ce document.

## Objectif

Uniformiser les contrats des deux widgets sans les fusionner :

```text
UGridCombatHudWidget
    -> lit les autorités runtime
    -> construit FGridCombatHudPartyMemberView[4]
    -> crée les quatre UGridCombatActionPanelWidget

UGridCombatActionPanelWidget
    -> reçoit SetView(...)
    -> rend un seul personnage
    -> ne lit aucune autorité runtime directement
```

Règle générale :

- C++ : état, projection, visibilité, couleurs d'état et orchestration ;
- WBP / UMG : hiérarchie, dimensions, typographie, images, padding et chrome ;
- aucun Event Graph n'est nécessaire dans ces deux WBP ;
- aucun widget de présentation n'est créé en fallback natif lorsqu'il appartient au layout authored.

## Convention de configuration commune

Les paramètres destinés à l'authoring sont des `EditDefaultsOnly` regroupés sous :

```text
Combat | UI | Appearance
Combat | UI | Classes
Combat | UI | Initiative
Combat | UI | Layout
```

Dans les deux Widget Blueprints, les régler de la même manière :

```text
Graph
-> My Blueprint
-> View Options
-> Show Inherited Variables
-> sélectionner la propriété
-> Details
-> Default Value
```

Il ne faut pas créer une variable Blueprint portant le même nom pour contourner
une propriété C++ héritée.

## WBP_GridCombatActionPanel

Parent : `GridCombatActionPanelWidget`.

Le widget est un composant de contenu. Il n'est plus responsable de son ancrage
dans le viewport.

```text
WBP_GridCombatActionPanel
└── SizeBox_ActionPanel
    └── Border_Background
        └── Overlay_Content
            ├── HorizontalBox_Content
            │   ├── SizeBox_Portrait
            │   │   └── Image_Portrait
            │   └── VerticalBox_Details
            │       ├── Text_Name
            │       ├── HorizontalBox_Health
            │       │   ├── Image_Health
            │       │   ├── Spacer_Health
            │       │   ├── Text_PV_Label
            │       │   └── Text_Health
            │       ├── HorizontalBox_Mana
            │       │   ├── Image_Mana
            │       │   ├── Spacer_Mana
            │       │   ├── Text_Mana_Label
            │       │   └── Text_Mana
            │       ├── HorizontalBox_ActionPoints
            │       │   ├── Image_ActionPoints
            │       │   ├── Spacer_ActionPoints
            │       │   └── Text_ActionPoints
            │       ├── Text_StatusEffects
            │       ├── Text_StatusFeedback
            │       └── Border_ActionState
            │           └── Text_ActionState
            └── Panel_DisabledOverlay
```

Bindings C++ à conserver exactement et à cocher **Is Variable** :

```text
Image_Portrait
Text_Name
Text_Health
Text_Mana
Text_ActionPoints
Text_StatusEffects
Text_StatusFeedback
Border_ActionState
Text_ActionState
Panel_DisabledOverlay
```

Les noms décoratifs `Image_Health`, `Spacer_Health`, etc. ne sont pas des bindings C++.

`Text_StatusEffects` et `Text_StatusFeedback` restent `BindWidgetOptional`
pendant la migration, mais font partie du contrat visuel courant. Le C++ ne les
crée plus automatiquement.

`Panel_DisabledOverlay` doit rester **Not Hit-Testable (Self & All Children)**.

Le `Canvas_Root` historique doit être supprimé. La taille du panneau se règle
dans `SizeBox_ActionPanel`; son emplacement est décidé uniquement par le HUD parent.

### Paramètres hérités

```text
Combat | UI | Appearance
    Disabled Opacity        = 0.45
    Ready Color
    Waiting Color
    Already Acted Color
    Incapacitated Color
    Defeated Color
```

## WBP_GridCombatHud

Parent : `GridCombatHudWidget`.

```text
WBP_GridCombatHud
└── Canvas_Root
    └── Panel_CombatHud
        ├── HorizontalBox_InitiativeArea
        │   └── Panel_Initiative
        ├── Panel_PartyMembers
        └── Panel_CombatBottomRight
            ├── Text_MobilityActionPoints
            ├── Button_EndTurn
            │   └── Text ("Fin du tour")
            └── Text_EndTurnDisabledReason
```

`Panel_PartyMembers` et `Panel_Initiative` doivent être vides dans le Designer.
Le C++ crée leurs enfants.

Ne pas réintroduire :

```text
Panel_Actions
Panel_ActionPalette
Panel_GlobalNavigation
Button_Nav*
Panel_Targeting
Text_TargetingInstructions
Text_TargetingCell
```

La navigation et la barre d'actions appartiennent à `WBP_GridPersistentHud`.
Le ciblage Cell/Area reste un backend C++ sans panneau texte dédié.

### Paramètres hérités

```text
Combat | UI | Classes
    Party Member Panel Widget Class = WBP_GridCombatActionPanel
    Initiative Slot Widget Class    = WBP_GridCombatHudInitiativeSlot

Combat | UI | Initiative
    Visible Initiative Slot Count   = 8

Combat | UI | Layout
    Party Member Panel Spacing      = 0
    Persistent Hud Bottom Clearance = 56
```

`Party Member Panel Spacing` ajoute un padding droit entre les panneaux générés
lorsque `Panel_PartyMembers` est un `HorizontalBox`. Le dernier panneau ne reçoit
pas de padding droit.

## Migration manuelle UE 5.5.4

### A. WBP_GridCombatActionPanel

1. Vérifier `Parent Class = GridCombatActionPanelWidget`.
2. Reparent `SizeBox_ActionPanel` comme racine puis supprimer `Canvas_Root`.
3. Renommer les trois lignes en `HorizontalBox_Health`, `HorizontalBox_Mana`
   et `HorizontalBox_ActionPoints`.
4. Ajouter `Text_StatusEffects` et `Text_StatusFeedback` directement dans
   `VerticalBox_Details`, avant `Border_ActionState`.
5. Cocher `Is Variable` pour tous les bindings C++ listés plus haut.
6. Vérifier `Panel_DisabledOverlay` couvrant et non interceptant.
7. Régler les valeurs héritées `Combat | UI | Appearance`.
8. Compiler et sauvegarder.

### B. WBP_GridCombatHud

1. Vérifier `Parent Class = GridCombatHudWidget`.
2. Conserver `Canvas_Root` plein écran.
3. Vérifier le nom `HorizontalBox_InitiativeArea`.
4. Laisser `Panel_Initiative` vide.
5. Laisser `Panel_PartyMembers` vide et de type `HorizontalBox`.
6. Vérifier `Panel_CombatBottomRight`.
7. Supprimer tout reste de barre d'actions, navigation globale ou panneau texte de ciblage.
8. Régler les variables héritées :
   - `Party Member Panel Widget Class = WBP_GridCombatActionPanel` ;
   - `Initiative Slot Widget Class = WBP_GridCombatHudInitiativeSlot` ;
   - `Visible Initiative Slot Count = 8` ;
   - `Party Member Panel Spacing` selon le rendu souhaité ;
   - `Persistent Hud Bottom Clearance = 56`.
9. Compiler et sauvegarder.

## Validation attendue

Ne pas considérer le ticket comme validé avant retour des logs UE locaux.

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.CombatUnify01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON12.CombatActionPanel"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON12.CombatHUD"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.MON16.6"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.GlobalHud01"
```

PIE :

- quatre panneaux pour un groupe complet ;
- portrait, nom, PV, mana, PA et état corrects ;
- effets de statut et dernier feedback visibles via les widgets authored ;
- aucun ancrage viewport dans le panneau enfant ;
- espacement piloté par le HUD ;
- initiative, PAM et Fin du tour inchangés ;
- aucune barre/navigation dupliquée dans le Combat HUD ;
- ciblage souris toujours fonctionnel via le backend existant.
