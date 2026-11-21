# ARM-CLEAN06.5H.53 — Archer N9 canonique dans le test ARMEMENT01.9

## Périmètre

`Grimrock.RPG.ARMEMENT01.9.StartingKits.TwoHandedAndFixedLevels` utilise désormais un `UWorld` et le vrai `UGridItemSubsystem` avec `Weapon_ShortBow_01` N9 et `Weapon_Dagger_01` N1. L'ancienne source `DA_Weapon_ShortBow_01` ou `DA_Weapon_Dagger_01` n'est plus lue par ce scénario.

Les entrées `FRPGStartingEquipmentEntry` utilisent explicitement `CanonicalItemId` et `ItemLevel`, sans pointeur de définition. La dague précédemment testée avec un niveau hérité de la définition est maintenant N1 explicite : le registre canonique interdit l'ancienne règle N0/default sur les armes. Le test vérifie la quantité unique, le slot physique MainHand de l'arc, l'occupation OffHand virtuelle, les niveaux physiques N9/N1, les GUID, le kit durable et l'absence d'enregistrement de définitions obsolètes.

## Suites séparées

`LegacyCreationUnchanged`, `DaggerAndShield` et `RejectionIsAtomic` conservent temporairement leur ancien dispositif pour être convertis indépendamment aux étapes suivantes. Aucune modification des classes et kits de production, des sauvegardes binaires ni du schéma runtime.

## Verrou Windows

Ne pas publier H.54 sans `UE5.5.4/SilentTask = SUCCESS` ET `UE5.5.4/LocalAuto = SUCCESS` sur le SHA H.53, avec compilation Editor, Shipping et automation `Grimrock.RPG.ARMEMENT01.9.StartingKits`. Aucun des 53 packages H.23 supprimé. Audit natif des sept SaveGames obligatoire avant tout retrait.
