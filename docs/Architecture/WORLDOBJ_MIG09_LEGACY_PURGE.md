# WORLDOBJ-MIG09 — Purge des compatibilités legacy

Statut : **MIG09-A candidat — validation locale UE5.5.4 requise**.

## 1. Contexte

MIG08 a réenregistré les LevelAssets de production dans les collections typées et a validé l'idempotence du migrateur (`Changed assets = 0`, `Errors = 0`). MIG09 peut donc supprimer les mécanismes qui ne servaient qu'à distinguer les assets pré-migration des assets cibles.

## 2. MIG09-A — autorité de définition sans marqueur de migration

Le resolver `GridObjectInstanceBehavior` ne consulte plus le statut `SparseBehaviorOverrideObjectIds` pour décider de l'autorité du comportement partagé.

Lorsqu'une `UGridObjectArchetypeAsset` est disponible :

```text
DefaultBehavior de la définition
        +
overrides strictement instance-owned
        =
comportement effectif
```

Les overrides instance-owned restent :

- Teleporter ;
- Transition ;
- Pit ;
- Receptacle.InitialContent ;
- Lock.bStartsUnlocked.

Un `FGridLevelObjectData` sans définition conserve temporairement son snapshot brut uniquement pour les anciens initializers/tests directs. Ce dernier fallback doit disparaître dans une tranche MIG09 suivante avant la suppression physique de `FGridLevelObjectData`.

## 3. Ponts encore à supprimer après MIG09-A

L'audit du `master` après MIG08 identifie encore notamment :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
SparseBehaviorOverrideObjectIds
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
GridLevelPlacementCompatibility
AGridItemActor::ArchetypeId
AGridItemActor::InitializeItem()
AGridItemActor::GetItemArchetypeId()
anciens initializers directs Button / Lever / Door / Pit
champs d'animation Transient pré-MIG04
anciens contrôles Slate qui éditent encore ces champs
```

## 4. Ordre de purge retenu

```text
MIG09-A  supprimer l'usage runtime du marqueur sparse pré-MIG08
MIG09-B  supprimer les identités/fallbacks Item legacy
MIG09-C  supprimer les champs d'animation Transient et anciens initializers
MIG09-D  migrer les derniers consommateurs vers les placements typés
MIG09-E  supprimer Objects / FGridLevelObjectData / compatibility projection
```

Chaque tranche doit laisser `master` compilable et être validée avec le filtre `Grimrock.WorldObjects` avant la suivante.
