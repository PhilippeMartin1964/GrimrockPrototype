# UI-SPLIT02 — Non-modal inventory world interaction and context-menu migration

Date : **20 septembre 2026**  
Statut : **AUTOMATION + VALIDATION MANUELLE PIE VALIDÉES — 20 septembre 2026**

## Décision

Les deux fenêtres split de l'inventaire sont des overlays non modaux :

~~~text
WBP_CharacterSheet   gauche
vue 3D               centre interactif
WBP_InventoryBag     droite
~~~

Tant qu'aucun menu contextuel/modal n'est ouvert, un clic dans la zone monde doit continuer à atteindre le runtime.

Exemple attendu :

~~~text
Inventaire ouvert
-> viser une torche sur son support dans la vue 3D centrale
-> cliquer
-> interaction monde exécutée
-> torche ajoutée au personnage sélectionné
-> OnPartyInventoryChanged
-> WBP_InventoryBag se rafraîchit immédiatement
~~~

Les pages legacy plein écran (Skills, Map, Journal, etc.) restent modales pendant leur migration.

## Correctif C++

AGrimrockPlayerController distingue désormais :

~~~text
split inventory workspace visible
    -> monde autorisé

legacy/full-page UI visible
    -> monde bloqué
~~~

La politique est centralisée dans :

~~~text
ShouldBlockWorldInteractionForInventoryUi(...)
~~~

Elle est utilisée à la fois par :

- ResolveLeftMouseInteraction ;
- UpdateHoveredInteractable.

Le menu contextuel d'item reste modal lorsqu'il est réellement ouvert.

## Régression clic droit constatée

Le C++ de clic droit n'a pas disparu :

~~~text
UGridInventorySlotWidget::NativeOnMouseButtonDown
-> HandleItemSlotRightClicked
-> BuildContextActionsForSlot
-> OnContextActionsRequested.Broadcast
~~~

La présentation visible était cependant implémentée dans l'Event Graph de l'ancien WBP_GridInventory.

Créer WBP_InventoryBag en copiant uniquement sa hiérarchie ne copie pas automatiquement :

- le binding OnContextActionsRequested ;
- la création de WBP_ItemActionMenu ;
- la variable CurrentItemActionMenu ;
- la logique de fermeture du menu ;
- la présentation Lire/Examiner éventuelle.

UI-SPLIT02 ajoute donc un diagnostic explicite :

~~~text
GridInventory RightClick PresentationMissing ... OnContextActionsRequestedUnbound
~~~

si le C++ construit correctement les actions mais qu'aucun presenter Blueprint n'est branché.

## Migration UMG requise

Le presenter du menu contextuel doit être déplacé depuis WBP_GridInventory vers WBP_InventoryBag.

Ne pas recréer les règles gameplay en Blueprint : il faut uniquement migrer la présentation existante.

WBP_InventoryBag doit recevoir :

~~~text
OnContextActionsRequested
-> créer/afficher WBP_ItemActionMenu
-> OwnerInventoryWidget = self
-> présenter LastContextActions
-> positionner Border_MenuPanel sous la souris
~~~

Le menu reste plein écran pour capter le clic extérieur, mais seul Border_MenuPanel est déplacé.

Les boutons continuent d'appeler :

~~~text
ExecuteInventoryContextActionByIndex(...)
~~~

## Automation

Filtre :

~~~text
Grimrock.UI.Split02
~~~

Test :

~~~text
Grimrock.UI.Split02.WorldInteractionPolicy
~~~

Ce test vérifie :

- UI fermée : monde autorisé ;
- shell legacy modal : monde bloqué ;
- workspace split ouvert : monde autorisé ;
- item tenu au curseur : routage monde conservé.

## Validation manuelle après migration UMG

1. ouvrir l'inventaire ;
2. cliquer une torche sur son support dans la zone 3D ;
3. vérifier qu'elle apparaît immédiatement dans le sac ;
4. clic droit sur la torche dans le sac ;
5. vérifier que WBP_ItemActionMenu apparaît ;
6. cliquer hors menu ;
7. vérifier que seul le menu contextuel se ferme.


## Validation Automation reçue

Validation locale du 20 septembre 2026 :

~~~text
Filter                 : Grimrock.UI.Split02
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
~~~

La politique C++ d'interaction monde avec le workspace split est validée. La suite concerne la migration UMG du presenter de menu contextuel vers WBP_InventoryBag.


## Validation manuelle PIE reçue

Le 20 septembre 2026, la migration du presenter de menu contextuel vers WBP_InventoryBag a été validée manuellement.

Validé :

~~~text
inventaire split ouvert
-> interaction avec le monde conservée

clic droit sur Item_Torch
-> 5 actions construites côté C++
-> WBP_ItemActionMenu affiché

clic extérieur
-> seul le menu contextuel se ferme

action du menu
-> inventaire rafraîchi correctement
~~~

Le problème Blueprint identifié lors de la migration était un chemin d'exécution manquant : Is Not Valid(CurrentItemActionMenu) devait rejoindre directement le Branch de création. La logique gameplay C++ n'était pas en cause.
