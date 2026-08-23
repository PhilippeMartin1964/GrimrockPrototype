#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"

bool UGridObjectPaletteAsset::ValidatePalette (TArray<FGridArchetypeValidationMessage>& OutMessages) const
{
    OutMessages.Reset ();

    TSet<FName> SeenEntryIds;
    for (const FGridObjectPaletteEntry& Entry : Entries)
    {
        const FString EntryName = Entry.EntryId.IsNone ()
            ? TEXT ("<unset>")
            : Entry.EntryId.ToString ();

        if (Entry.EntryId.IsNone ())
        {
            OutMessages.Emplace (EGridArchetypeValidationSeverity::Error, TEXT ("Palette entry requires EntryId."));
        }
        else if (SeenEntryIds.Contains (Entry.EntryId))
        {
            OutMessages.Emplace (
                EGridArchetypeValidationSeverity::Error,
                FString::Printf (TEXT ("Palette entry id '%s' is duplicated."), *EntryName));
        }
        else
        {
            SeenEntryIds.Add (Entry.EntryId);
        }

        if (!Entry.DefaultArchetype)
        {
            OutMessages.Emplace (
                EGridArchetypeValidationSeverity::Error,
                FString::Printf (TEXT ("Palette entry '%s' requires DefaultArchetype."), *EntryName));
            continue;
        }

        if (Entry.DefaultArchetype->ArchetypeId.IsNone ())
        {
            OutMessages.Emplace (
                EGridArchetypeValidationSeverity::Error,
                FString::Printf (TEXT ("Palette entry '%s' DefaultArchetype requires ArchetypeId."), *EntryName));
        }

        if (Entry.DefaultArchetype->SupportedType == EGridLevelObjectType::None)
        {
            OutMessages.Emplace (
                EGridArchetypeValidationSeverity::Error,
                FString::Printf (TEXT ("Palette entry '%s' DefaultArchetype SupportedType must not be None."), *EntryName));
        }

        if (Entry.DefaultArchetype->SupportedType ==
            EGridLevelObjectType::MonsterSpawn)
        {
            if (!Entry.DefaultMonsterDefinition)
            {
                OutMessages.Emplace (
                    EGridArchetypeValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("Palette entry '%s' requires DefaultMonsterDefinition for MonsterSpawn."),
                        *EntryName));
            }
            else
            {
                FString DefinitionError;
                if (!Entry.DefaultMonsterDefinition->ValidateDefinition (
                    DefinitionError))
                {
                    OutMessages.Emplace (
                        EGridArchetypeValidationSeverity::Error,
                        FString::Printf (
                            TEXT ("Palette entry '%s' has an invalid DefaultMonsterDefinition: %s"),
                            *EntryName,
                            *DefinitionError));
                }
            }
        }

        if (Entry.DefaultArchetype->SupportedType ==
            EGridLevelObjectType::StoryCompanion)
        {
            if (!Entry.DefaultStoryCompanionDefinition)
            {
                OutMessages.Emplace (
                    EGridArchetypeValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("Palette entry '%s' requires DefaultStoryCompanionDefinition for StoryCompanion."),
                        *EntryName));
            }
            else if (!Entry.DefaultStoryCompanionDefinition->IsValidDefinition ())
            {
                OutMessages.Emplace (
                    EGridArchetypeValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("Palette entry '%s' has an invalid DefaultStoryCompanionDefinition."),
                        *EntryName));
            }
        }
    }

    for (const FGridArchetypeValidationMessage& Message : OutMessages)
    {
        if (Message.Severity == EGridArchetypeValidationSeverity::Error)
        {
            return false;
        }
    }

    return true;
}
