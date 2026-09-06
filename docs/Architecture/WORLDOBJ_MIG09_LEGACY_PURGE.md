# WORLDOBJ-MIG09 — Purge des compatibilités legacy

Statut : **MIG09-A, MIG09-B1, MIG09-B2A, MIG09-B2B1, MIG09-B2B2, MIG09-B2B3, MIG09-B2C-A et MIG09-B2C-B validés ; MIG09-C-A candidat — validation locale UE5.5.4 requise**.

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

Deux anciennes API Blueprint/C++ restaient temporairement présentes après B1 :

```text
InitializeItem(...)
GetItemArchetypeId()
```

Elles ne possédaient plus aucune sémantique d'archétype :

- `InitializeItem()` écrivait seulement `ItemDefinitionId`, les tags et éventuellement le mesh fourni ;
- `GetItemArchetypeId()` retournait strictement `GetItemDefinitionId()` ;
- la propriété réfléchie `AGridItemActor::ArchetypeId` était physiquement supprimée ;
- le test MIG05 exigeait déjà son absence.

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

Les chemins de capture/restauration ne consultent plus `GetItemArchetypeId()` ni un fallback de réceptacle.

Le test `Grimrock.WorldObjects.MIG09.ReceptacleItemDefinitionAuthority` vérifie par réflexion que les deux propriétés supprimées ne réapparaissent pas.

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

MIG09-B2B1 a introduit un proxy C++ strictement transitoire afin d'isoler la modification de schéma des gros consommateurs runtime.

Le test `Grimrock.WorldObjects.MIG09.RuntimeItemSaveDefinitionAuthority` vérifie l'absence réfléchie de `ArchetypeId` et la présence de `ItemDefinitionId`.

MIG09-B2B1 a été validé localement sous UE5.5.4 avec le filtre `Grimrock.WorldObjects` : `32` succès, `1` warning, `0` échec, exit code `0`.

## 6. MIG09-B2B2 — consommateurs de l'état SaveGame

Les chemins principaux ne lisent et n'écrivent plus `FGridRuntimeItemState::ArchetypeId` :

- capture des world-items ;
- restauration des world-items ;
- restauration des contenus de réceptacles ;
- transfert d'items à travers les fosses.

Le comportement devient strict : un état runtime sans `ItemDefinitionId` n'est plus récupéré via une identité `ArchetypeId` historique.

MIG09-B2B2 a été validé localement sous UE5.5.4 avec le filtre `Grimrock.WorldObjects` : `32` succès, `1` warning, `0` échec, exit code `0`.

## 7. MIG09-B2B3 — suppression physique des dernières identités Item legacy

Sont physiquement supprimés :

```text
FGridRuntimeItemArchetypeCompatProxy
FGridSpawnedItemRuntimeEntry::ItemArchetypeId
```

Les world-items, la persistance et les réceptacles utilisent désormais uniquement `ItemDefinitionId`.

Le test MIG09 vérifie également que `FGridSpawnedItemRuntimeEntry` n'expose plus `ItemArchetypeId` par réflexion.

MIG09-B2B3 a été validé localement sous UE5.5.4 avec le filtre `Grimrock.WorldObjects` : `32` succès, `1` warning, `0` échec, exit code `0`.

Commit de production : `ef75d1d8e65e4767289489ec668028d6dcd85d3a` (`WORLDOBJ-MIG09 purge final item archetype runtime mirrors`).

## 8. MIG09-B2C-A — audit des références Blueprint aux anciennes API Item

Avant de supprimer les deux derniers noms d'API :

```text
InitializeItem(...)
GetItemArchetypeId()
```

MIG09-B2C-A ajoute un audit Editor réel des Blueprints sous `/Game/GrimrockPrototype`.

Le test :

```text
Grimrock.WorldObjects.MIG09.ItemLegacyBlueprintReferences
```

utilise l'Asset Registry, charge les `UBlueprint`, parcourt tous leurs graphes et inspecte les `UK2Node_CallFunction`. Toute référence à `InitializeItem` ou `GetItemArchetypeId` provoque un échec et affiche le package, le graphe, la fonction et le node concernés.

L'audit a été validé localement sous UE5.5.4 avec le filtre `Grimrock.WorldObjects` : `33` succès, `1` warning, `0` échec, exit code `0`.

## 9. MIG09-B2C-B — suppression définitive des aliases Item

Après l'audit Blueprint vert :

