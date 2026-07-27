# GrimrockPrototype — Monstres, animations, IA et combat tour par tour

## Prototype initial : Rat géant — `MON_RatGiant`

**Projet :** GrimrockPrototype  
**Moteur :** Unreal Engine 5.5.4  
**Architecture :** C++, orientée données, grille 32 × 32  
**Premier monstre :** Rat géant  
**Version du guide :** 0.1  
**Date :** 11 juillet 2026

---

## 1. Objectif général

L’objectif n’est pas seulement d’ajouter un Rat géant dans le donjon. Il faut construire une fondation capable d’accueillir ensuite l’ensemble des créatures définies dans les bestiaires de `docs/ArtBook`, notamment :

- l’Araignée mineure ;
- le Slime vert ;
- le Champignon toxique ;
- les Squelettes ;
- le Zombie ;
- la Mimique ;
- le Ver des cryptes ;
- la Gargouille ;
- le Golem de pierre ;
- les boss ;
- les créatures spéciales liées à des mécanismes ou à des énigmes.

Le bestiaire définit le Rat géant comme suit :

- nom technique : `MON_RatGiant` ;
- catégorie : Vermine ;
- danger : 1 ;
- rôle : tutoriel et harceleur faible ;
- comportement : `DirectMelee / FastHarasser` ;
- faiblesse : feu et armes tranchantes ;
- résistance : aucune ;
- butin potentiel : viande de rat, dent ou rien.

Le document illustré précise également que :

- `DirectMelee` recherche le chemin le plus court vers le joueur et attaque au contact ;
- `FastHarasser` attaque rapidement puis cherche à se repositionner.

Le système développé pour ce premier ennemi doit donc déjà séparer clairement :

1. la définition des données du monstre ;
2. sa représentation visuelle ;
3. son occupation de la grille ;
4. son comportement tactique ;
5. la résolution des attaques ;
6. la gestion des tours ;
7. la sauvegarde de son état ;
8. son intégration aux événements du niveau.

---

## 2. Principes architecturaux fondamentaux

### 2.1. La grille reste l’autorité absolue

Le déplacement d’un monstre ne doit pas être décidé par :

- la physique ;
- le NavMesh d’Unreal Engine ;
- le Root Motion ;
- l’Animation Blueprint ;
- les collisions de son Skeletal Mesh.

La position logique du monstre est toujours définie par :

```cpp
FIntPoint GridCell;
EGridEdge Facing;
```

Sa position visuelle dans le monde est uniquement la représentation animée de cette position logique.

Le runtime possède déjà les fonctions nécessaires pour interroger la grille :

```cpp
GetCellCenterWorld();
IsValidCell();
IsWalkableCell();
TryGetNeighborCell();
CanMove();
```

Ces fonctions doivent être réutilisées par les monstres plutôt que de créer un second système de déplacement.

### 2.2. Pas de Behavior Tree pour la première version

Unreal Engine permet de construire une IA avec :

- `AIController` ;
- Behavior Tree ;
- Blackboard ;
- NavMesh ;
- AI Perception.

Cette architecture serait disproportionnée pour le premier prototype, car :

- le donjon est une petite grille de 1 024 cases au maximum ;
- tous les déplacements sont orthogonaux ;
- les actions sont discrètes ;
- le combat est tour par tour ;
- les portes et les murs sont déjà gérés par le runtime de grille ;
- l’IA doit être déterministe et facile à tester.

La première version utilisera donc :

- un pathfinding de grille C++ ;
- un composant de comportement ;
- une évaluation simple des actions possibles ;
- un gestionnaire centralisé des tours.

Les Behavior Trees pourront rester une extension future pour les boss ou les comportements particulièrement complexes.

### 2.3. Pas de Root Motion pour les déplacements sur la grille

Pour GrimrockPrototype :

- les animations de marche seront réalisées sur place ;
- le code interpolera l’Actor d’un centre de case au suivant ;
- l’animation devra visuellement accompagner ce déplacement ;
- la position finale sera recalée exactement sur le centre de la case.

Cette solution évite toute dérive entre :

- la position logique ;
- le mesh ;
- la collision ;
- le système d’occupation.

### 2.4. Les animations ne déterminent jamais seules le résultat d’une attaque

L’attaque doit être résolue selon cette séquence :

```text
Décision IA
    ↓
Validation de l’action
    ↓
Calcul préalable du jet d’attaque et des dégâts
    ↓
Lecture de l’animation
    ↓
Anim Notify au moment de l’impact
    ↓
Application du résultat déjà calculé
    ↓
Fin de l’action
```

L’Anim Notify ne décide donc pas si l’attaque touche. Il indique seulement le moment visuel où le résultat doit être appliqué.

### 2.5. La logique doit rester indépendante du framerate

Aucune décision de combat ne doit dépendre du nombre d’images par seconde.

Le framerate sert uniquement à :

- interpoler les déplacements ;
- mettre à jour les animations ;
- afficher les effets visuels ;
- temporiser les transitions visuelles.

Les décisions, les jets, les dégâts, les coûts en points d’action et les changements d’état doivent être entièrement déterministes.

---

## 3. État actuel du projet à préserver

### 3.1. Le type `MonsterSpawn` existe déjà

`EGridLevelObjectType` contient déjà :

```cpp
MonsterSpawn
```

Le placement d’un monstre doit donc rester un objet du `UGridLevelAsset`, et non un Actor enregistré directement dans une carte Unreal.

Chaque placement possède déjà notamment :

- un `ObjectId` ;
- une cellule X/Y ;
- une orientation locale ;
- un `ArchetypeId` ;
- un état Enabled ;
- des paramètres de comportement.

### 3.2. Le niveau reste un asset unique

`UGridLevelAsset` contient :

- la taille de la grille ;
- les cellules ;
- la position initiale du groupe ;
- les objets ;
- les liens logiques.

Les monstres devront donc être ajoutés à :

```cpp
UGridLevelAsset::Objects
```

sous forme de placements `MonsterSpawn`.

### 3.3. Le groupe occupe une seule case

Le projet prévoit un groupe de un à six personnages répartis sur deux rangs de trois emplacements, mais le groupe complet occupe une seule cellule du donjon.

Cette distinction est essentielle :

- le déplacement est géré au niveau du groupe ;
- les attaques et dégâts sont gérés au niveau des personnages ;
- le monstre cible la case du groupe ;
- l’attaque du monstre cible ensuite un personnage précis dans la formation.

### 3.4. Les statistiques RPG existantes doivent être réutilisées

Les personnages possèdent déjà :

- des points de vie ;
- de la mana ;
- une armure physique ;
- une armure magique ;
- une initiative ;
- une précision ;
- une esquive ;
- des résistances aux différents types de dégâts.

Le système de combat des monstres doit utiliser ces structures, et non créer une seconde représentation des statistiques des personnages.

---

## 4. Structure C++ proposée

Créer un nouveau domaine fonctionnel sans surcharger `AGridLevelRuntimeActor`.

```text
Source/GrimrockPrototype/
├── Public/
│   └── Runtime/
│       ├── Combat/
│       │   ├── GridCombatTypes.h
│       │   ├── GridTurnManagerComponent.h
│       │   ├── GridCombatResolver.h
│       │   └── GridCombatLog.h
│       │
│       └── Monsters/
│           ├── GridMonsterTypes.h
│           ├── GridMonsterDefinitionAsset.h
│           ├── GridMonsterActor.h
│           ├── GridMonsterAnimInstance.h
│           ├── GridMonsterBehaviorComponent.h
│           ├── GridMonsterCombatComponent.h
│           └── GridMonsterPathfinder.h
│
└── Private/
    └── Runtime/
        ├── Combat/
        │   ├── GridTurnManagerComponent.cpp
        │   ├── GridCombatResolver.cpp
        │   └── GridCombatLog.cpp
        │
        └── Monsters/
            ├── GridMonsterDefinitionAsset.cpp
            ├── GridMonsterActor.cpp
            ├── GridMonsterAnimInstance.cpp
            ├── GridMonsterBehaviorComponent.cpp
            ├── GridMonsterCombatComponent.cpp
            └── GridMonsterPathfinder.cpp
```

### 4.1. Ne pas dériver le monstre de `AGridRuntimeObjectActor`

`AGridRuntimeObjectActor` est actuellement conçu autour d’un `UStaticMeshComponent`.

Un monstre animé nécessite au contraire :

- un `USkeletalMeshComponent` ;
- une capsule ou une boîte de collision ;
- un Animation Blueprint ;
- plusieurs composants de gameplay ;
- une gestion spécifique de ses animations et de sa mort.

La hiérarchie recommandée est donc :

```cpp
AGridMonsterActor : public AActor
```

Le spawn du niveau reste toutefois un `FGridLevelObjectData`.

Il faut ajouter à `AGridLevelRuntimeActor` un registre distinct :

