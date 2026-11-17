#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

namespace ARMCLEAN064X45
{
    // Fingerprint of authored gameplay/spatial state deliberately excludes
    // ONLY the obsolete ItemDefinition pointer being removed on a COPY.
    FString StableWorldFingerprint(const UGridLevelAsset& Level)
    {
        FString Result = FString::Printf(TEXT("Cells=%d;Objects=%d;Links=%d;Monsters=%d;Spawns=%d;Logic=%d;"),
            Level.Cells.Num(), Level.WorldObjectInstances.Num(), Level.Links.Num(),
            Level.MonsterSpawns.Num(), Level.ItemSpawns.Num(), Level.LogicObjects.Num());
        for (const FGridLooseItemInstance& Item : Level.LooseItemInstances)
        {
            Result += FString::Printf(TEXT("|%s;%s;%d;%d;%d;%d;%d;%s;%.6f;%s;%s;%s;%s;%s;%s"),
                *Item.InstanceId.ToString(EGuidFormats::Digits),
                *Item.CanonicalItemId.ToString(), Item.ItemLevel, Item.Quantity,
                Item.CellX, Item.CellY, static_cast<int32>(Item.SurfaceSide),
                *Item.LocalOffset.ToString(), Item.LocalYaw,
                *Item.LogicId.ToString(), *GetPathNameSafe(Item.ReadableContentAsset.Get()),
                *Item.ReadableContentId.ToString(), *Item.ReadTitleOverride.ToString(),
                *Item.ReadTextOverride.ToString(), *Item.PaletteEntryId.ToString());
            Result += TEXT("|") + Item.Notes;
        }
        return Result;
    }

