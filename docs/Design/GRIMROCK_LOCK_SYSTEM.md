# GrimrockPrototype — Système de serrures, clés, crochetage et pièges


> **Statut : spécification de conception prospective.**
> Ce document ancre le système de locks dans le modèle existant du prototype, mais ne doit pas être lu comme un contrat runtime déjà entièrement implémenté.
> Les fondations actuellement vérifiées côté architecture restent notamment `docs/Architecture/DOOR_MECHANISM_FOUNDATION.md`, `docs/Architecture/LINK_EVENT_COMMAND_FOUNDATION.md`, `docs/Architecture/RECEPTACLE_SYSTEM_FOUNDATION.md` et `docs/Architecture/ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md`.

> **Règle d'intégration :** les portes de donjon restent des passages commandables. Les serrures murales, clés, coffres verrouillés, conteneurs verrouillés et pièges doivent s'intégrer au modèle `SourceObject + SourceEvent -> TargetObject + TargetCommand`, sans créer de communication directe entre objets.

Version de conception : 1.0  
Projet : **GrimrockPrototype**  
Moteur : **Unreal Engine 5.5.4**  
Objectif : ancrer le système de serrures dans l'architecture du prototype Grimrock, en respectant la séparation entre portes, mécanismes, objets verrouillables, clés, crochetage et pièges.

---

# 1. Intention de conception

Le système de serrure doit permettre de gérer :

- des serrures murales actionnées par clé ;
- des serrures acceptant plusieurs clés possibles ;
- des profils de clés mécaniques ;
- des clés maîtresses ;
- le crochetage par un membre de l'équipe ;
- l'utilisation obligatoire d'un set de crochetage ;
- des serrures piégées ;
- des coffres, boîtes, autels ou réceptacles verrouillés ;
- des mécanismes logiques qui déclenchent l'ouverture d'une porte, l'activation d'un piège ou la libération d'un autre mécanisme.

La règle fondamentale est la suivante :

> **Une porte ne possède pas de serrure. Une porte est un objet passif commandable.**  
> **Une serrure est un mécanisme séparé qui déclenche des actions sur une porte ou sur un autre objet.**

Cette règle vaut pour les portes du donjon afin de conserver un même set de portes réutilisable avec des mécanismes d'ouverture variés : levier, chaîne, bouton, serrure murale, plaque de pression, réceptacle, trigger, script, etc.

En revanche, les objets comme les coffres, boîtes, petits conteneurs ou certains réceptacles peuvent porter leur propre serrure interne, car ils sont eux-mêmes l'objet verrouillable.

---

# 2. Principes d'architecture

## 2.1. Porte

Une porte doit rester simple.

Elle sait :

- si elle est ouverte ou fermée ;
- si elle est en mouvement ;
- si elle bloque le passage ;
- quelle animation jouer ;
- quel son jouer ;
- quelles commandes elle accepte : `Open`, `Close`, `Toggle`, `ForceOpen`, `ForceClose`.

Elle ne sait pas :

- quelle clé existe ;
- quelle serrure existe ;
- si le joueur possède un set de crochetage ;
- si la serrure est piégée ;
- pourquoi elle doit s'ouvrir.

La porte est donc une **cible logique**.

Exemple :

```text
Lock_Prison_A.OnUnlocked -> Door_Prison_A.Open
Chain_Gate_A.OnPulled    -> Door_Gate_A.Open
Lever_Hall_A.OnActivated -> Door_Secret_A.Toggle
```

---

## 2.2. Serrure murale

La serrure murale est un mécanisme interactif placé contre un mur.

Elle peut :

- recevoir un clic souris ;
- vérifier une clé ;
- vérifier un profil de clé ;
- vérifier une clé maîtresse ;
- tenter un crochetage ;
- déclencher un piège ;
- déclencher des liens logiques en cas de réussite ou d'échec.

Elle ne doit pas être considérée comme une partie de la porte. Elle est un objet indépendant du niveau.

Exemple :

```text
ObjectId: Lock_Prison_North
ObjectType: WallLock
Cell: X=8, Y=12
Facing: North
AcceptedKeyIds:
  - Key_Prison_North
CanBePicked: true
LockpickDifficulty: 8
OnUnlocked:
  - Target: Door_Prison_North
    Action: Open
```

---

## 2.3. Objet verrouillable

