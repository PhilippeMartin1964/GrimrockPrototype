# UI-CLEAN02 — Remove generated paper-doll legacy

Date : **20 septembre 2026**  
Statut : **AUTOMATION VALIDÉE — 20 septembre 2026**

## Objectif

Le paper doll actuel est entièrement authoré dans `WBP_CharacterSheet` avec les 18 `SlotWidget_*` manuels.

L'ancien générateur C++ de paper doll n'est plus appelé et ne doit pas rester comme deuxième chemin de construction potentiel.

## Suppressions

UI-CLEAN02 supprime de `UGridInventoryWidget` :

~~~text
BuildPaperDollEquipmentPanel()
CreatePaperDollEquipmentSlot()
ClearGeneratedPaperDollEquipmentPanel()

Border_EquipmentPanel

GeneratedPaperDollSlotWidgets
bPaperDollEquipmentPanelBuilt
bPaperDollMissingContainerLogged
bPaperDollMissingSlotClassLogged
LastBuiltPaperDollContainer
LastBuiltPaperDollSlotWidgetClass
~~~

Sont également supprimés les helpers et includes utilisés uniquement par ce générateur runtime.

## Chemin canonique conservé

Le paper doll est désormais exclusivement :

~~~text
WBP_CharacterSheet
-> SlotWidget_Head
-> SlotWidget_Face
-> ...
-> SlotWidget_MainHand
-> SlotWidget_OffHand
-> RegisterPaperDollEquipmentSlotWidgets()
-> RegisterEquipmentSlotWidget()
~~~

Le C++ ne construit plus de hiérarchie visuelle de paper doll.

`HasPaperDollPresentation()` se base uniquement sur la présence effective d'au moins un `SlotWidget_*`, et non plus sur un ancien conteneur `Border_EquipmentPanel`.

## Pourquoi

Le générateur runtime était explicitement marqué `Deprecated` et n'avait plus aucun appel de production.

Le conserver aurait maintenu :

- deux autorités possibles de construction du paper doll ;
- des tailles hardcodées C++ ;
- des containers UMG créés dynamiquement inutiles ;
- du state transient uniquement destiné au chemin abandonné.

UI-CLEAN02 supprime cette branche au lieu de la laisser dormant.

## Automation

Filtre :

~~~text
Grimrock.UI.Clean02
~~~

Test :

~~~text
Grimrock.UI.Clean02.NoGeneratedPaperDollLegacy
~~~

Le test vérifie que le générateur et son state ont disparu, tout en conservant les bindings manuels `SlotWidget_Head` et `SlotWidget_OffHand`.


## Validation Automation reçue

Validation locale du 20 septembre 2026 :

~~~text
Filter                 : Grimrock.UI.Clean02
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
~~~

Le générateur paper doll runtime supprimé n'est plus requis par le chemin canonique.
