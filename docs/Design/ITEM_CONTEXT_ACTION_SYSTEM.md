# Item Context Action System — inventaire, clic droit, drag/drop et cibles

Statut : **spécification de design cible**  
Portée : **UX inventaire, actions contextuelles, drag/drop, cibles en face du groupe, remplacement progressif du Cursor comme modèle public**  
Complète : `ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md`, `WALL_LOCK_MVP_RUNTIME_BEHAVIOR.md`, `GRIMROCK_LOCK_SYSTEM.md`

---

## 1. Intention

Le prototype possède déjà une logique de transport d'item par **Cursor**. Ce mécanisme a permis de valider le ramassage, le dépôt, les réceptacles et la serrure murale MVP.

Cependant, comme modèle d'interface principal, le Cursor devient trop lourd à long terme. Le jeu doit évoluer vers un système plus naturel :

```text
clic droit sur item
  -> menu d'actions contextuelles
  -> action explicite
  -> cible éventuelle
  -> résultat
```

Le drag/drop reste valable. Il ne remplace pas le menu contextuel ; il constitue un raccourci direct pour les actions évidentes.

Règle cible :

> Le joueur ne doit pas devoir comprendre un état abstrait de Cursor pour jouer.  
> Il doit manipuler des items par actions explicites : équiper, déposer, insérer, lire, consommer, scinder, combiner, utiliser sur une cible, etc.

---

## 2. Décision de design

Le Cursor ne doit plus être le modèle UX principal.

Il peut rester temporairement comme détail technique interne pendant la migration, mais le modèle visible doit devenir :

```text
Item sélectionné
  + Action contextuelle
  + Cible facultative
  = Résultat explicite
```

Exemples :

```text
clic droit sur clé      -> Insérer dans la serrure en face
clic droit sur torche   -> S'équiper / Placer sur le support en face / Déposer
clic droit sur potion   -> Consommer / Scinder / Déposer
clic droit sur livre    -> Lire / Déposer
clic droit sur pierre   -> Lancer / Déposer / Placer sur la plaque en face
clic droit sur armure   -> S'équiper / Déposer
```

---

## 3. Le tooltip correspond à Examiner

Dans ce projet, l'action **Examiner** ne doit pas nécessairement ouvrir une nouvelle fenêtre.

Décision :

> Examiner correspond au tooltip existant.

Donc :

```text
survol item
  -> tooltip
  -> nom, type, icône, description, poids, lumière, quantité, tags debug si nécessaire
```

Le menu contextuel peut éventuellement proposer `Examiner`, mais cette action doit réutiliser le même contenu que le tooltip, pas créer un second système d'information divergent.

---

## 4. Drag/drop

Le drag/drop reste valable.

Il sert aux actions directes et évidentes :

```text
drag item -> slot équipement
  = S'équiper

drag item -> sol / monde
  = Déposer

drag item -> réceptacle compatible
  = Insérer / Placer

drag item -> autre pile compatible
  = Fusionner pile

drag item -> zone de scission
  = Scinder, si stackable
```

Le drag/drop doit être vu comme un raccourci d'action, pas comme une logique séparée.

Règle :

```text
drag/drop et menu contextuel doivent appeler le même service d'action item.
```

Ils doivent produire les mêmes validations, les mêmes messages et les mêmes mutations d'état.

---

## 5. Clic droit : menu d'actions contextuelles

Le clic droit sur un slot d'inventaire doit construire dynamiquement les actions disponibles.

Sources de décision :

```text
- ItemType
- ItemTags
- état runtime de l'item
- quantité / stack
- source de l'item : inventaire, équipement, réceptacle, monde
- cible actuellement en face du groupe
- état de la cible
- règles de gameplay
```

Actions possibles :

```text
S'équiper
Déséquiper
Consommer
Lire
Utiliser
Utiliser sur la cible
Insérer dans la cible
Placer sur la cible
Déposer au sol
Lancer
Combiner
Scinder la pile
Allumer / Éteindre, si applicable plus tard
Examiner, si l'on veut forcer l'affichage du tooltip
```

Les actions impossibles peuvent être :

- masquées ;
- ou affichées grisées avec une raison.

Pour le prototype, la recommandation est :

```text
Masquer les actions non pertinentes.
Afficher en grisé seulement les actions utiles pédagogiquement.
```

Exemple : clé devant une serrure incompatible :

```text
Insérer dans la serrure    grisé : Cette clé ne convient pas.
Déposer au sol             actif
```

