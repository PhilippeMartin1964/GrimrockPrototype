# Inventory Blueprint Construction Guide

Ce guide décrit la construction attendue des Blueprints d'inventaire. Il complète `ITEM_CONTEXT_ACTION_SYSTEM.md`, `INVENTORY_INTERACTION_ROUTING.md` et `UI_INV2_CHARACTER_EQUIPMENT_PANEL.md`.

## Principe

Les Blueprints affichent et relaient les intentions. Le gameplay reste en C++.

Un Blueprint d'inventaire peut :

- afficher des slots, tooltips, menus et panneaux ;
- stocker les paramètres UI nécessaires, comme `SlotType`, `SlotIndex`, `EquipmentSlot` ou `ActionIndex` ;
- appeler une fonction C++ explicite ;
- fermer ou repositionner des widgets UI.

Un Blueprint d'inventaire ne doit pas :

- décider qu'un item est compatible avec une main, un slot d'equipement ou une cible ;
- déplacer directement un item entre inventaire, équipement, Cursor ou réceptacle ;
- appeler une porte, une serrure ou un lien gameplay ;
- utiliser `ExecuteInventoryContextAction(ActionType, ...)` depuis le menu visible ;
- ajouter de logique DPI, viewport ou scaling local.

## WBP_GridInventory

Parent class attendu : `UGridInventoryWidget`.

Rôle :

- racine de la page inventaire ;
- affichage de la Party ;
- affichage du personnage sélectionné ;
- affichage du paper doll d'equipement ;
- affichage des inventaires et slots ;
- réception de `OnContextActionsRequested` ;
- création et fermeture du menu contextuel ;
- implémentation des événements d'affichage `PresentItemExamination`, `PresentItemReading` et `OnItemActionMenuCloseRequested`.

`WBP_GridInventory` est le contenu de `Page_Inventory` dans `WBP_GrimrockMenu`. Il ne doit pas ajouter de `ScaleBox` local, de `SizeBox_DesignSurface` local, de calcul DPI ou de logique viewport. La surface 1920x1080 est portée par `UGrimrockDesignSurfaceWidget` via `WBP_GrimrockMenu`.

## Structure générale recommandée

```text
WBP_GridInventory
-> Border_InventoryPanel
   -> HorizontalBox_InventoryRoot
      -> SizeBox_PartyColumn
      -> SizeBox_SelectedCharacterPanel
      -> SizeBox_CharacterStatsPanel
      -> SizeBox_InventoryColumn ou panneau inventaire existant
```

La colonne Party et la zone inventaire peuvent évoluer, mais le panneau central personnage doit suivre la structure paper doll ci-dessous.

## Structure canonique du panneau paper doll

```text
SizeBox_SelectedCharacterPanel
-> Border_SelectedCharacterPanel
   -> Overlay_SelectedCharacterRoot
      -> Text_SelectedCharacterTitle
      -> Overlay_PaperDollArea
         -> VerticalBox_LeftEquipmentColumn
            -> SlotWidget_Head
            -> SlotWidget_Face
            -> SlotWidget_Amulet
            -> SlotWidget_Shoulders
            -> SlotWidget_Shirt
            -> SlotWidget_Chest
            -> SlotWidget_Cloak
            -> SlotWidget_Bracers
         -> SizeBox_CharacterFigure
            -> Overlay_CharacterFigure
               -> Image_CharacterFullBody
               -> Image_CharacterClassIcon ou decoration de classe optionnelle
         -> VerticalBox_RightEquipmentColumn
            -> SlotWidget_Gloves
            -> SlotWidget_Belt
            -> SlotWidget_Legs
            -> SlotWidget_Feet
            -> SlotWidget_Ring1
            -> SlotWidget_Ring2
            -> SlotWidget_Earring1
            -> SlotWidget_Earring2
         -> HorizontalBox_BottomHandsRow
            -> SlotWidget_MainHand
            -> Spacer_BottomHands
            -> SlotWidget_OffHand
```

Cette structure remplace la vision `UniformGrid_EquipmentSlots` comme cible finale. Une grille uniforme peut rester temporairement dans un asset de transition, mais elle n'est plus la référence du design final.

Le personnage doit être visible de pied en cap. L'image centrale doit être un visuel plein corps, même si elle est statique dans une première version.

## Liste officielle des slots paper doll

### Colonne gauche

