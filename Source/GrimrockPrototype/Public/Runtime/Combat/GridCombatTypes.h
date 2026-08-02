#pragma once

#include "CoreMinimal.h"
#include "Core/GridObjectBehavior.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridCombatTypes.generated.h"

class UTexture2D;

UENUM (BlueprintType)
enum class EGridPhysicalDamageSubtype : uint8
{
    None        UMETA (DisplayName = "None"),
    Slashing    UMETA (DisplayName = "Slashing"),
    Piercing    UMETA (DisplayName = "Piercing"),
    Bludgeoning UMETA (DisplayName = "Bludgeoning")
};

UENUM (BlueprintType)
enum class EGridAttackScalingAttribute : uint8
{
    None,
    Strength,
    Dexterity,
    Constitution,
    Intelligence,
    Wisdom,
    Charisma
};

UENUM (BlueprintType)
enum class EGridCombatActionType : uint8
{
    None          UMETA (DisplayName = "None"),
    Move          UMETA (DisplayName = "Move"),
    Turn          UMETA (DisplayName = "Turn"),
    MeleeAttack   UMETA (DisplayName = "Melee Attack"),
    RangedAttack  UMETA (DisplayName = "Ranged Attack"),
    Ability       UMETA (DisplayName = "Ability"),
    Defend        UMETA (DisplayName = "Defend"),
    Wait          UMETA (DisplayName = "Wait"),
    Retreat       UMETA (DisplayName = "Retreat"),
    Die           UMETA (DisplayName = "Die")
};

/** Declared owner of a player combat action definition. */
UENUM (BlueprintType)
enum class EGridCombatActionSourcePolicy : uint8
{
    None      UMETA (DisplayName = "None"),
    Universal UMETA (DisplayName = "Universal"),
    Equipment UMETA (DisplayName = "Equipment"),
    Ability   UMETA (DisplayName = "Ability"),
    Spell     UMETA (DisplayName = "Spell"),
    QuickItem UMETA (DisplayName = "Quick Item")
};

UENUM (BlueprintType)
enum class EGridCombatTargetingPolicy : uint8
{
    None             UMETA (DisplayName = "None"),
    Self             UMETA (DisplayName = "Self"),
    Ally             UMETA (DisplayName = "Ally"),
    FirstAxialTarget UMETA (DisplayName = "First Axial Target"),
    Cell             UMETA (DisplayName = "Cell"),
    Area             UMETA (DisplayName = "Area")
};

UENUM (BlueprintType)
enum class EGridCombatActionResolutionProfile : uint8
{
    None        UMETA (DisplayName = "None"),
    Attack      UMETA (DisplayName = "Attack"),
    Defense     UMETA (DisplayName = "Defense"),
    Effect      UMETA (DisplayName = "Effect"),
    Interaction UMETA (DisplayName = "Interaction")
};

/** UI-ready reason why a catalogue entry cannot currently be requested. */
UENUM (BlueprintType)
enum class EGridCombatActionAvailabilityReason : uint8
{
    None                     UMETA (DisplayName = "None"),
    CombatInactive           UMETA (DisplayName = "Combat Inactive"),
    InvalidCharacter         UMETA (DisplayName = "Invalid Character"),
    CharacterDefeated        UMETA (DisplayName = "Character Defeated"),
    NotActiveCombatant       UMETA (DisplayName = "Not Active Combatant"),
    PartyBusy                UMETA (DisplayName = "Party Busy"),
    InsufficientActionPoints UMETA (DisplayName = "Insufficient Action Points"),
    InsufficientMana         UMETA (DisplayName = "Insufficient Mana"),
    InsufficientSourceItems  UMETA (DisplayName = "Insufficient Source Items"),
    MissingRequirement       UMETA (DisplayName = "Missing Requirement"),
    CooldownActive           UMETA (DisplayName = "Cooldown Active"),
    ExecutionNotImplemented  UMETA (DisplayName = "Execution Not Implemented")
};

UENUM (BlueprintType)
enum class EGridCombatActionRequestRejectReason : uint8
{
    None                      UMETA (DisplayName = "None"),
    TurnManagerNotInitialized UMETA (DisplayName = "Turn Manager Not Initialized"),
    InvalidAction             UMETA (DisplayName = "Invalid Action"),
    ActionUnavailable         UMETA (DisplayName = "Action Unavailable"),
    UnsupportedResolution     UMETA (DisplayName = "Unsupported Resolution"),
    AttackRejected            UMETA (DisplayName = "Attack Rejected")
};

