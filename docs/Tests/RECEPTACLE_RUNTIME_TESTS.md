# Tests runtime manuels des réceptacles

Statut : spécification de consolidation, protocole PIE et résultats validés.

## Validation après simplification

Les tests suivants ont été validés en PIE après le commit
`592170e11a81a06115a6c99a139499f781e28d11` :

1. support de torche vide : torche acceptée ;
2. support de torche : pierre refusée ;
3. support de torche rempli : retrait de la torche réussi ;
4. alcôve vide : pierre acceptée ;
5. alcôve vide : torche acceptée ;
6. alcôve avec `InitialContent` Stone + Torch : objets visibles et non superposés ;
7. alcôve : retrait des objets réussi ;
8. alcôve pleine : refus `Full` ;
9. `ReceptacleDisableRemoval` : retrait bloqué ;
10. `ReceptacleEnableRemoval` : retrait de nouveau possible ;
11. `ReceptacleConsumeItem` : consommation réussie même lorsque le retrait est bloqué.

Ces résultats confirment que `bCanRemoveItem` est l'unique autorité du retrait
joueur et que la consommation par mécanisme en est indépendante.

## Objectif

Ce document définit les assets de test attendus et cinq scénarios PIE permettant de
valider ensemble :

- l'acceptation universelle ou par asset de définition ;
- l'autorisation runtime de retrait ;
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

`UGridObjectArchetypeAsset::DefaultBehavior.Receptacle` expose l'acceptation, la
capacité, le contenu initial et le placement. Pour chaque preset :

1. Créer une sous-classe Blueprint de `BP_GridReceptacleActor`.
2. Régler dans ses Class Defaults les propriétés runtime indiquées.
3. Créer un `UGridObjectArchetypeAsset`.
4. Régler `SupportedType=Receptacle`, `ObjectCategory=Receptacle`,
   `bIsInteractable=true` et `RuntimeActorClass` sur le Blueprint du preset.
5. Régler `DefaultBehavior.Receptacle` selon le tableau ci-dessous.
6. Ajouter l'archétype à `DA_ObjectPalette_Default` et au tableau
   `ObjectArchetypes` du `BP_GridLevelRuntimeActor` de la map de test.

Attention : `InitializeGridObject` remet actuellement `bCanRemoveItem` à `true`.
Utiliser les commandes `ReceptacleDisableRemoval` et `ReceptacleEnableRemoval`
pour modifier l'état pendant le test.

## Presets de réceptacle attendus

### TorchHolder

Assets :

```text
BP_Receptacle_TorchHolder
DA_Receptacle_TorchHolder
ArchetypeId = TorchHolder
```

Class Defaults :

```text
VisualPlacementMode = AttachedSocket
bAcceptAnyItem = false
MaxContainedItems = 1
bSimulatePhysicsWhenPlaced = false
```

Archetype :

```text
PlacementKind = Wall
Category = Receptacles
DefaultBehavior.Receptacle.bAcceptAnyItem = false
DefaultBehavior.Receptacle.AcceptedItems = [DA_Item_Torch]
DefaultBehavior.Receptacle.MaxContainedItems = 1
DefaultBehavior.Receptacle.VisualPlacementMode = AttachedSocket
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
VisualPlacementMode = PhysicalAtHit
bAcceptAnyItem = true
MaxContainedItems = 4
bSimulatePhysicsWhenPlaced = true
PhysicalPlacementSurfaceOffset = 5.0
```

Archetype :

```text
PlacementKind = Wall
Category = Receptacles
DefaultBehavior.Receptacle.bAcceptAnyItem = true
DefaultBehavior.Receptacle.MaxContainedItems = 4
DefaultBehavior.Receptacle.VisualPlacementMode = PhysicalAtHit
DefaultBehavior.Receptacle.bSimulatePhysicsWhenPlaced = true
DefaultBehavior.Receptacle.PhysicalPlacementSurfaceOffset = 5.0
```

### GemSocket_RemovalControlled

