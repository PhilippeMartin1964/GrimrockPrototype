# 11 — Référence des paramètres GridObjectArchetypeAsset

Statut : document actif de référence.  
Date : 2026-05-23  
Projet : GrimrockPrototype — `UGridObjectArchetypeAsset`

## 1. Objectif

Ce document explique les paramètres visibles dans les DataAssets de type `GridObjectArchetypeAsset`.

Il complète :

- `07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md`, qui audite les champs et leur usage technique ;
- `08_GRID_OBJECT_ARCHETYPE_DATA_ASSETS_AUDIT.md`, qui audite les DataAssets existants ;
- `09_GRID_OBJECT_ARCHETYPE_NAMING_NORMALIZATION_PLAN.md`, qui fixe les noms d’archétypes ;
- `10_GRID_EDITOR_UI_CONSISTENCY_CHECKLIST.md`, qui sert de checklist UX/runtime.

Le présent document est plus direct : il sert à comprendre quoi mettre dans chaque champ d’un DataAsset.

---

## 2. Règles générales

### 2.1 Archétype vs instance placée

Un `UGridObjectArchetypeAsset` définit les valeurs par défaut d’un objet concret : nom, type, catégorie, placement, mesh, classe runtime, lumière, interaction et comportement par défaut.

Lorsqu’un objet est placé dans le niveau, certaines valeurs sont copiées dans `FGridLevelObjectData`.

Règle :

```text
DataAsset = définition par défaut
Objet placé = instance dans le niveau
```

Exemples :

```text
DA_Door_Stone        -> définit une porte de pierre
Door_Stone @ (10,12) -> instance placée dans le niveau
```

### 2.2 Les trois classifications à ne pas confondre

| Concept | Champ C++ | Libellé UI | Rôle |
|---|---|---|---|
| Type gameplay | `SupportedType` | `Gameplay Type` | Vérité gameplay : Door, Button, Item, Receptacle, etc. |
| Catégorie fonctionnelle | `ObjectCategory` | `Functional Category` | Classification éditeur/validation : Mechanism, Receptacle, Item, Decoration, etc. |
| Catégorie palette | `Category` | `Palette Category` | Groupement visuel dans Paint Object : Doors, Mechanisms, Items, Floor Decorations, etc. |

Règle :

```text
SupportedType = ce que l’objet EST.
ObjectCategory = comment on le classe fonctionnellement.
Category = où il apparaît dans la palette.
```

### 2.3 Champs supprimés / à ne pas réintroduire

Les champs suivants ne doivent pas être réintroduits :

```text
RotationStepYaw
TriggerMode
Delay
Duration
Invert Connectors
Fire On Enter
Fire On Exit
SpawnedItemArchetypeId
Behavior Editor global
```

L’orientation est pilotée par `Edge / Facing` via le widget `North / East / South / West`.

Les connecteurs sont strictement :

```text
Source Object + Source Event + Target Object + Command
```

---

## 3. Tableau complet des paramètres UGridObjectArchetypeAsset

