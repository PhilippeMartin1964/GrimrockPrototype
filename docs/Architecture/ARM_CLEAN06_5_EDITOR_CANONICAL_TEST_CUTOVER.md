# ARM-CLEAN06.5 — Regroupement des tests Editor sur la seule autorité canonique

## Périmètre

Neuf sources de tests `Source/GrimrockPrototypeEditor/Private/Tests/`
ne chargent plus les anciens `DA_Weapon_*` / `DA_Item_*`
pour des tests de placement, de VFX et de persistance qui sont déjà
produits par le registre unique `UGridItemSubsystem`.

- `GridEditorARMEMENT0171PalettePlacementTests.cpp`: palette et
  trois armes pilotes, placement, rendu 3D et transform sur N1.
- `GridEditorARMEMENTLOT021322ArrowPaletteTests.cpp`: une entrée
  flèche N0, quantité, placement et non-mutation de `DA_ItemSystem`.
- `GridEditorARMEMENT0181PlacedItemLevelTests.cpp`: niveaux authored,
  collecte, drop sans pointeur source et restauration N1–N20.
- `GridEditorARMEMENT0182ItemSpawnLevelTests.cpp`: spawn, pickup,
  non-duplication et Save/Continue des niveaux d'items.
- `GridEditorItemVFX012AuthoringTests.cpp`: LiquidSparkle sur la potion
  N0 canonique et absence sur les flasques N0.
- `GridEditorItemVFX013AuthoringTests.cpp`: torch N0, PointLight,
  Niagara TorchFlame, activation et transform.
- `GridEditorItemVFX014ProductionPolicyTests.cpp`: tous les 48 ItemIds
  canoniques et unicité LiquidSparkle; anciennes propriétés retirées.
- `GridEditorEffectsOrg012DTorchPreflightTests.cpp`: vérification
  du Niagara TorchFlame par la projection canonique, tout en conservant
  le graphe des matériaux Niagara dans l'AssetRegistry.
- `GridEditorEffectsOrg011InventoryTests.cpp`: dépendances Niagara
  de la torche et de la potion via `VisualEffects` canonique plutôt
  que les deux anciennes définitions.

Les tests structuraux qui instancient des `UGridItemDefinitionAsset`
**transients** restent des tests Editor et ne sont pas un fallback de
production. Tous les tests conservent leur nom Automation existant,
afin de ne pas masquer l'échec d'une ancienne suite par un renommage.

## Conditions et protections

Lot C++ et documentation uniquement. Aucune modification des
`.uasset`, `.umap`, `.sav`, palettes native, textes UI ou assets
d'effets. Une validation du SHA exact par `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto` est requise avant tout commit supplémentaire.

Ce lot ne migre pas les anciens importeurs ou commandlets d'authoring
ni toutes les anciennes caractérisations de contenu Editor ; les
scripts et catalogues sources historiques restent à auditer.
Le rapport H.23 garde `deletion_authorized=false`. Après conversion
de tous les consommateurs réellement exécutables, régénérer la preuve
native sur le SHA candidat et vérifier 7/7 SaveGames, PIE,
Cook, Shipping, Save/Continue. Deux Readables sauvegardés sont
strictement préservés.