- `AGridItemActor::GetItemArchetypeId()` est physiquement supprimé ;
- `AGridItemActor::InitializeItem()` est physiquement supprimé, y compris son pont C++ temporaire ;
- `AGridLevelRuntimeActor::SpawnItemActorForDefinition()` n'appelle plus que `InitializeFromItemDefinition()` ou `InitializeFromItemDefinitionId()` ;
- le chargement préalable redondant de `WorldMesh` dans la factory est supprimé ;
- le chemin ID-only remet `ItemTags` à zéro afin de ne pas conserver des tags d'une définition précédente, tout en laissant intact le mesh déjà porté par un Blueprint lorsqu'aucune définition n'est disponible.

Le test MIG05 vérifie la sémantique du chemin ID-only : identité canonique, absence de `ItemDefinitionAsset`, suppression des tags obsolètes et conservation de la présentation existante.

MIG09-B2C-B a été validé localement sous UE5.5.4 avec le filtre `Grimrock.WorldObjects` : `33` succès, `1` warning, `0` échec, exit code `0`.

## 10. MIG09-C-A — audit des anciens initializers de mécanismes

Avant de supprimer les derniers helpers directs des mécanismes, MIG09-C-A ajoute un audit Editor des Blueprints sous `/Game/GrimrockPrototype` pour :

```text
InitializeButton(...)
InitializeLever(...)
InitializeDoor(...)
```

Le Pit n'a plus d'initializer direct dédié : `AGridPitTrapdoorActor` utilise déjà `InitializeMechanismVisuals()` puis `InitializeGridObject()`.

Le test :

```text
Grimrock.WorldObjects.MIG09.MechanismLegacyBlueprintReferences
```

charge tous les `UBlueprint`, parcourt les `UK2Node_CallFunction` et échoue si l'un de ces trois anciens noms est encore appelé.

L'inventaire C++ avant purge montre :

- `InitializeButton()` : aucun consommateur C++ externe à `AGridButtonActor` ;
- `InitializeLever()` : aucun consommateur C++ externe à `AGridLeverActor` ;
- `InitializeDoor()` : encore utilisé par des tests directs (`GridDoorAudioFeedbackTests.cpp`, `GridDoorPassageBlockingTests.cpp`, `GridMonsterMON14_3Tests.cpp`, `GridMonsterAcousticHearingTests.cpp`, `GridMON198ProductionPuzzleTests.cpp`).

Les champs pré-MIG04 à supprimer dans les sous-tranches suivantes sont les bridges `Transient` de `GridObjectBehavior.h` :

```text
FGridPitAnimationParams
FGridButtonAnimationParams.ButtonPressDistance
FGridButtonAnimationParams.ButtonPressDuration
FGridButtonAnimationParams.ButtonReleaseDuration
FGridLeverAnimationParams
FGridPressurePlateAnimationParams
FGridDoorAnimationParams.OpenHeight
FGridDoorAnimationParams.MoveDuration
```

Les états runtime `Transient` nécessaires aux acteurs (audio, alpha courant, état d'animation, compteurs de diagnostic) ne font pas partie de cette purge.

MIG09-C-A est candidat et doit être validé localement sous UE5.5.4 avec le filtre `Grimrock.WorldObjects`.

## 11. Ponts encore à supprimer après MIG09-C-A

```text
UGridLevelAsset::Objects
FGridLevelObjectData
SparseBehaviorOverrideObjectIds
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
GridLevelPlacementCompatibility
InitializeButton()
InitializeLever()
InitializeDoor()
champs d'animation Transient pré-MIG04
anciens contrôles Slate qui éditent encore ces champs
```

## 12. Ordre de purge retenu

```text
MIG09-A       supprimer l'usage runtime du marqueur sparse pré-MIG08         ✅ validé
MIG09-B1      supprimer le stockage AGridItemActor::ArchetypeId              ✅ validé
MIG09-B2A     supprimer les miroirs ItemArchetypeId des réceptacles           ✅ validé
MIG09-B2B1    retirer ArchetypeId du schéma SaveGame runtime                  ✅ validé
MIG09-B2B2    réécrire les consommateurs SaveGame                             ✅ validé
MIG09-B2B3    supprimer proxy C++ et cache spawn ItemArchetypeId               ✅ validé
MIG09-B2C-A   auditer les références Blueprint aux aliases Item               ✅ validé
MIG09-B2C-B   supprimer InitializeItem/GetItemArchetypeId après audit vert    ✅ validé
MIG09-C-A     auditer les anciens initializers Blueprint de mécanismes         ⏳ candidat
MIG09-C-B     migrer les consommateurs C++ puis supprimer les initializers
MIG09-C-C     supprimer les bridges d'animation Transient et contrôles Slate
MIG09-D       migrer les derniers consommateurs vers les placements typés
MIG09-E       supprimer Objects / FGridLevelObjectData / compatibility projection
```

Chaque tranche doit laisser `master` compilable et être validée avec le filtre `Grimrock.WorldObjects` avant la suivante.
