#pragma once

#include "CoreMinimal.h"
#include "Core/GridObjectBehavior.h"
#include "Runtime/Combat/GridCombatActionIdentityTypes.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridCombatTypes.generated.h"

class AGridThrownItemActor;
class UTexture2D;

UENUM(BlueprintType)
enum class EGridPhysicalDamageSubtype : uint8
{
	None UMETA(DisplayName = "None"),
	Slashing UMETA(DisplayName = "Slashing"),
	Piercing UMETA(DisplayName = "Piercing"),
	Bludgeoning UMETA(DisplayName = "Bludgeoning")
};

UENUM(BlueprintType)
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

UENUM(BlueprintType)
enum class EGridCombatActionType : uint8
{
	None UMETA(DisplayName = "None"),
	Move UMETA(DisplayName = "Move"),
	Turn UMETA(DisplayName = "Turn"),
	MeleeAttack UMETA(DisplayName = "Melee Attack"),
	RangedAttack UMETA(DisplayName = "Ranged Attack"),
	Ability UMETA(DisplayName = "Ability"),
	Defend UMETA(DisplayName = "Defend"),
	Wait UMETA(DisplayName = "Wait"),
	Retreat UMETA(DisplayName = "Retreat"),
	Die UMETA(DisplayName = "Die")
};

UENUM(BlueprintType)
enum class EGridCombatTargetingPolicy : uint8
{
	None UMETA(DisplayName = "None"),
	Self UMETA(DisplayName = "Self"),
	Ally UMETA(DisplayName = "Ally"),
	FirstAxialTarget UMETA(DisplayName = "First Axial Target"),
	Cell UMETA(DisplayName = "Cell"),
	Area UMETA(DisplayName = "Area"),
	Hostile UMETA(DisplayName = "Hostile"),
	Party UMETA(DisplayName = "Party"),
	FrontRowParty UMETA(DisplayName = "Front Row Party"),
	AllyOrHostile UMETA(DisplayName = "Ally Or Hostile")
};

UENUM(BlueprintType)
enum class EGridCombatActionResolutionProfile : uint8
{
	None UMETA(DisplayName = "None"),
	Attack UMETA(DisplayName = "Attack"),
	Defense UMETA(DisplayName = "Defense"),
	Effect UMETA(DisplayName = "Effect"),
	Interaction UMETA(DisplayName = "Interaction")
};

/** UI-ready reason why a catalogue entry cannot currently be requested. */
UENUM(BlueprintType)
enum class EGridCombatActionAvailabilityReason : uint8
{
	None UMETA(DisplayName = "None"),
	CombatInactive UMETA(DisplayName = "Combat Inactive"),
	InvalidCharacter UMETA(DisplayName = "Invalid Character"),
	CharacterDefeated UMETA(DisplayName = "Character Defeated"),
	NotActiveCombatant UMETA(DisplayName = "Not Active Combatant"),
	PartyBusy UMETA(DisplayName = "Party Busy"),
	InsufficientActionPoints UMETA(DisplayName = "Insufficient Action Points"),
	InsufficientMobilityActionPoints UMETA(DisplayName = "Insufficient Mobility Action Points"),
	InsufficientMana UMETA(DisplayName = "Insufficient Mana"),
	InsufficientSourceItems UMETA(DisplayName = "Insufficient Source Items"),
	MissingRequirement UMETA(DisplayName = "Missing Requirement"),
	RequiredOffensiveEquipmentUnavailable UMETA(DisplayName = "Required Offensive Equipment Unavailable"),
	CooldownActive UMETA(DisplayName = "Cooldown Active"),
	ExecutionNotImplemented UMETA(DisplayName = "Execution Not Implemented"),
	NoApplicableEffect UMETA(DisplayName = "No Applicable Effect")
};

UENUM(BlueprintType)
enum class EGridCombatActionRequestRejectReason : uint8
{
	None UMETA(DisplayName = "None"),
	TurnManagerNotInitialized UMETA(DisplayName = "Turn Manager Not Initialized"),
	InvalidAction UMETA(DisplayName = "Invalid Action"),
	ActionUnavailable UMETA(DisplayName = "Action Unavailable"),
	UnsupportedResolution UMETA(DisplayName = "Unsupported Resolution"),
	TargetRequired UMETA(DisplayName = "Target Required"),
	InvalidTarget UMETA(DisplayName = "Invalid Target"),
	AttackRejected UMETA(DisplayName = "Attack Rejected"),
	QuickItemRejected UMETA(DisplayName = "Quick Item Rejected"),
	ClassActionRejected UMETA(DisplayName = "Class Action Rejected")
};

UENUM(BlueprintType)
enum class EGridCombatPhase : uint8
{
	Exploration UMETA(DisplayName = "Exploration"),
	StartingCombat UMETA(DisplayName = "Starting Combat"),
	PlayerPhase UMETA(DisplayName = "Player Phase"),
	EnemyPhase UMETA(DisplayName = "Enemy Phase"),
	EndingRound UMETA(DisplayName = "Ending Round"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat")
};

/**
 * Shared vocabulary for individual party and monster turns in the global
 * initiative order.
 */
UENUM(BlueprintType)
enum class EGridCombatantTurnState : uint8
{
	Waiting UMETA(DisplayName = "Waiting"),
	Active UMETA(DisplayName = "Active"),
	Completed UMETA(DisplayName = "Completed"),
	Incapacitated UMETA(DisplayName = "Incapacitated"),
	Defeated UMETA(DisplayName = "Defeated")
};

/** Identifies which authority executes a global-initiative entry. */
UENUM(BlueprintType)
enum class EGridCombatantSide : uint8
{
	Party UMETA(DisplayName = "Party"),
	Monster UMETA(DisplayName = "Monster")
};

/**
 * UI-ready snapshot of one participant in the authoritative initiative order.
 * The stable id is a CharacterId for the party and a persistence id for a
 * monster. Actor pointers deliberately stay out of this public view model.
 */
USTRUCT(BlueprintType)
struct FGridCombatantInitiativeEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	FGuid CombatantId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	EGridCombatantSide Side = EGridCombatantSide::Party;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	int32 CharacterIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	int32 InitiativeRoll = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	int32 InitiativeBase = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	int32 InitiativeTotal = 0;

	/** Runtime haste/slow contribution. The encounter roll never changes. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	int32 InitiativeModifier = 0;

	/** Owner-only class bonus used only during the first encounter round. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	int32 FirstRoundInitiativeModifier = 0;

	/** Final Dexterity used only as the second deterministic tie-break. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	int32 Dexterity = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	int32 CurrentHealth = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	int32 MaximumHealth = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	EGridCombatantTurnState State = EGridCombatantTurnState::Waiting;

	bool IsValid() const
	{
		return CombatantId.IsValid() && (Side == EGridCombatantSide::Monster || CharacterIndex != INDEX_NONE);
	}

	bool IsPartyMember() const
	{
		return Side == EGridCombatantSide::Party;
	}

	int32 GetEffectiveInitiativeTotal() const
	{
		return static_cast<int32>(FMath::Clamp<int64>(
			static_cast<int64>(InitiativeTotal) + InitiativeModifier + FirstRoundInitiativeModifier, MIN_int32, MAX_int32));
	}
};

/** One future activation in the authoritative sliding initiative preview. */
USTRUCT(BlueprintType)
struct FGridInitiativePreviewEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	FGridCombatantInitiativeEntry Combatant;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	int32 RoundNumber = 0;

	/** Zero-based position of this activation inside its projected round. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	int32 ActivationIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	bool bIsActive = false;

	/** True when a round separator must be drawn before this activation. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Initiative")
	bool bStartsNewRound = false;
};

/** Authoritative per-round action-point state for one party member. */
USTRUCT(BlueprintType)
struct FGridPlayerCharacterTurnState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Turn")
	int32 CharacterIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Turn")
	FGuid CharacterId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Turn")
	EGridCombatantTurnState State = EGridCombatantTurnState::Waiting;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Turn")
	int32 MaximumActionPoints = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Turn")
	int32 RemainingActionPoints = 0;

	bool CanSpend(int32 Cost) const
	{
		return State == EGridCombatantTurnState::Active && Cost >= 0 && RemainingActionPoints >= Cost;
	}
};

/** Shared per-round movement budget for the party's single grid position. */
USTRUCT(BlueprintType)
struct FGridPartyMobilityState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Party Mobility")
	int32 RoundNumber = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Party Mobility")
	int32 MaximumMobilityActionPoints = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Party Mobility")
	int32 RemainingMobilityActionPoints = 0;

	bool CanSpend(int32 Cost) const
	{
		return Cost >= 0 && RemainingMobilityActionPoints >= Cost;
	}
};

/** Why an authoritative combat translation or rotation was refused. */
UENUM(BlueprintType)
enum class EGridPartyMovementRejectReason : uint8
{
	None UMETA(DisplayName = "None"),
	TurnManagerNotInitialized UMETA(DisplayName = "Turn Manager Not Initialized"),
	CombatInactive UMETA(DisplayName = "Combat Inactive"),
	NotPlayerTurn UMETA(DisplayName = "Not Player Turn"),
	PartyUnavailable UMETA(DisplayName = "Party Unavailable"),
	PartyBusy UMETA(DisplayName = "Party Busy"),
	NotActiveCombatant UMETA(DisplayName = "Not Active Combatant"),
	InvalidDirection UMETA(DisplayName = "Invalid Direction"),
	TargetCellUnavailable UMETA(DisplayName = "Target Cell Unavailable"),
	PassageBlocked UMETA(DisplayName = "Passage Blocked"),
	TargetCellOccupied UMETA(DisplayName = "Target Cell Occupied"),
	InsufficientActionPoints UMETA(DisplayName = "Insufficient Action Points"),
	InsufficientMobilityActionPoints UMETA(DisplayName = "Insufficient Mobility Action Points")
};

UENUM(BlueprintType)
enum class EGridPlayerAttackRejectReason : uint8
{
	None UMETA(DisplayName = "None"),
	TurnManagerNotInitialized UMETA(DisplayName = "Turn Manager Not Initialized"),
	CombatInactive UMETA(DisplayName = "Combat Inactive"),
	NotPlayerPhase UMETA(DisplayName = "Not Player Phase"),
	PartyUnavailable UMETA(DisplayName = "Party Unavailable"),
	PartyBusy UMETA(DisplayName = "Party Busy"),
	InvalidAttacker UMETA(DisplayName = "Invalid Attacker"),
	AttackerDefeated UMETA(DisplayName = "Attacker Defeated"),
	AttackerAlreadyActed UMETA(DisplayName = "Attacker Already Acted"),
	InvalidFacing UMETA(DisplayName = "Invalid Facing"),
	TargetCellUnavailable UMETA(DisplayName = "Target Cell Unavailable"),
	PassageBlocked UMETA(DisplayName = "Passage Blocked"),
	NoMonsterInFront UMETA(DisplayName = "No Monster In Front"),
	TargetNotInEncounter UMETA(DisplayName = "Target Not In Encounter"),
	TargetInactive UMETA(DisplayName = "Target Inactive"),
	TargetDefeated UMETA(DisplayName = "Target Defeated"),
	TargetOutOfRange UMETA(DisplayName = "Target Out Of Range"),
	EquippedItemDefinitionUnavailable UMETA(DisplayName = "Equipped Item Definition Unavailable"),
	InvalidOffensiveEquipment UMETA(DisplayName = "Invalid Offensive Equipment"),
	InsufficientActionPoints UMETA(DisplayName = "Insufficient Action Points"),
	NotActiveCombatant UMETA(DisplayName = "Not Active Combatant")
};

