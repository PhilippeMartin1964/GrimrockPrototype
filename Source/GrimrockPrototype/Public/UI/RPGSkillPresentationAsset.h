#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPGSkillPresentationAsset.generated.h"

class UTexture2D;

/**
 * Presentation-only metadata for one canonical Skill.
 *
 * Gameplay identity, name, description, governing attribute, rank policy and
 * RequirementGrants remain authoritative in URPGSkillAsset.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FRPGSkillPresentationDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Presentation")
	FName SkillId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Presentation", meta = (MultiLine = "true"))
	FText UsageDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Presentation", meta = (MultiLine = "true"))
	FText BuildAdvice;

	/** Presentation advice only; never a gameplay restriction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Presentation")
	TArray<FName> RecommendedClassIds;

	/** Presentation advice only; never a gameplay restriction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Presentation")
	TArray<FName> UsefulClassIds;

	/** Canonical SkillIds shown as related Skills in the detail panel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Presentation")
	TArray<FName> SynergySkillIds;

	bool IsValidDefinition() const;
};

/**
 * Single presentation catalog for the Skill browser/detail surface.
 * It owns no character state and duplicates no URPGSkillAsset gameplay fields.
 */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API URPGSkillPresentationAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Presentation", meta = (TitleProperty = "SkillId"))
	TArray<FRPGSkillPresentationDefinition> Skills;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Presentation")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Presentation")
	bool GetSkillPresentation(FName SkillId, FRPGSkillPresentationDefinition& OutDefinition) const;

	const FRPGSkillPresentationDefinition* FindSkill(FName SkillId) const;
};
