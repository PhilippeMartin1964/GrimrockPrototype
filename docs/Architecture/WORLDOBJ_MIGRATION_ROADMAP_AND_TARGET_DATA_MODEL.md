# WORLDOBJ — Roadmap MIG00 à MIG10 et modèle de données cible

Statut : document directeur de migration — mise à jour 2026-09-07.

> Référence architecturale prioritaire : `docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md`.
>
> La mind map décrit la cible ; ce document décrit la trajectoire. En cas de contradiction avec une note historique, la cible et les décisions de migration les plus récentes prévalent.

## 1. Invariants

```text
une définition = un concept permanent
une instance = placement + configuration locale minimale
le runtime = exécution + état mutable
le SaveGame = deltas mutables uniquement
```

En particulier :

- un item ramassable possède une seule `UGridItemDefinitionAsset` ;
- un objet du monde possède une seule définition permanente ;
- un monstre possède une seule `UGridMonsterDefinitionAsset` ;
- le niveau référence les définitions, il ne les duplique pas ;
- preview et runtime consomment les mêmes données ;
- `UGridLevelAsset` reste la source de vérité spatiale ;
- le SaveGame ne copie pas les données permanentes de définition.

## 2. Modèle cible

```text
GLOBAL DEFINITIONS
├── WorldObjectDefinition
├── ItemDefinition
├── MonsterDefinition
├── EnvironmentDefinition
├── ReadableContent
└── QuestDefinition
          │
          ▼
UGridLevelAsset
├── Cells
├── WorldObjectInstances
├── LooseItemInstances
├── MonsterSpawns
├── ItemSpawns
├── LogicObjects
├── Links / Lua / variables / quêtes
└── départ / transitions
          │
          ▼
Runtime Actors / Components
          │
          ▼
Runtime State / SaveGame
```

## 3. État des jalons

| Jalon | Statut | Résultat |
|---|---|---|
| MIG00 | ✅ | Caractérisation du contrat historique. |
| MIG01 | ✅ | Placement `Floor / Wall / Ceiling` + `U/V/N`. |
| MIG02 | ✅ | Spatial/boundary simplifié. |
| MIG03 | ✅ | `StaticPart` + `MovingParts`. |
| MIG04 | ✅ | Motion générique. |
| MIG05 | ✅ | Collectible direct : `UGridItemDefinitionAsset`. |
| MIG06 | ✅ | Definition + overrides strictement instance-owned. |
| MIG07 | ✅ | Collections de placements typées. |
| MIG08 | ✅ historique | Assets migrés ; outillage actif retiré. |
| MIG09-A/B/C/D | ✅ | Purges legacy successives. |
| MIG09-E1 | ✅ | Autorité persistante exclusivement typée. |
| MIG09-E2A | ✅ | Frontière runtime world-object native. |
| MIG09-E2B | ✅ validé | Runtime spécialisé hors cache `Objects`. |
| MIG09-E2C | 🟨 en cours | Editor/tests puis suppression physique du DTO. |
| MIG10 | ⬜ | Renommage final `UGridObjectArchetypeAsset` → `UGridWorldObjectDefinitionAsset`. |

Validation E2B :

```text
Grimrock.WorldObjects : 34 success / 0 warnings / 0 failed / exit 0
MON13.3 LifecyclePersistence : 0 failed / exit 0
```

## 4. Autorité persistante

```text
UGridLevelAsset
├── WorldObjectInstances
├── LooseItemInstances
├── MonsterSpawns
├── ItemSpawns
└── LogicObjects
```

`LooseItemInstance` représente un item déjà présent ; `ItemSpawn` représente un générateur.

`Objects` n'est plus une autorité persistante. Il reste uniquement comme cache de compatibilité transitoire jusqu'à la fin d'E2C.

## 5. MIG09-E2C — dernière purge

E2C doit aboutir à :

```text
Grid Editor -> placements typés
Tests       -> placements typés
Core        -> structures natives
Objects     -> supprimé
FGridLevelObjectData -> supprimé
GridLevelPlacementCompatibility -> supprimé
wrappers runtime DTO -> supprimés
```

### 5.1. Bloc courant : MonsterSpawn authoring natif

Le bloc courant retire les derniers round-trips MonsterSpawn par snapshot dans :

```text
GridMonsterSpawnConfiguration
GridLevelEditorActorPatrolRoute
```

La patrouille est éditée directement dans :

```text
UGridLevelAsset::MonsterSpawns
FGridMonsterSpawnInstance::PatrolMode
FGridMonsterSpawnInstance::PatrolWaypoints
```

Le runtime résout également la configuration initiale directement depuis `MonsterSpawns`.

### 5.2. Reste à migrer dans E2C

```text
Editor générique : sélection / inspector / overview / links / Lua / validation
Items : setters et inspection
WorldObjects : setters et inspection
LogicObjects / ItemSpawns : authoring et inspection
Tests : fixtures FGridLevelObjectData
Core : derniers helpers DTO
```

Une fois ces consommateurs migrés, suppression physique en une seule purge :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
CommitCompatibilityObjectEdit
RefreshLegacyObjectMirrorFromTyped
GetObjectCompatibilityView
BuildCompatibilityObjectProjectionFromTyped
RebuildTypedPlacementProjectionFromLegacy
EnableTypedPlacementStorageFromLegacy
GridLevelPlacementCompatibility.h
GridLevelPlacementConversion::To*
```

## 6. MIG10 — renommage final

Après clôture de MIG09 :

```text
UGridObjectArchetypeAsset
        ↓
UGridWorldObjectDefinitionAsset
```

Vocabulaire associé :

```text
ArchetypeId              -> DefinitionId / WorldObjectDefinitionId
ObjectArchetypes         -> WorldObjectDefinitions
FindObjectArchetypeById  -> FindWorldObjectDefinition...
```

## 7. Definition of Done finale

```text
[ ] items = une définition unique
[ ] objets du monde = une définition unique
[ ] monstres = une définition unique
[ ] placements LevelAsset typés uniquement
[ ] aucun FGridLevelObjectData
[ ] aucun Objects même transient
[ ] aucune projection legacy
[ ] preview et runtime consomment les mêmes définitions
[ ] SaveGame = deltas mutables uniquement
[ ] tests protègent les invariants
[ ] niveaux réels jouables après migration
[ ] documentation réconciliée avec la mind map
```

## 8. Validation courante

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Puis, pour le bloc MonsterSpawn E2C :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON14.3.1"
```

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON13.3.LifecyclePersistence"
```

## 9. Documents associés

- `docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md`
- `docs/Architecture/WORLDOBJ_MIG09_LEGACY_PURGE.md`
- `docs/Architecture/WORLDOBJ_MIG09_E2B_TYPED_RUNTIME_CONSUMERS.md`
- `docs/Design/12_GRID_OBJECT_INSTANCE_BEHAVIOR_RULE.md`
