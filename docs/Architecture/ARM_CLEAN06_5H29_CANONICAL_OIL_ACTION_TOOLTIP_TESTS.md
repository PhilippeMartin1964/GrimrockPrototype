# ARM-CLEAN06.5H.29 — Action et tooltip de l'huile par ItemSystem

## Tests convertis

Deux tests actifs ne chargent plus le package historique `DA_Item_Flask_Oil` : `Grimrock.RPG.ARMEMENT01.17.1.3.OilCombatAvailability` et `Grimrock.RPG.ARMEMENT01.17.1.3.OilTooltipPresentation`.

Le helper de fixture valide une racine de production `CanonicalStage/DA_ItemSystem`, résout `Item_Flask_Oil` au niveau physique N0 non-ARM, puis l'unique action détaillée liée `Action_Alchemist_OilSlick` avec `UGridItemSubsystem::ResolveDetailedNonArmAction`. La même définition d'action est utilisée pour les assertions de disponibilité et la projection du tooltip.

Les contrôles historiques sont conservés : surface d'huile ciblée, consommation d'une flasque, PA, absence de source, exécuteur désactivé, traduction, durée/portée, recalcul du tooltip à partir des données et exclusion des sorts non pertinents. Aucun raccourci ne reconstitue la mécanique à partir de l'ancien DataAsset.

## Exclusions et vérification

Aucun changement gameplay, huile Shipping/PIE, visualisation jaune/verte (issue #221), `.uasset`, `.umap`, `.sav` ou schéma persistant. Les 53 packages H.23 restent protégés et non supprimés.

H.30 est bloqué jusqu'à `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur le commit H.29, avec compilation Editor, Shipping et Automation, y compris les deux tests `Grimrock.RPG.ARMEMENT01.17.1.3`. Les sept SaveGames et la vérification native restent nécessaires avant le retrait des définitions historiques.
