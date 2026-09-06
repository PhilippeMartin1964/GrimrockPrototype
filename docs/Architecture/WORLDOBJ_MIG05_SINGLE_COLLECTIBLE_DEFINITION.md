# WORLDOBJ-MIG05 — Une définition unique par collectible

## Statut

Implémentation C++ de la cible MIG05. La migration/résauvegarde des assets existants est volontairement reportée à WORLDOBJ-MIG08.

## Règle cible

Un objet transportable n'est plus défini par un couple :

```text
UGridObjectArchetypeAsset
  +
UGridItemDefinitionAsset
```

La définition canonique est désormais :

```text
UGridItemDefinitionAsset
```

Le même asset porte déjà l'identité et la présentation nécessaires à toutes les formes d'un collectible :

- `ItemDefinitionId` ;
- nom et description ;
- type, poids, empilement ;
- équipement et actions de combat ;
- `Icon` ;
- `WorldMesh` ;
- `EquippedMesh` ;
- paramètres de lancer ;
- physique monde ;
- scintillement ;
- lumière et tags.

Il n'existe donc aucune raison de créer un `UGridObjectArchetypeAsset` uniquement pour permettre à une gemme, une clé, une pierre ou une potion d'exister au sol.

## Palette

`FGridObjectPaletteEntry` accepte maintenant deux familles exclusives de définition :

```text
World object
└─ DefaultArchetype : UGridObjectArchetypeAsset

Collectible
└─ DefaultItemDefinition : UGridItemDefinitionAsset
```

Lorsqu'une entrée utilise `DefaultItemDefinition` :

- `GetEffectiveObjectType()` retourne `Item` ;
- `GetEffectiveArchetypeId()` retourne `None` ;
- la catégorie par défaut est `Items` ;
- le nom d'affichage vient de l'ItemDefinition ;
- l'icône affichée dans la palette vient de `ItemDefinition.Icon` ;
- `PaletteEntry.Icon` doit rester vide afin d'éviter une seconde autorité visuelle ;
- aucun `DefaultArchetype` n'est requis.

Une entrée qui référence simultanément `DefaultItemDefinition` et `DefaultArchetype` est invalide : cela recréerait exactement la double autorité que MIG05 supprime.

De même, une entrée collectible directe qui renseigne aussi `PaletteEntry.Icon` est invalide : l'icône du collectible appartient à l'ItemDefinition, au même titre que son `WorldMesh`.

## Grid Editor

`AGridLevelEditorActor::ApplyPaletteEntry()` possède maintenant un chemin direct collectible.

Une entrée ItemDefinition sélectionnée prépare :

```text
PaintObjectType       = Item
ObjectArchetypeId     = None
SelectedArchetypeId   = None
ObjectBehavior.Item.ItemDefinitionAsset = DefaultItemDefinition
```

`ObjectBehavior` sert ici uniquement d'état de peinture temporaire de l'éditeur. Lors du placement, `PlaceSelectedObject()` copie déjà la référence canonique dans :

```text
FGridLevelObjectData::ItemDefinitionAsset
```

et garde :

```text
ItemDefinitionId = None
ArchetypeId      = None
```

pour un collectible créé selon le nouveau chemin.

Les anciennes entrées de palette basées sur un archétype continuent de fonctionner jusqu'à la migration des `.uasset` en MIG08.

## Preview éditeur

`UGridEditorPreviewComponent` reconnaît désormais un `Item` possédant directement `ItemDefinitionAsset`.

Le preview charge :

```text
ItemDefinitionAsset.WorldMesh
```

et utilise `AGridEditorPreviewObjectActor::InitializePreviewObject()`.

Ainsi le preview et le runtime affichent le même `WorldMesh` issu de la même définition, sans WorldObjectDefinition compagnon.

## Runtime

Le runtime supportait déjà presque entièrement le contrat cible avant MIG05 :

- `RebuildRuntimeObjects()` route `Type=Item` vers `AddPlacedItemActor()` avant la logique des world objects ;
- `ResolveObjectItemDefinitionAsset()` privilégie déjà `FGridLevelObjectData::ItemDefinitionAsset` ;
- `SpawnItemActorForDefinition()` fournit la factory générique ;
- `AGridItemActor::InitializeFromItemDefinition()` charge `WorldMesh` depuis l'ItemDefinition ;
- `GetObjectPlacementTransform()` possède déjà un chemin Item sans archetype pour le centre de cellule et le bord de cellule.

MIG05 ne remplace donc pas ce runtime fonctionnel : il rend enfin la palette et le preview capables d'emprunter ce chemin directement.

`AGridItemActor::ArchetypeId` est conservé temporairement comme miroir de compatibilité MIG09, mais il est maintenant :

- `Transient` ;
- visible en diagnostic seulement ;
- non éditable ;
- ignoré par l'interaction, le scintillement et le nudge physique.

L'identité runtime canonique est `ItemDefinitionId`, dérivée prioritairement de `ItemDefinitionAsset`.

## Réceptacles et ItemSpawn

Les réceptacles utilisent déjà des références directes `UGridItemDefinitionAsset` pour leurs contenus acceptés et initiaux. Cette partie est donc conforme à la cible MIG05.

`FGridLevelObjectData::ItemDefinitionAsset` est également le support direct prévu pour `ItemSpawn`. Le comportement commandé historique d'`ItemSpawn` reste hors du périmètre de MIG05 ; MIG05 normalise la définition référencée, pas l'implémentation d'une nouvelle commande de spawn.

## Compatibilité de migration

Avant MIG08, les assets de contenu existants peuvent encore contenir :

```text
Item palette entry
└─ DefaultArchetype
   └─ DefaultBehavior.Item.ItemDefinitionAsset
```

Ce chemin est conservé comme pont de lecture afin de ne pas casser les niveaux/assets avant leur migration réelle.

Les nouvelles données doivent utiliser :

```text
Item palette entry
└─ DefaultItemDefinition
```

La suppression physique des derniers champs/fallbacks historiques appartient à MIG09, après migration des assets.

## Tests

Famille :

```text
Grimrock.WorldObjects.MIG05
```

Contrats couverts :

- une entrée palette collectible est valide sans `DefaultArchetype` ;
- une entrée ne peut pas posséder simultanément ItemDefinition et ObjectArchetype ;
- une entrée collectible directe ne peut pas dupliquer `ItemDefinition.Icon` dans `PaletteEntry.Icon` ;
- `AGridItemActor::ArchetypeId` n'est plus une donnée d'authoring ;
- un item direct sans `ArchetypeId` obtient un transform monde valide ;
- le Generic Item Actor prend identité et `WorldMesh` dans l'ItemDefinition ;
- le Grid Editor peut sélectionner et placer une ItemDefinition directement.