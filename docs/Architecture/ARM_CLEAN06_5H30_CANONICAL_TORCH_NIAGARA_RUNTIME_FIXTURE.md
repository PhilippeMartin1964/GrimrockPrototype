# ARM-CLEAN06.5H.30 — Niagara torch runtime sans ancien item

## Objet

La fixture `Grimrock.Items.VFX01_3.TorchRuntimeAndHeldVisual` ne charge plus `DA_Item_Torch` et ne fait plus appel à `AGridItemActor::InitializeFromItemDefinition`. Elle construit un objet physique N0 `Item_Torch`, le résout par le vrai `UGridItemSubsystem` de sa `UGameInstance` de test et initialise son acteur avec `InitializeFromCanonicalItem`.

Le test vérifie explicitement l'absence de pointeur historique sur l'acteur du monde. Son état initial éteint, l'allumage de la flamme Niagara et du `PointLight`, leur extinction et réallumage sans duplication, le retrait/replacement, la torche tenue et l'indépendance entre deux instances restent contrôlés sans changement.

## Contrat de non-régression

Aucun `.uasset`, `.umap`, `.sav`, matériau Niagara, visuel, ciblage ou code gameplay n'est modifié. La racine `CanonicalStage/DA_ItemSystem` reste l'autorité unique pour lumière et VFX des objets N0; les 53 packages historiques H.23 ne sont pas effacés.

Ne pas publier H.31 avant `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur ce SHA, incluant compilation Development Editor/Shipping et `Grimrock.Items.VFX01_3.TorchRuntimeAndHeldVisual`, ainsi que les régressions d'illumination. L'audit natif et les sept sauvegardes restent nécessaires avant toute opération destructrice.
