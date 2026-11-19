#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "../ARMEMENT/GridEditorNativePalettePlan.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X55NativePalette,
    "Grimrock.RPG.ARM.CLEAN06_4X55.NativePalettePointerlessDryRun",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X55NativePalette::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    TStrongObjectPtr<UGridObjectPaletteAsset> Source(LoadObject<UGridObjectPaletteAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default")));
    if (!TestNotNull(TEXT("Real registry"), Root.Get()) ||
        !TestNotNull(TEXT("Real palette"), Source.Get()) ||
        !TestTrue(TEXT("Exact 34/38 registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
        return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    const bool bPaletteDirty = Source->GetOutermost()->IsDirty();
    const TArray<FGridObjectPaletteEntry> Original = Source->Entries;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Canonical registry initialized"),
        Registry->RegisterItemSystem(Root.Get(), Errors))) return false;

    TArray<FGridObjectPaletteEntry> Plan;
    FString Error;
    const bool bPlanned = FGridEditorNativePalettePlan::TryBuild(
        *Source, Registry.Get(), Plan, Error);
    if (!TestTrue(*FString::Printf(TEXT("Real palette dry-run: %s"), *Error), bPlanned) ||
        !TestEqual(TEXT("Complete palette entry count preserved"), Plan.Num(), Original.Num()) ||
        !TestNull(TEXT("Removed palette pointer schema"), FGridObjectPaletteEntry::StaticStruct()->FindPropertyByName(TEXT("DefaultItemDefinition"))))
        return false;

    auto SameEntry = [](const FGridObjectPaletteEntry& A, const FGridObjectPaletteEntry& B)
    {
        for (TFieldIterator<FProperty> Field(FGridObjectPaletteEntry::StaticStruct()); Field; ++Field)
        {
            if (!Field->Identical_InContainer(&A, &B)) return false;
        }
        return true;
    };

    int32 Direct = 0;
    int32 ArmIndex = INDEX_NONE;
    for (int32 Index = 0; Index < Plan.Num(); ++Index)
    {
        FGridObjectPaletteEntry Expected = Original[Index];
        if (Expected.IsDirectItemEntry())
        {
            ++Direct;
            if (Expected.CanonicalDefaultItemLevel > 0 && ArmIndex == INDEX_NONE)
                ArmIndex = Index;
        }
        if (!TestTrue(TEXT("Order and all reflected fields preserved"),
            SameEntry(Expected, Plan[Index])) ||
            !TestTrue(TEXT("Original entry untouched"), SameEntry(Original[Index], Source->Entries[Index])))
            return false;
    }
    if (!TestEqual(TEXT("Exactly 33 direct entries retained"), Direct, 33) ||
        !TestTrue(TEXT("At least one ARM entry exists"), ArmIndex != INDEX_NONE))
        return false;

    TStrongObjectPtr<UGridObjectPaletteAsset> Copy(
        NewObject<UGridObjectPaletteAsset>(GetTransientPackage()));
    Copy->Entries = Plan;
    TArray<FGridObjectPaletteEntry> Repeat;
    if (!TestTrue(TEXT("Already pointerless palette is accepted idempotently"),
        FGridEditorNativePalettePlan::TryBuild(*Copy.Get(), Registry.Get(), Repeat, Error)) ||
        !TestEqual(TEXT("Rechecked plan has same size"), Repeat.Num(), Plan.Num()))
        return false;
    for (int32 Index = 0; Index < Plan.Num(); ++Index)
        if (!TestTrue(TEXT("Repeated plan is identical"), SameEntry(Plan[Index], Repeat[Index])))
            return false;

    auto Refuses = [&](const TCHAR* Label)
    {
        TArray<FGridObjectPaletteEntry> Rejected = Plan;
        FString Reason;
        const bool bAccepted = FGridEditorNativePalettePlan::TryBuild(
            *Copy.Get(), Registry.Get(), Rejected, Reason);
        return TestFalse(Label, bAccepted) &&
            TestTrue(TEXT("Rejected transaction exposes no proposal"), Rejected.IsEmpty()) &&
            TestFalse(TEXT("Rejected transaction explains failure"), Reason.IsEmpty());
    };
    Copy->Entries[ArmIndex].CanonicalDefaultItemLevel = 0;
    if (!Refuses(TEXT("ARM N0 cannot pass native preflight"))) return false;
    Copy->Entries = Plan;
    Copy->Entries[ArmIndex].CanonicalItemId = TEXT("X55_UnknownItem");
    if (!Refuses(TEXT("Unknown ID cannot pass native preflight"))) return false;
    Copy->Entries = Plan;
    Copy->Entries.RemoveAt(ArmIndex);
    if (!Refuses(TEXT("Unexpected number of direct items rejected"))) return false;
    Copy->Entries = Plan;
    Copy->Entries[ArmIndex].DefaultWorldObjectDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Copy.Get());
    if (!Refuses(TEXT("Mixed direct identity and world source rejected"))) return false;

    AddInfo(TEXT("[ARM-CLEAN06.4X55] 33 real direct entries; canonical schema, field and presentation parity, no SavePackage."));
    return TestEqual(TEXT("Native palette remains read-only"), Source->GetOutermost()->IsDirty(), bPaletteDirty) &&
        TestEqual(TEXT("Canonical root remains read-only"), Root->GetOutermost()->IsDirty(), bRootDirty);
}
#endif