Certains objets peuvent porter leur propre serrure :

- coffre ;
- boîte ;
- caisse renforcée ;
- petit autel verrouillé ;
- réceptacle scellé ;
- armoire ;
- sarcophage ;
- reliquaire ;
- mécanisme ancien manipulable directement.

Dans ce cas, la serrure appartient à l'objet, car l'objet est lui-même la cible de l'ouverture.

Exemple :

```text
Chest_OldWoodenChest
  HasLock: true
  LockData:
    AcceptedKeyIds:
      - Key_OldChest
    CanBePicked: true
    LockpickDifficulty: 6
    TrapData:
      bHasTrap: true
      TrapType: PoisonNeedle
```

Important : cela ne contredit pas la règle concernant les portes.  
La règle corrigée est donc :

> **Les portes du donjon ne possèdent pas de serrure.**  
> **Les objets conteneurs peuvent posséder une serrure interne.**  
> **Le même modèle de données de serrure doit être réutilisé partout.**

---

# 3. Typologie des objets concernés

## 3.1. Objets qui ne portent pas de serrure

Ces objets sont commandés par des mécanismes externes :

| Objet | Serrure interne ? | Raison |
|---|---:|---|
| Porte standard | Non | Ouverte par levier, chaîne, serrure murale, plaque, script, etc. |
| Porte en fer | Non | Même principe que porte standard. |
| Herse | Non | Commandée par chaîne, levier ou mécanisme distant. |
| Porte secrète | Non de préférence | Ouverte par bouton secret, serrure murale ou script. |
| Passage secret | Non de préférence | Réagit aux événements logiques. |

---

## 3.2. Objets pouvant porter une serrure

Ces objets peuvent posséder une serrure interne :

| Objet | Serrure interne ? | Exemple |
|---|---:|---|
| Coffre | Oui | Coffre en bois, coffre en fer, coffre piégé. |
| Boîte | Oui | Boîte à bijoux, boîte de quête. |
| Reliquaire | Oui | Nécessite une clé sacrée ou crochetage impossible. |
| Sarcophage | Oui | Peut être verrouillé et piégé. |
| Armoire | Oui | Conteneur verrouillé. |
| Réceptacle spécial | Oui, optionnel | Nécessite une clé ou un sceau. |
| Mécanisme à clé | Oui | L'objet est à la fois serrure et mécanisme. |

---

# 4. Modèle logique général

Le système repose sur quatre familles d'objets :

```text
[Clé / outil / compétence]
        ↓
[Serrure ou mécanisme verrouillé]
        ↓
[Événement logique]
        ↓
[Porte, coffre, piège, message, trigger, script]
```

Une serrure ne doit pas forcément ouvrir quelque chose directement. Elle peut aussi :

- déverrouiller une chaîne ;
- activer un levier ;
- désactiver un piège ;
- ouvrir un coffre ;
- ouvrir une porte distante ;
- déclencher un message ;
- déclencher une embuscade ;
- changer une variable de niveau ;
- alimenter un script d'énigme.

---

# 5. Clés et profils de clés

## 5.1. Clé exacte

La méthode la plus simple consiste à dire qu'une serrure accepte une ou plusieurs clés exactes.

Exemple :

```text
AcceptedKeyIds:
  - Key_Prison_North
  - Key_Prison_Guard_Copy
```

Cela permet d'avoir plusieurs copies d'une même clé ou plusieurs clés différentes ouvrant la même serrure.

---

## 5.2. Profil de clé

Une clé peut aussi posséder un profil mécanique.

Exemple :

```text
KeyId: Key_Copper_Prison_A
ProfileId: KeyProfile_Prison_Copper
GrooveMask: 00110100
TeethCode: 4213
```

La serrure peut alors accepter toutes les clés d'un certain profil :

```text
AcceptedKeyProfiles:
  - KeyProfile_Prison_Copper
```

Cela permet de créer :

- des clés de secteur ;
- des clés de famille ;
- des clés anciennes compatibles avec certaines serrures ;
- des clés partiellement compatibles ;
- des clés maîtresses.

---

## 5.3. Tags de clé

Les tags permettent une compatibilité plus souple.

Exemple :

```text
KeyTags:
  - Key.Prison
  - Key.Copper
  - Key.Level01
```

Une serrure peut accepter :