---

## 6. Cible en face du groupe

Le runtime doit pouvoir identifier la cible principale en face du groupe.

Exemples :

```text
WallLock
TorchHolder
Alcove
OfferingBowl
ReadablePanel
Door
PressurePlate
EmptyCell
NoTarget
```

Cette cible influence le menu contextuel.

Exemple avec une clé :

```text
si cible = WallLock compatible
  -> Insérer dans la serrure

si cible = WallLock incompatible
  -> Insérer dans la serrure, grisé ou masqué

si cible = None
  -> Déposer au sol
```

Exemple avec une torche :

```text
si cible = TorchHolder vide
  -> Placer sur le support

si cible = TorchHolder occupé
  -> action grisée : Le support est déjà occupé.

si cible = None
  -> S'équiper
  -> Déposer au sol
```

---

## 7. Clic sur cible monde -> inventaire assisté

Certaines cibles doivent pouvoir ouvrir l'inventaire en mode assisté.

### 7.1. Serrure murale

Flux recommandé :

```text
clic sur serrure verrouillée
  -> chercher les clés compatibles dans l'inventaire
```

Cas 1 : aucune clé compatible

```text
Message : Il vous manque la clé adéquate.
```

Cas 2 : une ou plusieurs clés compatibles

```text
ouvrir l'inventaire
surligner les clés compatibles
mettre l'inventaire en TargetContext = WallLock
proposer l'action prioritaire : Insérer dans la serrure
```

Le joueur doit encore confirmer l'action :

```text
clic droit clé -> Insérer dans la serrure
```

ou équivalent UI : bouton `Insérer`.

Règle :

> La serrure ne doit pas consommer automatiquement une clé depuis l'inventaire sans action explicite du joueur.

---

## 8. Familles d'items et actions recommandées

## 8.1. Clés

Conditions :

```text
ItemType = Key
Tags possibles : Key, Key.Copper, Key.Prison, Key.Master
```

Actions typiques :

```text
Insérer dans la serrure en face
Déposer au sol
Examiner / tooltip
```

Si la serrure en face est compatible :

```text
Insérer dans la serrure
```

Si elle ne l'est pas :

```text
Cette clé ne convient pas.
```

Si aucune serrure n'est ciblée :

```text
Déposer au sol
Examiner
```

---

## 8.2. Torches

Règle validée :

> La torche est automatiquement allumée lorsqu'elle est équipée ou déposée sur un support de torche.

Actions typiques :

```text
S'équiper en main directrice
S'équiper en main secondaire
Placer sur le support de torche en face
Déposer au sol
Examiner / tooltip
```

Comportement :

```text
Torche équipée
  -> lumière activée automatiquement

Torche placée sur support
  -> lumière activée automatiquement

Torche déposée au sol
  -> à définir séparément : garder allumée ou non selon design futur
```

La torche ne doit pas nécessiter une action manuelle `Allumer` dans le MVP pour les cas équipement/support.

---

## 8.3. Équipement : armes, armures, boucliers, bijoux

Actions typiques :

```text
S'équiper
Déséquiper, si déjà équipé
Déposer au sol
Lancer, uniquement si l'objet est lançable
Examiner / tooltip
```

Règle de lancer :

> Lancer n'est proposé que pour les objets équipables et/ou objets explicitement lançables.

Le critère technique peut être :

```text
bThrowable = true
```

ou :

```text
ItemTags contient Throwable
```

Exemple : pierre lançable :

```text
ItemType = Misc ou Component
bThrowable = true
Tags = Throwable, WeightObject
Action proposée = Lancer
```

Exemple : épée non lançable :

```text
ItemType = Weapon
bThrowable = false
Action Lancer masquée
```

---

## 8.4. Pierres et objets de poids

Actions typiques :

```text
Lancer
Déposer au sol
Placer sur plaque de pression en face, si applicable
Déposer dans réceptacle compatible
Examiner / tooltip
```

Une pierre est un bon exemple d'objet à la fois :

- ramassable ;
- lançable ;
- utilisable comme poids ;
- déposable au sol ;
- potentiellement insérable dans certains réceptacles.

---

## 8.5. Potions et consommables

Actions typiques :

```text
Consommer
Scinder la pile, si quantité > 1
Déposer au sol
Combiner, si recette applicable
Examiner / tooltip
```

Consommer doit être explicite.

```text
clic droit potion -> Consommer
```

Pas de consommation automatique sans action.

---

