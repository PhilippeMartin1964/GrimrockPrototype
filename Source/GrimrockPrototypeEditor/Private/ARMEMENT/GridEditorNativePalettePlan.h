#pragma once

#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../EditorTools/Widgets/GridEditorPaletteReadModel.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

/** ARM-CLEAN06.4X.55 / 5G.26: read-only proof of the final native palette.
 * No Modify, MarkPackageDirty or SavePackage. Every direct row must already
 * be pointerless; this is no longer a migration path.
 */
struct FGridEditorNativePalettePlan
{
    static constexpr int32 ExpectedDirectEntries = 33;

    static bool TryBuild(const UGridObjectPaletteAsset& Source,
        const UGridItemSubsystem* Registry, TArray<FGridObjectPaletteEntry>& Out,
        FString& Error)
    {
        Out.Reset();
        Error.Reset();
        TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
        if (!IsValid(Registry) || !Source.ValidatePalette(Messages))
        {
            Error = TEXT("Missing canonical registry or invalid original palette.");
            return false;
        }

        TArray<FGridObjectPaletteEntry> Candidate = Source.Entries;
        int32 Direct = 0;
        for (FGridObjectPaletteEntry& Entry : Candidate)
        {
            if (!Entry.IsDirectItemEntry())
            {
                continue;
            }
            ++Direct;
            if (Entry.ItemId.IsNone() ||
                !Entry.IsValidEntry() ||
                Entry.DefaultMonsterDefinition || Entry.DefaultStoryCompanionDefinition)
            {
                Error = FString::Printf(TEXT("Invalid direct entry: %s"), *Entry.EntryId.ToString());
                return false;
            }
            const FGridEditorPaletteReadView Before =
                FGridEditorPaletteReadModel::Resolve(Entry, Registry);
            if (!Before.bRegisteredSource || !Before.bResolved ||
                Before.ItemId != Entry.ItemId)
            {
                Error = FString::Printf(TEXT("Unresolved canonical item or physical level: %s"),
                    *Entry.EntryId.ToString());
                return false;
            }
            const FGridEditorPaletteReadView After =
                FGridEditorPaletteReadModel::Resolve(Entry, Registry);
            if (!Entry.IsValidEntry() || !After.bRegisteredSource || !After.bResolved ||
                After.ItemId != Before.ItemId ||
                !After.DisplayName.EqualTo(Before.DisplayName) ||
                !After.Description.EqualTo(Before.Description) ||
                After.Icon.ToSoftObjectPath() != Before.Icon.ToSoftObjectPath())
            {
                Error = FString::Printf(TEXT("Pointerless presentation changed: %s"),
                    *Entry.EntryId.ToString());
                return false;
            }
        }
        if (Direct != ExpectedDirectEntries)
        {
            Error = FString::Printf(TEXT("Expected 33 canonical direct entries, got %d."), Direct);
            return false;
        }
        TStrongObjectPtr<UGridObjectPaletteAsset> Copy(
            NewObject<UGridObjectPaletteAsset>(GetTransientPackage()));
        Copy->Entries = Candidate;
        if (!Copy->ValidatePalette(Messages))
        {
            Error = TEXT("Pointerless candidate failed structural palette validation.");
            return false;
        }
        Out = MoveTemp(Candidate);
        return true;
    }
};
