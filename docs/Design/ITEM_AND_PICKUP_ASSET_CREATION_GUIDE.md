# Guide de création des items ramassables et plaçables

Statut : **guide de production Design / Content**  
Portée : **assets Unreal et configuration éditeur**  
Ne remplace pas : `docs/Architecture/ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md`, qui décrit le socle runtime réellement implémenté.

---

## 1. Objet du document

Ce document fixe la procédure standard pour créer un objet de jeu pouvant être ramassé au sol, placé dans une alcôve ou un autre réceptacle, affiché dans l'inventaire, manipulé par le curseur, éventuellement équipé, lancé, utilisé comme clé, composant, torche, arme, nourriture ou objet de quête.

Le but est d'éviter de redécouvrir à chaque nouvel item la différence entre l'item logique, l'objet plaçable dans la grille, son mesh, son acteur Blueprint, son entrée de palette, son comportement dans les réceptacles et son affichage dans le tooltip d'inventaire.

---

## 2. Deux familles d'objets dans le projet

### 2.1 Objets fixes ou structurels

Ces objets appartiennent d'abord au niveau. Ils sont généralement commandés par le système de liens ou participent à la géométrie jouable.

Exemples : sols, murs, plafonds, portes, portes secrètes, boutons, leviers, plaques de pression, serrures murales, chaînes, téléporteurs, triggers, décorations fixes, lumières fixes, réceptacles eux-mêmes : alcôves, supports, autels, bols, coffres.

Ils sont principalement définis par :

```text
DA_Object_XXX
GridObjectArchetypeAsset
```

et éventuellement par :

```text
BP_XXXActor
SM_XXX
M_XXX
```

### 2.2 Items ramassables, transportables ou plaçables

Ces objets ont une identité d'item et peuvent changer de propriétaire.

Exemples : clé, pierre, torche, arme, bouclier, armure, gemme, potion, parchemin, livre, nourriture, composant, objet de quête, outil, ressource.

Ils nécessitent en général **deux DataAssets distincts** :

```text
DA_Item_XXX
GridItemDefinitionAsset

DA_Object_XXXPickup
GridObjectArchetypeAsset
```

Le premier définit **ce qu'est l'item**. Le second définit **comment cet item est placé dans un niveau**.

---

## 3. Chaîne complète d'un item ramassable

Pour un item standard, la chaîne complète est :

```text
SM_XXX
  -> mesh visuel monde / preview / équipé

Icon_XXX
  -> icône d'inventaire et tooltip

DA_Item_XXX
  -> définition logique stable de l'item

BP_GridItemActor ou BP_XXXActor
  -> représentation runtime de l'item

DA_Object_XXXPickup
  -> archétype plaçable dans la grille

DA_ObjectPalette_Default
  -> exposition dans la palette de l'éditeur

DA_GridLevelAsset / L_GrimrockEditor
  -> placement concret dans le niveau

Réceptacle.InitialContent
  -> placement initial dans une alcôve, un coffre, un support, etc.
```

---

## 4. `DA_Item_XXX` — définition logique de l'item

Classe Unreal :

```text
GridItemDefinitionAsset
```

Emplacement recommandé :

```text
Content/Grimrock/Core/DataAssets/Items/
```

Exemple :

```text
Content/Grimrock/Core/DataAssets/Items/DA_Item_CopperKey
```

### 4.1 Rôle

`DA_Item_XXX` est la source de vérité métier de l'item. Il répond aux questions : quel est l'identifiant stable, le nom, la description, le type, le poids, l'icône, le mesh monde, les slots d'équipement, les paramètres de lancer, la lumière et les tags métier.

### 4.2 Paramètres obligatoires

