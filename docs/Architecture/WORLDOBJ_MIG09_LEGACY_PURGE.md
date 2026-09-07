# WORLDOBJ-MIG09 — Purge des compatibilités legacy

Statut : **MIG09-A à MIG09-E1 validés ; MIG09-E2A candidat ; MIG09-E2B/E2C à faire ; MIG10 après MIG09**.

Date de mise à jour : 2026-09-07.

## 1. Référence et règle

Référence architecturale prioritaire :

```text
docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md
```

Règle de migration :

```text
une donnée cible existe
        -> les consommateurs sont migrés
        -> le legacy est supprimé physiquement
        -> un test empêche sa réapparition
```

Le contrat cible reste :

```text
Definition
+ instance typée minimale
+ runtime/save delta
```

## 2. État validé

| Tranche | État | Résultat |
|---|---|---|
| MIG09-A | ✅ | Autorité Definition sans marqueur sparse. |
| MIG09-B* | ✅ | Identité Item legacy purgée. |
| MIG09-C | ✅ | Initializers mécanismes et animation spécialisée purgés ; `MovingParts[].Motion` est l'autorité visuelle. |
| MIG09-D1 | ✅ | `SparseBehaviorOverrideObjectIds` supprimé. |
| MIG09-D2 | ✅ | Les écritures principales du Grid Editor ne dépendent plus de `CommitCompatibilityObjectEdit()`. |
| MIG09-D3 | ✅ | `RebuildPreview()` est read-only ; le Grid Editor n'effectue plus de write-back implicite du miroir. |
| MIG09-E1 | ✅ | Les cinq collections typées sont l'unique autorité persistante ; `Objects` n'est plus qu'un cache transient. |

Validation locale MIG09-E1 :

```text
Build                  : OK
Succeeded              : 32
Succeeded with warnings: 1
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-163759
```

Le warning `Visual Studio 2022 compiler is not a preferred version` est un warning UBT distinct du résultat Automation.

## 3. Audit d'entrée MIG09-E

L'audit du `master` validé avant E1 a montré que `FGridLevelObjectData` n'est pas seulement le stockage historique de `UGridLevelAsset` : le type est encore utilisé comme DTO transitoire par de nombreux acteurs, composants, outils Editor et tests.

Le code search GitHub retournait **154 fichiers source** contenant encore `FGridLevelObjectData` au début de MIG09-E, dont notamment :

- 23 fichiers Public ;
- 24 fichiers Runtime Private ;
- 43 fichiers de tests runtime ;
- le reste dans l'Editor et ses tests.

Supprimer le type par alias ou macro masquerait le problème et violerait la règle « aucun legacy juste au cas où ».

MIG09-E garde donc deux objectifs architecturaux :

```text
MIG09-E1  supprimer l'autorité persistante monolithique
MIG09-E2  supprimer physiquement le DTO FGridLevelObjectData et ses projections
```

E2 est exécuté en macro-tranches cohérentes afin de conserver un `master` compilable entre les frontières runtime, Editor et tests ; aucune de ces macro-tranches ne change la cible finale.

## 4. MIG09-E1 — Typed Authority Only

État : **validé**.

### 4.1. Autorité persistante

Le marqueur suivant est supprimé physiquement :

```text
bTypedPlacementStorageAuthoritative
```

Il n'existe plus deux modes de stockage possibles. Les seules données persistantes de placement sont :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

`UsesSparseBehaviorOverrides()` consulte directement `WorldObjectInstances`.

### 4.2. `Objects` n'est plus persistant

Pendant E1/E2 uniquement, le champ :

```text
UGridLevelAsset::Objects
```

reste présent pour les lecteurs E2 non encore migrés, mais il est :

```text
UPROPERTY(Transient)
```

Il n'est donc plus une donnée d'authoring sérialisée. `PostLoad()` le reconstruit depuis les collections typées, qui sont l'unique autorité persistante.

### 4.3. Lifecycle du LevelAsset

Les opérations courantes sont désormais toujours typées :

```text
AddObject(snapshot transitoire)
        -> collection typée
        -> cache E1 reconstruit

RemoveObjectById
        -> collection typée

EnsureObjectIds
        -> collections typées

ClearLevel
        -> collections typées
```

Le snapshot `FGridLevelObjectData` reste provisoirement accepté par `AddObject()` uniquement parce que E2 doit migrer ses nombreux callers ; il ne redevient jamais persistent.

### 4.4. Retrait de l'outillage MIG08 actif

MIG08 a déjà rempli son rôle et les assets courants ont été migrés/validés. E1 a supprimé l'outillage Editor actif devenu historique :

```text
GridWorldObjectMIG08MigrationService
GridWorldObjectMIG08Commandlet
GridEditorWorldObjectMIG08MigrationTests
```

Les notes MIG08 restent de la documentation historique, pas une API active.

## 5. MIG09-E2 — suppression physique du DTO legacy

