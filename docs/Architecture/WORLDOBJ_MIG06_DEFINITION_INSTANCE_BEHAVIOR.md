# WORLDOBJ-MIG06 — Definition vs Instance : comportement sparse

## Statut

MIG06-A a introduit le stockage sparse et le pont de compatibilité des niveaux historiques.

MIG06-B finalise le contrat C++ : les règles partagées sont résolues depuis `UGridObjectArchetypeAsset::DefaultBehavior`, tandis que le `LevelAsset` ne conserve que les données réellement propres à l'instance.

Validation UE5.5.4 de la clôture MIG06 : **à effectuer après publication du commit MIG06-B**.

---

## Problème supprimé

Avant MIG06, le placement faisait conceptuellement :

```text
PlacedObject.Behavior = Definition.DefaultBehavior
```

Chaque instance conservait donc une copie complète de la définition. Une modification ultérieure du Data Asset ne se propageait plus naturellement aux objets déjà placés.

MIG06 remplace ce modèle par :

```text
Definition.DefaultBehavior
        +
Instance sparse overrides
        =
Effective Behavior
```

Il n'existe plus deux autorités de comportement pour un nouvel objet.

---

## Données stockées par instance

`GridObjectInstanceBehavior::BuildSparseOverrides()` ne conserve que :

- `Teleporter` : destination locale ;
- `Transition` : destination/configuration locale ;
- `Pit` : état/configuration initiale propre au niveau ;
- `Receptacle.InitialContent` : contenu initial du réceptacle placé ;
- `Lock.bStartsUnlocked` : état initial local de la serrure.

Les champs déjà séparés dans `FGridLevelObjectData` restent eux aussi naturellement locaux : position, bord, état enabled/active, Tag, LogicId, texte lisible surchargé, références directes d'item/monstre/compagnon, etc.

---

## Données appartenant exclusivement à la définition

Pour une instance sparse, les familles suivantes sont désormais reprises depuis `DefaultBehavior` :

- `ButtonAnimation.ButtonHoldTime` ;
- règles de serrure : clés acceptées, consommation, messages ;
- règles génériques de réceptacle : acceptation, capacité, mode de placement, physique ;
- `PressurePlateWeight` ;
- chaîne de porte : présence, distance et durée de traction ;
- plus généralement tout champ partagé non explicitement listé parmi les overrides d'instance.

Les paramètres géométriques de mécanismes restent quant à eux sous l'autorité de `StaticPart` / `MovingParts[].Motion`, conformément à MIG03/MIG04.

---

## Resolver runtime commun

`AGridRuntimeObjectActor` expose :

```cpp
FGridObjectBehaviorParams ResolveEffectiveBehavior(const FGridLevelObjectData& ObjectData) const;
```

En runtime normal, l'acteur est possédé par `AGridLevelRuntimeActor`. Le resolver récupère donc :

1. le `LevelAsset` courant ;
2. l'archétype par `ArchetypeId` ;
3. le statut sparse de l'`ObjectId` ;
4. le comportement effectif via `GridObjectInstanceBehavior::Resolve()`.

Les appels C++ historiques qui instancient directement un acteur sans `AGridLevelRuntimeActor` propriétaire restent automatiquement en mode snapshot legacy. Cela évite de casser les anciens tests avant MIG08/MIG09.

---

## Consommateurs migrés en MIG06-B

### Pressure Plate

`AGridPressurePlateActor` et `UGridActivationComponent` utilisent le comportement effectif pour :

- `bActivateWhenPartyPresent` ;
- `bUseItemWeight` ;
- `RequiredItemWeight` ;
- `bCountEdgeItems`.

### Door

`AGridDoorActor::InitializeGridObject()` résout la chaîne de porte depuis la définition.

Le chemin `InitializeDoor()` reste uniquement un helper de compatibilité directe jusqu'à MIG09.

### Receptacle

`AGridReceptacleActor` résout depuis la définition :

- `bAcceptAnyItem` ;
- `AcceptedItems` ;
- `MaxContainedItems` ;
- `VisualPlacementMode` ;
- `bSimulatePhysicsWhenPlaced` ;
- offsets de placement physique.

`InitialContent` reste une donnée d'instance et est réappliqué par le resolver.

### Wall Lock

L'audit MIG06-B a identifié une dépendance supplémentaire : `AGridWallLockActor` lisait encore directement `ObjectData.Behavior.Lock`.

Il utilise désormais le comportement effectif pour :

- clés acceptées ;
- règle de consommation ;
- messages verrouillé/déverrouillé/clé manquante ;

alors que `bStartsUnlocked` reste l'override local de l'instance.

---

## Grid Editor

Les nouveaux placements sont marqués dans :

```text
SparseBehaviorOverrideObjectIds
```

`GetSelectedObjectData()` fournit désormais à l'inspecteur une **vue temporaire résolue** de l'objet sélectionné. Les widgets peuvent donc continuer à manipuler un `FGridLevelObjectData` complet sans que cette vue soit sérialisée telle quelle dans le niveau.

Lorsqu'une modification est appliquée, `ApplyBehaviorToSelectedObject()` repasse par `BuildSparseOverrides()` : les valeurs appartenant à la définition ne sont donc pas réintroduites comme copies locales.

La validation d'archétype contrôle les règles partagées de réceptacle. La validation d'instance ne doit conserver que les vérifications liées aux données locales, notamment `InitialContent`.

---

## Compatibilité des anciens niveaux

MIG08 n'ayant pas encore réenregistré les `.uasset`, l'absence de l'`ObjectId` dans `SparseBehaviorOverrideObjectIds` signifie :

```text
objet pré-MIG06
=> ObjectData.Behavior reste son snapshot historique complet
```

La présence de l'`ObjectId` signifie :

```text
objet MIG06+
=> Definition.DefaultBehavior + sparse overrides
```

Cette distinction disparaîtra lorsque la migration de contenu réelle sera terminée et que les ponts historiques seront purgés en MIG09.

---

## Tests MIG06

Famille :

```text
Grimrock.WorldObjects.MIG06
```

Les tests vérifient notamment :

- la priorité de la définition sur les champs partagés ;
- la conservation des données réellement locales ;
- le comportement legacy d'un objet pré-MIG06 ;
- l'absence de copie de `PressurePlateWeight` ;
- l'absence de copie des règles de chaîne de porte ;
- l'absence de copie des règles génériques de réceptacle ;
- l'absence de copie des règles de serrure autres que l'état initial ;
- la conservation de `Receptacle.InitialContent` ;
- le placement et la sélection sparse dans le Grid Editor.

---

## Critère de clôture

MIG06 peut être déclaré clos lorsque :

1. le build UE5.5.4 passe ;
2. `Grimrock.WorldObjects` ne contient aucun échec ;
3. les nouveaux placements ne recopient plus les règles partagées ;
4. le runtime et l'inspecteur obtiennent ces règles via la définition ;
5. les anciens niveaux restent lisibles jusqu'à MIG08.

La suite est alors **MIG07 — scinder `FGridLevelObjectData` en structures typées**.
