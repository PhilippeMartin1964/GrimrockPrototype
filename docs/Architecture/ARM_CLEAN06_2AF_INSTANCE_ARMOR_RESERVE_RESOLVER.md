# ARM-CLEAN06.2AF — Protection résiduelle de la véritable instance

Ce jalon raccorde le calculateur pur 06.2AE à `UGridItemSubsystem`
avec `ResolveArmorReserveForInstance(const FGridItemInstance&, OutReserve)`.
Il reste indépendant du runtime de combat historique.

La méthode prend **une instance réelle**, dont l'identité `ItemDefinitionId`,
le niveau `ItemLevel` (1–20) et les deux dettes d'armure proviennent
des données persistées. Elle exige un GUID valide et une instance valide,
rejette explicitement le niveau Legacy 0 et les dépenses négatives.
La définition est retrouvée **exclusivement** dans le registre
`DA_ItemSystem` canonique, via `ResolveItem()` ; les munitions,
consommables, items inconnus et profils incomplets sont refusés.

La protection nominale est calculée depuis le seul
`WeaponCatalogue` pour le niveau propre à **l'objet**, et non le
niveau de son porteur. Les protections restantes sont
`max(0, Nominal - AbsorbedEquipmentPhysicalArmor)` et
`max(0, Nominal - AbsorbedEquipmentMagicalArmor)`.
Les deux compteurs consommés sont conservés tels quels, même
si l'objet est déplacé d'OffHand vers l'inventaire ou s'ils
dépassent la protection nominale. **Aucune recharge** n'est
effectuée. Les anciens `DisplayName` et `Weight` sont
ignorés pour ce calcul, pas supprimés des SaveGame.

Le test Automation
`Grimrock.RPG.ARM.CLEAN06_2.Canonical.InstancedArmorReserveSingleRegistry`
vérifie le registre absent, un bouclier de 3 points dont
2 sont consommés, un passage au sac, la dette supérieure
au nominal, la sortie remise à zéro après un refus,
le niveau 0, la dette négative, les munitions, une potion,
l'identité inconnue et le GUID absent, puis la récupération
sans perte d'état.

Il ne modifie pas les `.uasset`, `.umap`, les SaveGame,
les WBP ni les routines historiques de dépense/recharge.
Il n'active pas encore le nouveau système : sa validation
Windows 5.5.4 sur le SHA exact reste obligatoire avant 06.3.
