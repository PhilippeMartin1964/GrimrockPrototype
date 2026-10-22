#include "ARMEMENT/GridWeaponFireBombPaletteCommandlet.h"
#include "ARMEMENT/GridWeaponFireBombPalettePlan.h"

#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT011023
{
    static constexpr const TCHAR* PalettePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");
}

UGridWeaponFireBombPaletteCommandlet::UGridWeaponFireBombPaletteCommandlet()
{
    IsEditor = true;
    IsClient = false;
    IsServer = false;
    LogToConsole = true;
}

int32 UGridWeaponFireBombPaletteCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    UGridObjectPaletteAsset* Palette =
        LoadObject<UGridObjectPaletteAsset>(nullptr, ARMEMENT011023::PalettePath);
    if (!IsValid(Palette))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.3] BLOCKED: production palette missing"));
        return 1;
    }

    TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
    if (!Palette->ValidatePalette(Messages))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.3] BLOCKED: existing palette invalid, no writes"));
        for (const auto& Message : Messages)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.3] %s"), *Message.Message);
        }
        return 1;
    }

    TArray<ARMEMENT011023::FResolved> Targets;
    for (const ARMEMENT011023::FSpec& Spec : ARMEMENT011023::Specs)
    {
        UGridItemDefinitionAsset* Item =
            LoadObject<UGridItemDefinitionAsset>(nullptr, Spec.ObjectPath);
        if (!IsValid(Item) || !Item->IsValidDefinition() ||
            Item->ItemDefinitionId != FName(Spec.ItemDefinitionId))
        {
            UE_LOG(LogTemp, Error,
                TEXT("[ARMEMENT01.10.2.3] BLOCKED %s: missing/invalid authoritative item at %s"),
                Spec.EntryId, Spec.ObjectPath);
            return 2;
        }
        ARMEMENT011023::FResolved& Resolved = Targets.AddDefaulted_GetRef();
        Resolved.Spec = &Spec;
        Resolved.Item = Item;
    }

    TArray<FGridObjectPaletteEntry> CandidateEntries;
    int32 Added = 0;
    int32 Moved = 0;
    FString Error;
    if (!ARMEMENT011023::Propose(Palette->Entries, Targets, CandidateEntries, Added, Moved, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.3] BLOCKED: %s; no writes"), *Error);
        return 3;
    }

    UGridObjectPaletteAsset* Candidate = NewObject<UGridObjectPaletteAsset>(GetTransientPackage());
    Candidate->Entries = CandidateEntries;
    if (!Candidate->ValidatePalette(Messages))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.3] BLOCKED: proposed palette invalid; no writes"));
        for (const auto& Message : Messages)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.3] %s"), *Message.Message);
        }
        return 4;
    }

    for (const ARMEMENT011023::FResolved& Target : Targets)
    {
        const FGridObjectPaletteEntry* Original =
            Palette->FindEntryById(FName(Target.Spec->EntryId));
        if (!Original)
        {
            UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.10.2.3] ADD %s -> %s"),
                Target.Spec->EntryId, Target.Spec->Category);
        }
        else if (Original->PaletteCategory != FName(Target.Spec->Category))
        {
            UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.10.2.3] MOVE %s: %s -> %s"),
                Target.Spec->EntryId, *Original->PaletteCategory.ToString(), Target.Spec->Category);
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.10.2.3] ALREADY %s -> %s"),
                Target.Spec->EntryId, Target.Spec->Category);
        }
    }
    if (!bApply)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[ARMEMENT01.10.2.3] READ-ONLY PREFLIGHT: +%d entries, %d moved; 0 writes"),
            Added, Moved);
        return 0;
    }
    if (Added == 0 && Moved == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.10.2.3] APPLY: already complete; 0 writes"));
        return 0;
    }

    UPackage* Package = Palette->GetOutermost();
    if (!IsValid(Package))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.3] BLOCKED: palette package unavailable"));
        return 5;
    }

    // All changes are restricted to the five requested entries. Other entries,
    // their original order and the complete containing palette are preserved.
    Palette->Modify();
    Palette->Entries = MoveTemp(CandidateEntries);
    Palette->MarkPackageDirty();

    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Palette, *Filename, SaveArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.3] ERROR: failed to save %s"), *Filename);
        return 6;
    }
    UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.10.2.3] SAVED palette: +%d entries, %d moved; unrelated entries preserved"),
        Added, Moved);
    return 0;
}
