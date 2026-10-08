#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridMonsterCategoryAsset.generated.h"

/**
 * Canonical presentation authority for one bestiary category.
 *
 * CategoryId remains the gameplay identity used by monster definitions,
 * combat filters and Skills. DisplayName is the player-facing label shared by
 * Favored Enemy and future bestiary/Codex presentation.
 */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridMonsterCategoryAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Category")
	FName CategoryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Category")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Category", meta = (MultiLine = "true"))
	FText Description;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UFUNCTION(BlueprintPure, Category = "Monster|Category|Validation")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Monster|Category|Validation")
	bool ValidateDefinition(UPARAM(ref) FString& OutError) const;

	/** Production/runtime lookup by gameplay CategoryId. Returns null if no canonical presentation asset exists. */
	static const UGridMonsterCategoryAsset* ResolveByCategoryId(FName CategoryId);

	static const TCHAR* ProductionFolder();
};
