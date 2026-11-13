# ARM-CLEAN06.2R — Pas de mains/type implicites pour les ARM futurs

Les 82 archétypes numériques ARM001–ARM082
restent disponibles sous `DA_WeaponCatalogue`,
avec les 1 640 valeurs N1–N20 exactes.

Le **nouveau** `FGridCanonicalWeaponArchetype`
distingue les valeurs numériques déjà importées
et la preuve de ses deux propriétés de gameplay :
`HandPolicy` et `Kind`.
`bHandAndKindProven` vaut `false` par défaut :
l'existence d'un ARM dans le classeur n'autorise
pas le système à inventer sa permission de main,
ni à supposer qu'un passif est Shield ou Focus.

Le registre `UGridItemSystemAsset::Validate`
exige `bHandAndKindProven=true` uniquement pour
les archétypes effectivement liés à un
`UGridArmamentDefinitionAsset` concret.
Le résolveur `UGridItemSubsystem::ResolveItem`
refuse également un item concret marqué non prouvé.
Pour les 60 ARM sans item de production, la
bibliothèque de progression reste valide et
ne nécessite ni DataAsset, ni visual, ni
politique de mains arbitraire.

La valeur `bHandAndKindProven` est un garde
d'authoring ; elle ne donne aucun droit de
combat, ne crée pas d'action ni de réaction,
et ne remplace pas la preuve distincte des
munitions, effets ou présentations.

Le test `Grimrock.RPG.ARM.CLEAN06_2.Canonical.ActiveArmamentSemanticGate`
vérifie une bibliothèque de 82 profils,
un ARM inactif marqué non prouvé qui ne
bloque pas le registre, puis le refus de
l'ARM014 concret si sa preuve de mains/type
est retirée. Aucun asset binaire ni
lecteur historique n'est modifié.

Avant l'authoring binaire des 22 ARM,
les politiques réellement mesurées en
06.2F seront inscrites explicitement,
puis vérifiées sur les 22 `ItemPath`
du manifeste. Les 60 autres pourront être
authorés progressivement sans exiger une
migration fictive de toutes les références.
