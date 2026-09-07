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
- les Data Assets ne portent pas d'état runtime mutable ;
- `UGridLevelAsset` reste la source de vérité spatiale ;
- le SaveGame ne copie pas meshes, sons ou paramètres permanents.

Schéma cible :

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
├── grille
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

## 2. État actuel

Dernière révision validée avant MIG09-E2A :

```text
268d68659619f036f62c5549242e5453c37c21f0
WORLDOBJ-MIG09 align sparse marker test with typed authority
```

Validation locale UE5.5.4 de MIG09-E1 :

```text
Succeeded              : 32
Succeeded with warnings: 1
Failed                 : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-163759
```

État des jalons :

| Jalon | Statut | Résultat |
|---|---|---|
| MIG00 | ✅ validé | Caractérisation du contrat historique. |
| MIG01 | ✅ validé | Placement `Floor / Wall / Ceiling` + `U/V/N`. |
| MIG02 | ✅ validé | Spatial/boundary simplifié. |
| MIG03 | ✅ intégré | `StaticPart` + `MovingParts`. |
| MIG04 | ✅ intégré | Motion générique des mécanismes. |
| MIG05 | ✅ intégré | Collectible direct : `UGridItemDefinitionAsset`. |
| MIG06 | ✅ intégré | Definition + overrides strictement instance-owned. |
| MIG07 | ✅ intégré | Collections de placements typées. |
| MIG08 | ✅ historique | Assets migrés ; outillage actif retiré en E1. |
| MIG09-A/B/C/D | ✅ validés | Purges legacy successives ; Grid Editor détaché du write-back implicite. |
| MIG09-E1 | ✅ validé | Autorité persistante exclusivement typée ; marqueur supprimé ; `Objects` transitoire. |
| MIG09-E2A | 🟨 candidat | Frontière runtime world-object explicite, non sérialisée. |
| MIG09-E2B | ⬜ à faire | Runtime spécialisé et lectures directes des collections typées. |
| MIG09-E2C | ⬜ à faire | Editor/tests puis suppression physique du DTO et des projections. |
| MIG10 | ⬜ à faire | Renommage final `UGridObjectArchetypeAsset` → `UGridWorldObjectDefinitionAsset`. |

MIG10 ne commence pas avant la suppression complète de MIG09-E2.

## 3. Modèle spatial cible

```text
PlacementSurface
├── Floor
├── Wall
└── Ceiling
```

```text
U = première tangente
V = seconde tangente ; verticale sur Wall
N = normale à la surface
```

Interprétation :

```text
Floor   : N = hauteur au-dessus du sol
Wall    : N = profondeur / inset
Ceiling : N = distance sous le plafond
```

La frontière topologique reste distincte de la surface de placement.

## 4. Composition visuelle

```text
WorldObject Definition
├── StaticPart optional
└── MovingParts
    ├── Part0 optional
    │   └── Motion
    └── Part1 optional
        └── Motion
```

`Motion` : Type, Axis, Pivot, Amount, Duration.

Après MIG09-C, ce bloc est l'unique autorité géométrique et temporelle des mécanismes. Les règles gameplay telles que `ButtonHoldTime`, poids de plaque ou chaîne de porte restent séparées.

## 5. Definition / Instance / Runtime

```text
Effective runtime object
    = Definition
    + Level Instance Configuration
    + Saved Runtime Delta
```

### Definition

Possède identité, placement autorisé, spatial permanent, visual/motion, interaction générique, audio/VFX, lumière, classe runtime et comportement partagé.

### Instance

Possède uniquement ce qui varie avec le placement : identité persistante, référence Definition, cellule/côté, transform local éventuel, état initial, Tag/Notes, destination et données strictement locales.

### Runtime / Save

Possède les états mutables et deltas de persistance, pas les données permanentes de Definition.

## 6. Placements typés du niveau

Autorité persistante cible :

```text
UGridLevelAsset
├── WorldObjectInstances
├── LooseItemInstances
├── MonsterSpawns
├── ItemSpawns
└── LogicObjects
```

`LooseItemInstance` représente un item déjà présent ; `ItemSpawn` représente un générateur.

## 7. MIG09 — purge finale

### Validé

```text
MIG09-A       autorité Definition sans marqueur sparse              ✅
MIG09-B*      identité Item legacy                                  ✅
MIG09-C       mécanismes + animation spécialisée                    ✅
MIG09-D1      SparseBehaviorOverrideObjectIds                       ✅
MIG09-D2      write-through Grid Editor                             ✅
MIG09-D3      fallback preview / read mirror Editor                 ✅
MIG09-E1      autorité persistante exclusivement typée              ✅
```

