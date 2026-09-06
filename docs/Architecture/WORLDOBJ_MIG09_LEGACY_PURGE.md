# WORLDOBJ-MIG09 — Purge des compatibilités legacy

Statut : **MIG09-A, MIG09-B1 et MIG09-B2A validés ; MIG09-B2B1 candidat — validation locale UE5.5.4 requise**.

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

## 3. MIG09-B1 — une seule identité runtime pour `AGridItemActor`

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

MIG09-B1 a été validé localement sous UE5.5.4 avec le filtre `Grimrock.WorldObjects` : `30` succès, `1` warning, `0` échec, exit code `0`.

## 4. MIG09-B2A — réceptacles : `ItemDefinitionId` uniquement

Le sous-système Receptacle ne conserve plus de seconde identité d'item :

```text
FGridContainedReceptacleItem
├── RuntimeObjectId
├── ItemDefinitionId       <- identité unique
├── ItemDefinition
├── ItemActor
└── état runtime / lecture / présentation
```

Sont physiquement supprimés :

- `FGridContainedReceptacleItem::ItemArchetypeId` ;
- `AGridReceptacleActor::ContainedItemArchetypeId`.

Les chemins de capture/restauration ne consultent plus `GetItemArchetypeId()` ni un fallback de réceptacle. Un item contenu sans `ItemDefinitionId` est invalide et n'est pas sérialisé silencieusement sous un ancien identifiant.

Le test `Grimrock.WorldObjects.MIG09.ReceptacleItemDefinitionAuthority` vérifie par réflexion que les deux propriétés réceptacle supprimées ne réapparaissent pas.

MIG09-B2A a été validé localement sous UE5.5.4 avec le filtre `Grimrock.WorldObjects` : `31` succès, `1` warning, `0` échec, exit code `0`.

## 5. MIG09-B2B1 — schéma SaveGame des items

`FGridRuntimeItemState::ArchetypeId` n'est plus une propriété réfléchie et n'est plus sérialisé dans le SaveGame.

Le schéma persistant devient :

```text
FGridRuntimeItemState
├── ObjectId
├── ItemDefinitionId       <- identité persistée unique
├── Quantity
├── Cell / Edge / Transform
├── état physique / conteneur / lumière
└── contenu lisible
```

Pour isoler cette modification de schéma des réécritures de gros consommateurs runtime, un proxy C++ transitoire `FGridRuntimeItemArchetypeCompatProxy` porte encore le nom source `ArchetypeId`. Il ne contient aucune valeur : toute lecture ou écriture est redirigée vers `ItemDefinitionId`. Ce proxy n'est ni `UPROPERTY`, ni `SaveGame`, ni visible en Blueprint.

Le test `Grimrock.WorldObjects.MIG09.RuntimeItemSaveDefinitionAuthority` vérifie :

- absence réfléchie de `ArchetypeId` dans `FGridRuntimeItemState` ;
- présence de `ItemDefinitionId` ;
- lecture/écriture du proxy redirigée vers `ItemDefinitionId` sans second stockage.

Le proxy disparaîtra en MIG09-B2B2 après remplacement textuel des derniers consommateurs.

## 6. Ponts encore à supprimer après MIG09-B2B1

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
FGridRuntimeItemArchetypeCompatProxy
anciens initializers directs Button / Lever / Door / Pit
champs d'animation Transient pré-MIG04
anciens contrôles Slate qui éditent encore ces champs
```

## 7. Ordre de purge retenu

```text
MIG09-A     supprimer l'usage runtime du marqueur sparse pré-MIG08        ✅ validé
MIG09-B1    supprimer le stockage AGridItemActor::ArchetypeId             ✅ validé
MIG09-B2A   supprimer les miroirs ItemArchetypeId des réceptacles          ✅ validé
MIG09-B2B1  retirer ArchetypeId du schéma SaveGame runtime                 ⏳ candidat
MIG09-B2B2  réécrire consommateurs + supprimer proxy et cache spawn
MIG09-B2C   supprimer aliases InitializeItem/GetItemArchetypeId après audit Blueprint
MIG09-C     supprimer les champs d'animation Transient et anciens initializers
MIG09-D     migrer les derniers consommateurs vers les placements typés
MIG09-E     supprimer Objects / FGridLevelObjectData / compatibility projection
```

Chaque tranche doit laisser `master` compilable et être validée avec le filtre `Grimrock.WorldObjects` avant la suivante.
