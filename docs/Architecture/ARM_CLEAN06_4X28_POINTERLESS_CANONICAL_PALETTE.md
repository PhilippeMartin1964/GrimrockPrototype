# ARM-CLEAN06.4X.28 — Palette canonique sans pointeur vers ItemDefinition

**17 novembre 2026 — Parent :** `0ca06dfbd351b364830e233762bf843ee87786ba`. Périmètre : C++ runtime/core + C++ Editor + test transitoire + documentation. Aucun `.uasset`, `.umap`, `WBP`, `SaveGame` ou .ini écrit.

## Objectif

Les étapes X.25–X.27 permettent de visualiser, valider, sélectionner et modifier un loose item par `CanonicalItemId` sans `ItemDefinition`. La palette restait cependant bloquée : `FGridObjectPaletteEntry::IsDirectItemEntry()` ne reconnaissait que l'ancien pointeur, `ApplyPaletteEntry` le demandait et les gardes de `PlaceSelectedObject` / `ApplyEditedSelectedObject` refusaient systématiquement une source canonique sans `DefaultItemDefinition`.

## Contrat de données

- Une entrée de palette est un *collectible direct* lorsqu'elle porte soit un `CanonicalItemId` explicite, soit un `DefaultItemDefinition` historique. Les champs `CanonicalItemId` et `CanonicalDefaultItemLevel` sont éditables sur une entrée de palette **sans réécrire les entrées réelles existantes**.
- Les accès génériques `GetEffectiveObjectType`, `GetEffectiveDisplayName` et `IsValidEntry` néréférencent plus un ancien asset absent. L'asset de palette vérifie seulement la structure (définition d'objet/collectible mutuellement exclusive, absence d'icône locale copiée, plage N0–N20 et cohérence d'un ancien pointeur optionnel). Le **registre** reste l'unique autorité sémantique des ItemIds et niveaux ; aucun second catalogue de palette n'est introduit.
- `FGridEditorCanonicalPaletteReadModel` continue de fournir nom, description et icône depuis `DA_ItemSystem`. Une entrée portant un ID canonique mais invalide échoue sans fallback historique.
- L'éditeur peut maintenant sélectionner et placer une entrée `CanonicalItemId` + `CanonicalDefaultItemLevel` sans vieux pointeur : `PlaceSelectedObject` écrit directement les deux champs sur la nouvelle `FGridLooseItemInstance`. La garde est appliquée **avant** modification de la grille et compare l'identité, la politique physique et tout pointeur de staging. Une entrée non enregistrée, ARM N0, non-ARM N1 ou contradictoire est rejetée.
- `ApplyEditedSelectedObject` reconnaît le même chemin. Il ne réinitialise pas un niveau N1–N20 modifié sur une instance dont l'ItemId est resté identique. GUID, quantité et coordonnées restent propriété du `LevelAsset`.
- Les anciennes entrées sans `CanonicalItemId` et les fixtures historiques de placement conservent leur comportement tant que les fichiers natifs n'ont pas été migrés.

## Tests

`Grimrock.RPG.ARM.CLEAN06_4X28.PointerlessCanonicalPalettePlacement` : un monde `EditorPreview`, un `LevelAsset` et une palette **transitoires**, avec les vrais objets `Weapon_Dagger_01` N20 et `Item_Arrows_01` N0 et sans ancien `DefaultItemDefinition`. Vérification de la classification palette, validation structurelle, projection de l'icône, sélection, deux placements distincts avec GUID, niveau physique, modification du niveau vers N12 sans reset, puis refus des entrées ARM N0, ID inconnu et ancien pointeur contradictoire. Les tests X.11–X.14 de palette réelle restent obligatoires.

## Suites

Ceci ne migre pas les **33 références de palette réelles**, ni les **44 loose + 18 contenus de réceptacles**, ni les visual profiles/physiques natifs des 22 ARM. Ces binaires doivent être réécrits de façon atomique par les outils UE, preuves visuelles + tests indépendants, puis les anciens pointeurs pourront être supprimés. Les kits C++ hors-registre et les quatre loots réels hors registre restent des dépendances de migration. **ARM-CLEAN06.4 partielle ; 06.5 BLOQUÉE.**