```text
AcceptedKeyTags:
  - Key.Prison
```

Ce système est utile pour les clés génériques ou les clés maîtresses de zone.

---

## 5.4. Clé maîtresse

Une clé peut être marquée comme clé maîtresse.

Exemple :

```text
KeyId: Key_Master_Dungeon_A
bIsMasterKey: true
KeyTags:
  - Key.Master
  - Key.Master.DungeonA
```

La serrure décide si elle accepte ou non les clés maîtresses :

```text
bAllowMasterKey: true
```

Certaines serrures importantes doivent pouvoir refuser les clés maîtresses :

```text
bAllowMasterKey: false
```

---

# 6. Données C++ proposées

## 6.1. État de serrure

```cpp
UENUM(BlueprintType)
enum class EGridLockState : uint8
{
    Unlocked,
    Locked,
    Jammed,
    Broken
};
```

- `Unlocked` : la serrure est ouverte.
- `Locked` : la serrure est verrouillée.
- `Jammed` : la serrure est bloquée, généralement à cause d'un échec critique ou d'un piège.
- `Broken` : la serrure est détruite ou inutilisable.

---

## 6.2. Profil de clé

```cpp
USTRUCT(BlueprintType)
struct FGridKeyProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ProfileId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 GrooveMask = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 TeethCode = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Quality = 0;
};
```

Le profil mécanique est volontairement simplifié. Il permet une logique de compatibilité sans simuler une vraie serrure physique.

---

## 6.3. Données de clé

```cpp
USTRUCT(BlueprintType)
struct FGridKeyData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName KeyId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGridKeyProfile Profile;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FName> KeyTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIsMasterKey = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 RemainingUses = -1;
};
```

`RemainingUses = -1` signifie usages illimités.

---

## 6.4. Données de serrure

```cpp
USTRUCT(BlueprintType)
struct FGridLockData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName LockId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EGridLockState InitialState = EGridLockState::Locked;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FName> AcceptedKeyIds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FName> AcceptedKeyProfiles;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FName> AcceptedKeyTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bAllowMasterKey = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bCanBePicked = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 LockpickDifficulty = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName RequiredToolTag = "Tool.LockpickSet";

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bStayUnlocked = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bConsumeKeyOnUse = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bHasTrap = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="bHasTrap"))
    FGridLockTrapData TrapData;
};
```

---

# 7. Résultat d'une tentative de déverrouillage

Il est important de ne pas retourner seulement `true` ou `false`. Le runtime doit savoir pourquoi l'action a réussi ou échoué.

```cpp
UENUM(BlueprintType)
enum class EGridUnlockResultType : uint8
{
    SuccessWithKey,
    SuccessWithMasterKey,
    SuccessWithLockpick,
    FailedNoKey,
    FailedNoTool,
    FailedSkillTooLow,
    FailedUnpickable,
    FailedJammed,
    FailedBroken,
    FailedTrapTriggered
};
```

```cpp
USTRUCT(BlueprintType)
struct FGridUnlockResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EGridUnlockResultType ResultType;

    UPROPERTY(BlueprintReadOnly)
    FName UsedKeyId;

    UPROPERTY(BlueprintReadOnly)
    FName UsedCharacterId;

    UPROPERTY(BlueprintReadOnly)
    FText Message;
};
```

---

# 8. Crochetage

## 8.1. Conditions de crochetage

Une serrure peut être crochetée si :

1. elle est verrouillée ;
2. elle n'est pas cassée ;
3. elle n'est pas bloquée ;
4. `bCanBePicked == true` ;
5. l'équipe possède un item avec le tag `Tool.LockpickSet` ;
6. au moins un personnage possède une compétence suffisante.

---

## 8.2. Compétence d'équipe

Le système doit chercher le meilleur membre de l'équipe.

Exemple :

```text
BestLockpickingSkill = Max(PartyMember.Lockpicking)
```

La réussite déterministe recommandée pour le prototype :

```text
BestLockpickingSkill + ToolBonus >= LockpickDifficulty
```

Cette approche évite la frustration d'un système aléatoire dans un dungeon crawler à énigmes.

---

## 8.3. Outils de crochetage

Exemples :

