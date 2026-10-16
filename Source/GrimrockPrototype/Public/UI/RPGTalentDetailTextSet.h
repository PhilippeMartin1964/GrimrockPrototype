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
 * Legacy Talent presentation TextSet retained during UI-TEXT-CENTRAL01.
 *
 * Static Talent UI vocabulary now belongs to UI.csv / ST_UI. Runtime access to
 * this asset is transitional and limited to Talent.Recipe.* display names until
 * Crafting owns recipe content.
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

	/** Legacy production asset retained for deferred Talent.Recipe.* content. */
	static const URPGTalentDetailTextSet* LoadProduction();

	/** Transitional resolver used only by the recipe fallback bridge. */
	static FText ResolveProductionText(FName TextId);
};
