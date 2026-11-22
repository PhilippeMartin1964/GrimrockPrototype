#include "ARMEMENT/GridWeaponPipelinePalette.h"
#include "EditorTools/GridEditorItemLevelPolicy.h"
#include "Runtime/Items/GridItemSubsystem.h"

namespace
{
    bool IsCanonicalDirect(const FGridObjectPaletteEntry& Entry, const UGridItemSubsystem* Registry)
    {
        return Entry.IsDirectItemEntry() &&
            !Entry.DefaultWorldObjectDefinition && !Entry.Icon &&
            !Entry.DefaultMonsterDefinition && !Entry.DefaultStoryCompanionDefinition &&
            FGridEditorItemLevelPolicy::CanAuthorItemLevel(
                Registry, Entry.CanonicalItemId, Entry.CanonicalDefaultItemLevel);
    }
}

EGridWeaponPipelinePaletteState FGridWeaponPipelinePalette::Inspect(
    const UGridObjectPaletteAsset& Palette, FName ItemId,
    const UGridItemSubsystem* Registry, bool bAllowItemsCategory, FString& Detail)
{
    using State = EGridWeaponPipelinePaletteState;
    Detail.Reset();
    if (ItemId.IsNone() || !IsValid(Registry) || !Registry->IsConfigured() ||
        !Registry->ContainsItem(ItemId))
    {
        Detail = TEXT("InvalidCanonicalPaletteIdentity");
        return State::Conflict;
    }
    int32 Count = 0;
    int32 Aliases = 0;
    const FGridObjectPaletteEntry* Matching = nullptr;
    for (const FGridObjectPaletteEntry& Entry : Palette.Entries)
    {
        if (Entry.EntryId == ItemId)
        {
            ++Count;
            Matching = &Entry;
        }
        if (Entry.CanonicalItemId == ItemId && Entry.EntryId != ItemId)
            ++Aliases;
        if (Entry.IsDirectItemEntry() && !IsCanonicalDirect(Entry, Registry))
        {
            Detail = TEXT("InvalidCanonicalDirectEntry");
            return State::Conflict;
        }
    }
    if (Count > 1 || Aliases > 0)
    {
        Detail = Count > 1 ? TEXT("DuplicatePaletteEntryId") : TEXT("SameItemAlreadyRegisteredUnderDifferentId");
        return State::Conflict;
    }
    if (!Matching)
    {
        Detail = TEXT("DirectItemPaletteEntryMissing");
        return State::Missing;
    }
    if (!IsCanonicalDirect(*Matching, Registry) || Matching->CanonicalItemId != ItemId ||
        !Matching->IsValidEntry())
    {
        Detail = TEXT("PaletteEntryReferencesDifferentItem");
        return State::Conflict;
    }
    if (Matching->PaletteCategory != FName(TEXT("Weapon")) &&
        !(bAllowItemsCategory && Matching->PaletteCategory == FName(TEXT("Items"))))
    {
        Detail = TEXT("UnexpectedPaletteCategory");
        return State::Conflict;
    }
    Detail = TEXT("DirectCanonicalItemPaletteEntryChecked");
    return State::PresentSame;
}

bool FGridWeaponPipelinePalette::Propose(
    const UGridObjectPaletteAsset& Palette, FName ItemId,
    const UGridItemSubsystem* Registry, int32 NewPhysicalLevel, bool bAllowItemsCategory,
    TArray<FGridObjectPaletteEntry>& Out, bool& bAdded, FString& Error)
{
    Out.Reset();
    bAdded = false;
    Error.Reset();
    TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
    if (!Palette.ValidatePalette(Messages))
    {
        Error = TEXT("ExistingPaletteInvalid");
        return false;
    }
    const auto Status = Inspect(Palette, ItemId, Registry, bAllowItemsCategory, Error);
    if (Status == EGridWeaponPipelinePaletteState::Conflict)
        return false;
    if (Status == EGridWeaponPipelinePaletteState::Missing &&
        !FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry, ItemId, NewPhysicalLevel))
    {
        Error = TEXT("InvalidNewPhysicalLevel");
        return false;
    }
    TArray<FGridObjectPaletteEntry> Candidate = Palette.Entries;
    if (Status == EGridWeaponPipelinePaletteState::Missing)
    {
        FGridObjectPaletteEntry& Entry = Candidate.AddDefaulted_GetRef();
        Entry.EntryId = ItemId;
        Entry.PaletteCategory = TEXT("Weapon");
        Entry.CanonicalItemId = ItemId;
        Entry.CanonicalDefaultItemLevel = NewPhysicalLevel;
    }
    UGridObjectPaletteAsset* Copy = NewObject<UGridObjectPaletteAsset>(GetTransientPackage());
    Copy->Entries = Candidate;
    if (!Copy->ValidatePalette(Messages))
    {
        Error = TEXT("ProposedPaletteInvalid");
        return false;
    }
    Out = MoveTemp(Candidate);
    bAdded = Status == EGridWeaponPipelinePaletteState::Missing;
    Error.Reset();
    return true;
}