## 8.6. Livres, parchemins, inscriptions portables

Actions typiques :

```text
Lire
Déposer au sol
Examiner / tooltip
```

Lire ouvre un contenu textuel, différent du tooltip.

Différence :

```text
Examiner / tooltip
  -> description courte de l'objet

Lire
  -> contenu narratif complet du livre, parchemin ou document
```

---

## 8.7. Composants et objets combinables

Actions typiques :

```text
Combiner
Scinder la pile
Déposer au sol
Examiner / tooltip
```

Combiner doit ouvrir un mode ou une fenêtre de combinaison, ou permettre :

```text
Utiliser avec...
```

Exemples futurs :

```text
pierre + corde
clé cassée + fragment de clé
fiole vide + liquide
herbe + mortier
```

---

## 9. Structures C++ proposées

## 9.1. Types d'action

```cpp
UENUM(BlueprintType)
enum class EGridItemActionType : uint8
{
    None,

    Equip,
    Unequip,

    Consume,
    Read,
    Examine,

    Use,
    UseOnTarget,
    InsertIntoTarget,
    PlaceOnTarget,

    DropToGround,
    Throw,

    Combine,
    SplitStack,

    ToggleLight
};
```

`ToggleLight` est prévu pour plus tard. Pour la torche MVP, l'allumage est automatique à l'équipement ou au placement sur support.

---

## 9.2. Requête d'action

```cpp
USTRUCT(BlueprintType)
struct FGridItemActionRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    EGridItemActionType ActionType = EGridItemActionType::None;

    UPROPERTY(BlueprintReadWrite)
    FGridItemInstance Item;

    UPROPERTY(BlueprintReadWrite)
    int32 SourceCharacterIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 SourceInventorySlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    EGridEquipmentSlot SourceEquipmentSlot = EGridEquipmentSlot::None;

    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadWrite)
    FGuid TargetObjectId;

    UPROPERTY(BlueprintReadWrite)
    int32 TargetCellX = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 TargetCellY = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    EGridEdge TargetEdge = EGridEdge::None;

    UPROPERTY(BlueprintReadWrite)
    FHitResult HitResult;
};
```

---

## 9.3. Résultat d'action

```cpp
USTRUCT(BlueprintType)
struct FGridItemActionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly)
    FText Message;

    UPROPERTY(BlueprintReadOnly)
    bool bItemMoved = false;

    UPROPERTY(BlueprintReadOnly)
    bool bItemConsumed = false;

    UPROPERTY(BlueprintReadOnly)
    bool bTargetActivated = false;
};
```

---

## 9.4. Service central

Créer ou étendre un service :

```cpp
UGridItemActionService
```

Responsabilités :

```text
BuildAvailableActions(Item, Context)
CanExecuteAction(Request, OutReason)
ExecuteAction(Request, OutResult)
```

Ce service doit être appelé par :

```text
- menu contextuel clic droit ;
- drag/drop ;
- raccourcis clavier éventuels ;
- interaction monde ouvrant l'inventaire assisté.
```

Règle :

> Un item ne doit pas être déplacé, consommé, équipé ou inséré par plusieurs chemins de code divergents.

---

## 9.5. Interface de cible utilisable

Les objets du monde compatibles avec les items doivent exposer une interface commune :

```cpp
UINTERFACE(BlueprintType)
class UGridItemUseTargetInterface : public UInterface
{
    GENERATED_BODY()
};
```

Méthodes proposées :

```cpp
CanUseItemOnTarget(ItemInstance, OutReason)
UseItemOnTarget(ItemInstance, OutResult)
GetCompatibleItemDefinitionIds()
GetCompatibleItemTags()
GetPreferredItemAction()
```

Exemples :

```text
WallLockActor
  PreferredAction = InsertIntoTarget
  CompatibleItems = clés acceptées

TorchHolder
  PreferredAction = PlaceOnTarget
  CompatibleItems = torches

Alcove
  PreferredAction = InsertIntoTarget / PlaceOnTarget
  CompatibleItems = tous ou liste filtrée

EquipmentSlot
  PreferredAction = Equip
  CompatibleItems = selon slot
```

---

## 10. UX cible par cas

## 10.1. Serrure murale

```text
clic sur serrure
  -> si aucune clé compatible : message
  -> si clés compatibles : ouvrir inventaire, surligner clés

clic droit clé compatible
  -> Insérer dans la serrure
  -> clé retirée de l'inventaire
  -> clé attachée à la serrure
  -> WallLock.Activated
  -> Door.Open
```