```text
Tool_LockpickSet_Basic
  Tags:
    - Tool.LockpickSet
  ToolBonus: 0

Tool_LockpickSet_Fine
  Tags:
    - Tool.LockpickSet
  ToolBonus: 2

Tool_LockpickSet_Master
  Tags:
    - Tool.LockpickSet
  ToolBonus: 4
```

La durabilité peut être ajoutée plus tard :

```text
Durability: 10
BreakOnFailure: false
```

---

# 9. Serrures piégées

Les serrures piégées doivent être pensées dès le départ, même si leur implémentation complète vient plus tard.

Une serrure piégée peut réagir à :

- une mauvaise clé ;
- une absence de clé ;
- une tentative de crochetage ;
- un crochetage raté ;
- un crochetage réussi ;
- une ouverture sans désamorçage préalable ;
- une inspection ratée ;
- une inspection réussie.

---

## 9.1. Données de piège

```cpp
UENUM(BlueprintType)
enum class EGridLockTrapTriggerPolicy : uint8
{
    Never,
    OnWrongKey,
    OnPickAttempt,
    OnPickFailure,
    OnUnlockSuccess,
    OnOpenContainer,
    OnInteractionIfNotDisarmed
};
```

```cpp
UENUM(BlueprintType)
enum class EGridLockTrapState : uint8
{
    Armed,
    Disarmed,
    Triggered,
    Disabled
};
```

```cpp
UENUM(BlueprintType)
enum class EGridLockTrapType : uint8
{
    None,
    PoisonNeedle,
    Dart,
    FireBurst,
    GasCloud,
    Alarm,
    SpawnMonsters,
    CloseDoors,
    Teleport,
    Scripted
};
```

```cpp
USTRUCT(BlueprintType)
struct FGridLockTrapData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName TrapId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EGridLockTrapType TrapType = EGridLockTrapType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EGridLockTrapState InitialTrapState = EGridLockTrapState::Armed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EGridLockTrapTriggerPolicy TriggerPolicy = EGridLockTrapTriggerPolicy::OnPickFailure;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bCanBeDetected = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 DetectDifficulty = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bCanBeDisarmed = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 DisarmDifficulty = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Damage = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName DamageType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FGridLogicLink> OnTrapTriggeredLinks;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FGridLogicLink> OnTrapDisarmedLinks;
};
```

---

## 9.2. Détection des pièges

Le système peut fonctionner en deux temps :

1. le joueur examine la serrure ;
2. si un personnage possède assez de compétence, le piège est révélé.

Compétences possibles :

```text
Perception
Mechanisms
Lockpicking
Traps
```

Pour garder le prototype simple, on peut commencer avec :

```text
BestTrapDetectionSkill >= DetectDifficulty
```

Puis :

```text
BestDisarmSkill + ToolBonus >= DisarmDifficulty
```

---

## 9.3. Déclenchement des pièges

Exemples :

```text
Trap_PoisonNeedle:
  TriggerPolicy: OnPickFailure
  DamageType: Poison
  Damage: 5

Trap_Alarm:
  TriggerPolicy: OnWrongKey
  OnTrapTriggered:
    - Target: Spawn_Guards_A
      Action: Activate

Trap_AncientGas:
  TriggerPolicy: OnOpenContainer
  OnTrapTriggered:
    - Target: GasEmitter_A
      Action: Activate
```

---

# 10. Acteurs recommandés

## 10.1. `AGridDoorActor`

Rôle : porte commandable.

Ne contient pas de serrure.

Actions exposées :

```cpp
Open()
Close()
Toggle()
ForceOpen()
ForceClose()
SetEnabled(bool bEnabled)
```

---

## 10.2. `AGridWallLockActor`

Rôle : serrure murale indépendante.

Contient :

```text
GridPosition
Facing
LockData
CurrentLockState
CurrentTrapState
LogicLinks
```

Événements possibles :

```text
OnUnlockedWithKey
OnUnlockedWithMasterKey
OnPicked
OnUnlockFailed
OnWrongKey
OnTrapDetected
OnTrapDisarmed
OnTrapTriggered
OnJammed
OnBroken
```

---

## 10.3. `AGridLockableContainerActor`

Rôle : base pour les coffres, boîtes, reliquaires, sarcophages, etc.

Contient :

```text
ContainerId
InventoryContent
bHasLock
LockData
CurrentLockState
CurrentTrapState
OpenState
```

Actions :

