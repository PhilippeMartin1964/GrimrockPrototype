# Fondation du ramassage et du placement des items

## 1. Portée

Ce document décrit le socle réellement implémenté pour les items placés dans un niveau, leur ramassage, leur passage par l'inventaire ou le curseur, leur insertion dans un réceptacle, leur dépôt libre dans le monde et leur représentation visuelle lorsqu'ils sont équipés. Il ne définit pas de butin aléatoire.

## 2. Cartographie du code

| Domaine | Déclaration | Implémentation |
|---|---|---|
| Objet placé | `Source/GrimrockPrototype/Public/Core/GridTypes.h` | structure sans `.cpp` |
| Paramètres de définition | `Source/GrimrockPrototype/Public/Core/GridWorldObjectDefinitionAsset.h` | `Source/GrimrockPrototype/Private/Core/GridWorldObjectDefinitionAsset.cpp` |
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
| `FGridLooseItemInstance` | placement persistant dans `UGridLevelAsset::LooseItemInstances` |
| `ContainedReceptacleItem` | état d'une instance possédée et éventuellement visualisée par un réceptacle |
| `CursorItem` | instance temporairement possédée par le curseur |
| `InventoryItem` | instance stockée dans une case d'un personnage du groupe |

## 3. Modèle de données

`UGridItemDefinitionAsset` est la définition stable : identifiant, type, poids, emplacements d'équipement compatibles, icône, maillages, lumière et tags.

`FGridItemInstance` est l'instance runtime. Son identité est `RuntimeObjectId`; sa nature est `ItemDefinitionId`. Les champs `OwnerType`, `OwnerGuid`, `OwnerCharacterIndex` et `EquipmentSlot` décrivent son propriétaire logique.

`FGridLooseItemInstance` est le placement persistant. Sa référence `ItemDefinition` est l'unique définition canonique du collectible. `ItemDefinitionId` identifie la définition dans les états runtime/save ; il ne constitue pas une seconde autorité d'authoring du placement.

La palette référence directement `DefaultItemDefinition`, sans `DefaultWorldObjectDefinition` compagnon. Le placement n'utilise aucun fallback de définition world-object. Voir le [contrat courant](WORLD_OBJECT_DEFINITIONS_AND_PLACED_OBJECTS.md).

## 4. Génération dans le monde

`AGridLevelRuntimeActor::RebuildRuntimeObjects()` traite les objets `Item` actifs par `AddPlacedItemActor()`. L'acteur créé est enregistré dans `SpawnedItemEntries` avec sa cellule, son arête, son identifiant d'objet et sa définition.

Un item avec `Edge=None` est placé au centre de la cellule. Un item avec une arête cardinale utilise le placement au bord du sol, même si sa définition est normalement centré. Ce comportement est propre aux items placés.

`AGridItemActor::ConfigureAsWorldPickup()` active la collision, la visibilité et la physique nécessaires au ramassage. Depuis `ITEM-LIGHT02`, un item neuf matérialisé depuis une `UGridItemDefinitionAsset` initialise son état lumineux exactement une fois depuis `ItemDefinition->IsLightEnabledByDefault()`. Les transferts ultérieurs conservent ensuite `FGridItemInstance::bLightsEnabled` au lieu de recalculer cet état depuis la définition.

Un item inséré dans un réceptacle `PhysicalAtHit` est détaché, positionné au point cliqué avec l'offset de surface, puis configuré comme pickup physique si `bSimulatePhysicsWhenPlaced` est actif.

## 5. Accessibilité du ramassage

La portée physique du clic ne suffit pas. `CanPartyPickupItemEntry()` impose aussi une règle de grille.

