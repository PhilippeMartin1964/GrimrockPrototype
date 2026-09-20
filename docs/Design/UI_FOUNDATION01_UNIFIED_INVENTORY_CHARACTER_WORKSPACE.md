# UI-FOUNDATION01 — Unified Inventory / Character Workspace

Date : **20 septembre 2026**  
Statut : **IMPLEMENTATION C++ + CONTRAT UMG ; validation locale UE5.5.4 à fournir**

## Objectif

UI-FOUNDATION01 pose le socle de la refonte validée de l'interface Inventaire / Feuille de personnage sans recréer les systèmes déjà fonctionnels.

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
        ├── grille du personnage sélectionné
        └── charge / poids
```

La barre globale `ESC / I / K / G / M / J / H` et la hotbar sont traitées séparément dans **UI-NAV01** et **UI-HOTBAR01**.

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
