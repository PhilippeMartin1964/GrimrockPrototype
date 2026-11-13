# ARMEMENT01.17.3D.1 — Migration native du catalyseur

> **ARM-CLEAN06 — ARCHITECTURE CIBLE APPROUVÉE (13.11.2026).** [Contrat](../Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md), [migration](../Architecture/ARM_CLEAN06_MIGRATION_PLAN.md). Les mécaniques/test historiques demeurent ; les anciens schémas DataAsset sont remplacés. **Non implémentée : zéro legacy dans le runtime final.**


Date : 5 novembre 2026. Base : 66570694 (correction CheckAddress).
Objectif : persister dans le DataAsset approuvé les paramètres C6
actuellement projetés par ProjectApprovedAction, sans remplacer le contenu artistique.

## Contrat

- Un seul binaire autorisé : DA_Item_SurfaceCatalyst_01.uasset.
- Contrôle strict des ItemDefinitionId / ActionIdOverride et références EXACTES
  SM_Catalyst_01 et T_Catalyst_Icon ; refus si artist asset inattendu.
- Persistage seulement des cinq champs QuickItemCombatAction : TargetingPolicy=Area,
  AreaRadiusCells=1, AreaShape=Square, bAllowNoSurfaceReaction=true,
  bUsableDuringExploration=true. Coût, portée, LOS et effets C6 inchangés.
- L'effet Niagara LiquidSparkle/NS_LiquideSparkle est exclusif à la potion
  de soin (ITEM-VFX01.4A). Seule cette attribution éventuelle est retirée
  du catalyseur ; tous les autres effets VFX artistiques sont préservés.
- L'authoring ne réexécute jamais Configure sur l'asset existant.

## Production protégée

Un test exige les cinq champs BRUTS sérialisés et une politique VFX conforme.
Au seul échec SerializedCatalystContract (éventuellement accompagné par
Grimrock.Items.VFX01_4A.ProductionAssetAudit), l'agent Windows peut lancer
MigrateSurfaceCatalystNativeData.ps1 dans son worktree isolé.
Avant toute modification : contrôle SHA, worktree propre, origin, fichier LFS,
présence des assets de l'artiste, sauvegarde privée de l'original avec SHA-256.
Seul le .uasset existant peut être commité, avec un seul push non forcé.

Commandlet : RPGAlchemistAuthoring -UpgradeSurfaceCatalystNative -Apply.
La cause précise de l'audit VFX01_4A (#44) sera confirmée par les tests UE ;
il n'est pas correct de présumer qu'elle provient uniquement du catalyseur.

## Validation et prochaines étapes

1. Compilation puis suite complète UE5.5.4/LocalAuto sur le commit code.
2. Si les échecs sont exactement ceux autorisés, migration Unreal native.
3. Validation intégrale sur le nouveau SHA contenant le seul DataAsset.
4. ARMEMENT01.17.3D.3 : retrait du pont de normalisation C++ uniquement
   APRÈS preuve de succès du DataAsset natif, puis nouvelle validation.
5. Rendu PIE/GPU et packaging indépendants de -NullRHI.

Aucun asset artiste (mesh, icône, PBR) n'est créé, supprimé ou remplacé.

## ARMEMENT01.17.3D.3 — Suppression du pont C++ (5 novembre 2026)

- Preuve de précondition : commit b4d1c92a, validation Windows UE5.5.4/LocalAuto SUCCESS.
- Suppression de ProjectApprovedAction dans FGridCatalystAreaResolver et de
  l'appel artificiel depuis UGridItemDefinitionAsset::BuildQuickItemCombatActionDefinition.
- Les actions QuickItem utilisent désormais les champs autoritaires du DataAsset,
  sans exception codée en dur pour le Mage/Tisseur de surfaces.
- Le test SquareArea formule explicitement son contrat C6 sans passage par
  une fonction de conversion transitoire.
- Nouveau test NativeActionWithoutBridge : compare l'action résolue aux
  propriétés brutes (shape, radius, ciblage, exploration, PA, LOS et coût item).
- Aucun uasset, umap, mesh, icône, matériau, traduction ou SaveGame modifié.
- Exiger à nouveau UE5.5.4/LocalAuto=SUCCESS sur le SHA de 17.3D.3.

## ARMEMENT01.17.3D.4 — Clôture des générateurs hérités

Préconditions démontrées : validations locales UE5.5.4 SUCCESS sur
b4d1c92a (DataAsset natif) et 20d013ab (suppression du pont C++).
Les anciens scripts de création des cinq assets procéduraux deviennent
inutiles et dangereux pour les assets approuvés par l'artiste :
AuthorSurfaceCatalystVisuals.ps1 et FinalizeSurfaceCatalystVisuals.ps1
sont retirés, ainsi que la commande SurfaceCatalystVisuals et son moteur
de peinture/duplication procédurale.

Le workflow de migration native 17.3D (script temporaire,
commande UpgradeSurfaceCatalystNative et autorisation d'exécution dans
l'agent Windows) est également retiré après succès : de futurs edits du
DataAsset ne doivent jamais être réécrits automatiquement par une recette
ponctuelle. Le code garde seulement MatchesSerializedProduction pour les
tests permanents, et MatchesProductionAssets pour auditer l'art.

Aucun asset binaire n'est supprimé, ni M_SurfaceCatalyst_Flask existant :
une éventuelle suppression d'un matériau de production exigerait un audit
des dépendances Unreal avant intervention.

Validation demandée sur le commit 17.3D.4 : UE5.5.4/LocalAuto full Grimrock.
