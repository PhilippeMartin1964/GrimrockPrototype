# Architecture des portes et mécanismes

> **Contrat courant (2026-09-12)** : placements typés, `UGridWorldObjectDefinitionAsset`, motion générique `MovingParts[].Motion` et états initiaux sémantiques. Voir aussi [Définitions et placements typés](WORLD_OBJECT_DEFINITIONS_AND_PLACED_OBJECTS.md).

Les items, le curseur et leur transfert vers les réceptacles sont documentés dans [`ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md`](ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md).

## 1. Objet du document

Ce document décrit la fondation qui relie une porte placée, sa définition réutilisable, son acteur runtime, les commandes issues des mécanismes, son animation et la passabilité de la grille.

Il ne décrit pas le système général d'inventaire ni les puzzles particuliers.

## 2. Vocabulaire

**Porte placée** : `FGridWorldObjectInstance` de type `Door`, persistée dans `UGridLevelAsset::WorldObjectInstances` et référant une `UGridWorldObjectDefinitionAsset` par `WorldObjectDefinitionId`.

**Arête de porte** : triplet cellule `CellX`, `CellY` et bord cardinal `WallSide`.

**État initial** : `InstanceConfig.bDoorInitiallyOpen`. Une porte placée existe parce qu'elle est présente dans `WorldObjectInstances` ; il n'existe plus de booléen générique d'existence ou d'activité.

**État de passage** : état maintenu par `UGridDoorSystemComponent`. `CanMove()` ne considère une porte franchissable que lorsque l'ouverture est réellement arrivée à son état terminal ouvert.

**État visuel** : alpha et cible d'animation de `AGridDoorActor`. `IsFullyOpen()` / `IsFullyClosed()` représentent les états terminaux.

**Mécanisme source** : bouton, levier, plaque, réceptacle, trigger ou autre émetteur dont un événement sélectionne un `FGridObjectLink`.

![Flux commande, état et passage](../Images/door_10_1_command_state_flow.svg)

## 3. Cartographie du code

| Domaine | Déclaration / implémentation | Responsabilité |
|---|---|---|
| Placement typé | `Core/GridLevelPlacementTypes.h` | Identité, cellule, bord, état initial et overrides d'instance. |
| Définition | `Core/GridWorldObjectDefinitionAsset.h/.cpp` | Présentation, motion, audio et comportement partagé. |
| Motion | `Core/GridWorldObjectVisual.h` | `StaticPart`, `MovingParts`, type/axe/pivot/amplitude/durées. |
| Niveau | `Core/GridLevelAsset.h/.cpp` | `WorldObjectInstances`, `Links` et validation de base. |
| Acteur visuel | `Runtime/GridDoorActor.h/.cpp` | Animation et représentation runtime de la porte. |
| État central | `Runtime/GridDoorSystemComponent.h/.cpp` | Index des portes, blocage de passage et synchronisation d'état. |
| Niveau runtime | `Runtime/GridLevelRuntimeActor.h/.cpp` | Résolution des arêtes, commandes et `CanMove()`. |
| Liens | `Runtime/GridActivationComponent.h/.cpp` | Traduit les commandes de lien en opérations de porte. |
| Éditeur | module `GrimrockPrototypeEditor` | Placement, Selected Object, liens, validation et preview. |

## 4. Données persistantes

Une porte placée porte notamment :

- `InstanceId`, identité stable pour les liens et la persistance ;
- `Type=Door` ;
- `WorldObjectDefinitionId` ;
- `CellX`, `CellY`, `WallSide` ;
- `InstanceConfig.bDoorInitiallyOpen` ;
- les éventuels `MovingPartOverrides` sparse ;
- les éventuels overrides de chaîne (`DoorChainMode`, durée de traction) ;
- les données locales de serrure lorsqu'elles sont utilisées.

La définition porte la composition visuelle et la motion partagée :

```text
StaticPart
MovingParts.Part0
  ├── Mesh
  ├── LocalTransform
  └── Motion { Type, Axis, Pivot, Amount, Duration, ReverseDuration }
MovingParts.Part1      // optionnelle
```

Il n'existe plus de `DoorAnimation.OpenHeight` comme autorité de course. Pour une translation, `Motion.Amount` est exprimé en centimètres ; pour une rotation, en degrés. `Duration` est la durée forward et `ReverseDuration` la durée inverse optionnelle.

Un placement peut surcharger uniquement les propriétés autorisées par `MovingPartOverrides` : transform local, amplitude et durée forward. Mesh, type de motion, axe, pivot et durée inverse restent Definition-owned.

La cellule doit rester franchissable et l'arête structurelle de la porte ne doit pas être doublée par un mur bloquant. Une porte entièrement ouverte n'annule pas un mur structurel `Solid` qui bloquerait encore `CanMove()`.

## 5. Initialisation runtime

`AGridLevelRuntimeActor` résout la définition, construit `FGridRuntimeWorldObjectData`, génère la classe runtime et initialise les visuels.

Pour une porte :

1. `InstanceConfig.bDoorInitiallyOpen` est normalisé dans le payload runtime ;
2. `AGridDoorActor` résout les `MovingParts` effectives (Definition + overrides sparse) ;
3. l'alpha initial vaut 0 pour une porte fermée et 1 pour une porte ouverte ;
4. `UGridDoorSystemComponent` indexe la porte par arête ;
5. le passage initial est bloqué tant que la porte n'est pas entièrement ouverte.

`FGridRuntimeWorldObjectData` est une frontière d'implémentation non persistée. Ses champs normalisés ne doivent pas être utilisés comme modèle d'authoring.