UENUM (BlueprintType)
enum class EGridCombatPhase : uint8
{
    Exploration    UMETA (DisplayName = "Exploration"),
    StartingCombat UMETA (DisplayName = "Starting Combat"),
    PlayerPhase    UMETA (DisplayName = "Player Phase"),
    EnemyPhase     UMETA (DisplayName = "Enemy Phase"),
    EndingRound    UMETA (DisplayName = "Ending Round"),
    Victory        UMETA (DisplayName = "Victory"),
    Defeat         UMETA (DisplayName = "Defeat")
};

/**
 * Shared vocabulary for individual party and monster turns in the global
 * initiative order.
 */
UENUM (BlueprintType)
enum class EGridCombatantTurnState : uint8
{
    Waiting       UMETA (DisplayName = "Waiting"),
    Active        UMETA (DisplayName = "Active"),
    Completed     UMETA (DisplayName = "Completed"),
    Incapacitated UMETA (DisplayName = "Incapacitated"),
    Defeated      UMETA (DisplayName = "Defeated")
};

/** Identifies which authority executes a global-initiative entry. */
UENUM (BlueprintType)
enum class EGridCombatantSide : uint8
{
    Party   UMETA (DisplayName = "Party"),
    Monster UMETA (DisplayName = "Monster")
};

/**
 * UI-ready snapshot of one participant in the authoritative initiative order.
 * The stable id is a CharacterId for the party and a persistence id for a
 * monster. Actor pointers deliberately stay out of this public view model.
 */
USTRUCT (BlueprintType)
struct FGridCombatantInitiativeEntry
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    FGuid CombatantId;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    EGridCombatantSide Side = EGridCombatantSide::Party;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    int32 CharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    FText DisplayName;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    int32 InitiativeRoll = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    int32 InitiativeBase = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    int32 InitiativeTotal = 0;

    /** Final Dexterity used only as the second deterministic tie-break. */
    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    int32 Dexterity = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    int32 CurrentHealth = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    int32 MaximumHealth = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Initiative")
    EGridCombatantTurnState State = EGridCombatantTurnState::Waiting;

    bool IsValid () const
    {
        return CombatantId.IsValid () &&
            (Side == EGridCombatantSide::Monster ||
                CharacterIndex != INDEX_NONE);
    }

    bool IsPartyMember () const
    {
        return Side == EGridCombatantSide::Party;
    }
};

/** Authoritative per-round action-point state for one party member. */
USTRUCT (BlueprintType)
struct FGridPlayerCharacterTurnState
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Turn")
    int32 CharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Turn")
    FGuid CharacterId;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Turn")
    EGridCombatantTurnState State = EGridCombatantTurnState::Waiting;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Turn")
    int32 MaximumActionPoints = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Turn")
    int32 RemainingActionPoints = 0;

    bool CanSpend (int32 Cost) const
    {
        return State == EGridCombatantTurnState::Active &&
            Cost >= 0 &&
            RemainingActionPoints >= Cost;
    }
};

/** Shared per-round movement budget for the party's single grid position. */
USTRUCT (BlueprintType)
struct FGridPartyMobilityState
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Party Mobility")
    int32 RoundNumber = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Party Mobility")
    int32 MaximumMobilityActionPoints = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Party Mobility")
    int32 RemainingMobilityActionPoints = 0;

    bool CanSpend (int32 Cost) const
    {
        return Cost >= 0 && RemainingMobilityActionPoints >= Cost;
    }
};

/** Why an authoritative combat translation or rotation was refused. */
UENUM (BlueprintType)
enum class EGridPartyMovementRejectReason : uint8
{
    None                             UMETA (DisplayName = "None"),
    TurnManagerNotInitialized        UMETA (DisplayName = "Turn Manager Not Initialized"),
    CombatInactive                   UMETA (DisplayName = "Combat Inactive"),
    NotPlayerTurn                    UMETA (DisplayName = "Not Player Turn"),
    PartyUnavailable                 UMETA (DisplayName = "Party Unavailable"),
    PartyBusy                        UMETA (DisplayName = "Party Busy"),
    NotActiveCombatant               UMETA (DisplayName = "Not Active Combatant"),
    InvalidDirection                 UMETA (DisplayName = "Invalid Direction"),
    TargetCellUnavailable            UMETA (DisplayName = "Target Cell Unavailable"),
    PassageBlocked                   UMETA (DisplayName = "Passage Blocked"),
    TargetCellOccupied               UMETA (DisplayName = "Target Cell Occupied"),
    InsufficientActionPoints         UMETA (DisplayName = "Insufficient Action Points"),
    InsufficientMobilityActionPoints UMETA (DisplayName = "Insufficient Mobility Action Points")
};

