# Définitions des objets du monde et objets placés

Statut : **contrat actif après WORLDOBJ-MIG10**, 2026-09-09.

## 1. Autorités

Une `UGridWorldObjectDefinitionAsset` définit un concept permanent. Son `DefinitionId` est résolu depuis le `WorldObjectDefinitionId` d'un placement. La définition porte les visuels, la motion, le placement par défaut, l'audio, la classe runtime et les règles comportementales partagées.

Le niveau conserve uniquement les collections de placements typées :

| Collection de `UGridLevelAsset` | Structure | Référence de définition |
|---|---|---|
| `WorldObjectInstances` | `FGridWorldObjectInstance` | `WorldObjectDefinitionId` |
| `LooseItemInstances` | `FGridLooseItemInstance` | `ItemDefinition` directe |
| `MonsterSpawns` | `FGridMonsterSpawnInstance` | `MonsterDefinition` directe |
| `ItemSpawns` | `FGridItemSpawnInstance` | `ItemDefinition` directe |
| `LogicObjects` | `FGridLogicObjectInstance` | Configuration logique typée |

Les items ramassables utilisent une seule `UGridItemDefinitionAsset`. Les monstres utilisent une seule `UGridMonsterDefinitionAsset`. Un item déjà présent et un générateur d'items restent deux sortes de placements distinctes.

## 2. Instance world-object

`FGridWorldObjectInstance`, déclaré dans `Core/GridLevelPlacementTypes.h`, contient :

- `InstanceId`, identité stable ; `WorldObjectDefinitionId`, référence de définition ; `Type`, discriminateur fonctionnel ;
- `CellX`, `CellY`, `WallSide`, et l'override optionnel `bHasLocalTransformOverride` / `LocalTransformOverride` ;
- `bInitiallyEnabled`, `bInitiallyActive`, `LogicId`, `Tag`, `Notes`, `PaletteEntryId` et `ReadableTextOverride` ;
- `InstanceConfig`, configuration locale strictement limitée à `Teleporter`, `Transition`, `Pit`, `ReceptacleInitialContent` et `bStartsUnlocked`.

La géométrie d'animation et les règles permanentes ne sont pas recopiées dans cette instance. Le comportement effectif résout `Definition.DefaultBehavior` avec les seules données locales autorisées. Voir la [règle Definition / Instance](../Design/12_GRID_OBJECT_INSTANCE_BEHAVIOR_RULE.md).

## 3. Palette et édition

`UGridObjectPaletteAsset::Entries` expose des `FGridObjectPaletteEntry`. Une entrée world-object référence `DefaultWorldObjectDefinition` ; une entrée collectible référence directement `DefaultItemDefinition`. Le libellé, la catégorie et l'icône de palette relèvent de la présentation de l'éditeur.

`AGridLevelEditorActor::FindWorldObjectDefinitionById()` résout une définition depuis la palette. `FGridObjectPaletteEntry::GetEffectiveWorldObjectDefinitionId()` fournit l'identifiant effectif d'une entrée world-object. `PaletteEntryId` conserve une provenance d'authoring, sans remplacer l'identité de placement ni la référence de définition.

`AGridLevelEditorActor` édite les collections natives de `UGridLevelAsset`. `ValidateCurrentLevel()` contrôle les identités, placements, références, liens, définitions et règles spécifiques. Les messages ne constituent pas une migration ou une resauvegarde implicite.

## 4. Preview et runtime

La palette alimente `AGridLevelRuntimeActor::WorldObjectDefinitions`. `FindWorldObjectDefinition()` résout le `WorldObjectDefinitionId`. Preview et runtime consomment cette même définition et les placements typés ; ils partagent `GridPlacementTransformResolver`.

`UGridEditorPreviewComponent` initialise les objets de preview depuis la définition. Les items résolvent leur `WorldMesh` depuis `ItemDefinition`, et les monstres leur présentation depuis `MonsterDefinition`. Un aperçu ne certifie pas le fonctionnement interactif du niveau.

`FGridRuntimeWorldObjectData` est une frontière C++ non réfléchie d'initialisation runtime spécialisée. Elle est construite depuis `FGridWorldObjectInstance` et n'est pas stockée dans le LevelAsset. Les acteurs runtime et leurs caches sont transitoires.

## 5. Persistance

Le SaveGame conserve les états mutables et les identités/références nécessaires à leur restauration : portes, mécanismes, présence, items, réceptacles, monstres et logique. Il ne copie pas les définitions permanentes ni leur composition visuelle. « Deltas mutables » n'interdit pas les identifiants nécessaires pour retrouver ces définitions.

Les anciennes collections monolithiques et projections de compatibilité LevelAsset ont été supprimées en MIG09. MIG10 ne fournit aucun alias ou wrapper C++ de l'ancien nom.

## 6. Noms Unreal et chemins historiques

Le renommage historique `UGridObjectArchetypeAsset` → `UGridWorldObjectDefinitionAsset` est achevé. Les Core Redirects de `Config/DefaultEngine.ini` restent temporaires mais nécessaires pendant la période où des Blueprints/assets externes éventuels peuvent encore contenir l'ancien nom. Ils sont détaillés dans le [rapport de clôture](WORLDOBJ_MIG10_FINAL.md).

`GridObjectArchetypeAsset` peut subsister dans certains chemins de packages historiques, notamment `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/`. Ce chemin ne représente plus une classe ni un concept architectural actif. Les assets concernés sont des `UGridWorldObjectDefinitionAsset`.

Une éventuelle réorganisation Content nécessitera une tâche dédiée avec AssetTools ; MIG10-C ne déplace aucun package.

## 7. Références

- [Paramètres de définition](../Design/11_GRID_WORLD_OBJECT_DEFINITION_PARAMETERS_REFERENCE.md)
- [Roadmap et invariants](WORLDOBJ_MIGRATION_ROADMAP_AND_TARGET_DATA_MODEL.md)
- [Audio des objets](../Design/GRID_OBJECT_AUDIO_SYSTEM.md)
- [Réceptacles](RECEPTACLE_SYSTEM_FOUNDATION.md)
- [Objets lisibles](READABLE_OBJECTS_AND_FEEDBACK_FOUNDATION.md)
