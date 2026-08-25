#pragma once

#include "CoreMinimal.h"
#include "GridLogicTypes.generated.h"

/** Data-only logical primitives executed by the Event -> Command graph. */
UENUM(BlueprintType)
enum class EGridLogicNodeType : uint8
{
	Relay UMETA(DisplayName = "Relay"),
	SetBool UMETA(DisplayName = "Set Bool"),
	ToggleBool UMETA(DisplayName = "Toggle Bool"),
	SetInt UMETA(DisplayName = "Set Int"),
	AddInt UMETA(DisplayName = "Add Int"),
	SubtractInt UMETA(DisplayName = "Subtract Int"),
	ResetVariable UMETA(DisplayName = "Reset Variable"),
	CompareBool UMETA(DisplayName = "Compare Bool"),
	CompareInt UMETA(DisplayName = "Compare Int"),
	Latch UMETA(DisplayName = "Latch")
};

UENUM(BlueprintType)
enum class EGridLogicIntComparison : uint8
{
	Equal UMETA(DisplayName = "Equal"),
	NotEqual UMETA(DisplayName = "Not Equal"),
	Less UMETA(DisplayName = "Less"),
	LessOrEqual UMETA(DisplayName = "Less Or Equal"),
	Greater UMETA(DisplayName = "Greater"),
	GreaterOrEqual UMETA(DisplayName = "Greater Or Equal")
};

/**
 * Persistent configuration of one MON19.2.3 logical node.
 * Runtime values always live in GridLevelVariableStore / FGridLevelRuntimeState.
 */
USTRUCT(BlueprintType)
struct FGridLogicNodeParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic")
	EGridLogicNodeType NodeType = EGridLogicNodeType::Relay;

	/** Variable consumed by every primitive except Relay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic", meta = (EditCondition = "NodeType != EGridLogicNodeType::Relay", EditConditionHides))
	FName VariableId = NAME_None;

	/** SetBool value or CompareBool expected value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic|Bool",
		meta = (EditCondition = "NodeType == EGridLogicNodeType::SetBool || NodeType == EGridLogicNodeType::CompareBool", EditConditionHides))
	bool bBoolValue = false;

	/** SetInt value, Add/Sub operand or CompareInt threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic|Int",
		meta = (EditCondition =
					"NodeType == EGridLogicNodeType::SetInt || NodeType == EGridLogicNodeType::AddInt || NodeType == EGridLogicNodeType::SubtractInt || NodeType == EGridLogicNodeType::CompareInt",
			EditConditionHides))
	int32 IntValue = 0;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Logic|Int", meta = (EditCondition = "NodeType == EGridLogicNodeType::CompareInt", EditConditionHides))
	EGridLogicIntComparison IntComparison = EGridLogicIntComparison::Equal;
};