USTRUCT(BlueprintType)
struct FGridPlayerAttackRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	FGuid RequestId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	int32 RoundNumber = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	int32 AttackerCharacterIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	FGuid AttackerCharacterId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	FGuid TargetMonsterId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	FIntPoint PartyCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	FIntPoint TargetCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	EGridEdge PartyFacing = EGridEdge::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	int32 RangeCells = 1;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	FName AttackId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	FName OffensiveItemDefinitionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	EGridEquipmentSlot OffensiveEquipmentSlot = EGridEquipmentSlot::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	int32 ActionPointCost = 2;

	/**
     * Recoverable projectile already committed by the authoritative attack
     * transaction. Presentation may animate/configure it but never consumes
     * the source item itself.
     */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Player Attack")
	TObjectPtr<AGridThrownItemActor> PreparedThrownItemActor = nullptr;

	bool IsValid() const
	{
		const bool bCardinalFacing =
			PartyFacing == EGridEdge::North || PartyFacing == EGridEdge::East || PartyFacing == EGridEdge::South || PartyFacing == EGridEdge::West;
		const bool bUnarmed = OffensiveItemDefinitionId.IsNone() && OffensiveEquipmentSlot == EGridEquipmentSlot::None;
		const bool bEquipped = !OffensiveItemDefinitionId.IsNone() &&
			(OffensiveEquipmentSlot == EGridEquipmentSlot::MainHand || OffensiveEquipmentSlot == EGridEquipmentSlot::OffHand);
		const bool bQuickItem = !OffensiveItemDefinitionId.IsNone() && OffensiveEquipmentSlot == EGridEquipmentSlot::None;
		return RequestId.IsValid() && RoundNumber > 0 && AttackerCharacterIndex != INDEX_NONE && AttackerCharacterId.IsValid() && TargetMonsterId.IsValid() &&
			bCardinalFacing && RangeCells > 0 && ActionPointCost > 0 && !AttackId.IsNone() && (bUnarmed || bEquipped || bQuickItem);
	}
};

USTRUCT(BlueprintType)
struct FGridAttackSourceStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Source")
	int32 Accuracy = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Source")
	int32 DamageBonus = 0;

	/**
	 * Integer base-damage coefficient applied after the weapon roll + flat/scaling
	 * bonus and before critical damage. 100 preserves historical behavior.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Source", meta = (ClampMin = "0", ClampMax = "1000"))
	int32 RawDamagePercent = 100;

	/** Multiplicative outgoing damage scale. 1.0 preserves historical behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Source", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	/** Critical chance expressed in percentage points on the attack d20. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Source", meta = (ClampMin = "0", ClampMax = "100"))
	int32 CriticalChancePercent = 5;

	/** Critical raw-damage multiplier in percent. 200 means historical x2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Source", meta = (ClampMin = "100", ClampMax = "1000"))
	int32 CriticalDamagePercent = 200;
};

USTRUCT(BlueprintType)
struct FGridAttackTargetStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target")
	int32 Evasion = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0"))
	int32 CurrentHealth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0"))
	int32 PhysicalArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0"))
	int32 MagicalArmor = 0;

	/** Percentage mitigation. Negative values represent vulnerability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "-100", ClampMax = "100"))
	int32 ResistancePercent = 0;

	/** Species, status or environmental multiplier applied before resistance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct FGridAttackDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
	EGridDamageType DamageType = EGridDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
	EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack", meta = (ClampMin = "0"))
	int32 MinDamage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack", meta = (ClampMin = "0"))
	int32 MaxDamage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
	int32 AccuracyBonus = 0;

	/** Deterministic direct-damage actions can opt out of the ordinary d20 hit gate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
	bool bAlwaysHits = false;

	/** When false, a successful hit never upgrades to a critical hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
	bool bCanCriticalHit = true;

	bool IsValid() const
	{
		return MinDamage >= 0 && MaxDamage >= MinDamage && (DamageType == EGridDamageType::Physical || PhysicalSubtype == EGridPhysicalDamageSubtype::None);
	}
};

USTRUCT(BlueprintType)
struct FGridOffensiveEquipmentProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offense")
	FName AttackId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offense")
	FGridAttackDefinition AttackDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offense")
	int32 FlatDamageBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offense")
	EGridAttackScalingAttribute DamageScalingAttribute = EGridAttackScalingAttribute::Strength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offense", meta = (ClampMin = "1", ClampMax = "32"))
	int32 RangeCells = 1;

	bool IsValid() const
	{
		return !AttackId.IsNone() && AttackDefinition.IsValid() && AttackDefinition.MaxDamage > 0 && RangeCells >= 1 && RangeCells <= 32;
	}
};

/**
 * RPG03.9 generic projection for attacks whose base damage comes from the
 * currently equipped offensive item. The authored action owns costs, targeting
 * and the WD coefficient; the item remains authoritative for its damage roll,
 * flat bonus and scaling attribute.
 */
USTRUCT(BlueprintType)
struct FGridCombatWeaponAttackProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Weapon Attack")
	bool bUseEquippedWeapon = false;

	/** RPG02 WD coefficient. 100 = the resolved equipped-weapon base damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Weapon Attack", meta = (ClampMin = "1", ClampMax = "500"))
	int32 WeaponDamagePercent = 100;

	/** Every tag must exist on the selected offensive item's definition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Weapon Attack")
	TArray<FName> RequiredItemTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Weapon Attack")
	TArray<EGridPhysicalDamageSubtype> AllowedPhysicalSubtypes;

	/** Requires the resolved offensive item to have a range greater than one cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Weapon Attack")
	bool bRequireRangedWeapon = false;

	/** Use the equipped weapon's authored range instead of the action's absolute RangeCells. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Weapon Attack")
	bool bUseWeaponRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Weapon Attack", meta = (ClampMin = "-31", ClampMax = "31"))
	int32 WeaponRangeModifier = 0;

	/** Allows the standard unarmed profile when no offensive item is equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Weapon Attack")
	bool bAllowUnarmed = false;

	/** Optional action-owned damage descriptor; Min/Max, flat bonus and scaling still come from the weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Weapon Attack")
	bool bOverrideDamageDescriptor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Weapon Attack")
	EGridDamageType OverrideDamageType = EGridDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Weapon Attack")
	EGridPhysicalDamageSubtype OverridePhysicalSubtype = EGridPhysicalDamageSubtype::None;

	bool MatchesItemTags(const TArray<FName>& ItemTags) const
	{
		for (const FName RequiredTag : RequiredItemTags)
		{
			if (!ItemTags.Contains(RequiredTag))
			{
				return false;
			}
		}
		return true;
	}

	bool ApplyToOffensiveProfile(FName ActionId, int32 ActionRangeCells, FGridOffensiveEquipmentProfile& InOutProfile) const
	{
		if (!bUseEquippedWeapon || ActionId.IsNone() || ActionRangeCells < 1 || ActionRangeCells > 32 || !InOutProfile.IsValid())
		{
			return false;
		}
		if (!AllowedPhysicalSubtypes.IsEmpty() &&
			(InOutProfile.AttackDefinition.DamageType != EGridDamageType::Physical ||
				!AllowedPhysicalSubtypes.Contains(InOutProfile.AttackDefinition.PhysicalSubtype)))
		{
			return false;
		}
		if (bRequireRangedWeapon && InOutProfile.RangeCells <= 1)
		{
			return false;
		}
		InOutProfile.AttackId = ActionId;
		InOutProfile.RangeCells = bUseWeaponRange
			? FMath::Clamp(InOutProfile.RangeCells + WeaponRangeModifier, 1, 32)
			: ActionRangeCells;
		if (bOverrideDamageDescriptor)
		{
			InOutProfile.AttackDefinition.DamageType = OverrideDamageType;
			InOutProfile.AttackDefinition.PhysicalSubtype =
				OverrideDamageType == EGridDamageType::Physical ? OverridePhysicalSubtype : EGridPhysicalDamageSubtype::None;
		}
		return InOutProfile.IsValid();
	}

	bool IsValid() const
	{
		if (!bUseEquippedWeapon)
		{
			return true;
		}
		if (WeaponDamagePercent < 1 || WeaponDamagePercent > 500 ||
			WeaponRangeModifier < -31 || WeaponRangeModifier > 31 ||
			(!bUseWeaponRange && WeaponRangeModifier != 0) ||
			(bAllowUnarmed && (!RequiredItemTags.IsEmpty() || bRequireRangedWeapon)))
		{
			return false;
		}
		TSet<FName> SeenTags;
		for (const FName Tag : RequiredItemTags)
		{
			if (Tag.IsNone() || SeenTags.Contains(Tag))
			{
				return false;
			}
			SeenTags.Add(Tag);
		}
		TSet<EGridPhysicalDamageSubtype> SeenSubtypes;
		for (const EGridPhysicalDamageSubtype Subtype : AllowedPhysicalSubtypes)
		{
			if (Subtype == EGridPhysicalDamageSubtype::None || SeenSubtypes.Contains(Subtype))
			{
				return false;
			}
			SeenSubtypes.Add(Subtype);
		}
		return !bOverrideDamageDescriptor || OverrideDamageType == EGridDamageType::Physical ||
			OverridePhysicalSubtype == EGridPhysicalDamageSubtype::None;
	}
};

UENUM(BlueprintType)
enum class EGridCombatMovementSubject : uint8
{
	PartyGroup UMETA(DisplayName = "Party Group"),
	TargetCombatant UMETA(DisplayName = "Target Combatant")
};

UENUM(BlueprintType)
enum class EGridCombatMovementDirection : uint8
{
	ForwardFromFacing UMETA(DisplayName = "Forward From Facing"),
	BackwardFromFacing UMETA(DisplayName = "Backward From Facing"),
	LeftFromFacing UMETA(DisplayName = "Left From Facing"),
	RightFromFacing UMETA(DisplayName = "Right From Facing"),
	AwayFromSource UMETA(DisplayName = "Away From Source")
};

/**
 * C5 tactical/forced movement primitive. PartyGroup always moves as one
 * formation anchor; TargetCombatant is reserved for targeted C8 execution.
 */
