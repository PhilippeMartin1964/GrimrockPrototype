# Notes d’audit du système d’objets

Statut : **mis à jour après WORLDOBJ-MIG10, ALIGN-A et ALIGN-B5.3 — 2026-09-09**.

Ce document conserve les conclusions utiles de l’audit initial, mais les anciens noms d’archétypes et helpers ne doivent plus être lus comme le contrat courant. Le modèle actif est documenté dans `docs/Design/02_OBJECT_ARCHETYPES.md` et `docs/Architecture/WORLDOBJ_MIG10_FINAL.md`.

## Résolutions apportées par ALIGN-A

- les helpers de définition morts `RequiresEdgePlacement()`, `SupportsCenterPlacement()`, `SupportsWallPlacement()`, `AllowsInvisibleRuntimeObject()` et `IsCeilingPlaced()` ont été supprimés en ALIGN-A1 ;
- les helpers de paramètres morts `UsesWallPlacementParams()`, `UsesCenterPlacementParams()`, `UsesReadableParams()`, `UsesItemParams()`, `UsesTriggerParams()`, `UsesMovingMeshParams()`, `UsesFixedMeshParams()` et `UsesRuntimeActorClass()` ont été supprimés en ALIGN-A2 ;
- la synchronisation preview ne complète plus indéfiniment une liste sérialisée : ALIGN-A4 reconstruit `WorldObjectDefinitions` à partir de la palette canonique et remplace la projection si nécessaire ;
- `L_GrimrockEditor` a été resauvegardée en ALIGN-A5 pour retirer les références sérialisées résiduelles ;
- six anciens world-object pickups ont été supprimés en ALIGN-A6 après audit AssetRegistry et validation runtime : BlueGem, CopperKey, IronKey, Shuriken, Stone et TestNote ;
- `DA_MonsterSpawn`, `DA_Archetype_CustomRecruiter_Service` et `DA_Archetype_StoryCompanion_Recruit` restent volontairement présents et référencés.

## Résolutions apportées par ALIGN-B5

- ALIGN-B5.1 a fait consommer directement `PlacementSurface + DefaultLocalPosition.U/V/N` par le resolver runtime, à comportement identique ;
- ALIGN-B5.2 a migré les lecteurs éditeur et le dernier lecteur runtime vers ce même contrat permanent ;
- ALIGN-B5.3 a supprimé physiquement les anciennes projections de placement `PlacementKind`, `PlacementZOffset`, `WallInset`, `LocalOffsetAlongWall`, `LocalOffsetVertical` ainsi que `RefreshPlacementRuntimeProjection()`, `IsEdgePlaced()`, `IsCenterPlaced()` et `IsWallPlaced()` ;
- `PostLoad()` reste conservé parce qu’il porte encore la migration audio legacy des portes ; seul le hook `PostEditChangeProperty()` qui ne servait qu’à la projection de placement a été supprimé.

## Contrat courant

Les objets structurels et mécanismes utilisent `UGridWorldObjectDefinitionAsset`. Les collectibles utilisent directement `UGridItemDefinitionAsset` via `FGridObjectPaletteEntry::DefaultItemDefinition` et `FGridLooseItemInstance::ItemDefinition`.

Le placement world-object possède désormais une autorité unique :

```text
PlacementSurface
DefaultLocalPosition.U / V / N
```

`Floor`, `Wall` et `Ceiling` sont les seules surfaces d’authoring valides. Pour les placements muraux, la face concrète reste portée par l’instance via `WallSide` / `EGridEdge`.

Il n’existe plus de projection parallèle de placement dans `UGridWorldObjectDefinitionAsset`. Le runtime et l’éditeur consomment directement le contrat ci-dessus.

## Points encore volontairement conservés

- la preview reste plus simple que certains acteurs runtime composites ;
- modifier une identité de définition placée n’implique pas une migration automatique des données déjà sérialisées ;
- plusieurs fonctions `BlueprintCallable` restent conservées tant qu’un audit Blueprint n’a pas prouvé qu’elles sont inutilisées ;
- les anciens tableaux/paramètres audio de porte restent présents comme migration audio distincte ;
- `ItemActorClass` reste conservé ;
- `FGridWorldObjectInstance::Type` reste conservé ;
- `WorldObjectDefinitions` reste le registre runtime des définitions world-object ; son éventuelle simplification relève d’un chantier distinct ;
- `bCanShareCell` et `bCanShareAnchor` restent présents comme bridges de partage distincts du placement ;
- `Floor` et `Ceiling` exposent `U/V` dans le modèle de coordonnées, mais ces composantes ne sont pas encore consommées par les transforms runtime centrées ;
- le plan plafond courant reste à 200 cm dans les chemins de résolution existants ;
- les semantics de frontières et de murs directionnels ne sont pas modifiées par ALIGN-B5.

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