    bool ValidateLoose(const UGridLevelAsset& Level, const UGridItemSubsystem& Registry,
        int32& OutLegacyCount, FString& OutError)
    {
        OutLegacyCount = 0;
        OutError.Reset();
        if (Level.LooseItemInstances.Num() != 44 || !Level.ItemSpawns.IsEmpty())
        {
            OutError = TEXT("Expected exactly 44 authored loose items and zero spawns.");
            return false;
        }
        TSet<FGuid> Seen;
        for (const FGridLooseItemInstance& Item : Level.LooseItemInstances)
        {
            if (!Item.InstanceId.IsValid() || Seen.Contains(Item.InstanceId) ||
                !Level.IsValidCoord(Item.CellX, Item.CellY) ||
                Item.CanonicalItemId.IsNone() || Item.Quantity < 1 ||
                Item.ItemLevel < 0 || Item.ItemLevel > 20 ||
                Item.LocalOffset.ContainsNaN() || !FMath::IsFinite(Item.LocalYaw))
            {
                OutError = TEXT("Invalid GUID, placement, quantity or canonical physical level.");
                return false;
            }
            Seen.Add(Item.InstanceId);
            FGridResolvedItem Resolved;
            if (!Registry.ResolveItem(Item.CanonicalItemId, Item.ItemLevel, Resolved) ||
                !Resolved.IsValid() || Resolved.ItemId != Item.CanonicalItemId ||
                Resolved.ItemLevel != Item.ItemLevel ||
                (!Resolved.bIsArmament && Item.ItemLevel != 0) ||
                Item.Quantity > FMath::Max(1, Resolved.MaxStack))
            {
                OutError = TEXT("One real loose item cannot resolve its registered physical identity.");
                return false;
            }
            if (Item.ItemDefinition)
            {
                const UGridItemDefinitionAsset* Historical = Item.ItemDefinition.Get();
                if (!IsValid(Historical) ||
                    Historical->ItemDefinitionId != Item.CanonicalItemId ||
                    (Resolved.bIsArmament &&
                        Historical->WeaponArchetypeId != Resolved.ArchetypeId) ||
                    (!Resolved.bIsArmament && !Historical->WeaponArchetypeId.IsNone()))
                {
                    OutError = TEXT("A persisted historical pointer disagrees with the registered item.");
                    return false;
                }
                ++OutLegacyCount;
            }
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X45NativeLoosePreflight,
    "Grimrock.RPG.ARM.CLEAN06_4X45.NativeLoose44GuardedPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X45NativeLoosePreflight::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    TStrongObjectPtr<UGridLevelAsset> Real(LoadObject<UGridLevelAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
                      "DA_GridLevel_00.DA_GridLevel_00")));
    if (!TestNotNull(TEXT("Real canonical item registry"), Root.Get()) ||
        !TestNotNull(TEXT("Exact native target level"), Real.Get())) return false;
    const bool bSourceDirty = Real->GetOutermost()->IsDirty();

    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> RegisterErrors;
    if (!TestTrue(TEXT("Real ItemSystem is configured"),
        Registry->RegisterItemSystem(Root.Get(), RegisterErrors)))
    {
        for (const FString& Error : RegisterErrors) AddError(Error);
        return false;
    }

    int32 OriginalOldPointers = 0;
    FString Error;
    if (!TestTrue(TEXT("All 44 native loose rows are safe and canonical"),
        ARMCLEAN064X45::ValidateLoose(*Real.Get(), *Registry.Get(),
            OriginalOldPointers, Error)))
    {
        AddError(Error);
        return false;
    }
    if (!TestTrue(TEXT("No partial/unreviewed binary migration of the 44 rows"),
        OriginalOldPointers == 44 || OriginalOldPointers == 0))
    {
        return false;
    }

    // Never call Modify(), SavePackage(), or touch the production level.
    // This copy is an exact in-memory dry run for the single property only.
    TStrongObjectPtr<UGridLevelAsset> Copy(DuplicateObject<UGridLevelAsset>(
        Real.Get(), GetTransientPackage()));
    if (!TestNotNull(TEXT("Transient, disposable copy of native level"), Copy.Get()))
        return false;

    const FString Before = ARMCLEAN064X45::StableWorldFingerprint(*Copy.Get());
    for (FGridLooseItemInstance& Item : Copy->LooseItemInstances)
    {
        Item.ItemDefinition = nullptr;
    }
    int32 PointerlessCount = -1;
    if (!TestTrue(TEXT("Disposable target still validates after pointer removal"),
        ARMCLEAN064X45::ValidateLoose(*Copy.Get(), *Registry.Get(),
            PointerlessCount, Error)))
    {
        AddError(Error);
        return false;
    }
    if (!TestEqual(TEXT("No legacy pointer remains in disposable copy"),
            PointerlessCount, 0) ||
        !TestEqual(TEXT("All 44 authored fields remain stable"),
            ARMCLEAN064X45::StableWorldFingerprint(*Copy.Get()), Before) ||
        !TestEqual(TEXT("Real level package never becomes dirty"),
            Real->GetOutermost()->IsDirty(), bSourceDirty))
    {
        return false;
    }

    // A corrupted physical level cannot pass the same gate on the copy.
    FGridLooseItemInstance* FirstArm = Copy->LooseItemInstances.FindByPredicate(
        [Registry = Registry.Get()](const FGridLooseItemInstance& Item)
        {
            FGridResolvedItem Canonical;
            return Registry->ResolveItem(Item.CanonicalItemId, Item.ItemLevel, Canonical) &&
                Canonical.bIsArmament;
        });
    if (!TestNotNull(TEXT("There are authored ARM among the 44"), FirstArm))
        return false;
    FirstArm->ItemLevel = 0;
    int32 InvalidCount = 0;
    if (!TestFalse(TEXT("ARM N0 corruption is rejected on disposable level"),
        ARMCLEAN064X45::ValidateLoose(*Copy.Get(), *Registry.Get(),
            InvalidCount, Error)))
    {
        return false;
    }
    AddInfo(FString::Printf(
        TEXT("[ARM-CLEAN06.4X45] 44 native loose items, legacy pointers %d, ")
        TEXT("disposable pointerless dry-run + physical-level rejection; no package saved."),
        OriginalOldPointers));
    return !HasAnyErrors();
}
#endif
