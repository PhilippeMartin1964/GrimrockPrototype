# Wall Lock MVP — comportement runtime validé

Statut : **documentation MVP actuelle**  
Portée : **serrure murale, clé en inventaire, clé en curseur, connecteurs**  
Complète : `GRIMROCK_LOCK_SYSTEM.md`  
Ne remplace pas : `docs/Architecture/RECEPTACLE_SYSTEM_FOUNDATION.md` ni `docs/Architecture/ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md`

---

## 1. Objet du document

Ce document décrit le comportement réellement retenu pour le MVP des serrures murales après validation de la **Copper Key** et de la serrure murale **WallLockPit / Copper Wall Lock** dans `L_GrimrockEditor`.

Il clarifie en particulier une question de gameplay importante :

> Pourquoi une serrure murale peut-elle être ouverte avec une clé simplement présente dans l'inventaire, alors qu'elle peut aussi être ouverte en plaçant la clé dans le curseur et en l'insérant physiquement dans la serrure ?

Réponse courte :

> Ces deux gestes ne représentent pas exactement la même intention joueur.  
> Le premier est une **utilisation automatique depuis l'inventaire**.  
> Le second est une **insertion manuelle et diégétique de la clé dans la serrure**.

Les deux chemins doivent cependant converger vers le même résultat logique :

```text
WallLock.UnlockSuccess
  -> EGridObjectEvent::Activated
  -> Door.Open via connecteur
```

Une serrure murale ne doit jamais ouvrir une porte par `ItemInserted`.

---

## 2. Règle architecturale inchangée

La porte reste un objet passif commandable.

```text
Serrure murale
  -> émet un événement
  -> connecteur
  -> commande sur la porte
```

La porte ne connaît pas :

- la clé ;
- la serrure ;
- l'inventaire ;
- le curseur ;
- le crochetage ;
- les pièges.

Pour le MVP Copper Key :

```text
CopperWallLock.Activated -> Door.Open
```

Le connecteur correct est donc :

```text
SourceObject = CopperWallLock / WallLockPit
SourceEvent  = Activated
TargetObject = Door cible
Command      = Open
```

Le connecteur incorrect est :

```text
CopperWallLock.ItemInserted -> Door.Open
```

`ItemInserted` reste valable pour les vrais réceptacles : alcôves, supports, autels, bols d'offrande, etc. Il ne doit pas être le signal principal d'ouverture d'une serrure murale.

---

## 3. Les deux gestes joueur

## 3.1. Utilisation automatique depuis l'inventaire

### Description

Le joueur possède la clé dans l'inventaire d'un personnage actif et clique directement sur la serrure murale.

Le système cherche alors une clé compatible dans les inventaires des personnages actifs.

Flux :

```text
Clé dans inventaire
  -> clic sur WallLock
  -> recherche multi-personnages
  -> clé compatible trouvée
  -> UnlockSuccess Source=Inventory
  -> Event Activated
  -> Door.Open
```

### Pourquoi ce mode existe

Ce mode est un raccourci ergonomique.

Il évite d'obliger le joueur à :

1. ouvrir l'inventaire ;
2. prendre la clé ;
3. la mettre dans le curseur ;
4. fermer l'inventaire ;
5. cliquer sur la serrure.

Il correspond au comportement fréquent des RPG et dungeon crawlers modernes : si le groupe possède la clé, le jeu peut l'utiliser automatiquement.

### Effet sur la clé

Par défaut MVP :

```text
bConsumeKeyOnUnlock = false
```

Donc la clé reste dans l'inventaire.

C'est logique pour une clé réutilisable, par exemple :

- clé de secteur ;
- clé de prison ;
- clé de service ;
- clé pouvant ouvrir plusieurs serrures.

Si une serrure doit consommer la clé, cela doit être configuré explicitement :

```text
bConsumeKeyOnUnlock = true
```

Dans ce cas, la clé doit être retirée de l'inventaire au déverrouillage.

### Visuel de clé dans la serrure

Dans ce mode, la clé n'est pas forcément affichée dans la serrure.

Justification :

- l'action est abstraite ;
- la clé reste dans l'inventaire si elle n'est pas consommée ;
- le joueur n'a pas explicitement placé la clé dans le monde.

