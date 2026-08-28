# Notes d'audit du système d'objets

## Fichiers relus

- `Source/GrimrockPrototype/Public/Core/GridTypes.h`
- `Source/GrimrockPrototype/Public/Core/GridLevelAsset.h`
- `Source/GrimrockPrototype/Private/Core/GridLevelAsset.cpp`
- `Source/GrimrockPrototype/Public/Core/GridObjectArchetypeAsset.h`
- `Source/GrimrockPrototype/Private/Core/GridObjectArchetypeAsset.cpp`
- `Source/GrimrockPrototype/Public/Core/GridObjectPaletteAsset.h`
- `Source/GrimrockPrototype/Private/Core/GridObjectPaletteAsset.cpp`
- `Source/GrimrockPrototype/Public/Core/GridObjectBehavior.h`
- `Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h`
- `Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp`
- bases, acteurs concrets et composants de preview sous `Source/GrimrockPrototype/Public/Runtime/` et `Private/Runtime/`
- acteur, mode, toolkit et widgets sous `Source/GrimrockPrototypeEditor/`
- documentation existante sous `docs/Architecture/`, `docs/Design/` et `docs/GridObjectArchetypeAsset_Conventions.md`

## Clarifications apportées

- distinction explicite entre archétype partagé, entrée de palette, donnée placée persistante et acteur runtime ;
- inventaire complet des champs de `FGridLevelObjectData` ;
- séparation entre données copiées au placement et données relues dynamiquement depuis l'archétype ;
- confirmation que les objets déjà placés ne sont pas automatiquement resynchronisés après modification d'un archétype ;
- description séparée des chemins preview et runtime ;
- correction de la base d'acteur citée : `AGridRuntimeObjectActor`, aucun `GridObjectActor.h` n'étant présent ;
- documentation de `PaletteEntryId` comme provenance éditeur sans rôle de résolution runtime.

## Validations ajoutées

Dans `AGridLevelEditorActor::ValidateCurrentLevel()` :

- erreur si `ArchetypeId` est vide ;
- erreur si l'archétype n'est pas exposé par la palette assignée ;
- erreur si `FGridLevelObjectData::Type` diffère de `SupportedType` ;
- avertissement si un objet central non-item conserve un bord cardinal ;
- avertissement si `PaletteEntryId` n'existe plus ;
- avertissement si l'entrée de palette pointe désormais vers un autre archétype.

Ces contrôles ne modifient ni les données ni le gameplay.

## Code suspect conservé

- `IsRuntimeSpawnableObject()` contient un fallback par `Type`, alors que le spawn final exige toujours une classe issue d'un archétype ;
- `SpawnRuntimeObjectActor()` exige un mesh même pour les types déclarés invisibles par `AllowsInvisibleRuntimeObject()` ;
- `SyncPreviewRuntimeObjectArchetypesFromPalette()` ajoute les références sans purger les archétypes retirés de la palette ;
- TD07.3.6 a supprimé les miroirs legacy `bPlaceOnEdge` et `bPlaceAtCellCenter`; `PlacementKind` est l'autorité unique ;
- la preview utilise un mesh simplifié et non les véritables acteurs runtime ;
- changer localement `ArchetypeId` ne resynchronise pas les autres copies locales ;
- plusieurs fonctions `BlueprintCallable` semblent internes, mais aucune n'a été retirée sans audit des Blueprints.

## Conservé volontairement

- toutes les API publiques et Blueprint existantes ;
- toutes les branches de gameplay spécialisées ;
- le fallback de compatibilité des items par identifiant ;
- les logs de diagnostic temporaires déjà présents ;
- tous les `.uasset` et `.umap`.

## Points laissés pour plus tard

- décider si les objets invisibles doivent pouvoir être générés sans mesh ;
- définir ou supprimer le fallback runtime par `EGridLevelObjectType` ;
- décider si la synchronisation de palette doit remplacer ou seulement compléter `ObjectArchetypes` ;
- définir une commande explicite de resynchronisation ou migration des objets placés ;
- auditer les usages Blueprint avant toute réduction d'API.