```cpp
UPROPERTY(Transient)
TMap<FGuid, TObjectPtr<AGridMonsterActor>> SpawnedMonsterActors;
```

### 4.2. Répartition des responsabilités

#### `AGridMonsterActor`

- représente l’instance dans le monde ;
- contient le Skeletal Mesh ;
- conserve la cellule, l’orientation et les valeurs runtime ;
- orchestre les animations et les transitions visuelles.

#### `UGridMonsterDefinitionAsset`

- contient les données de définition ;
- référence le mesh, l’Animation Blueprint, les attaques et les sons ;
- définit les statistiques et le profil tactique.

#### `UGridMonsterBehaviorComponent`

- analyse la situation ;
- choisit les actions ;
- gère la perception logique ;
- ne joue aucune animation directement.

#### `UGridMonsterCombatComponent`

- prépare les attaques ;
- reçoit et applique les dégâts ;
- gère la mort ;
- déclenche le butin.

#### `UGridTurnManagerComponent`

- décide quand un participant peut agir ;
- séquence les tours ;
- verrouille les entrées incompatibles ;
- détecte la victoire et la défaite ;
- conserve le ring buffer du journal de combat runtime.

#### `FGridCombatResolver`

- exécute les calculs purs ;
- ne connaît ni le monde ni les animations ;
- est facilement testable avec des Automation Tests.

#### `FGridCombatLogFormatter`

- produit les textes localisables du journal sans lire d’Actor ;
- formate uniquement les résultats déjà calculés ;
- ne relance aucun jet et ne recalcule aucun dégât.

---

## 5. Types fondamentaux

### 5.1. Profils d’IA

```cpp
UENUM(BlueprintType)
enum class EGridMonsterAIProfile : uint8
{
    DirectMelee,
    FastHarasser,
    SlowPressure,
    RangedKeeper,
    Ambush,
    PuzzleLinked
};
```

Un monstre pourra cumuler un profil principal et plusieurs traits :

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly)
EGridMonsterAIProfile PrimaryAIProfile;

UPROPERTY(EditAnywhere, BlueprintReadOnly)
TArray<EGridMonsterAIProfile> AdditionalAIProfiles;
```

Pour le Rat géant :

```text
PrimaryAIProfile      = DirectMelee
AdditionalAIProfiles  = FastHarasser
```

### 5.2. État logique du monstre

```cpp
UENUM(BlueprintType)
enum class EGridMonsterState : uint8
{
    Dormant,
    Idle,
    Alert,
    Pursuing,
    Attacking,
    Repositioning,
    Hurt,
    Dead
};
```

### 5.3. Types d’actions

```cpp
UENUM(BlueprintType)
enum class EGridCombatActionType : uint8
{
    None,
    Move,
    Turn,
    MeleeAttack,
    RangedAttack,
    Ability,
    Defend,
    Wait,
    Retreat,
    Die
};
```

### 5.4. Action préparée

```cpp
USTRUCT(BlueprintType)
struct FGridCombatAction
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGuid ActionId;

    UPROPERTY(BlueprintReadOnly)
    EGridCombatActionType Type = EGridCombatActionType::None;

    UPROPERTY(BlueprintReadOnly)
    FGuid SourceId;

    UPROPERTY(BlueprintReadOnly)
    FGuid TargetActorId;

    UPROPERTY(BlueprintReadOnly)
    FIntPoint TargetCell = FIntPoint::ZeroValue;

    UPROPERTY(BlueprintReadOnly)
    int32 ActionPointCost = 1;

    UPROPERTY(BlueprintReadOnly)
    bool bHit = false;

    UPROPERTY(BlueprintReadOnly)
    int32 RolledDamage = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bOutcomeCommitted = false;
};
```

Cette structure permet de préparer complètement une action avant de lancer son animation.

### 5.5. Phases du combat

```cpp
UENUM(BlueprintType)
enum class EGridCombatPhase : uint8
{
    Exploration,
    StartingCombat,
    PlayerPhase,
    EnemyPhase,
    EndingRound,
    Victory,
    Defeat
};
```

---

## 6. DataAsset de définition d’un monstre

Créer :

```cpp
UGridMonsterDefinitionAsset : public UPrimaryDataAsset
```

L’utilisation de `UPrimaryDataAsset` est recommandée afin de faciliter ultérieurement :

- l’Asset Manager ;
- le chargement asynchrone ;
- les références par identifiant ;
- la création de contenu par les joueurs ;
- la validation centralisée des définitions.

### 6.1. Identité

```cpp
FName MonsterId;
FText DisplayName;
FText Description;
FName CategoryId;
int32 DangerLevel;
```

Pour le rat :

```text
MonsterId    = MON_RatGiant
DisplayName  = Rat géant
CategoryId   = Vermin
DangerLevel  = 1
```

### 6.2. Représentation

```cpp
TSoftObjectPtr<USkeletalMesh> SkeletalMesh;
TSubclassOf<UAnimInstance> AnimationClass;
TSubclassOf<AGridMonsterActor> MonsterActorClass;
FVector VisualScale = FVector::OneVector;
FVector VisualOffset = FVector::ZeroVector;
```

### 6.3. Statistiques de combat

```cpp
int32 MaxHealth;
int32 PhysicalArmor;
int32 MagicalArmor;
int32 Initiative;
int32 Accuracy;
int32 Evasion;
int32 ActionPointsPerTurn;
```

### 6.4. Déplacement et occupation

```cpp
FIntPoint GridFootprint = FIntPoint(1, 1);
float MoveDuration;
float TurnDuration;
bool bBlocksMovement = true;
bool bCanOpenDoors = false;
bool bCanUseTeleporters = false;
```

### 6.5. Perception

```cpp
int32 SightRangeCells;
int32 HearingRangeCells;
int32 AggroPropagationRange;
bool bSharesAggroWithGroup;
```

### 6.6. Profil tactique

```cpp
EGridMonsterAIProfile PrimaryAIProfile;
TArray<EGridMonsterAIProfile> AdditionalAIProfiles;

int32 PreferredMinDistance;
int32 PreferredMaxDistance;
float RetreatChance;
float LowHealthThreshold;
```

### 6.7. Attaques

Créer une structure :

```cpp
USTRUCT(BlueprintType)
struct FGridMonsterAttackDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName AttackId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EGridDamageType DamageType = EGridDamageType::Physical;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EGridPhysicalDamageSubtype PhysicalSubtype =
        EGridPhysicalDamageSubtype::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MinDamage = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxDamage = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 AccuracyBonus = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 RangeCells = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 ActionPointCost = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ImpactNotifyName = TEXT("Monster.AttackImpact");

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<USoundBase> AttackSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UNiagaraSystem> ImpactVFX;
};
```

### 6.8. Résistances et vulnérabilités

Prévoir une structure explicite plutôt que des multiplicateurs dispersés dans le code :

```cpp
USTRUCT(BlueprintType)
struct FGridMonsterDamageModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EGridDamageType DamageType = EGridDamageType::Physical;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EGridPhysicalDamageSubtype PhysicalSubtype =
        EGridPhysicalDamageSubtype::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float DamageMultiplier = 1.0f;
};
```

Exemples :

```text
Feu       : 1,50
Tranchant : 1,25
Poison    : 0,50
Sacré     : 2,00
```

### 6.9. Loot

```cpp
USTRUCT(BlueprintType)
struct FGridMonsterLootEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ItemDefinitionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float DropChance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MinQuantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxQuantity = 1;
};
```

Le DataAsset du monstre contient :

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly)
TArray<FGridMonsterLootEntry> LootTable;
```

---

## 7. Distinction entre dégâts physiques et type d’arme

Le projet possède actuellement :

```cpp
EGridDamageType
{
    Physical,
    Fire,
    Ice,
    Lightning,
    Poison,
    Holy,
    Necrotic,
    Arcane
};
```

Cela ne permet pas encore de représenter la faiblesse du rat aux armes tranchantes.

Il faut ajouter un type séparé :

```cpp
UENUM(BlueprintType)
enum class EGridPhysicalDamageSubtype : uint8
{
    None,
    Slashing,
    Piercing,
    Bludgeoning
};
```

Il ne faut pas remplacer `Physical` dans `EGridDamageType`, car :

- le type général sert à déterminer l’armure physique ;
- le sous-type sert aux vulnérabilités et résistances spécifiques.

Exemple :

```text
Épée :
DamageType      = Physical
PhysicalSubtype = Slashing

Masse :
DamageType      = Physical
PhysicalSubtype = Bludgeoning

Morsure :
DamageType      = Physical
PhysicalSubtype = Piercing
```

Les nouvelles valeurs d’énumérations existantes devront toujours être ajoutées à la fin afin de ne pas modifier les valeurs numériques déjà sérialisées dans les assets.

---

