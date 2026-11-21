# ARM-CLEAN06.5H.42 — Arc court deux mains depuis ItemSystem

## Remplacement de la vérification d'authoring historique

`Grimrock.RPG.ARMEMENT01.7.TwoHands.ProductionShortBowDefinition` ne charge plus `DA_Weapon_ShortBow_01`. Il exige la définition ARM physique N1 `Weapon_ShortBow_01` depuis `FGridCanonicalRegistryFixture`, vérifie sa politique `TwoHanded` et l'autorisation de demandes d'équipement depuis MainHand et OffHand.

Les cinq autres tests du fichier utilisent déjà des définitions canoniques transitoires pour vérifier le stockage physique unique en MainHand, la réservation virtuelle OffHand, les opérations de curseur, la restitution atomique et l'absence de doublons. Leur code et leurs assertions restent inchangés.

L'ancien include `Runtime/GridItemDefinitionAsset.h` a été retiré du fichier.

## Validation et périmètre

Aucun changement runtime, code de gameplay, `.uasset`, `.umap` ni `.sav`. Les 53 packages historiques du rapport H.23 ne sont pas supprimés.

Ne pas autoriser H.43 avant `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` en SUCCESS sur le SHA H.42, avec compilations Editor/Shipping et `Grimrock.RPG.ARMEMENT01.7.TwoHands`. L'audit des sept SaveGames demeure nécessaire avant tout retrait natif.
