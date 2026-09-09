#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridWorldObjectDefinitionAsset.h"
#include "GridTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "GridObjectPaletteAsset.generated.h"

class UTexture2D;
class UGridWorldObjectDefinitionAsset;
class URPGStoryCompanionAsset;

USTRUCT(BlueprintType)
struct FGridObjectPaletteEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FName EntryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette",
		meta = (ToolTip = "Optional label override for this palette tile. Leave empty to use the referenced definition display name."))
	FText DisplayNameOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette",
		meta = (ToolTip = "Optional grouping override for this palette tile. Leave empty to use the referenced definition category."))
	FName CategoryOverride = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette",
		meta = (ToolTip = "Palette icon for world-object entries. Direct collectible entries use DefaultItemDefinition.Icon and must leave this unset."))
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** World-object definition for non-collectible entries. Collectible items use DefaultItemDefinition directly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	TObjectPtr<UGridWorldObjectDefinitionAsset> DefaultWorldObjectDefinition = nullptr;

	/** WORLDOBJ-MIG05: canonical direct palette definition for a collectible Item. No companion WorldObjectDefinition is required. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Item")
	TObjectPtr<UGridItemDefinitionAsset> DefaultItemDefinition = nullptr;

	/** Required default when the definition places a MonsterSpawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Monster")
	TObjectPtr<UGridMonsterDefinitionAsset> DefaultMonsterDefinition = nullptr;

	/** MON20.4.5 default copied into a placed StoryCompanion target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Story Companion")
	TObjectPtr<URPGStoryCompanionAsset> DefaultStoryCompanionDefinition = nullptr;

	bool IsDirectItemEntry() const
	{
		return DefaultItemDefinition != nullptr;
	}

	FName GetEffectiveWorldObjectDefinitionId() const
	{
		return IsDirectItemEntry() ? NAME_None : (DefaultWorldObjectDefinition ? DefaultWorldObjectDefinition->DefinitionId : NAME_None);
	}

	EGridLevelObjectType GetEffectiveObjectType() const
	{
		return IsDirectItemEntry() ? EGridLevelObjectType::Item
			: (DefaultWorldObjectDefinition ? DefaultWorldObjectDefinition->SupportedType : EGridLevelObjectType::None);
	}

	FName GetEffectiveCategory() const
	{
		if (!CategoryOverride.IsNone())
		{
			return CategoryOverride;
		}

		if (IsDirectItemEntry())
		{
			return FName(TEXT("Items"));
		}

		if (DefaultWorldObjectDefinition && !DefaultWorldObjectDefinition->Category.IsNone())
		{
			return DefaultWorldObjectDefinition->Category;
		}

		return FName(TEXT("Uncategorized"));
	}

	FText GetEffectiveDisplayName() const
	{
		if (!DisplayNameOverride.IsEmpty())
		{
			return DisplayNameOverride;
		}

		if (IsDirectItemEntry())
		{
			if (!DefaultItemDefinition->DisplayName.IsEmpty())
			{
				return DefaultItemDefinition->DisplayName;
			}
			return !DefaultItemDefinition->ItemDefinitionId.IsNone() ? FText::FromName(DefaultItemDefinition->ItemDefinitionId) : FText::FromName(EntryId);
		}

		if (DefaultWorldObjectDefinition && !DefaultWorldObjectDefinition->DisplayName.IsEmpty())
		{
			return DefaultWorldObjectDefinition->DisplayName;
		}

		const FName EffectiveWorldObjectDefinitionId = GetEffectiveWorldObjectDefinitionId();
		return !EffectiveWorldObjectDefinitionId.IsNone() ? FText::FromName(EffectiveWorldObjectDefinitionId) : FText::FromName(EntryId);
	}

	bool IsValidEntry() const
	{
		if (EntryId.IsNone())
		{
			return false;
		}

		if (IsDirectItemEntry())
		{
			return !DefaultWorldObjectDefinition && !Icon && DefaultItemDefinition->IsValidDefinition();
		}

		return DefaultWorldObjectDefinition && !DefaultWorldObjectDefinition->DefinitionId.IsNone() && DefaultWorldObjectDefinition->SupportedType != EGridLevelObjectType::None;
	}
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridObjectPaletteAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	TArray<FGridObjectPaletteEntry> Entries;

	const FGridObjectPaletteEntry* FindEntryById(FName EntryId) const
	{
		return Entries.FindByPredicate(
			[EntryId](const FGridObjectPaletteEntry& Entry)
			{
				return Entry.EntryId == EntryId;
			});
	}

	bool ValidatePalette(TArray<FGridWorldObjectDefinitionValidationMessage>& OutMessages) const;
};