USTRUCT(BlueprintType)
struct FGridCombatMovementEffectProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Movement")
	EGridCombatMovementSubject Subject = EGridCombatMovementSubject::PartyGroup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Movement")
	EGridCombatMovementDirection Direction = EGridCombatMovementDirection::BackwardFromFacing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Movement", meta = (ClampMin = "1", ClampMax = "8"))
	int32 DistanceCells = 1;

	/** Shared party PAM paid in addition to the action's ordinary AP cost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Movement", meta = (ClampMin = "0", ClampMax = "4"))
	int32 MobilityActionPointCost = 0;

	/** Forced movement ignores bBlockTranslation but never ignores geometry/occupancy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Movement")
	bool bForced = false;

	bool IsValid() const
	{
		if (DistanceCells < 1 || DistanceCells > 8 || MobilityActionPointCost < 0 || MobilityActionPointCost > 4)
		{
			return false;
		}
		if (Subject == EGridCombatMovementSubject::PartyGroup)
		{
			return Direction != EGridCombatMovementDirection::AwayFromSource;
		}
		return MobilityActionPointCost == 0;
	}
};

USTRUCT(BlueprintType)
struct FGridCombatMovementResolution
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Movement")
	FIntPoint FromCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Movement")
	FIntPoint ToCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Movement")
	EGridEdge Direction = EGridEdge::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Movement")
	int32 DistanceCells = 0;

	bool IsValid() const
	{
		return Direction != EGridEdge::None && DistanceCells > 0 && FromCell != ToCell;
	}
};

UENUM(BlueprintType)
enum class EGridCombatReactionTrigger : uint8
{
	None UMETA(DisplayName = "None"),
	ActionResolved UMETA(DisplayName = "Action Resolved"),
	AttackHit UMETA(DisplayName = "Attack Hit"),
	AttackMiss UMETA(DisplayName = "Attack Miss"),
	TargetDefeated UMETA(DisplayName = "Target Defeated"),
	DirectDamageReceived UMETA(DisplayName = "Direct Damage Received"),
	SurfaceReaction UMETA(DisplayName = "Surface Reaction"),
	IncomingAttackHit UMETA(DisplayName = "Incoming Attack Hit"),
	OwnedStatusTargetDefeated UMETA(DisplayName = "Owned Status Target Defeated")
};

UENUM(BlueprintType)
enum class EGridCombatReactionLimit : uint8
{
	Unlimited UMETA(DisplayName = "Unlimited"),
	OncePerRound UMETA(DisplayName = "Once Per Round"),
	OncePerAction UMETA(DisplayName = "Once Per Action")
};

/**
 * Generic C4 trigger authored by Talents or Status Effects.
 * Runtime consumers react to the emitted match; production never switches on ReactionId.
 */
USTRUCT(BlueprintType)
struct FGridCombatReactionProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction")
	FName ReactionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction")
	EGridCombatReactionTrigger Trigger = EGridCombatReactionTrigger::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction")
	EGridCombatReactionLimit Limit = EGridCombatReactionLimit::Unlimited;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Filter")
	TArray<FName> ActionIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Filter")
	TArray<EGridCombatActionSourcePolicy> SourcePolicies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Filter")
	TArray<EGridCombatActionType> ActionTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Filter")
	TArray<EGridDamageType> DamageTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Filter")
	TArray<FName> RequiredSourceTags;

	/** Requirements owned by the reaction owner before this profile is projected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Filter")
	TArray<FName> RequiredOwnerRequirementIds;

	/** Generic semantic filter used by stealth/status reactions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Filter")
	bool bRequireOffensiveAction = false;

	/** Every listed target status must belong to the reaction owner on the event target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Filter")
	TArray<FName> RequiredTargetStatusEffectIdsFromOwner;

	/** Reaction-generated events are ignored by default to prevent recursive chains. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction")
	bool bAllowReactionGeneratedEvents = false;

	/** When authored on a Status Effect, consume that status after a matching event. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction")
	bool bConsumeOwningStatus = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Response")
	FName CounterAttackActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Response", meta = (ClampMin = "1", ClampMax = "32"))
	int32 CounterAttackRangeCells = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Response")
	FGridCombatWeaponAttackProfile CounterAttackWeaponProfile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Response", meta = (ClampMin = "0", ClampMax = "100"))
	int32 InterceptFinalDamagePercent = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Response")
	bool bRequireOwnerFrontRow = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Response")
	bool bRequireEventTargetFrontRow = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Response")
	FName ApplyOwnerStatusEffectId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Response", meta = (ClampMin = "-1", ClampMax = "12"))
	int32 ApplyOwnerStatusDurationOverride = INDEX_NONE;

	/** Transfer this owner-sourced target status to the nearest living hostile after a qualifying event. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Response")
	FName TransferOwnedTargetStatusEffectId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Response", meta = (ClampMin = "1", ClampMax = "32"))
	int32 TransferTargetRangeCells = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reaction|Response", meta = (ClampMin = "-1", ClampMax = "12"))
	int32 TransferStatusDurationOverride = INDEX_NONE;

	bool IsValid() const
	{
		if (ReactionId.IsNone() || Trigger == EGridCombatReactionTrigger::None || CounterAttackRangeCells < 1 || CounterAttackRangeCells > 32 ||
			InterceptFinalDamagePercent < 0 || InterceptFinalDamagePercent > 100)
		{
			return false;
		}
		const bool bHasCounterAttack = CounterAttackWeaponProfile.bUseEquippedWeapon;
		if (bHasCounterAttack != !CounterAttackActionId.IsNone() || !CounterAttackWeaponProfile.IsValid())
		{
			return false;
		}
		if (bHasCounterAttack && Trigger != EGridCombatReactionTrigger::AttackMiss)
		{
			return false;
		}
		if (InterceptFinalDamagePercent > 0 && Trigger != EGridCombatReactionTrigger::IncomingAttackHit)
		{
			return false;
		}
		for (const FName Id : ActionIds)
		{
			if (Id.IsNone())
			{
				return false;
			}
		}
		for (const EGridCombatActionSourcePolicy Policy : SourcePolicies)
		{
			if (Policy == EGridCombatActionSourcePolicy::None)
			{
				return false;
			}
		}
		for (const EGridCombatActionType Type : ActionTypes)
		{
			if (Type == EGridCombatActionType::None)
			{
				return false;
			}
		}
		TSet<FName> SeenSourceTags;
		for (const FName Tag : RequiredSourceTags)
		{
			if (Tag.IsNone() || SeenSourceTags.Contains(Tag))
			{
				return false;
			}
			SeenSourceTags.Add(Tag);
		}
		TSet<FName> SeenOwnerRequirements;
		for (const FName RequirementId : RequiredOwnerRequirementIds)
		{
			if (RequirementId.IsNone() || SeenOwnerRequirements.Contains(RequirementId))
			{
				return false;
			}
			SeenOwnerRequirements.Add(RequirementId);
		}
		TSet<FName> SeenTargetStatuses;
		for (const FName EffectId : RequiredTargetStatusEffectIdsFromOwner)
		{
			if (EffectId.IsNone() || SeenTargetStatuses.Contains(EffectId))
			{
				return false;
			}
			SeenTargetStatuses.Add(EffectId);
		}
		if (!TransferOwnedTargetStatusEffectId.IsNone() &&
			(Trigger != EGridCombatReactionTrigger::OwnedStatusTargetDefeated || TransferTargetRangeCells < 1 || TransferTargetRangeCells > 32))
		{
			return false;
		}
		if (TransferOwnedTargetStatusEffectId.IsNone() && TransferStatusDurationOverride != INDEX_NONE)
		{
			return false;
		}
		if (TransferStatusDurationOverride < INDEX_NONE)
		{
			return false;
		}
		if (ApplyOwnerStatusDurationOverride < INDEX_NONE)
		{
			return false;
		}
		if (ApplyOwnerStatusEffectId.IsNone() && ApplyOwnerStatusDurationOverride != INDEX_NONE)
		{
			return false;
		}
		return true;
	}
};

USTRUCT(BlueprintType)
struct FGridCombatReactionEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FGuid EventId;

	/** Stable identity shared by every event emitted by one resolved action. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FGuid ActionInstanceId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	int32 RoundNumber = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	EGridCombatReactionTrigger Trigger = EGridCombatReactionTrigger::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FGuid SourceCombatantId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FGuid TargetCombatantId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FName ActionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	EGridCombatActionSourcePolicy SourcePolicy = EGridCombatActionSourcePolicy::Universal;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	EGridCombatActionType ActionType = EGridCombatActionType::Ability;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	EGridDamageType DamageType = EGridDamageType::Physical;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	TArray<FName> SourceTags;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	bool bOffensiveAction = false;

	/** Filled per reaction owner for target statuses whose SourceId equals that owner. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	TArray<FName> TargetStatusEffectIdsFromOwner;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	bool bReactionGenerated = false;

	bool IsValid() const
	{
		return EventId.IsValid() && ActionInstanceId.IsValid() && RoundNumber >= 1 && Trigger != EGridCombatReactionTrigger::None &&
			SourceCombatantId.IsValid() && TargetCombatantId.IsValid();
	}
};

USTRUCT(BlueprintType)
struct FGridCombatReactionMatch
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FName ReactionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FGuid OwnerCombatantId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FName OwningStatusEffectId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	bool bConsumeOwningStatus = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FName CounterAttackActionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	int32 CounterAttackRangeCells = 1;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FGridCombatWeaponAttackProfile CounterAttackWeaponProfile;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	int32 InterceptFinalDamagePercent = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	bool bRequireOwnerFrontRow = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	bool bRequireEventTargetFrontRow = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FName ApplyOwnerStatusEffectId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	int32 ApplyOwnerStatusDurationOverride = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FName TransferOwnedTargetStatusEffectId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	int32 TransferTargetRangeCells = 1;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	int32 TransferStatusDurationOverride = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Reaction")
	FGridCombatReactionEvent Event;
};

UENUM(BlueprintType)
enum class EGridCombatStatusApplicationTrigger : uint8
{
	None UMETA(DisplayName = "None"),
	AfterResolution UMETA(DisplayName = "After Resolution"),
	AfterSuccessfulHit UMETA(DisplayName = "After Successful Hit")
};

UENUM(BlueprintType)
enum class EGridCombatStatusArmorGate : uint8
{
	None UMETA(DisplayName = "None"),
	PhysicalArmorDepleted UMETA(DisplayName = "Physical Armor Depleted"),
	MagicalArmorDepleted UMETA(DisplayName = "Magical Armor Depleted")
};

/**
 * Generic C1 secondary status application attached to one combat action.
 * StatusEffectId is the stable GridStatusEffect primary-asset identity.
 */
UENUM(BlueprintType)
enum class EGridCombatResolvedTargetScope : uint8
{
	AllResolvedTargets UMETA(DisplayName = "All Resolved Targets"),
	PrimaryTargetOnly UMETA(DisplayName = "Primary Target Only")
};