| Widget | Libellé UI | Slot logique |
|---|---|---|
| `SlotWidget_Head` | Tête | `Head` |
| `SlotWidget_Face` | Visage | `Face` |
| `SlotWidget_Amulet` | Amulette | `Amulet` |
| `SlotWidget_Shoulders` | Épaules | `Shoulders` |
| `SlotWidget_Shirt` | Chemise | `Shirt` |
| `SlotWidget_Chest` | Torse | `Chest` |
| `SlotWidget_Cloak` | Cape | `Cloak` |
| `SlotWidget_Bracers` | Brassards | `Bracers` |

### Colonne droite

| Widget | Libellé UI | Slot logique |
|---|---|---|
| `SlotWidget_Gloves` | Gants | `Gloves` |
| `SlotWidget_Belt` | Ceinture | `Belt` |
| `SlotWidget_Legs` | Jambes | `Legs` |
| `SlotWidget_Feet` | Bottes | `Feet` |
| `SlotWidget_Ring1` | Anneau I | `Ring1` |
| `SlotWidget_Ring2` | Anneau II | `Ring2` |
| `SlotWidget_Earring1` | Bijou d'oreille I | `Earring1` |
| `SlotWidget_Earring2` | Bijou d'oreille II | `Earring2` |

### Bas

| Widget | Libellé UI | Slot logique |
|---|---|---|
| `SlotWidget_MainHand` | Main principale | `MainHand` |
| `SlotWidget_OffHand` | Main secondaire | `OffHand` |

## Etat fonctionnel actuel

Les slots suivants peuvent être enregistrés dès maintenant si le C++ les expose :

```text
SlotWidget_Head       -> Head
SlotWidget_Amulet     -> Amulet
SlotWidget_Shoulders  -> Shoulders
SlotWidget_Chest      -> Chest
SlotWidget_Cloak      -> Cloak
SlotWidget_Gloves     -> Gloves
SlotWidget_Belt       -> Belt
SlotWidget_Legs       -> Legs
SlotWidget_Feet       -> Feet
SlotWidget_Ring1      -> Ring1
SlotWidget_Ring2      -> Ring2
SlotWidget_MainHand   -> MainHand
SlotWidget_OffHand    -> OffHand
```

Les slots suivants sont la cible visuelle, mais nécessitent une étape C++ d'alignement avant d'être enregistrés fonctionnellement :

```text
SlotWidget_Face
SlotWidget_Shirt
SlotWidget_Bracers
SlotWidget_Earring1
SlotWidget_Earring2
```

Tant que ces slots ne sont pas ajoutés à `EGridEquipmentSlot` et `FGridCharacterEquipmentState`, ils doivent rester des placeholders visuels ou être désactivés.

## Slots exclus du paper doll

`SlotWidget_Cursor` ne doit pas être enfant du paper doll. Le Cursor est un état temporaire de manipulation, pas un équipement.

Les anciens concepts `Talisman`, `QuickSlot1`, `QuickSlot2` et `Accessory` ne font pas partie du panneau paper doll validé. Ils pourront être repris plus tard pour une barre rapide, un système de talismans actifs ou des accessoires secondaires, mais pas comme slots autour du personnage.

## Panneau de statistiques

Le panneau de statistiques peut être placé à droite du personnage. Structure recommandée :

```text
SizeBox_CharacterStatsPanel
-> Border_CharacterStatsPanel
   -> ScrollBox_StatsSections ou VerticalBox_StatsSections
      -> Border_DetailsSection
      -> Border_AttributesSection
      -> Border_DerivedStatsSection
      -> Border_CombatSection
      -> Border_MobilityProgressionSection
      -> Border_ResistancesSection
```

Les BindWidget existants de `UGridInventoryWidget` doivent être conservés et réutilisés :

- `Text_CharacterName` ;
- `Text_CharacterRace` ;
- `Text_CharacterClass` ;
- `Text_CharacterLevel` ;
- `Text_CharacterExperience` ;
- `Text_CharacterStrength` ;
- `Text_CharacterDexterity` ;
- `Text_CharacterConstitution` ;
- `Text_CharacterIntelligence` ;
- `Text_CharacterWisdom` ;
- `Text_CharacterCharisma` ;
- `Text_CharacterHealth` ;
- `Text_CharacterMana` ;
- `Text_CharacterCarryWeight` ;
- `Image_CharacterPortrait` ou futur `Image_CharacterFullBody` ;
- `Image_CharacterClassIcon` ;
- `Border_CharacterClassAccent`.

## Couche UI au-dessus

La couche UI au-dessus de l'inventaire doit rester séparée du paper doll :

- `WBP_ItemActionMenu` ;
- `WBP_ItemInspectPanel` ;
- `WBP_ItemReadPanel` ;
- éventuellement `SlotWidget_Cursor` ou une couche de curseur dédiée.

