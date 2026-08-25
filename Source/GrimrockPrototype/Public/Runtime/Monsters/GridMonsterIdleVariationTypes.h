#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "GridMonsterIdleVariationTypes.generated.h"

class UAnimSequenceBase;

USTRUCT(BlueprintType)
struct FGridMonsterIdleVariationDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Idle Variation")
	FName VariationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Idle Variation")
	TSoftObjectPtr<UAnimSequenceBase> Animation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Idle Variation", meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Idle Variation", meta = (ClampMin = "0.01"))
	float ExpectedDuration = 1.0f;

	bool IsValidDefinition() const;
};

USTRUCT(BlueprintType)
struct FGridMonsterIdleVariationPlaybackRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient)
	int32 SequenceNumber = 0;

	UPROPERTY(BlueprintReadOnly, Transient)
	int32 OccurrenceNumber = 0;

	UPROPERTY(BlueprintReadOnly, Transient)
	FName MonsterId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient)
	FName VariationId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient)
	int32 VariationIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Transient)
	TObjectPtr<UAnimSequenceBase> Animation = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient)
	FName SlotName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Transient)
	float PlayRate = 1.0f;

	UPROPERTY(BlueprintReadOnly, Transient)
	float EffectiveDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient)
	float BlendInTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient)
	float BlendOutTime = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridMonsterIdleVariationPlaybackRequestedSignature, FGridMonsterIdleVariationPlaybackRequest, Request);

class GRIMROCKPROTOTYPE_API FGridMonsterIdleVariationSelector
{
public:
	static int32 BuildPresentationSeed(const FGuid& PersistenceId, FName MonsterId, int32 OccurrenceNumber);

	static int32 SelectVariationIndex(int32 PresentationSeed, int32 VariationCount, int32 PreviousVariationIndex, bool bAvoidImmediateRepeat);

	static float SelectDelay(int32 PresentationSeed, float MinimumDelay, float MaximumDelay);
};