![Accessibilité d'un item placé](../Images/item_10_1_pickup_accessibility.svg)

- item libre au sol, `Edge=None` : distance horizontale physique <= `WorldItemPickupReach`, **200 cm** par défaut ;
- aucune cellule cardinale ni orientation du groupe n’est imposée à un item libre visible à portée ;
- le premier impact du trace `Visibility` détermine l’obstacle ; `CanMove()` n’intervient pas ;
- une cible diagonale ou dans une autre cellule reste accessible si sa distance physique respecte la portée ;
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

Un composant qui bloque effectivement le rayon `Visibility` masque l’item. Le rayon peut passer entre les barreaux d’une grille fermée. Aucun second rayon ni repli ne cherche une cible derrière un impact bloquant.

## 6. Destination d'un ramassage

Le ramassage monde ne place pas l'item sur le curseur. `TryPickupItemActor()` et `TryPickupItemAtCell()` construisent une `FGridItemInstance`, puis appellent `AddItemInstanceToSelectedCharacterInventory()`.

L'acteur monde n'est détruit et retiré de `SpawnedItemEntries` qu'après l'ajout réussi. Si l'inventaire ne possède pas la capacité nécessaire dans ses piles existantes et ses cases libres, l'item reste intact dans le monde.

Le poids est recalculé après l'ajout, mais ne bloque pas l'opération.

### Empilement des items

Lorsqu'un item rejoint l'inventaire, `UGridPartyInventoryComponent` applique sa `UGridItemDefinitionAsset`.

Si la définition indique `bStackable=true`, l'ajout complète d'abord les piles existantes du même `ItemDefinitionId` jusqu'à `MaxStackSize`, puis crée une nouvelle pile si nécessaire.

L'opération reste atomique : si toute la quantité ne peut pas être ajoutée, l'inventaire n'est pas modifié et l'item source reste intact. Aucun slot supplémentaire n'est créé automatiquement lorsque l'inventaire est plein.

Le ramassage d'un item placé initialement ajoute une quantité de 1. Un item runtime déposé dans le monde conserve la quantité portée par sa pile.

### Séparation de pile

Les items stackables peuvent être séparés depuis l'interface d'inventaire.

La règle UI retenue est :

- clic ou drag normal : prend ou déplace la pile complète ;
- CTRL + clic ou drag : prend une seule unité de la pile.

Une unité séparée reçoit un nouveau `RuntimeObjectId`. La pile restante conserve son `RuntimeObjectId`.

Cette logique est gérée par `UGridPartyInventoryComponent` afin de rester indépendante des Blueprints UI. Un item non stackable ou une pile d'une seule unité est toujours pris intégralement.

### Résolution automatique des définitions

Le composant d'inventaire peut enregistrer automatiquement les définitions d'items rencontrées pendant le runtime.

Quand un item est ramassé depuis le niveau, la définition est résolue via `AGridLevelRuntimeActor::ResolveRuntimeItemDefinition`, puis enregistrée dans `UGridPartyInventoryComponent`.

Cela évite de devoir renseigner manuellement chaque `DA_Item_*` dans le Blueprint du pawn, tant que l'item provient d'un LevelAsset, d'une définition ou d'un contenu runtime résoluble.

### Tags d'items

Les tags métier d'un item appartiennent à `UGridItemDefinitionAsset.ItemTags`.

Le placement conserve éventuellement un `Tag` local ; il ne recopie pas les tags métier dans une définition world-object compagnon. La palette valide directement `DefaultItemDefinition` pour les collectibles.


La portée de main canonique est `AGridLevelRuntimeActor::WorldItemPickupReach`, **200 cm** par défaut. Son nom sérialisé est conservé pour compatibilité Blueprint. Le pickup d’un item libre, la pose depuis le CursorItem ou la hotbar et la décision `PlaceItem` / `AimThrow` utilisent ce même paramètre. La distance est horizontale, entre le PartyPawn et la position physique de l’item ou le point d’impact ciblé ; elle prime sur l’adjacence logique des cellules. `CanMove()` n’est pas consulté pour un item libre visible à portée. Les items d’arête et les réceptacles conservent leurs règles spécifiques.

Le premier impact bloquant du trace `Visibility` possède l’obstacle : un mur plein ou un barreau touché intercepte le rayon ; un espace entre les barreaux laisse atteindre la cible. Une porte fermée ne constitue donc pas, à elle seule, un veto logique. Le projectile conserve sa propre collision physique : il ne traverse la grille que si sa collision passe réellement entre les barreaux.

## 7. Curseur et transferts

`UGridPartyInventoryComponent` est la source de vérité du curseur avec `bHasCursorItem` et `CursorItem`. Un item pris depuis une case d'inventaire ou d'équipement change de propriétaire logique pour `Cursor`.

![Flux du curseur](../Images/item_10_2_cursor_transfer_flow.svg)

- inventaire vers curseur : refus si le curseur est occupé ;
- curseur vers case vide : déplacement puis vidage du curseur ;
- curseur vers case occupée : échange ;
- curseur vers inventaire plein : refus, curseur inchangé ;
- curseur vers réceptacle : le curseur est vidé seulement après insertion réussie ;
- dépôt libre du curseur dans le monde : création d'un item runtime, puis vidage du curseur seulement après succès.

`UGridItemTransferService` couvre les transferts inventaire vers réceptacle, équipement vers réceptacle et réceptacle vers inventaire. Il capture la source et la restaure si l'écriture de destination échoue.

![Propriétaires logiques d'une instance](../Images/item_10_4_world_inventory_receptacle.svg)

### Retour visuel

- `Take` : item directement touché, à portée et accessible ;
- `Forbidden` : premier acteur interactif touché hors de portée ;
- `PlaceItem` : curseur occupé et réceptacle directement touché, accessible et compatible ;
- `CannotPlaceItem` : curseur occupé sans cible directe valide, hors portée ou refusé ;
- `Locked` : valeur du contrat d'interface, mais non produite par le chemin générique des items audité ici.

Quand le curseur est occupé, un réceptacle directement touché garde la priorité. Si le premier hit `Visibility` n'est pas un réceptacle, le contrôleur tente un dépôt libre sur la cellule résolue depuis le hit. Aucun fallback ne traverse un impact bloquant ; un rayon passant entre les barreaux peut néanmoins atteindre une cible derrière une grille fermée.

### Dépôt libre dans le monde

Un item détenu par le curseur peut être déposé librement dans le monde, sans passer par un réceptacle.

Le dépôt crée une instance runtime d'item dans le niveau courant. Le `LevelAsset` n'est pas modifié : il reste la description des placements initiaux du level designer.

L'opération est atomique :

- si le dépôt réussit, le curseur est vidé ;
- si le dépôt échoue, le curseur conserve l'item.

Un item déposé est enregistré dans `SpawnedItemActors` et `SpawnedItemEntries`, puis redevient ramassable par le système de pickup standard. Sa quantité est conservée, qu'il s'agisse d'une pile complète ou d'une unité séparée.

`FGridLevelRuntimeState::Items` capture son `RuntimeObjectId`, son `ItemDefinitionId`, sa quantité, sa cellule, son arête et sa transform. Le dépôt est donc restauré lors d'une transition de niveau sans créer de placement dans le DataAsset.

La pose accepte une cellule jouable résolue depuis un hit `Visibility` à portée de main. Le décalage horizontal exact du point d’impact est conservé, sans recentrage qui pourrait repousser la destination hors de portée.

### Lancer d'item

Les items peuvent être rendus lançables via leur `UGridItemDefinitionAsset`.

La première version utilise :

- `bThrowable` pour autoriser le lancer ;
- `ThrowSpeed` pour la vitesse initiale ;
- `ThrowArc` pour une légère composante verticale ;
- `ThrowLifeSeconds` pour la durée maximale du projectile ;
- `ThrowImpactDropOffset` pour stabiliser le dépôt après impact.

### Interaction souris avec item en curseur

Le clic droit est réservé au free look / mouvement de tête du groupe. Le système de lancer ne doit jamais détourner `RightMouseButton`.

Le clic gauche est contextuel lorsqu'un item est présent dans le curseur :

1. Si la cible est un réceptacle compatible et accessible, l'item est placé dans le réceptacle.
2. Sinon, si la cible est une zone proche compatible, l'item est posé librement dans le monde.
3. La frontière physique est inclusive et commune au curseur et à la hotbar :
   - cible à distance horizontale <= portée de main : pose directe ; un refus conserve la source, sans projectile ;
   - cible à distance horizontale > portée de main : lancer normal, sous réserve des conditions physiques de lancer.
4. Si l'item n'est pas lançable et ne peut pas être posé, l'action échoue sans modifier le curseur.

Le clic gauche sans item en curseur conserve le comportement d'interaction normal : boutons, leviers, items monde, panneaux, torches, etc.

### Dépiler depuis le curseur

Lorsqu'un item stackable est porté par le curseur, les actions monde consomment une seule unité par clic.

Exemples :

- `Pierre x5` + clic gauche sol proche : pose `Pierre x1`, le curseur devient `Pierre x4`.
- `Pierre x4` + clic gauche réceptacle : insère `Pierre x1`, le curseur devient `Pierre x3`.
- `Pierre x3` + clic gauche lointain : lance `Pierre x1`, le curseur devient `Pierre x2`.

Le système ne dépose pas toute la pile par défaut. Un dépôt complet de pile pourra être ajouté plus tard avec un modificateur dédié. L'unité séparée reçoit son propre `RuntimeObjectId`, tandis que la pile restante conserve le sien. Le curseur n'est décrémenté qu'après la réussite de l'action monde.

### Information donnée par le curseur

Le curseur ne doit pas révéler les objets interactifs situés à plusieurs cellules.

Pour les items libres, `Take` et `PlaceItem` suivent la portée physique de main. Les interactions d’arête et les autres acteurs conservent leurs contrôles spécifiques de distance et d’accessibilité.

Un objet visible mais trop éloigné ne doit pas afficher un curseur `Forbidden`. Le curseur reste neutre.

Lorsqu'un item lançable est porté par le curseur, un état de visée (`AimThrow`) peut être affiché pour indiquer que l'objet peut être lancé vers la cible visible. Cet état ne révèle pas une interaction distante ; il indique seulement la possibilité de lancer l'objet tenu.

Le Blueprint `WBP_GridMouseCursor` associe `PlaceItem` à `Cursor_PlaceItem` et `AimThrow` à `Cursor_Aim`.

Le clic droit reste exclusivement réservé au free look / mouvement de tête du groupe.

### Registre runtime des définitions d'items

Les définitions `DA_Item_xxx` ne sont plus ajoutées manuellement dans `BP_GrimrockPartyPawn`.

Le composant d'inventaire maintient un registre runtime interne non éditable. Les définitions sont enregistrées automatiquement lorsqu'un item issu du niveau ou du runtime référence un `UGridItemDefinitionAsset`.

Le flux normal est :

1. `DA_Object_xxx` référence `DA_Item_xxx` dans `DefaultBehavior.Item.ItemDefinitionAsset`.
2. Le runtime résout cette définition au spawn ou au pickup.
3. L'inventaire enregistre automatiquement cette définition.
4. Les données d'item sont appliquées depuis `DA_Item_xxx`.

Le tableau manuel `Items.ItemDefinitions` n'est plus exposé dans `BP_GrimrockPartyPawn`.

Les alcôves acceptent plusieurs items, tandis que les supports de torche restent single-slot. Lorsqu'un item du curseur vise un réceptacle accessible, le placement ou son refus est traité par le réceptacle et ne retombe pas dans le lancer.

Seul le slot cursor permet cette insertion par clic. Un item équipé en `MainHand` ou `OffHand` reste équipé et ne modifie pas le comportement de retrait d'un item déjà contenu.

### Jet court et lancer

Le lancer physique et l'attaque de jet sont deux notions séparées qui partagent le même projectile `AGridThrownItemActor`.

#### Lancer physique / exploration

Un objet peut être lancé sans compétence de combat lorsqu'il est manipulable à une main et que son poids ne dépasse pas la capacité du personnage :

```text
MaxThrowableWeightKg = Strength * 0.25
```

Exemples : Force 10 = 2,5 kg ; Force 12 = 3,0 kg ; Force 20 = 5,0 kg.

La Force et le rapport poids réel / poids maximal modulent aussi `ThrowSpeed`. À poids égal, un personnage plus fort lance plus vite ; un objet proche de sa limite part moins vite.

Le lancer utilitaire canonique part de la `MainHand` du personnage sélectionné. L'action contextuelle **Lancer** devient désactivée avec une raison explicite lorsque l'objet est trop lourd ou non manipulable à une main. Le lancer depuis le curseur reste un raccourci secondaire de l'interaction souris et applique la même règle physique.

Aucune compétence n'est requise pour le lancer utilitaire : une pierre peut donc franchir une fosse et atteindre une PressurePlate. À l'impact le projectile redevient un item monde, donc son `Weight` participe immédiatement au calcul de la plaque.

#### Attaque de jet

Une attaque de jet reste une action de combat distincte. `bCombatThrowWeapon` indique qu'une action de combat consomme et lance physiquement l'objet. Les anciens assets basés sur `bThrowable` restent compatibles mais ce champ n'est plus authoré.

Les compétences restent data-driven via `FGridCombatActionDefinition::Requirements`. Une pierre peut ainsi être lançable physiquement par tout personnage assez fort tout en exigeant une compétence de lancer pour être utilisée comme véritable attaque.

#### Présentation

`EGridThrowVisualMode` fournit :

- `Stable` : aucune rotation supplémentaire ;
- `Tumble` : rotation lente multi-axes, valeur par défaut pour pierre/bouteille/objet irrégulier ;
- `Spin` : rotation rapide, adaptée aux shurikens et disques.

Le défaut `Tumble` utilise 180°/s au lieu de l'ancien spin générique à 1080°/s. Le shuriken doit être authoré en `Spin` pour conserver sa rotation rapide.

Chaque lancer utilise la vitesse et l’arc normaux de la définition d’item, avec le facteur de Force. Une seule unité est transférée, puis l’impact ou l’expiration utilise le dépôt monde standard afin que l’objet redevienne ramassable, persistant et compatible avec les PressurePlates par poids.

### PressurePlates et poids des items

Les PressurePlates peuvent être activées par la présence du joueur, par le poids des items déposés sur leur cellule, ou par les deux.

La configuration logique est portée par `PressurePlateWeight` :

- `bActivateWhenPartyPresent` conserve le comportement classique : le joueur active la plaque en marchant dessus ;
- `bUseItemWeight` active le calcul du poids des items déposés ;
- `RequiredItemWeight` définit le seuil minimal à atteindre ;
- `bCountEdgeItems` permet de compter aussi les items placés sur un bord de cellule.

Le poids total est calculé à partir des `UGridItemDefinitionAsset` :

```text
TotalWeight = Somme(ItemDefinition.Weight * Quantity)
```

Les items déposés dans le monde sont stockés dans l'état runtime. Le `LevelAsset` n'est pas modifié.

L'évaluation est centralisée dans `UGridActivationComponent::RefreshPressurePlatesAtCell`. Elle est relancée quand le groupe change de cellule, quand un item est déposé ou ramassé, et après la restauration d'un niveau. La plaque ne déclenche `Activated` ou `Deactivated` que lorsque son état logique change.

## 8. Torches

Une torche peut apparaître sous trois formes distinctes :

- item placé dans le niveau : `AGridItemActor` autonome, ramassable selon cellule et arête ;
- item contenu dans un support : instance possédée par `AGridReceptacleActor`,
  interaction et retrait gouvernés par `bCanRemoveItem` ;
- item équipé : instance dans l'équipement, représentée par un `HeldItemActor` visuel attaché au groupe.

![Modèle des torches](../Images/item_10_3_torch_holder_model.svg)

`HeldItemActor` n'est pas une seconde instance de gameplay. Le commentaire et le code de `AGrimrockPartyPawn` le traitent comme une représentation visuelle ; la propriété réelle reste dans l'inventaire, l'équipement, le curseur, un réceptacle ou le monde.

Tous les items tenus, torche comprise, utilisent désormais l'`AGridItemActor` générique initialisé depuis `UGridItemDefinitionAsset`. `HeldTorchActorClass`, `DefaultHeldItemDefinitionId` et les anciens chemins spécifiques à la torche ont été physiquement supprimés du contrat runtime. Le mesh tenu vient de `EquippedMesh`, avec `WorldMesh` comme fallback via `LoadHeldMesh()`.

La position du **mesh tenu** est contrôlée par `HeldItemRoot` et `HeldItemRelativeLocation / Rotation / Scale`. La position de la **flamme Niagara** est contrôlée par `DA_Item_Torch.LightEmitter.NiagaraRelativeLocation / Rotation`. `GridPartyIllumination` est un proxy séparé qui ne porte que le PointLight ergonomique du groupe : déplacer ce composant ne déplace ni le mesh de la torche ni sa flamme.

Lorsqu'une torche équipée est allumée, son `HeldItemActor` garde le canal Niagara mais délègue son PointLight à `UGridPartyIlluminationComponent`. Voir [`PARTY_LIGHT01_EQUIPMENT_DRIVEN_PARTY_ILLUMINATION.md`](PARTY_LIGHT01_EQUIPMENT_DRIVEN_PARTY_ILLUMINATION.md).

## 9. Validation éditeur

`AGridLevelEditorActor::ValidateCurrentLevel()` signale désormais :

- un item sans définition résoluble dans l'objet ou la définition ;
- un asset de définition dont `ItemDefinitionId` est vide ;
- un conflit entre `ItemDefinitionAsset` et `ItemDefinitionId` locaux ;
- un item placé sur une cellule vide ou bloquant l'occupation.

Les validations existantes continuent de contrôler l'identifiant d'objet, la définition, le type, la palette et le placement.

## 10. Diagnostics

Les refus de ramassage journalisent la cellule du groupe, son orientation, la cellule de l'item et son arête. Les échecs d'ajout indiquent `InventoryFull`. Les transferts du service journalisent l'opération, le résultat, `ItemDefinitionId` et `RuntimeObjectId`. Les refus de dépôt souris distinguent notamment l'absence de cible, la portée, l'arête et l'incompatibilité.

Les évaluations d'acceptation utilisées par le survol sont silencieuses. Un clic réellement refusé peut produire un warning court. Le détail complet des règles et de l'item candidat n'est journalisé qu'avec `bLogDiagnostics=true`, au niveau `VeryVerbose`.

## 11. Invariants

- Une instance valide possède un `RuntimeObjectId`, un `ItemDefinitionId` et une quantité positive.
- Une instance ne doit avoir qu'un propriétaire logique à la fois.
- Un refus de destination ne doit pas supprimer la source.
- Un ajout d'inventaire doit accepter toute la quantité demandée ou ne modifier aucun slot.
- Chaque pile nouvellement créée possède son propre `RuntimeObjectId`.
- Une unité séparée possède un nouvel identifiant ; la pile source conserve le sien.
- Un dépôt monde ne modifie jamais le `LevelAsset`.
- Le survol et l'action finale utilisent la même règle d'accessibilité.
- Un acteur visuel tenu ne constitue pas une nouvelle instance.
- Un item contenu appartient au réceptacle et ne doit pas être ramassé directement par le niveau.

## 12. Tests manuels PIE

1. Ramasser un item central depuis sa cellule : il rejoint l'inventaire sélectionné.
2. Vérifier le pickup à 200 cm exactement, puis son refus au-delà. Une cible diagonale visible à portée reste accessible.
3. Dans la cellule du groupe, vérifier qu'un item d'arête est accessible uniquement en regardant cette arête.
4. Dans la cellule devant le groupe, vérifier que seule l'arête faisant face au groupe est accessible.
5. Vérifier le pickup derrière une grille fermée : barreau touché = cible masquée ; espace entre barreaux = cible accessible à portée. Un mur plein intercepte toujours le rayon.
6. Ramasser plusieurs exemplaires d'un item stackable : les piles existantes sont complétées jusqu'à `MaxStackSize`, puis une nouvelle pile est créée.
7. Avec une pile de trois unités, CTRL + clic puis CTRL + drag : le curseur reçoit une unité et la pile source en conserve deux.
8. Avec une pile dans le curseur, déposer sur une cellule valide : une seule unité apparaît et la pile du curseur est décrémentée.
9. Ramasser les items déposés : leurs quantités rejoignent les piles compatibles de l'inventaire.
10. Tenter un dépôt hors portée, hors des cellules autorisées ou sans définition résoluble : le curseur reste inchangé.
11. Remplir les piles et les cases libres, puis tenter un ramassage : l'acteur monde reste présent et l'inventaire ne change pas.
12. Déplacer un item inventaire vers le curseur, tenter avec un curseur déjà occupé, puis tester une case vide, une case occupée et un inventaire plein.
13. Cliquer hors portée, sur un réceptacle incompatible puis compatible : le curseur reste inchangé sur les refus.
14. Tester le retrait autorisé, puis désactivé et réactivé par les commandes de réceptacle.
15. Prendre une torche depuis le bon puis le mauvais côté d'un support ; la redéposer dans le support.
16. Vérifier la lumière, le visuel du support et le visuel tenu après retrait et redépôt.
17. Valider un niveau contenant un item sans définition, sur cellule non jouable et avec identifiants contradictoires.
18. Avec `bActivateWhenPartyPresent=true` et `bUseItemWeight=false`, entrer puis sortir d'une PressurePlate : vérifier un seul `Activated`, puis un seul `Deactivated`.
19. Avec une plaque à poids seul et un seuil de `3.0`, déposer trois pierres de poids `1.0`, puis en ramasser une : vérifier l'activation à `3.0` et la désactivation à `2.0`.
20. Déposer directement une pile de trois pierres : vérifier que la contribution vaut `3.0`, pas `1.0`.
21. Avec les deux sources actives, laisser trois pierres sur la plaque puis entrer et sortir : vérifier que la plaque reste pressée sans événement supplémentaire.
22. Relier la plaque à une porte et vérifier que l'activation et la désactivation par poids utilisent les mêmes liens que la présence du groupe.
23. Maintenir le clic droit et déplacer la souris : vérifier le free look et l'absence de projectile.
24. Avec une pierre dans le curseur, cliquer gauche sur un réceptacle compatible proche : vérifier que le réceptacle reste prioritaire.
25. Cliquer gauche sur une cellule valide proche : vérifier un dépôt libre sans projectile.
26. Avec une pile de trois pierres, viser une cible non posable à <= 200 cm : vérifier le refus, zéro projectile et une quantité inchangée.
27. À 200 cm, vérifier une pose directe. À 200,01 cm et 201 cm, vérifier un lancer normal d’une unité.
28. Lancer la dernière unité d'une pile : vérifier que le curseur est vidé seulement après la création du projectile.
29. Essayer de lancer un item avec `bThrowable=false` : vérifier le feedback et l'absence de mutation du curseur.
30. Ramasser une pierre après son impact : vérifier qu'elle rejoint une pile compatible.
31. Lancer une pierre de poids `1.0` sur une PressurePlate dont le seuil vaut `1.0` : vérifier sa conversion en item monde et l'activation de la plaque.
32. Avec `Pierre x3` dans le curseur, déposer au sol puis dans un réceptacle : vérifier que chaque action consomme exactement une unité.
33. Survoler un bouton, panneau ou item situé à plusieurs cellules sans item dans le curseur : vérifier que le curseur reste `Default`.
34. Avec une pierre lançable dans le curseur, viser une surface lointaine dans la portée de lancer : vérifier l'état `AimThrow`.
35. Avec un item non lançable dans le curseur, viser une surface lointaine : vérifier que le curseur reste `Default`.

## 13. Limites actuelles

- Le ramassage monde alimente directement l'inventaire, pas le curseur.
- Le dépôt libre exige une cellule jouable et un point visible à portée de main, sans contrainte d’adjacence logique.
- Le système ne simule pas encore une physique réaliste de contact avec la plaque. Un item compte pour le poids s'il est enregistré comme item runtime sur la cellule de la PressurePlate.
- Le lancer ne gère pas encore les dégâts, les ennemis, la physique réaliste de rebond, les sons d'impact ou la charge de puissance.
- Les compétences, la précision, les dégâts et les effets sur les ennemis ne sont pas encore implémentés.
- Le nombre de cases d'inventaire n'est pas étendu automatiquement.
- Les actifs `.uasset` déterminent les variantes concrètes de supports et ne sont pas audités par ce document.

Les curseurs d'item et le retour court d'inventaire plein sont décrits dans
[`READABLE_OBJECTS_AND_FEEDBACK_FOUNDATION.md`](READABLE_OBJECTS_AND_FEEDBACK_FOUNDATION.md).

Les validations d'item sont présentées dans
[`LEVEL_VALIDATION_PANEL_FOUNDATION.md`](LEVEL_VALIDATION_PANEL_FOUNDATION.md).


## THROW01 — Lancer utilitaire MainHand avec visée (31.08.2026)

Le lancer physique d'exploration reste distinct d'une attaque de jet de combat.

Depuis l'équipement, le menu contextuel **Lancer** ouvre directement le mode de visée physique pour l'objet actuellement en MainHand. L'objet reste équipé tant que le joueur n'a pas confirmé une cible valide. Le curseur utilise l'état `AimThrow`; un clic gauche confirme, tandis que `Échap` ou le clic droit annule sans consommer ni déplacer l'objet.

Depuis la barre `1–9,0`, un objet lançable glissé depuis l’inventaire crée un binding stable `ThrowItem_<ItemDefinitionId>`. Ce raccourci réserve une unité sans la déplacer avant le clic : pose directe à portée de main, lancer normal au-delà. Les noms exposés de visée sont conservés pour stabilité de l’API Blueprint.

La cible de visée est obtenue par le premier impact du trace `Visibility`. Un mur opaque intercepte donc la visée au lieu de permettre de sélectionner directement un point situé derrière lui. Au déclenchement, le projectile est créé sur le rayon caméra → cible, donc visuellement au centre exact de `Cursor_Aim`. Le point de spawn reste volontairement en avant de la caméra et, pour une cible très proche, avant le premier impact afin de ne jamais naître derrière un mur ou une porte. La trajectoire reste ensuite une vraie trajectoire de `AGridThrownItemActor` : Force, poids, `ThrowSpeed`, `ThrowArc` et gravité déterminent le déplacement réel.

La collision du projectile reste l'autorité physique. `UProjectileMovementComponent::bSweepCollision` est explicitement activé et la sphère du projectile bloque `WorldStatic`, `WorldDynamic` et `PhysicsBody`. Un mur ou une porte bloquante doit donc provoquer l'impact et la conversion standard en World Item ; aucun mécanisme spécial « projectile traverse/active » n'est introduit.

HOTBAR01.2.1 supprime l'ancien binding synthétique de lancer MainHand. En phase prototype, aucune compatibilité de sauvegarde n'est maintenue pour ce binding obsolète. Les chemins autoritaires sont désormais uniquement :

- **MainHand → Lancer** pour lancer l'objet équipé ;
- **Inventaire → `ThrowItem_<ItemDefinitionId>` → hotbar** pour poser à portée de main ou lancer au-delà.
