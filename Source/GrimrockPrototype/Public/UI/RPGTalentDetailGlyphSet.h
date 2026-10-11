#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/GridSkillsUiTypes.h"
#include "RPGTalentDetailGlyphSet.generated.h"

class UTexture2D;

/** Shared class-agnostic glyph authority for structured Talent detail lines. */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API URPGTalentDetailGlyphSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Cost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> ManaCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> ItemCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Action;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Target;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Range;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Area;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> LineOfSight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> ResolutionCount;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Cooldown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> LevelRequirement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Prerequisite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Exclusivity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Recipe;

	/** Unknown or intentionally unglyphed semantics return nullptr. */
	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Detail|Glyphs")
	UTexture2D* ResolveGlyph(EGridTalentDetailLineSemantic Semantic) const;
};
