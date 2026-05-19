# GrimrockPrototype — Archétypes d’objets

## Objectif

Ce document définit les archétypes concrets à créer ou à vérifier dans le projet.

Un archétype représente un objet visible et sélectionnable dans l’éditeur, même s’il partage sa classe C++ avec d’autres objets.

---

## Principe

Un archétype doit préciser :

```text
ArchetypeId
DisplayName
Category
ActorClass
PlacementType
InitialState
EmittedEvents
AcceptedCommands
Meshes / Materials / Preview
Behavior parameters
```

L’archétype permet de séparer :

- le comportement C++ ;
- l’identité de l’objet ;
- son apparence ;
- ses paramètres de gameplay.

---

## Champs recommandés pour `UGridObjectArchetypeAsset`

Champs à viser progressivement :

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid Object")
FName ArchetypeId;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid Object")
FText DisplayName;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid Object")
EGridObjectCategory Category;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Runtime")
TSubclassOf<AActor> ActorClass;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="State")
EGridObjectInitialState InitialState;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Events")
TArray<EGridObjectEvent> EmittedEvents;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Commands")
TArray<EGridObjectCommand> AcceptedCommands;
```

Selon l’existant, ces noms doivent être adaptés sans casser la compilation.

---

## Archétypes de mécanismes

| ArchetypeId recommandé | Nom éditeur | Catégorie | Classe runtime | Remarque |
|---|---|---|---|---|
| `Button_Normal` | Bouton | `Mechanism` | `AGridButtonActor` | bouton standard |
| `Button_Secret` | Bouton secret | `Mechanism` | `AGridButtonActor` | mesh discret, objet distinct |
| `Button_Wall` | Bouton mural | `Mechanism` | `AGridButtonActor` | variante murale visible |
| `Lever_Standard` | Levier | `Mechanism` | `AGridLeverActor` | deux états |
| `PressurePlate_Stone` | Plaque de pression | `Mechanism` | `AGridPressurePlateActor` | présence joueur ou item |
| `Trigger_Floor` | Trigger de sol | `Trigger` | `AGridTriggerActor` | invisible ou discret |
| `Rune_Magic` | Rune magique | `Mechanism` / `Light` / `Decoration` | à déterminer | dépend de l’usage |
| `Timer_Default` | Timer | `Mechanism` | `AGridTimerActor` | logique sans mesh obligatoire |

---

## Archétypes de réceptacles

| ArchetypeId recommandé | Nom éditeur | Catégorie | Classe runtime | Remarque |
|---|---|---|---|---|
| `Receptacle_Generic` | Réceptacle | `Receptacle` | `AGridReceptacleActor` | base générique |
| `Receptacle_Alcove` | Alcove | `Receptacle` | `AGridReceptacleActor` | niche murale |
| `Receptacle_TorchHolder` | Support de torche | `Receptacle` | `AGridReceptacleActor` | accepte une torche |
| `Receptacle_Altar` | Autel | `Receptacle` | `AGridReceptacleActor` | objet posé / offrande |
| `Receptacle_OfferingBowl` | Bol d’offrande | `Receptacle` | `AGridReceptacleActor` | peut consommer l’objet |
| `Receptacle_CoinSlot` | Fente à pièce | `Receptacle` | `AGridReceptacleActor` | accepte `Coin` |
| `Lock_Keyhole` | Serrure | `Receptacle` | `AGridReceptacleActor` ou `AGridLockActor` | accepte clé |

Note Patch E : les réceptacles concrets sont des archétypes. `Receptacle_Alcove`, `Receptacle_TorchHolder`, `Receptacle_Altar` et `Receptacle_OfferingBowl` restent tous `SupportedType = Receptacle` et utilisent une `RuntimeActorClass` dérivée de `AGridReceptacleActor`. Les comportements spécifiques seront ajoutés plus tard via `Behavior` et les commandes, pas par multiplication de `EGridLevelObjectType`.

Note Patch F : les réceptacles utilisent des règles d’acceptation configurables dans `Behavior.Receptacle`. Par défaut `bAcceptAnyItem=true` conserve le comportement existant. Les restrictions se font par `AcceptedArchetypeIds`, `AcceptedItemTags` et `RejectedItemArchetypeIds`, toujours sur les `ArchetypeId`/tags d’items, sans créer de nouveaux `EGridLevelObjectType`.

---

## Archétypes de passages

| ArchetypeId recommandé | Nom éditeur | Catégorie | Classe runtime | Remarque |
|---|---|---|---|---|
| `Door_Stone` | Porte | `Passage` | `AGridDoorActor` | porte standard |
| `Door_Secret` | Porte secrète | `Passage` | `AGridSecretDoorActor` | partie fixe + partie mobile |
| `Trapdoor_Stone` | Trappe | `Passage` | `AGridTrapdoorActor` | à créer plus tard |
| `Teleporter_Rune` | Téléporteur | `Passage` | `AGridTeleporterActor` | à créer plus tard |

---

## Archétypes d’items

| ArchetypeId recommandé | Nom éditeur | Catégorie | Classe runtime | Remarque |
|---|---|---|---|---|
| `Item_Key` | Clé | `Item` | `AGridItemActor` | clé générique |
| `Item_Coin` | Pièce | `Item` | `AGridItemActor` | fente / offrande |
| `Item_Torch` | Torche | `Item` ou `Light` | `AGridItemActor` | peut être placée dans un support |

---

## Archétypes Readable / Spawn

| ArchetypeId recommandé | Nom éditeur | Catégorie | Classe runtime | Remarque |
|---|---|---|---|---|
| `Readable_WallInscription` | WallInscription | `Readable` | système existant | ne pas renommer |
| `Spawn_Player` | Spawn joueur | `Spawn` | marker ou donnée | position initiale |
| `Spawn_Monster` | Spawn monstre | `Spawn` | futur système | plus tard |
| `Spawn_Item` | Spawn item | `Spawn` | futur système | plus tard |

---

## Paramètres spécifiques des réceptacles

Les réceptacles concrets doivent idéalement partager les mêmes paramètres :

```cpp
FName RequiredItemId;
FName RequiredItemTag;
bool bConsumeInsertedItem;
bool bAllowItemRemoval;
bool bDisplayInsertedItem;
bool bEmitOnInsert;
bool bEmitOnRemove;
```

Exemples :

### Support de torche

```text
RequiredItemTag = Torch
bConsumeInsertedItem = false
bAllowItemRemoval = true
bDisplayInsertedItem = true
```

### Fente à pièce

```text
RequiredItemTag = Coin
bConsumeInsertedItem = true
bAllowItemRemoval = false
bDisplayInsertedItem = false
```

### Bol d’offrande

```text
RequiredItemTag = Offering
bConsumeInsertedItem = true ou false selon énigme
bAllowItemRemoval = selon énigme
bDisplayInsertedItem = true ou false
```

---

## Paramètres spécifiques des portes

Les portes et portes secrètes doivent pouvoir recevoir :

```cpp
float OpenDuration;
bool bStartsOpen;
bool bStartsLocked;
bool bBlocksMovementWhenClosed;
bool bBlocksMovementWhileOpening;
```

Pour `AGridSecretDoorActor`, il faut préserver l’idée :

```text
mesh fixe + mesh mobile
```

La partie fixe doit être visible en édition et en runtime.

Note Patch D : `Door_Secret` reste un archétype `Door`. La logique runtime utilise `EGridLevelObjectType::Door` et accepte une `RuntimeActorClass` dérivée de `AGridDoorActor`, par exemple `AGridSecretDoorActor` ou un Blueprint dérivé. Le preview éditeur actuel affiche un mesh principal unique (`PreviewMesh`, puis `MovingMesh`, puis `FixedMesh`) et ne rend pas encore un composite fixe + mobile complet.

---

## Règle d’évolution

Chaque nouvel objet doit d’abord être ajouté comme archétype avant de créer une nouvelle classe C++.

Créer une nouvelle classe C++ uniquement si :

1. le comportement ne peut pas être exprimé par paramètres ;
2. le runtime nécessite une logique spécifique ;
3. l’éditeur ou la preview nécessite un traitement particulier ;
4. la classe existante deviendrait trop complexe.