---

## 10.2. Support de torche

```text
clic sur support vide
  -> si torche disponible : ouvrir inventaire, surligner torches

clic droit torche
  -> Placer sur le support
  -> torche retirée de l'inventaire
  -> torche attachée au support
  -> lumière activée automatiquement
```

---

## 10.3. Équipement

```text
clic droit arme
  -> S'équiper en main directrice
  -> S'équiper en main secondaire, si autorisé

clic droit armure
  -> S'équiper

clic droit objet équipé
  -> Déséquiper
```

Le drag/drop vers un slot d'équipement reste autorisé.

---

## 10.4. Dépôt au sol

```text
clic droit item
  -> Déposer au sol
```

ou :

```text
drag item vers monde
  -> Déposer au sol
```

Les deux chemins doivent appeler la même action `DropToGround`.

---

## 11. Roadmap de migration

### Étape 1 — Documentation et décision

- Documenter que le Cursor est transitoire comme UX principale.
- Conserver drag/drop.
- Définir le menu clic droit comme cible.
- Définir le clic sur cible monde -> inventaire assisté.

### Étape 2 — Menu contextuel d'inventaire

Créer :

```text
WBP_ItemActionMenu
```

Déclenché par :

```text
clic droit sur slot inventaire
```

Actions minimales :

```text
S'équiper
Déposer au sol
Lire
Consommer
Insérer / Placer sur cible
Scinder la pile
```

### Étape 3 — Action service

Créer :

```text
UGridItemActionService
```

ou étendre proprement `UGridItemTransferService` si l'on veut éviter une nouvelle classe.

### Étape 4 — Serrure assistée

Changer le flux WallLock cible :

```text
clic serrure
  -> inventaire assisté
  -> clés compatibles surlignées
  -> action Insérer
```

Ne plus faire d'auto-unlock silencieux depuis l'inventaire.

### Étape 5 — Support de torche assisté

Même logique :

```text
clic support vide
  -> inventaire assisté
  -> torches surlignées
  -> action Placer sur le support
```

### Étape 6 — Déprécier le Cursor public

Le Cursor peut rester utilisé en interne pendant la transition, mais il ne doit plus être la seule manière de manipuler les items.

### Étape 7 — Nettoyage

Lorsque le système d'actions est stable :

- supprimer les chemins redondants ;
- garder drag/drop comme raccourci ;
- unifier tous les messages ;
- centraliser les mutations de propriété d'item.

---

## 12. Règle finale

Règle à retenir :

> Le joueur manipule les objets par actions explicites, pas par magie d'inventaire.  
> Le clic droit propose les actions.  
> Le drag/drop reste un raccourci.  
> Le tooltip est l'examen.  
> Les cibles du monde peuvent ouvrir l'inventaire en mode assisté.  
> La torche s'allume automatiquement lorsqu'elle est équipée ou placée sur un support de torche.  
> Lancer n'est proposé que pour les objets réellement lançables.  
> Les mutations d'item doivent passer par un service central.

---

## 13. État d'implémentation du Patch 1

Le premier patch fournit la fondation C++ de consultation des actions contextuelles, sans encore exécuter les mutations d'inventaire.

### Types et résolution

- `GridItemActionTypes.h` définit `EGridItemActionType`, `EGridFacingTargetType`, `FGridItemContextAction`, `FGridItemActionContext` et `FGridFacingTargetContext`.
- `UGridItemContextActionLibrary::BuildInventorySlotContextActions` construit les actions disponibles pour un slot.
- `UGridItemContextActionLibrary::BuildItemContextActions` centralise les règles de disponibilité.
- `UGridItemContextActionLibrary::ResolveFacingTarget` effectue une trace `ECC_Visibility` depuis la caméra dans la direction de grille du groupe et classe le premier acteur pertinent comme serrure murale, réceptacle, support de torche, objet lisible, porte ou mécanisme.

Les actions actuellement exposées sont notamment `Utiliser`, `Équiper`, `Déséquiper`, `Placer`, `Insérer`, `Lire`, `Boire`, `Manger`, `Donner` et `Détruire`. Leur disponibilité dépend de la définition d'item, de son emplacement et de la cible faisant face au groupe. `Lancer` n'est pas proposé dans le menu contextuel d'inventaire pour le moment.

### Intégration UMG