## 6. État logique, visuel et passabilité

`AGridDoorActor` conserve notamment :

- la cible ouverte/fermée ;
- l'alpha de motion courant ;
- l'état d'animation ;
- `IsFullyOpen()` / `IsFullyClosed()`.

La règle de passage est volontairement stricte :

```text
porte fermée          -> passage bloqué
porte en ouverture    -> passage bloqué
porte entièrement ouverte -> passage autorisé
porte en fermeture    -> passage bloqué
```

Le test `Grimrock.Runtime.Doors.PassageBlockedUntilFullyOpen` verrouille ce contrat pour une porte normale et une porte secrète.

Une inversion de commande pendant une animation change la cible sans considérer une porte partiellement ouverte comme franchissable. Le système central et l'acteur visuel restent ainsi cohérents.

![Cohérence des états de porte](../Images/door_10_3_state_consistency.svg)

## 7. Commandes applicables

`UGridActivationComponent` peut appliquer à une porte :

| Commande | Effet |
|---|---|
| `Open` | Demande l'ouverture. |
| `Activate` | Alias fonctionnel d'ouverture pour les liens génériques. |
| `Close` | Demande la fermeture. |
| `Deactivate` | Alias fonctionnel de fermeture. |
| `Toggle` | Inverse la cible courante. |

`AGridLevelRuntimeActor` résout l'arête directe puis, lorsque nécessaire, l'arête opposée de la cellule voisine. Les deux côtés d'une même séparation adressent donc la même porte.

Les commandes sont idempotentes : redemander la cible déjà atteinte ne crée pas une seconde animation incohérente.

## 8. Sources de commande

![Sources possibles d’une commande de porte](../Images/door_10_2_mechanism_sources.svg)

Les chemins suivants convergent vers le même système :

- bouton `Activated` ;
- levier `Activated` / `Deactivated` ;
- plaque `Activated` / `Deactivated` ;
- réceptacle `ItemInserted`, `ItemRemoved` ou `ItemChanged` ;
- trigger et logique de niveau ;
- callbacks Lua via les commandes runtime autorisées.

`SourceEvent` sélectionne le lien. `Command` choisit l'opération cible. Pour les réceptacles, voir [`RECEPTACLE_SYSTEM_FOUNDATION.md`](RECEPTACLE_SYSTEM_FOUNDATION.md).

## 9. Leviers et plaques de pression

Un levier placé ne possède pas d'override « On at Start ». Il commence au repos / Off ; sa motion est définie par `MovingParts[].Motion`.

Une plaque de pression ne possède pas d'état « Pressed at Start » authoré. Elle démarre relâchée, puis `UGridActivationComponent` calcule son état effectif à partir des règles de la définition ou de l'override d'instance : présence du groupe, présence d'un monstre si cette règle est activée, poids des items et règles de bord.

Cette séparation évite qu'un ancien booléen générique « actif » signifie selon le type « porte ouverte », « levier On » ou « plaque pressée ».

## 10. Chaîne optionnelle

La chaîne reste une interaction de porte :

- son hit-test sert à l'interaction souris ;
- sa motion est indépendante de la source de vérité de passage ;
- `PullChain()` déclenche finalement la commande de porte via le runtime ;
- `DoorChainMode` peut hériter, forcer l'activation ou la désactivation au niveau de l'instance ;
- la durée de traction peut être surchargée par instance, mais la géométrie partagée reste Definition-owned.

La chaîne ne modifie donc pas directement la passabilité en contournant le système central.

## 11. `CanMove()` et collision

Le déplacement case par case consulte les données de grille et le système de porte ; la collision physique du mesh n'est pas la source de vérité du déplacement logique.

Une porte ne devient franchissable qu'après l'état terminal ouvert. Les composants de collision ou de visibilité peuvent servir au clic et à la présentation, mais ne remplacent pas cette règle.

## 12. Persistance

Le SaveGame conserve les deltas runtime nécessaires à la restauration de la porte. Lors d'un chargement, le placement fournit le défaut sémantique (`bDoorInitiallyOpen`) et le runtime persistant, lorsqu'il existe, reprend ensuite l'autorité sur l'état mutable.

La composition visuelle, les meshes et la motion partagée ne sont pas dupliqués dans la sauvegarde : ils sont retrouvés via la définition.

## 13. Éditeur

Dans **Selected Object**, une porte expose `Open at Start`, directement mappé sur `InstanceConfig.bDoorInitiallyOpen`.

Le panneau de motion d'instance permet de surcharger la course/angle et la durée forward des parties mobiles sans recopier la définition complète. L'UI doit distinguer clairement la valeur de Definition et l'override local.

La validation doit signaler notamment :

- une porte murale sans `WallSide` cardinal ;
- une définition absente ou incompatible ;
- une frontière incohérente avec la géométrie structurelle ;
- des liens dont la commande n'est pas supportée par la cible.

## 14. Règles d'architecture

1. `WorldObjectInstances` conserve les portes placées.
2. `InstanceConfig.bDoorInitiallyOpen` est l'unique état initial authoré de la porte.
3. `MovingParts[].Motion` est l'autorité de mouvement partagée.
4. Les overrides de mouvement d'instance restent sparse.
5. Le système de porte décide de la passabilité logique.
6. `AGridDoorActor` représente et anime cet état.
7. Une porte partiellement ouverte reste bloquante.
8. Toute commande, y compris la chaîne et Lua, passe par l'API runtime centrale.
9. `CanMove()` reste autoritaire face à la collision physique des meshes.
10. Les deux côtés d'une arête résolvent la même porte.