## WBP_ItemActionMenu

Parent class recommandé : `UUserWidget`.

Rôle :

- widget plein écran ;
- capture du clic extérieur ;
- conteneur du panneau de boutons ;
- relai des clics de boutons vers `UGridInventoryWidget`.

Hiérarchie recommandée :

```text
CanvasPanel_Root
-> Border_ClickCatcher ou Button_CloseArea
-> Border_MenuPanel
   -> VerticalBox_Actions
      -> WBP_ItemActionButton
```

Règles :

- `WBP_ItemActionMenu` reste plein écran ;
- ne jamais appeler `CurrentItemActionMenu.SetPositionInViewport(MousePosition)` ;
- seul `Border_MenuPanel` est déplacé à la souris via son `CanvasSlot` ;
- le clic extérieur appelle `OwnerInventoryWidget.CloseItemActionMenu("ClickOutside")` ;
- le menu visible exécute toujours par `ActionIndex`.

## WBP_ItemActionButton

Parent class recommandé : `UUserWidget`.

Variables attendues :

- `OwnerMenu` ;
- `ActionIndex` ;
- `ActionLabel` ;
- `bActionEnabled`.

Événement :

```text
OnClicked -> OwnerMenu.ExecuteActionByIndex(ActionIndex)
```

Ne pas exécuter par `ActionType`, car plusieurs actions peuvent partager le même type, par exemple plusieurs destinations `Equip`.

## WBP_ItemToolTip

Parent class recommandé : `UUserWidget`.

Rôle : information passive au survol.

Données recommandées :

- nom ;
- type ;
- poids ;
- description courte ;
- compatibilités principales ;
- état de lumière si applicable.

Le tooltip ne remplace pas l'action `Examiner`.

## Widgets de slots

Parent C++ attendu : `UGridInventorySlotWidget`.

Slots concernés :

- slots d'inventaire ;
- slots paper doll fonctionnels ;
- `MainHand` et `OffHand` ;
- `Cursor` hors paper doll.

Variables attendues :

- `SlotType` ;
- `InventorySlotIndex` ;
- `EquipmentSlot` pour les slots d'équipement ;
- `OwningInventoryWidget`.

Interactions :

- clic gauche : relai vers `HandleRegisteredSlotClicked` ou `HandleEquipmentSlotClicked` ;
- clic droit : `HandleItemSlotRightClicked(SlotType, InventorySlotIndex)` ;
- drop : `HandleSlotDrop(SourceSlotType, SourceSlotIndex, SlotType, InventorySlotIndex, ...)`.

Les Blueprints ne décident jamais de la compatibilité item/slot.

## WBP_ItemReadPanel

Parent class recommandé : `UUserWidget`.

Rôle : afficher le contenu textuel long d'un item lisible.

Hiérarchie recommandée :

```text
CanvasPanel_Root
-> Border_ClickCatcher
-> Border_ReadPanel
   -> VerticalBox_Root
      -> Text_Title
      -> ScrollBox_Content
         -> Text_ReadText
      -> Button_Close
         -> Text_Close
```

Fermeture :

- `Button_Close.OnClicked` appelle `OwnerInventoryWidget.CloseItemReadPanel("CloseButton")` ;
- `Border_ClickCatcher.OnMouseButtonDown` appelle `OwnerInventoryWidget.CloseItemReadPanel("ClickOutside")` ;
- `OnItemReadPanelCloseRequested` retire uniquement `CurrentItemReadPanel`.

`RemoveFromParent` ne doit jamais viser `WBP_GridInventory`.

## Checklist de construction

- Le personnage plein corps est au centre du panneau paper doll.
- Les slots paper doll sont autour du personnage, pas dans une grille séparée.
- `SlotWidget_Cursor` est hors paper doll.
- Aucun `ScaleBox` local dans `WBP_GridInventory`.
- Aucun `SizeBox_DesignSurface` local dans `WBP_GridInventory`.
- Les slots fonctionnels appellent `RegisterEquipmentSlotWidget`.
- Les slots non encore supportés par C++ restent placeholders visuels.
- Aucun Blueprint ne décide de la compatibilité item/slot.
- `WBP_ItemActionMenu` reste plein écran.
- `Border_MenuPanel` est positionné via `CanvasSlot`.
- Aucun `SetPositionInViewport` sur le menu plein écran.
- Les boutons appellent `ExecuteInventoryContextActionByIndex`.
- `RemoveFromParent` cible uniquement le widget à fermer, jamais `WBP_GridInventory`.
- Aucun message de debug Blueprint temporaire.
