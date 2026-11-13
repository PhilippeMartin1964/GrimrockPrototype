# ARM-CLEAN06.3C — Raccourcis des armes équipées depuis le registre unique

**Premier lecteur runtime effectivement basculé** : la création de
raccourcis d'attaque pour une des 22 armes du registre canonique.

`UGridPartyInventoryComponent::SetCharacterCombatHotbarBindingFromItem`
consulte le `UGridItemSubsystem` attaché à la véritable
`UGameInstance`. Pour un `ItemId` enregistré, il ne lit plus
`UGridItemDefinitionAsset.CombatActions`, ni
`CompatibleEquipmentSlots` et ne charge pas l'ancien DataAsset.
Il résout les `ItemActionBinding` de la racine unique, vérifie
la source `MainHand/OffHand` et l'identité GUID du vrai slot
équipé avant d'enregistrer l'`ActionId`.

Pour une identité canonique invalide, un objet passif ou un niveau
legacy 0, il **refuse** l'action, sans basculer vers l'ancien
système ni substituer arbitrairement N1. La sélection ne consomme
pas de ressources et ne réécrit pas les sauvegardes.

Les consommables, objets non canoniques et raccourcis de lancer
depuis le sac restent temporairement sur leurs lecteurs existants.
C'est une migration progressive par famille, pas deux autorités
pour la **même** arme. La lecture et l'exécution des actions
déjà présentes sur la hotbar restent à traiter séparément.

Test `RealNativeRegistryAndActions` : vérifie désormais
l'appartenance réelle des 22 `ItemId` dans le registre
et le refus d'une identité inconnue. Régression complète
`Grimrock` et validation PIE requises après les autres
lecteurs de combat, pas pour ce seul ajout.

Aucun .uasset, niveau, SaveGame, ni arme hors manifeste modifiés.
