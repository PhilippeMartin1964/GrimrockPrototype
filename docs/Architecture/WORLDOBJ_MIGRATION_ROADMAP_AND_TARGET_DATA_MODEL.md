# WORLDOBJ — Roadmap MIG00 à MIG10 et modèle de données cible

Statut : **WORLDOBJ MIG00 → MIG10 ✅ CLOSED**

Mise à jour : **2026-09-09**

Clôture du modèle et du vocabulaire, sans certification visuelle de tous les niveaux. Le [rapport MIG10 final](WORLDOBJ_MIG10_FINAL.md) consigne 350 tests sans échec, le rebuild complet non-unity, 92 DataAssets et 3 maps chargeables. Le défaut de départ préexistant de `DA_GridLevel_01` reste inchangé.

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
| MIG09-E2B | ✅ | Runtime spécialisé hors cache `Objects`. |
| MIG09-E2C | ✅ | Consommateurs et fixtures natifs ; DTO/cache supprimés ; FINAL-A/B/C/D validés. |
| MIG10-A | ✅ | Type C++ et API WorldObjectDefinition finalisés. |
| MIG10-B | ✅ | 38 définitions sérialisées migrées, identifiants préservés. |
| MIG10-C | ✅ | Audit, documentation, régression finale et clôture. |

## 4. Autorité persistante

Les seules collections persistantes de placement sont :

```text
UGridLevelAsset
├── WorldObjectInstances
├── LooseItemInstances
├── MonsterSpawns
├── ItemSpawns
└── LogicObjects
```

`LooseItemInstance` représente un item déjà présent ; `ItemSpawn` représente un générateur.

`UGridLevelAsset::Objects`, `FGridLevelObjectData` et les projections de compatibilité ont été physiquement supprimés dans FINAL-C (`046d58bc`). Editor, runtime et fixtures consomment les structures natives. `FGridRuntimeWorldObjectData` est conservé uniquement comme frontière d'initialisation runtime spécialisée non persistante ; il n'est ni stocké dans le niveau ni un DTO général d'authoring.

## 5. Validations acquises au 8 septembre 2026

Le premier lot Runtime E2C (`5be706ab`) a été validé localement :

```text
Grimrock.MON19.4.LuaBridge                    : 4 success / 1 warning / 0 failed / exit 0
Grimrock.MON19.7.1.LuaAuthoring.LogicIdCommand: 0 success / 1 warning / 0 failed / exit 0
Grimrock.Monsters.Perception.AcousticHearing : 1 success / 0 warning / 0 failed / exit 0
Grimrock.WorldObjects                         : 34 success / 0 warning / 0 failed / exit 0
```

Le lot TD01.3.2 (`688209e2`) est également validé localement :

```text
Grimrock.TechnicalDebt.TD01_3.EventCommandContract.RuntimeHardening
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0

Grimrock.WorldObjects
Succeeded              : 34
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

## 6. MIG09-E2C-FINAL — étapes de clôture

Les descriptions FINAL-A à FINAL-C ci-dessous conservent la stratégie historique. Ces trois blocs sont publiés ; les anciens états transitoires ne décrivent plus le code actuel. Le [rapport FINAL-D](WORLDOBJ_MIG09_FINAL_D.md) fournit la validation finale et ses limites.

### FINAL-A — consommateurs de production natifs

Objectif : aucun consommateur de production ne doit reconstruire une collection monolithique `FGridLevelObjectData` pour travailler sur le niveau.

FINAL-A a migré `UGridActivationComponent`, qui conservait auparavant :

```text
BuildCompatibilityObjectProjectionFromTyped()
        -> IndexedObjects : TArray<FGridLevelObjectData>
        -> index ObjectId -> index de DTO
```

Cible :

```text
UGridLevelAsset typed arrays
        -> index runtime d'identités FGuid uniquement
        -> lookup direct dans la collection typée native