## 8. Statistiques initiales proposées pour le Rat géant

Ces valeurs constituent une base d’équilibrage, et non encore une règle définitive.

| Propriété | Valeur initiale |
|---|---:|
| Points de vie | 8 |
| Armure physique | 0 |
| Armure magique | 0 |
| Initiative | 12 |
| Précision | +2 |
| Esquive | +1 |
| Points d’action par tour | 2 |
| Portée de vue | 5 cases |
| Portée d’ouïe ou d’odorat | 3 cases |
| Taille | 1 × 1 case |
| Déplacement | 1 case pour 1 PA |
| Rotation | gratuite ou 0 PA |
| Expérience | 10 |

### 8.1. Attaque : morsure

```text
AttackId           = Attack_Bite
Dégâts             = 1d4 + 1
Type général       = Physical
Sous-type          = Piercing
Portée             = 1 case
Coût               = 1 PA
Bonus de précision = +0
```

### 8.2. Vulnérabilités

```text
Feu       : dégâts × 1,50
Tranchant : dégâts × 1,25
```

### 8.3. Butin proposé

```text
Key_Iron       : 100 % indépendamment
Item_RatTooth  :  40 % indépendamment
Item_RatMeat   :  80 % indépendamment
```

Ces valeurs ne forment pas une table exclusive. Plusieurs objets peuvent tomber lors de la même mort et leur somme peut dépasser 100 %.

---

## 9. Actor du monstre

### 9.1. Composants

```cpp
UCLASS()
class GRIMROCKPROTOTYPE_API AGridMonsterActor : public AActor
{
    GENERATED_BODY()

public:
    AGridMonsterActor();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> CollisionComponent;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UGridMonsterBehaviorComponent> BehaviorComponent;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UGridMonsterCombatComponent> CombatComponent;
};
```

### 9.2. Données runtime

```cpp
UPROPERTY(BlueprintReadOnly)
FGuid SpawnObjectId;

UPROPERTY(BlueprintReadOnly)
TObjectPtr<UGridMonsterDefinitionAsset> Definition;

UPROPERTY(BlueprintReadOnly)
FIntPoint CurrentCell;

UPROPERTY(BlueprintReadOnly)
EGridEdge Facing;

UPROPERTY(BlueprintReadOnly)
EGridMonsterState MonsterState;

UPROPERTY(BlueprintReadOnly)
int32 CurrentHealth;

UPROPERTY(BlueprintReadOnly)
int32 CurrentPhysicalArmor;

UPROPERTY(BlueprintReadOnly)
int32 CurrentMagicalArmor;

UPROPERTY(BlueprintReadOnly)
int32 RemainingActionPoints;

UPROPERTY(BlueprintReadOnly)
bool bIsMoving;

UPROPERTY(BlueprintReadOnly)
bool bIsExecutingAction;

UPROPERTY(BlueprintReadOnly)
bool bIsDead;
```

### 9.3. API minimale

```cpp
void InitializeMonster(
    const FGridLevelObjectData& SpawnData,
    UGridMonsterDefinitionAsset* MonsterDefinition,
    AGridLevelRuntimeActor* RuntimeActor);

void BeginTurn();
void EndTurn();

bool CanExecuteAction(const FGridCombatAction& Action) const;
void ExecuteAction(const FGridCombatAction& Action);

void StartGridMove(const FIntPoint& Destination);
void StartGridTurn(EGridEdge NewFacing);

void PlayAttack(const FGridCombatAction& Action);
void CommitAttackImpact();

void ApplyDamage(const FGridDamageResult& DamageResult);
void HandleDeath();
```

### 9.4. Tick limité aux transitions visuelles

Le monstre ne doit pas réfléchir à chaque Tick.

Le Tick sert uniquement à :

- interpoler un déplacement ;
- interpoler une rotation ;
- mettre à jour un timer visuel ;
- attendre la fin d’une animation lorsque cela est nécessaire.

Le composant de comportement ne construit ses décisions qu’au début du tour du monstre.

---

## 10. Occupation dynamique des cases

Le niveau connaît actuellement la géométrie statique et les objets du niveau. Il faut lui ajouter une couche d’occupation dynamique.

### 10.1. Registre d’occupation

Créer dans le runtime ou dans un composant dédié :

```cpp
TMap<FIntPoint, FGuid> MonsterOccupancy;
TMap<FIntPoint, FGuid> ReservedMonsterCells;
```

`MonsterOccupancy` représente les positions validées.

`ReservedMonsterCells` représente les destinations déjà choisies par des actions en cours.

### 10.2. Règles

Une case est accessible au monstre si :

1. elle existe ;
2. elle est marchable ;
3. aucun mur ne bloque le passage ;
4. aucune porte fermée ne bloque le passage ;
5. elle n’est pas occupée par un autre monstre ;
6. elle n’est pas réservée par un autre monstre ;
7. elle n’est pas la cellule du groupe, sauf pour calculer une attaque.

Le Rat géant :

- ne peut pas ouvrir une porte ;
- ne peut pas traverser une porte fermée ;
- ne peut pas traverser un autre rat ;
- ne partage pas une case avec un autre monstre dans la première version.

### 10.3. Réservation avant l’animation

Lorsqu’un monstre décide de se déplacer :

```text
Cellule actuelle : (5, 4)
Destination      : (5, 5)
```

la case `(5, 5)` doit être réservée immédiatement.

Cela empêche deux rats d’essayer de s’y déplacer simultanément pendant leurs animations.

### 10.4. Libération des réservations

Une réservation doit être libérée dans tous les cas suivants :

- déplacement terminé ;
- action annulée ;
- mort du monstre ;
- destruction de l’Actor ;
- reconstruction du niveau ;
- changement de niveau ;
- erreur de validation avant déplacement.

Prévoir une fonction centralisée :

```cpp
void ReleaseMonsterReservation(FGuid MonsterId);
```

---

## 11. Pathfinding de grille

### 11.1. Algorithme initial

La grille maximale ne contient que :

```text
32 × 32 = 1 024 cases
```

Un parcours en largeur, ou BFS, est suffisant pour la première version :

- toutes les transitions coûtent une case ;
- le résultat est déterministe ;
- l’algorithme est facile à déboguer ;
- il respecte naturellement les quatre directions.

A* pourra être ajouté plus tard si :

- plusieurs niveaux sont réunis dans une grande grille ;
- les coûts de terrain varient ;
- les monstres possèdent des règles de déplacement complexes.

### 11.2. Ordre déterministe des voisins

Toujours analyser les voisins dans un ordre stable, par exemple :

```text
Nord
Est
Sud
Ouest
```

Sans ordre stable, deux exécutions identiques pourraient produire des chemins visuellement différents.

### 11.3. API proposée

```cpp
struct FGridMonsterPathQuery
{
    FIntPoint Start;
    FIntPoint Goal;
    FGuid MovingMonsterId;
    bool bAllowGoalOccupiedByParty = true;
};

bool FindPath(
    const FGridMonsterPathQuery& Query,
    TArray<FIntPoint>& OutPath) const;
```

Le résultat ne devra pas inclure la case de départ.

### 11.4. But du pathfinding

Pour un monstre de mêlée, la cible du pathfinding ne doit pas nécessairement être la cellule du groupe.

Il est souvent préférable de rechercher une case d’attaque :

```text
Nord du groupe
Est du groupe
Sud du groupe
Ouest du groupe
```

Le pathfinder choisit alors la case d’attaque accessible la plus proche.

Cette règle évite d’autoriser implicitement le déplacement d’un monstre sur la case occupée par le groupe.

---

## 12. Système de combat tour par tour

### 12.1. Modèle recommandé pour la première version

Le système de règles prévoit déjà :

- une attaque principale ;
- des capacités actives ;
- des sorts ;
- l’utilisation d’objets ;
- la défense ;
- l’attente.

Pour éviter de construire immédiatement un système d’initiative trop complexe, la première version utilisera des manches divisées en phases.

```text
Début de la manche
    ↓
Phase du joueur
    ↓
Phase des monstres
    ↓
Fin de la manche
```

### 12.2. Phase du joueur

Pendant la phase du joueur :

- chaque personnage vivant peut accomplir une action de combat ;
- le joueur choisit l’ordre de ses personnages ;
- le groupe peut effectuer un déplacement collectif ;
- le groupe peut effectuer une rotation collective ;
- le joueur peut terminer volontairement sa phase.

Le déplacement ou la rotation du groupe ne doit pas remplacer l’action personnelle d’un personnage, sauf si l’équilibrage ultérieur le justifie.

### 12.3. Phase des monstres

Pendant la phase des monstres :

1. les monstres vivants sont triés par initiative ;
2. chaque monstre reçoit ses points d’action ;
3. son composant de comportement prépare une liste d’actions ;
4. les actions sont exécutées séquentiellement ;
5. le monstre termine son tour ;
6. le gestionnaire passe au monstre suivant.

