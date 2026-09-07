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

Dernière révision validée :

```text
ddbbe4d2d286aabec40c8cfd79f6ed407d16f587
WORLDOBJ-MIG09 detach Grid Editor writes from compatibility commit
```

Validation locale UE5.5.4 de MIG09-D2 :

```text
Succeeded              : 35
Succeeded with warnings: 1
Failed                 : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-154104
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
| MIG08 | ✅ intégré | Migration/réenregistrement et autorité typée. |
| MIG09-A/B/C/D1/D2 | ✅ validés | Purges legacy successives jusqu'au write-through Grid Editor. |
| MIG09-D3 | 🟨 candidat | Preview en lecture seule et confinement du miroir legacy hors des chemins Editor courants. |
| MIG09-E | ⬜ à faire | Supprimer toute la projection `Objects / FGridLevelObjectData`. |
| MIG10 | ⬜ à faire | Renommage final `UGridObjectArchetypeAsset` → `UGridWorldObjectDefinitionAsset`. |

MIG10 ne commence pas avant la suppression complète de MIG09-E.

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

```text
Floor   : N = hauteur au-dessus du sol
Wall    : N = profondeur / inset
Ceiling : N = distance sous le plafond
```

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

Après MIG09-C, ce bloc est l'unique autorité géométrique et temporelle des mécanismes.

## 5. Definition / Instance / Runtime

```text
Effective runtime object
    = Definition
    + Level Instance Configuration
    + Saved Runtime Delta
```

### Definition

Identité, placement autorisé, spatial permanent, visual/motion, interaction générique, audio/VFX, lumière, classe runtime et comportement partagé.

### Instance

`InstanceId`, référence Definition, cellule/côté, état initial, Tag/Notes, destination, contenu initial et overrides explicitement nécessaires.

### Runtime / Save

États mutables et deltas de persistance uniquement.

## 6. Placements typés du niveau

La cible reste :

```text
UGridLevelAsset
├── WorldObjectInstances
├── LooseItemInstances
├── MonsterSpawns
├── ItemSpawns
└── LogicObjects
```

## 7. MIG09 — purge finale

### Validé

```text
MIG09-A       autorité Definition sans marqueur sparse              ✅
MIG09-B*      identité Item legacy                                  ✅
MIG09-C       mécanismes + animation spécialisée                    ✅
MIG09-D1      SparseBehaviorOverrideObjectIds                       ✅
MIG09-D2      écritures Grid Editor hors compatibility commit       ✅
```

### MIG09-D3 — confinement du miroir

Candidat courant.

Objectifs :

- `RebuildPreview()` devient strictement read-only ;
- supprimer son fallback de write-back ;
- ne plus appeler `GetObjectCompatibilityView()` dans les chemins Editor courants ;
- rendre les rafraîchissements du miroir explicites dans les rares tests qui modifient directement les collections typées ;
- confiner toute la projection legacy au coeur `UGridLevelAsset` et au service de migration.

D3 ne crée aucune nouvelle façade. Les primitives legacy restantes sont supprimées ensemble en MIG09-E.

### MIG09-E — suppression de la projection legacy

Une seule tranche substantielle doit supprimer :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
bTypedPlacementStorageAuthoritative
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
GridLevelPlacementCompatibility
projection legacy -> typed
```

Critère de sortie : Editor, runtime et tests consomment exclusivement les placements typés.

## 8. MIG10 — renommage final

Après MIG09-E :

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

## 9. Ownership cible

| Couche | Possède | Ne possède pas |
|---|---|---|
| Definition Data Asset | Identité/propriétés permanentes. | Position de niveau, état mutable. |
| `UGridLevelAsset` | Layout, placements, logique, état initial, références. | Copie complète des définitions. |
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
