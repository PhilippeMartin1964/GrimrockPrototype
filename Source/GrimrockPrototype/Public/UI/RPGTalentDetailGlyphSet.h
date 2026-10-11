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
	TObjectPtr<UTexture2D> Target;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Range;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Cooldown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> LevelRequirement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Detail|Glyphs")
	TObjectPtr<UTexture2D> Prerequisite;

	/** Unsupported semantics intentionally return nullptr so the text row remains readable without a misleading glyph. */
	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Detail|Glyphs")
	UTexture2D* ResolveGlyph(EGridTalentDetailLineSemantic Semantic) const;
};