### 12.4. Initiative du premier camp

Au début d’un combat :

```text
Initiative du groupe
contre
Initiative du groupe ennemi
```

Une formule initiale simple peut être utilisée :

```text
Initiative du groupe =
moyenne des initiatives des personnages vivants + D20

Initiative ennemie =
initiative la plus élevée des monstres + D20
```

Le camp gagnant agit en premier.

L’architecture devra néanmoins permettre plus tard un véritable ordre individuel mélangeant :

- personnages ;
- monstres ;
- invocations ;
- objets actifs.

### 12.5. Début du combat

Le combat commence lorsqu’au moins un monstre hostile :

- voit le groupe ;
- entend le groupe ;
- subit des dégâts ;
- est activé par un trigger ;
- reçoit une commande d’agression ;
- rejoint un groupe de rencontre déjà en combat.

Le gestionnaire doit alors :

1. figer les commandes d’exploration en cours ;
2. vider le buffer d’entrée ;
3. construire la liste des participants ;
4. déterminer le camp qui commence ;
5. afficher l’interface de combat ;
6. démarrer la première phase.

### 12.6. Fin du combat

Le combat prend fin lorsque :

- tous les monstres hostiles de la rencontre sont morts ;
- tous les monstres ont fui ou sont désactivés ;
- tous les personnages sont incapables de combattre ;
- un événement spécial met fin à la rencontre.

Après une victoire :

- terminer toutes les animations en cours ;
- distribuer l’expérience ;
- générer ou rendre accessibles les butins ;
- émettre `CombatEnded` ;
- revenir à `Exploration`.

---

## 13. Gestionnaire central des tours

Ajouter à `AGridLevelRuntimeActor` :

```cpp
UPROPERTY(VisibleAnywhere)
TObjectPtr<UGridTurnManagerComponent> TurnManagerComponent;
```

### 13.1. Responsabilités

`UGridTurnManagerComponent` devra :

- détecter ou recevoir le début du combat ;
- verrouiller les entrées incompatibles ;
- construire la liste des participants ;
- commencer une manche ;
- gérer la phase joueur ;
- gérer la phase ennemie ;
- lancer les tours des monstres ;
- attendre la fin des animations ;
- détecter la victoire ;
- détecter la défaite ;
- revenir à l’exploration ;
- émettre les événements correspondants.

### 13.2. File d’actions

Le gestionnaire doit exécuter une seule action animée à la fois.

```cpp
TArray<FGridCombatAction> PendingActions;
TOptional<FGridCombatAction> ActiveAction;
```

Séquence :

```text
Ajouter l’action
    ↓
Valider l’action
    ↓
Passer l’action en ActiveAction
    ↓
Lancer l’animation
    ↓
Appliquer l’impact au Notify
    ↓
Recevoir ActionComplete
    ↓
Retirer l’action
    ↓
Passer à la suivante
```

### 13.3. Gestion des entrées

Le Pawn possède déjà :

- un buffer d’entrée ;
- des commandes de déplacement ;
- des commandes de rotation ;
- une notion d’état occupé.

Au début d’un combat :

- vider le buffer d’entrée ;
- interdire le déplacement pendant la phase ennemie ;
- interdire les déplacements pendant une animation en cours ;
- autoriser uniquement les commandes correspondant à la phase actuelle.

### 13.4. Gestion des timers de sécurité

Chaque action animée doit posséder une durée maximale autorisée.

```cpp
float ExpectedDuration;
float TimeoutDuration;
```

Le timeout peut être calculé ainsi :

```text
TimeoutDuration = ExpectedDuration + 0,5 seconde
```

À l’expiration :

- appliquer l’impact s’il n’a pas encore été appliqué et si l’action le permet ;
- terminer l’action ;
- produire un log d’erreur ;
- poursuivre la file des tours.

---

## 14. Résolution des attaques

Les règles actuelles définissent :

```text
Jet d’attaque =
D20 + précision + bonus d’arme + modificateur de caractéristique

Défense =
10 + esquive + armure légère + bouclier

Dégâts =
dégâts de l’arme + modificateur de caractéristique + bonus
```

Les dégâts réduisent ensuite l’armure correspondante, puis les points de vie.

### 14.1. Résolveur commun

Créer un service C++ commun :

```cpp
class FGridCombatResolver
{
public:
    static FGridAttackResult ResolveAttack(
        const FGridAttackSourceStats& Source,
        const FGridAttackTargetStats& Target,
        const FGridAttackDefinition& Attack,
        FRandomStream& RandomStream);
};
```

Le même résolveur devra être utilisable par :

- les personnages ;
- les monstres ;
- les pièges ;
- les projectiles ;
- les surfaces.

### 14.2. Résultat d’attaque

```cpp
USTRUCT(BlueprintType)
struct FGridAttackResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bHit = false;

    UPROPERTY(BlueprintReadOnly)
    bool bCriticalHit = false;

    UPROPERTY(BlueprintReadOnly)
    int32 AttackRoll = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DefenseValue = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 RawDamage = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DamageAfterResistance = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 PhysicalArmorDamage = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 MagicalArmorDamage = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 HealthDamage = 0;
};
```

### 14.3. Calcul d’une vulnérabilité

Ordre recommandé :

```text
Dégâts bruts
    ↓
Bonus ou malus de l’attaque
    ↓
Multiplicateur de type général
    ↓
Multiplicateur du sous-type physique
    ↓
Résistance finale de la cible
    ↓
Armure physique ou magique
    ↓
Points de vie
```

Exemple :

```text
Épée infligeant 8 dégâts physiques tranchants
Rat vulnérable au tranchant × 1,25

8 × 1,25 = 10 dégâts avant armure
```

### 14.4. Choix de la cible dans le groupe

Le groupe occupe une seule cellule, mais contient jusqu’à six personnages.

Le Rat géant privilégiera :

1. les personnages vivants de la première ligne ;
2. une cible choisie aléatoirement parmi eux ;
3. à défaut, un personnage vivant de la seconde ligne.

Une pondération future pourra prendre en compte :

- la menace ;
- les blessures ;
- l’armure ;
- une provocation ;
- la proximité dans la formation ;
- les effets de camouflage.

### 14.5. Générateur aléatoire déterministe

Utiliser `FRandomStream` plutôt que les fonctions aléatoires globales.

Chaque rencontre peut posséder une graine :

```cpp
int32 EncounterRandomSeed;
FRandomStream CombatRandomStream;
```

Cela permet :

- de reproduire un bug ;
- d’écrire des tests stables ;
- de conserver la cohérence après sauvegarde ;
- de rejouer une séquence dans les outils de debug.

---

## 15. Intelligence artificielle du Rat géant

### 15.1. Perception

Pour la première version, utiliser une perception logique de grille plutôt que `UAIPerceptionComponent`.

Dans ce prototype tour par tour, une perception discrète est plus appropriée :

```text
Vue :
- 5 cases ;
- ligne droite ou ligne de vue calculée ;
- bloquée par les murs et portes fermées.

Ouïe / odorat :
- 3 cases ;
- peut fonctionner autour d’un angle ;
- ne nécessite pas de visibilité directe.

Agression forcée :
- le rat subit des dégâts ;
- un trigger l’active ;
- un membre de son groupe devient agressif.
```

### 15.2. États comportementaux

```text
Dormant
    ↓
Idle
    ↓
Alert
    ↓
Pursuing
    ↓
Attacking
    ↓
Repositioning
    ↓
Pursuing
```

Tout état peut rejoindre :

```text
Hurt
Dead
```

### 15.3. Algorithme d’un tour

Pseudo-code :

```cpp
void UGridMonsterBehaviorComponent::BuildTurnActions(
    TArray<FGridCombatAction>& OutActions)
{
    if (!OwnerMonster || OwnerMonster->IsDead())
    {
        return;
    }

    int32 RemainingAP = OwnerMonster->GetRemainingActionPoints();

    while (RemainingAP > 0)
    {
        const int32 Distance = GetDistanceToParty();

        if (Distance == 1 && CanBiteParty())
        {
            OutActions.Add(BuildBiteAction());
            RemainingAP -= BiteActionCost;

            if (RemainingAP > 0 && ShouldRepositionAfterAttack())
            {
                if (TOptional<FIntPoint> RetreatCell =
                    FindBestRetreatCell())
                {
                    OutActions.Add(
                        BuildMoveAction(RetreatCell.GetValue()));
                    RemainingAP -= 1;
                }
            }

            break;
        }

        if (TOptional<FIntPoint> NextCell = FindNextPursuitCell())
        {
            OutActions.Add(BuildMoveAction(NextCell.GetValue()));
            RemainingAP -= 1;
            continue;
        }

        OutActions.Add(BuildWaitAction());
        break;
    }
}
```

### 15.4. Comportement `DirectMelee`

