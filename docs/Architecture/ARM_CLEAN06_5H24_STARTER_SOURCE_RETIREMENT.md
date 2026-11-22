# ARM-CLEAN06.5H.24 — Retrait du chargeur historique de kits

## Portee

- Les 18 orientations de depart des six classes utilisent exclusivement les identites canoniques et les niveaux du `UGridItemSubsystem` configure par `DA_ItemSystem`.
- Aucun chemin de chargement de definitions historiques dans `RPGStartingEquipmentPresetCatalog.cpp`, en Editor, Automation ou Shipping.
- Suppression de la liste ancienne des 12 kits, de l'ancien point d'entree inutilise, et du garde-fou qui permettait une exception de fixture Automation.
- Le test `RegistrylessApprovedStarterFixture` prouve maintenant l'echec ferme si le registre est absent, meme en Automation.
- Les dix anciens tests dependants ont ete convertis en deux lots de fixtures canoniques valides par Windows sur `ccff6235` et `0a23aed8`.

## Barriere de retrait des assets natifs

Ce jalon **ne supprime aucun .uasset, niveau ou SaveGame**. Les 48 definitions historiques demeurent des candidates a une transaction Unreal Editor native, precedee d'un audit frais au SHA exact et de sauvegardes SHA256. Les deux catalogues sources d'authoring, deux Readables utilises par les sauvegardes et le redirecteur ThrowingKnife restent proteges. Les sept sauvegardes v27 privees ne sont pas modifiees.

## Verification

Gate obligatoire sur le commit de ce jalon : `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS, incluant build Development Editor, Shipping compile-only et suite `Grimrock`. La preuve PIE visuelle et le Cook/Package complet restent des validations distinctes avant cloture finale ARM-CLEAN06.