| Paramètre | Rôle | Recommandation |
|---|---|---|
| `ItemDefinitionId` | Identifiant stable de gameplay | Obligatoire. Ne doit jamais changer une fois utilisé dans un niveau. |
| `DisplayName` | Nom affiché au joueur | Obligatoire. Sert au tooltip. |
| `Description` | Texte descriptif | Recommandé. Sert au tooltip. |
| `ItemType` | Type d'item | Obligatoire. `Key`, `Weapon`, `Gem`, `Torch`, etc. |
| `Weight` | Poids unitaire | Obligatoire, même si 0. |
| `Icon` | Icône UI | Recommandé pour tout item inventoriable. |
| `WorldMesh` | Mesh dans le monde | Recommandé pour item ramassable. |

### 4.3 Identité

#### `ItemDefinitionId`

Convention recommandée :

```text
Key_Copper
Stone_Rough
Torch_Wooden
Gem_Blue
Weapon_Dagger_Iron
Food_Bread
Quest_RelicFragment01
```

Règles : ne pas mettre `DA_` dans l'identifiant, ne pas utiliser d'espaces, préférer l'anglais technique stable, ne jamais réutiliser un identifiant pour un autre item, ne pas renommer après placement dans un niveau.

#### `DisplayName`

Nom joueur. Exemples :

```text
Copper Key
Rough Stone
Wooden Torch
Blue Gem
Iron Dagger
```

#### `Description`

Texte du tooltip. Exemple :

```text
A small copper key, worn by age. It probably opens a simple lock nearby.
```

### 4.4 Type d'item

`ItemType` doit être choisi dans `EGridItemType` :

```text
None
Torch
Weapon
Shield
Armor
Jewelry
Key
Gem
Potion
Scroll
Book
Food
Component
Quest
Misc
```

Règles : une clé doit être `Key`; une pierre simple peut être `Misc` ou `Component`; un objet de quête doit être `Quest` si sa perte peut bloquer une progression; une gemme de mécanisme doit être `Gem` ou `Quest` selon son importance.

### 4.5 Poids

`Weight` est le poids unitaire.

| Type | Poids indicatif |
|---|---:|
| Clé | 0.05 à 0.10 |
| Pierre légère | 0.5 à 1.5 |
| Torche | 1.0 |
| Gemme | 0.1 |
| Arme légère | 1.0 à 3.0 |
| Arme lourde | 4.0 à 8.0 |
| Armure | 5.0 à 25.0 |

### 4.6 Stack

| Paramètre | Rôle |
|---|---|
| `bStackable` | Autorise plusieurs unités dans une même case d'inventaire. |
| `MaxStackSize` | Taille maximale d'une pile. |

Règles : clé, arme, armure généralement non stackables; composants, nourriture et ressources souvent stackables; pierre selon usage.

### 4.7 Équipement

| Paramètre | Rôle |
|---|---|
| `CompatibleEquipmentSlots` | Liste des slots où l'item peut être équipé. |
| `EquippedMesh` | Mesh utilisé si l'item est visible sur le personnage. |

Exemples : arme -> `MainHand`; bouclier -> `OffHand`; torche -> `MainHand` / `OffHand`; amulette -> `Amulet`; anneau -> `Ring1` / `Ring2`.

Pour une clé :

```text
CompatibleEquipmentSlots = vide
EquippedMesh = vide
```

### 4.8 Visuels

| Paramètre | Rôle |
|---|---|
| `Icon` | Icône inventaire et tooltip. |
| `WorldMesh` | Mesh utilisé lorsque l'item est dans le monde. |
| `EquippedMesh` | Mesh utilisé lorsqu'il est équipé ou tenu. |

Règles : `Icon` carré, lisible, idéalement 512x512 avec alpha; `WorldMesh` avec pivot propre et échelle UE correcte; `EquippedMesh` peut rester vide tant que l'item n'est pas visible en main.

### 4.9 Lancer d'objet

| Paramètre | Rôle |
|---|---|
| `bThrowable` | Autorise le lancer. |
| `ThrowSpeed` | Vitesse initiale. |
| `ThrowArc` | Composante verticale. |
| `ThrowLifeSeconds` | Durée de vie du projectile. |
| `ThrowImpactDropOffset` | Offset après impact. |

Clé : généralement non lançable au début. Pierre : lançable. Objet de quête : éviter le lancer tant que la restauration n'est pas robuste.

