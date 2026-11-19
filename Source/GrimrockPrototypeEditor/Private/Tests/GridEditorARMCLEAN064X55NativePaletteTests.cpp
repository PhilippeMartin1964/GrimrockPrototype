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
    int32 Old = 0;
    const bool bPlanned = FGridEditorNativePalettePlan::TryBuild(
        *Source, Registry.Get(), Plan, Old, Error);
    if (!TestTrue(*FString::Printf(TEXT("Real palette dry-run: %s"), *Error), bPlanned) ||
        !TestEqual(TEXT("Complete palette entry count preserved"), Plan.Num(), Original.Num()) ||
        !TestEqual(TEXT("All 33 pointers already removed"), Old, 0))
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
    int32 RepeatOld = -1;
    if (!TestTrue(TEXT("Already pointerless palette is accepted idempotently"),
        FGridEditorNativePalettePlan::TryBuild(*Copy.Get(), Registry.Get(), Repeat, RepeatOld, Error)) ||
        !TestEqual(TEXT("No pointers in rechecked plan"), RepeatOld, 0) ||
        !TestEqual(TEXT("Rechecked plan has same size"), Repeat.Num(), Plan.Num()))
        return false;
    for (int32 Index = 0; Index < Plan.Num(); ++Index)
        if (!TestTrue(TEXT("Repeated plan is identical"), SameEntry(Plan[Index], Repeat[Index])))
            return false;

    auto Refuses = [&](const TCHAR* Label)
    {
        TArray<FGridObjectPaletteEntry> Rejected = Plan;
        int32 RejectedOld = -1;
        FString Reason;
        const bool bAccepted = FGridEditorNativePalettePlan::TryBuild(
            *Copy.Get(), Registry.Get(), Rejected, RejectedOld, Reason);
        return TestFalse(Label, bAccepted) &&
            TestTrue(TEXT("Rejected transaction exposes no proposal"), Rejected.IsEmpty()) &&
            TestEqual(TEXT("Rejected transaction exposes no old count"), RejectedOld, 0) &&
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
    UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>(Copy.Get());
    Legacy->ItemDefinitionId = Plan[ArmIndex].CanonicalItemId;
    Copy->Entries[ArmIndex].DefaultItemDefinition = Legacy;
    if (!Refuses(TEXT("Even matching historical pointer rejected after cutover"))) return false;

    AddInfo(FString::Printf(TEXT("[ARM-CLEAN06.4X55] 33 real direct entries, old=%d; "
        "complete pointerless projection, field and presentation parity, no SavePackage."), Old));
    return TestEqual(TEXT("Native palette remains read-only"), Source->GetOutermost()->IsDirty(), bPaletteDirty) &&
        TestEqual(TEXT("Canonical root remains read-only"), Root->GetOutermost()->IsDirty(), bRootDirty);
}
#endif