Objectif final E2 : supprimer en totalité :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
RebuildTypedPlacementProjectionFromLegacy()
EnableTypedPlacementStorageFromLegacy()
GridLevelPlacementCompatibility.h
GridLevelPlacementConversion::To*
```

Les domaines doivent consommer leurs structures natives :

```text
FGridWorldObjectInstance
FGridLooseItemInstance
FGridMonsterSpawnInstance
FGridItemSpawnInstance
FGridLogicObjectInstance
```

Un payload runtime est permis uniquement s'il représente explicitement la couche Runtime, n'est jamais sérialisé dans `UGridLevelAsset` et n'est pas un alias du DTO legacy.

### 5.1. MIG09-E2A — Runtime World Object Boundary

État : **candidat**.

E2A introduit :

```text
FGridRuntimeWorldObjectData
```

Ce type est un `struct` C++ runtime-only, non réfléchi et non sérialisé. Il porte uniquement ce dont un acteur world-object a besoin pendant son initialisation : identité runtime, type, cellule/côté, identifiant de Definition, état initial, texte lisible local et comportement d'instance.

Deux chemins de construction existent pendant la migration :

```text
FGridWorldObjectInstance      -> FGridRuntimeWorldObjectData   cible native
FGridLevelObjectData          -> FGridRuntimeWorldObjectData   pont E2 temporaire
```

Le second constructeur disparaît avec `FGridLevelObjectData` en sortie E2.

`AGridRuntimeObjectActor` possède désormais la frontière C++ native :

```text
InitializeRuntimeWorldObjectBase(...)
InitializeRuntimeWorldObject(...)
ResolveEffectiveBehavior(FGridRuntimeWorldObjectData)
```

Les anciennes fonctions réfléchies `InitializeGridObject*()` restent temporairement des wrappers de compatibilité ; elles construisent le payload runtime puis délèguent au chemin natif.

`AGridMechanismActor` suit le même principe pour la composition visuelle :

```text
InitializeMechanismVisuals(FGridLevelObjectData)      wrapper temporaire
InitializeRuntimeMechanismVisuals(FGridRuntimeWorldObjectData)   runtime natif
```

Acteurs migrés dans E2A :

```text
AGridButtonActor
AGridLeverActor
AGridPressurePlateActor
AGridDoorActor
AGridTriggerActor
AGridGenericObjectActor
```

Le resolver `GridObjectInstanceBehavior` accepte directement le payload runtime. Ses overloads `FGridLevelObjectData` restent uniquement pour les consommateurs Editor/tests non encore migrés.

Test de garde ajouté :

```text
Grimrock.WorldObjects.MIG09.RuntimeWorldObjectPayload
```

Il vérifie la projection d'une `FGridWorldObjectInstance` vers le payload runtime ainsi que la règle :

```text
Behavior effectif = Definition + overrides strictement instance-owned
```

### 5.2. MIG09-E2B — Runtime spécialisé et structures natives

À faire après validation E2A :

- migrer `Receptacle`, `WallLock` et `PitTrapdoor` vers la frontière runtime native ;
- faire consommer directement `WorldObjectInstances` par `AGridLevelRuntimeActor` ;
- faire consommer directement `LooseItemInstances` aux items monde ;
- faire consommer directement `MonsterSpawns` au runtime monstre/encounter/persistence ;
- migrer Activation, DoorSystem, transitions/pits et diagnostics hors du DTO legacy ;
- éliminer les lectures runtime directes de `LevelAsset->Objects`.

### 5.3. MIG09-E2C — Editor, tests et suppression physique

Dernière macro-tranche E2 :

- Grid Editor sur placements typés sans snapshot monolithique ;
- fixtures/tests convertis aux structures natives adaptées ;
- suppression de `Objects` ;
- suppression de `FGridLevelObjectData` ;
- suppression de `GridLevelPlacementCompatibility.h` et de toutes les projections `To*` ;
- suppression des wrappers runtime temporaires E2 ;
- tests de réflexion garantissant l'absence définitive du legacy.

## 6. Sortie MIG09

MIG09 est terminé uniquement lorsque :

```text
[ ] aucun Objects sérialisé ou transitoire
[ ] aucun FGridLevelObjectData
[ ] aucun marqueur d'autorité
[ ] aucune projection legacy <-> typed
[ ] runtime sur structures natives / payload runtime légitime
[ ] Editor sur placements typés
[ ] tests sans fixtures legacy actives
[ ] Grimrock.WorldObjects : 0 Failed
```

Ensuite seulement :

```text
MIG10
UGridObjectArchetypeAsset
        -> UGridWorldObjectDefinitionAsset
```

## 7. Validation E2A

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Critères :

```text
[ ] Development Editor build OK
[ ] 0 Failed
[ ] Process exit code 0
[ ] RuntimeWorldObjectPayload vert
[ ] Button / Lever / PressurePlate / Door continuent leurs contrats MIG04/MIG06
[ ] aucun changement de données persistantes de niveau
```
