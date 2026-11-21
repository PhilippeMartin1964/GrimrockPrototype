# ARM-CLEAN06.5H.25 — Test Shuriken autonome du DA_Weapon_Shuriken historique

`Grimrock.Items.ITEM_THROW_CLEAN01` chargeait explicitement `/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Shuriken` et exigeait l'ancienne définition `UGridItemDefinitionAsset` en production. Le test vérifie désormais le **Shuriken physique N0, non-ARM** par `UGridItemSubsystem::ResolveItem("Shuriken",0)` en utilisant la seule racine `CanonicalStage/DA_ItemSystem` : identité non-ARM, `GenericThrow.bCombatRecoverable` explicite et vitesse de lancer positive. Aucun chargement du vieux `.uasset` du Shuriken.

**Rectificatif H.25.1 :** le Shuriken figure parmi les 26 identités non-ARM N0, et non parmi les 22 ARM N1–N20. Le premier test le résolvait au mauvais niveau : l'agent Windows a correctement refusé cette hypothèse. La politique physique est dans `UGridNonArmamentDefinitionAsset::GenericThrow`, projetée depuis la définition source ; la catégorie canonique de Shuriken ne doit jamais être déduite de l'emplacement de son ancien `.uasset` dans le dossier Weapons.

La partie du test qui caractérise le **comportement de l'ancienne classe C++** sur un objet purement transitoire reste en place temporairement. Elle doit être retirée lors du jalon de suppression de cette classe, pas remplacée par un fallback de production.

Aucun `.uasset`, `.umap`, `.sav`, manifeste d'armes, UI ou code gameplay modifié. Le nombre de packages du périmètre natif H.23 demeure 53 tant qu'Unreal Editor n'a pas autorisé un retrait ciblé. La validation exigée est `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur SHA exact, y compris compilation Editor et Shipping.