Si le rat n’est pas adjacent :

- trouver le chemin le plus court ;
- avancer vers le groupe ;
- attaquer s’il lui reste suffisamment de PA après son déplacement.

Exemple avec 2 PA :

```text
Distance 1 :
Morsure + repositionnement éventuel

Distance 2 :
Déplacement + morsure

Distance 3 ou plus :
Déplacement + déplacement
```

### 15.5. Comportement `FastHarasser`

Après une morsure, le rat tente de quitter une situation trop exposée.

Une case de repli reçoit un score selon :

```text
+ distance supplémentaire par rapport au groupe
+ présence d’une deuxième issue
+ possibilité de poursuivre le combat au tour suivant
- cul-de-sac
- proximité d’un autre rat bloquant
- case réservée
- surface dangereuse
```

Le rat ne doit cependant pas se replier systématiquement. Pour un danger de niveau 1, une valeur initiale raisonnable serait :

```text
RetreatChance = 40 %
```

Il reste au contact si :

- aucune case sûre n’existe ;
- il se trouve dans un cul-de-sac ;
- son déplacement bloquerait un allié ;
- un autre comportement prioritaire s’applique.

### 15.6. Dernière position connue

Si le rat perd la perception du groupe :

1. il se dirige vers la dernière cellule connue ;
2. il attend un tour ;
3. il revient à `Idle` s’il ne retrouve aucune cible.

### 15.7. Aggression de groupe

Chaque placement peut avoir :

```cpp
FName EncounterGroupId;
```

Lorsqu’un rat devient agressif :

- les rats du même groupe ;
- situés à une distance configurable ;
- et capables de recevoir l’alerte ;

passent également à l’état `Alert`.

### 15.8. Gestion des portes

Le Rat géant ne peut pas ouvrir une porte.

Comportement initial :

- porte ouverte : passage autorisé ;
- porte fermée : passage bloqué ;
- porte qui se ferme pendant un déplacement réservé : annuler le déplacement et libérer la réservation ;
- porte ouverte par un événement : recalculer le chemin au prochain tour.

---

## 16. Pipeline des animations

L’ArtBook demande que chaque créature soit immédiatement reconnaissable dans un couloir sombre, de face, de profil ou de trois-quarts. La silhouette doit primer sur les petits détails.

### 16.1. Structure Content

```text
Content/Grimrock/Monsters/RatGiant/
├── Meshes/
│   ├── SK_MON_RatGiant
│   ├── SKEL_MON_RatGiant
│   └── PHYS_MON_RatGiant
│
├── Materials/
│   ├── M_MON_RatGiant_Body
│   └── MI_MON_RatGiant_Body
│
├── Textures/
│   ├── T_MON_RatGiant_BaseColor
│   ├── T_MON_RatGiant_Normal
│   ├── T_MON_RatGiant_ORM
│   └── T_MON_RatGiant_Opacity
│
├── Animations/
│   ├── A_MON_RatGiant_Idle_A
│   ├── A_MON_RatGiant_Idle_Sniff
│   ├── A_MON_RatGiant_Alert
│   ├── A_MON_RatGiant_Move_Fwd
│   ├── A_MON_RatGiant_Turn_L
│   ├── A_MON_RatGiant_Turn_R
│   ├── A_MON_RatGiant_Attack_Bite
│   ├── A_MON_RatGiant_Hurt
│   └── A_MON_RatGiant_Death
│
├── Montages/
│   ├── AM_MON_RatGiant_Attack_Bite
│   ├── AM_MON_RatGiant_Hurt
│   └── AM_MON_RatGiant_Death
│
├── Animation/
│   └── ABP_MON_RatGiant
│
├── Blueprints/
│   └── BP_MON_RatGiant
│
├── Data/
│   └── DA_MON_RatGiant
│
├── Audio/
│   ├── S_MON_RatGiant_Idle
│   ├── S_MON_RatGiant_Alert
│   ├── S_MON_RatGiant_Bite
│   ├── S_MON_RatGiant_Hurt
│   └── S_MON_RatGiant_Death
│
└── VFX/
    └── NS_MON_RatGiant_Hit
```

### 16.2. Squelette recommandé

Le squelette devra au minimum comporter :

```text
root
└── pelvis
    ├── spine_01
    │   ├── spine_02
    │   │   ├── neck
    │   │   │   ├── head
    │   │   │   │   └── jaw
    │   │   ├── front_leg_l
    │   │   │   └── front_paw_l
    │   │   └── front_leg_r
    │   │       └── front_paw_r
    │   ├── back_leg_l
    │   │   └── back_paw_l
    │   └── back_leg_r
    │       └── back_paw_r
    │
    └── tail_01
        └── tail_02
            └── tail_03
                └── tail_04
```

Règles d’importation :

```text
Unité        : centimètre
Axe avant    : +X
Axe vertical : +Z
Root         : au centre du sol
Transforms   : appliquées avant export
Animation    : sur place
```

### 16.3. Animations minimales

| Animation | Durée recommandée | Usage |
|---|---:|---|
| Idle A | 1,5 à 2,5 s | respiration, petits mouvements |
| Idle Sniff | 1 à 1,5 s | variation d’attente |
| Alert | 0,4 à 0,6 s | perception du groupe |
| Move Forward | 0,28 à 0,36 s | une case |
| Turn Left | 0,12 à 0,18 s | rotation de 90° |
| Turn Right | 0,12 à 0,18 s | rotation de 90° |
| Bite | 0,5 à 0,65 s | attaque principale |
| Hurt | 0,3 à 0,45 s | réception d’un coup |
| Death | 0,9 à 1,3 s | mort |

Une animation de recul dédiée pourra être ajoutée plus tard. La première version peut réutiliser l’animation de déplacement avec une vitesse ou une direction adaptée.

### 16.4. Contraintes de lisibilité

L’animation du rat doit rester lisible :

- à une case de distance ;
- dans un couloir étroit ;
- sous une lumière faible ;
- derrière une grille ;
- avec une caméra en vue subjective.

L’attaque doit donc posséder :

- une anticipation visible ;
- une ouverture nette de la gueule ;
- un impact lisible ;
- un retour suffisamment court pour ne pas ralentir les tours.

---

## 17. Animation Blueprint

### 17.1. Classe C++ de base

Créer :

```cpp
UCLASS(Transient, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridMonsterAnimInstance
    : public UAnimInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category="Monster")
    EGridMonsterState MonsterState;

    UPROPERTY(BlueprintReadOnly, Category="Monster")
    bool bIsMoving = false;

    UPROPERTY(BlueprintReadOnly, Category="Monster")
    bool bIsTurning = false;

    UPROPERTY(BlueprintReadOnly, Category="Monster")
    bool bIsDead = false;

    UPROPERTY(BlueprintReadOnly, Category="Monster")
    float MoveAlpha = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Monster")
    int32 TurnDirection = 0;

    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
```

Cette classe récupère son `AGridMonsterActor` propriétaire et copie les variables nécessaires à l’Animation Blueprint.

### 17.2. State Machine de locomotion

State Machine proposée :

```text
Idle
 ├── Move
 ├── TurnLeft
 ├── TurnRight
 └── Dead
```

Transitions :

```text
Idle → Move       si bIsMoving
Move → Idle       si !bIsMoving

Idle → TurnLeft   si TurnDirection < 0
Idle → TurnRight  si TurnDirection > 0

Tout état → Dead  si bIsDead
```

### 17.3. Montages

Utiliser des Animation Montages pour :

- morsure ;
- réaction aux dégâts ;
- mort ;
- capacités spéciales futures.

La locomotion reste dans la State Machine.

### 17.4. Priorités des Montages

Ordre de priorité recommandé :

```text
Death
Hurt
Attack
Locomotion
Idle variation
```

Une animation de mort ne doit jamais être interrompue par Hurt ou Attack.

Une animation Hurt ne doit pas relancer une action déjà annulée par la mort.

---

## 18. Animation Notifies

### 18.1. Notifies du Rat géant

#### Morsure

```text
Monster.AttackWhoosh
Monster.AttackImpact
Monster.ActionComplete
```

#### Marche

```text
Monster.FootstepFront
Monster.FootstepBack
```

#### Mort

```text
Monster.DeathImpact
Monster.ActionComplete
```

### 18.2. Position de l’impact

Dans l’animation de morsure :

```text
0 %        : préparation
20 %       : recul de la tête
45 à 55 %  : fermeture des mâchoires
100 %      : retour
```

Le Notify `Monster.AttackImpact` doit être placé au moment de la fermeture des mâchoires.

### 18.3. Sécurité contre les blocages

Une animation ou un Montage mal configuré ne doit jamais bloquer le gestionnaire des tours.

Prévoir trois moyens de terminer une action :