USTRUCT(BlueprintType)
struct FGridCombatStatusApplicationProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status")
	FName StatusEffectId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status")
	EGridCombatStatusApplicationTrigger Trigger = EGridCombatStatusApplicationTrigger::AfterResolution;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status")
	EGridCombatStatusArmorGate ArmorGate = EGridCombatStatusArmorGate::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status", meta = (ClampMin = "1"))
	int32 InitialStackCount = 1;

	/** INDEX_NONE uses the status definition default. Zero is reserved for permanent effects. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status", meta = (ClampMin = "-1"))
	int32 DurationOverride = INDEX_NONE;

	/** INDEX_NONE uses the status definition default potency. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status", meta = (ClampMin = "-1"))
	int32 PotencyOverride = INDEX_NONE;

	/** C8 batch scope; primary is deterministic and resolved before secondary targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status")
	EGridCombatResolvedTargetScope TargetScope = EGridCombatResolvedTargetScope::AllResolvedTargets;

	bool IsValid() const
	{
		return !StatusEffectId.IsNone() && Trigger != EGridCombatStatusApplicationTrigger::None && InitialStackCount >= 1 &&
			DurationOverride >= INDEX_NONE && PotencyOverride >= INDEX_NONE;
	}
};

/** C8 deterministic removal filter. Identity/tag filters use OR; disposition further restricts matches. */
UENUM(BlueprintType)
enum class EGridCombatStatusRemovalTargetSide : uint8
{
	Any UMETA(DisplayName = "Any"),
	Party UMETA(DisplayName = "Party"),
	Hostile UMETA(DisplayName = "Hostile")
};

USTRUCT(BlueprintType)
struct FGridCombatStatusRemovalProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status Removal|Filter")
	TArray<FName> EffectIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status Removal|Filter")
	TArray<FName> AnyStatusTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status Removal|Filter")
	TArray<EGridStatusEffectDisposition> AllowedDispositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status Removal|Filter")
	EGridCombatStatusRemovalTargetSide TargetSide = EGridCombatStatusRemovalTargetSide::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Status Removal", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaximumRemovals = 1;

	bool IsValid() const
	{
		if (MaximumRemovals < 1 || MaximumRemovals > 16 ||
			(EffectIds.IsEmpty() && AnyStatusTags.IsEmpty() && AllowedDispositions.IsEmpty()))
		{
			return false;
		}
		TSet<FName> Seen;
		for (const FName Id : EffectIds)
		{
			if (Id.IsNone() || Seen.Contains(Id))
			{
				return false;
			}
			Seen.Add(Id);
		}
		Seen.Reset();
		for (const FName Tag : AnyStatusTags)
		{
			if (Tag.IsNone() || Seen.Contains(Tag))
			{
				return false;
			}
			Seen.Add(Tag);
		}
		return true;
	}
};

/** C8 reusable target eligibility filter; empty fields are wildcards. */
USTRUCT(BlueprintType)
struct FGridCombatTargetFilterProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target Filter")
	TArray<FName> AllowedMonsterCategoryIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target Filter")
	TArray<FName> RequiredStatusEffectIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target Filter")
	TArray<FName> RequiredStatusTags;

	/** When true, every required status id must have SourceId equal to the acting combatant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target Filter")
	bool bRequiredStatusesFromSource = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target Filter|Vitals")
	bool bRequirePhysicalArmorDepleted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target Filter|Vitals")
	bool bRequireMagicalArmorDepleted = false;

	/** Inclusive current-HP ceiling. Zero disables the health-percentage filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Target Filter|Vitals", meta = (ClampMin = "0", ClampMax = "100"))
	int32 MaximumHealthPercent = 0;

	bool IsValid() const
	{
		TSet<FName> Seen;
		for (const FName Id : AllowedMonsterCategoryIds)
		{
			if (Id.IsNone() || Seen.Contains(Id))
			{
				return false;
			}
			Seen.Add(Id);
		}
		Seen.Reset();
		for (const FName Id : RequiredStatusEffectIds)
		{
			if (Id.IsNone() || Seen.Contains(Id))
			{
				return false;
			}
			Seen.Add(Id);
		}
		Seen.Reset();
		for (const FName Tag : RequiredStatusTags)
		{
			if (Tag.IsNone() || Seen.Contains(Tag))
			{
				return false;
			}
			Seen.Add(Tag);
		}
		return (!bRequiredStatusesFromSource || !RequiredStatusEffectIds.IsEmpty()) &&
			MaximumHealthPercent >= 0 && MaximumHealthPercent <= 100;
	}
};


UENUM(BlueprintType)
enum class EGridCombatArmorPool : uint8
{
	Physical UMETA(DisplayName = "Physical Armor"),
	Magical UMETA(DisplayName = "Magical Armor")
};

UENUM(BlueprintType)
enum class EGridCombatArmorEffectOperation : uint8
{
	Restore UMETA(DisplayName = "Restore"),
	Damage UMETA(DisplayName = "Damage")
};

UENUM(BlueprintType)
enum class EGridCombatArmorEffectMagnitude : uint8
{
	Flat UMETA(DisplayName = "Flat"),
	ReferencePercent UMETA(DisplayName = "Reference Percent"),
	RawDamagePercent UMETA(DisplayName = "Raw Damage Percent")
};

UENUM(BlueprintType)
enum class EGridCombatArmorEffectTrigger : uint8
{
	AfterResolution UMETA(DisplayName = "After Resolution"),
	AfterSuccessfulHit UMETA(DisplayName = "After Successful Hit")
};

UENUM(BlueprintType)
enum class EGridCombatSurfaceType : uint8
{
	None UMETA(DisplayName = "None"),
	Fire UMETA(DisplayName = "Fire"),
	Water UMETA(DisplayName = "Water"),
	Ice UMETA(DisplayName = "Ice"),
	Poison UMETA(DisplayName = "Poison"),
	Oil UMETA(DisplayName = "Oil"),
	ElectrifiedWater UMETA(DisplayName = "Electrified Water"),
	Smoke UMETA(DisplayName = "Smoke"),
	PoisonCloud UMETA(DisplayName = "Poison Cloud")
};

UENUM(BlueprintType)
enum class EGridCombatSurfaceInteraction : uint8
{
	None UMETA(DisplayName = "None"),
	Fire UMETA(DisplayName = "Fire"),
	Ice UMETA(DisplayName = "Ice"),
	Lightning UMETA(DisplayName = "Lightning"),
	Wind UMETA(DisplayName = "Wind")
};

/** C6 authoring payload used to create/replace a persistent runtime cell surface. */
USTRUCT(BlueprintType)
struct FGridCombatSurfaceEffectProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Surface")
	EGridCombatSurfaceType SurfaceType = EGridCombatSurfaceType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Surface", meta = (ClampMin = "1", ClampMax = "6"))
	int32 DurationRounds = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Surface|Periodic Damage")
	EGridDamageType PeriodicDamageType = EGridDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Surface|Periodic Damage", meta = (ClampMin = "0", ClampMax = "1000"))
	int32 PeriodicDamagePerRound = 0;

	/** Optional C1 payload resolved after periodic surface damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Surface|Status")
	TArray<FGridCombatStatusApplicationProfile> PeriodicStatusApplications;

	bool IsValid() const
	{
		if (SurfaceType == EGridCombatSurfaceType::None || DurationRounds < 1 || DurationRounds > 6 || PeriodicDamagePerRound < 0)
		{
			return false;
		}
		for (const FGridCombatStatusApplicationProfile& Status : PeriodicStatusApplications)
		{
			if (!Status.IsValid() || Status.Trigger != EGridCombatStatusApplicationTrigger::AfterResolution)
			{
				return false;
			}
		}
		return true;
	}
};

/** SaveGame-safe authoritative state for one active surface cell. */
USTRUCT(BlueprintType)
struct FGridCombatSurfaceState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Surface")
	EGridCombatSurfaceType SurfaceType = EGridCombatSurfaceType::None;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Surface")
	int32 RemainingRounds = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Surface")
	FGuid SourceCombatantId;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Surface")
	FName SourceActionId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Surface")
	EGridDamageType PeriodicDamageType = EGridDamageType::Physical;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Surface")
	int32 PeriodicDamagePerRound = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Surface")
	TArray<FGridCombatStatusApplicationProfile> PeriodicStatusApplications;

	bool IsValid() const
	{
		if (SurfaceType == EGridCombatSurfaceType::None || RemainingRounds < 1 || RemainingRounds > 6 || PeriodicDamagePerRound < 0)
		{
			return false;
		}
		for (const FGridCombatStatusApplicationProfile& Status : PeriodicStatusApplications)
		{
			if (!Status.IsValid() || Status.Trigger != EGridCombatStatusApplicationTrigger::AfterResolution)
			{
				return false;
			}
		}
		return true;
	}
};

USTRUCT(BlueprintType)
struct FGridCombatSurfaceReactionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Surface")
	bool bReacted = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Surface")
	bool bRemoveSurface = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Surface")
	EGridCombatSurfaceType OutputSurfaceType = EGridCombatSurfaceType::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Surface")
	bool bExplosive = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Surface")
	EGridDamageType ExplosionDamageType = EGridDamageType::Fire;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Surface")
	int32 ExplosionDamagePercentModifier = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Surface")
	int32 ExplosionAreaRadiusModifier = 0;
};

/** Caster-side values consumed by C3 flat-magnitude scaling. */
USTRUCT(BlueprintType)
struct FGridCombatArmorEffectSourceContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor")
	FRPGAttributes Attributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor")
	TArray<FRPGSkillRank> SkillRanks;
};

/**
 * Generic C3 direct armor-pool mutation.
 * Restore is clamped to the target reference pool; Damage never spills into HP.
 */
