# Fondation du ramassage et du placement des items

## 1. Portée

Ce document décrit le socle réellement implémenté pour les items placés dans un niveau, leur ramassage, leur passage par l'inventaire ou le curseur, leur insertion dans un réceptacle et leur représentation visuelle lorsqu'ils sont équipés. Il ne définit ni butin aléatoire, ni empilement avancé, ni dépôt libre dans le monde.

## 2. Cartographie du code

| Domaine | Déclaration | Implémentation |
|---|---|---|
| Objet placé | `Source/GrimrockPrototype/Public/Core/GridTypes.h` | structure sans `.cpp` |
| Paramètres d'archétype | `Source/GrimrockPrototype/Public/Core/GridObjectArchetypeAsset.h` | `Source/GrimrockPrototype/Private/Core/GridObjectArchetypeAsset.cpp` |
| Définition d'item | `Source/GrimrockPrototype/Public/Runtime/GridItemDefinitionAsset.h` | `Source/GrimrockPrototype/Private/Runtime/GridItemDefinitionAsset.cpp` |
| Instance et propriété | `Source/GrimrockPrototype/Public/Runtime/GridInventoryTypes.h` | structures sans `.cpp` |
| Acteur d'item | `Source/GrimrockPrototype/Public/Runtime/GridItemActor.h` | `Source/GrimrockPrototype/Private/Runtime/GridItemActor.cpp` |
| Génération et ramassage | `Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h` | `Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp` |
| Inventaire et curseur | `Source/GrimrockPrototype/Public/Runtime/GridPartyInventoryComponent.h` | `Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponent.cpp` |
| Transferts atomiques | `Source/GrimrockPrototype/Public/Runtime/GridItemTransferService.h` | `Source/GrimrockPrototype/Private/Runtime/GridItemTransferService.cpp` |
| Groupe et visuel tenu | `Source/GrimrockPrototype/Public/Runtime/GrimrockPartyPawn.h` | `Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawn.cpp` |
| Souris | `Source/GrimrockPrototype/Public/Runtime/GrimrockPlayerController.h` | `Source/GrimrockPrototype/Private/Runtime/GrimrockPlayerController.cpp` |
| Validation éditeur | `Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEditorActor.h` | `Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActor.cpp` |

### Vocabulaire

| Terme | Rôle réel |
|---|---|
| `ItemDefinitionAsset` | référence directe vers les données stables d'un item |
| `ItemDefinitionId` | identifiant stable utilisable sans référence directe chargée |
| `ItemActor` | représentation physique ou visuelle d'un item |
| `ItemInstance` | identité runtime, quantité, poids, lumière et propriétaire logique |
| `PlacedObject` | enregistrement persistant d'un item dans `UGridLevelAsset::Objects` |
| `ContainedReceptacleItem` | état d'une instance possédée et éventuellement visualisée par un réceptacle |
| `CursorItem` | instance temporairement possédée par le curseur |
| `InventoryItem` | instance stockée dans une case d'un personnage du groupe |

## 3. Modèle de données

`UGridItemDefinitionAsset` est la définition stable : identifiant, type, poids, emplacements d'équipement compatibles, icône, maillages, lumière et tags.

`FGridItemInstance` est l'instance runtime. Son identité est `RuntimeObjectId`; sa nature est `ItemDefinitionId`. Les champs `OwnerType`, `OwnerGuid`, `OwnerCharacterIndex` et `EquipmentSlot` décrivent son propriétaire logique.

`FGridLevelObjectData` de type `Item` est un placement persistant. Sa définition effective est résolue dans cet ordre :

1. `ItemDefinitionAsset` de l'objet ;
2. `ItemDefinitionId` de l'objet ;
3. définition d'item des paramètres par défaut de l'archétype.

`ArchetypeId` sélectionne l'archétype de placement et de génération. Il ne doit pas remplacer une définition d'item explicite dans les nouvelles données.

## 4. Génération dans le monde

`AGridLevelRuntimeActor::RebuildRuntimeObjects()` traite les objets `Item` actifs par `AddPlacedItemActor()`. L'acteur créé est enregistré dans `SpawnedItemEntries` avec sa cellule, son arête, son identifiant d'objet et sa définition.

