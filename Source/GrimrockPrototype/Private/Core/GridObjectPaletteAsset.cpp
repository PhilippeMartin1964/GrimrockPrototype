#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"

bool UGridObjectPaletteAsset::ValidatePalette(TArray<FGridWorldObjectDefinitionValidationMessage>& OutMessages) const
{
	OutMessages.Reset();

	TSet<FName> SeenEntryIds;
	for (const FGridObjectPaletteEntry& Entry : Entries)
	{
		const FString EntryName = Entry.EntryId.IsNone() ? TEXT("<unset>") : Entry.EntryId.ToString();

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

		if (Entry.DefaultItemDefinition)
		{
			if (Entry.DefaultWorldObjectDefinition)
			{
				OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
					FString::Printf(TEXT("Palette entry '%s' must reference either DefaultItemDefinition or DefaultWorldObjectDefinition, not both."), *EntryName));
			}

			if (Entry.Icon)
			{
				OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
					FString::Printf(TEXT("Palette entry '%s' is a direct collectible and must use DefaultItemDefinition.Icon instead of duplicating Palette Icon."), *EntryName));
			}

			if (!Entry.DefaultItemDefinition->IsValidDefinition())
			{
				OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Error,
					FString::Printf(TEXT("Palette entry '%s' has an invalid DefaultItemDefinition."), *EntryName));
			}
			continue;
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