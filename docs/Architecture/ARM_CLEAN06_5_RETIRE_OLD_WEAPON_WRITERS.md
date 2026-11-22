# ARM-CLEAN06.5 — Retrait des anciens writers de catalogues

Les trois commandlets hérités ne doivent plus construire ni publier
DA_WeaponCatalogue, DA_WeaponVisualCatalogue et les données d'affinité
de classes historiques. La production utilise DA_ItemSystem et les
quatre catalogues canoniques déjà validés.

Sont retirés : GridWeaponCatalogueCompactCommandlet,
GridWeaponVisualFinalizeCommandlet et
GridWeaponClassAffinityImportCommandlet avec leurs en-têtes, ainsi que
CompactARMWeaponCatalogue.ps1, PublishARMCompactCatalogue.ps1,
ValidateARMWeaponCleanup.ps1, FinalizeARMEMENT0163.ps1 et
AuthorARMEMENT01952Affinities.ps1. Ces scripts ne doivent plus
modifier les anciennes autorités en vue d'une nouvelle production.

Le test ProductionVisualAssets du catalyseur teste désormais directement
le N0 Item_SurfaceCatalyst_01 via UGridItemSubsystem et conserve les
contrôles de texture, maillage et matériaux de l'artiste. Le helper
RPGSurfaceCatalystVisualAuthoring (chargement DA_Item_* ancien) est retiré.

Le code de base de la migration et les rapports anciens restent
documentaires. Les scripts toujours utiles au contre-audit natif H.23
et à la validation locale ne sont pas modifiés.

Aucun asset UE5, WBP, SaveGame ou redirector n'est modifié.
Deletions natives : interdites jusqu'au SHA final vérifié, le snapshot
privé des sept sauvegardes, les contrôles AssetRegistry et la recette
PIE, Cook et Shipping.
