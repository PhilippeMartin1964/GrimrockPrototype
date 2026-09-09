# GrimrockPrototype — Définitions d’objets

Statut : **modèle actif après WORLDOBJ-MIG10 et ALIGN-A — 2026-09-09**.

Ce document donne la vue d’ensemble du modèle courant. La référence détaillée des champs de `UGridWorldObjectDefinitionAsset` reste [11_GRID_WORLD_OBJECT_DEFINITION_PARAMETERS_REFERENCE.md](11_GRID_WORLD_OBJECT_DEFINITION_PARAMETERS_REFERENCE.md).

## 1. Principe : une définition canonique par nature d’objet

Le projet ne doit pas créer deux DataAssets pour représenter une seule chose ramassable.

Il existe plusieurs familles de définitions, chacune avec une responsabilité distincte :

| Nature | Définition canonique | Placement palette |
|---|---|---|
| Objet fixe ou mécanisme de grille | `UGridWorldObjectDefinitionAsset` | `DefaultWorldObjectDefinition` |
| Item ramassable / transportable | `UGridItemDefinitionAsset` | `DefaultItemDefinition` |
| Monstre | `UGridMonsterDefinitionAsset` | `DefaultMonsterDefinition` lorsqu’un spawn en a besoin |
| Compagnon narratif | `URPGStoryCompanionAsset` | `DefaultStoryCompanionDefinition` lorsqu’un placement en a besoin |

Une entrée de palette collectible est donc une entrée **direct item** :

```text
FGridObjectPaletteEntry
  DefaultItemDefinition = DA_Item_XXX
  DefaultWorldObjectDefinition = None
```

`FGridObjectPaletteEntry::IsValidEntry()` impose ce contrat. Une entrée item directe ne doit pas avoir de définition world-object compagnon ni d’icône de palette séparée ; elle utilise la définition et l’icône de l’item.

## 2. Objets du monde

`UGridWorldObjectDefinitionAsset` définit les objets qui appartiennent structurellement au niveau : portes, boutons, leviers, plaques de pression, réceptacles, téléporteurs, pits, décorations fixes, lumières, spawns et autres mécanismes.

Les principaux groupes de données d’authoring sont :

```text
Definition
  DefinitionId
  DisplayName
  SupportedType
  Description

Palette
  Category

Definition
  ObjectCategory

Placement
  PlacementSurface = Floor | Wall | Ceiling
  DefaultLocalPosition = U / V / N

Spatial Behavior
  bBlocksMovement
  bOccupiesBoundary
  bReplacesStandardWall

Interaction / Light
  bIsInteractable
  bIsReadable
  ReadableText
  bIsLightSource
  ...

Visual
  StaticPart
  MovingParts[0..1]

Runtime
  RuntimeActorClass

Defaults / Behavior
  DefaultBehavior

Audio
  DefaultAudioAttenuation
  AudioEvents
```

### 2.1 Placement

L’autorité d’authoring est :

```text
PlacementSurface
DefaultLocalPosition.U
DefaultLocalPosition.V
DefaultLocalPosition.N
```

Les champs suivants existent encore comme **projection transiente interne** pour des consommateurs de transforms qui n’ont pas encore été entièrement rabattus sur le modèle U/V/N :

```text
PlacementKind
PlacementZOffset
WallInset
LocalOffsetAlongWall
LocalOffsetVertical
```

Ils ne sont pas des paramètres à authorer dans les DataAssets et ne doivent pas être présentés comme le modèle cible.

## 3. Items ramassables

Un collectible est défini **une seule fois** par `UGridItemDefinitionAsset`.

Chaîne canonique :

```text
DA_Item_XXX
  UGridItemDefinitionAsset
       |
       +--> DA_ObjectPalette_Default.DefaultItemDefinition
       |
       +--> FGridLooseItemInstance.ItemDefinition
       |
       +--> AGridItemActor
       |
       +--> inventaire / équipement / dépôt / réceptacle
```

