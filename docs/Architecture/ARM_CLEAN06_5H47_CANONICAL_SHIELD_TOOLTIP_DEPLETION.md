# ARM-CLEAN06.5H.47 — Tooltip ARM075, usure et comparaison canoniques

## Migration

Les tests `Grimrock.UI.TooltipClean01.ShieldNominalFiveAndRemaining` et `ShieldAfterVictoryAndGlobalComparison` ne chargent plus `DA_Weapon_RoundShield_01` ni `DA_Weapon_GreatAxe_01`. Les données ARM075 N1 et ARM031 N1 sont résolues par `UGridItemSubsystem`. `FGridWeaponTooltipProjection::Build` reçoit systématiquement un pointeur de définition historique nul.

Pour la comparaison d'équipement, le composant d'inventaire est rattaché à un vrai monde transitoire via `FGridCanonicalWeightTestWorld`. Le widget de slot transitoire retrouve le registre canonique en utilisant son composant d'inventaire parent, selon le chemin de production déjà présent dans `GridInventorySlotWidget.cpp`.

## Non-régressions

Les assertions d'origine sont conservées : nominal ARM075 toujours égal à 5, épuisement à 0/5 après dégâts supérieurs à la réserve, exactement une ligne `WeaponPhysicalArmorRemaining`, comparaison d'armure avant victoire (-2), aucune recharge pendant une prévisualisation, recharge après victoire (5/5), comparaison après victoire (-5), conservation du GUID et des HP. Une vérification s'assure que la racine canonique reste en lecture seule.

Aucun code gameplay, WBP, `.uasset`, `.umap`, `.sav`, effet Niagara ou UI de production n'est modifié. Les 53 packages historiques identifiés dans H.23 restent présents.

## Barrière de validation

H.48 est bloqué jusqu'à `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur ce commit, avec compilations Development Editor et Shipping et Automation `Grimrock.UI.TooltipClean01`. Audit natif des sept sauvegardes encore requis avant tout retrait destructeur.
