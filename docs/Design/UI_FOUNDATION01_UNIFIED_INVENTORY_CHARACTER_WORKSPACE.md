> **ARCHIVE — UI-CLEAN01 :** ce document décrit un jalon historique désormais supprimé du code. Ne pas implémenter les APIs/workspace décrits ci-dessous. La structure actuelle est `WBP_CharacterSheet + WBP_InventoryBag`; voir `docs/Design/UI_CLEAN01_REMOVE_MONOLITHIC_INVENTORY.md`.

# UI-FOUNDATION01 — Unified Inventory / Character Workspace

Date : **20 septembre 2026**  
Statut : **ARCHIVE — code monolithique supprimé par UI-CLEAN01**

## Objectif

UI-FOUNDATION01 a posé le socle logique de la séparation Inventaire / Feuille de personnage sans recréer les systèmes déjà fonctionnels. Depuis UI-SPLIT01, les deux responsabilités ne sont plus deux panneaux d'un même WBP : elles deviennent deux fenêtres viewport réellement indépendantes.

La décision structurante est :

```text
un personnage sélectionné
    -> une feuille personnage
    -> un équipement / paper doll
    -> un inventaire
```

`UGridPartyInventoryComponent::SelectedCharacterIndex` reste l'unique autorité de sélection.

## Réutilisation obligatoire

Le ticket conserve :

- `WBP_GrimrockMenu / UGrimrockMenuWidget` comme shell joueur ;
- `WBP_GridInventory / UGridInventoryWidget` comme workspace Inventaire + Personnage ;
- les six portraits / `UGridPartyMemberWidget` existants ;
- le paper doll et les `UGridInventorySlotWidget` existants ;
- l'inventaire autoritaire `UGridPartyInventoryComponent` ;
- le Cursor item, drag/drop, menus contextuels, lecture et tooltips existants ;
- la hotbar MON12 comme unique hotbar ;
- la surface logique 1920x1080 de `UGrimrockDesignSurfaceWidget`.

Aucun second inventaire, second `SelectedCharacterIndex`, second stockage hotbar ou Graph Blueprint métier n'est introduit.

## Workspace cible

```text
WBP_GrimrockMenu
└── Page_Inventory / WBP_GridInventory
    ├── Panel_CharacterSheet      gauche
    │   ├── portraits Party
    │   ├── attributs / stats
    │   └── paper doll / équipement
    ├── zone centrale transparente
    │   └── vue subjective du donjon reste visible
    └── Panel_InventoryBag        droite
        ├── filtres / tri futurs
        ├── UNE grille du personnage sélectionné
        └── charge / poids
```

La barre globale `ESC / I / K / G / M / J / H` est traitée dans **UI-NAV01**. Elle est désormais explicitement **toujours visible**, collée au bas de l'écran et extérieure au workspace Inventory. La hotbar MON12 existante reste à sa droite dans la même bande visuelle ; UI-HOTBAR01 traitera seulement ses évolutions fonctionnelles/visuelles futures.

## Contrat C++ ajouté

`UGridInventoryWidget` expose maintenant :

```text
Panel_CharacterSheet       BindWidgetOptional
Panel_InventoryBag         BindWidgetOptional
Button_CloseCharacterSheet BindWidgetOptional
Button_CloseInventoryBag   BindWidgetOptional

ResetInventoryWorkspace()
SetCharacterSheetPanelVisible(...)
SetInventoryBagPanelVisible(...)
IsCharacterSheetPanelVisible()
IsInventoryBagPanelVisible()
```

Les deux panneaux sont indépendamment fermables. Leur fermeture ne détruit ni le widget, ni l'inventaire, ni le personnage sélectionné.

`UGrimrockMenuWidget::OpenInventoryWorkspace()` :

1. active la page `Inventory` existante ;
2. restaure les deux panneaux ;
3. rafraîchit l'inventaire.

Le chemin historique `I -> ToggleInventoryWidget()` est conservé pour éviter un refactor transversal inutile, mais une réouverture par `I` revient désormais explicitement au workspace Inventaire et restaure les deux panneaux.

## Contrat UMG à appliquer

Aucun `.uasset` n'est modifié à l'aveugle dans ce ticket.

Dans `WBP_GridInventory`, la prochaine passe UMG doit créer ou renommer les deux wrappers exacts :

```text
Panel_CharacterSheet
Panel_InventoryBag
```

et, lorsque les croix sont posées :

```text
Button_CloseCharacterSheet
Button_CloseInventoryBag
```

Ils sont optionnels côté C++ : l'asset actuel continue donc à fonctionner avant sa refonte visuelle.

Le contenu existant doit être **déplacé**, pas recréé :

- panneau sélection / stats / paper doll -> `Panel_CharacterSheet` ;
- grille de slots / poids -> `Panel_InventoryBag`.

La zone centrale doit rester transparente et ne pas poser de widget plein écran Hit-Testable au-dessus de la vue 3D.

## Hors périmètre

UI-FOUNDATION01 ne réalise pas encore :

- les bindings `K / G / M / J / H / ESC` ;
- le nouveau style visuel final ;
- les filtres / tri ;
- le transfert par drop sur portraits ;
- les statistiques dérivées supplémentaires ;
- la comparaison d'équipement ;
- le remaniement de la hotbar.

Ces points appartiennent aux tickets suivants de la roadmap UI.

## Automation ajoutée

Filtre :

```text
Grimrock.UI.Foundation01
```

Test :

```text
Grimrock.UI.Foundation01.WorkspaceState
```

Il caractérise :

- les deux panneaux visibles par défaut ;
- fermeture indépendante ;
- absence de couplage entre les deux états ;
- restauration des deux panneaux par `ResetInventoryWorkspace()`.

## Validation locale demandée

Après récupération du commit :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Foundation01"
```

Puis PIE ciblé après la passe UMG :

1. `I` ouvre l'inventaire ;
2. personnage / équipement / slots existants fonctionnent encore ;
3. chaque croix ferme uniquement son panneau ;
4. fermeture puis réouverture par `I` restaure les deux panneaux ;
5. aucune perte de Cursor item ;
6. aucun `BindWidget` critique ;
7. la vue 3D reste visible au centre.

Ne pas déclarer UI-FOUNDATION01 validé tant que cette sortie locale et le PIE n'ont pas été fournis.


## Clarification UI-INV01

Le panneau `Panel_InventoryBag` est désormais explicitement un **single-bag view** : une seule grille est affichée et son contenu suit `SelectedCharacterIndex`. Les inventaires des autres membres ne sont jamais empilés simultanément.


## Clôture UI-CLEAN01

Les APIs de workspace interne décrites dans ce document ainsi que le test `Grimrock.UI.Foundation01.WorkspaceState` ont été supprimés par UI-CLEAN01. Ce document est conservé uniquement comme historique de décision.