Il ne faut plus créer :

```text
DA_Object_XXXPickup
UGridWorldObjectDefinitionAsset compagnon
```

Les anciens `DA_Object_BlueGemPickup`, `DA_Object_KeyCopperPickup`, `DA_Object_KeyIronPickup`, `DA_Object_ShurikenPickup`, `DA_Object_StonePickup` et `DA_Object_TestNotePickup` ont été retirés par ALIGN-A6 après audit AssetRegistry et nettoyage de `L_GrimrockEditor`.

### 3.1 Exemple : torche

La torche placée au sol est l’item lui-même :

```text
DA_Item_Torch
  ItemDefinitionId
  DisplayName
  ItemType
  WorldMesh
  Icon
  règles gameplay
```

`DA_Item_Torch` **n’a pas** de `SupportedType`, car ce champ appartient à `UGridWorldObjectDefinitionAsset`. Dans la palette, la tuile de torche doit référencer `DefaultItemDefinition = DA_Item_Torch` et laisser `DefaultWorldObjectDefinition` vide.

Un éventuel générateur de torches est un mécanisme distinct d’une torche déjà présente au sol.

## 4. Réceptacles

Le réceptacle lui-même est un objet du monde et utilise `UGridWorldObjectDefinitionAsset`.

Son contenu et ses filtres pointent directement vers les définitions d’items :

```text
DefaultBehavior.Receptacle
  bAcceptAnyItem
  AcceptedItems[].ItemDefinition
  InitialContent[].ItemDefinition
  InitialContent[].Quantity
  MaxContainedItems
  VisualPlacementMode
```

Les anciens concepts `RequiredItemId` / `RequiredItemTag` ne constituent pas le contrat courant du réceptacle générique.

Une serrure spécialisée utilise `DefaultBehavior.Lock.AcceptedKeyItems` et, lorsqu’un fallback d’identifiant est volontairement nécessaire, `AcceptedKeyIds`.

## 5. Preview et runtime

La palette est l’autorité de la liste de définitions world-object du preview éditeur.

Depuis ALIGN-A4, `SyncPreviewRuntimeWorldObjectDefinitionsFromPalette()` reconstruit la projection canonique à partir des `DefaultWorldObjectDefinition` présentes dans la palette puis remplace la liste du preview si elle diffère. Une définition retirée de la palette ne doit donc plus rester sérialisée indéfiniment dans `L_GrimrockEditor`.

Les items directs ne sont pas ajoutés à `PreviewRuntimeActor.WorldObjectDefinitions` : leur identité passe par `DefaultItemDefinition`.

## 6. Règle d’évolution

Créer une nouvelle classe C++ uniquement si le comportement ne peut pas être exprimé proprement par les paramètres existants ou si le runtime nécessite réellement une logique spécialisée.

Pour un nouvel item, commencer par `UGridItemDefinitionAsset` et **ne pas** créer de world-object compagnon.

Pour un nouvel objet structurel ou mécanisme, commencer par `UGridWorldObjectDefinitionAsset` et réutiliser une classe runtime existante lorsque cela suffit.

## 7. Références

- [11_GRID_WORLD_OBJECT_DEFINITION_PARAMETERS_REFERENCE.md](11_GRID_WORLD_OBJECT_DEFINITION_PARAMETERS_REFERENCE.md) — référence détaillée des paramètres world-object.
- [ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md](ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md) — création des items ; son annexe historique est explicitement non normative.
- [../Architecture/WORLDOBJ_MIG10_FINAL.md](../Architecture/WORLDOBJ_MIG10_FINAL.md) — clôture de la migration de vocabulaire et du modèle typé.
- [../Architecture/ALIGN_A_DEAD_CODE_AND_ASSET_HYGIENE_REPORT.md](../Architecture/ALIGN_A_DEAD_CODE_AND_ASSET_HYGIENE_REPORT.md) — nettoyage de code mort et d’assets réalisé après MIG10.