USTRUCT(BlueprintType)
struct FGridCombatArmorEffectProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor")
	EGridCombatArmorPool Pool = EGridCombatArmorPool::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor")
	EGridCombatArmorEffectOperation Operation = EGridCombatArmorEffectOperation::Restore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor")
	EGridCombatArmorEffectMagnitude Magnitude = EGridCombatArmorEffectMagnitude::Flat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor")
	EGridCombatArmorEffectTrigger Trigger = EGridCombatArmorEffectTrigger::AfterResolution;

	/** Base flat points or percentage, according to Magnitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 Amount = 1;

	/** Optional caster attribute modifier contribution for Flat magnitudes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor|Scaling")
	EGridAttackScalingAttribute ScalingAttribute = EGridAttackScalingAttribute::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor|Scaling", meta = (ClampMin = "0", ClampMax = "10"))
	int32 AttributeModifierScale = 0;

	/** Optional stable RPG SkillId contribution for Flat magnitudes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor|Scaling")
	FName ScalingSkillId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor|Scaling", meta = (ClampMin = "0", ClampMax = "10"))
	int32 SkillRankScale = 0;

	bool IsValid() const
	{
		if (Amount <= 0 || AttributeModifierScale < 0 || SkillRankScale < 0)
		{
			return false;
		}
		const bool bAttributeScalingValid =
			(ScalingAttribute == EGridAttackScalingAttribute::None) == (AttributeModifierScale == 0);
		const bool bSkillScalingValid = ScalingSkillId.IsNone() == (SkillRankScale == 0);
		if (!bAttributeScalingValid || !bSkillScalingValid)
		{
			return false;
		}
		if (Magnitude != EGridCombatArmorEffectMagnitude::Flat && (AttributeModifierScale != 0 || SkillRankScale != 0))
		{
			return false;
		}
		if (Magnitude == EGridCombatArmorEffectMagnitude::RawDamagePercent)
		{
			return Operation == EGridCombatArmorEffectOperation::Damage && Trigger == EGridCombatArmorEffectTrigger::AfterSuccessfulHit;
		}
		return true;
	}
};

USTRUCT(BlueprintType)
struct FGridCombatArmorPoolSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor")
	int32 CurrentPhysicalArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor")
	int32 ReferencePhysicalArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor")
	int32 CurrentMagicalArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Armor")
	int32 ReferenceMagicalArmor = 0;

	bool IsValid() const
	{
		return CurrentPhysicalArmor >= 0 && ReferencePhysicalArmor >= 0 && CurrentMagicalArmor >= 0 && ReferenceMagicalArmor >= 0;
	}
};

USTRUCT(BlueprintType)
struct FGridCombatArmorEffectResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Armor")
	EGridCombatArmorPool Pool = EGridCombatArmorPool::Physical;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Armor")
	EGridCombatArmorEffectOperation Operation = EGridCombatArmorEffectOperation::Restore;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Armor")
	int32 ArmorBefore = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Armor")
	int32 ArmorAfter = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Armor")
	int32 ReferenceArmor = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Armor")
	int32 RequestedAmount = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Armor")
	int32 AppliedAmount = 0;

	bool DidMutate() const
	{
		return AppliedAmount > 0 && ArmorBefore != ArmorAfter;
	}
};

/** Resource costs declared by an action before runtime modifiers. */
USTRUCT(BlueprintType)
struct FGridCombatActionResourceCosts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Costs", meta = (ClampMin = "0"))
	int32 ManaCost = 0;

	/** Number of units consumed from an item source after acceptance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Costs", meta = (ClampMin = "0"))
	int32 SourceItemQuantityCost = 0;

	bool IsValid() const
	{
		return ManaCost >= 0 && SourceItemQuantityCost >= 0;
	}
};

/**
 * Generic C2 combat modifier authored by progression choices.
 * Empty filters are wildcards. Runtime consumers aggregate matching profiles;
 * durable character state never stores the aggregate.
 */
UENUM(BlueprintType)
enum class EGridCombatTargetCondition : uint8
{
	None UMETA(DisplayName = "None"),
	RearArc UMETA(DisplayName = "Rear Arc"),
	HasActedThisRound UMETA(DisplayName = "Has Acted This Round"),
	HasNotActedThisRound UMETA(DisplayName = "Has Not Acted This Round"),
	PhysicalControl UMETA(DisplayName = "Physical Control")
};

USTRUCT(BlueprintType)
struct FGridCombatModifierProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Filter")
	TArray<FName> ActionIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Filter")
	TArray<FName> SourceDefinitionIds;

	/** Every listed tag must be present on the resolved action source. Empty = wildcard. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Filter")
	TArray<FName> RequiredSourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Filter")
	TArray<EGridCombatActionSourcePolicy> SourcePolicies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Filter")
	TArray<EGridCombatActionType> ActionTypes;

	/** Empty = any targeting policy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Filter")
	TArray<EGridCombatTargetingPolicy> TargetingPolicies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Filter")
	TArray<EGridDamageType> DamageTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Filter")
	TArray<EGridPhysicalDamageSubtype> PhysicalSubtypes;

	/** Requirements owned by the character before this profile is projected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Filter")
	TArray<FName> RequiredOwnerRequirementIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Target")
	TArray<EGridCombatTargetCondition> RequiredTargetConditions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Target")
	TArray<EGridCombatTargetCondition> AnyTargetConditions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Target")
	bool bExcludeAreaActions = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Target")
	TArray<FName> RequiredTargetStatusEffectIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Target")
	bool bRequiredTargetStatusesFromOwner = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Target")
	TArray<FName> AllowedTargetMonsterCategoryIds;

	/** OR filter over generic semantic tags authored by the target definition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Target")
	TArray<FName> AnyTargetSemanticTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Target")
	bool bRequirePartyStationarySincePreviousActivation = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier")
	int32 AccuracyModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier")
	int32 EvasionModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier", meta = (ClampMin = "-100", ClampMax = "500"))
	int32 OutgoingDamagePercentModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier", meta = (ClampMin = "-100", ClampMax = "500"))
	int32 IncomingDamagePercentModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier", meta = (ClampMin = "-100", ClampMax = "100"))
	int32 CriticalChancePercentModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier", meta = (ClampMin = "-100", ClampMax = "800"))
	int32 CriticalDamagePercentModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier", meta = (ClampMin = "-500", ClampMax = "500"))
	int32 WeaponDamagePercentModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier")
	FGridDamageResistanceSet ResistanceModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier", meta = (ClampMin = "-6", ClampMax = "6"))
	int32 ActionPointCostModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier", meta = (ClampMin = "-100", ClampMax = "100"))
	int32 ManaCostModifier = 0;

	/** Positive-cost actions are clamped to this floor after mana modifiers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier", meta = (ClampMin = "0", ClampMax = "100"))
	int32 MinimumManaCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier", meta = (ClampMin = "-32", ClampMax = "32"))
	int32 RangeCellsModifier = 0;

	/** C7 positive Health/Mana/Armor magnitude modifier after skill scaling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Quick Item", meta = (ClampMin = "-100", ClampMax = "500"))
	int32 PositiveEffectPercentModifier = 0;

	/** Direct-damage modifier when an Area action hits an allied target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Area", meta = (ClampMin = "-100", ClampMax = "500"))
	int32 FriendlyDirectDamagePercentModifier = 0;

	/** Additional direct-damage modifier when the Area action hits its own source character. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Area", meta = (ClampMin = "-100", ClampMax = "500"))
	int32 SelfDirectDamagePercentModifier = 0;

	/** C7/C8 number of extra living allies receiving a secondary positive QuickItem effect. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Quick Item", meta = (ClampMin = "0", ClampMax = "6"))
	int32 QuickItemSecondaryTargetCount = 0;

	/** C7 future C8 hook: secondary positive effect magnitude as percent of primary. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Quick Item", meta = (ClampMin = "0", ClampMax = "100"))
	int32 QuickItemSecondaryMagnitudePercent = 0;

	/** C7 future C8 hook: secondary status duration as percent of primary. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Quick Item", meta = (ClampMin = "0", ClampMax = "100"))
	int32 QuickItemSecondaryDurationPercent = 0;

	/** Percentage modifier to the target's reconstructible physical armor reference pool. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Armor", meta = (ClampMin = "-100", ClampMax = "500"))
	int32 PhysicalArmorReferencePercentModifier = 0;

	/** Percentage modifier to the target's reconstructible magical armor reference pool. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Armor", meta = (ClampMin = "-100", ClampMax = "500"))
	int32 MagicalArmorReferencePercentModifier = 0;

	/** Percentage modifier to physical-armor restoration received by the target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Armor", meta = (ClampMin = "-100", ClampMax = "500"))
	int32 PhysicalArmorRestorationPercentModifier = 0;

	/** Percentage modifier to magical-armor restoration received by the target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Armor", meta = (ClampMin = "-100", ClampMax = "500"))
	int32 MagicalArmorRestorationPercentModifier = 0;

	/** C6 bonus applied only when the owner creates a surface from the matching action. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Surface", meta = (ClampMin = "-6", ClampMax = "6"))
	int32 SurfaceDurationRoundsModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Surface", meta = (ClampMin = "-100", ClampMax = "500"))
	int32 SurfacePeriodicDamagePercentModifier = 0;

	/** C4+C6 modifier applied to explosive surface reactions triggered by the matching action. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Surface", meta = (ClampMin = "-100", ClampMax = "500"))
	int32 SurfaceReactionDamagePercentModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Modifier|Surface", meta = (ClampMin = "-8", ClampMax = "8"))
	int32 SurfaceReactionAreaRadiusModifier = 0;

	bool HasAnyModifier() const
	{
		return AccuracyModifier != 0 || EvasionModifier != 0 || OutgoingDamagePercentModifier != 0 || IncomingDamagePercentModifier != 0 ||
			CriticalChancePercentModifier != 0 || CriticalDamagePercentModifier != 0 || WeaponDamagePercentModifier != 0 ||
			!ResistanceModifiers.IsEmpty() || ActionPointCostModifier != 0 ||
			ManaCostModifier != 0 || MinimumManaCost != 0 || RangeCellsModifier != 0 || PositiveEffectPercentModifier != 0 ||
			FriendlyDirectDamagePercentModifier != 0 || SelfDirectDamagePercentModifier != 0 ||
			QuickItemSecondaryTargetCount != 0 || QuickItemSecondaryMagnitudePercent != 0 ||
			QuickItemSecondaryDurationPercent != 0 || PhysicalArmorReferencePercentModifier != 0 ||
			MagicalArmorReferencePercentModifier != 0 || PhysicalArmorRestorationPercentModifier != 0 ||
			MagicalArmorRestorationPercentModifier != 0 || SurfaceDurationRoundsModifier != 0 ||
			SurfacePeriodicDamagePercentModifier != 0 || SurfaceReactionDamagePercentModifier != 0 ||
			SurfaceReactionAreaRadiusModifier != 0;
	}

	bool IsValid() const
	{
		if (!HasAnyModifier() || OutgoingDamagePercentModifier < -100 || OutgoingDamagePercentModifier > 500 ||
			IncomingDamagePercentModifier < -100 || IncomingDamagePercentModifier > 500 || CriticalChancePercentModifier < -100 ||
			CriticalChancePercentModifier > 100 || CriticalDamagePercentModifier < -100 || CriticalDamagePercentModifier > 800 ||
			WeaponDamagePercentModifier < -500 || WeaponDamagePercentModifier > 500 || ActionPointCostModifier < -6 || ActionPointCostModifier > 6 || ManaCostModifier < -100 || ManaCostModifier > 100 ||
			MinimumManaCost < 0 || MinimumManaCost > 100 || RangeCellsModifier < -32 || RangeCellsModifier > 32 || PositiveEffectPercentModifier < -100 || PositiveEffectPercentModifier > 500 ||
			FriendlyDirectDamagePercentModifier < -100 || FriendlyDirectDamagePercentModifier > 500 ||
			SelfDirectDamagePercentModifier < -100 || SelfDirectDamagePercentModifier > 500 || QuickItemSecondaryTargetCount < 0 ||
			QuickItemSecondaryTargetCount > 6 || QuickItemSecondaryMagnitudePercent < 0 || QuickItemSecondaryMagnitudePercent > 100 ||
			QuickItemSecondaryDurationPercent < 0 || QuickItemSecondaryDurationPercent > 100 || PhysicalArmorReferencePercentModifier < -100 ||
			PhysicalArmorReferencePercentModifier > 500 || MagicalArmorReferencePercentModifier < -100 ||
			MagicalArmorReferencePercentModifier > 500 || PhysicalArmorRestorationPercentModifier < -100 ||
			PhysicalArmorRestorationPercentModifier > 500 || MagicalArmorRestorationPercentModifier < -100 ||
			MagicalArmorRestorationPercentModifier > 500 || SurfaceDurationRoundsModifier < -6 || SurfaceDurationRoundsModifier > 6 ||
			SurfacePeriodicDamagePercentModifier < -100 || SurfacePeriodicDamagePercentModifier > 500 ||
			SurfaceReactionDamagePercentModifier < -100 || SurfaceReactionDamagePercentModifier > 500 ||
			SurfaceReactionAreaRadiusModifier < -8 || SurfaceReactionAreaRadiusModifier > 8)
		{
			return false;
		}
		for (const FName Id : ActionIds)
		{
			if (Id.IsNone())
			{
				return false;
			}
		}
		for (const FName Id : SourceDefinitionIds)
		{
			if (Id.IsNone())
			{
				return false;
			}
		}
		{
			TSet<FName> SeenTags;
			for (const FName Tag : RequiredSourceTags)
			{
				if (Tag.IsNone() || SeenTags.Contains(Tag))
				{
					return false;
				}
				SeenTags.Add(Tag);
			}
		}
		for (const EGridCombatActionSourcePolicy Policy : SourcePolicies)
		{
			if (Policy == EGridCombatActionSourcePolicy::None)
			{
				return false;
			}
		}
		for (const EGridCombatActionType Type : ActionTypes)
		{
			if (Type == EGridCombatActionType::None)
			{
				return false;
			}
		}
		{
			TSet<EGridCombatTargetingPolicy> SeenTargetingPolicies;
			for (const EGridCombatTargetingPolicy Policy : TargetingPolicies)
			{
				if (Policy == EGridCombatTargetingPolicy::None || SeenTargetingPolicies.Contains(Policy))
				{
					return false;
				}
				SeenTargetingPolicies.Add(Policy);
			}
		}
		for (const EGridPhysicalDamageSubtype Subtype : PhysicalSubtypes)
		{
			if (Subtype == EGridPhysicalDamageSubtype::None)
			{
				return false;
			}
		}
		{
			TSet<FName> SeenOwnerRequirements;
			for (const FName RequirementId : RequiredOwnerRequirementIds)
			{
				if (RequirementId.IsNone() || SeenOwnerRequirements.Contains(RequirementId))
				{
					return false;
				}
				SeenOwnerRequirements.Add(RequirementId);
			}
		}
		auto ConditionsValid = [](const TArray<EGridCombatTargetCondition>& Conditions)
		{
			TSet<EGridCombatTargetCondition> Seen;
			for (const EGridCombatTargetCondition Condition : Conditions)
			{
				if (Condition == EGridCombatTargetCondition::None || Seen.Contains(Condition))
				{
					return false;
				}
				Seen.Add(Condition);
			}
			return true;
		};
		TSet<FName> SeenTargetStatuses;
		for (const FName EffectId : RequiredTargetStatusEffectIds)
		{
			if (EffectId.IsNone() || SeenTargetStatuses.Contains(EffectId))
			{
				return false;
			}
			SeenTargetStatuses.Add(EffectId);
		}
		TSet<FName> SeenCategories;
		for (const FName CategoryId : AllowedTargetMonsterCategoryIds)
		{
			if (CategoryId.IsNone() || SeenCategories.Contains(CategoryId))
			{
				return false;
			}
			SeenCategories.Add(CategoryId);
		}
		TSet<FName> SeenSemanticTags;
		for (const FName Tag : AnyTargetSemanticTags)
		{
			if (Tag.IsNone() || SeenSemanticTags.Contains(Tag))
			{
				return false;
			}
			SeenSemanticTags.Add(Tag);
		}
		return (!bRequiredTargetStatusesFromOwner || !RequiredTargetStatusEffectIds.IsEmpty()) &&
			ConditionsValid(RequiredTargetConditions) && ConditionsValid(AnyTargetConditions);
	}
};

/** Optional C7 Alchemy/QuickItem skill-rank scaling. */
USTRUCT(BlueprintType)
struct FGridCombatQuickItemScalingProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Quick Item|Scaling")
	FName ScalingSkillId = NAME_None;

	/** Added to direct attack DamageBonus for each rank of ScalingSkillId. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Quick Item|Scaling", meta = (ClampMin = "0", ClampMax = "10"))
	int32 DirectDamageSkillRankScale = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Quick Item|Scaling", meta = (ClampMin = "0", ClampMax = "10"))
	int32 RestoreHealthSkillRankScale = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Quick Item|Scaling", meta = (ClampMin = "0", ClampMax = "10"))
	int32 RestoreManaSkillRankScale = 0;

	bool HasAnyScaling() const
	{
		return DirectDamageSkillRankScale > 0 || RestoreHealthSkillRankScale > 0 || RestoreManaSkillRankScale > 0;
	}

	bool IsValid() const
	{
		if (DirectDamageSkillRankScale < 0 || RestoreHealthSkillRankScale < 0 || RestoreManaSkillRankScale < 0)
		{
			return false;
		}
		return HasAnyScaling() ? !ScalingSkillId.IsNone() : ScalingSkillId.IsNone();
	}
};

/** Generic rank scaling added to direct attack damage after attribute scaling. */
USTRUCT(BlueprintType)
struct FGridCombatDirectDamageScalingProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Direct Damage Scaling")
	FName ScalingSkillId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Direct Damage Scaling", meta = (ClampMin = "0", ClampMax = "10"))
	int32 SkillRankScale = 0;

	bool IsValid() const
	{
		return SkillRankScale >= 0 && SkillRankScale <= 10 &&
			(SkillRankScale > 0 ? !ScalingSkillId.IsNone() : ScalingSkillId.IsNone());
	}
};