```cpp
Interact(Party)
TryOpen(Party)
OpenContainer()
CloseContainer()
TryUnlock(Party)
TryPick(Party)
TryDetectTrap(Party)
TryDisarmTrap(Party)
```

Types dérivés possibles :

```text
AGridChestActor
AGridBoxActor
AGridReliquaryActor
AGridSarcophagusActor
AGridLockedReceptacleActor
```

---

## 10.4. `UGridLockComponent`

Pour éviter de dupliquer la logique entre une serrure murale et un coffre, il est préférable de créer un composant réutilisable.

```cpp
UCLASS(ClassGroup=(Grimrock), meta=(BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridLockComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid Lock")
    FGridLockData LockData;

    UPROPERTY(BlueprintReadOnly, Category="Grid Lock")
    EGridLockState CurrentLockState;

    UFUNCTION(BlueprintCallable)
    FGridUnlockResult TryUnlock(AGrimrockPartyPawn* Party);

    UFUNCTION(BlueprintCallable)
    FGridUnlockResult TryPick(AGrimrockPartyPawn* Party);

    UFUNCTION(BlueprintCallable)
    bool IsLocked() const;

    UFUNCTION(BlueprintCallable)
    void ForceUnlock();
};
```

Utilisation :

```text
AGridWallLockActor possède UGridLockComponent
AGridChestActor possède UGridLockComponent
AGridBoxActor possède UGridLockComponent
AGridReliquaryActor possède UGridLockComponent
```

Mais :

```text
AGridDoorActor ne possède pas UGridLockComponent
```

C'est une règle volontaire.

---

# 11. Flux runtime

## 11.1. Interaction avec une serrure murale

```text
1. Le joueur clique sur la serrure murale.
2. La serrure vérifie son état.
3. Si elle est déjà déverrouillée :
   - elle déclenche éventuellement OnAlreadyUnlocked.
4. Si elle est verrouillée :
   - elle cherche une clé compatible dans l'inventaire de l'équipe.
5. Si une clé compatible existe :
   - elle vérifie les pièges éventuels.
   - elle déverrouille.
   - elle déclenche OnUnlockedWithKey.
6. Si aucune clé n'existe :
   - elle vérifie si le crochetage est autorisé.
7. Si le crochetage est possible :
   - elle vérifie le set de crochetage.
   - elle vérifie la compétence de l'équipe.
8. Si crochetage réussi :
   - elle vérifie les pièges éventuels.
   - elle déverrouille.
   - elle déclenche OnPicked.
9. Si échec :
   - elle déclenche OnUnlockFailed ou OnPickFailed.
   - elle déclenche éventuellement le piège.
```

---

## 11.2. Interaction avec un coffre verrouillé

```text
1. Le joueur clique sur le coffre.
2. Le coffre vérifie s'il possède une serrure.
3. Si aucune serrure :
   - il s'ouvre.
4. Si serrure verrouillée :
   - il tente une clé.
   - sinon il tente le crochetage si autorisé.
5. Si succès :
   - il déclenche les pièges configurés selon leur politique.
   - il ouvre le coffre.
6. Si échec :
   - il affiche un message.
   - il déclenche éventuellement le piège.
```

---

# 12. Liens logiques

Les serrures doivent s'intégrer au système d'événements du niveau.

Exemples :

```text
Lock_CellBlock_A.OnUnlockedWithKey -> Door_CellBlock_A.Open
Lock_CellBlock_A.OnPicked          -> Door_CellBlock_A.Open
Lock_CellBlock_A.OnWrongKey        -> Message_CellBlock_WrongKey.Show
Lock_CellBlock_A.OnTrapTriggered   -> Spawn_Guards_A.Activate
```

Pour un coffre :

```text
Chest_Old_A.OnOpened          -> QuestFlag_FoundRelic.SetTrue
Chest_Old_A.OnTrapTriggered   -> GasEmitter_A.Activate
Chest_Old_A.OnTrapDisarmed    -> Message_TrapSafe.Show
```

Pour une chaîne :

```text
Lock_Chain_A.OnUnlockedWithKey -> Chain_A.Enable
Chain_A.OnPulled               -> Door_Gate_A.Open
```

---

# 13. Intégration dans le `GridLevelAsset`

## 13.1. Serrure murale