| Groupe UI | Champ C++ | Type | Rôle | À régler quand | Exemple / valeur typique | Remarques |
|---|---|---|---|---|---|---|
| Archetype | `ArchetypeId` | `FName` | Identifiant stable de l’archétype. | Toujours. | `Door_Stone`, `Item_Torch` | Ne pas renommer sans migration des niveaux et références. |
| Archetype | `DisplayName` | `FText` | Nom lisible dans l’éditeur. | Toujours. | `Stone Door`, `Torch` | Utilisé dans l’inspecteur, la palette et les connecteurs. |
| Archetype | `SupportedType` | `EGridLevelObjectType` | Type gameplay réel. | Toujours. | `Door`, `Button`, `Item` | Source principale pour le runtime et l’éditeur. |
| Archetype | `Description` | `FText` | Description humaine de l’archétype. | Recommandé. | `Pickable torch item.` | Documentation d’authoring. |
| Defaults | `bDefaultInitiallyEnabled` | `bool` | Définit si les nouvelles instances sont activées au départ. | Toujours. | `true` | Copié dans l’objet placé comme `Enabled at Start`. |
| Defaults | `bDefaultInitiallyActive` | `bool` | Définit si les nouvelles instances commencent dans leur état actif. | Selon type. | Door fermée = `false`, levier actif = `true` | Copié dans l’objet placé comme `Active at Start`. Le sens dépend du type. |
| Defaults | `DefaultTag` | `FName` | Tag par défaut d’instance. | Rare / avancé. | `Torch`, `Key`, `Offering` | À garder technique ; ne pas utiliser pour remplacer les archetypes. |
| Defaults | `DefaultBehavior` | `FGridObjectBehaviorParams` | Paramètres par défaut spécifiques encore utiles. | Button, Receptacle, Teleporter. | Voir section 4. | Ne contient plus Activation/Trigger/ItemSpawn génériques. |
| Item | `ItemTags` | `TArray<FName>` | Tags d’item utilisés par les réceptacles et futurs inventaires. | Pour `SupportedType=Item`. | `Torch`, `Key`, `Coin` | Sert au matching par tag dans les réceptacles. |
| Palette | `Category` | `FName` | Catégorie de palette. | Toujours pour objets plaçables. | `Doors`, `Mechanisms`, `Items` | N’a pas d’impact gameplay. |
| Archetype | `ObjectCategory` | `EGridObjectCategory` | Catégorie fonctionnelle éditeur/validation. | Toujours. | `Mechanism`, `Item`, `Decoration` | Ne remplace pas `SupportedType`. |
| Placement | `PlacementKind` | `EGridObjectPlacementKind` | Source de vérité du placement. | Toujours. | `Edge`, `Wall`, `Floor`, `Center` | Détermine preview, placement, orientation et transform runtime. |
| Placement / Legacy | `bPlaceOnEdge` | `bool` | Ancien flag de placement edge. | Ne plus utiliser. | `false` | Legacy seulement. `PlacementKind` prime. |
| Placement / Legacy | `bPlaceAtCellCenter` | `bool` | Ancien flag de placement au centre. | Ne plus utiliser. | `true` ou legacy | Legacy seulement. `PlacementKind` prime. |
| Placement | `bCanShareCell` | `bool` | Autorise le partage de cellule avec d’autres objets. | Décorations, triggers, items. | Floor decoration = `true` | Empêche ou autorise les conflits de placement. |
| Placement | `bCanShareAnchor` | `bool` | Autorise le partage du même edge/ancre. | Objets edge/wall. | Door = souvent `false`, deco = selon besoin | Évite les chevauchements sur le même edge. |
| Placement | `bBlocksMovement` | `bool` | Blocage générique de mouvement. | Props bloquants non-door. | Généralement `false` | Les portes sont bloquées par le système de portes, pas par ce flag. |
| Interaction | `bIsInteractable` | `bool` | Indique que l’objet peut répondre à une interaction runtime directe si l’acteur le supporte. | Buttons, levers, receptacles, items pickup. | `Item_Torch=true` | Champ indicatif/runtime selon chemin d’acteur. |
| Interaction | `bIsReadable` | `bool` | Active le comportement readable. | Inscriptions ou objets lisibles. | `WallInscription=true` | Différent de `ObjectCategory=Readable`. |
| Interaction | `ReadableText` | `FText` | Texte par défaut de lecture. | Si `bIsReadable=true`. | Texte d’inscription. | Peut être surchargé par l’objet placé. |
| Interaction | `bShowReadableOnlyOnce` | `bool` | Affiche le texte une seule fois. | Readable avancé. | `false` | À utiliser seulement si le gameplay le demande. |
| Light | `bIsLightSource` | `bool` | Indique que l’objet crée/configure une lumière runtime. | Torches murales, lumières, runes lumineuses. | Torche au sol = `false` | Ne pas confondre torche au sol et torche en main/support. |
| Light | `LightColor` | `FLinearColor` | Couleur de la lumière runtime. | Si `bIsLightSource=true`. | Chaud/orangé pour torche. | Réglage archetype-only. |
| Light | `LightIntensity` | `float` | Intensité de la lumière. | Si `bIsLightSource=true`. | Faible pour torche dungeon. | Trop fort détruit l’ambiance sombre. |
| Light | `LightRadius` | `float` | Rayon de lumière. | Si `bIsLightSource=true`. | Quelques cellules maximum. | À régler avec la taille cellule 200 cm. |
| Light | `bUseLightFlicker` | `bool` | Demande un flicker si le chemin runtime le supporte. | Torches / flammes. | `true` pour support de torche | Le support réel dépend des composants runtime. |
| Visual | `PreviewMesh` | `UStaticMesh*` | Mesh principal/simple et mesh de preview. | Objet visible simple ou item. | `SM_Torch`, `SM_Door` | Malgré le nom, ce n’est pas seulement editor-preview. |
| Visual | `PreviewMaterial` | `UMaterialInterface*` | Matériau principal/simple et preview. | Si besoin d’override matériel. | `MI_StoneDoor` | Optionnel si le mesh porte déjà ses matériaux. |
| Visual | `FixedMesh` | `UStaticMesh*` | Partie fixe d’un objet composite. | Portes secrètes, supports, mécanismes composites. | partie fixe de porte secrète | Advanced. |
| Visual | `MovingMesh` | `UStaticMesh*` | Partie animée/mobile d’un objet composite. | Porte, bouton, levier, secret door. | panneau mobile de porte | Advanced. |
| Visual | `FixedMaterial` | `UMaterialInterface*` | Matériau de la partie fixe. | Si `FixedMesh` a besoin d’override. | `MI_WallStone` | Advanced. |
| Visual | `MovingMaterial` | `UMaterialInterface*` | Matériau de la partie mobile. | Si `MovingMesh` a besoin d’override. | `MI_DoorStone` | Advanced. |
| Runtime | `RuntimeActorClass` | `TSubclassOf<AGridRuntimeObjectActor>` | Classe runtime des objets non-item. | Door, Button, Lever, Receptacle, PressurePlate, etc. | `BP_GridDoorActor` | Non utilisé pour les items pickup. |
| Runtime | `ItemActorClass` | `TSubclassOf<AGridItemActor>` | Classe runtime des items manipulables. | `SupportedType=Item`. | `BP_Item_Torch` | Pour item au sol / inventaire futur. Fallback C++ possible. |
| Placement | `PlacementZOffset` | `float` | Offset vertical de placement. | Tous objets visibles. | `10` pour item sol, plus haut pour wall. | Influence aussi les centres logiques de connecteurs. |
| Placement / Wall | `WallInset` | `float` | Distance depuis le mur/edge pour objets Wall/Edge. | Wall/Edge, items sur edge. | `30` pour Item_Torch edge. | Sert aussi au placement floor-edge des items. |
| Placement / Wall | `LocalOffsetAlongWall` | `float` | Décalage latéral le long du mur. | Wall/Edge. | `0` par défaut | Utile pour ajuster bouton/support. |
| Placement / Wall | `LocalOffsetVertical` | `float` | Décalage vertical additionnel. | Wall/Edge. | `0` par défaut | S’ajoute à `PlacementZOffset`. |