1. `Monster.ActionComplete` ;
2. événement de fin du Montage ;
3. timer de sécurité basé sur la durée maximale attendue.

L’impact doit également être protégé :

```cpp
if (!CurrentAction.bOutcomeCommitted)
{
    CommitCurrentActionOutcome();
}
```

Ainsi, un Notify dupliqué ne peut pas infliger deux fois les dégâts.

### 18.4. Validation de la cible au moment de l’impact

L’action est calculée avant l’animation, mais le système doit encore vérifier que la cible existe au moment de l’impact.

Cas possibles :

- la cible est toujours valide : appliquer le résultat ;
- la cible est morte entre-temps : annuler l’impact ;
- le combat est terminé : annuler l’impact ;
- l’Actor visuel manque mais les données de personnage existent : appliquer les données sans VFX.

---

## 19. Déplacement visuel du monstre

### 19.1. Début du déplacement

Lorsqu’une action Move commence :

1. valider la cellule ;
2. réserver la destination ;
3. conserver la cellule de départ ;
4. définir la cellule logique cible ;
5. lancer l’animation Move ;
6. interpoler l’Actor.

```cpp
MoveStartWorld = GetActorLocation();
MoveTargetWorld = RuntimeActor->GetCellCenterWorld(TargetX, TargetY);
MoveElapsed = 0.0f;
bIsMoving = true;
```

### 19.2. Mise à jour

```cpp
MoveElapsed += DeltaSeconds;

const float Alpha =
    FMath::Clamp(MoveElapsed / MoveDuration, 0.0f, 1.0f);

const float SmoothedAlpha =
    FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

SetActorLocation(
    FMath::Lerp(MoveStartWorld, MoveTargetWorld, SmoothedAlpha));
```

### 19.3. Fin du déplacement

À la fin :

```cpp
SetActorLocation(MoveTargetWorld);
CurrentCell = TargetCell;
ReleaseOldOccupancy();
CommitReservedCell();
bIsMoving = false;
```

Ne jamais déduire la cellule finale à partir de la position flottante de l’Actor.

### 19.4. Rotation

La rotation reste discrète :

```text
Nord
Est
Sud
Ouest
```

Une rotation de 90° est interpolée visuellement, puis recalée exactement sur l’angle final.

```cpp
TurnTargetYaw = GridDirectionToYaw(NewFacing);
```

---

## 20. Intégration dans l’éditeur de niveau

### 20.1. Données supplémentaires de placement

Ajouter à `FGridLevelObjectData` :

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Monster")
TObjectPtr<UGridMonsterDefinitionAsset> MonsterDefinitionAsset;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Monster")
FName MonsterDefinitionId = NAME_None;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Monster")
EGridEdge InitialFacing = EGridEdge::North;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Monster")
FName EncounterGroupId = NAME_None;
```

`LocalYaw` pourrait être réutilisé, mais une orientation discrète est préférable pour un gameplay sur grille.

### 20.2. Palette

Ajouter une entrée :

```text
Id            : MON_RatGiant
Type          : MonsterSpawn
Category      : Monsters/Vermin
Definition    : DA_MON_RatGiant
Preview       : icône ou mesh du rat
PlacementKind : Center
```

### 20.3. Inspecteur

Lorsque le type est `MonsterSpawn`, afficher :

- Monster Definition ;
- Initial Facing ;
- Initially Enabled ;
- Encounter Group ;
- Activation Mode ;
- Notes ;
- liens sortants ;
- liens entrants.

### 20.4. Spawn runtime

Dans le runtime :

```cpp
void AGridLevelRuntimeActor::AddMonsterSpawn(
    const FGridLevelObjectData& ObjectData)
{
    UGridMonsterDefinitionAsset* Definition =
        ResolveMonsterDefinition(ObjectData);

    if (!Definition)
    {
        UE_LOG(LogGridMonster, Error, TEXT("Missing monster definition"));
        return;
    }

    AGridMonsterActor* Monster = SpawnMonsterActor(...);
    Monster->InitializeMonster(ObjectData, Definition, this);

    SpawnedMonsterActors.Add(ObjectData.ObjectId, Monster);
    RegisterMonsterOccupancy(Monster);
}
```

### 20.5. Aperçu dans l’éditeur

Pour éviter de faire tourner l’IA dans l’éditeur :

- afficher le mesh ou une icône de prévisualisation ;
- ne pas créer le composant de combat actif ;
- ne pas lancer l’Animation Blueprint si cela nuit aux performances ;
- afficher l’orientation avec une flèche ;
- afficher le groupe de rencontre en surimpression de debug.

---

## 21. Événements et liens logiques

Le système actuel repose sur :

```text
Objet source + événement
    →
Objet cible + commande
```

Les objets ne doivent pas se connaître directement ; ils émettent des événements que le système central transforme en commandes.

### 21.1. Nouvelles commandes

Ajouter à la fin de `EGridObjectCommand` :

```cpp
SpawnMonster,
DespawnMonster,
WakeMonster,
SetMonsterAggressive
```

Une autre possibilité consiste à réutiliser les commandes générales existantes :

```text
Spawn
Despawn
Enable
Disable
Activate
```

La réutilisation est préférable si la sémantique reste suffisamment claire et si les cibles refusent proprement les commandes non supportées.

### 21.2. Nouveaux événements

Ajouter à la fin de `EGridObjectEvent` :

```cpp
MonsterSpawned,
MonsterAggroed,
MonsterDamaged,
MonsterDied,
CombatStarted,
CombatEnded
```

Exemples :

```text
Rat_01.MonsterDied
    → Door_Secret_01.Open

Trigger_01.Entered
    → RatSpawner_01.Spawn

Lever_01.Activated
    → Rat_01.SetMonsterAggressive
```

L’`ObjectId` du placement demeure l’identité stable du monstre pendant la partie.

### 21.3. Événements de rencontre

Prévoir également la possibilité d’émettre :

```text
EncounterStarted
EncounterCompleted
```

Ces événements sont plus pratiques que `MonsterDied` lorsqu’une porte doit s’ouvrir après la mort de tous les monstres d’un groupe.

---

## 22. Sauvegarde de l’état des monstres

Le runtime sauvegarde actuellement les portes, objets interactifs, objets placés, items et réceptacles. Il faut ajouter les monstres.

### 22.1. Structure de sauvegarde

```cpp
USTRUCT(BlueprintType)
struct FGridRuntimeMonsterState
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid ObjectId;

    UPROPERTY(SaveGame)
    FName MonsterDefinitionId;

    UPROPERTY(SaveGame)
    int32 CellX = INDEX_NONE;

    UPROPERTY(SaveGame)
    int32 CellY = INDEX_NONE;

    UPROPERTY(SaveGame)
    EGridEdge Facing = EGridEdge::North;

    UPROPERTY(SaveGame)
    int32 CurrentHealth = 0;

    UPROPERTY(SaveGame)
    int32 CurrentPhysicalArmor = 0;

    UPROPERTY(SaveGame)
    int32 CurrentMagicalArmor = 0;

    UPROPERTY(SaveGame)
    bool bSpawned = true;

    UPROPERTY(SaveGame)
    bool bDead = false;

    UPROPERTY(SaveGame)
    EGridMonsterState AIState = EGridMonsterState::Idle;

    UPROPERTY(SaveGame)
    FIntPoint LastKnownPartyCell = FIntPoint::ZeroValue;

    UPROPERTY(SaveGame)
    int32 RandomSeed = 0;
};
```

Dans `FGridLevelRuntimeState` :

```cpp
UPROPERTY(SaveGame, BlueprintReadWrite)
TMap<FGuid, FGridRuntimeMonsterState> Monsters;
```

### 22.2. Capture

L’état doit être capturé :

- avant un changement de niveau ;
- avant une sauvegarde ;
- avant une reconstruction du runtime ;
- éventuellement après chaque fin de combat.

### 22.3. Restauration

Lors du chargement :

- un monstre mort ne doit pas réapparaître ;
- un monstre déplacé doit revenir sur sa dernière cellule ;
- ses points de vie doivent être restaurés ;
- son état d’agression peut être conservé ;
- ses effets actifs devront être réhydratés ultérieurement.

### 22.4. Sauvegarde pendant une animation

Ne pas sauvegarder une position interpolée.

Si une sauvegarde est demandée pendant une action :

- soit attendre la fin de l’action ;
- soit enregistrer la cellule logique validée ;
- soit annuler visuellement l’interpolation et recaler le monstre.

Pour la première version, il est recommandé d’interdire la sauvegarde pendant la phase ennemie ou pendant une action animée.

---

## 23. Mort, corps et butin

### 23.1. Séquence de mort

```text
Dégâts appliqués
    ↓
PV <= 0
    ↓
État logique Dead
    ↓
Retrait immédiat du combat et de l’occupation
    ↓
Lecture du Montage de mort
    ↓
Émission MonsterDied
    ↓
