# ARM-CLEAN06.5G.133 — Retrait des réserves et bonus legacy pour une identité inconnue

**Parent :** `ec21801000b48476c8ac922e80c12f7875e50dc6` (5G.132, statuts Windows obligatoires avant publication).

## Problème

`FGridEquipmentArmorReadModel::TryResolve` réinterprétait les identités inconnues avec `UGridItemDefinitionAsset::GetEffectiveEquipmentStatBonusForInstance`, même avec un `UGridItemSubsystem` configuré. `FGridEquipmentContributionReadModel` reprenait bonus et résistances de ces anciennes définitions. Une présence physique mal résolue aurait donc pu recevoir de la protection et des caractéristiques sans définition dans `DA_ItemSystem`.

## Correction et tests

Le lecteur d'armure reste strictement canonique pour les identités enregistrées (niveau physique réel, dettes d'armure conservées). Pour les inconnues dans un monde configuré, il réinitialise la vue et refuse la lecture ; les bonus et résistances restent vides. Le fallback de calcul historique, compilé seulement avec `WITH_DEV_AUTOMATION_TESTS`, exige un registre absent et `GIsAutomationTesting`. Ainsi aucune formule historique de protection n'est retenue dans le binaire de production.

Les tests `Grimrock.RPG.ARM.CLEAN06_3.CanonicalEquipmentArmorReadModel` et `CanonicalEquipmentDiagnosticContributions` vérifient maintenant le refus configuré et conservent des assertions des mêmes réserves anciennes **sans registre**. Les tests réels ARM N1/N20 et non-ARM N0 restent présents.

**Invariants :** aucune modification du schéma `FGridItemInstance`, aucune migration de `.uasset`, de SaveGame, de WBP ou de localisation, et aucune modification du calcul canonique. Gates : `SilentTask` et `LocalAuto` SUCCESS sur nouveau SHA ; test PIE visuel distinct à la recette 06.6.
