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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual")
	FName ClassId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual")
	TSoftObjectPtr<UTexture2D> ClassIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Class Visual")
	FLinearColor AccentColor = FLinearColor::White;

	UFUNCTION(BlueprintPure, Category = "RPG|Class Visual")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Class Visual")
	bool IsValidForClass(FName InClassId) const;
};
