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
- détecte la victoire et la défaite.

#### `FGridCombatResolver`

- exécute les calculs purs ;
- ne connaît ni le monde ni les animations ;
- est facilement testable avec des Automation Tests.

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

    UPROPERTY(EditAnywhere,