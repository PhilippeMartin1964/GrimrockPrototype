# UI-CLEAN04 — Remove primitive/debug inventory display helpers

Date : **20 septembre 2026**  
Statut : **IMPLEMENTATION C++ ; validation locale UE5.5.4 à fournir**

## Objectif

Les premiers prototypes d'Inventaire exposaient plusieurs fonctions Blueprint qui fabriquaient des chaînes de diagnostic directement depuis `UGridInventoryWidget`.

Elles produisaient par exemple :

~~~text
Cursor: Item_Torch
MainHand: Item_Sword
OffHand: Empty
Slot 3: Item_Key
SelectedCharacter: 0 Elias Guerrier Lv1
~~~

La présentation actuelle ne fonctionne plus ainsi :

- les slots sont des `UGridInventorySlotWidget` qui reçoivent des `FGridItemInstance` ;
- la feuille personnage est projetée vers des BindWidget dédiés ;
- les portraits reçoivent `FGridInventoryCharacterSummary` ;
- aucun écran runtime ne doit afficher les anciennes chaînes de debug.

## Suppressions

UI-CLEAN04 retire :

~~~text
GetItemDisplayString()
GetCursorItemDisplayText()
GetMainHandDisplayText()
GetOffHandDisplayText()
GetInventorySlotDisplayText()
GetCharacterDisplayText()
GetSelectedCharacterDisplayText()
~~~

Ces fonctions n'avaient plus de consommateur C++ autre qu'elles-mêmes.

## Chemins canoniques conservés

Items :

~~~text
FGridItemInstance
-> UGridInventorySlotWidget::SetItem()
~~~

Personnage :

~~~text
UGridPartyInventoryComponent
-> FGridInventoryCharacterSummary
-> RefreshSelectedCharacterDetails()
-> Text_Character*
~~~

Portraits :

~~~text
FGridInventoryCharacterSummary
-> UGridPartyMemberWidget::SetCharacterSummary()
~~~

Il n'y a donc plus de deuxième couche de présentation basée sur des FString de diagnostic.

## Automation

Filtre :

~~~text
Grimrock.UI.Clean04
~~~

Test :

~~~text
Grimrock.UI.Clean04.NoPrimitiveDisplayHelpers
~~~

Le test vérifie la disparition des sept helpers et la présence des routes canoniques `GetCharacterSummary` et `RefreshInventory`.

## Contrôle UMG après build

Compiler/sauvegarder `WBP_CharacterSheet`, `WBP_InventoryBag` et `WBP_ItemActionMenu`.

Un nœud Blueprint cassé faisant référence à l'une des sept anciennes fonctions indiquerait un appel réellement encore utilisé et devra être signalé. Aucun fallback ne sera réintroduit sans besoin fonctionnel démontré.
