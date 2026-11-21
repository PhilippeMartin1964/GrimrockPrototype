# ARM-CLEAN06.5H.52 — Delta de dégâts physiques ARM025 / ARM001 depuis ItemSystem

## Migration

`Grimrock.UI.WeaponTooltip04.PhysicalSubtypeRawDamage` ne lit plus `DA_Weapon_WarPick_01` et `DA_Weapon_Dagger_01`, ni leurs `TryGetWeaponCatalogueProfile`. Les armes ARM025 War Pick N1 et ARM001 Dagger N1 proviennent désormais de `FGridCanonicalRegistryFixture`. Le calcul des lignes de comparaison est exécuté avec `FGridWeaponTooltipProjection::BuildComparison(nullptr, ..., nullptr, ..., Registry)`.

Les assertions conservent les deux types Physique, leurs sous-types Perforant/Tranchant distincts, les deltas de dégâts minimum et maximum `+3`, leur couleur sémantique positive et l'absence de delta mathématique pour le type et les formules de dés. La racine ItemSystem reste en lecture seule.

## Frontière

Les autres tests `WeaponTooltip04` restent inchangés et certains utilisent encore des fixtures de simulation historique expressément destinées à caractériser l'ancien format. Aucun code UI/gameplay, `.uasset`, `.umap`, `.sav` modifié. Aucun des 53 packages historiques H.23 n'est supprimé.

H.53 est bloqué jusqu'aux deux statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur le SHA H.52, compilations Editor/Shipping et Automation `Grimrock.UI.WeaponTooltip04`. Le retrait des packages historiques reste subordonné au nouvel audit natif des sept SaveGames.
