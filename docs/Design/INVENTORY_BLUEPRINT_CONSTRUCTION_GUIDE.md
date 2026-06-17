# Inventory Blueprint Construction Guide

Ce guide décrit la construction attendue des Blueprints d'inventaire. Il complète `ITEM_CONTEXT_ACTION_SYSTEM.md` et `INVENTORY_INTERACTION_ROUTING.md`.

## Principe

Les Blueprints affichent et relaient les intentions. Le gameplay reste en C++.

Un Blueprint d'inventaire peut :

- afficher des slots, tooltips, menus et panneaux ;
- stocker les paramètres UI nécessaires, comme `SlotType`, `SlotIndex` ou `ActionIndex` ;
- appeler une fonction C++ explicite ;
- fermer ou repositionner des widgets UI.

Un Blueprint d'inventaire ne doit pas :

- décider qu'un item est compatible avec une main ou une cible ;
- déplacer directement un item entre inventaire, équipement, Cursor ou réceptacle ;
- appeler une porte, une serrure ou un lien gameplay ;
- utiliser `ExecuteInventoryContextAction(ActionType, ...)` depuis le menu visible.

## WBP_GridInventory

Parent class attendu : `UGridInventoryWidget`.

Rôle :

- racine de la page inventaire ;
- création/affichage des slots ;
- réception de `OnContextActionsRequested` ;
- création et fermeture du menu contextuel ;
- implémentation des événements d'affichage `PresentItemExamination`, `PresentItemReading` et `OnItemActionMenuCloseRequested`.

Hiérarchie recommandée :

- `Root` canvas ou overlay ;
- zone `Page_Inventory` ;
- grille d'inventaire ;
- slots `MainHand`, `OffHand`, `Cursor` ;
- couche UI au-dessus pour `WBP_ItemActionMenu`, `WBP_ItemInspectPanel`, `WBP_ItemReadPanel`.

```mermaid
flowchart TD
    Root["WBP_GridInventory<br/>UGridInventoryWidget"] --> Page["Page_Inventory"]
    Page --> Grid["InventoryGrid"]
    Page --> Main["Slot MainHand"]
    Page --> Off["Slot OffHand"]
    Page --> Cursor["Slot Cursor"]
    Root --> Overlay["Overlay UI"]
    Overlay --> Menu["WBP_ItemActionMenu"]
    Overlay --> Inspect["WBP_ItemInspectPanel futur"]
    Overlay --> Read["WBP_ItemReadPanel futur"]
```

Variables obligatoires :

- `CurrentItemActionMenu` : référence au menu courant ;
- `ItemActionMenuClass` : classe du menu ;
- références aux panels de slots si la construction est manuelle.

Événements à implémenter :

- `OnContextActionsRequested(SlotType, SlotIndex)` : créer ou réinitialiser `WBP_ItemActionMenu` ;
- `OnItemActionMenuCloseRequested(Reason)` : retirer uniquement `CurrentItemActionMenu` ;
- `PresentItemExamination(Item, ExaminationText)` : afficher un panneau d'inspection stable ou transitoire ;
- `PresentItemReading(Item, Title, ReadText)` : afficher le panneau de lecture.

Erreurs à éviter :

- ne jamais faire `RemoveFromParent` sur `WBP_GridInventory` pour fermer le menu ;
- ne pas changer de `TopTabs` à la fermeture du menu ;
- ne pas appeler `ExecuteInventoryContextAction(ActionType, ...)` depuis les boutons visibles.

## WBP_ItemActionMenu

Parent class recommandé : `UUserWidget`.

Rôle :

- widget plein écran ;
- capture du clic extérieur ;
- conteneur du panneau de boutons ;
- relai des clics de boutons vers `UGridInventoryWidget`.

Hiérarchie recommandée :

- `CanvasPanel_Root` plein écran ;
- `Button_CloseArea` ou `Border_ClickCatcher` plein écran, invisible ou alpha `0.0` si le hit-test fonctionne ;
- `Border_MenuPanel` au-dessus du click catcher ;
- conteneur vertical pour les `WBP_ItemActionButton`.

```mermaid
flowchart TD
    Root["WBP_ItemActionMenu<br/>plein écran"] --> Catcher["Border_ClickCatcher<br/>plein écran"]
    Root --> Panel["Border_MenuPanel<br/>positionné à la souris"]
    Panel --> Actions["VerticalBox_Actions"]
    Actions --> Button["WBP_ItemActionButton"]
```

Pourquoi le menu reste plein écran :

```mermaid
flowchart LR
    Full["Widget plein écran"] --> Outside["Capture clic extérieur"]
    Full --> Inside["Contient Border_MenuPanel"]
    Inside --> Position["CanvasSlot positionné à la souris"]
    Outside --> Close["CloseItemActionMenu"]
    Position --> Buttons["Boutons par ActionIndex"]
```

Variables obligatoires :

- `OwnerInventoryWidget` : référence vers `WBP_GridInventory` / `UGridInventoryWidget` ;
- `SourceSlotType` ;
- `SourceSlotIndex` ;
- liste locale de boutons si besoin.

