# ARM-CLEAN06.5H.61 — Trois ARM de lancer équipées depuis ItemSystem

## Migration

Dans `Grimrock.RPG.ARMEMENT.LOT02.16.3.RealThrowingN1`, la partie « arme physiquement équipée » des trois cas `Weapon_ThrowingKnife_01` (ARM054), `Weapon_Javelin_01` (ARM057) et `Weapon_Dart_01` (ARM058) est convertie à `ItemSystem` N1. Une `FGridCanonicalRegistryFixture` vérifie l'identité, l'archétype, la politique de projectile, la non-empilabilité et le droit d'attaquer depuis MainHand ; chaque monde de combat est équipé d'un `FGridCanonicalLootTestScope`.

Les assertions de transaction de combat subsistent : coût d'un objet et des PA, un seul tir, retrait de l'arme de la main, aucune copie dans le sac, projectile réel unique et même GUID/quantité que l'arme consommée. Aucune définition historique n'est chargée ou enregistrée par ce premier sous-scénario.

## Fixture historique isolée et conservée

La seconde partie vérifie encore un ancien `QuickItem` de lancer de sac, avec `BuildInventoryCombatActionDefinition` historique. Sa définition n'est désormais chargée que dans ce bloc de compatibilité **sans registre**, et jamais dans les mondes du premier sous-scénario. Il ne faut pas la convertir mécaniquement en attaque QuickItem : le nouveau chemin de production pour un ARM physique depuis le sac utilise `MakePhysicalThrowItemActionId`, puis une visée via le contrôleur. Ce chemin nécessite une fixture controller/visée dédiée (H.62).

Aucun runtime, gameplay, HUD, WBP, `.uasset`, `.umap` ni `.sav` modifié. Aucun des 53 packages historiques H.23 n'est supprimé.

## Verrou de validation

Avant H.62 : `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur ce SHA, compilations Editor/Shipping et `Grimrock.RPG.ARMEMENT.LOT02.16.3.RealThrowingN1`. Audit natif des sept sauvegardes requis avant suppression.
