# ARM-CLEAN06.4X.16.10 — Restauration non-ARM canonique du monde

Base : `7886a9b757b639f87d6ed8202e9d34e0d19c1a64`, double validation Windows SUCCESS ; les serrures ont été validées en PIE.

## Correctif effectif

`AGridLevelRuntimeActor::ApplyCurrentLevelRuntimeState()` restaure désormais une définition **non-ARM connue du registre canonique** avec `InitializeFromCanonicalItem` pour les objets libres et les objets de réceptacle, récupérant mesh, configuration lumineuse et effets Niagara. Le simple `ApplyCanonicalWorldVisual` ne pouvait auparavant restaurer que le mesh. Il n'est pas permis de reprendre un ancien `UGridItemDefinitionAsset` pour ces identités. Les ARM conservent leur voie dédiée jusqu'à migration complète des présentations ; les IDs non inscrits au registre conservent la logique historique. Un ID canonique connu mais invalide à son niveau physique est rejeté sans passage par un pointeur legacy.

## Preuves de non-régression

Deux tests `Grimrock.RPG.ARM.CLEAN06_4X1610` : rechargement réel d'un niveau transitoire via `SaveGameToMemory / LoadGameFromMemory`, torches N0 allumée et éteinte au sol, puis torche N0 dans un réceptacle. Vérification des GUID, ItemId, lumière, configuration PointLight et Niagara, absence de pointeur historique et permissions du réceptacle.

Aucune écriture de .sav, .uasset, .umap, WBP ou SavePackage. Aucun changement de schéma SaveGame v27. Les tests `-NullRHI` ne prouvent pas encore PIE/Chaos/Cook/Shipping.

## Validation et suite

Exiger `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur le SHA exact de 16.10. Ensuite : autres lecteurs historiques, vérification sur copies privées de SaveGame v27, ARM visuels, puis nettoyage contrôlé 06.5 et recette complète 06.6 ; jamais de suppression aveugle des champs legacy.
