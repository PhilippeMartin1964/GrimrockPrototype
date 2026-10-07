#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPGTalentPresentationAsset.generated.h"

class UTexture2D;

/**
 * Pure presentation metadata for one Talent branch.
 * Array order inside FRPGClassPresentationDefinition is the visual left -> center -> right order.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FRPGTalentBranchPresentationDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	FName TalentBranchId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation", meta = (MultiLine = "true"))
	FText ShortDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	TSoftObjectPtr<UTexture2D> BranchEmblemTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	FLinearColor AccentColor = FLinearColor::White;

	bool IsValidDefinition() const;
};

/**
 * Pure presentation metadata for one RPG class.
 * Branches must contain exactly three entries in visual left -> center -> right order.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FRPGClassPresentationDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	FName ClassId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	TSoftObjectPtr<UTexture2D> PortraitTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	TSoftObjectPtr<UTexture2D> ClassEmblemTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	FLinearColor PrimaryColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	FLinearColor SecondaryColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	FLinearColor AccentColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	FLinearColor GlowColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation")
	TArray<TSoftObjectPtr<UTexture2D>> MotifTextures;

	/** Canonical visual order: index 0 = left, 1 = center, 2 = right. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation", meta = (TitleProperty = "TalentBranchId"))
	TArray<FRPGTalentBranchPresentationDefinition> Branches;

	bool IsValidDefinition() const;
	const FRPGTalentBranchPresentationDefinition* FindBranch(FName TalentBranchId) const;
};

/**
 * Single data-driven presentation catalog for the RPG Talent Tree.
 * It owns no gameplay rules and stores no character state.
 */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API URPGTalentPresentationAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Presentation", meta = (TitleProperty = "ClassId"))
	TArray<FRPGClassPresentationDefinition> Classes;

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Presentation")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Presentation")
	bool GetClassPresentation(FName ClassId, FRPGClassPresentationDefinition& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Presentation")
	bool GetBranchPresentation(FName ClassId, FName TalentBranchId, FRPGTalentBranchPresentationDefinition& OutDefinition) const;

	const FRPGClassPresentationDefinition* FindClass(FName ClassId) const;
};
