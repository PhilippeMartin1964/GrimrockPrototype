#pragma once

#include "CoreMinimal.h"

class UGridPartyInventoryComponent;
struct FGridCharacterInventoryState;

enum class ERPGAttributePointTarget : uint8
{
	Strength,
	Dexterity,
	Constitution,
	Intelligence,
	Wisdom,
	Charisma
};

enum class ERPGAttributePointMutationRejectReason : uint8
{
	None,
	InvalidInventory,
	InvalidCharacter,
	InvalidLevel,
	InvalidDefinition,
	InvalidAttributeState,
	InvalidPointBalance,
	NoAttributePoints,
	AttributeCapReached,
	InvalidSessionFloor,
	NoSessionPurchaseToUndo,
	MutationRejected
};

struct FRPGAttributePointBalance
{
	int32 GrantedPoints = 0;
	int32 SpentPoints = 0;
	int32 RemainingPoints = 0;
	int32 StartingAttributeTotal = 0;
};

struct FRPGAttributePointMutationResult
{
	bool bCommitted = false;
	ERPGAttributePointMutationRejectReason RejectReason = ERPGAttributePointMutationRejectReason::None;
	ERPGAttributePointTarget Target = ERPGAttributePointTarget::Strength;
	int32 PreviousValue = 0;
	int32 NewValue = 0;
	int32 GrantedPoints = 0;
	int32 SpentPoints = 0;
	int32 RemainingPoints = 0;
};

/**
 * RPG-ATTR01 sole authority for player-facing Attribute Point allocation.
 *
 * No Attribute Point counter or allocation snapshot is persisted. Granted
 * points derive from Level. Spent points derive from the durable Attributes
 * total relative to the canonical class + race starting total.
 */
struct GRIMROCKPROTOTYPE_API FRPGAttributePointService
{
	static constexpr int32 MaximumBaseAttributeValue = 20;

	/** +1 point at levels 4, 8, 12, 16 and 20. Invalid levels return zero. */
	static int32 GetTotalPointsGranted(int32 CharacterLevel);

	/** Reconstructs the complete Attribute Point balance without persisted currency. */
	static bool TryGetBalance(const FGridCharacterInventoryState& CharacterState, FRPGAttributePointBalance& OutBalance);

	/** Reads one durable base attribute from Character.Attributes. */
	static int32 GetAttributeValue(const FGridCharacterInventoryState& CharacterState, ERPGAttributePointTarget Target);

	/** Pure availability query used by the Character Sheet presentation. */
	static ERPGAttributePointMutationRejectReason GetIncreaseAvailability(
		const FGridCharacterInventoryState& CharacterState,
		ERPGAttributePointTarget Target);

	/** Spends one point and raises one base attribute by one. */
	static bool TryPurchasePoint(
		UGridPartyInventoryComponent* PartyInventoryComponent,
		int32 CharacterIndex,
		ERPGAttributePointTarget Target,
		FRPGAttributePointMutationResult& OutResult);

	/**
	 * Refunds one point bought during the current Character Sheet session.
	 * SessionFloorValue is transient presentation-session state and prevents
	 * this correction path from becoming a permanent respec.
	 */
	static bool TryRefundPurchasedPoint(
		UGridPartyInventoryComponent* PartyInventoryComponent,
		int32 CharacterIndex,
		ERPGAttributePointTarget Target,
		int32 SessionFloorValue,
		FRPGAttributePointMutationResult& OutResult);
};
