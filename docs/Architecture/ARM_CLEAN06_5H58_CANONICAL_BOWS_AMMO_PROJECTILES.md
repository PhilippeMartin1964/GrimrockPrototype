# ARM-CLEAN06.5H.58 — ShortBow/RecurveBow N1 et flèches N0 canoniques

## Objet

`Grimrock.RPG.ARMEMENT.LOT02.16.2.BowN1Execution` résout `Weapon_ShortBow_01` (ARM045) et `Weapon_RecurveBow_01` (ARM048) au niveau physique N1, ainsi que `Item_Arrows_01` au niveau N0, depuis `DA_ItemSystem` avec `FGridCanonicalRegistryFixture`.

Le test compare l'identité de l'archétype ARM et le `RequiredAmmoKindId` des arcs à `AmmoKindId` des flèches, sans emprunter la propriété héritée `RequiredAmmunitionItemDefinitionId`. Les armes conservent leur politique deux mains et leur autorisation MainHand. Les stocks de deux flèches utilisent `ApplyItemDefinitionToInstance`, sans chargement ni enregistrement historique.

## Monde réel et non-régressions

Chacun des trois mondes de test par arc (tir accepté, refus par mur solide, refus hors portée N1) reçoit un `FGridCanonicalLootTestScope` jusqu'à destruction du monde. Le test conserve les assertions de création d'un vrai projectile récupérable, décrément d'une seule flèche, GUID du stock, coût en PA et journal uniques, refus sans flèches, refus devant mur avec raison `PassageBlocked`, refus à N1+1 avec raison `TargetOutOfRange` et absence de dépense en cas de refus.

Les autres tests MON12, notamment son authoring audio transitoire et les armes de lancer, sont inchangés. Aucun gameplay, `.uasset`, `.umap`, `.sav`, GPU, Niagara, audio de production ou schéma sauvegardé modifié; les 53 packages H.23 ne sont pas supprimés.

## Validation

Avant H.59 : `UE5.5.4/SilentTask = SUCCESS` ET `UE5.5.4/LocalAuto = SUCCESS` sur le SHA H.58 (Editor, Shipping, Automation `Grimrock.RPG.ARMEMENT.LOT02.16.2.BowN1Execution`). Les sept sauvegardes privées nécessitent toujours la vérification native préalable à toute suppression.
