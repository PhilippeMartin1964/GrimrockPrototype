# ARM-CLEAN06.5H.33 — Mage SurfaceWeaver et catalyseur canoniques en New Game/SaveContinue

Le scénario `Grimrock.RPG.ARMEMENT01.17.2B.SurfaceWeaverNewGameSaveContinue` ne charge plus l'ancien `DA_Item_SurfaceCatalyst_01` ni n'enregistre `UGridItemDefinitionAsset`. Il utilise une `UWorld` / `UGameInstance` de test, son `UGridItemSubsystem` réel, l'identité `Item_SurfaceCatalyst_01` N0 et son action détaillée `Action_Mage_SurfaceCatalyst_01`.

Les trois orientations Mage sont générées avec `TryPopulateApprovedTransientClass(..., Registry)`, et les recettes inspectées par `CanonicalItemId`. Les assertions maintenues portent sur le bâton élémentaire, la quantité 1, le niveau N0, la possession de l'objet dans le sac, le niveau initial du Mage, l'absence de talent N2 automatique et l'absence de pointeur historique.

La création réelle par `CreateInitialCharacter`, la sérialisation binaire en mémoire et `RestorePartyInventoryState` sur un nouveau Pawn confirment GUID, kit, quantité et propriété sans écrire de sauvegarde joueur. La classe source Mage ne change pas.

Aucun gameplay, Blueprint, VFX, `.uasset`, `.umap`, `.sav` ou registre de production n'est modifié; les 53 packages H.23 ne sont pas supprimés. H.34 ne pourra être publié qu'après `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur ce SHA, y compris Editor, Shipping et `Grimrock.RPG.ARMEMENT01.17.2B`.