### MIG09-E2 — suppression du DTO monolithique

L'audit d'entrée E a trouvé `FGridLevelObjectData` dans 154 fichiers source. Le type était devenu à la fois ancien stockage et DTO transitoire runtime/editor. Un alias conservant le nom serait un faux nettoyage.

E2 est donc découpé en trois macro-tranches cohérentes :

```text
E2A  établir une frontière runtime world-object explicite
E2B  migrer runtime spécialisé / LevelRuntime vers les collections natives
E2C  migrer Editor/tests et supprimer physiquement DTO + projections
```

### MIG09-E2A — Runtime World Object Boundary

Candidat courant.

`FGridRuntimeWorldObjectData` est un payload C++ runtime-only, non réfléchi et non sérialisé. Il est construit nativement depuis `FGridWorldObjectInstance` et porte uniquement les données requises par l'initialisation d'un acteur world-object.

Les acteurs génériques et mécanismes migrés utilisent :

```text
InitializeRuntimeWorldObject(...)
InitializeRuntimeWorldObjectBase(...)
InitializeRuntimeMechanismVisuals(...)
ResolveEffectiveBehavior(FGridRuntimeWorldObjectData)
```

Les wrappers `FGridLevelObjectData` restent provisoires uniquement pour maintenir les tests/Editor non encore migrés jusqu'à E2C. Ils ne sont pas une autorité et ne sont jamais stockés dans le niveau.

Acteurs migrés E2A :

```text
Button
Lever
PressurePlate
Door
Trigger
GenericObject
```

Test de garde :

```text
Grimrock.WorldObjects.MIG09.RuntimeWorldObjectPayload
```

### MIG09-E2B

Après validation E2A :

- Receptacle, WallLock et PitTrapdoor sur frontière runtime native ;
- `AGridLevelRuntimeActor` directement sur `WorldObjectInstances` ;
- items monde directement sur `LooseItemInstances` ;
- monstres/encounters/persistence directement sur `MonsterSpawns` ;
- Activation, DoorSystem, transitions/pits et diagnostics hors DTO legacy ;
- aucune lecture runtime directe de `LevelAsset->Objects`.

### MIG09-E2C

Dernière macro-tranche :

- Grid Editor sur placements typés ;
- fixtures/tests convertis ;
- suppression de `Objects` ;
- suppression de `FGridLevelObjectData` ;
- suppression de `GridLevelPlacementCompatibility.h` ;
- suppression de `GridLevelPlacementConversion::To*` ;
- suppression des wrappers runtime E2 temporaires ;
- tests de réflexion anti-régression.

## 8. MIG10 — renommage final

Après MIG09-E2 :

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

Ce renommage reste volontairement dernier pour ne pas mélanger schéma sérialisé et renommage de classe.

## 9. Ownership cible

| Couche | Possède | Ne possède pas |
|---|---|---|
| Definition Data Asset | Identité/propriétés permanentes. | Position de niveau, état mutable. |
| `UGridLevelAsset` | Layout, placements typés, logique, état initial, références. | Copie complète des définitions, miroir monolithique. |
| Runtime Actor/Component | Exécution, animation, collision, état courant. | Authoring permanent. |
| SaveGame/RuntimeState | Deltas mutables. | Meshes, sons, définition complète. |

## 10. Definition of Done finale

```text
[ ] aucun pont MIG01/MIG02/MIG03/MIG05/MIG06 requis
[ ] aucun Behavior visuel dupliquant Motion
[ ] items = une définition unique
[ ] objets du monde = une définition unique
[ ] monstres = une définition unique
[ ] placements LevelAsset typés uniquement
[ ] aucun FGridLevelObjectData
[ ] aucun Objects même transient
[ ] preview et runtime consomment les mêmes définitions
[ ] SaveGame = deltas mutables uniquement
[ ] tests protègent les invariants
[ ] niveaux réels jouables après migration
[ ] documentation réconciliée avec la mind map
```

## 11. Validation courante

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

## 12. Documents associés

- `docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md`
- `docs/Architecture/WORLDOBJ_MIG09_LEGACY_PURGE.md`
- `docs/Architecture/WORLDOBJ_MIG04_GENERIC_MOTION.md`
- `docs/Design/12_GRID_OBJECT_INSTANCE_BEHAVIOR_RULE.md`

Les anciennes notes MIG07/MIG08 restent historiques ; elles ne doivent pas servir à réintroduire une autorité supprimée.