UENUM (BlueprintType)
enum class EGridPlayerAttackRejectReason : uint8
{
    None                      UMETA (DisplayName = "None"),
    TurnManagerNotInitialized UMETA (DisplayName = "Turn Manager Not Initialized"),
    CombatInactive            UMETA (DisplayName = "Combat Inactive"),
    NotPlayerPhase            UMETA (DisplayName = "Not Player Phase"),
    PartyUnavailable          UMETA (DisplayName = "Party Unavailable"),
    PartyBusy                 UMETA (DisplayName = "Party Busy"),
    InvalidAttacker           UMETA (DisplayName = "Invalid Attacker"),
    AttackerDefeated          UMETA (DisplayName = "Attacker Defeated"),
    AttackerAlreadyActed      UMETA (DisplayName = "Attacker Already Acted"),
    InvalidFacing             UMETA (DisplayName = "Invalid Facing"),
    TargetCellUnavailable     UMETA (DisplayName = "Target Cell Unavailable"),
    PassageBlocked            UMETA (DisplayName = "Passage Blocked"),
    NoMonsterInFront          UMETA (DisplayName = "No Monster In Front"),
    TargetNotInEncounter      UMETA (DisplayName = "Target Not In Encounter"),
    TargetInactive            UMETA (DisplayName = "Target Inactive"),
    TargetDefeated            UMETA (DisplayName = "Target Defeated"),
    TargetOutOfRange          UMETA (DisplayName = "Target Out Of Range"),
    EquippedItemDefinitionUnavailable UMETA (DisplayName = "Equipped Item Definition Unavailable"),
    InvalidOffensiveEquipment UMETA (DisplayName = "Invalid Offensive Equipment"),
    InsufficientActionPoints  UMETA (DisplayName = "Insufficient Action Points"),
    NotActiveCombatant        UMETA (DisplayName = "Not Active Combatant")
};

USTRUCT (BlueprintType)
struct FGridPlayerAttackRequest
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    FGuid RequestId;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    int32 RoundNumber = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    int32 AttackerCharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    FGuid AttackerCharacterId;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    FGuid TargetMonsterId;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    FIntPoint PartyCell = FIntPoint::ZeroValue;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    FIntPoint TargetCell = FIntPoint::ZeroValue;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    EGridEdge PartyFacing = EGridEdge::None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    int32 RangeCells = 1;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    FName AttackId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    FName OffensiveItemDefinitionId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    EGridEquipmentSlot OffensiveEquipmentSlot = EGridEquipmentSlot::None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
    int32 ActionPointCost = 2;

    bool IsValid () const
    {
        const bool bCardinalFacing =
            PartyFacing == EGridEdge::North ||
            PartyFacing == EGridEdge::East ||
            PartyFacing == EGridEdge::South ||
            PartyFacing == EGridEdge::West;
        const bool bUnarmed =
            OffensiveItemDefinitionId.IsNone () &&
            OffensiveEquipmentSlot == EGridEquipmentSlot::None;
        const bool bEquipped =
            !OffensiveItemDefinitionId.IsNone () &&
            (OffensiveEquipmentSlot == EGridEquipmentSlot::MainHand ||
                OffensiveEquipmentSlot == EGridEquipmentSlot::OffHand);
        return RequestId.IsValid () &&
            RoundNumber > 0 &&
            AttackerCharacterIndex != INDEX_NONE &&
            AttackerCharacterId.IsValid () &&
            TargetMonsterId.IsValid () &&
            bCardinalFacing &&
            RangeCells > 0 &&
            ActionPointCost > 0 &&
            !AttackId.IsNone () &&
            (bUnarmed || bEquipped);
    }
};

USTRUCT (BlueprintType)
struct FGridAttackSourceStats
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Source")
    int32 Accuracy = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Source")
    int32 DamageBonus = 0;
};

