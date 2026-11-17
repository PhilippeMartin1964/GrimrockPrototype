#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

bool UGridObjectPaletteAsset::ValidatePalette(TArray<FGridWorldObjectDefinitionValidationMessage>& OutMessages) const
{
	OutMessages.Reset();

	TSet<FName> SeenEntryIds;
	for (const FGridObjectPaletteEntry& Entry : Entries)
	{
		const FString EntryName = Entry.EntryId.IsNone() ? TEXT("<unset>") : Entry.EntryId.ToString();
		if (Entry.PaletteCategory.IsNone())
		{
			OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
				FString::Printf(TEXT("Palette entry '%s' requires Palette Category."), *EntryName));
		}

		if (Entry.EntryId.IsNone())
		{
			OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Palette entry requires EntryId."));
		}
		else if (SeenEntryIds.Contains(Entry.EntryId))
		{
			OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error, FString::Printf(TEXT("Palette entry id '%s' is duplicated."), *EntryName));
		}
		else
		{
			SeenEntryIds.Add(Entry.EntryId);
		}

		if (Entry.DefaultWorldObjectDefinition && Entry.DefaultWorldObjectDefinition->SupportedType == EGridLevelObjectType::Item)
		{
			OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
				FString::Printf(TEXT("Palette entry '%s' cannot use a WorldObjectDefinition for a collectible Item. Use DefaultItemDefinition instead."), *EntryName));
		}

		if (Entry.IsDirectItemEntry())
		{
			if (Entry.DefaultWorldObjectDefinition)
			{
				OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
					FString::Printf(TEXT("Palette entry '%s' must reference either a collectible or a world-object definition, not both."), *EntryName));
			}
			if (Entry.Icon)
			{
				if (IsValid(Entry.DefaultItemDefinition.Get()))
				{
					OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
						FString::Printf(TEXT("Palette entry '%s' is a direct collectible and must use DefaultItemDefinition.Icon instead of duplicating Palette Icon."), *EntryName));
				}
				else
				{
					OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
						FString::Printf(TEXT("Palette entry '%s' is a direct collectible and must use the item icon from ItemSystem."), *EntryName));
				}
			}
			if (Entry.CanonicalDefaultItemLevel < 0 || Entry.CanonicalDefaultItemLevel > 20)
			{
				OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
					FString::Printf(TEXT("Palette entry '%s' has a physical item level outside N0-N20."), *EntryName));
			}
			if (IsValid(Entry.DefaultItemDefinition.Get()))
			{
				if (!Entry.DefaultItemDefinition->IsValidDefinition())
				{
					OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
						FString::Printf(TEXT("Palette entry '%s' has an invalid DefaultItemDefinition."), *EntryName));
				}
				if (!Entry.CanonicalItemId.IsNone() &&
					Entry.DefaultItemDefinition->ItemDefinitionId != Entry.CanonicalItemId)
				{
					OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
						FString::Printf(TEXT("Palette entry '%s' has contradictory canonical and historical item identities."), *EntryName));
				}
			}
			continue; // Registry and level-family checks happen in Editor authoring.
		}

		if (!Entry.DefaultWorldObjectDefinition)
		{
			OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
				FString::Printf(TEXT("Palette entry '%s' requires DefaultWorldObjectDefinition or DefaultItemDefinition."), *EntryName));
			continue;
		}

		if (Entry.DefaultWorldObjectDefinition->DefinitionId.IsNone())
		{
			OutMessages.Emplace(
				EGridWorldObjectDefinitionValidationSeverity::Error, FString::Printf(TEXT("Palette entry '%s' DefaultWorldObjectDefinition requires WorldObjectDefinitionId."), *EntryName));
		}

		if (Entry.DefaultWorldObjectDefinition->SupportedType == EGridLevelObjectType::None)
		{
			OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
				FString::Printf(TEXT("Palette entry '%s' DefaultWorldObjectDefinition SupportedType must not be None."), *EntryName));
		}

		if (Entry.DefaultWorldObjectDefinition->SupportedType == EGridLevelObjectType::MonsterSpawn)
		{
			if (!Entry.DefaultMonsterDefinition)
			{
				OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
					FString::Printf(TEXT("Palette entry '%s' requires DefaultMonsterDefinition for MonsterSpawn."), *EntryName));
			}
			else
			{
				FString DefinitionError;
				if (!Entry.DefaultMonsterDefinition->ValidateDefinition(DefinitionError))
				{
					OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
						FString::Printf(TEXT("Palette entry '%s' has an invalid DefaultMonsterDefinition: %s"), *EntryName, *DefinitionError));
				}
			}
		}

		if (Entry.DefaultWorldObjectDefinition->SupportedType == EGridLevelObjectType::StoryCompanion)
		{
			if (!Entry.DefaultStoryCompanionDefinition)
			{
				OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
					FString::Printf(TEXT("Palette entry '%s' requires DefaultStoryCompanionDefinition for StoryCompanion."), *EntryName));
			}
			else if (!Entry.DefaultStoryCompanionDefinition->IsValidDefinition())
			{
				OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
					FString::Printf(TEXT("Palette entry '%s' has an invalid DefaultStoryCompanionDefinition."), *EntryName));
			}
		}
	}

	for (const FGridWorldObjectDefinitionValidationMessage& Message : OutMessages)
	{
		if (Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Error)
		{
			return false;
		}
	}

	return true;
}

#if WITH_EDITOR
void UGridObjectPaletteAsset::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	// Only normalize the entry being edited. A collectible must never retain
	// a palette-level icon that is hidden by EditConditionHides.
	const bool bNewItemDefinition =
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FGridObjectPaletteEntry, DefaultItemDefinition) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FGridObjectPaletteEntry, CanonicalItemId);
	const bool bArrayEntryAddedOrDuplicated =
		PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd ||
		PropertyChangedEvent.ChangeType == EPropertyChangeType::Duplicate;
	if ((bNewItemDefinition || bArrayEntryAddedOrDuplicated) &&
		PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UGridObjectPaletteAsset, Entries))
	{
		const int32 EditedEntryIndex = PropertyChangedEvent.GetArrayIndex(TEXT("Entries"));
		if (Entries.IsValidIndex(EditedEntryIndex))
		{
			Entries[EditedEntryIndex].ClearRedundantIconForDirectItem();
		}
	}

	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR
