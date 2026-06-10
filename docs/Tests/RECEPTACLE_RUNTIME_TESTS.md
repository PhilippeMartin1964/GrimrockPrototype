# Tests runtime manuels des réceptacles

Statut : spécification de consolidation et protocole PIE pour les GO 1 à 10.

## Objectif

Ce document définit les assets de test attendus et six scénarios PIE permettant de
valider ensemble :

- l'acceptation par définition, tag et type ;
- les politiques `Returnable`, `Locked`, `ConsumeOnInsert` et `ConsumeOnTrigger` ;
- le placement `AttachedSocket` et `PhysicalAtHit` ;
- les commandes de réceptacle ;
- les conditions de lien, les seuils de poids et l'inversion.

Les assets Unreal (`.uasset` et `.umap`) ne doivent pas être fabriqués manuellement
hors de l'Unreal Editor. La procédure ci-dessous est la source versionnée pour
créer les presets et la map `L_ReceptacleTest`.

## Emplacements recommandés

```text
/Game/GrimrockPrototype/Core/DataAssets/Items/Test
/Game/GrimrockPrototype/Core/DataAssets/Receptacles/Test
/Game/GrimrockPrototype/Blueprints/Runtime/Receptacles/Test
/Game/GrimrockPrototype/Maps/L_ReceptacleTest
```

Assets existants réutilisables :

```text
/Game/GrimrockPrototype/Blueprints/Runtime/BP_GridReceptacleActor
/Game/GrimrockPrototype/Blueprints/Runtime/BP_Receptacle_WallTorchHolder
/Game/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder
/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch
/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default
/Game/GrimrockPrototype/Maps/L_GrimrockRuntime
```

## Règle de création des presets

`UGridObjectArchetypeAsset::DefaultBehavior.Receptacle` expose les filtres legacy,
la capacité et le placement physique, mais pas encore toutes les propriétés des
GO récents. Pour chaque preset :

1. Créer une sous-classe Blueprint de `BP_GridReceptacleActor`.
2. Régler dans ses Class Defaults `ReceptacleKind`, `VisualPlacementMode`,
   `ItemPolicy`, `AcceptedItemTypes` et les autres propriétés runtime indiquées.
3. Créer un `UGridObjectArchetypeAsset`.
4. Régler `SupportedType=Receptacle`, `ObjectCategory=Receptacle`,
   `bIsInteractable=true` et `RuntimeActorClass` sur le Blueprint du preset.
5. Régler `DefaultBehavior.Receptacle` selon le tableau ci-dessous.
6. Ajouter l'archétype à `DA_ObjectPalette_Default` et au tableau
   `ObjectArchetypes` du `BP_GridLevelRuntimeActor` de la map de test.

Attention : `InitializeGridObject` remet actuellement `bCanInsertItem` et
`bCanRemoveItem` à `true`. Utiliser `ItemPolicy=Locked` pour garantir le verrouillage
initial et les commandes du GO 7 pour modifier l'état pendant le test.

## Presets de réceptacle attendus

### TorchHolder_Returnable

Assets :

```text
BP_Receptacle_TorchHolder_Returnable
DA_Receptacle_TorchHolder_Returnable
ArchetypeId = TorchHolder_Returnable
```

Class Defaults :

```text
ReceptacleKind = Presentation
StorageMode = SingleSlot
VisualPlacementMode = AttachedSocket
ItemPolicy = Returnable
bAcceptAnyItem = false
AcceptedItemTypes = [Torch]
MaxContainedItems = 1
bSimulatePhysicsWhenPlaced = false
```

Archetype :

```text
PlacementKind = Wall
Category = Receptacles
DefaultBehavior.Receptacle.bAcceptAnyItem = false
DefaultBehavior.Receptacle.AcceptedItemTags = [Torch]
DefaultBehavior.Receptacle.AcceptedArchetypeIds = [Torch_Test]
DefaultBehavior.Receptacle.MaxContainedItems = 1
DefaultBehavior.Receptacle.bUsePhysicalPlacement = false
```

Le mesh, le point d'attache et la classe d'item peuvent reprendre les valeurs de
`BP_Receptacle_WallTorchHolder` et `DA_Receptacle_TorchHolder`.

### Alcove_AnyItem

Assets :

```text
BP_Receptacle_Alcove_AnyItem
DA_Receptacle_Alcove_AnyItem
ArchetypeId = Alcove_AnyItem
```

Class Defaults :

```text
ReceptacleKind = Presentation
StorageMode = MultiSlot
VisualPlacementMode = PhysicalAtHit
ItemPolicy = Returnable
bAcceptAnyItem = true
MaxContainedItems = 4
bSimulatePhysicsWhenPlaced = false
PhysicalPlacementSurfaceOffset = 10.0
```

Archetype :

```text
PlacementKind = Wall
Category = Receptacles
DefaultBehavior.Receptacle.bAcceptAnyItem = true
DefaultBehavior.Receptacle.MaxContainedItems = 4
DefaultBehavior.Receptacle.bUsePhysicalPlacement = true
```

### GemSocket_Locked

Assets :

```text
BP_Receptacle_GemSocket_Locked
DA_Receptacle_GemSocket_Locked
ArchetypeId = GemSocket_Locked
```

Class Defaults :

```text
ReceptacleKind = Mechanism
StorageMode = SingleSlot
VisualPlacementMode = AttachedSocket
ItemPolicy = Locked
bAcceptAnyItem = false
AcceptedItemTypes = [Gem]
MaxContainedItems = 1
```

Archetype :

```text
PlacementKind = Wall
Category = Mechanisms
DefaultBehavior.Receptacle.bAcceptAnyItem = false
DefaultBehavior.Receptacle.AcceptedItemTags = [RedGem]
DefaultBehavior.Receptacle.MaxContainedItems = 1
```

Ce preset valide à la fois le filtre par tag et le filtre par type. Un item
explicitement ajouté à `RejectedItemArchetypeIds` doit rester refusé.

### OfferingBowl_ConsumeOnInsert

Assets :

```text
BP_Receptacle_OfferingBowl_ConsumeOnInsert
DA_Receptacle_OfferingBowl_ConsumeOnInsert
ArchetypeId = OfferingBowl_ConsumeOnInsert
```

Class Defaults :

```text
ReceptacleKind = Mechanism
StorageMode = MultiSlot
VisualPlacementMode = AttachedSocket
ItemPolicy = ConsumeOnInsert
bAcceptAnyItem = false
MaxContainedItems = 4
```

Archetype :

```text
PlacementKind = Floor
Category = Mechanisms
DefaultBehavior.Receptacle.bAcceptAnyItem = false
DefaultBehavior.Receptacle.AcceptedItemTags = [Offering]
DefaultBehavior.Receptacle.MaxContainedItems = 4
```

### WeightPlate_Receptacle

Assets :

```text
BP_Receptacle_WeightPlate
DA_Receptacle_WeightPlate
ArchetypeId = WeightPlate_Receptacle
```

Class Defaults :

```text
ReceptacleKind = Mechanism
StorageMode = MultiSlot
VisualPlacementMode = PhysicalAtHit
ItemPolicy = Returnable
bAcceptAnyItem = false
MaxContainedItems = 4
bSimulatePhysicsWhenPlaced = false
```

Archetype :

```text
PlacementKind = Floor
Category = Mechanisms
DefaultBehavior.Receptacle.bAcceptAnyItem = false
DefaultBehavior.Receptacle.AcceptedItemTags = [Stone]
DefaultBehavior.Receptacle.MaxContainedItems = 4
DefaultBehavior.Receptacle.bUsePhysicalPlacement = true
```

### SecretAltar_ConsumeOnTrigger

Assets :

```text
BP_Receptacle_SecretAltar
DA_Receptacle_SecretAltar
ArchetypeId = SecretAltar_ConsumeOnTrigger
```

Class Defaults :

```text
ReceptacleKind = Mechanism
StorageMode = MultiSlot
VisualPlacementMode = AttachedSocket
ItemPolicy = ConsumeOnTrigger
bAcceptAnyItem = true
MaxContainedItems = 4
```

Archetype :

```text
PlacementKind = Floor
Category = Mechanisms
DefaultBehavior.Receptacle.bAcceptAnyItem = true
DefaultBehavior.Receptacle.MaxContainedItems = 4
```

`ConsumeOnTrigger` conserve l'item. La consommation est déclenchée explicitement
par `ReceptacleConsumeItem` ou `ReceptacleConsumeAllItems`.

## Items de test attendus

Créer cinq `UGridItemDefinitionAsset`. Les poids ci-dessous sont les valeurs de
référence pour rendre le test de seuil déterministe.

| Asset | ItemDefinitionId | ItemType | ItemTags | Weight |
|---|---|---|---|---:|
| `DA_Item_Torch_Test` | `Torch_Test` | `Torch` | `Torch`, `LightSource` | 1.0 |
| `DA_Item_Gem_Red_Test` | `Gem_Red_Test` | `Gem` | `Gem`, `RedGem` | 0.5 |
| `DA_Item_Stone_Light_Test` | `Stone_Light_Test` | `Misc` | `Offering`, `Stone` | 2.0 |
| `DA_Item_Stone_Heavy_Test` | `Stone_Heavy_Test` | `Misc` | `Offering`, `Stone` | 12.0 |
| `DA_Item_Sword_Test` | `Sword_Test` | `Weapon` | `Weapon` | 5.0 |

Réglages complémentaires :

```text
Quantity par défaut = 1
bStackable = false
MaxStackSize = 1
```

Pour `Torch_Test` :

```text
bCanEmitLight = true
bDefaultLightEnabled = true
LightRadius = 600.0
```

Assigner un `WorldMesh` simple à chaque item. Il est acceptable de réutiliser un
mesh de debug existant ; le test porte sur la logique et non sur l'art final.

## Création de L_ReceptacleTest

Ne pas créer le fichier `.umap` hors de l'Unreal Editor.

1. Dupliquer `L_GrimrockRuntime` sous
   `/Game/GrimrockPrototype/Maps/L_ReceptacleTest`.
2. Dupliquer `DA_GridLevelAsset` sous
   `/Game/GrimrockPrototype/Core/DataAssets/Tests/DA_GridLevel_ReceptacleTest`.
3. Dans le `BP_GridLevelRuntimeActor` de la nouvelle map, assigner ce LevelAsset.
4. Ajouter les six archétypes de réceptacle à `ObjectArchetypes`.
5. Ajouter les définitions d'items aux objets Item placés dans le LevelAsset via
   `ItemDefinitionAsset` et `ItemDefinitionId`.
6. Placer les six réceptacles dans des cellules séparées et accessibles.
7. Placer un bouton de test près du `SecretAltar_ConsumeOnTrigger`.
8. Placer un bouton `GemSocketUnlock` et un bouton `WeightPlateReset`.
9. Vérifier que chaque `ObjectId` est un GUID unique.

Disposition conseillée :

| Zone | Cellule | Objet |
|---|---|---|
| A | `(2,2)` mur nord | `TorchHolder_Returnable` |
| B | `(4,2)` mur nord | `Alcove_AnyItem` |
| C | `(6,2)` mur nord | `GemSocket_Locked` |
| D | `(2,5)` sol | `OfferingBowl_ConsumeOnInsert` |
| E | `(4,5)` sol | `WeightPlate_Receptacle` |
| F | `(6,5)` sol | `SecretAltar_ConsumeOnTrigger` |
| F | `(6,6)` mur | bouton `SecretAltarTrigger` |
| C | `(7,2)` mur | bouton `GemSocketUnlock` |
| E | `(5,5)` mur | bouton `WeightPlateReset` |

Placer au moins une instance ramassable de chaque item de test. Pour répéter un
scénario sans relancer PIE, placer deux exemplaires des pierres et de la gemme.

## Configuration des liens conditionnels

Les champs du GO 9 sont stockés dans `FGridObjectLink`, mais ne sont pas encore
exposés dans le formulaire Slate de création des connecteurs. Créer d'abord le
lien dans l'outil, puis ouvrir le LevelAsset et régler directement son entrée
dans le tableau `Links`.

### Lien de seuil de poids

Ajouter un auto-lien sur `WeightPlate_Receptacle` :

```text
SourceObjectId = WeightPlateGuid
TargetObjectId = WeightPlateGuid
SourceEvent = ItemInserted
Command = ReceptacleLock
Condition = ReceptacleWeightAtLeast
ConditionWeight = 10.0
bInvertCondition = false
```

La pierre légère ne passe pas le seuil. La pierre lourde passe le seuil et
verrouille le réceptacle.

### Lien de consommation inversée

Ajouter un lien du bouton vers `SecretAltar_ConsumeOnTrigger` :

```text
SourceObjectId = SecretAltarButtonGuid
TargetObjectId = SecretAltarGuid
SourceEvent = Activated
Command = ReceptacleConsumeAllItems
Condition = ReceptacleContainsItemTag
ConditionItemTag = RedGem
bInvertCondition = true
```

Le bouton consomme le contenu si l'autel ne contient pas de `RedGem`. Il ne
consomme rien si une gemme rouge est présente.

### Liens de réinitialisation

Ajouter les deux liens sans condition :

```text
SourceObjectId = GemSocketUnlockButtonGuid
TargetObjectId = GemSocketGuid
SourceEvent = Activated
Command = ReceptacleUnlock
Condition = None
```

```text
SourceObjectId = WeightPlateResetButtonGuid
TargetObjectId = WeightPlateGuid
SourceEvent = Activated
Command = ReceptacleUnlock
Condition = None
```

