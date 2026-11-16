# ARM-CLEAN06.4X.12 — Write-through canonique des placements directs de palette

Source validée : ARM-CLEAN06.4X.11 (`56f09ecdaab2104549d4469a6ee11807f69705c4`),
avec `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS.
X.11 vérifie les **33 entrées directes** de la palette réelle
(`CanonicalItemId`, `CanonicalDefaultItemLevel`).

## Portée

Pour une entrée directe `Item` réellement canonique :
- `ApplyPaletteEntry` refuse une contradiction entre l'ancien pointeur
  et le `CanonicalItemId` déjà matérialisé ;
- `PlaceSelectedObject` refuse la source contradictoire **avant**
  `LevelAsset->Modify`, puis écrit à chaque nouvelle
  `FGridLooseItemInstance` le `CanonicalItemId` et son
  **niveau physique par défaut exact** ;
- `ApplyEditedSelectedObject` synchronise l'identité quand la palette
  change, mais conserve le `ItemLevel` personnalisé si l'ItemId reste
  identique. Il ne remplace jamais un ARM N20 par son niveau de palette
  lors d'une simple modification des notes ou du texte.

Le pointeur `ItemDefinition` reste stocké provisoirement dans les
placements pour les lecteurs historiques/SaveGame non encore convertis.
Il n'est **pas** autorisé à renommer une identité canonique persistée.
Les fixtures de palette sans `CanonicalItemId` conservent leur
parcours historique, sans nouveau fallback pour les entrées converties.

## Test

`Grimrock.RPG.ARM.CLEAN06_4X12.CanonicalPalettePlacementAndEdit`
place deux entrées réellement authorées, ARM et non-ARM, dans un
`UGridLevelAsset` transitoire ; vérifie ItemId, GUID, niveau, quantité,
cellule, indépendance des placements, conservation de N20 à l'édition
et rejet d'une entrée contradictoire. La palette et les vrais niveaux
ne sont pas sauvegardés.

**Limite :** les tuiles Slate (texte, icône, recherche) consultent
encore temporairement `DefaultItemDefinition`. Cette présentation
nécessite un cutover spécifique après qualification de parité
libellé/description/icône. Aucun `.uasset`, `.umap`, `.sav`, WBP ni
Blueprint n'est modifié. Compiler et lancer la suite `Grimrock`
UE 5.5.4 sur le commit final ; PIE n'est pas prouvé par `-NullRHI`.