---

## 3.2. Insertion manuelle depuis le curseur

### Description

Le joueur prend la clé dans l'inventaire, la place dans le curseur, puis clique sur la serrure murale.

Flux :

```text
Clé dans curseur
  -> clic sur WallLock
  -> vérification de compatibilité
  -> UnlockSuccess Source=Cursor
  -> clé retirée du curseur
  -> clé attachée à ItemAttachPoint
  -> Event Activated
  -> Door.Open
```

### Pourquoi ce mode existe

Ce mode est plus diégétique et plus proche d'une interaction physique.

Il est intéressant pour :

- une serrure importante ;
- un mécanisme ancien ;
- une clé sacrificielle ;
- une clé qui reste dans la serrure ;
- une énigme visuelle où la clé insérée fait partie du décor ;
- une serrure où l'objet inséré doit rester visible.

Le joueur fait explicitement le geste :

```text
Je prends cette clé et je l'insère ici.
```

### Effet sur la clé

Dans le MVP consolidé :

- la clé quitte le curseur ;
- elle est attachée dans la serrure ;
- elle ne simule pas la physique ;
- elle n'a pas de collision de pickup ;
- elle n'est plus récupérable ;
- elle reste visible dans la serrure.

C'est le comportement voulu pour une clé insérée physiquement.

### Visuel de clé dans la serrure

La clé doit être attachée à :

```text
ItemAttachPoint
```

sur `BP_GridWallLockActor`.

Le réglage fin se fait dans le Blueprint :

- position légèrement enfoncée ;
- rotation alignée sur la fente ;
- échelle correcte ;
- pas de physique ;
- pas de collision de ramassage.

---

## 4. Pourquoi conserver les deux modes dans le MVP ?

Les deux modes répondent à deux besoins différents.

| Mode | Intention | Effet sur la clé | Usage recommandé |
|---|---|---|---|
| Clé dans inventaire | Ergonomie / auto-use | La clé reste si `bConsumeKeyOnUnlock=false` | Serrures ordinaires, clés réutilisables |
| Clé dans curseur | Insertion volontaire | La clé est insérée et reste dans la serrure | Serrures importantes, clés sacrificielles, mécanismes visuels |

Le comportement n'est donc pas forcément contradictoire.

Mais il doit être explicite dans le design, car il produit deux résultats visuels différents.

---

## 5. Règle MVP retenue

Pour le MVP actuel, une serrure murale peut accepter les deux chemins :

```text
Inventory key path
Cursor key path
```

mais les deux doivent obligatoirement produire le même événement logique :

```text
EGridObjectEvent::Activated
```

et jamais dépendre de :

```text
EGridObjectEvent::ItemInserted
```

Règle finale MVP :

```text
clé en inventaire OU clé en curseur
  -> WallLock UnlockSuccess
  -> Activated
  -> Door.Open
```

---

## 6. Limite connue du MVP

Le MVP ne possède pas encore de politique explicite de mode d'utilisation de clé.

Actuellement, le comportement est implicite :

```text
- si la clé est dans l'inventaire : auto-use ;
- si la clé est dans le curseur : insertion physique.
```

À terme, il serait préférable d'ajouter une politique configurable, par exemple :

```cpp
UENUM(BlueprintType)
enum class EGridWallLockKeyUsePolicy : uint8
{
    AutoUseFromInventoryOnly,
    InsertFromCursorOnly,
    AutoUseOrInsert,
};
```

Interprétation :

| Policy | Comportement |
|---|---|
| `AutoUseFromInventoryOnly` | La clé peut être utilisée depuis l'inventaire. Aucun visuel inséré. |
| `InsertFromCursorOnly` | Le joueur doit tenir la clé dans le curseur. La clé est insérée dans la serrure. |
| `AutoUseOrInsert` | Les deux gestes sont acceptés. |

Pour `WallLockPit`, si le design souhaite absolument que la clé reste visible dans la serrure, la meilleure politique future serait :

```text
InsertFromCursorOnly
```

ou une variante :

```text
AutoUseAndInsertVisual
```

mais cette dernière demande de décider si la clé doit être consommée depuis l'inventaire et visible dans la serrure, ce qui peut surprendre le joueur.

