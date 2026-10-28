#include "ARMEMENT/GridArrowPaletteAuthoringCommandlet.h"

#include "Core/GridObjectPaletteAsset.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace ArrowPalette
{
    static constexpr const TCHAR* PalettePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");
    static constexpr const TCHAR* ArrowPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.DA_Item_Arrows_01");
    static const FName ArrowId(TEXT("Item_Arrows_01"));
    static const FName ItemsCategory(TEXT("Items"));

    bool IsCanonical(const FGridObjectPaletteEntry& Entry, const UGridItemDefinitionAsset* Item)
    {
        return Entry.EntryId == ArrowId && Entry.PaletteCategory == ItemsCategory &&
            Entry.DefaultItemDefinition.Get() == Item &&
            Entry.DefaultWorldObjectDefinition == nullptr && Entry.Icon == nullptr &&
            Entry.DefaultMonsterDefinition == nullptr &&
            Entry.DefaultStoryCompanionDefinition == nullptr && Entry.IsValidEntry();
    }
}

UGridArrowPaletteAuthoringCommandlet::UGridArrowPaletteAuthoringCommandlet()
{
    IsEditor = true;
    IsClient = false;
    IsServer = false;
    LogToConsole = true;
}

int32 UGridArrowPaletteAuthoringCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    UGridObjectPaletteAsset* Palette =
        LoadObject<UGridObjectPaletteAsset>(nullptr, ArrowPalette::PalettePath);
    UGridItemDefinitionAsset* Arrows =
        LoadObject<UGridItemDefinitionAsset>(nullptr, ArrowPalette::ArrowPath);
    if (!IsValid(Palette) || !IsValid(Arrows) ||
        Arrows->ItemDefinitionId != ArrowPalette::ArrowId ||
        !Arrows->IsValidDefinition() || !Arrows->bStackable ||
        Arrows->MaxStackSize != 64 ||
        !FMath::IsNearlyEqual(Arrows->Weight, 0.04f) ||
        Arrows->bCombatThrowWeapon)
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.13.2.2] BLOCKED: invalid palette or arrow stack"));
        return 1;
    }

    TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
    if (!Palette->ValidatePalette(Messages))
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.13.2.2] BLOCKED: source palette invalid"));
        return 2;
    }

    int32 MatchingCount = 0;
    int32 AliasCount = 0;
    const FGridObjectPaletteEntry* Existing = nullptr;
    for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
    {
        if (Entry.EntryId == ArrowPalette::ArrowId)
        {
            ++MatchingCount;
            Existing = &Entry;
        }
        if (Entry.DefaultItemDefinition.Get() == Arrows &&
            Entry.EntryId != ArrowPalette::ArrowId)
        {
            ++AliasCount;
        }
    }
    if (MatchingCount > 1 || AliasCount > 0 ||
        (Existing && !ArrowPalette::IsCanonical(*Existing, Arrows)))
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.13.2.2] CONFLICT: duplicate or different arrow binding"));
        return 3;
    }
    if (Existing)
    {
        UE_LOG(LogTemp, Display, TEXT("[LOT02.13.2.2] PRESENT_SAME Item_Arrows_01 Category=Items"));
        return 0;
    }

    TArray<FGridObjectPaletteEntry> Proposed = Palette->Entries;
    FGridObjectPaletteEntry& NewEntry = Proposed.AddDefaulted_GetRef();
    NewEntry.EntryId = ArrowPalette::ArrowId;
    NewEntry.PaletteCategory = ArrowPalette::ItemsCategory;
    NewEntry.DefaultItemDefinition = Arrows;

    UGridObjectPaletteAsset* Candidate =
        NewObject<UGridObjectPaletteAsset>(GetTransientPackage());
    Candidate->Entries = Proposed;
    if (!ArrowPalette::IsCanonical(NewEntry, Arrows) ||
        !Candidate->ValidatePalette(Messages))
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.13.2.2] BLOCKED: invalid proposed entry"));
        return 4;
    }

    UE_LOG(LogTemp, Display, TEXT("[LOT02.13.2.2] READY Item_Arrows_01 Category=Items Added=1"));
    if (!bApply)
    {
        UE_LOG(LogTemp, Display, TEXT("[LOT02.13.2.2] DRY_RUN no packages written"));
        return 0;
    }
    UPackage* Package = Palette->GetOutermost();
    if (!IsValid(Package) || Package->IsDirty())
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.13.2.2] CONFLICT: palette package already dirty"));
        return 5;
    }
    Palette->Modify();
    Palette->Entries = MoveTemp(Proposed);
    Palette->MarkPackageDirty();

    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Palette, *Filename, Args))
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.13.2.2] SAVE_FAILED: palette package; no automatic rollback"));
        return 6;
    }
    UE_LOG(LogTemp, Display, TEXT("[LOT02.13.2.2] SAVED Palette=DA_ObjectPalette_Default Added=1"));
    return 0;
}