/**
 * Optional owner-requirement projection for one canonical action.
 * Exactly one matching variant is applied at catalogue projection time.
 */
USTRUCT(BlueprintType)
struct FGridCombatActionOwnerVariantProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Owner Variant")
	TArray<FName> RequiredOwnerRequirementIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Owner Variant")
	TArray<FName> AddedSourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Owner Variant")
	bool bOverrideDamageDescriptor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Owner Variant")
	EGridDamageType OverrideDamageType = EGridDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Owner Variant")
	EGridPhysicalDamageSubtype OverridePhysicalSubtype = EGridPhysicalDamageSubtype::None;

	/** Variant-only statuses appended to the canonical action after projection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Owner Variant")
	TArray<FGridCombatStatusApplicationProfile> StatusApplications;

	bool IsValid() const
	{
		if (RequiredOwnerRequirementIds.IsEmpty() ||
			(bOverrideDamageDescriptor && OverrideDamageType != EGridDamageType::Physical &&
				OverridePhysicalSubtype != EGridPhysicalDamageSubtype::None))
		{
			return false;
		}
		TSet<FName> Seen;
		for (const FName RequirementId : RequiredOwnerRequirementIds)
		{
			if (RequirementId.IsNone() || Seen.Contains(RequirementId))
			{
				return false;
			}
			Seen.Add(RequirementId);
		}
		Seen.Reset();
		for (const FName Tag : AddedSourceTags)
		{
			if (Tag.IsNone() || Seen.Contains(Tag))
			{
				return false;
			}
			Seen.Add(Tag);
		}
		return StatusApplications.ContainsByPredicate(
			[](const FGridCombatStatusApplicationProfile& Profile)
			{
				return !Profile.IsValid();
			}) == false;
	}
};

/** Immediate self-targeted restoration used by supported combat effects. */
USTRUCT(BlueprintType)
struct FGridCombatActionEffectProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Effect", meta = (ClampMin = "0"))
	int32 RestoreHealth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Effect", meta = (ClampMin = "0"))
	int32 RestoreMana = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Effect", meta = (ClampMin = "0", ClampMax = "100"))
	int32 RestoreHealthMaximumPercent = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Effect", meta = (ClampMin = "0", ClampMax = "100"))
	int32 RestoreManaMaximumPercent = 0;

	static int32 ResolveMaximumPercentAmount(int32 MaximumValue, int32 Percent)
	{
		if (MaximumValue <= 0 || Percent <= 0)
		{
			return 0;
		}
		const int64 Numerator = static_cast<int64>(MaximumValue) * static_cast<int64>(Percent);
		return FMath::Max(1, static_cast<int32>(FMath::Clamp<int64>((Numerator + 99) / 100, 0, MAX_int32)));
	}

	int32 ResolveHealthRestore(int32 MaximumHealth) const
	{
		return FMath::Max(0, RestoreHealth) + ResolveMaximumPercentAmount(MaximumHealth, RestoreHealthMaximumPercent);
	}

	int32 ResolveManaRestore(int32 MaximumMana) const
	{
		return FMath::Max(0, RestoreMana) + ResolveMaximumPercentAmount(MaximumMana, RestoreManaMaximumPercent);
	}

	bool IsValid() const
	{
		return RestoreHealth >= 0 && RestoreMana >= 0 && RestoreHealthMaximumPercent >= 0 && RestoreHealthMaximumPercent <= 100 &&
			RestoreManaMaximumPercent >= 0 && RestoreManaMaximumPercent <= 100 &&
			(RestoreHealth > 0 || RestoreMana > 0 || RestoreHealthMaximumPercent > 0 || RestoreManaMaximumPercent > 0);
	}
};

/**
 * Data-oriented definition shared by equipment, abilities and spells.
 * Resolution and presentation stay separate from catalogue construction.
 */
USTRUCT(BlueprintType)
struct FGridCombatTrapEffectProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Trap")
	bool bPlaceTrap = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Trap")
	FName TrapId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Trap", meta = (ClampMin = "1", ClampMax = "6"))
	int32 DurationRounds = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Trap", meta = (ClampMin = "0", ClampMax = "1000"))
	int32 BaseDamage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Trap")
	EGridDamageType DamageType = EGridDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Trap")
	EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Trap")
	EGridAttackScalingAttribute DamageScalingAttribute = EGridAttackScalingAttribute::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Trap")
	TArray<FGridCombatStatusApplicationProfile> StatusApplications;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Trap")
	bool bConsumeOnTrigger = true;

	bool IsValid() const
	{
		if (!bPlaceTrap) return true;
		if (TrapId.IsNone() || DurationRounds < 1 || DurationRounds > 6 || BaseDamage < 0 ||
			(DamageType != EGridDamageType::Physical && PhysicalSubtype != EGridPhysicalDamageSubtype::None))
		{
			return false;
		}
		for (const FGridCombatStatusApplicationProfile& Profile : StatusApplications)
		{
			if (!Profile.IsValid()) return false;
		}
		return true;
	}
};

