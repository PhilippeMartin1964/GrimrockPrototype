# Notes d’audit du système d’objets

Statut : **mis à jour après WORLDOBJ-MIG10 et ALIGN-A — 2026-09-09**.

Ce document conserve les conclusions utiles de l’audit initial, mais les anciens noms d’archétypes et helpers ne doivent plus être lus comme le contrat courant. Le modèle actif est documenté dans `docs/Design/02_OBJECT_ARCHETYPES.md` et `docs/Architecture/WORLDOBJ_MIG10_FINAL.md`.

## Résolutions apportées par ALIGN-A

- les helpers de définition morts `RequiresEdgePlacement()`, `SupportsCenterPlacement()`, `SupportsWallPlacement()`, `AllowsInvisibleRuntimeObject()` et `IsCeilingPlaced()` ont été supprimés en ALIGN-A1 ;
- les helpers de paramètres morts `UsesWallPlacementParams()`, `UsesCenterPlacementParams()`, `UsesReadableParams()`, `UsesItemParams()`, `UsesTriggerParams()`, `UsesMovingMeshParams()`, `UsesFixedMeshParams()` et `UsesRuntimeActorClass()` ont été supprimés en ALIGN-A2 ;
- la synchronisation preview ne complète plus indéfiniment une liste sérialisée : ALIGN-A4 reconstruit `WorldObjectDefinitions` à partir de la palette canonique et remplace la projection si nécessaire ;
- `L_GrimrockEditor` a été resauvegardée en ALIGN-A5 pour retirer les références sérialisées résiduelles ;
- six anciens world-object pickups ont été supprimés en ALIGN-A6 après audit AssetRegistry et validation runtime : BlueGem, CopperKey, IronKey, Shuriken, Stone et TestNote ;
- `DA_MonsterSpawn`, `DA_Archetype_CustomRecruiter_Service` et `DA_Archetype_StoryCompanion_Recruit` restent volontairement présents et référencés.

## Contrat courant

Les objets structurels et mécanismes utilisent `UGridWorldObjectDefinitionAsset`. Les collectibles utilisent directement `UGridItemDefinitionAsset` via `FGridObjectPaletteEntry::DefaultItemDefinition` et `FGridLooseItemInstance::ItemDefinition`.

Le placement d’authoring est défini par :

```text
PlacementSurface
DefaultLocalPosition.U / V / N
```

Les champs `PlacementKind`, `PlacementZOffset`, `WallInset`, `LocalOffsetAlongWall` et `LocalOffsetVertical` restent des projections `Transient` internes. `RefreshPlacementRuntimeProjection()`, `IsCenterPlaced()` et `IsEdgePlaced()` restent également conservés parce que des consommateurs existants les utilisent encore ; ALIGN-A ne les traite pas comme code mort.

## Points encore volontairement conservés

- `IsRuntimeSpawnableObject()` et ses fallbacks non liés directement au nettoyage A1/A2 n’ont pas été modifiés sans audit dédié ;
- la preview reste plus simple que certains acteurs runtime composites ;
- modifier une identité de définition placée n’implique pas une migration automatique des données déjà sérialisées ;
- plusieurs fonctions `BlueprintCallable` restent conservées tant qu’un audit Blueprint n’a pas prouvé qu’elles sont inutilisées ;
- les anciens tableaux/paramètres audio de porte restent présents comme migration audio distincte ;
- `ItemActorClass` reste conservé ;
- `FGridWorldObjectInstance::Type` reste conservé ;
- les ponts de projection placement restent conservés tant que les consommateurs de transforms ne sont pas tous rabattus sur `PlacementSurface` et U/V/N.

## Politique pour les assets

Un nom ancien ou suspect ne suffit jamais pour supprimer un `.uasset`.

La procédure retenue par ALIGN-A est :

1. vérifier l’existence via AssetRegistry ;
2. lister les referencers on-disk ;
3. vérifier les usages code, chaînes, soft paths et chargements dynamiques ;
4. nettoyer d’abord les références sérialisées légitimes ;
5. supprimer via Unreal Editor ;
6. valider le runtime et les transferts d’items ;
7. seulement ensuite committer la suppression.

Le test `Grimrock.Editor.ALIGN_A3.AssetReferenceAudit` encode désormais l’invariant attendu : les six anciens pickup definitions doivent rester absents, tandis que les trois définitions conservées doivent rester présentes.

## Références

- `docs/Design/02_OBJECT_ARCHETYPES.md`
- `docs/Design/11_GRID_WORLD_OBJECT_DEFINITION_PARAMETERS_REFERENCE.md`
- `docs/Design/ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md`
- `docs/Architecture/WORLDOBJ_MIG10_FINAL.md`
- `docs/Architecture/ALIGN_A_DEAD_CODE_AND_ASSET_HYGIENE_REPORT.md`