Assets :

```text
BP_Receptacle_GemSocket_RemovalControlled
DA_Receptacle_GemSocket_RemovalControlled
ArchetypeId = GemSocket_RemovalControlled
```

Class Defaults :

```text
VisualPlacementMode = AttachedSocket
bAcceptAnyItem = true
MaxContainedItems = 1
```

Archetype :

```text
PlacementKind = Wall
Category = Mechanisms
DefaultBehavior.Receptacle.bAcceptAnyItem = true
DefaultBehavior.Receptacle.MaxContainedItems = 1
```

Ce preset valide le contrôle du retrait par les commandes dédiées. Les filtres
détaillés ne font plus partie du modèle actuel.

### WeightPlate_Receptacle

Assets :

```text
BP_Receptacle_WeightPlate
DA_Receptacle_WeightPlate
ArchetypeId = WeightPlate_Receptacle
```

Class Defaults :

```text
VisualPlacementMode = PhysicalAtHit
bAcceptAnyItem = false
MaxContainedItems = 4
bSimulatePhysicsWhenPlaced = false
```

Archetype :

```text
PlacementKind = Floor
Category = Mechanisms
DefaultBehavior.Receptacle.bAcceptAnyItem = true
DefaultBehavior.Receptacle.MaxContainedItems = 4
DefaultBehavior.Receptacle.VisualPlacementMode = PhysicalAtHit
```

### SecretAltar

Assets :

```text
BP_Receptacle_SecretAltar
DA_Receptacle_SecretAltar
ArchetypeId = SecretAltar
```

Class Defaults :

```text
VisualPlacementMode = AttachedSocket
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

La consommation est déclenchée explicitement par `ReceptacleConsumeItem` ou
`ReceptacleConsumeAllItems`.

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
4. Ajouter les cinq archétypes de réceptacle à `ObjectArchetypes`.
5. Ajouter les définitions d'items aux objets Item placés dans le LevelAsset via
   `ItemDefinitionAsset` et `ItemDefinitionId`.
6. Placer les cinq réceptacles dans des cellules séparées et accessibles.
7. Placer un bouton de test près du `SecretAltar`.
8. Placer un bouton `GemSocketEnableRemoval` et un bouton `WeightPlateReset`.
9. Vérifier que chaque `ObjectId` est un GUID unique.

Disposition conseillée :

| Zone | Cellule | Objet |
|---|---|---|
| A | `(2,2)` mur nord | `TorchHolder` |
| B | `(4,2)` mur nord | `Alcove_AnyItem` |
| C | `(6,2)` mur nord | `GemSocket_RemovalControlled` |
| E | `(4,5)` sol | `WeightPlate_Receptacle` |
| F | `(6,5)` sol | `SecretAltar` |
| F | `(6,6)` mur | bouton `SecretAltarTrigger` |
| C | `(7,2)` mur | bouton `GemSocketEnableRemoval` |
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
Command = ReceptacleDisableRemoval
Condition = ReceptacleWeightAtLeast
ConditionWeight = 10.0
bInvertCondition = false
```

La pierre légère ne passe pas le seuil. La pierre lourde passe le seuil et
désactive le retrait du réceptacle.

### Lien de consommation inversée

