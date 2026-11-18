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
		meta = (DisplayName = "Palette Category",
			ToolTip = "Editor-only grouping used to organize this entry in the Grid Editor palette."))
	FName PaletteCategory = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette",
		meta = (ToolTip = "Palette icon for world-object entries only. Direct collectibles resolve their icon from ItemSystem.",
			EditCondition = "DefaultWorldObjectDefinition != nullptr && DefaultItemDefinition == nullptr", EditConditionHides))
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** World-object definition for non-collectible entries. Collectible items use DefaultItemDefinition directly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	TObjectPtr<UGridWorldObjectDefinitionAsset> DefaultWorldObjectDefinition = nullptr;

	/** Historical serialized source, read-only in Details. Author new direct entries by CanonicalItemId. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Palette|Item")
	TObjectPtr<UGridItemDefinitionAsset> DefaultItemDefinition = nullptr;

	/** One stable ItemId for direct collectibles; the one-shot 06.4W
	 * Editor migration derives it from the original authored definition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Item|Canonical")
	FName CanonicalItemId = NAME_None;

	/** ARM-CLEAN06.4X.5: physical default for NEW direct palette placements.
	 * N1-N20 for ARM, N0 for unlevelled items; never infer N1 at runtime.
	 * One-shot Editor migration derives this from the original source.
	 * The old source pointer remains until the verified reader cutover. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Item|Canonical",
		meta = (ClampMin = "0", ClampMax = "20"))
	int32 CanonicalDefaultItemLevel = 0;

	/** Required default when the definition places a MonsterSpawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Monster")
	TObjectPtr<UGridMonsterDefinitionAsset> DefaultMonsterDefinition = nullptr;

	/** MON20.4.5 default copied into a placed StoryCompanion target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Story Companion")
	TObjectPtr<URPGStoryCompanionAsset> DefaultStoryCompanionDefinition = nullptr;

	bool IsDirectItemEntry() const
	{
		return DefaultItemDefinition != nullptr || !CanonicalItemId.IsNone();
	}

#if WITH_EDITOR
	/** Prevent a hidden icon override from surviving a switch to a direct collectible. */
	bool ClearRedundantIconForDirectItem()
	{
		if (!IsDirectItemEntry() || !Icon)
		{
			return false;
		}
		Icon = nullptr;
		return true;
	}
#endif

	#if WITH_EDITOR
	/** Temporary Editor migration/inspection predicate. Compare identity only,
	 *  never resolve item gameplay/presentation from the old source. */
	bool MatchesDirectItemSource(const UGridItemDefinitionAsset* Source) const
	{
		if (!IsValid(Source) || Source->ItemDefinitionId.IsNone()) return false;
		if (!CanonicalItemId.IsNone())
		{
			return CanonicalItemId == Source->ItemDefinitionId &&
				(!DefaultItemDefinition || DefaultItemDefinition.Get() == Source);
		}
		return DefaultItemDefinition.Get() == Source;
	}
	#endif

	FName GetEffectiveWorldObjectDefinitionId() const
	{
		return IsDirectItemEntry() ? NAME_None : (DefaultWorldObjectDefinition ? DefaultWorldObjectDefinition->DefinitionId : NAME_None);
	}

	EGridLevelObjectType GetEffectiveObjectType() const
	{
		return IsDirectItemEntry() ? EGridLevelObjectType::Item
			: (DefaultWorldObjectDefinition ? DefaultWorldObjectDefinition->SupportedType : EGridLevelObjectType::None);
	}

	FName GetPaletteCategory() const
	{
		return PaletteCategory.IsNone() ? FName(TEXT("Uncategorized")) : PaletteCategory;
	}

	FText GetEffectiveDisplayName() const
	{
		if (IsDirectItemEntry())
		{
			if (!IsValid(DefaultItemDefinition.Get()))
			{
				return FText::FromName(CanonicalItemId);
			}
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
			if (DefaultWorldObjectDefinition || Icon || CanonicalDefaultItemLevel < 0 ||
				CanonicalDefaultItemLevel > 20)
			{
				return false;
			}
			if (!CanonicalItemId.IsNone() && IsValid(DefaultItemDefinition.Get()) &&
				DefaultItemDefinition->ItemDefinitionId != CanonicalItemId)
			{
				return false;
			}
			return !CanonicalItemId.IsNone() ||
				(IsValid(DefaultItemDefinition.Get()) &&
				 DefaultItemDefinition->IsValidDefinition());
		}

		return DefaultWorldObjectDefinition && !DefaultWorldObjectDefinition->DefinitionId.IsNone() && DefaultWorldObjectDefinition->SupportedType != EGridLevelObjectType::None
			&& DefaultWorldObjectDefinition->SupportedType != EGridLevelObjectType::Item;
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

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif
};
