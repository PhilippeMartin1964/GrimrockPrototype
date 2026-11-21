# ARM-CLEAN06.5G.135 — Verrouillage de la matérialisation physique directe

**Parent :** `59d4831bbf80b8cea00ed0869deea7aaa4e78eff`. Validation des deux statuts Windows du parent exigée avant publication.

## Constat

`FGridItemInstanceMaterializationReadModel::TryApply` résout correctement toutes les identités canoniques à leur niveau physique N0/N1–N20. Mais sa branche `WITH_DEV_AUTOMATION_TESTS` permettait à une définition historique explicite d'être matérialisée **alors même que le caller avait fourni un `UGridItemSubsystem` configuré**, si l'ItemId était inconnu. Le lot 5G.134 verrouille déjà l'entrée dans le sac et le calcul des piles, mais ce lecteur bas niveau doit adopter le même contrat.

## Correction

- Pour un ItemId hors registre, la matérialisation historique n'est possible **que** si le registre est absent et que `GIsAutomationTesting` est actif. En production, le repli historique reste inaccessible.
- Le test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalInstanceMaterialization` vérifie désormais qu'une identité inconnue ne devient pas valide face au vrai root et qu'un refus laisse intacts GUID, quantité et niveau.
- La fixture isolée sans registre conserve la caractérisation du nom, de la masse 1.75 kg, du rejet d'une masse négative et du rejet d'une arme legacy sans profil. Les tests de 4 ARM aux niveaux N1/N20, flèches N0 et d'invariance de tous les champs durables restent inchangés.

**Aucun changement de schéma Unreal, de SaveGame, de DataAsset, de WBP ou de niveau.** Gates : `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur le SHA du lot. PIE et migration finale 06.5/06.6 encore dus.