### 4.10 Lumière

| Paramètre | Rôle |
|---|---|
| `bCanEmitLight` | Item capable d'émettre de la lumière. |
| `bDefaultLightEnabled` | État initial. |
| `LightRadius` | Rayon lumineux. |

Torche typique :

```text
bCanEmitLight = true
bDefaultLightEnabled = true
LightRadius = 600
```

Clé :

```text
bCanEmitLight = false
```

### 4.11 Tags métier

`ItemTags` décrit la nature métier de l'item.

Exemples :

```text
Key
Key.Copper
Key.Dungeon
Key.Prison
Tool
Tool.LockpickSet
Quest
Quest.CrownFragment
Gem
Gem.Blue
Throwable
WeightObject
```

Règles : les tags métier appartiennent à `DA_Item_XXX`; ne pas les recopier inutilement dans `DA_Object_XXXPickup`; utiliser les tags pour les compatibilités futures : serrures, recettes, mécanismes, filtres d'inventaire.

---

## 5. Impact sur le tooltip d'inventaire

Le tooltip doit lire les informations depuis `DA_Item_XXX` et l'instance runtime.

### 5.1 Champs issus de `DA_Item_XXX`

| Élément tooltip | Source |
|---|---|
| `ItemIcon` | `Icon` |
| `Text_ItemName` | `DisplayName` |
| `Text_ItemType` | `ItemType` |
| `Text_Description` | `Description` |
| `Text_Weight` | `Weight` |
| Indication lumière | `bCanEmitLight`, `bDefaultLightEnabled`, état runtime |

### 5.2 Champs issus de l'instance runtime

| Élément tooltip | Source |
|---|---|
| Quantité | `FGridItemInstance.Quantity` |
| Lumière actuellement active | `FGridItemInstance.bLightsEnabled` |
| Identité runtime | `RuntimeObjectId`, seulement debug |
| Propriétaire | `OwnerType`, debug ou diagnostics |

Règle importante : le tooltip ne doit pas dépendre de `DA_Object_XXXPickup`. `DA_Object_XXXPickup` sert au placement dans le monde, pas à l'identité d'inventaire.

---

## 6. `SM_XXX` — Static Mesh

Emplacement recommandé :

```text
Content/Grimrock/Meshes/Items/
```

Exemples :

```text
SM_Key_Copper
SM_Stone_Rough
SM_Torch_Wooden
SM_Gem_Blue
```

Paramètres attendus : échelle correcte en centimètres UE, pivot utile, orientation cohérente, matériaux assignés, collision simple suffisante, pas de transform exotique importé depuis Blender, `Apply All Transforms` côté Blender si nécessaire avant export.

Pour un item ramassable :

```text
WorldMesh = mesh mobile / ramassable
PreviewMesh = souvent le même mesh
EquippedMesh = mesh spécifique si tenu/équipé
```

Pour un objet structurel :

```text
FixedMesh = partie fixe
MovingMesh = partie animée
PreviewMesh = fallback ou preview éditeur
```

---

## 7. `BP_XXXActor` — acteur runtime

La majorité des items peut utiliser :

```text
AGridItemActor
BP_GridItemActor
```

Créer un `BP_XXXActor` spécifique seulement si l'item a besoin de logique ou de composants particuliers : torche avec lumière visible, objet animé, projectile particulier, item avec plusieurs composants, item interactif spécial, item équipé avec sockets particuliers.

Pour la Copper Key, commencer avec l'acteur item générique.

---

## 8. `DA_Object_XXXPickup` — objet plaçable dans le niveau

Classe Unreal :

```text
GridObjectArchetypeAsset
```

Emplacement recommandé :

```text
Content/Grimrock/Core/DataAssets/ObjectArchetypes/Items/
```

Exemple :

```text
DA_Object_KeyCopperPickup
```

### 8.1 Rôle

