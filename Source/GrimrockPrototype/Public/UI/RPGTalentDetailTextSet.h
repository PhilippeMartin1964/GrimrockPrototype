#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPGTalentDetailTextSet.generated.h"

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FRPGTalentDetailTextEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Text")
	FName TextId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Text", meta = (MultiLine = "true"))
	FText Text;

	bool IsValidDefinition() const;
};

/**
 * Editable/localizable player-facing vocabulary for Talent detail rows.
 * C++ owns stable TextIds and data only; wording lives in this asset and layout lives in UMG.
 */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API URPGTalentDetailTextSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Text", meta = (TitleProperty = "TextId"))
	TArray<FRPGTalentDetailTextEntry> Entries;

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Detail|Text")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Detail|Text")
	FText ResolveText(FName TextId) const;

	/** Canonical production asset used by the runtime Talent presentation pipeline. */
	static const URPGTalentDetailTextSet* LoadProduction();

	/** Resolves one production Talent UI text identity. No C++ wording fallback. */
	static FText ResolveProductionText(FName TextId);
};
