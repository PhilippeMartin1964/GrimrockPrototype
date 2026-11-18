#pragma once

#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../EditorTools/Widgets/GridEditorCanonicalPaletteReadModel.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

/** ARM-CLEAN06.4X.55: bounded native palette conversion plan.
 *  No Modify, MarkPackageDirty or SavePackage. Caller owns publication.
 *  Only the 33 already registered direct entries may lose a pointer;
 *  a partly converted production palette is deliberately rejected.
 */
struct FGridEditorNativePalettePlan
{
    static constexpr int32 ExpectedDirectEntries = 33;

    static bool TryBuild(const UGridObjectPaletteAsset& Source,
        const UGridItemSubsystem* Registry, TArray<FGridObjectPaletteEntry>& Out,
        int32& OldPointers, FString& Error)
    {
        Out.Reset();
        OldPointers = 0;
        Error.Reset();
        TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
        if (!IsValid(Registry) || !Source.ValidatePalette(Messages))
        {
            Error = TEXT("Missing canonical registry or invalid original palette.");
            return false;
        }

        TArray<FGridObjectPaletteEntry> Candidate = Source.Entries;
        int32 Direct = 0;
        int32 Old = 0;
        for (FGridObjectPaletteEntry& Entry : Candidate)
        {
            if (!Entry.IsDirectItemEntry())
            {
                continue;
            }
            ++Direct;
            const UGridItemDefinitionAsset* Legacy = Entry.DefaultItemDefinition.Get();
            if (Entry.CanonicalItemId.IsNone() ||
                !Entry.IsValidEntry() ||
                Entry.DefaultMonsterDefinition || Entry.DefaultStoryCompanionDefinition ||
                (Legacy && (!IsValid(Legacy) ||
                    Legacy->ItemDefinitionId != Entry.CanonicalItemId)))
            {
                Error = FString::Printf(TEXT("Invalid direct entry: %s"), *Entry.EntryId.ToString());
                return false;
            }
            const FGridEditorCanonicalPaletteReadView Before =
                FGridEditorCanonicalPaletteReadModel::Resolve(Entry, Registry);
            if (!Before.bCanonicalSource || !Before.bResolved ||
                Before.ItemId != Entry.CanonicalItemId)
            {
                Error = FString::Printf(TEXT("Unresolved canonical item or physical level: %s"),
                    *Entry.EntryId.ToString());
                return false;
            }
            if (Legacy)
            {
                ++Old;
                if (Entry.GetEffectiveDisplayName().ToString() != Before.DisplayName.ToString() ||
                    Legacy->Description.ToString() != Before.Description.ToString() ||
                    Legacy->Icon.ToSoftObjectPath() != Before.Icon.ToSoftObjectPath())
                {
                    Error = FString::Printf(TEXT("Historical presentation differs: %s"),
                        *Entry.EntryId.ToString());
                    return false;
                }
            }
            Entry.DefaultItemDefinition = nullptr;
            const FGridEditorCanonicalPaletteReadView After =
                FGridEditorCanonicalPaletteReadModel::Resolve(Entry, Registry);
            if (!Entry.IsValidEntry() || !After.bCanonicalSource || !After.bResolved ||
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
        if (Direct != ExpectedDirectEntries || (Old != 0 && Old != ExpectedDirectEntries))
        {
            Error = FString::Printf(TEXT("Expected 33 direct entries and 33 or 0 old pointers, got %d/%d."),
                Direct, Old);
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
        OldPointers = Old;
        Out = MoveTemp(Candidate);
        return true;
    }
};