Ajouter un lien du bouton vers `SecretAltar` :

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
SourceObjectId = GemSocketEnableRemovalButtonGuid
TargetObjectId = GemSocketGuid
SourceEvent = Activated
Command = ReceptacleEnableRemoval
Condition = None
```

```text
SourceObjectId = WeightPlateResetButtonGuid
TargetObjectId = WeightPlateGuid
SourceEvent = Activated
Command = ReceptacleEnableRemoval
Condition = None
```

Pour observer les conditions échouées, lancer PIE avec :

```text
Log LogTemp Verbose
```

## Scénarios PIE

### A. Support de torche

1. Équiper ou ramasser `Torch_Test`.
2. Interagir avec `TorchHolder`.
3. Vérifier que la torche est présente dans `AcceptedItems`, acceptée par sa
   définition et attachée au support.
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

### C. Gem socket avec retrait contrôlé

1. Essayer `Sword_Test` et vérifier le refus.
2. Insérer `Gem_Red_Test`.
3. Vérifier `ContainsItemTag(RedGem)=true` et `ContainsItemType(Gem)=true`.
4. Déclencher `ReceptacleDisableRemoval`, puis tenter de retirer la gemme.
5. Vérifier que le retrait est refusé avec `bCanRemoveItem=false`.
6. Appuyer sur `GemSocketEnableRemoval`, qui exécute
   `ReceptacleEnableRemoval`, puis
   retirer la gemme.

Résultat attendu : filtre gemme opérationnel, retrait bloqué par
`ReceptacleDisableRemoval`, puis réactivé par `ReceptacleEnableRemoval`.

### E. Condition de poids

1. Insérer `Stone_Light_Test` dans `WeightPlate_Receptacle`.
2. Vérifier que la condition `WeightAtLeast(10.0)` échoue et que le retrait reste
   possible.
3. Retirer la pierre légère.
4. Insérer `Stone_Heavy_Test`.
5. Vérifier que le lien exécute `ReceptacleDisableRemoval`.
6. Tenter le retrait et vérifier qu'il est refusé.
7. Appuyer sur `WeightPlateReset` pour réinitialiser le preset.

Résultat attendu : le poids total utilise `Weight * Quantity`, et seul le seuil
atteint déclenche la commande.

### F. Condition inversée

1. Insérer `Stone_Light_Test` dans `SecretAltar`.
2. Appuyer sur `SecretAltarTrigger`.
3. Vérifier que `ContainsItemTag(RedGem)` vaut faux, puis que l'inversion rend la
   condition vraie et consomme la pierre.
4. Insérer `Gem_Red_Test`.
5. Appuyer de nouveau sur le bouton.
6. Vérifier que la condition brute vaut vrai, que l'inversion la rend fausse et
   que la gemme reste dans l'autel.
7. Retirer la gemme ou consommer manuellement le contenu pour réinitialiser.

Résultat attendu : `bInvertCondition` est appliqué après l'évaluation et la
consommation ne se produit qu'avec une commande explicite.

## Checklist de non-régression

- Le support de torche utilise `AttachedSocket`, `MaxContainedItems=1`,
  `bAcceptAnyItem=false` et `AcceptedItems=[DA_Item_Torch]`.
- `DA_Item_Torch` est accepté par le support de torche.
- `DA_Item_Stone` est refusé par le support de torche sans perte ni dépôt au sol.
- L'alcôve utilise `PhysicalAtHit`, `MaxContainedItems=8` et
  `bAcceptAnyItem=true`.
- L'alcôve accepte une torche puis une pierre, dans la limite de sa capacité.
- Une alcôve avec `InitialContent` contenant une pierre et une torche crée les
  deux entrées avec leur quantité.
- Les items initiaux multiples utilisent les offsets déterministes autour de
  `ItemAttachPoint` et ne sont pas superposés.
- Un dépôt manuel dans l'alcôve utilise le point cliqué et l'offset de surface,
  pas l'offset des items initiaux.
- Une insertion au-delà de `MaxContainedItems` est refusée avec `Full`.
- Avec le curseur vide, cliquer directement un item contenu le retire.
- Les refus ne suppriment ni ne déposent les items.
- Le `CursorSlot` n'est pas requis pour les transferts service testés.
- Aucun coffre ni UI de conteneur n'est utilisé.
- Les liens sans condition (`Condition=None`) continuent de s'exécuter.
- Les conditions échouées ne déclenchent aucune commande.
- Les logs de condition restent en `Verbose`.

## Validation finale

Le GO 10 est validé lorsque les cinq scénarios passent dans
`L_ReceptacleTest`, que la compilation Editor réussit et que les assets créés
dans Unreal Editor sont sauvegardés puis ajoutés à Git dans un commit dédié.
