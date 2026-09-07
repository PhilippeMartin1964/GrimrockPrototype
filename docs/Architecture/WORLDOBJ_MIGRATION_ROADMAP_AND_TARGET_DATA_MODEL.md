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

Dernière révision validée avant MIG09-E1 :

```text
23f94e8369ba5ff469345ceef1e672563d0bcc40
WORLDOBJ-MIG09 isolate Grid Editor from legacy read mirror
```

Validation locale UE5.5.4 de MIG09-D3 :

```text
Succeeded              : 35
Succeeded with warnings: 1
Failed                 : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-155940
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
| MIG09-E1 | 🟨 candidat | Autorité persistante exclusivement typée ; marqueur supprimé ; `Objects` transitoire. |
| MIG09-E2 | ⬜ à faire | Supprimer `Objects`, `FGridLevelObjectData` et toutes les projections legacy. |
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
```

### MIG09-E — découverte de portée

L'audit d'entrée E a trouvé `FGridLevelObjectData` dans **154 fichiers source**. Le type est devenu au fil du projet à la fois ancien stockage et DTO transitoire runtime/editor.

Un alias conservant le nom serait un faux nettoyage. La purge est donc faite en deux macro-tranches :

```text
E1  supprimer l'autorité persistante monolithique
E2  supprimer le DTO et migrer tous ses consommateurs
```

### MIG09-E1 — autorité typée inconditionnelle

Candidat courant.

Objectifs réalisés :

- supprimer physiquement `bTypedPlacementStorageAuthoritative` ;
- rendre les cinq collections typées inconditionnellement autoritaires ;
- faire de `Objects` un cache `Transient`, donc non sérialisé ;
- reconstruire ce cache uniquement depuis les placements typés pour les derniers lecteurs E2 ;
- retirer l'outillage actif `MIG08MigrationService` / `MIG08Commandlet` devenu historique ;
- protéger le contrat par réflexion Automation.

Cette tranche supprime la possibilité qu'un `.uasset` sauvegardé ait encore `Objects` comme source d'authoring.

### MIG09-E2 — modèle monolithique et DTO

Après validation E1, supprimer physiquement :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
RebuildTypedPlacementProjectionFromLegacy()
EnableTypedPlacementStorageFromLegacy()
GridLevelPlacementCompatibility.h
GridLevelPlacementConversion::ToWorldObject / ToLooseItem / ToMonsterSpawn / ToItemSpawn / ToLogicObject
```

Les consommateurs doivent utiliser les types natifs de leur domaine. Un éventuel payload runtime résolu est permis uniquement s'il représente explicitement la couche Runtime et n'est ni sérialisé dans le niveau ni un alias legacy.

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