USTRUCT(BlueprintType)
struct FGridCombatTrapState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Trap")
	FName TrapId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Trap")
	int32 RemainingRounds = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Trap")
	FGuid SourceCombatantId;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Trap")
	FName SourceActionId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Trap")
	int32 RawDamage = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Trap")
	EGridDamageType DamageType = EGridDamageType::Physical;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Trap")
	EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Trap")
	TArray<FGridCombatStatusApplicationProfile> StatusApplications;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Combat|Trap")
	bool bConsumeOnTrigger = true;

	bool IsValid() const
	{
		if (TrapId.IsNone() || RemainingRounds <= 0 || RawDamage < 0 ||
			(DamageType != EGridDamageType::Physical && PhysicalSubtype != EGridPhysicalDamageSubtype::None))
		{
			return false;
		}
		for (const FGridCombatStatusApplicationProfile& Profile : StatusApplications)
		{
			if (!Profile.IsValid())
			{
				return false;
			}
		}
		return true;
	}
};

USTRUCT(BlueprintType)
struct FGridCombatSkillCheckProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Skill Check")
	FName SkillId = NAME_None;

	/** Uses the target definition's generic SkillCheckDifficulty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Skill Check")
	bool bUseTargetDifficulty = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Skill Check", meta = (ClampMin = "0", ClampMax = "40"))
	int32 FixedDifficulty = 0;

	bool IsEnabled() const
	{
		return !SkillId.IsNone();
	}

	bool IsValid() const
	{
		if (!IsEnabled())
		{
			return !bUseTargetDifficulty && FixedDifficulty == 0;
		}
		return bUseTargetDifficulty ? FixedDifficulty == 0 : FixedDifficulty > 0 && FixedDifficulty <= 40;
	}
};

USTRUCT(BlueprintType)
struct FGridCombatActionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	FName ActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	EGridCombatActionType ActionType = EGridCombatActionType::Ability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	EGridCombatActionSourcePolicy SourcePolicy = EGridCombatActionSourcePolicy::None;

	/** C7 semantic tags projected from the source item; combat logic matches tags, never TalentIds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Source")
	TArray<FName> SourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	EGridCombatTargetingPolicy TargetingPolicy = EGridCombatTargetingPolicy::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	EGridCombatActionResolutionProfile ResolutionProfile = EGridCombatActionResolutionProfile::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Costs", meta = (ClampMin = "0", ClampMax = "6"))
	int32 ActionPointCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Costs")
	FGridCombatActionResourceCosts ResourceCosts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Targeting", meta = (ClampMin = "0", ClampMax = "32"))
	int32 RangeCells = 0;

	/** Grid LOS to the selected hostile/cell must be clear before committing the action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Targeting")
	bool bRequiresLineOfSight = false;

	/** C8 optional eligibility filter for the target(s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Targeting")
	FGridCombatTargetFilterProfile TargetFilter;

	/** 0 = every eligible target; otherwise deterministic cap after ordering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Targeting", meta = (ClampMin = "0", ClampMax = "16"))
	int32 MaximumResolvedTargets = 0;

	/** Optional deterministic chain: every next hostile must be within this Manhattan distance of the previous one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Targeting", meta = (ClampMin = "0", ClampMax = "8"))
	int32 ChainJumpRangeCells = 0;

	/** Independent resolutions against each target (Rapid Shot = 2); resources are paid once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Resolution", meta = (ClampMin = "1", ClampMax = "8"))
	int32 ResolutionCount = 1;

	/** Accuracy delta applied to every attack resolution after the first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Resolution", meta = (ClampMin = "-20", ClampMax = "20"))
	int32 SubsequentResolutionAccuracyModifier = 0;

	/** Manhattan radius around the selected cell when TargetingPolicy is Area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Targeting", meta = (ClampMin = "0", ClampMax = "8"))
	int32 AreaRadiusCells = 0;

	/** C8 opt-in: Area attack direct damage/statuses may also affect living party members on covered party cells. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Targeting")
	bool bAffectsAlliesInArea = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Requirements")
	TArray<FName> Requirements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Cooldown", meta = (ClampMin = "0"))
	int32 CooldownRounds = 0;

	/** Stable key resolved by the presentation layer; NAME_None uses source defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Presentation")
	FName PresentationProfileId = NAME_None;

	/**
	 * Optional RPG03.9 weapon projection. When enabled, OffensiveProfile stays
	 * empty and the runtime resolves the equipped weapon exactly once per action.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Resolution")
	FGridCombatWeaponAttackProfile WeaponAttackProfile;

	/** Attack payload used when ResolutionProfile is Attack and WeaponAttackProfile is disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Resolution")
	FGridOffensiveEquipmentProfile OffensiveProfile;

	/** Immediate self payload used by supported Effect actions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Resolution")
	FGridCombatActionEffectProfile EffectProfile;

	/** Optional generic direct-damage skill scaling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Resolution")
	FGridCombatDirectDamageScalingProfile DirectDamageScaling;

	/** Optional C7 skill scaling used only when SourcePolicy=QuickItem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Quick Item")
	FGridCombatQuickItemScalingProfile QuickItemScaling;

	/** Optional owner-conditioned projection data; removed from runtime copies after selection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Owner Variant")
	TArray<FGridCombatActionOwnerVariantProfile> OwnerVariants;

	/** C1 secondary status applications resolved after the primary action/effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Status")
	TArray<FGridCombatStatusApplicationProfile> StatusApplications;

	/** C8 deterministic status removals resolved through the MON16 lifecycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Status")
	TArray<FGridCombatStatusRemovalProfile> StatusRemovals;

	/** C3 direct armor effects resolved before C1 ArmorGate evaluation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Armor")
	TArray<FGridCombatArmorEffectProfile> ArmorEffects;

	/** C5 tactical/forced movement effects. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Movement")
	TArray<FGridCombatMovementEffectProfile> MovementEffects;

	/** C5 opt-in: relocate the whole party directly to the selected Cell without PAM or cell-transition events. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Movement")
	bool bRelocatePartyToTargetCell = false;

	/** C6 persistent cell-surface creation payloads; C8 owns cell/area execution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Surface")
	TArray<FGridCombatSurfaceEffectProfile> SurfaceEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Trap")
	FGridCombatTrapEffectProfile TrapEffect;

	/** Optional generic targeted Skill Check gate. Failed checks still resolve/pay the attempted action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action|Skill Check")
	FGridCombatSkillCheckProfile SkillCheck;

	bool IsValid() const
	{
		const bool bWeaponAttackProfileValid = WeaponAttackProfile.IsValid() &&
			(!WeaponAttackProfile.bUseEquippedWeapon ||
				(ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
					(SourcePolicy == EGridCombatActionSourcePolicy::Ability || SourcePolicy == EGridCombatActionSourcePolicy::Spell)));
		const bool bAttackProfileValid = ResolutionProfile != EGridCombatActionResolutionProfile::Attack ||
			(ActionPointCost > 0 &&
				(WeaponAttackProfile.bUseEquippedWeapon ? !OffensiveProfile.IsValid() : OffensiveProfile.IsValid()));
		const bool bAttackRangeValid = ResolutionProfile != EGridCombatActionResolutionProfile::Attack ||
			WeaponAttackProfile.bUseEquippedWeapon || RangeCells == OffensiveProfile.RangeCells;
		const bool bTargetingRangeValid = (TargetingPolicy != EGridCombatTargetingPolicy::FirstAxialTarget &&
											  TargetingPolicy != EGridCombatTargetingPolicy::Cell && TargetingPolicy != EGridCombatTargetingPolicy::Area &&
											  TargetingPolicy != EGridCombatTargetingPolicy::Hostile &&
											  TargetingPolicy != EGridCombatTargetingPolicy::AllyOrHostile) ||
			RangeCells > 0;
		const bool bAreaRadiusValid = TargetingPolicy != EGridCombatTargetingPolicy::Area || AreaRadiusCells > 0;
		const bool bLineOfSightValid = !bRequiresLineOfSight ||
			TargetingPolicy == EGridCombatTargetingPolicy::FirstAxialTarget ||
			TargetingPolicy == EGridCombatTargetingPolicy::Hostile ||
			TargetingPolicy == EGridCombatTargetingPolicy::AllyOrHostile ||
			TargetingPolicy == EGridCombatTargetingPolicy::Cell ||
			TargetingPolicy == EGridCombatTargetingPolicy::Area;
		const bool bFriendlyAreaValid = !bAffectsAlliesInArea ||
			(TargetingPolicy == EGridCombatTargetingPolicy::Area && ResolutionProfile == EGridCombatActionResolutionProfile::Attack);
		const bool bStatusApplicationsValid = StatusApplications.ContainsByPredicate(
			[this](const FGridCombatStatusApplicationProfile& Profile)
			{
				return !Profile.IsValid() ||
					(Profile.Trigger == EGridCombatStatusApplicationTrigger::AfterSuccessfulHit &&
						ResolutionProfile != EGridCombatActionResolutionProfile::Attack);
			}) == false;
		const bool bStatusRemovalsValid = StatusRemovals.ContainsByPredicate(
			[](const FGridCombatStatusRemovalProfile& Profile)
			{
				return !Profile.IsValid();
			}) == false && (StatusRemovals.IsEmpty() || ResolutionProfile == EGridCombatActionResolutionProfile::Effect);
		const bool bArmorEffectsValid = ArmorEffects.ContainsByPredicate(
			[this](const FGridCombatArmorEffectProfile& Profile)
			{
				if (!Profile.IsValid())
				{
					return true;
				}
				if (ResolutionProfile == EGridCombatActionResolutionProfile::Attack)
				{
					return Profile.Operation != EGridCombatArmorEffectOperation::Damage ||
						Profile.Trigger != EGridCombatArmorEffectTrigger::AfterSuccessfulHit;
				}
				if (ResolutionProfile != EGridCombatActionResolutionProfile::Effect ||
					Profile.Trigger != EGridCombatArmorEffectTrigger::AfterResolution)
				{
					return true;
				}
				return Profile.Operation == EGridCombatArmorEffectOperation::Damage &&
					Profile.Magnitude == EGridCombatArmorEffectMagnitude::RawDamagePercent;
			}) == false;
		const bool bMovementEffectsValid = MovementEffects.ContainsByPredicate(
			[this](const FGridCombatMovementEffectProfile& Profile)
			{
				return !Profile.IsValid() || ResolutionProfile != EGridCombatActionResolutionProfile::Effect;
			}) == false;
		const bool bSelectedCellRelocationValid = !bRelocatePartyToTargetCell ||
			(ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
				TargetingPolicy == EGridCombatTargetingPolicy::Cell && RangeCells > 0 && MovementEffects.IsEmpty());
		const bool bTrapEffectValid = TrapEffect.IsValid() &&
			(!TrapEffect.bPlaceTrap ||
				(ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
					(TargetingPolicy == EGridCombatTargetingPolicy::Cell || TargetingPolicy == EGridCombatTargetingPolicy::Area)));
		const bool bSkillCheckValid = SkillCheck.IsValid() &&
			(!SkillCheck.IsEnabled() ||
				(ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
					TargetingPolicy == EGridCombatTargetingPolicy::Hostile));
		const bool bSurfaceEffectsValid = SurfaceEffects.ContainsByPredicate(
			[this](const FGridCombatSurfaceEffectProfile& Profile)
			{
				return !Profile.IsValid() ||
					(ResolutionProfile != EGridCombatActionResolutionProfile::Effect &&
						ResolutionProfile != EGridCombatActionResolutionProfile::Attack);
			}) == false;
		bool bSourceTagsValid = true;
		{
			TSet<FName> SeenTags;
			for (const FName Tag : SourceTags)
			{
				if (Tag.IsNone() || SeenTags.Contains(Tag))
				{
					bSourceTagsValid = false;
					break;
				}
				SeenTags.Add(Tag);
			}
		}
		const bool bDirectDamageScalingValid =
			DirectDamageScaling.IsValid() && (DirectDamageScaling.SkillRankScale == 0 || ResolutionProfile == EGridCombatActionResolutionProfile::Attack);
		const bool bQuickItemScalingValid =
			QuickItemScaling.IsValid() && (!QuickItemScaling.HasAnyScaling() || SourcePolicy == EGridCombatActionSourcePolicy::QuickItem);
		const bool bChainValid = ChainJumpRangeCells >= 0 && ChainJumpRangeCells <= 8 &&
			(ChainJumpRangeCells == 0 ||
				(ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
					TargetingPolicy == EGridCombatTargetingPolicy::Cell && MaximumResolvedTargets >= 2));
		const bool bOwnerVariantsValid = OwnerVariants.ContainsByPredicate(
			[this](const FGridCombatActionOwnerVariantProfile& Variant)
			{
				if (!Variant.IsValid() || (Variant.bOverrideDamageDescriptor &&
					(ResolutionProfile != EGridCombatActionResolutionProfile::Attack || !OffensiveProfile.IsValid())))
				{
					return true;
				}
				return Variant.StatusApplications.ContainsByPredicate(
					[this](const FGridCombatStatusApplicationProfile& Profile)
					{
						return Profile.Trigger == EGridCombatStatusApplicationTrigger::AfterSuccessfulHit &&
							ResolutionProfile != EGridCombatActionResolutionProfile::Attack;
					});
			}) == false;
		return !ActionId.IsNone() && ActionType != EGridCombatActionType::None && SourcePolicy != EGridCombatActionSourcePolicy::None &&
			TargetingPolicy != EGridCombatTargetingPolicy::None && ResolutionProfile != EGridCombatActionResolutionProfile::None && ActionPointCost >= 0 &&
			ActionPointCost <= 6 && ResourceCosts.IsValid() && RangeCells >= 0 && RangeCells <= 32 && AreaRadiusCells >= 0 && AreaRadiusCells <= 8 &&
			CooldownRounds >= 0 && MaximumResolvedTargets >= 0 && MaximumResolvedTargets <= 16 && ResolutionCount >= 1 && ResolutionCount <= 8 &&
			SubsequentResolutionAccuracyModifier >= -20 && SubsequentResolutionAccuracyModifier <= 20 &&
			(ResolutionCount > 1 || SubsequentResolutionAccuracyModifier == 0) && TargetFilter.IsValid() && bWeaponAttackProfileValid &&
			bAttackProfileValid && bAttackRangeValid && bTargetingRangeValid && bAreaRadiusValid && bLineOfSightValid && bFriendlyAreaValid &&
			bStatusApplicationsValid &&
			bStatusRemovalsValid && bArmorEffectsValid && bMovementEffectsValid && bSelectedCellRelocationValid &&
			bTrapEffectValid && bSkillCheckValid && bSurfaceEffectsValid &&
			bSourceTagsValid && bDirectDamageScalingValid && bQuickItemScalingValid && bChainValid && bOwnerVariantsValid &&
			(ResolutionCount == 1 || ResolutionProfile == EGridCombatActionResolutionProfile::Attack);
	}
};

/** One concrete source contributing a definition to the catalogue builder. */
USTRUCT(BlueprintType)
struct FGridCombatActionContribution
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Catalog")
	FGridCombatActionDefinition Definition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Catalog")
	FName SourceDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Catalog")
	FGuid SourceRuntimeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Catalog")
	EGridEquipmentSlot SourceEquipmentSlot = EGridEquipmentSlot::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Catalog")
	int32 AvailableSourceQuantity = 0;

	bool IsValid() const
	{
		return Definition.IsValid() && (Definition.SourcePolicy == EGridCombatActionSourcePolicy::Universal || !SourceDefinitionId.IsNone());
	}
};

