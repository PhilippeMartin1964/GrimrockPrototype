#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPGClassVisualAsset.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API URPGClassVisualAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual")
	FName ClassId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual")
	TSoftObjectPtr<UTexture2D> ClassIcon;

	/** Wide class-specific banner used by Skills/Talents and other class headers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual")
	TSoftObjectPtr<UTexture2D> Banner;

	/** Compact class-specific flag/emblem used by Skills/Talents and related chrome. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual")
	TSoftObjectPtr<UTexture2D> Flag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual")
	FLinearColor AccentColor = FLinearColor::White;

	UFUNCTION(BlueprintPure, Category = "RPG|Class Visual")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Class Visual")
	bool IsValidForClass(FName InClassId) const;
};
