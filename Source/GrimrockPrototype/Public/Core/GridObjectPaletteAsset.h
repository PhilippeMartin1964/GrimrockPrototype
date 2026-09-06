#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridObjectArchetypeAsset.h"
#include "GridTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "GridObjectPaletteAsset.generated.h"

class UTexture2D;
class UGridObjectArchetypeAsset;
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
	TObjectPtr<UGridObjectArchetypeAsset> DefaultArchetype = nullptr;

	/** WORLDOBJ-MIG05: canonical direct palette definition for a collectible Item. No companion ObjectArchetype is required. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Item")
	TObjectPtr<UGridItemDefinitionAsset> DefaultItemDefinition = nullptr;

	/** Required default when the archetype places a MonsterSpawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Monster")
	TObjectPtr<UGridMonsterDefinitionAsset> DefaultMonsterDefinition = nullptr;

	/** MON20.4.5 default copied into a placed StoryCompanion target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Story Companion")
	TObjectPtr<URPGStoryCompanionAsset> DefaultStoryCompanionDefinition = nullptr;

	bool IsDirectItemEntry() const
	{
		return DefaultItemDefinition != nullptr;
	}

	FName GetEffectiveArchetypeId() const
	{
		return IsDirectItemEntry() ? NAME_None : (DefaultArchetype ? DefaultArchetype->ArchetypeId : NAME_None);
	}

	EGridLevelObjectType GetEffectiveObjectType() const
	{
		return IsDirectItemEntry() ? EGridLevelObjectType::Item
			: (DefaultArchetype ? DefaultArchetype->SupportedType : EGridLevelObjectType::None);
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

		if (DefaultArchetype && !DefaultArchetype->Category.IsNone())
		{
			return DefaultArchetype->Category;
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

		if (DefaultArchetype && !DefaultArchetype->DisplayName.IsEmpty())
		{
			return DefaultArchetype->DisplayName;
		}

		const FName EffectiveArchetypeId = GetEffectiveArchetypeId();
		return !EffectiveArchetypeId.IsNone() ? FText::FromName(EffectiveArchetypeId) : FText::FromName(EntryId);
	}

	bool IsValidEntry() const
	{
		if (EntryId.IsNone())
		{
			return false;
		}

		if (IsDirectItemEntry())
		{
			return !DefaultArchetype && !Icon && DefaultItemDefinition->IsValidDefinition();
		}

		return DefaultArchetype && !DefaultArchetype->ArchetypeId.IsNone() && DefaultArchetype->SupportedType != EGridLevelObjectType::None;
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

	bool ValidatePalette(TArray<FGridArchetypeValidationMessage>& OutMessages) const;
};