`UGridInventorySlotWidget` transmet la demande de menu contextuel au widget d'inventaire. `UGridInventoryWidget` expose les propriétés et fonctions Blueprint nécessaires pour construire, afficher et fermer ce menu, ainsi qu'un delegate pour la sélection d'une action.

L'asset UMG doit :

1. lier le clic droit du slot à la demande de menu contextuel ;
2. créer les entrées à partir des actions retournées par la bibliothèque ;
3. afficher le libellé et l'état activé/désactivé ;
4. fermer le menu après sélection ou clic extérieur.

Ce patch ne modifie aucun asset `.uasset` : ce câblage reste donc une opération manuelle dans l'éditeur Unreal.

### Compatibilité et limites

- Le drag/drop et le Cursor existants restent inchangés.
- Les actions sont calculées mais ne sont pas encore exécutées.
- Les mutations devront être raccordées au service central prévu par cette spécification.
- Le scan automatique de l'inventaire par la serrure murale reste un comportement transitoire jusqu'au patch d'exécution des actions.
- Une plaque de pression n'est pas considérée comme une cible d'action directe.

Les logs `GridItemContextActions Build`, `GridItemContextActions FacingTarget` et `GridInventory ContextMenuRequested` permettent de diagnostiquer cette fondation.

---

## 14. Exécution minimale et câblage UMG

Le Patch 2 exécute les actions `Examine`, `Read`, `Equip`, `Unequip`, `InsertIntoTarget`, `PlaceOnTarget` et `DropToGround`.

- `Examine` réutilise le texte de tooltip du `UGridInventorySlotWidget` source et appelle l'événement Blueprint `PresentItemExamination`.
- `Read` affiche le contenu textuel long d'un item lisible via l'événement Blueprint `PresentItemReading`.
- `Equip` utilise l'action exacte sélectionnée : plusieurs entrées peuvent partager `ActionType=Equip`, et `EquipmentSlot` distingue la main directrice de la main secondaire.
- `Unequip` affiche le libellé court `Enlever`, conserve `EquipmentSlot`, puis retire l'item de la main concernée et le replace dans l'inventaire du personnage sélectionné si un slot est disponible.
- `InsertIntoTarget` transfère une clé du slot d'inventaire vers `AGridWallLockActor`, attache son visuel et émet uniquement `Activated`.
- `PlaceOnTarget` utilise `UGridItemTransferService` pour transférer un item depuis l'inventaire, `MainHand` ou `OffHand` vers un réceptacle compatible. Les alcôves sont traitées comme des réceptacles ; les supports de torche activent en plus la lumière de la torche placée.
- `DropToGround` dépose l'item sur la cellule actuelle du groupe, puis retire l'item de l'inventaire ou de l'équipement uniquement si le spawn au sol réussit.

Les autres actions restent calculées mais leur exécution produit le log `GridItemActions Execute NotImplemented`.
`Throw` n'est pas généré dans ce menu : le lancer passera par l'équipement en main directrice et le futur système d'arme ou projectile.