```text
GridObject:
  ObjectId: Lock_Prison_North
  ObjectType: WallLock
  Cell: 8,12
  Facing: North
  MeshId: SM_WallLock_Iron_01
  LockData:
    LockId: LockData_Prison_North
    InitialState: Locked
    AcceptedKeyIds:
      - Key_Prison_North
    AcceptedKeyProfiles:
      - KeyProfile_Prison_Copper
    bCanBePicked: true
    LockpickDifficulty: 8
    bHasTrap: false
  Links:
    OnUnlockedWithKey:
      - Target: Door_Prison_North
        Action: Open
    OnPicked:
      - Target: Door_Prison_North
        Action: Open
```

---

## 13.2. Coffre verrouillé et piégé

```text
GridObject:
  ObjectId: Chest_Armory_A
  ObjectType: Chest
  Cell: 4,7
  Facing: East
  MeshId: SM_Chest_Wood_Iron_01
  bHasInventory: true
  InventoryContent:
    - Item_Sword_Rusty
    - Item_Gold_25
  bHasLock: true
  LockData:
    LockId: LockData_ArmoryChest_A
    InitialState: Locked
    AcceptedKeyIds:
      - Key_Armory_Chest
    bCanBePicked: true
    LockpickDifficulty: 12
    bHasTrap: true
    TrapData:
      TrapId: Trap_ArmoryChest_PoisonNeedle
      TrapType: PoisonNeedle
      TriggerPolicy: OnPickFailure
      DetectDifficulty: 10
      DisarmDifficulty: 12
      Damage: 6
      DamageType: Poison
```

---

# 14. Interaction souris et feedback utilisateur

Le curseur doit indiquer l'action possible.

| Situation | Curseur recommandé |
|---|---|
| Serrure + clé disponible | Curseur clé |
| Serrure + crochetage possible | Curseur crochet / outils |
| Serrure verrouillée sans solution visible | Cadenas fermé |
| Serrure déjà ouverte | Cadenas ouvert |
| Serrure piégée détectée | Cadenas avec danger |
| Coffre verrouillé | Cadenas / clé |
| Coffre ouvert | Main / interaction |

Messages possibles :

```text
La clé tourne dans la serrure.
Cette clé ne convient pas.
La serrure résiste.
Il faudrait un set de crochetage.
Personne dans l'équipe n'est assez habile.
Vous entendez un déclic inquiétant.
Un piège se déclenche !
Le piège est désamorcé.
Cette serrure semble impossible à crocheter.
```

---

# 15. Validation éditeur

Le mode éditeur devrait détecter les erreurs fréquentes.

## 15.1. Serrures murales

Avertissements :

- serrure verrouillée sans clé acceptée ;
- serrure verrouillée non crochetable et sans lien logique utile ;
- serrure qui cible une porte inexistante ;
- serrure avec `OnUnlocked` vide ;
- serrure piégée sans effet configuré ;
- serrure avec difficulté de crochetage supérieure à toutes les compétences prévues ;
- clé référencée inexistante ;
- profil de clé référencé inexistant.

---

## 15.2. Coffres et boîtes

Avertissements :

- coffre verrouillé sans clé et non crochetable ;
- coffre piégé sans possibilité de détection ou de désamorçage ;
- coffre sans contenu ;
- coffre de quête verrouillé de manière irréversible ;
- piège configuré mais sans effet ;
- clé de coffre placée derrière le coffre lui-même.

---

## 15.3. Clés

Avertissements :

- clé qui n'ouvre rien ;
- clé maîtresse acceptée par trop de serrures critiques ;
- clé consommée alors qu'elle est nécessaire ailleurs ;
- doublons de `KeyId` ;
- profils mécaniques incohérents.

---

# 16. Règles de design recommandées

## 16.1. Pour les portes

- Ne jamais mettre de serrure directement dans `GridDoorActor`.
- Toujours passer par un mécanisme externe.
- Une porte peut être commandée par plusieurs mécanismes.
- Un mécanisme peut commander plusieurs portes.

Exemple :

```text
Lever_A.OnActivated -> Door_A.Open
Lock_B.OnUnlocked   -> Door_A.Open
Plate_C.OnPressed   -> Door_A.Close
```

---

## 16.2. Pour les coffres

- Un coffre peut porter une serrure interne.
- Un coffre peut être piégé.
- Le piège doit pouvoir être détecté et désamorcé.
- Le crochetage peut déclencher le piège.
- L'ouverture avec la bonne clé peut éventuellement éviter le piège.