---

## 4. Sous-paramètres de DefaultBehavior

`DefaultBehavior` contient uniquement les groupes encore utiles :

```text
Teleporter
Receptacle
ButtonAnimation
```

Les anciens groupes génériques `Activation`, `Trigger` et `ItemSpawn` ont été supprimés.

### 4.1 Teleporter

| Champ | Type | Rôle | À utiliser quand | Exemple |
|---|---|---|---|---|
| `Teleporter.TargetCellX` | `int32` | Cellule X cible. | `SupportedType=Teleporter`. | `12` |
| `Teleporter.TargetCellY` | `int32` | Cellule Y cible. | `SupportedType=Teleporter`. | `8` |

Statut : prévu / partiel selon état du runtime Teleporter.

### 4.2 Receptacle

| Champ | Type | Rôle | UI recommandée | Remarques |
|---|---|---|---|---|
| `Receptacle.bAcceptAnyItem` | `bool` | Accepte tout item si vrai. | Checkbox `Accept Any Item`. | Par défaut `true`. |
| `Receptacle.AcceptedItemTags` | `TArray<FName>` | Accepte les items portant au moins un tag. | Advanced / masqué dans l’UI normale. | Utile pour variantes futures. |
| `Receptacle.AcceptedArchetypeIds` | `TArray<FName>` | Liste stricte des items acceptés. | Dropdown `Accepted Items`. | Ne pas taper à la main dans l’inspecteur. |
| `Receptacle.RejectedItemArchetypeIds` | `TArray<FName>` | Exclusions explicites. | Advanced / masqué. | Rare ; utile avec `Accept Any Item`. |
| `Receptacle.InitialContainedItemArchetypeId` | `FName` | Item présent au démarrage. | Dropdown `Initial Content`. | `None + items`. |

Règle UX :

```text
Accepted Items et Initial Content doivent proposer uniquement des items.
```

Exemple `Receptacle_TorchHolder` :

```text
Accept Any Item = false
Accepted Items = Item_Torch
Initial Content = Item_Torch ou None selon le niveau
```

### 4.3 ButtonAnimation

| Champ | Type | Rôle | Exemple | Remarques |
|---|---|---|---|---|
| `ButtonAnimation.ButtonPressDistance` | `float` | Distance d’enfoncement du bouton. | `6.0` | Visuel/animation. |
| `ButtonAnimation.ButtonPressDuration` | `float` | Durée d’enfoncement. | `0.08` | Court et réactif. |
| `ButtonAnimation.ButtonReleaseDuration` | `float` | Durée de relâchement. | `0.10` | Peut être légèrement plus long. |
| `ButtonAnimation.ButtonHoldTime` | `float` | Temps de maintien avant relâchement. | `0.15` | Ne remplace pas un système de connecteur. |

---

## 5. Recommandations par type d’archétype

## 5.1 Door / Door_Stone / Door_Secret

