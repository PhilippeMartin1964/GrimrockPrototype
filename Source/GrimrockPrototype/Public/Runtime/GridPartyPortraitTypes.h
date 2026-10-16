#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "GridPartyPortraitTypes.generated.h"

/**
 * UI-PARTYHUD01.1 read-only projection for one persistent party portrait.
 * Gameplay authority stays in FGridCharacterInventoryState / UGridPartyInventoryComponent.
 */
USTRUCT(BlueprintType)
struct FGridPartyPortraitView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits")
	FGuid CharacterId;

	/** Current runtime index resolved from CharacterId. Never persisted as identity. */
	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits")
	int32 CharacterIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits")
	bool bIsSelected = false;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Health")
	int32 CurrentHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Health")
	int32 MaximumHealth = 1;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Health")
	float HealthRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Mana")
	int32 CurrentMana = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Mana")
	int32 MaximumMana = 0;

	/** Mana bar is shown only when this is true. */
	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Mana")
	bool bHasMana = false;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Mana")
	float ManaRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Armor")
	int32 CurrentPhysicalArmor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Armor")
	int32 ReferencePhysicalArmor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Armor")
	float PhysicalArmorRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Armor")
	int32 CurrentMagicalArmor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Armor")
	int32 ReferenceMagicalArmor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Party Portraits|Armor")
	float MagicalArmorRatio = 0.0f;
};