USTRUCT (BlueprintType)
struct FGridAttackTargetStats
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target")
    int32 Evasion = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0"))
    int32 CurrentHealth = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0"))
    int32 PhysicalArmor = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0"))
    int32 MagicalArmor = 0;

    /** Percentage mitigation. Negative values represent vulnerability. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "-100", ClampMax = "100"))
    int32 ResistancePercent = 0;

    /** Species, status or environmental multiplier applied before resistance. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0.0"))
    float DamageMultiplier = 1.0f;
};

USTRUCT (BlueprintType)
struct FGridAttackDefinition
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
    EGridDamageType DamageType = EGridDamageType::Physical;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
    EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack", meta = (ClampMin = "0"))
    int32 MinDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack", meta = (ClampMin = "0"))
    int32 MaxDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
    int32 AccuracyBonus = 0;

    bool IsValid () const
    {
        return MinDamage >= 0 &&
            MaxDamage >= MinDamage &&
            (DamageType == EGridDamageType::Physical || PhysicalSubtype == EGridPhysicalDamageSubtype::None);
    }
};

USTRUCT (BlueprintType)
struct FGridOffensiveEquipmentProfile
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offense")
    FName AttackId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offense")
    FGridAttackDefinition AttackDefinition;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offense")
    int32 FlatDamageBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offense")
    EGridAttackScalingAttribute DamageScalingAttribute =
        EGridAttackScalingAttribute::Strength;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Equipment|Offense",
        meta = (ClampMin = "1", ClampMax = "32"))
    int32 RangeCells = 1;

    bool IsValid () const
    {
        return !AttackId.IsNone () &&
            AttackDefinition.IsValid () &&
            AttackDefinition.MaxDamage > 0 &&
            RangeCells >= 1 &&
            RangeCells <= 32;
    }
};

/** Resource costs declared by an action before runtime modifiers. */
USTRUCT (BlueprintType)
struct FGridCombatActionResourceCosts
{
    GENERATED_BODY ()

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Combat|Action|Costs",
        meta = (ClampMin = "0"))
    int32 ManaCost = 0;

    /** Number of units consumed from an item source after acceptance. */
    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Combat|Action|Costs",
        meta = (ClampMin = "0"))
    int32 SourceItemQuantityCost = 0;

    bool IsValid () const
    {
        return ManaCost >= 0 && SourceItemQuantityCost >= 0;
    }
};

/**
 * Data-oriented definition shared by equipment, abilities and spells.
 * Resolution and presentation stay separate from catalogue construction.
 */
USTRUCT (BlueprintType)
struct FGridCombatActionDefinition
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FName ActionId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FText DisplayName;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Combat|Action",
        meta = (MultiLine = "true"))
    FText Description;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    EGridCombatActionType ActionType = EGridCombatActionType::Ability;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    EGridCombatActionSourcePolicy SourcePolicy =
        EGridCombatActionSourcePolicy::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    EGridCombatTargetingPolicy TargetingPolicy =
        EGridCombatTargetingPolicy::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    EGridCombatActionResolutionProfile ResolutionProfile =
        EGridCombatActionResolutionProfile::None;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Combat|Action|Costs",
        meta = (ClampMin = "0", ClampMax = "6"))
    int32 ActionPointCost = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Costs")
    FGridCombatActionResourceCosts ResourceCosts;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Combat|Action|Targeting",
        meta = (ClampMin = "0", ClampMax = "32"))
    int32 RangeCells = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Requirements")
    TArray<FName> Requirements;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Combat|Action|Cooldown",
        meta = (ClampMin = "0"))
    int32 CooldownRounds = 0;

    /** Stable key resolved by the presentation layer; NAME_None uses source defaults. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Presentation")
    FName PresentationProfileId = NAME_None;

    /** Attack payload used when ResolutionProfile is Attack. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Resolution")
    FGridOffensiveEquipmentProfile OffensiveProfile;

    bool IsValid () const
    {
        const bool bAttackProfileValid =
            ResolutionProfile != EGridCombatActionResolutionProfile::Attack ||
            (OffensiveProfile.IsValid () && ActionPointCost > 0);
        const bool bAttackRangeValid =
            ResolutionProfile != EGridCombatActionResolutionProfile::Attack ||
            RangeCells == OffensiveProfile.RangeCells;
        const bool bTargetingRangeValid =
            (TargetingPolicy !=
                    EGridCombatTargetingPolicy::FirstAxialTarget &&
                TargetingPolicy != EGridCombatTargetingPolicy::Cell &&
                TargetingPolicy != EGridCombatTargetingPolicy::Area) ||
            RangeCells > 0;
        return !ActionId.IsNone () &&
            ActionType != EGridCombatActionType::None &&
            SourcePolicy != EGridCombatActionSourcePolicy::None &&
            TargetingPolicy != EGridCombatTargetingPolicy::None &&
            ResolutionProfile != EGridCombatActionResolutionProfile::None &&
            ActionPointCost >= 0 &&
            ActionPointCost <= 6 &&
            ResourceCosts.IsValid () &&
            RangeCells >= 0 &&
            RangeCells <= 32 &&
            CooldownRounds >= 0 &&
            bAttackProfileValid &&
            bAttackRangeValid &&
            bTargetingRangeValid;
    }
};

/** One concrete source contributing a definition to the catalogue builder. */
USTRUCT (BlueprintType)
struct FGridCombatActionContribution
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Catalog")
    FGridCombatActionDefinition Definition;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Catalog")
    FName SourceDefinitionId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Catalog")
    FGuid SourceRuntimeId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Catalog")
    EGridEquipmentSlot SourceEquipmentSlot = EGridEquipmentSlot::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Catalog")
    int32 AvailableSourceQuantity = 0;

    bool IsValid () const
    {
        return Definition.IsValid () &&
            (Definition.SourcePolicy ==
                    EGridCombatActionSourcePolicy::Universal ||
                !SourceDefinitionId.IsNone ());
    }
};