| Paramètre | Valeur recommandée |
|---|---|
| `SupportedType` | `Door` |
| `ObjectCategory` | `Mechanism` ou `Passage` selon enum disponible |
| `Category` | `Doors` |
| `PlacementKind` | `Edge` |
| `bDefaultInitiallyEnabled` | `true` |
| `bDefaultInitiallyActive` | `false` pour porte fermée au départ |
| `bBlocksMovement` | `false` sauf cas générique ; le blocage de porte est géré par le système de portes |
| `RuntimeActorClass` | BP ou classe dérivée de `AGridDoorActor` |
| `ItemActorClass` | `None` |

## 5.2 Button / Lever / PressurePlate

| Paramètre | Button | Lever | PressurePlate |
|---|---|---|---|
| `SupportedType` | `Button` | `Lever` | `PressurePlate` |
| `ObjectCategory` | `Mechanism` | `Mechanism` | `Mechanism` |
| `Category` | `Mechanisms` | `Mechanisms` | `Mechanisms` |
| `PlacementKind` | `Wall` ou `Edge` | `Wall` ou `Edge` | `Floor` ou `Center` |
| `bIsInteractable` | `true` | `true` | selon runtime |
| `RuntimeActorClass` | Button actor | Lever actor | PressurePlate actor |
| `DefaultBehavior` | ButtonAnimation utile | généralement vide | généralement vide |

## 5.3 Receptacle_TorchHolder

| Paramètre | Valeur recommandée |
|---|---|
| `SupportedType` | `Receptacle` |
| `ObjectCategory` | `Receptacle` |
| `Category` | `Receptacles` |
| `PlacementKind` | `Wall` ou `Edge` |
| `bIsInteractable` | `true` |
| `RuntimeActorClass` | BP/classe dérivée de `AGridReceptacleActor` |
| `DefaultBehavior.Receptacle.bAcceptAnyItem` | `false` pour support dédié |
| `DefaultBehavior.Receptacle.AcceptedArchetypeIds` | `Item_Torch` |
| `DefaultBehavior.Receptacle.InitialContainedItemArchetypeId` | `Item_Torch` ou `None` selon niveau |

## 5.4 Item_Torch

| Paramètre | Valeur recommandée |
|---|---|
| `SupportedType` | `Item` |
| `ObjectCategory` | `Item` |
| `Category` | `Items` |
| `PlacementKind` | `Edge` si posé près d’un bord, sinon `Floor` |
| `bCanShareCell` | `true` |
| `bCanShareAnchor` | `true` |
| `bBlocksMovement` | `false` |
| `bIsInteractable` | `true` |
| `bIsLightSource` | `false` pour l’item au sol |
| `PreviewMesh` | mesh de torche éteinte |
| `RuntimeActorClass` | `None` |
| `ItemActorClass` | `BP_Item_Torch` ou fallback `AGridItemActor` |
| `PlacementZOffset` | environ `8` à `12` |
| `WallInset` | environ `24` à `35` si `PlacementKind=Edge` |

Règle :

```text
Item_Torch au sol = item physique, ramassable, éteint.
Torche en main/support = état porté/attaché, pas item libre au sol.
```

## 5.5 Floor Decorations

| Paramètre | Valeur recommandée |
|---|---|
| `SupportedType` | `Decoration` |
| `ObjectCategory` | `Decoration` |
| `Category` | `Floor Decorations` |
| `PlacementKind` | `Floor` ou `Center` |
| `bCanShareCell` | `true` |
| `bCanShareAnchor` | `true` |
| `bBlocksMovement` | `false` |
| `bIsInteractable` | `false` |
| `bIsReadable` | `false` sauf décoration lisible spécifique |
| `bIsLightSource` | `false`, sauf rune lumineuse volontaire |
| `RuntimeActorClass` | actor générique si nécessaire |
| `ItemActorClass` | `None` |

Les décorations visibles au sol sont orientables via le widget `North / East / South / West`.

---

## 6. Checklist rapide avant sauvegarde UE5

Pour chaque DataAsset :

```text
[ ] ArchetypeId stable et canonique
[ ] DisplayName lisible
[ ] Gameplay Type correct
[ ] Functional Category correct
[ ] Palette Category correct
[ ] Placement Kind correct
[ ] Mesh principal renseigné si objet visible
[ ] Runtime Actor Class renseigné si objet runtime non-item
[ ] Item Actor Class renseigné ou fallback accepté si item
[ ] Runtime Interactable cohérent
[ ] Runtime Light Source cohérent
[ ] bBlocksMovement non utilisé à tort pour les portes
[ ] Réceptacle configuré par dropdown items, pas saisie libre
[ ] Item au sol non connecté au système CONNECTORS
[ ] Pas de champ obsolète réintroduit
```

---

## 7. Priorité documentaire

En cas de contradiction :

1. `99_DECISIONS_LOG.md` et les décisions les plus récentes priment ;
2. ce document sert de référence pratique pour les paramètres ;
3. `07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md` reste la référence d’audit technique ;
4. `08` et `09` restent les références pour les DataAssets existants et leur nommage.
