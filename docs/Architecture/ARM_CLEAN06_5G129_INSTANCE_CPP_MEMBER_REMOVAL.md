# ARM-CLEAN06.5G.129 — Suppression des deux anciens membres C++ d'instance

**Parent :** `bbb11f375811f682e4810665004d6d5f98f0e3bd` (5G.128, validé SilentTask/LocalAuto SUCCESS sur son SHA).

## Changement de schéma C++ seulement

Le type `FGridItemInstance` perd désormais **physiquement** ses deux membres C++ non réfléchis `FText DisplayName` et `float Weight`. Depuis 5G.93, ils ne sont plus `UPROPERTY` et ne sont ni visibles au Blueprint ni sérialisés par les propriétés UE. Leur retrait C++ ne change donc pas le contrat réfléchi du SaveGame. Les identifiants, quantités, niveaux d'objet, dette d'armure, propriétaire, état d'éclairage, contenu lisible et position restent en place.

La dernière copie de `Stone.DisplayName` relevée dans `GridHotbarContext01Tests.cpp` est également supprimée. Les noms et poids des **définitions** (`UGridItemDefinitionAsset` et `UGridItemSystemAsset`) restent inchangés : ils ne constituent pas des caches d'instance. Les structures qui portent légitimement `DisplayName` ou `Weight` (personnages, profils résolus, résumés et vues de présentation) ne sont pas modifiées.

## Garanties et limites

- Une tentative d'accès C++ à `FGridItemInstance::DisplayName/Weight` doit désormais échouer à la compilation, ce qui étend la protection au-delà des tests de non-réflexion existants de `CanonicalInstanceMaterialization`.
- Les contrats de nom, poids, nombre de pierres et lien de raccourci demeurent testés par `Grimrock.UI.HotbarContext01` et la suite globale `Grimrock`.
- Pas de changement de `.uasset`, `.umap`, WBP, localisation, fichiers de sauvegarde personnels ou conversion binaire. Pas de lecteur de compatibilité réintroduit.
- La validation est conditionnée à `UE5.5.4/SilentTask=success` et `UE5.5.4/LocalAuto=success` sur le SHA publié. Les effets PIE et le chargement visuel réel ne sont pas prouvés par NullRHI ; l'étape 06.6 de recette intégrale reste distincte.