/** Transient, UI-ready result produced without resolving or paying an action. */
USTRUCT (BlueprintType)
struct FGridAvailableCombatAction
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    FGridCombatActionDefinition Definition;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    int32 CharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    FGuid CharacterId;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    FName SourceDefinitionId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    FGuid SourceRuntimeId;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    EGridEquipmentSlot SourceEquipmentSlot = EGridEquipmentSlot::None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    int32 CurrentActionPointCost = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    int32 CurrentManaCost = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    int32 CurrentSourceItemQuantityCost = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    bool bEnabled = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    EGridCombatActionAvailabilityReason AvailabilityReason =
        EGridCombatActionAvailabilityReason::None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    FText DisabledReason;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    FGuid SuggestedTargetId;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
    FIntPoint SuggestedTargetCell = FIntPoint::ZeroValue;

    bool MatchesSource (
        FName ActionId,
        EGridCombatActionSourcePolicy SourcePolicy,
        FName InSourceDefinitionId,
        EGridEquipmentSlot InSourceEquipmentSlot) const
    {
        return Definition.ActionId == ActionId &&
            Definition.SourcePolicy == SourcePolicy &&
            SourceDefinitionId == InSourceDefinitionId &&
            SourceEquipmentSlot == InSourceEquipmentSlot;
    }

    bool IsValid () const
    {
        return CharacterIndex != INDEX_NONE &&
            CharacterId.IsValid () &&
            Definition.IsValid () &&
            CurrentActionPointCost >= 0 &&
            CurrentManaCost >= 0 &&
            CurrentSourceItemQuantityCost >= 0;
    }
};

USTRUCT (BlueprintType)
struct FGridCombatAction
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FGuid ActionId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    EGridCombatActionType Type = EGridCombatActionType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FGuid SourceActorId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FGuid TargetActorId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    int32 TargetCharacterIndex = INDEX_NONE;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FIntPoint TargetCell = FIntPoint::ZeroValue;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FName AttackId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action", meta = (ClampMin = "0"))
    int32 ActionPointCost = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
    bool bHit = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
    bool bCriticalHit = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
    int32 RolledDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
    bool bOutcomeCommitted = false;

    /** MON7 marks the free turn and move that belong to a post-attack retreat. */
    UPROPERTY (BlueprintReadOnly, Category = "Combat|Action")
    bool bIsRepositioningAction = false;

    bool IsValid () const
    {
        return Type != EGridCombatActionType::None && ActionPointCost >= 0;
    }
};

USTRUCT (BlueprintType)
struct FGridAttackResult
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    bool bHit = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    bool bCriticalHit = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 NaturalAttackRoll = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 AttackRoll = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 DefenseValue = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 RawDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 ResistancePercent = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    float DamageMultiplier = 1.0f;

    /** Damage remaining after multipliers and percentage resistance, before armor. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 DamageAfterModifiers = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 PhysicalArmorDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 MagicalArmorDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 HealthDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 TargetHealthBefore = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 TargetHealthAfter = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    EGridDamageType DamageType = EGridDamageType::Physical;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

    int32 GetTotalAppliedDamage () const
    {
        return PhysicalArmorDamage + MagicalArmorDamage + HealthDamage;
    }
};

/** Result of the generic MON12 action request entry point. */
USTRUCT (BlueprintType)
struct FGridCombatActionRequestResult
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action")
    bool bAccepted = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action")
    EGridCombatActionRequestRejectReason RejectReason =
        EGridCombatActionRequestRejectReason::None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action")
    FGridAvailableCombatAction Action;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action")
    FGridPlayerAttackRequest AttackRequest;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action")
    FGridAttackResult AttackResult;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Action")
    EGridPlayerAttackRejectReason AttackRejectReason =
        EGridPlayerAttackRejectReason::None;
};
