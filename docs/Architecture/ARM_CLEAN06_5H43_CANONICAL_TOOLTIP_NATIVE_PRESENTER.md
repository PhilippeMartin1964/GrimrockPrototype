# ARM-CLEAN06.5H.43 — Présentateur natif du tooltip ARM002 N9

## Test migré

Le test `Grimrock.UI.WeaponTooltip04.NativePresenterAndLocalization` ne charge plus `DA_Weapon_AssassinDagger_01`. Sa fixture `FGridCanonicalRegistryFixture` résout `Weapon_AssassinDagger_01` au niveau physique N9 depuis le registre ItemSystem, puis appelle `FGridWeaponTooltipProjection::Build(nullptr, Instance, Profile, Registry)` sans aucune définition héritée.

Les contrôles d'origine restent : clés de localisation réellement présentes dans ST_UI, visibilité du niveau, refus des vues invalides, deltas positifs de PA, absence de couleur fictive pour les faits non numériques, étiquettes d'armes localisées et non-publication de la famille technique.

## Périmètre

Aucun code gameplay ou UI natif, `.uasset`, `.umap`, `.sav` ni Blueprint WBP modifié. Les 53 packages historiques H.23 restent en place.

H.44 requiert `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur le SHA du commit, incluant compilations Editor/Shipping et Automation `Grimrock.UI.WeaponTooltip04.NativePresenterAndLocalization`. Les sept sauvegardes natives restent un préalable à toute suppression de packages.