Génération du butin
    ↓
Corps visuel ou disparition
```

Le retrait logique doit être immédiat afin que :

- la cellule ne bloque plus les autres participants ;
- le tour du monstre soit annulé ;
- la victoire puisse être détectée.

Le mesh peut rester visible jusqu’à la fin de l’animation.

### 23.2. Corps

Première version recommandée :

- le corps ne bloque pas le déplacement ;
- il reste visible ;
- il n’utilise pas de ragdoll ;
- il peut disparaître après un délai ou au changement de niveau.

Le ragdoll pourra être ajouté ultérieurement, mais il ne devra jamais modifier l’occupation logique de la grille.

### 23.3. Butin

À la mort :

1. évaluer indépendamment chaque entrée de la table de butin ;
2. collecter toutes les entrées réussies ;
3. générer un `FGridItemInstance` par résultat ;
4. placer chaque item sur la cellule du monstre avec un offset déterministe ;
5. enregistrer chaque item dans le runtime du niveau.

Le butin doit utiliser le système d’items existant, sans inventaire spécifique aux monstres dans la première version.

Chaque entrée possède une chance individuelle comprise entre 0 et 1. Un drop à `1.0` est garanti, un drop à `0.4` possède 40 % de chance indépendamment des autres, et la somme de la table peut dépasser 1. Le sous-seed déterministe associe la graine du monstre à l’`ItemDefinitionId` : réordonner la table ou ajouter une entrée ne modifie pas les jets existants. L’échec de placement d’un objet n’arrête pas les autres résultats.

Chaque objet placé reçoit un `RuntimeObjectId` distinct. Les items au sol sont persistés séparément dans des `FGridRuntimeItemState`. Restaurer un monstre mort conserve `bLootGenerated=true` et ne rejoue jamais les jets.

### 23.4. Expérience

L’expérience est attribuée à la fin de la rencontre, et non nécessairement à chaque mort individuelle.

Cela évite :

- les doubles attributions ;
- les incohérences si un monstre est ressuscité ;
- la distribution avant la fin d’une phase ;
- la dispersion de la logique de progression.

---

## 24. Sons et effets visuels

### 24.1. Sons minimaux

Le Rat géant devra posséder :

- respiration ou petits cris d’attente ;
- cri d’alerte ;
- bruit de course ou de griffes ;
- morsure ;
- réception d’un coup ;
- mort.

Les sons d’attente doivent être espacés et légèrement aléatoires pour éviter la répétition.

### 24.2. VFX minimaux

Pour la première version :

- petit effet de sang à l’impact ;
- nombre de dégâts dans l’interface ;
- éventuel flash sur le personnage touché ;
- effet de feu spécifique en cas de dégâts de feu.

Les VFX doivent être déclenchés au même moment que `Monster.AttackImpact` ou que l’impact reçu.

### 24.3. Séparation audio et gameplay

L’absence d’un son ou d’un VFX ne doit jamais empêcher :

- l’application des dégâts ;
- la fin d’une action ;
- la poursuite du tour ;
- la sauvegarde.

---

## 25. Interface utilisateur de combat

Le premier prototype doit au minimum afficher :

- la phase actuelle ;
- le numéro de la manche ;
- le nom du participant actif ;
- les personnages ayant déjà agi ;
- un bouton ou une commande pour terminer la phase joueur ;
- les dégâts et les échecs ;
- la victoire ou la défaite.

### 25.1. Feedback de cible

Quand le joueur prépare une attaque :

- mettre en évidence le monstre ciblé ;
- afficher si la portée est valide ;
- afficher le type d’attaque ;
- ne pas dévoiler obligatoirement toutes les résistances avant identification.

### 25.2. Journal de combat

Le journal de combat peut afficher :

```text
Aëlric attaque le Rat géant.
Jet d’attaque : 17 contre Défense 12 — Réussite.
Dégâts : 6 tranchants, vulnérabilité comprise.
Le Rat géant subit 6 dégâts.
```

MON10.1 fournit ce journal runtime structuré. Une
`FGridCombatLogEntry` représente une entrée et
`EGridCombatLogEntryType` distingue dix types d’événements. Le
`FGridCombatLogFormatter` produit les textes localisables.

`UGridTurnManagerComponent` conserve les entrées dans un ring buffer dont
`MaxCombatLogEntries` vaut 128 par défaut. Le delegate Blueprint
`OnCombatLogEntryAdded` permet à un futur widget UMG de recevoir chaque
nouvelle entrée.

Le journal est alimenté par le `FGridAttackResult` déjà produit par le
résolveur, et non par les animations. `FGridAttackResult` reste la source de
vérité : aucun jet et aucun dégât ne sont recalculés pour le feedback.

Le journal décrit la rencontre courante et n’est pas sauvegardé. Aucun widget
UMG final n’est encore créé par MON10.1.

---

## 26. Journalisation et outils de debug

Créer des catégories distinctes :

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogGridCombat, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogGridMonster, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogGridAI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogGridAnimation, Log, All);
```

Exemples :

```text
[GridCombat] Round 3 started
[GridCombat] Enemy phase started

[GridAI] Rat_01 AP=2 DistanceToParty=2
[GridAI] Rat_01 selected Move (4,5) -> (4,6)
[GridAI] Rat_01 selected Bite target Character[1]

[GridAnimation] Rat_01 montage Attack_Bite started
[GridAnimation] Rat_01 AttackImpact notify
[GridCombat] Rat_01 hit Character[1] for 4 physical damage

[GridMonster] Rat_01 died at cell (4,6)
```

### 26.1. Overlay de debug

Afficher au-dessus d’un monstre :

```text
Rat_01
State: Pursuing
Cell: 4,6
HP: 6/8
AP: 1/2
Target: Party
```

Afficher éventuellement sur la grille :

- chemin retenu ;
- cases occupées ;
- cases réservées ;
- portée de perception ;
- destination choisie ;
- score des cases de repli.

### 26.2. Commandes de debug utiles

Prévoir des fonctions `CallInEditor` ou BlueprintCallable :

```text
Spawn Test Rat
Kill Selected Monster
Wake All Monsters
Start Combat
End Combat
Log Monster Occupancy
Log Current Turn State
Show Monster Paths
```

---

## 27. Tests automatisés

### 27.1. Tests unitaires — pathfinding

- chemin direct ;
- mur bloquant ;
- porte fermée ;
- porte ouverte ;
- cul-de-sac ;
- case occupée ;
- case réservée ;
- absence de chemin ;
- ordre déterministe des voisins.

### 27.2. Tests unitaires — occupation

- deux monstres ne peuvent pas occuper la même case ;
- une réservation empêche une seconde réservation ;
- une mort libère la case ;
- une annulation libère la réservation ;
- une reconstruction vide les registres ;
- un changement de niveau ne conserve aucune réservation transitoire.

### 27.3. Tests unitaires — IA du rat

- à une case, il attaque ;
- à deux cases, il avance puis attaque ;
- à trois cases, il avance deux fois ;
- après une attaque, il se replie si une case sûre existe ;
- il ne se replie pas dans un cul-de-sac ;
- il ne traverse pas une porte fermée ;
- il rejoint la dernière position connue ;
- il partage correctement l’agression avec son groupe.

### 27.4. Tests unitaires — combat

- attaque réussie ;
- attaque manquée ;
- dégâts appliqués une seule fois ;
- vulnérabilité au feu ;
- vulnérabilité au tranchant ;
- armure physique avant les points de vie ;
- mort et émission de l’événement ;
- cible morte avant l’impact ;
- action terminée même sans Notify.

### 27.5. Tests unitaires — sauvegarde

- rat vivant et blessé ;
- rat déplacé ;
- rat agressif ;
- rat mort ;
- plusieurs drops indépendants ;
- drop garanti ;
- somme des chances supérieure à 1 ;
- stabilité des jets par `ItemDefinitionId` après réordonnancement ou ajout ;
- butin généré et items au sol persistés séparément ;
- absence de duplication du butin après restauration ;
- changement de niveau puis retour ;
- restauration de la graine aléatoire.

### 27.6. Tests fonctionnels

Créer :

```text
Content/Grimrock/Dev/Tests/Combat/L_CombatRatTest
```

Préparer plusieurs zones :

1. couloir droit ;
2. angle à 90° ;
3. porte fermée ;
4. cul-de-sac ;
5. deux rats dans un même groupe ;
6. rat devant une plaque de pression ;
7. rat activé par un trigger ;
8. sauvegarde et rechargement.

---

## 28. Critères de validation du premier prototype

Le jalon Rat géant sera considéré comme validé lorsque :