Le menu contextuel propose les mains compatibles selon `CompatibleEquipmentSlots` dans la définition d'item. L'action sélectionnée détermine exactement la main cible. Une torche compatible `MainHand` et `OffHand` peut proposer les deux mains ; si elle ne déclare que `MainHand`, seule l'action `S'équiper en main directrice` apparaît. Le système ne doit jamais rediriger une action `OffHand` vers `MainHand`.

La lumière équipée est recalculée globalement depuis l'équipement courant du personnage sélectionné. `MainHand` et `OffHand` contribuent toutes les deux : le résultat est actif si au moins une main contient un item lumineux. Équiper ou déposer un item non lumineux dans une main ne doit jamais éteindre une source lumineuse encore équipée dans l'autre main.

Le drag/drop entre deux slots occupés effectue un échange atomique sans utiliser le Cursor. Avant l'échange, chaque item est validé contre son slot de destination : un item déplacé vers `MainHand` ou `OffHand` doit être compatible avec cette main, tandis qu'un retour vers un slot d'inventaire valide est toujours accepté. Si une compatibilité échoue, aucun item n'est déplacé.

`PlaceOnTarget` ne doit pas passer par le Cursor : un transfert depuis une main équipée retire l'item de `MainHand` ou `OffHand` seulement après acceptation par la cible. Le transfert échoue sans mutation si le réceptacle rejette l'item.

### WBP_ItemActionMenu

Le menu UMG doit conserver `SlotType` et `SlotIndex` reçus avec `OnContextActionsRequested`. Chaque bouton doit aussi conserver son `ActionIndex`, l'entrée `FGridItemContextAction` correspondante et son `OwnerMenu`, car plusieurs boutons peuvent partager le même `ActionType`.

Le menu doit :

1. créer un bouton pour chaque entrée de `LastContextActions` ;
2. transmettre l'`Array Index` de `LastContextActions` au `WBP_ItemActionButton` comme `ActionIndex` ;
3. utiliser `Label` comme texte du bouton ;
4. appliquer `bEnabled` à l'état interactif du bouton ;
5. au clic, appeler `OwnerMenu.ExecuteActionByIndex(ActionIndex)` ;
6. dans `ExecuteActionByIndex`, appeler `OwnerInventoryWidget.ExecuteInventoryContextActionByIndex(SourceSlotType, SourceSlotIndex, ActionIndex)` ;
7. ne plus utiliser `ExecuteInventoryContextAction(ActionType, SourceSlotType, SourceSlotIndex)` depuis le menu contextuel visible ;
8. fermer le menu après un résultat positif.

`ExecuteInventoryContextAction(ActionType, SlotType, SlotIndex)` reste disponible pour compatibilité, mais elle refuse les actions ambiguës comme `Equip` lorsque plusieurs entrées partagent le même `ActionType`. Le menu doit utiliser l'exécution par index pour garantir que le bouton cliqué exécute exactement l'action affichée, notamment `EquipmentSlot=MainHand` ou `EquipmentSlot=OffHand`.

Pour fermer le menu au clic extérieur, `WBP_ItemActionMenu` doit contenir un fond transparent plein écran derrière le panneau, par exemple `Button_CloseArea` ou `Border_ClickCatcher`. Son clic appelle `OwnerInventoryWidget.CloseItemActionMenu("ClickOutside")`, puis le Blueprint gère `OnItemActionMenuCloseRequested` en retirant uniquement `WBP_ItemActionMenu`. `RemoveFromParent` ne doit jamais viser `WBP_GridInventory`, afin de préserver `Page_Inventory` et `TopTabs`.

`WBP_ItemActionMenu` reste plein écran pour capter le clic extérieur. Seul le panneau interne `Border_MenuPanel` doit être repositionné à la souris via son `CanvasSlot`; `SetPositionInViewport` ne doit pas être appelé sur le widget plein écran du menu.

Dans le Blueprint dérivé de `UGridInventoryWidget`, implémenter `PresentItemExamination(Item, ExaminationText)` en affichant `ExaminationText` dans le panneau d'information existant ou dans le même widget visuel que le tooltip. Aucun second contenu descriptif ne doit être reconstruit dans le menu.

### Read / Lire

`Examiner` et `Lire` ont deux rôles distincts :

- `Examiner` affiche la description courte ou le tooltip de l'item.
- `Lire` affiche le contenu textuel long de l'item.

L'action `Lire` est générée uniquement pour les items lisibles : `ItemType=Book`, `ItemType=Scroll`, tag `Readable` ou `Lisible`, ou définition d'item avec `ReadText` renseigné. Elle ne doit pas apparaître pour les clés, torches, pierres, armes, nourritures ou potions non lisibles.

Dans `UGridItemDefinitionAsset`, `Description` reste le texte court de tooltip/examen, tandis que `ReadText` contient le texte long affiché par `Lire`. Si `ReadText` est vide, l'exécution peut afficher `Description`, puis le message français `Rien de particulier n'est écrit.` en dernier recours.

Dans le Blueprint dérivé de `UGridInventoryWidget`, implémenter `PresentItemReading(Item, Title, ReadText)` pour afficher un panneau de lecture persistant. Le widget conseillé est `WBP_ItemReadPanel` avec une structure simple :

1. `Border` ou fond sombre ;
2. `Text_Title` pour le nom de l'item ;
3. `ScrollBox` ou `RichTextBlock` pour le contenu ;
4. indication discrète : `Cliquez ou déplacez-vous pour fermer`.

Le panneau de lecture reste affiché jusqu'à une action volontaire : clic souris, déplacement, rotation, fermeture ou `Escape` si disponible. Ce patch ne fusionne pas les items lisibles d'inventaire avec `DA_WallInscription` ni avec le système de panneaux muraux ; une convergence future peut seulement réutiliser un widget d'affichage commun.

Ce patch ne modifie aucun asset `.uasset` ; le câblage de `WBP_ItemActionMenu` et de `WBP_ItemReadPanel` reste manuel dans l'éditeur Unreal.