/** Transient, UI-ready result produced without resolving or paying an action. */
USTRUCT(BlueprintType)
struct FGridAvailableCombatAction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	FGridCombatActionDefinition Definition;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	int32 CharacterIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	FGuid CharacterId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	FName SourceDefinitionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	FGuid SourceRuntimeId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	EGridEquipmentSlot SourceEquipmentSlot = EGridEquipmentSlot::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	int32 CurrentActionPointCost = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	int32 CurrentManaCost = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	int32 CurrentSourceItemQuantityCost = 0;

	/** Total matching units across every inventory stack. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	int32 CurrentSourceItemQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	bool bEnabled = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	EGridCombatActionAvailabilityReason AvailabilityReason = EGridCombatActionAvailabilityReason::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	FText DisabledReason;

	/** Stable, deterministic RequirementIds not currently satisfied. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	TArray<FName> MissingRequirements;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	FGuid SuggestedTargetId;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action Catalog")
	FIntPoint SuggestedTargetCell = FIntPoint::ZeroValue;

	bool MatchesSource(FName ActionId, EGridCombatActionSourcePolicy SourcePolicy, FName InSourceDefinitionId, EGridEquipmentSlot InSourceEquipmentSlot) const
	{
		return Definition.ActionId == ActionId && Definition.SourcePolicy == SourcePolicy && SourceDefinitionId == InSourceDefinitionId &&
			SourceEquipmentSlot == InSourceEquipmentSlot;
	}

	bool IsValid() const
	{
		return CharacterIndex != INDEX_NONE && CharacterId.IsValid() && Definition.IsValid() && CurrentActionPointCost >= 0 && CurrentManaCost >= 0 &&
			CurrentSourceItemQuantityCost >= 0 && CurrentSourceItemQuantity >= 0;
	}
};

USTRUCT(BlueprintType)
struct FGridCombatAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	FGuid ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	EGridCombatActionType Type = EGridCombatActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	FGuid SourceActorId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	FGuid TargetActorId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	int32 TargetCharacterIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	FIntPoint TargetCell = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
	FName AttackId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action", meta = (ClampMin = "0"))
	int32 ActionPointCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
	bool bHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
	bool bCriticalHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
	int32 RolledDamage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
	bool bOutcomeCommitted = false;

	/** MON7 marks the free turn and move that belong to a post-attack retreat. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Action")
	bool bIsRepositioningAction = false;

	bool IsValid() const
	{
		return Type != EGridCombatActionType::None && ActionPointCost >= 0;
	}
};

USTRUCT(BlueprintType)
struct FGridAttackResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	bool bHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	bool bCriticalHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	int32 NaturalAttackRoll = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	int32 AttackRoll = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	int32 DefenseValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	int32 RawDamage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	int32 ResistancePercent = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	float DamageMultiplier = 1.0f;

	/** Damage remaining after multipliers and percentage resistance, before armor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	int32 DamageAfterModifiers = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	int32 PhysicalArmorDamage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	int32 MagicalArmorDamage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	int32 HealthDamage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	int32 TargetHealthBefore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	int32 TargetHealthAfter = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	EGridDamageType DamageType = EGridDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
	EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

	int32 GetTotalAppliedDamage() const
	{
		return PhysicalArmorDamage + MagicalArmorDamage + HealthDamage;
	}
};

/** Resource and vital snapshot produced by a QuickItem combat action. */
USTRUCT(BlueprintType)
struct FGridCombatQuickItemResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Quick Item")
	int32 SourceQuantityBefore = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Quick Item")
	int32 SourceQuantityAfter = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Quick Item")
	int32 HealthBefore = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Quick Item")
	int32 HealthAfter = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Quick Item")
	int32 ManaBefore = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Quick Item")
	int32 ManaAfter = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Quick Item")
	int32 PhysicalArmorBefore = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Quick Item")
	int32 PhysicalArmorAfter = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Quick Item")
	int32 MagicalArmorBefore = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Quick Item")
	int32 MagicalArmorAfter = 0;
};

/** Resource snapshot produced by a non-item ability or spell. */
USTRUCT(BlueprintType)
struct FGridCombatClassActionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Class Action")
	int32 HealthBefore = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Class Action")
	int32 HealthAfter = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Class Action")
	int32 ManaBefore = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Class Action")
	int32 ManaAfter = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Class Action")
	int32 PhysicalArmorBefore = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Class Action")
	int32 PhysicalArmorAfter = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Class Action")
	int32 MagicalArmorBefore = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Class Action")
	int32 MagicalArmorAfter = 0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Class Action")
	bool bMovementStarted = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Class Action")
	FIntPoint MovementFromCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Class Action")
	FIntPoint MovementToCell = FIntPoint::ZeroValue;
};

/** Pure preview of one explicit cell/area target before resources are paid. */
USTRUCT(BlueprintType)
struct FGridCombatActionTargetingPreview
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	bool bValid = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	FGridAvailableCombatAction Action;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	FIntPoint TargetCell = FIntPoint(INDEX_NONE, INDEX_NONE);

	/** Every valid dungeon cell covered by the action, including an empty cell. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	TArray<FIntPoint> AffectedCells;

	/** Living encounter monsters that would be resolved, in deterministic order. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	TArray<FGuid> TargetMonsterIds;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	FText InvalidReason;
};

/** Outcome of an accepted cell/area action resolved once against one or more targets. */
USTRUCT(BlueprintType)
struct FGridCombatTargetedActionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	FIntPoint TargetCell = FIntPoint(INDEX_NONE, INDEX_NONE);

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	TArray<FIntPoint> AffectedCells;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	TArray<FGuid> TargetMonsterIds;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	TArray<int32> TargetCharacterIndices;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	TArray<FGridPlayerAttackRequest> AttackRequests;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Targeting")
	TArray<FGridAttackResult> AttackResults;
};

/** Result of the generic MON12 action request entry point. */
USTRUCT(BlueprintType)
struct FGridCombatActionRequestResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action")
	bool bAccepted = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action")
	EGridCombatActionRequestRejectReason RejectReason = EGridCombatActionRequestRejectReason::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action")
	FGridAvailableCombatAction Action;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action")
	FGridPlayerAttackRequest AttackRequest;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action")
	FGridAttackResult AttackResult;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action")
	EGridPlayerAttackRejectReason AttackRejectReason = EGridPlayerAttackRejectReason::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action")
	FGridCombatQuickItemResult QuickItemResult;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action")
	FGridCombatClassActionResult ClassActionResult;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat|Action")
	FGridCombatTargetedActionResult TargetedActionResult;
};