Variables `Expose on Spawn` recommandées :

- `OwnerInventoryWidget` ;
- `SourceSlotType` ;
- `SourceSlotIndex` ;
- position souris initiale.

Événements à implémenter :

- construction des boutons à partir de `OwnerInventoryWidget.LastContextActions` ;
- clic extérieur : `OwnerInventoryWidget.CloseItemActionMenu("ClickOutside")` ;
- `ExecuteActionByIndex(ActionIndex)` : appeler `OwnerInventoryWidget.ExecuteInventoryContextActionByIndex(SourceSlotType, SourceSlotIndex, ActionIndex)`.

Règles de positionnement :

- `WBP_ItemActionMenu` reste plein écran ;
- ne jamais appeler `CurrentItemActionMenu.SetPositionInViewport(MousePosition)` ;
- seul `Border_MenuPanel` est déplacé à la souris via son `CanvasSlot`.

```text
À ne jamais faire :
CurrentItemActionMenu.SetPositionInViewport(MousePosition)

À faire :
WBP_ItemActionMenu reste plein écran.
Border_MenuPanel est repositionné via son CanvasSlot.
```

Erreurs à éviter :

- placer le widget plein écran à la position souris ;
- laisser le click catcher au-dessus du panneau ;
- utiliser un alpha visible de debug ;
- laisser des messages de debug Blueprint temporaires ;
- retirer `WBP_GridInventory` au lieu du menu.

## WBP_ItemActionButton

Parent class recommandé : `UUserWidget`.

Rôle :

- représenter une entrée de menu ;
- conserver l'index exact de l'action ;
- relayer le clic au menu parent.

Variables obligatoires :

- `OwnerMenu` ;
- `ActionIndex` ;
- `ActionLabel` ;
- `bActionEnabled`.

Variables `Expose on Spawn` :

- `OwnerMenu` ;
- `ActionIndex` ;
- `ActionLabel` ;
- `bActionEnabled`.

Événement :

- `OnClicked` appelle `OwnerMenu.ExecuteActionByIndex(ActionIndex)`.

Erreur critique à éviter :

- ne pas exécuter par `ActionType`, car plusieurs boutons peuvent partager `Equip`.

## WBP_ItemToolTip

Parent class recommandé : `UUserWidget`.

Rôle :

- information passive au survol ;
- nom, type, poids, description courte, compatibilités principales.

Données :

- `DisplayName` ;
- `Description` ;
- `ItemType` ;
- `CompatibleEquipmentSlots` ;
- indication lumière si applicable.

Erreur à éviter :

- ne pas confondre tooltip et action `Examiner`.

## Widgets De Slots

Parent C++ attendu : `UGridInventorySlotWidget`.

Slots concernés :

- slots d'inventaire ;
- `MainHand` ;
- `OffHand` ;
- `Cursor`.

Variables obligatoires :

- `SlotType` ;
- `InventorySlotIndex` ;
- `OwningInventoryWidget`.

Responsabilités :

- afficher l'item courant ;
- relayer clic gauche, clic droit et drag/drop ;
- ne pas appliquer de règle gameplay locale.

Interactions :

- clic droit : `HandleItemSlotRightClicked(SlotType, InventorySlotIndex)` ;
- drop : `HandleSlotDrop(SourceSlotType, SourceSlotIndex, SlotType, InventorySlotIndex, ...)`.

## Futur WBP_ItemInspectPanel

Parent recommandé : `UUserWidget`.

Rôle :

- panneau stable pour `Examiner` ;
- affichage détaillé d'un item sans le confondre avec le tooltip.

Contenu recommandé :

- titre ;
- icône ;
- description courte enrichie ;
- type, poids, compatibilités ;
- état lumière ou tags utiles.

Fermeture :

- clic volontaire ;
- bouton fermer ;
- Escape si disponible.

## Futur WBP_ItemReadPanel

Parent recommandé : `UUserWidget`.

Rôle :

- panneau persistant pour `Lire` ;
- livres, parchemins, notes et textes longs.

Hiérarchie recommandée :

- fond sombre ;
- `Text_Title` ;
- `ScrollBox` ou `RichTextBlock` ;
- indication `Cliquez ou déplacez-vous pour fermer`.

Données :

- `Title` ;
- `ReadText`.

Règle :

- `ReadText` vient de la définition d'item ;
- `Description` reste le texte court de tooltip/examen.

## Checklist De Construction

- `WBP_ItemActionMenu` plein écran.
- `Border_ClickCatcher` derrière `Border_MenuPanel`.
- `Border_MenuPanel` positionné via `CanvasSlot`.
- Aucun `SetPositionInViewport` sur le menu plein écran.
- `WBP_ItemActionButton` reçoit `ActionIndex`.
- Les boutons appellent `ExecuteInventoryContextActionByIndex`.
- Le clic extérieur appelle `CloseItemActionMenu("ClickOutside")`.
- `RemoveFromParent` cible uniquement `WBP_ItemActionMenu`.
- Aucun message de debug Blueprint temporaire.
- Aucune logique gameplay locale dans les Blueprints.
