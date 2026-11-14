# ARM-CLEAN06.3AS — Matérialisation et piles d'inventaire sans DA_Weapon

Parent : `779514e` (06.3AR), tous les statuts Windows verts.

## Problème

Même pour un `ItemId` connu de `DA_ItemSystem`, le lecteur
`FGridItemInstanceMaterializationReadModel` exigeait un
`UGridItemDefinitionAsset` et l'inventaire s'en servait encore pour
`bStackable`/`MaxStackSize`, y compris pour les munitions.
Cela empêcherait le retrait vérifié des anciennes définitions en 06.4.

## Autorité

`FGridCanonicalInventoryStackReadModel` centralise la capacité d'une pile :

- `DA_ItemSystem::ResolveItem(ItemId, ItemLevel).MaxStack` pour toute
  identité enregistrée, y compris munitions N0 et armes N1–N20 ;
- aucun retour aux anciens `MaxStackSize` pour une identité connue,
  même si le niveau est invalide ou l'asset historique contredit le registre ;
- capacité historique uniquement pour une identité absente du registre.

Les entrées `ApplyItemDefinitionToInstance`,
`CanAddItemToCharacterInventory`, `AddItemToCharacterInventory`,
`TrySplitInventoryStackToFirstFreeSlot`, les déplacements/merges
d'inventaire et du curseur, et l'action contextuelle `SplitStack`
emploient désormais ce modèle.

Une instance ARM à **niveau explicite** est matérialisable sans
`DA_Weapon_*`. Une instance ARM N0 **ne se transforme jamais en N1** :
l'ancien `DefaultWeaponItemLevel` reste consulté uniquement si le
véritable asset d'authoring historique est présent. L'héritage N0
historique sera retiré après conversion 06.4. Une munition réelle N0
demeure N0.

Le modèle préserve `RuntimeObjectId`, propriétaire, réserves
d'armure consommées, état d'éclairage, niveau d'instance et
libellés personnalisés. Les transferts de piles conservent les GUID
originaux selon le contrat existant, avec GUID neuf pour chaque
nouvelle pile.

Les données `.uasset`, SaveGame et les structures sérialisées ne
sont pas modifiées. La procédure `RehydrateOwnedItemDefinitions`
des sauvegardes pré-migration nécessite encore la conversion
commandlet 06.4 ; ce jalon ne prétend pas supprimer toute la dette
historique.

## Validation

Test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryMaterializationWithoutLegacyDefinition`
avec `UGameInstance` réelle et `DA_ItemSystem` : flèches N0,
MaxStack dynamique du vrai registre, entrée, fusion, séparation,
cursor merge, anciennes capacités contradictoires, arme N19 sans
définition historique, arme N0 invalide sans réécriture et contrôle
d'un item hors registre.

Suites de régression attendues : `Grimrock.RPG.ARM.CLEAN06_3.*`,
`Grimrock.TechnicalDebt.TD06_4.InventoryStackMerge`, et intégralité
`Grimrock`.

**Localisation :** `UI.csv → ST_UI → WBP → Game/fr/en` inchangé.
Le label existant `NSLOCTEXT("GridItemActions", "SplitStack", "Scinder")`
est strictement réutilisé ; aucun nouveau texte WBP ou asset
de localisation. Rendu PIE/GPU non prouvé par la validation `-NullRHI`.
