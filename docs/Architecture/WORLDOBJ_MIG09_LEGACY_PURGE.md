# WORLDOBJ-MIG09 — Purge des compatibilités legacy

Statut : **MIG09-A à MIG09-D validés ; MIG09-E1 candidat ; MIG09-E2 à faire ; MIG10 après MIG09**.

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

Validation locale MIG09-D3 :

```text
Build                  : OK
Succeeded              : 35
Succeeded with warnings: 1
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-155940
```

Le warning de version du compilateur Visual Studio 2022 est un warning UBT distinct du résultat Automation.

## 3. Audit d'entrée MIG09-E

L'audit du `master` validé `23f94e8369ba5ff469345ceef1e672563d0bcc40` a montré que `FGridLevelObjectData` n'est pas seulement le stockage historique de `UGridLevelAsset` : le type est encore utilisé comme DTO transitoire par de nombreux acteurs, composants, outils Editor et tests.

Le code search GitHub retourne **154 fichiers source** contenant encore `FGridLevelObjectData` au début de MIG09-E, dont notamment :

- 23 fichiers Public ;
- 24 fichiers Runtime Private ;
- 43 fichiers de tests runtime ;
- le reste dans l'Editor et ses tests.

Supprimer le type par alias ou macro masquerait le problème et violerait la règle « aucun legacy juste au cas où ».

MIG09-E est donc découpé en **deux macro-tranches**, sans micro-commits :

```text
MIG09-E1  supprimer l'autorité persistante monolithique
MIG09-E2  supprimer physiquement le DTO FGridLevelObjectData et ses projections
```

## 4. MIG09-E1 — Typed Authority Only

État : **candidat**.

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

Pendant E1 uniquement, le champ :

```text
UGridLevelAsset::Objects
```

reste présent pour les lecteurs E2 non encore migrés, mais devient :

```text
UPROPERTY(Transient)
```

Il n'est donc plus une donnée d'authoring sérialisée. `PostLoad()` le reconstruit depuis les collections typées, qui sont l'unique autorité persistante.

Aucune écriture actuelle ne doit prendre `Objects` comme source de vérité.

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

MIG08 a déjà rempli son rôle et les assets courants ont été migrés/validés. E1 supprime l'outillage Editor actif devenu historique :

```text
GridWorldObjectMIG08MigrationService
GridWorldObjectMIG08Commandlet
GridEditorWorldObjectMIG08MigrationTests
```

Les notes MIG08 restent de la documentation historique, pas une API active.

### 4.5. Tests de garde

`Grimrock.WorldObjects.MIG07.TypedLifecycleSchema` vérifie désormais :

```text
bTypedPlacementStorageAuthoritative absent
Objects encore présent pendant E1
Objects porte CPF_Transient
```

`Grimrock.WorldObjects.MIG07.TypedAuthoritySchema` vérifie l'absence du marqueur et la présence des cinq collections typées.

## 5. MIG09-E2 — suppression physique du DTO legacy

Après validation de E1, E2 doit supprimer en une macro-tranche :

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

Les acteurs runtime recevront les structures natives adaptées :

```text
FGridWorldObjectInstance
FGridLooseItemInstance
FGridMonsterSpawnInstance
FGridItemSpawnInstance
FGridLogicObjectInstance
```

Si un payload runtime résolu est réellement nécessaire, il doit représenter **explicitement** la couche Runtime et ne doit jamais être un alias de `FGridLevelObjectData`, ni être stocké dans `UGridLevelAsset`.

Le Grid Editor doit de son côté éditer les placements typés, pas un snapshot monolithique reconstitué.

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

## 7. Validation E1

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
[ ] bTypedPlacementStorageAuthoritative absent de la réflexion
[ ] Objects = CPF_Transient
[ ] TypedLifecycle / TypedAuthority verts
```