`DA_Object_XXXPickup` est l'entrée plaçable dans la grille. Il répond aux questions : comment l'objet apparaît-il dans la palette, quel type de niveau est créé lorsqu'on le place, où peut-il être placé, quel mesh sert à la preview, quel acteur runtime est utilisé, quel `DA_Item_XXX` est injecté dans l'objet placé.

### 8.2 Paramètres principaux

| Paramètre | Valeur pour un pickup |
|---|---|
| `ArchetypeId` | `Item_CopperKey_Pickup`, `Item_Stone_Rough_Pickup`, etc. |
| `DisplayName` | Nom dans la palette |
| `SupportedType` | `Item` |
| `ObjectCategory` | `Item` |
| `Category` | `Items`, `Items/Keys`, `Items/Quest`, etc. |
| `PlacementKind` | `Floor` ou `Center`, parfois `Edge` |
| `bCanShareCell` | généralement `true` |
| `bCanShareAnchor` | généralement `true` |
| `PreviewMesh` | mesh monde ou preview |
| `PreviewMaterial` | optionnel |
| `RuntimeActorClass` | généralement vide pour item si le runtime item a son chemin propre, ou classe compatible si le pipeline le requiert |
| `ItemActorClass` | `BP_GridItemActor` ou spécialisation |
| `PlacementZOffset` | hauteur au-dessus du sol |
| `DefaultBehavior.Item.ItemDefinitionAsset` | `DA_Item_XXX` |
| `DefaultBehavior.Item.ItemDefinitionId` | identifiant stable de l'item |

### 8.3 `ArchetypeId`

Convention recommandée :

```text
Item_CopperKey_Pickup
Item_RoughStone_Pickup
Item_WoodenTorch_Pickup
Item_BlueGem_Pickup
Item_IronDagger_Pickup
```

Ne pas mettre `DA_` dans `ArchetypeId`; ne pas confondre avec `ItemDefinitionId`.

### 8.4 `SupportedType`

Pour un item ramassable :

```text
SupportedType = Item
```

### 8.5 Catégorie palette

Recommandations :

```text
Category = Items
Category = Items/Keys
Category = Items/Tools
Category = Items/Weapons
Category = Items/Food
Category = Items/Quest
```

`ObjectCategory` doit rester :

```text
ObjectCategory = Item
```

### 8.6 Placement

Pour un item au sol :

```text
PlacementKind = Floor
```

ou :

```text
PlacementKind = Center
```

Pour un item posé sur une arête de la cellule :

```text
PlacementKind = Edge
```

Attention : le runtime applique des règles d'accessibilité différentes entre un item central et un item sur arête.

### 8.7 DefaultBehavior.Item

Lien essentiel entre l'objet plaçable et l'item logique :

```text
DefaultBehavior.Item.ItemDefinitionAsset = DA_Item_CopperKey
DefaultBehavior.Item.ItemDefinitionId    = Key_Copper
```

Renseigner les deux quand c'est possible.

---

## 9. Impact de `DA_ObjectPalette_Default`

`DA_ObjectPalette_Default` contrôle ce que l'éditeur de grille propose dans sa palette.

Créer `DA_Item_XXX` et `DA_Object_XXXPickup` ne suffit pas toujours. Il faut aussi ajouter :

```text
DA_Object_XXXPickup
```

à :

```text
DA_ObjectPalette_Default
```

ou à l'asset de palette actuellement utilisé par `L_GrimrockEditor`.

Effet attendu :

```text
Grimrock Grid Editor Mode
  -> catégorie Items / Keys
  -> Copper Key
```

Le placement doit créer un objet de niveau :

```text
Type = Item
ArchetypeId = Item_CopperKey_Pickup
ItemDefinitionAsset = DA_Item_CopperKey
ItemDefinitionId = Key_Copper
```

À vérifier : l'objet apparaît dans la bonne catégorie, la preview s'affiche, le placement crée bien un objet `Item`, le runtime génère un item ramassable, l'item rejoint l'inventaire au clic.

---

## 10. Placement direct dans le niveau

Un item peut être placé directement dans `DA_GridLevelAsset::Objects`.