```

Le niveau expose des helpers de lecture typés :

```text
FindWorldObjectInstanceById
FindLooseItemInstanceById
FindMonsterSpawnInstanceById
FindItemSpawnInstanceById
FindLogicObjectInstanceById
GetTypedPlacementType
FindTypedPlacementIdsByLogicId
```

`GridLogicRuntime` possède une API native `FGridLogicObjectInstance`. Sa surcharge legacy, conservée temporairement entre FINAL-A et FINAL-B, a été supprimée dans FINAL-C.

FINAL-A n'est considéré validé qu'après build UE5.5.4 et régressions locales.

### FINAL-B — fixtures restantes en lot

Migrer ensemble les fixtures historiques encore branchées sur `Objects` / `FGridLevelObjectData`, notamment les familles MON19.8, MON19.2 historiques, MON13/14, StoryCompanion/Recruiter et MIG07 de caractérisation.

Les tests qui caractérisent volontairement une conversion legacy sont soit réécrits contre l'invariant cible, soit supprimés s'ils n'ont plus de contrat utile à protéger.

### FINAL-C — purge physique

Supprimer en une seule passe :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
CommitCompatibilityObjectEdit
RefreshLegacyObjectMirrorFromTyped
GetObjectCompatibilityView
BuildCompatibilityObjectProjectionFromTyped
TryGetCompatibilityObjectSnapshot
RebuildTypedPlacementProjectionFromLegacy
EnableTypedPlacementStorageFromLegacy
GridLevelPlacementCompatibility.h
GridLevelPlacementConversion::To*
wrappers/adapters runtime DTO restants
```

Aucun substitut monolithique ne doit être créé sous un autre nom.

### FINAL-D — validation et clôture MIG09

```text
1. build Development Editor UE5.5.4
2. Grimrock.WorldObjects
3. filtres Runtime/Event/Lua/Logic/Monster critiques
4. filtres Grid Editor critiques
5. niveau réel jouable
6. recherche statique : aucun symbole legacy cible
7. documentation réconciliée
```

FINAL-D a clos MIG09 sans renommage de classe. MIG10 a ensuite finalisé les noms dans une étape distincte.

## 7. MIG10 — renommage final clos

A ✅ — `UGridWorldObjectDefinitionAsset`, `DefinitionId`, références de placement `WorldObjectDefinitionId`, palette `DefaultWorldObjectDefinition`, registre `WorldObjectDefinitions` et lookup `FindWorldObjectDefinitionById` sont les noms finaux.

B ✅ — 38 définitions resauvegardées avec le nouveau class tag ; 38 identifiants préservés et uniques. Les placements et liens sont inchangés.

C ✅ — documentation active réconciliée, rebuild complet UE5.5.4 non-unity vert, baseline FINAL-D étendue à 350 tests (0 failed, 0 not run), 92 DataAssets et 3 maps chargeables. Voir le [rapport final](WORLDOBJ_MIG10_FINAL.md).

Les Core Redirects temporaires restent nécessaires pour les Blueprints/assets externes éventuels conservant les anciens noms. Aucun alias ou wrapper C++ de compatibilité MIG10 n'est ajouté.

Le chemin de package historique `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/` est volontairement conservé : il ne représente plus une classe ni un concept architectural actif. Une réorganisation Content éventuelle relève d'une tâche distincte avec AssetTools.

## 8. Definition of Done MIG09

```text
[x] aucun Objects sérialisé ou transient
[x] aucun FGridLevelObjectData
[x] aucune projection legacy <-> typed
[x] aucun consommateur de production sur DTO legacy
[x] runtime sur structures natives / payload runtime légitime
[x] Editor sur placements typés
[x] tests sans fixtures legacy actives
[x] rebuild complet Development Editor non-unity réussi
[x] Automation FINAL-D : 349 tests, 0 Failed
[x] niveaux réels chargés et audités ; réserves détaillées dans FINAL-D
[x] documentation réconciliée avec la mind map pour MIG09
[x] git diff --check réussi
```

La jouabilité interactive n'est pas certifiée : le départ de `DA_GridLevel_01` est invalide depuis avant la purge et aucun smoke test visuel n'a été exécuté. Ces réserves de contenu sont distinctes de la clôture technique MIG09.

## 9. Definition of Done finale MIG00 → MIG10

```text
[x] items = une définition unique
[x] objets du monde = une définition unique
[x] monstres = une définition unique
[x] placements LevelAsset typés uniquement
[x] preview et runtime consomment les mêmes définitions
[x] SaveGame = deltas mutables uniquement
[x] tests protègent les invariants
[x] vocabulaire WorldObjectDefinition finalisé
```

## 10. Commande de validation ciblée

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Cette commande ciblée ne remplace pas la régression finale : les 17 familles effectivement exécutées et leurs résultats sont consignés dans le rapport MIG10 final.

## 11. Documents associés

- `docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md`
- `docs/Architecture/WORLDOBJ_MIG09_LEGACY_PURGE.md`
- `docs/Architecture/WORLDOBJ_MIG09_E2C_EDITOR_AUTHORITY.md`
- `docs/Architecture/WORLDOBJ_MIG09_E2B_TYPED_RUNTIME_CONSUMERS.md`
- `docs/Design/12_GRID_OBJECT_INSTANCE_BEHAVIOR_RULE.md`
