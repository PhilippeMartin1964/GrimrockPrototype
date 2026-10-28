#include "ARMEMENT/GridWeaponPipelinePalette.h"

#include "Runtime/GridItemDefinitionAsset.h"

EGridWeaponPipelinePaletteState FGridWeaponPipelinePalette::Inspect(
    const UGridObjectPaletteAsset& Palette, FName EntryId,
    const UGridItemDefinitionAsset* Item, bool bAllowLegacyItems, FString& Detail)
{
    Detail.Reset();
    if (EntryId.IsNone() || !IsValid(Item) || !Item->IsValidDefinition() ||
        Item->ItemDefinitionId != EntryId)
    {
        Detail = TEXT("InvalidPaletteItemOrIdentity");
        return EGridWeaponPipelinePaletteState::Conflict;
    }

    int32 EntryCount = 0;
    int32 AliasCount = 0;
    const FGridObjectPaletteEntry* Matching = nullptr;
    for (const FGridObjectPaletteEntry& Entry : Palette.Entries)
    {
        if (Entry.EntryId == EntryId)
        {
            ++EntryCount;
            Matching = &Entry;
        }
        if (Entry.DefaultItemDefinition.Get() == Item &&
            Entry.EntryId != EntryId)
        {
            ++AliasCount;
        }
    }
    if (EntryCount > 1 || AliasCount > 0)
    {
        Detail = EntryCount > 1 ? TEXT("DuplicatePaletteEntryId") :
            TEXT("SameItemAlreadyRegisteredUnderDifferentId");
        return EGridWeaponPipelinePaletteState::Conflict;
    }
    if (!Matching)
    {
        Detail = TEXT("DirectItemPaletteEntryMissing");
        return EGridWeaponPipelinePaletteState::Missing;
    }
    if (Matching->DefaultItemDefinition.Get() != Item)
    {
        Detail = TEXT("PaletteEntryReferencesDifferentItem");
        return EGridWeaponPipelinePaletteState::Conflict;
    }
    if (Matching->PaletteCategory != FName(TEXT("Weapon")) &&
        !(bAllowLegacyItems && Matching->PaletteCategory == FName(TEXT("Items"))))
    {
        Detail = TEXT("UnexpectedPaletteCategory");
        return EGridWeaponPipelinePaletteState::Conflict;
    }
    if (Matching->Icon != nullptr ||
        Matching->DefaultWorldObjectDefinition != nullptr ||
        Matching->DefaultMonsterDefinition != nullptr ||
        Matching->DefaultStoryCompanionDefinition != nullptr ||
        !Matching->IsValidEntry())
    {
        Detail = TEXT("InvalidDirectItemPaletteEntry");
        return EGridWeaponPipelinePaletteState::Conflict;
    }
    Detail = TEXT("DirectItemPaletteEntryChecked");
    return EGridWeaponPipelinePaletteState::PresentSame;
}

bool FGridWeaponPipelinePalette::Propose(
    const UGridObjectPaletteAsset& Palette, FName EntryId,
    UGridItemDefinitionAsset* Item, bool bAllowLegacyItems,
    TArray<FGridObjectPaletteEntry>& Out, bool& bAdded, FString& Error)
{
    Out.Reset();
    bAdded = false;
    TArray<FGridWorldObjectDefinitionValidationMessage> Validation;
    if (!Palette.ValidatePalette(Validation))
    {
        Error = TEXT("ExistingPaletteInvalid");
        return false;
    }
    FString Detail;
    const EGridWeaponPipelinePaletteState Status =
        Inspect(Palette, EntryId, Item, bAllowLegacyItems, Detail);
    if (Status == EGridWeaponPipelinePaletteState::Conflict)
    {
        Error = Detail;
        return false;
    }
    Out = Palette.Entries;
    if (Status == EGridWeaponPipelinePaletteState::Missing)
    {
        FGridObjectPaletteEntry& Entry = Out.AddDefaulted_GetRef();
        Entry.EntryId = EntryId;
        Entry.PaletteCategory = FName(TEXT("Weapon"));
        Entry.DefaultItemDefinition = Item;
        bAdded = true;
    }
    UGridObjectPaletteAsset* Candidate = NewObject<UGridObjectPaletteAsset>(
        GetTransientPackage());
    Candidate->Entries = Out;
    if (!Candidate->ValidatePalette(Validation))
    {
        Error = TEXT("ProposedPaletteInvalid");
        Out.Reset();
        bAdded = false;
        return false;
    }
    return true;
}

bool FGridWeaponPipelinePalette::ProposeDirectItem(
    const UGridObjectPaletteAsset& Palette, UGridItemDefinitionAsset* Item,
    FName Category, TArray<FGridObjectPaletteEntry>& Out, bool& bAdded,
    FString& Error)
{
    Out.Reset();
    bAdded = false;
    TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
    if (!IsValid(Item) || !Item->IsValidDefinition() ||
        Item->ItemDefinitionId.IsNone() || Category.IsNone() ||
        !Palette.ValidatePalette(Messages))
    {
        Error = TEXT("InvalidItemOrOriginalPalette");
        return false;
    }
    int32 Count = 0;
    int32 Aliases = 0;
    const FGridObjectPaletteEntry* Existing = nullptr;
    for (const FGridObjectPaletteEntry& Entry : Palette.Entries)
    {
        if (Entry.EntryId == Item->ItemDefinitionId)
        {
            ++Count;
            Existing = &Entry;
        }
        if (Entry.DefaultItemDefinition.Get() == Item &&
            Entry.EntryId != Item->ItemDefinitionId)
            ++Aliases;
    }
    if (Count > 1 || Aliases > 0 || (Existing &&
        (Existing->PaletteCategory != Category ||
         Existing->DefaultItemDefinition.Get() != Item ||
         Existing->Icon != nullptr ||
         Existing->DefaultWorldObjectDefinition != nullptr ||
         Existing->DefaultMonsterDefinition != nullptr ||
         Existing->DefaultStoryCompanionDefinition != nullptr ||
         !Existing->IsValidEntry())))
    {
        Error = TEXT("DuplicateAliasOrConflictingExistingItem");
        return false;
    }
    Out = Palette.Entries;
    if (!Existing)
    {
        FGridObjectPaletteEntry& Entry = Out.AddDefaulted_GetRef();
        Entry.EntryId = Item->ItemDefinitionId;
        Entry.PaletteCategory = Category;
        Entry.DefaultItemDefinition = Item;
        bAdded = true;
    }
    UGridObjectPaletteAsset* Candidate =
        NewObject<UGridObjectPaletteAsset>(GetTransientPackage());
    Candidate->Entries = Out;
    if (!Candidate->ValidatePalette(Messages))
    {
        Error = TEXT("InvalidProposedPalette");
        Out.Reset();
        bAdded = false;
        return false;
    }
    return true;
}