Champs importants :

```text
Type = Item
CellX / CellY
Edge
ArchetypeId = Item_XXX_Pickup
ItemDefinitionAsset = DA_Item_XXX
ItemDefinitionId = XXX
```

Règle de résolution : utiliser `ItemDefinitionAsset` si renseigné; sinon `ItemDefinitionId`; sinon `DefaultBehavior.Item` de l'archétype.

Bonne pratique : renseigner `ArchetypeId` pour le placement et la preview, mais aussi `ItemDefinitionAsset` ou `ItemDefinitionId` pour éviter toute ambiguïté.

---

## 11. Item placé dans un réceptacle

Pour placer un item dans une alcôve, un support, un autel, un bol, un coffre ou tout autre réceptacle :

```text
Objet réceptacle
  -> Behavior.Receptacle.InitialContent
      -> ItemDefinition = DA_Item_XXX
      -> Quantity = N
```

Règles : le réceptacle contient des items logiques, pas des `DA_Object_XXXPickup`; utiliser `DA_Item_XXX`, pas l'archétype de pickup; le visuel contenu est généré par le réceptacle; le joueur retire ensuite l'item vers l'inventaire.

Exemple :

```text
Alcove_A
InitialContent:
  - DA_Item_CopperKey
```

---

## 12. Exemple complet : Copper Key

### 12.1 Static Mesh

```text
SM_Key_Copper
Path: Content/Grimrock/Meshes/Items/SM_Key_Copper
```

### 12.2 Icône

```text
Icon_CopperKey
Path: Content/Grimrock/Icons/Items/Icon_CopperKey
```

### 12.3 Définition d'item

```text
DA_Item_CopperKey
Class: GridItemDefinitionAsset
Path: Content/Grimrock/Core/DataAssets/Items/DA_Item_CopperKey
```

Paramètres :

```text
ItemDefinitionId = Key_Copper
DisplayName = Copper Key
Description = A small copper key, worn by age.
ItemType = Key
Weight = 0.1
bStackable = false
MaxStackSize = 1
CompatibleEquipmentSlots = empty
Icon = Icon_CopperKey
WorldMesh = SM_Key_Copper
EquippedMesh = empty
bThrowable = false
bCanEmitLight = false
ItemTags:
  - Key
  - Key.Copper
  - Key.Dungeon
```

### 12.4 Acteur item

```text
BP_GridItemActor
```

ou, seulement si nécessaire plus tard :

```text
BP_KeyItemActor
```

### 12.5 Archétype plaçable

```text
DA_Object_KeyCopperPickup
Class: GridObjectArchetypeAsset
Path: Content/Grimrock/Core/DataAssets/ObjectArchetypes/Items/DA_Object_KeyCopperPickup
```

Paramètres :

```text
ArchetypeId = Item_CopperKey_Pickup
DisplayName = Copper Key
SupportedType = Item
ObjectCategory = Item
Category = Items/Keys
PlacementKind = Floor
bCanShareCell = true
bCanShareAnchor = true
PreviewMesh = SM_Key_Copper
PreviewMaterial = empty or copper material
RuntimeActorClass = empty unless required by current item pipeline
ItemActorClass = BP_GridItemActor
PlacementZOffset = 12
DefaultBehavior.Item.ItemDefinitionAsset = DA_Item_CopperKey
DefaultBehavior.Item.ItemDefinitionId = Key_Copper
```

### 12.6 Palette

Ajouter :

```text
DA_Object_KeyCopperPickup
```

à :

```text
DA_ObjectPalette_Default
```

### 12.7 Placement au sol

Dans `L_GrimrockEditor` / `DA_GridLevelAsset` :

```text
Type = Item
ArchetypeId = Item_CopperKey_Pickup
ItemDefinitionAsset = DA_Item_CopperKey
ItemDefinitionId = Key_Copper
CellX / CellY = position choisie
Edge = None ou edge si placement bord
```

### 12.8 Placement dans une alcôve

Dans l'objet alcôve :

