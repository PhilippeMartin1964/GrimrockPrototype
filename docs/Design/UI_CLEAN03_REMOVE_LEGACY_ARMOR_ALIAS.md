# UI-CLEAN03 — Remove legacy physical-armor alias

Date : **20 septembre 2026**  
Statut : **AUTOMATION + CONTRÔLE UMG VALIDÉS — 20 septembre 2026**

## Objectif

La migration de la feuille personnage utilise désormais le binding canonique :

~~~text
Text_CharacterPhysicalArmor
~~~

L'ancien alias :

~~~text
Text_CharacterArmor
~~~

était encore alimenté en parallèle uniquement pour compatibilité avec l'ancien WBP monolithique.

Depuis UI-CLEAN01, cet ancien écran a été supprimé. L'alias n'a donc plus de valeur.

## Suppression

UI-CLEAN03 retire de `UGridInventoryWidget` :

~~~text
Text_CharacterArmor
~~~

et supprime les deux projections doublonnées qui écrivaient la même valeur dans :

~~~text
Text_CharacterArmor
Text_CharacterPhysicalArmor
~~~

La projection devient unique :

~~~text
PhysicalArmorText
-> Text_CharacterPhysicalArmor
~~~

Aucune donnée gameplay ne change. La source reste :

~~~text
FGridInventoryCharacterSummary
-> Resources.CurrentPhysicalArmor
-> EquipmentStatBonus.ArmorBonus
~~~

## Contrat UMG

Le seul nom valide pour l'armure physique est désormais :

~~~text
Text_CharacterPhysicalArmor
~~~

Si `WBP_CharacterSheet` contient encore un widget nommé `Text_CharacterArmor`, il doit être supprimé dans le Designer après validation du build.

## Automation

Filtre :

~~~text
Grimrock.UI.Clean03
~~~

Test :

~~~text
Grimrock.UI.Clean03.NoLegacyArmorAlias
~~~

Le test vérifie que :

- `Text_CharacterArmor` n'existe plus dans la classe native ;
- `Text_CharacterPhysicalArmor` existe toujours ;
- `Text_CharacterMagicalArmor` existe toujours.


## Validation reçue

~~~text
Filter                 : Grimrock.UI.Clean03
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
~~~

Contrôle UMG manuel reçu : `WBP_CharacterSheet` ne contient aucun widget `Text_CharacterArmor`. Aucun nettoyage asset supplémentaire n'est requis pour ce binding.