Exemple intéressant :

```text
Bonne clé       -> coffre ouvert sans déclencher le piège
Crochetage      -> coffre ouvert mais piège déclenché
Désamorçage     -> coffre ouvert sans danger
Mauvaise clé    -> piège déclenché
```

---

## 16.3. Pour les serrures piégées

- Toujours pouvoir configurer quand le piège se déclenche.
- Ne pas supposer que tous les pièges se déclenchent sur échec.
- Prévoir des pièges narratifs et logiques, pas seulement des dégâts.

Exemples :

```text
PoisonNeedle     -> dégâts poison sur le personnage actif
Alarm            -> spawn de gardes
GasCloud         -> zone de gaz dans la cellule
CloseDoors       -> fermeture de portes derrière le joueur
Teleport         -> déplacement forcé
Scripted         -> événement personnalisé
```

---

# 17. Roadmap d'implémentation

## Étape 1 — Types de base

Créer :

```text
GridLockTypes.h
GridKeyTypes.h
GridLockTrapTypes.h
```

Avec :

```text
FGridKeyData
FGridKeyProfile
FGridLockData
FGridLockTrapData
FGridUnlockResult
EGridLockState
EGridUnlockResultType
```

---

## Étape 2 — Composant réutilisable

Créer :

```text
UGridLockComponent
```

Ce composant doit gérer :

```text
TryUnlock
TryPick
FindCompatibleKey
CheckKeyProfile
CheckKeyTags
CheckMasterKey
CheckTrapPolicy
TriggerTrap
ForceUnlock
```

---

## Étape 3 — Serrure murale

Créer :

```text
AGridWallLockActor
BP_GridWallLockActor
SM_WallLock_...
```

Il doit :

```text
- être placé contre un mur ;
- recevoir l'interaction souris ;
- utiliser UGridLockComponent ;
- déclencher des liens logiques.
```

---

## Étape 4 — Coffres et boîtes

Créer une base :

```text
AGridLockableContainerActor
```

Puis :

```text
AGridChestActor
AGridBoxActor
```

Ils doivent :

```text
- contenir un inventaire ;
- posséder optionnellement une serrure ;
- posséder optionnellement un piège ;
- s'ouvrir après réussite.
```

---

## Étape 5 — Crochetage

Ajouter côté équipe :

```text
BestLockpickingSkill
HasToolWithTag(Tool.LockpickSet)
GetBestLockpickToolBonus
```

Puis brancher :

```text
LockpickDifficulty
ToolBonus
CharacterSkill
```

---

## Étape 6 — Pièges

Ajouter :

```text
Trap detection
Trap disarming
Trap triggered events
Trap messages
Trap damage/effects
```

Commencer simple :

```text
PoisonNeedle
Alarm
GasCloud
Scripted
```

---

## Étape 7 — Éditeur

Ajouter dans le mode éditeur :

```text
Palette WallLock
Palette Chest
Palette Box
Properties LockData
Properties TrapData
AcceptedKeyIds
AcceptedKeyProfiles
AcceptedKeyTags
LockpickDifficulty
TrapTriggerPolicy
Validation du niveau
```

---

# 18. Synthèse finale

Le système doit être construit autour de cette séparation :

```text
Door = objet commandable, jamais porteur de serrure
WallLock = mécanisme mural utilisant clé/crochetage/piège
Container = objet verrouillable pouvant posséder sa serrure interne
Key = item d'inventaire avec identifiant, profil et tags
Lockpick = outil d'inventaire requis pour le crochetage
PartySkill = compétence utilisée pour résoudre le crochetage ou le désamorçage
Trap = sous-système optionnel attaché à une serrure
LogicLinks = système reliant mécanismes, portes, coffres, pièges et scripts
```

La phrase de référence à garder pour le développement est :

> **Une clé n'ouvre pas une porte. Une clé agit sur une serrure. La serrure déclenche ensuite un événement.**

Et pour les conteneurs :

> **Un coffre peut avoir une serrure, parce que le coffre est lui-même l'objet verrouillable.**

Ce modèle permet de conserver des portes standardisées tout en offrant une grande variété de mécanismes, d'énigmes, de clés, de pièges et d'interactions dans le donjon.
