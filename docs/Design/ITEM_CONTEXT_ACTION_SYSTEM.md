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
S'équiper en main principale
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
  -> S'équiper en main principale
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

## 13. Etat d'implementation du Patch 1

Le premier patch fournit la fondation C++ de consultation des actions contextuelles, sans encore executer les mutations d'inventaire.

### Types et resolution

- `GridItemActionTypes.h` definit `EGridItemActionType`, `EGridFacingTargetType`, `FGridItemContextAction`, `FGridItemActionContext` et `FGridFacingTargetContext`.
- `UGridItemContextActionLibrary::BuildInventorySlotContextActions` construit les actions disponibles pour un slot.
- `UGridItemContextActionLibrary::BuildItemContextActions` centralise les regles de disponibilite.
- `UGridItemContextActionLibrary::ResolveFacingTarget` effectue une trace `ECC_Visibility` depuis la camera dans la direction de grille du groupe et classe le premier acteur pertinent comme serrure murale, receptacle, support de torche, objet lisible, porte ou mecanisme.

Les actions actuellement exposees sont notamment `Utiliser`, `Equiper`, `Desequiper`, `Placer`, `Inserer`, `Lire`, `Boire`, `Manger`, `Lancer`, `Donner` et `Detruire`. Leur disponibilite depend de la definition d'item, de son emplacement et de la cible faisant face au groupe.

### Integration UMG

`UGridInventorySlotWidget` transmet la demande de menu contextuel au widget d'inventaire. `UGridInventoryWidget` expose les proprietes et fonctions Blueprint necessaires pour construire, afficher et fermer ce menu, ainsi qu'un delegate pour la selection d'une action.

L'asset UMG doit :

1. lier le clic droit du slot a la demande de menu contextuel ;
2. creer les entrees a partir des actions retournees par la bibliotheque ;
3. afficher le libelle et l'etat active/desactive ;
4. fermer le menu apres selection ou clic exterieur.

Ce patch ne modifie aucun asset `.uasset` : ce cablage reste donc une operation manuelle dans l'editeur Unreal.

### Compatibilite et limites

- Le drag/drop et le Cursor existants restent inchanges.
- Les actions sont calculees mais ne sont pas encore executees.
- Les mutations devront etre raccordees au service central prevu par cette specification.
- Le scan automatique de l'inventaire par la serrure murale reste un comportement transitoire jusqu'au patch d'execution des actions.
- Une plaque de pression n'est pas consideree comme une cible d'action directe.

Les logs `GridItemContextActions Build`, `GridItemContextActions FacingTarget` et `GridInventory ContextMenuRequested` permettent de diagnostiquer cette fondation.