```text
Behavior.Receptacle.InitialContent:
  - ItemDefinition = DA_Item_CopperKey
    Quantity = 1
```

### 12.9 Serrure compatible

Dans la serrure murale :

```text
Behavior.Lock.AcceptedKeyItems:
  - DA_Item_CopperKey

Behavior.Lock.AcceptedKeyIds:
  - Key_Copper
```

Lien :

```text
CopperWallLock.Activated -> Door.Open
```

---

## 13. Exemple complet : pierre ramassable

### 13.1 Définition d'item

```text
DA_Item_RoughStone
ItemDefinitionId = Stone_Rough
DisplayName = Rough Stone
ItemType = Misc
Weight = 1.0
bStackable = false
WorldMesh = SM_Stone_Rough
Icon = Icon_Stone_Rough
bThrowable = true
ThrowSpeed = 1200
ThrowArc = 0.08
ThrowLifeSeconds = 5
ThrowImpactDropOffset = 12
ItemTags:
  - Stone
  - Throwable
  - WeightObject
```

### 13.2 Archétype plaçable

```text
DA_Object_StonePickup
ArchetypeId = Item_RoughStone_Pickup
SupportedType = Item
ObjectCategory = Item
Category = Items/Props
PlacementKind = Floor
PreviewMesh = SM_Stone_Rough
DefaultBehavior.Item.ItemDefinitionAsset = DA_Item_RoughStone
DefaultBehavior.Item.ItemDefinitionId = Stone_Rough
```

---

## 14. Erreurs fréquentes

### 14.1 Créer seulement `DA_Item_XXX`

Symptôme : l'item existe comme définition, mais il n'apparaît pas dans la palette et n'est pas facilement plaçable.

Correction : créer aussi `DA_Object_XXXPickup` et l'ajouter à `DA_ObjectPalette_Default`.

### 14.2 Créer seulement `DA_Object_XXXPickup`

Symptôme : l'objet est plaçable, mais l'inventaire ne connaît pas correctement son nom, son type, son icône ou ses tags.

Correction : créer `DA_Item_XXX` et l'assigner dans `DefaultBehavior.Item`.

### 14.3 Mettre les tags métier sur l'archétype au lieu de l'item

Symptôme : la compatibilité de serrure, recette ou filtre d'inventaire ne fonctionne pas de manière stable.

Correction : mettre `Key.Copper`, `Tool.LockpickSet`, `Quest`, etc. dans `DA_Item_XXX.ItemTags`.

### 14.4 Utiliser `DA_` dans les identifiants

Mauvais :

```text
ItemDefinitionId = DA_Item_CopperKey
ArchetypeId = DA_Object_KeyCopperPickup
```

Bon :

```text
ItemDefinitionId = Key_Copper
ArchetypeId = Item_CopperKey_Pickup
```

### 14.5 Confondre pickup et contenu de réceptacle

Mauvais :

```text
Receptacle.InitialContent = DA_Object_KeyCopperPickup
```

Bon :

```text
Receptacle.InitialContent = DA_Item_CopperKey
```

### 14.6 Oublier `DA_ObjectPalette_Default`

Symptôme : l'asset existe, mais l'éditeur ne le propose pas.

Correction : ajouter l'archétype dans la palette utilisée par l'éditeur.

---

## 15. Checklist création d'un nouvel item

### 15.1 Préparation visuelle

- [ ] Créer ou importer `SM_XXX`.
- [ ] Vérifier échelle, pivot, orientation.
- [ ] Créer ou importer `Icon_XXX`.
- [ ] Vérifier fond alpha et lisibilité à petite taille.
- [ ] Créer ou assigner matériaux.

### 15.2 Définition logique

- [ ] Créer `DA_Item_XXX`.
- [ ] Renseigner `ItemDefinitionId`.
- [ ] Renseigner `DisplayName`.
- [ ] Renseigner `Description`.
- [ ] Choisir `ItemType`.
- [ ] Renseigner `Weight`.
- [ ] Configurer `bStackable` / `MaxStackSize`.
- [ ] Assigner `Icon`.
- [ ] Assigner `WorldMesh`.
- [ ] Assigner `EquippedMesh` si nécessaire.
- [ ] Configurer `bThrowable` si nécessaire.
- [ ] Configurer lumière si nécessaire.
- [ ] Ajouter les `ItemTags`.

