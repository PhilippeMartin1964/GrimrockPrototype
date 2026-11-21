# ARM-CLEAN06.5H.49 — Tooltips ARM002 N1/N9 et ARM075 N12 depuis ItemSystem

## Périmètre

Les deux fixtures `Grimrock.UI.WeaponTooltip04.FixedLevelsAndDice` et `Grimrock.UI.WeaponTooltip04.PassiveShieldAndLegacy` cessent de charger les anciens `DA_Weapon_AssassinDagger_01` et `DA_Weapon_RoundShield_01` pour leurs profils de production. Une `FGridCanonicalRegistryFixture` résout ARM002 N1/N9 et ARM075 N12 dans `DA_ItemSystem`; `FGridWeaponTooltipProjection::Build` reçoit une définition historique nulle et le registre réel.

Les assertions de dés d'attaque, minimum/maximum, PA, niveau réel, armure passive et absence de dés/PA fictifs restent intactes, mais la source des chiffres est désormais `FGridResolvedItem::Weapon`. Le registre est vérifié en lecture seule.

## Fixture historique volontaire

Le `UGridItemDefinitionAsset` synthétique `Legacy_ItemWithoutCatalogue` est conservé exclusivement pour tester qu'un objet non enregistré n'acquiert pas rétroactivement de profil. Le niveau invalide N21 doit également échouer en présence du registre canonique.

Les autres tests du fichier (intégration du slot, comparaison, PA et sous-type physique) restent à migrer dans des lots séparés. Ne pas supprimer leurs anciennes définitions par cette étape.

Aucun changement runtime, UI, `.uasset`, `.umap`, `.sav`. Aucun retrait des 53 packages historiques H.23.

H.50 est interdit avant `UE5.5.4/SilentTask = SUCCESS` ET `UE5.5.4/LocalAuto = SUCCESS` sur ce SHA, y compris compilations Editor/Shipping et tests `Grimrock.UI.WeaponTooltip04`. Le retrait des packages exige encore l'audit natif des sept SaveGames.
