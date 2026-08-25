#pragma once

#include "CoreMinimal.h"
#include "Magic/GridSpellCastTransaction.h"
#include "GridSpellTargeting.generated.h"

UENUM(BlueprintType)
enum class EGridSpellTargetingRejectReason : uint8
{
	None UMETA(DisplayName = "None"),
	InvalidSpellDefinition UMETA(DisplayName = "Invalid Spell Definition"),
	InvalidRequest UMETA(DisplayName = "Invalid Request"),
	MissingTarget UMETA(DisplayName = "Missing Target"),
	TargetIdentityMismatch UMETA(DisplayName = "Target Identity Mismatch"),
	InvalidTargetRelation UMETA(DisplayName = "Invalid Target Relation"),
	TargetOutOfRange UMETA(DisplayName = "Target Out Of Range"),
	TargetNotAxial UMETA(DisplayName = "Target Not Axial"),
	LineOfSightBlocked UMETA(DisplayName = "Line Of Sight Blocked")
};

/**
 * Runtime information supplied by the authoritative world/party/monster layer.
 * MON18.4 deliberately does not store Actor pointers in spell requests.
 */
USTRUCT(BlueprintType)
struct FGridSpellTargetingContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic|Targeting")
	FIntPoint CasterCell = FIntPoint::ZeroValue;

	/** Resolved entity identity for Ally / FirstAxialTarget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic|Targeting")
	FGuid ResolvedTargetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic|Targeting")
	FIntPoint ResolvedTargetCell = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic|Targeting")
	bool bHasResolvedTargetCell = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic|Targeting")
	bool bResolvedTargetIsAlly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic|Targeting")
	bool bResolvedTargetIsHostile = false;

	/** Result of the authoritative grid LOS query for the resolved target/cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic|Targeting")
	bool bLineOfSightClear = true;
};

USTRUCT(BlueprintType)
struct FGridSpellResolvedTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Targeting")
	FGuid TargetId;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Targeting")
	FIntPoint GridCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Targeting")
	bool bHasGridCell = false;
};

struct GRIMROCKPROTOTYPE_API FGridSpellTargetingService
{
	static EGridSpellTargetingRejectReason ValidateAndResolveTarget(const FGridSpellDefinition& Definition, const FGridSpellCastRequest& Request,
		const FGridSpellTargetingContext& Context, FGridSpellResolvedTarget& OutResolvedTarget);
};

UENUM(BlueprintType)
enum class EGridSpellCastPipelineRejectStage : uint8
{
	None UMETA(DisplayName = "None"),
	Targeting UMETA(DisplayName = "Targeting"),
	Transaction UMETA(DisplayName = "Transaction")
};

/**
 * MON18.4 integrated boundary: target validation is completed before MON18.3
 * is allowed to mutate PA/mana. Effects are still outside this pipeline.
 */
struct GRIMROCKPROTOTYPE_API FGridSpellCastPipelineService
{
	static bool TryValidateTargetAndCommitCosts(const FGridSpellDefinition& Definition, const FGridSpellCastRequest& Request,
		const FGridSpellTargetingContext& TargetingContext, const FGridCharacterSpellbookState& Spellbook, FRPGDerivedStats& InOutCharacterStats,
		FGridPlayerCharacterTurnState& InOutTurnState, FGridSpellResolvedTarget& OutResolvedTarget, FGridSpellCastCostReceipt& OutReceipt,
		EGridSpellCastPipelineRejectStage& OutRejectStage, EGridSpellTargetingRejectReason& OutTargetingRejectReason,
		EGridSpellCastTransactionRejectReason& OutTransactionRejectReason);
};
