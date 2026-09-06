# WORLDOBJ-MIG09 — Purge des compatibilités legacy

Statut : **MIG09-A validé ; MIG09-B1 candidat — validation locale UE5.5.4 requise**.

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

Les overrides instance-owned restent : Teleporter, Transition, Pit, `Receptacle.InitialContent` et `Lock.bStartsUnlocked`.

MIG09-A a été validé localement sous UE5.5.4 avec le filtre `Grimrock.WorldObjects` : `30` succès, `1` warning, `0` échec, exit code `0`.

## 3. MIG09-B1 — une seule identité runtime pour les items

`AGridItemActor` ne possède plus de champ `ArchetypeId`.

La source de vérité est désormais uniquement :

```text
ItemDefinitionAsset
        +
ItemDefinitionId
```

Deux anciennes API Blueprint/C++ restent temporairement présentes afin d'éviter de casser silencieusement un Blueprint binaire non visible au code search :

```text
InitializeItem(...)
GetItemArchetypeId()
```

Elles ne possèdent plus aucune sémantique d'archétype :

- `InitializeItem()` écrit seulement `ItemDefinitionId`, les tags et éventuellement le mesh fourni ;
- `GetItemArchetypeId()` retourne strictement `GetItemDefinitionId()` ;
- la propriété réfléchie `AGridItemActor::ArchetypeId` est physiquement supprimée ;
- le test MIG05 exige désormais son absence.

Ces deux aliases doivent être supprimés en MIG09-B2 après remplacement/audit des derniers consommateurs et des éventuelles références Blueprint.

## 4. Ponts encore à supprimer après MIG09-B1

```text
UGridLevelAsset::Objects
FGridLevelObjectData
SparseBehaviorOverrideObjectIds
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
GridLevelPlacementCompatibility
AGridItemActor::InitializeItem() alias
AGridItemActor::GetItemArchetypeId() alias
FGridSpawnedItemRuntimeEntry::ItemArchetypeId
FGridRuntimeItemState::ArchetypeId
FGridContainedReceptacleItem::ItemArchetypeId
AGridReceptacleActor::ContainedItemArchetypeId
anciens initializers directs Button / Lever / Door / Pit
champs d'animation Transient pré-MIG04
anciens contrôles Slate qui éditent encore ces champs
```

## 5. Ordre de purge retenu

```text
MIG09-A   supprimer l'usage runtime du marqueur sparse pré-MIG08       ✅ validé
MIG09-B1  supprimer le stockage AGridItemActor::ArchetypeId            ⏳ candidat
MIG09-B2  supprimer aliases/fallbacks et états ItemArchetypeId restants
MIG09-C   supprimer les champs d'animation Transient et anciens initializers
MIG09-D   migrer les derniers consommateurs vers les placements typés
MIG09-E   supprimer Objects / FGridLevelObjectData / compatibility projection
```

Chaque tranche doit laisser `master` compilable et être validée avec le filtre `Grimrock.WorldObjects` avant la suivante.
