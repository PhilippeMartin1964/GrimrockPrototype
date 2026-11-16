# ARM-CLEAN06.4X.17 — Régression : torche impossible à équiper

## Reproduction utilisateur (16 novembre 2026)

Une torche peut être retirée de son support et y être remise. Le
retrait vers le sac fonctionne, les armes, potions, flasques et
combats fonctionnent. Mais la torche ne peut plus être placée
en `MainHand` ni `OffHand`, supprimant la source lumineuse du groupe.

## Cause vérifiée dans le code

L'ancien cutover `ARM-CLEAN06.3G` branche
`CanEquipItemToSlot` et `FGridCanonicalEquipmentSlotsReadModel`
sur les identités du registre, puis refuse **tous**
les `Resolved.bIsArmament==false`. Or la nouvelle classe
`UGridOtherEquipmentDefinitionAsset` contient déjà
la politique non-ARM réelle `HandPolicy` et
`EquipmentSlots`, y compris pour les torches.

## Correction

`UGridItemSubsystem::ResolveItem` projette désormais
`NonArmEquipmentSlots` et `bNonArmTwoHanded` depuis
la seule `UGridOtherEquipmentDefinitionAsset`, sans
consulter de vieux `UGridItemDefinitionAsset` pour
les identités enregistrées. Les emplacements des ARM
restent fournis par le catalogue d'armes.

Le modèle des menus, tooltips et glisser-déposer utilise
ces slots, et `CanEquipItemToSlot` accepte le même contrat.
`IsTwoHandedItem` respecte également les équipements
canoniques non-ARM qui occupent deux mains. Un item
non-ARM physique est toujours N0. Les munitions et
consommables n'ont pas d'emplacements par cette projection.
La conversion ne force jamais `bLightsEnabled` : l'état
de lumière de chaque instance survit au transfert.

## Régression automatisée

`Grimrock.RPG.ARM.CLEAN06_4X17.TorchCanonicalNonArmHandEquip`
charge la véritable `DA_Item_Torch` et le registre
`DA_ItemSystem`, vérifie les mains et le profil lumineux,
équipe deux instances N0 depuis le sac dans chacune des
mains et contrôle GUID/niveau/lumière. Vérifie qu'une
munition non-ARM ne gagne pas d'autorisation de main
même si l'ancien DataAsset en déclare une.

**Limites :** les tests `-NullRHI` ne certifient
pas le rendu réel de la torche tenue ni la luminosité
effective en PIE. Ces deux points doivent être
retestés en jeu après compilation et validation UE.
Aucun `.uasset`, `.umap` ni `.sav` modifié.