1. le Rat géant peut être placé dans l’éditeur de grille ;
2. son placement est sauvegardé dans le `UGridLevelAsset` ;
3. il apparaît au lancement du runtime ;
4. il possède un Skeletal Mesh et une animation Idle ;
5. il détecte le groupe ;
6. il passe de Idle à Alert puis Pursuing ;
7. il trouve un chemin respectant murs, portes et occupation ;
8. il se déplace exactement de centre de case à centre de case ;
9. il ne partage jamais une case avec un autre monstre ;
10. il attaque le groupe à distance 1 ;
11. l’impact correspond au Notify de morsure ;
12. les dégâts ne sont appliqués qu’une fois ;
13. il peut tenter de se repositionner après l’attaque ;
14. il reçoit des dégâts ;
15. le feu et le tranchant appliquent leurs vulnérabilités ;
16. il joue une réaction aux dégâts ;
17. il meurt correctement ;
18. sa case est libérée ;
19. son événement `MonsterDied` est émis ;
20. son butin est placé dans le monde ;
21. son état persiste après une sauvegarde ;
22. le gestionnaire revient correctement à l’exploration après la victoire.

---

## 29. Ordre d’implémentation recommandé

### Étape MON1 — Types et DataAsset

Créer :

- `GridMonsterTypes.h` ;
- `GridCombatTypes.h` ;
- `UGridMonsterDefinitionAsset` ;
- `DA_MON_RatGiant`.

Aucun déplacement et aucune animation à ce stade.

**Validation :** le DataAsset compile, s’ouvre dans l’éditeur et expose toutes les propriétés prévues.

### Étape MON2 — Actor animé

Créer :

- `AGridMonsterActor` ;
- `UGridMonsterAnimInstance` ;
- `BP_MON_RatGiant` ;
- `ABP_MON_RatGiant`.

Valider uniquement :

- spawn ;
- Idle ;
- orientation ;
- position au centre de la case.

### Étape MON3 — Occupation et déplacement

Créer :

- registre d’occupation ;
- réservation des cases ;
- interpolation ;
- rotation ;
- State Machine Move/Turn.

### Étape MON4 — Pathfinding et perception

Créer :

- BFS ;
- ligne de vue ;
- portée d’ouïe ;
- dernier emplacement connu ;
- affichage de debug du chemin.

### Étape MON5 — Gestion des tours

Créer :

- `UGridTurnManagerComponent` ;
- phases de combat ;
- verrouillage des entrées ;
- file des monstres ;
- gestion des PA.

### Étape MON6 — Résolution du combat

Créer :

- `FGridCombatResolver` ;
- attaque de morsure ;
- sélection de la cible ;
- armure et points de vie ;
- Anim Notify d’impact.

### Étape MON7 — Comportement FastHarasser

Ajouter :

- attaque puis repli ;
- scoring des cases ;
- gestion des culs-de-sac ;
- agression de groupe.

### Étape MON8 — Mort, événements et butin

Ajouter :

- Montage de mort ;
- événement `MonsterDied` ;
- génération du loot ;
- liens avec les mécanismes.

### Étape MON9 — Sauvegarde

Ajouter :

- `FGridRuntimeMonsterState` ;
- capture ;
- restauration ;
- tests de transition entre niveaux.

### Étape MON10.1 — Journal de combat et feedback runtime

Statut : implémenté.

Ajouter :

- journal structuré ;
- formatteur localisable ;
- ring buffer ;
- delegate Blueprint ;
- victoire et défaite ;
- tests MON10.

### Étape MON10.2 — Audio

À venir :

- sons d’attaque ;
- sons d’impact ;
- sons de blessure ;
- sons de mort ;
- variations d’ambiance.

### Étape MON10.3 — VFX

À venir :

- impacts ;
- dégâts ;
- sang ;
- effets élémentaires ;
- feedback visuel du personnage ciblé.

### Étape MON10.4 — Variations d’Idle

À venir :

- choix déterministe de variations ;
- temporisation ;
- absence d’influence gameplay.

### Étape MON10.5 — Équilibrage et optimisation

À venir :

- graine par rencontre ;
- réglage des statistiques ;
- réduction des logs de diagnostic ;
- analyse des performances ;
- normalisation des fins de ligne.

---

## 30. Dépendances Unreal Engine

Le `Build.cs` actuel utilise notamment :

- Engine ;
- Enhanced Input ;
- UMG ;
- Niagara.

Il ne dépend pas encore de `AIModule`, `GameplayTasks` ou `NavigationSystem`.

Pour l’architecture proposée, il n’est pas nécessaire d’ajouter ces modules.

Le pathfinding et l’IA seront intégralement gérés par la grille.

Une extension future vers Behavior Tree demanderait probablement :

```csharp
"AIModule",
"GameplayTasks",
"NavigationSystem"
```

Il est préférable de ne pas les ajouter avant qu’un besoin concret ne soit démontré.

---

## 31. Optimisation et extensibilité

### 31.1. Aucun raisonnement IA par Tick

La décision n’est calculée qu’au début du tour du monstre.

### 31.2. Chargement différé des assets

Les références visuelles et audio du DataAsset doivent être des `TSoftObjectPtr` lorsque cela est raisonnable.

### 31.3. Pas de duplication par espèce

Ne pas créer une classe C++ spécifique pour chaque monstre si les différences peuvent être décrites dans un DataAsset.

Créer une sous-classe uniquement lorsqu’une créature possède une mécanique vraiment unique.

Exemples probables :

- Mimique ;
- Golem lié à une énigme ;
- boss à plusieurs phases.

### 31.4. Préparer les tailles supérieures à 1 × 1

Même si le Rat géant occupe une seule case, le système d’occupation doit accepter un `GridFootprint` afin de préparer les grosses créatures.

La première implémentation peut néanmoins valider uniquement les footprints 1 × 1 et produire un log explicite pour les autres.

---

## 32. Architecture finale résumée

```text
UGridLevelAsset
    contient les MonsterSpawn
                │
                ▼
AGridLevelRuntimeActor
    génère et enregistre les monstres
                │
                ├── UGridTurnManagerComponent
                │       gère les manches et les phases
                │
                ├── Registre d’occupation
                │       contrôle les cellules
                │
                └── AGridMonsterActor
                        │
                        ├── UGridMonsterDefinitionAsset
                        │       données et équilibrage
                        │
                        ├── UGridMonsterBehaviorComponent
                        │       perception et choix tactique
                        │
                        ├── UGridMonsterCombatComponent
                        │       attaques, dégâts et mort
                        │
                        ├── USkeletalMeshComponent
                        │       représentation animée
                        │
                        └── UGridMonsterAnimInstance
                                interface avec l’Animation Blueprint
```

La séparation principale doit rester :

```text
DataAsset       = ce qu’est le monstre
Actor           = son instance dans le monde
Behavior        = ce qu’il décide
Turn Manager    = quand il peut agir
Combat Resolver = comment l’action est calculée
Animation       = comment l’action est montrée
Grid Runtime    = où l’action peut avoir lieu
```

Cette fondation pourra accueillir tous les profils déjà prévus dans le bestiaire :

```text
DirectMelee
FastHarasser
SlowPressure
RangedKeeper
Ambush
PuzzleLinked
```

sans devoir reconstruire le système pour chaque nouvelle créature.

---

## 33. Références internes au projet

- `docs/ArtBook/Bestiaire_des_Profondeurs_Volume_I_FINAL.md`
- `docs/ArtBook/Bestiaire_des_Profondeurs_Volume_I_illustre.md`
- `docs/Rules/RPG_Core_Rules_v0_1.md`
- `docs/Rules/RPG_Damage_Types_Countermeasures_v0_1.md`
- `docs/Design/03_EVENT_COMMAND_LINKS.md`
- `docs/Design/MON10_COMBAT_FEEDBACK_SETUP.md`
- `Source/GrimrockPrototype/Public/Core/GridTypes.h`
- `Source/GrimrockPrototype/Public/Core/GridLevelAsset.h`
- `Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h`
- `Source/GrimrockPrototype/Public/Runtime/GridDungeonRuntimeState.h`
- `Source/GrimrockPrototype/Public/Runtime/GrimrockPartyPawn.h`
- `Source/GrimrockPrototype/Public/Runtime/GridInventoryTypes.h`
- `Source/GrimrockPrototype/Public/RPG/RPGCharacterTypes.h`

## 34. Références Unreal Engine

- Animation Blueprints : <https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-blueprints-in-unreal-engine>
- Animation Montages : <https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-montage-in-unreal-engine>
- Animation Notifies : <https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-notifies-in-unreal-engine>
- State Machines : <https://dev.epicgames.com/documentation/en-us/unreal-engine/state-machines-in-unreal-engine>
- Root Motion : <https://dev.epicgames.com/documentation/en-us/unreal-engine/root-motion-in-unreal-engine>
- Behavior Trees : <https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-trees-in-unreal-engine>
- AI Perception : <https://dev.epicgames.com/documentation/en-us/unreal-engine/ai-perception-in-unreal-engine>