---

## 7. Messages joueur

Tous les textes visibles doivent être en français.

Messages recommandés pour la Copper Wall Lock :

```text
LockedMessage:
  La serrure est verrouillée.

UnlockSuccessMessage / UnlockedMessage:
  La serrure s'ouvre avec un déclic métallique.

AlreadyUnlockedMessage:
  La serrure est déjà déverrouillée.

MissingKeyMessage:
  Il vous manque la clé adéquate.
```

Important : le message de succès et le message déjà ouvert ne doivent pas être confondus.

Mauvais :

```text
UnlockSuccess -> La serrure est déjà déverrouillée.
```

Bon :

```text
UnlockSuccess   -> La serrure s'ouvre avec un déclic métallique.
AlreadyUnlocked -> La serrure est déjà déverrouillée.
```

---

## 8. Logs PIE attendus

### 8.1. Sans clé

```text
GridWallLock UnlockFailed MissingKey
AcceptedKeys=[Key_Copper]
Message=Il vous manque la clé adéquate.
```

Aucun événement `Activated` ne doit ouvrir la porte.

---

### 8.2. Clé dans inventaire

```text
GridWallLock InventoryScan ... ItemDefinitionIds=[Key_Copper]
GridWallLock UnlockSuccess ... Source=Inventory
GridWallLock ActivatedEventEmitted ... LinkExecuted=true
Grid link executed ... Command=Open Success=true
```

La porte doit s'ouvrir via :

```text
Event=Activated
```

---

### 8.3. Clé dans curseur

```text
GridInventory WorldDrop RoutedToWallLock Item=Key_Copper Target=...
GridWallLock CursorKeyAttempt ... CursorItemDefinitionId=Key_Copper
GridWallLock UnlockSuccess ... Source=Cursor
GridWallLock ActivatedEventEmitted ... LinkExecuted=true
Grid link executed ... Command=Open Success=true
```

La porte ne doit pas dépendre de :

```text
Event=ItemInserted
```

---

## 9. Configuration attendue des assets

### 9.1. `DA_Item_CopperKey`

```text
ItemDefinitionId = Key_Copper
DisplayName = Clé en cuivre
ItemType = Key
```

---

### 9.2. `DA_LockCopperWallLock` / `WallLockPit`

```text
RuntimeActorClass = BP_GridWallLockActor
SupportedType = Receptacle

Behavior.Lock.AcceptedKeyItems:
  - DA_Item_CopperKey

Behavior.Lock.AcceptedKeyIds:
  - Key_Copper

Behavior.Lock.bStartsUnlocked = false
Behavior.Lock.bConsumeKeyOnUnlock = false
```

Messages :

```text
LockedMessage = La serrure est verrouillée.
UnlockedMessage = La serrure s'ouvre avec un déclic métallique.
MissingKeyMessage = Il vous manque la clé adéquate.
```

Visuel de clé insérée :

```text
VisualPlacementMode = AttachedSocket
bSimulatePhysicsWhenPlaced = false
MaxContainedItems = 1
bCanRemoveItem = false
```

---

### 9.3. Connecteur

Le connecteur correct est :

```text
CopperWallLock.Activated -> Door.Open
```

Le connecteur incorrect est :

```text
CopperWallLock.ItemInserted -> Door.Open
```

---

## 10. Décision de design

La règle retenue pour le MVP est :

> Une serrure murale peut être déverrouillée automatiquement si le groupe possède la clé, ou manuellement si le joueur insère la clé depuis le curseur.  
> Dans les deux cas, l'ouverture de la porte passe uniquement par `Activated`.  
> `ItemInserted` reste réservé aux vrais réceptacles.

Cette décision permet de garder :

- l'ergonomie du clic direct ;
- la satisfaction visuelle de l'insertion manuelle ;
- l'architecture propre `SourceEvent -> TargetCommand` ;
- la séparation entre serrure murale et porte.

Décision future à prendre :

> Certaines serrures devront-elles imposer `InsertFromCursorOnly` pour renforcer le côté énigme / mécanisme physique ?

Pour les serrures importantes ou sacrificielles, la réponse recommandée est oui.
