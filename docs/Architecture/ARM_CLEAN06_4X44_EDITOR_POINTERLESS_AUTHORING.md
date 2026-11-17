# ARM-CLEAN06.4X.44 — Le Level Editor cesse d'écrire les anciennes définitions des objets canoniques

**Parent :** `53dc881d2be1559fe3154bae7a86b0e0f26867d7`. **Nature :** cutover d'authoring C++ Editor et tests
sur objets transitoires uniquement ; aucun `.uasset`, `.umap` ni SaveGame modifié.

## Preuve préalable

L'audit X.19 comptait 95 références monde/palette possédant déjà
un `CanonicalItemId` et un niveau physique compatible, dont 44
objets libres, 18 contenus de réceptacles et 33 éléments de palette.
Le runtime X.39/X.40 construit déjà les objets libres connus sans
`DA_Weapon_*`. En revanche, la sélection d'une entrée de palette
encore dotée d'un `DefaultItemDefinition` recopiait cette ancienne
référence dans le scratch editor, puis `PlaceSelectedObject`
et `ApplyEditedSelectedObject` l'enregistraient à nouveau.

## Modifications

- `ApplyPaletteEntry` valide toujours l'ID et le niveau
  canoniques, mais **ne stage plus** `DefaultItemDefinition` pour
  une entrée portant `CanonicalItemId`.
- `PlaceSelectedObject` crée les nouveaux `FGridLooseItemInstance`
  canoniques avec `ItemDefinition=nullptr`, en conservant ID,
  `ItemLevel`, GUID, cellule, orientation, texte et propriétés.
- `ApplyEditedSelectedObject` purge le vieux pointeur **seulement**
  sur l'entrée explicitement modifiée par l'utilisateur ; la même
  identité préserve son niveau N1–N20 existant. Aucun balayage des
  anciennes lignes du niveau n'est effectué.
- `SetSelectedObjectItemDefinitionAsset` refuse désormais
  d'attacher un ancien UObject **même de même ItemId** à un objet
  dont `CanonicalItemId` est déjà persisté. `nullptr` reste autorisé.

Les anciennes entrées hors registre demeurent sur leur authoring
historique. Les mismatches d'ID, N0 ARM, N1 non-ARM et registre
absent restent refusés. Les versions binaires de la palette et
des quatre niveaux ne sont pas modifiées dans ce lot.

## Régression Windows initiale du même lot

Le premier SHA X.44 a compilé, mais cinq tests existants
supposaient encore que les **nouveaux** placements enregistrent
la référence ancienne `ItemDefinition` :
`ITEMSTACK01.MultipleCollectiblesSameCell`,
`CLEAN06_4X12.CanonicalPalettePlacementAndEdit`,
`LOT02.ArrowPaletteDirectCollectible`,
`ARMEMENT01.10.3.TransientEditorPlacement`, et
`ARMEMENT01.7.Palette.PlaceAndPreviewRealItems`.
Les cinq fixtures ont été alignées sur le contrat nouveau
**dans ce même lot atomique**, sans repli legacy en production.
Leurs attentes de placement lisent désormais `CanonicalItemId`,
`ItemLevel` et `InstanceId`, et les attentes de mesh/transform
de prévisualisation proviennent de `FGridResolvedItem` au niveau
physique, pas d'un ancien `DA_Weapon_*`. Les tests d'assets
historiques existants (non migrés) ne sont pas modifiés.

## Tests

Extension de
`Grimrock.RPG.ARM.CLEAN06_4X28.PointerlessCanonicalPalettePlacement` :
création d'une entrée d'arme canonique accompagnée d'un vieil asset
cohérent, sélection, nouveau placement pointerless N20, puis édition
explicite d'une ligne N12 préexistante avec pointeur legacy.
Vérification d'ID/GUID/niveau, de la purge locale du pointeur et du
refus d'une réintroduction par le picker. Conservation des scénarios
pointerless, ID contradictoire, ARM N0 et ID non enregistré. Le test
`Grimrock.WorldObjects.MIG05.EditorDirectItemPlacement` couvre le
chemin hors registre, inchangé.

**Statut : 06.4 PARTIELLE ; 06.5 BLOQUÉE.** La prochaine étape
nécessite la conversion UE Editor des packages originaux déjà
recensés, avec sauvegardes de sécurité et preuves de réouverture.