### 15.3 Archétype plaçable

- [ ] Créer `DA_Object_XXXPickup`.
- [ ] Renseigner `ArchetypeId`.
- [ ] Renseigner `DisplayName`.
- [ ] Mettre `SupportedType = Item`.
- [ ] Mettre `ObjectCategory = Item`.
- [ ] Choisir `Category`.
- [ ] Choisir `PlacementKind`.
- [ ] Assigner `PreviewMesh`.
- [ ] Assigner `ItemActorClass`.
- [ ] Configurer `PlacementZOffset`.
- [ ] Assigner `DefaultBehavior.Item.ItemDefinitionAsset`.
- [ ] Assigner `DefaultBehavior.Item.ItemDefinitionId`.

### 15.4 Palette

- [ ] Ajouter `DA_Object_XXXPickup` à `DA_ObjectPalette_Default`.
- [ ] Vérifier la catégorie dans l'éditeur.
- [ ] Vérifier que la preview s'affiche.

### 15.5 Placement

- [ ] Placer l'item dans `L_GrimrockEditor`.
- [ ] Vérifier `Type = Item`.
- [ ] Vérifier `ArchetypeId`.
- [ ] Vérifier `ItemDefinitionAsset`.
- [ ] Vérifier `ItemDefinitionId`.
- [ ] Vérifier position cellule/arête.

### 15.6 Réceptacle optionnel

- [ ] Placer l'item dans `Behavior.Receptacle.InitialContent` si nécessaire.
- [ ] Vérifier `Quantity`.
- [ ] Tester retrait vers inventaire.

### 15.7 Test PIE

- [ ] L'item apparaît dans le monde.
- [ ] Le curseur d'interaction apparaît.
- [ ] Le clic ramasse l'item.
- [ ] L'item est ajouté au personnage sélectionné.
- [ ] Le tooltip affiche nom, type, icône, description, poids.
- [ ] La quantité est correcte.
- [ ] L'item peut être déposé si le système le permet.
- [ ] L'item peut être replacé dans un réceptacle compatible.
- [ ] Le comportement spécifique fonctionne : clé, torche, pierre lançable, etc.

---

## 16. Checklist spécifique : clé

- [ ] `ItemType = Key`.
- [ ] `ItemDefinitionId` stable, par exemple `Key_Copper`.
- [ ] Tag générique `Key`.
- [ ] Tag spécifique `Key.Copper`.
- [ ] Non stackable.
- [ ] Non throwable au début, sauf décision contraire.
- [ ] Pas d'équipement.
- [ ] Poids faible.
- [ ] Icône claire.
- [ ] Mesh monde visible.
- [ ] Archétype pickup présent dans palette.
- [ ] Serrure compatible configurée avec `AcceptedKeyItems` et/ou `AcceptedKeyIds`.

---

## 17. Checklist spécifique : objet de poids / projectile

- [ ] `ItemType = Misc` ou `Component`.
- [ ] `bThrowable = true`.
- [ ] Paramètres de lancer configurés.
- [ ] Poids cohérent.
- [ ] Collision et mesh adaptés.
- [ ] Tags `Throwable`, `WeightObject` si utile.
- [ ] Test avec plaque de pression si l'objet doit servir de poids.

---

## 18. Règle finale

Pour tout nouvel item ramassable :

```text
DA_Item_XXX
  = identité d'inventaire et gameplay

DA_Object_XXXPickup
  = manière de placer cette identité dans un niveau

DA_ObjectPalette_Default
  = exposition dans l'éditeur

Réceptacle.InitialContent
  = contenu logique, donc DA_Item_XXX, jamais DA_Object_XXXPickup
```

Ne jamais confondre ces rôles.