Pour observer les conditions échouées, lancer PIE avec :

```text
Log LogTemp Verbose
```

## Scénarios PIE

### A. Support de torche returnable

1. Équiper ou ramasser `Torch_Test`.
2. Interagir avec `TorchHolder_Returnable`.
3. Vérifier que la torche est acceptée par type ou tag et attachée au support.
4. Interagir de nouveau sans item tenu.
5. Vérifier que la torche revient dans l'inventaire avec le même
   `RuntimeObjectId`.
6. Essayer `Sword_Test` et vérifier le refus sans perte de l'item.

Résultat attendu : insertion et retrait réussis, lumière et identité conservées.

### B. Alcôve libre

1. Insérer `Sword_Test` dans `Alcove_AnyItem`.
2. Insérer ensuite `Gem_Red_Test`.
3. Vérifier que les deux items restent dans `ContainedItems`.
4. Vérifier leur placement au point d'impact, sans simulation physique.
5. Retirer les items et vérifier que le contenu revient à zéro.

Résultat attendu : tout item valide est accepté jusqu'à la capacité, sans dépôt
au sol ni duplication.

### C. Gem socket locked

1. Essayer `Sword_Test` et vérifier le refus.
2. Insérer `Gem_Red_Test`.
3. Vérifier `ContainsItemTag(RedGem)=true` et `ContainsItemType(Gem)=true`.
4. Tenter de retirer la gemme.
5. Vérifier que le retrait est refusé même si `bCanRemoveItem=true`.
6. Appuyer sur `GemSocketUnlock`, puis retirer la gemme.

Résultat attendu : filtre gemme opérationnel, politique `Locked` prioritaire,
puis retrait possible après passage à `Returnable`.

### D. Offrande consume on insert

1. Insérer `Stone_Light_Test` dans `OfferingBowl_ConsumeOnInsert`.
2. Vérifier que l'insertion réussit avant la consommation.
3. Vérifier que `ContainedItems` revient immédiatement à zéro.
4. Vérifier que les événements d'insertion sont publiés avant la consommation,
   puis qu'un nouvel `ItemChanged` signale le contenu vide.
5. Essayer `Sword_Test` et vérifier le refus par absence de tag `Offering`.

Résultat attendu : l'offrande acceptée est consommée une seule fois ; l'item
refusé reste à sa source.

### E. Condition de poids

1. Insérer `Stone_Light_Test` dans `WeightPlate_Receptacle`.
2. Vérifier que la condition `WeightAtLeast(10.0)` échoue et que le retrait reste
   possible.
3. Retirer la pierre légère.
4. Insérer `Stone_Heavy_Test`.
5. Vérifier que le lien exécute `ReceptacleLock`.
6. Tenter le retrait et vérifier qu'il est refusé.
7. Appuyer sur `WeightPlateReset` pour réinitialiser le preset.

Résultat attendu : le poids total utilise `Weight * Quantity`, et seul le seuil
atteint déclenche la commande.

### F. Condition inversée

1. Insérer `Stone_Light_Test` dans `SecretAltar_ConsumeOnTrigger`.
2. Appuyer sur `SecretAltarTrigger`.
3. Vérifier que `ContainsItemTag(RedGem)` vaut faux, puis que l'inversion rend la
   condition vraie et consomme la pierre.
4. Insérer `Gem_Red_Test`.
5. Appuyer de nouveau sur le bouton.
6. Vérifier que la condition brute vaut vrai, que l'inversion la rend fausse et
   que la gemme reste dans l'autel.
7. Retirer la gemme ou consommer manuellement le contenu pour réinitialiser.

Résultat attendu : `bInvertCondition` est appliqué après l'évaluation et
`ConsumeOnTrigger` ne consomme rien sans commande explicite.

## Checklist de non-régression

- Le support de torche existant conserve son comportement `AttachedSocket`.
- Les refus ne suppriment ni ne déposent les items.
- Le `CursorSlot` n'est pas requis pour les transferts service testés.
- Aucun coffre ni UI de conteneur n'est utilisé.
- `DisplaySlots` n'est pas requis.
- Les liens sans condition (`Condition=None`) continuent de s'exécuter.
- Les conditions échouées ne déclenchent aucune commande.
- Les logs de condition restent en `Verbose`.

## Validation finale

Le GO 10 est validé lorsque les six scénarios passent dans
`L_ReceptacleTest`, que la compilation Editor réussit et que les assets créés
dans Unreal Editor sont sauvegardés puis ajoutés à Git dans un commit dédié.