Un item avec `Edge=None` est placé au centre de la cellule. Un item avec une arête cardinale utilise le placement au bord du sol, même si son archétype est normalement centré. Ce comportement est propre aux items placés.

`AGridItemActor::ConfigureAsWorldPickup()` active la collision, la visibilité et la physique nécessaires au ramassage. L'état lumineux d'un item placé par le niveau est actuellement désactivé après sa création par `OnRemovedFromWorld()`. Une torche contenue ou tenue suit un autre chemin lumineux.

## 5. Accessibilité du ramassage

La portée physique du clic ne suffit pas. `CanPartyPickupItemEntry()` impose aussi une règle de grille.

![Accessibilité d'un item placé](../Images/item_10_1_pickup_accessibility.svg)

- item central, `Edge=None` : le groupe doit être dans la même cellule ;
- item sur une arête de la cellule du groupe : l'arête doit être celle que le groupe regarde ;
- item dans la cellule située devant : il doit être sur l'arête opposée à la direction regardée, donc face au groupe ;
- toute autre cellule ou arête est refusée.

`AGridItemActor::CanInteract()` appelle la même vérification que le ramassage final. Le curseur de survol ne promet donc plus une action que `TryPickupItemActor()` refuserait ensuite.

Les items contenus dans un réceptacle délèguent leur interaction au réceptacle propriétaire. Ils ne passent pas par cette règle de ramassage direct.

### Chemin souris et obstacles

`AGrimrockPlayerController` déprojette la souris puis effectue un unique rayon `ECC_Visibility`. Seul le premier impact bloquant est examiné. Le chemin est :

1. premier impact `Visibility` ;
2. acteur implémentant `IGridInteractableInterface` ;
3. distance maximale ;
4. `CanInteract()` ;
5. curseur d'interaction ;
6. `InteractWithHit()` ;
7. validation de grille et transfert.

Un mur, une porte fermée ou un autre composant bloquant `Visibility` masque donc l'item. Aucun second rayon ni repli implicite ne cherche une cible derrière l'obstacle.

## 6. Destination d'un ramassage

Le ramassage monde ne place pas l'item sur le curseur. `TryPickupItemActor()` et `TryPickupItemAtCell()` construisent une `FGridItemInstance`, puis appellent `AddItemInstanceToSelectedCharacterInventory()`.

L'acteur monde n'est détruit et retiré de `SpawnedItemEntries` qu'après l'ajout réussi. Si l'inventaire ne possède aucune case libre, l'item reste intact dans le monde.

La capacité actuelle dépend des cases libres. Le poids est recalculé, mais ne bloque pas l'ajout.

## 7. Curseur et transferts

`UGridPartyInventoryComponent` est la source de vérité du curseur avec `bHasCursorItem` et `CursorItem`. Un item pris depuis une case d'inventaire ou d'équipement change de propriétaire logique pour `Cursor`.

![Flux du curseur](../Images/item_10_2_cursor_transfer_flow.svg)

- inventaire vers curseur : refus si le curseur est occupé ;
- curseur vers case vide : déplacement puis vidage du curseur ;
- curseur vers case occupée : échange ;
- curseur vers inventaire plein : refus, curseur inchangé ;
- curseur vers réceptacle : le curseur est vidé seulement après insertion réussie ;
- dépôt libre du curseur dans le monde : `TryDropCursorItem()` est déclaré mais retourne actuellement `NotImplemented`.

`UGridItemTransferService` couvre les transferts inventaire vers réceptacle, équipement vers réceptacle et réceptacle vers inventaire. Il capture la source et la restaure si l'écriture de destination échoue.

![Propriétaires logiques d'une instance](../Images/item_10_4_world_inventory_receptacle.svg)

### Retour visuel

- `Take` : item directement touché, à portée et accessible ;
- `Forbidden` : premier acteur interactif touché hors de portée ;
- `PlaceItem` : curseur occupé et réceptacle directement touché, accessible et compatible ;
- `CannotPlaceItem` : curseur occupé sans cible directe valide, hors portée ou refusé ;
- `Locked` : valeur du contrat d'interface, mais non produite par le chemin générique des items audité ici.

Quand le curseur est occupé, le clic monde ne recherche qu'un réceptacle directement touché. Il n'existe ni dépôt magique dans une cible voisine, ni dépôt libre au sol ou sur une arête.

## 8. Torches

Une torche peut apparaître sous trois formes distinctes :

- item placé dans le niveau : `AGridItemActor` autonome, ramassable selon cellule et arête ;
- item contenu dans un support : instance possédée par `AGridReceptacleActor`, interaction et retrait gouvernés par sa politique ;
- item équipé : instance dans l'équipement, représentée par un `HeldItemActor` visuel attaché au groupe.

![Modèle des torches](../Images/item_10_3_torch_holder_model.svg)

`HeldItemActor` n'est pas une seconde instance de gameplay. Le commentaire et le code de `AGrimrockPartyPawn` le traitent comme une représentation visuelle ; la propriété réelle reste dans l'inventaire, l'équipement, le curseur, un réceptacle ou le monde.

`HeldTorchActorClass` est une classe visuelle spécialisée utilisée pour `DefaultHeldItemDefinitionId`. Les autres items tenus sont générés par le runtime à partir de leur définition.

## 9. Validation éditeur

`AGridLevelEditorActor::ValidateCurrentLevel()` signale désormais :

- un item sans définition résoluble dans l'objet ou l'archétype ;
- un asset de définition dont `ItemDefinitionId` est vide ;
- un conflit entre `ItemDefinitionAsset` et `ItemDefinitionId` locaux ;
- un item placé sur une cellule vide ou bloquant l'occupation.

Les validations existantes continuent de contrôler l'identifiant d'objet, l'archétype, le type, la palette et le placement.

## 10. Diagnostics

Les refus de ramassage journalisent la cellule du groupe, son orientation, la cellule de l'item et son arête. Les échecs d'ajout indiquent `InventoryFull`. Les transferts du service journalisent l'opération, le résultat, `ItemDefinitionId` et `RuntimeObjectId`. Les refus de dépôt souris distinguent notamment l'absence de cible, la portée, l'arête et l'incompatibilité.

## 11. Invariants

- Une instance valide possède un `RuntimeObjectId`, un `ItemDefinitionId` et une quantité positive.
- Une instance ne doit avoir qu'un propriétaire logique à la fois.
- Un refus de destination ne doit pas supprimer la source.
- Le survol et l'action finale utilisent la même règle d'accessibilité.
- Un acteur visuel tenu ne constitue pas une nouvelle instance.
- Un item contenu appartient au réceptacle et ne doit pas être ramassé directement par le niveau.

## 12. Tests manuels PIE

1. Ramasser un item central depuis sa cellule : il rejoint l'inventaire sélectionné.
2. Cliquer un item central depuis une cellule adjacente ou non adjacente : le curseur ne propose pas le ramassage.
3. Dans la cellule du groupe, vérifier qu'un item d'arête est accessible uniquement en regardant cette arête.
4. Dans la cellule devant le groupe, vérifier que seule l'arête faisant face au groupe est accessible.
5. Placer l'item derrière un mur, une porte fermée puis un autre bloqueur `Visibility` : le premier impact empêche le ramassage.
6. Remplir l'inventaire, puis tenter un ramassage : l'acteur monde reste présent.
7. Déplacer un item inventaire vers le curseur, tenter avec un curseur déjà occupé, puis tester une case vide, une case occupée et un inventaire plein.
8. Cliquer sans réceptacle direct, hors portée, sur un réceptacle incompatible puis compatible : le curseur reste inchangé sur les refus.
9. Tester le retrait autorisé, interdit et verrouillé d'un réceptacle.
10. Prendre une torche depuis le bon puis le mauvais côté d'un support ; la redéposer dans un support simple puis retournable.
11. Vérifier la lumière, le visuel du support et le visuel tenu après retrait et redépôt.
12. Valider un niveau contenant un item sans définition, sur cellule non jouable et avec identifiants contradictoires.

## 13. Limites actuelles

- Le dépôt libre d'un item du curseur dans le monde n'est pas implémenté.
- Le ramassage monde alimente directement l'inventaire, pas le curseur.
- Les piles ne sont pas fusionnées par le chemin de ramassage.
- Les actifs `.uasset` déterminent les variantes concrètes de supports et ne sont pas audités par ce document.
