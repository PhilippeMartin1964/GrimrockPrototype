# UI-SPLIT03 — Role-aware paper-doll registration

Date : **20 septembre 2026**  
Statut : **IMPLEMENTATION C++ ; validation locale UE5.5.4 à fournir**

## Problème

Après UI-SPLIT01/UI-SPLIT02, deux widgets distincts dérivent toujours de UGridInventoryWidget :

~~~text
WBP_CharacterSheet -> UGridCharacterSheetWidget
WBP_InventoryBag   -> UGridInventoryBagWidget
~~~

Le sac de droite ne contient volontairement aucun slot d'équipement. Pourtant NativeConstruct appelait encore RegisterPaperDollEquipmentSlotWidgets() sans vérifier le rôle visuel du widget.

Conséquence : à chaque ouverture du workspace split, WBP_InventoryBag produisait 18 faux warnings :

~~~text
GridInventory PaperDoll SlotMissing Widget=SlotWidget_Head ...
...
GridInventory PaperDoll SlotMissing Widget=SlotWidget_OffHand ...
~~~

Le CharacterSheet, lui, pouvait être correctement enregistré et validé.

## Correction

UGridInventoryWidget expose désormais :

~~~text
HasPaperDollPresentation()
~~~

Depuis UI-CLEAN02, la méthode retourne vrai uniquement si la vue possède réellement au moins un des 18 widgets `SlotWidget_*` du paper doll. L'ancien `Border_EquipmentPanel` C++ a été supprimé avec le générateur runtime.

NativeConstruct devient :

~~~text
RegisterBoundPartyMemberWidgets()

if (HasPaperDollPresentation())
{
    RegisterPaperDollEquipmentSlotWidgets()
    ValidatePaperDollEquipmentRegistration()
}

RefreshRegisteredSlotWidgets()
~~~

Ainsi :

- WBP_CharacterSheet conserve l'enregistrement et la validation des 15 slots ;
- WBP_InventoryBag ne tente plus d'enregistrer des slots qu'il ne doit pas posséder ;
- les vrais warnings d'un CharacterSheet incomplet restent visibles ;
- aucun warning n'est simplement masqué globalement.

## Automation

Filtre :

~~~text
Grimrock.UI.Split03
~~~

Test :

~~~text
Grimrock.UI.Split03.PaperDollRole
~~~

Le test vérifie qu'un InventoryBag sans paper doll est reconnu comme tel et qu'une vue possédant un slot paper doll est reconnue comme présentation paper doll.

## Validation manuelle attendue

Après validation Automation, ouvrir l'inventaire en PIE et vérifier :

~~~text
GridInventory PaperDoll Validation OK Registered=18
~~~

pour la feuille de personnage, sans série de 18 warnings SlotMissing provenant du sac.


## Mise à jour UI-CLEAN02

Le générateur runtime de paper doll et `Border_EquipmentPanel` ont été supprimés. La détection de rôle repose désormais uniquement sur les slots manuels réellement authorés.
