#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "../ARMEMENT/GridEditorNativeDormantOverridesPlan.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X59NativeDormantPlan,
    "Grimrock.RPG.ARM.CLEAN06_4X59.NativeDormantOverridesPlan",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X59NativeDormantPlan::RunTest(const FString&)
{
    using FPlan = FGridEditorNativeDormantOverridesPlan;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real ItemSystem"), Root.Get()) ||
        !TestTrue(TEXT("Exact 34/38 registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
        return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Real ItemSystem configured"), Registry->RegisterItemSystem(Root.Get(), Errors)))
        return false;

    TArray<TStrongObjectPtr<UGridLevelAsset>> Owned;
    TArray<UGridLevelAsset*> Levels;
    TArray<bool> Dirty;
    for (int32 Index = 0; Index < FPlan::LevelCount; ++Index)
    {
        Owned.Emplace(LoadObject<UGridLevelAsset>(nullptr, FPlan::Path(Index)));
        UGridLevelAsset* Level = Owned.Last().Get();
        if (!TestNotNull(TEXT("Exact real level"), Level)) return false;
        Levels.Add(Level);
        Dirty.Add(Level->GetOutermost()->IsDirty());
    }
    TArray<FPlan::FLevel> Plan;
    FString Error;
    int32 Old = INDEX_NONE;
    const bool bPlan = FPlan::TryBuild(Levels, Registry.Get(), Plan, Old, Error);
    if (!TestTrue(*FString::Printf(TEXT("Exact real dormant plan: %s"), *Error), bPlan) ||
        !TestEqual(TEXT("No dormant source pointers"), Old, 0) ||
        !TestEqual(TEXT("Four level projections"), Plan.Num(), 4))
        return false;
    for (int32 Index = 0; Index < Levels.Num(); ++Index)
    {
        TArray<FGridWorldObjectInstance> Expected = Levels[Index]->WorldObjectInstances;
        int32 Rows = 0;
        for (FGridWorldObjectInstance& Object : Expected)
        {
            const int32 Target = FPlan::TargetIndex(Object.InstanceId);
            if (Target == INDEX_NONE) continue;
            ++Rows;
            auto& O = Object.InstanceConfig.InteractionOverrides;
            if (Target == 0) O.ReceptacleRules.AcceptedItems.Reset();
        }
        if (!TestEqual(TEXT("Original distribution preserved"), Rows, FPlan::OriginalRows(Index)) ||
            !TestTrue(TEXT("Every reflected field and ordering preserved except old source"),
                FPlan::SameObjects(Expected, Plan[Index].WorldObjects)) ||
            !TestFalse(TEXT("Complete normalized level manifest not empty"),
                FPlan::Manifest(*Levels[Index], Plan[Index]).IsEmpty()) ||
            !TestEqual(TEXT("Original package remains read-only"), Levels[Index]->GetOutermost()->IsDirty(), Dirty[Index]))
            return false;
    }

    TStrongObjectPtr<UGridLevelAsset> Copy(NewObject<UGridLevelAsset>(GetTransientPackage()));
    Copy->Width = Levels[0]->Width;
    Copy->Height = Levels[0]->Height;
    Copy->WorldObjectInstances = Plan[0].WorldObjects;
    FPlan::FLevel Repeated;
    if (!TestTrue(TEXT("Dormant cleanup is idempotent"),
            FPlan::TryBuildLevel(*Copy.Get(), Registry.Get(), 0, Repeated, Error)) ||
        !TestEqual(TEXT("No old dormant sources survive"), Repeated.Old, 0) ||
        !TestTrue(TEXT("Projection remains identical"), FPlan::SameObjects(Repeated.WorldObjects, Plan[0].WorldObjects)))
        return false;

    auto* Owner = Copy->WorldObjectInstances.FindByPredicate([](const FGridWorldObjectInstance& Object)
        { return FPlan::TargetIndex(Object.InstanceId) == 0; });
    if (!TestNotNull(TEXT("Exact native torch holder"), Owner)) return false;
    Owner->InstanceConfig.InteractionOverrides.bOverrideReceptacleRules = true;
    if (!TestFalse(TEXT("Active override cannot be cleaned"),
            FPlan::TryBuildLevel(*Copy.Get(), Registry.Get(), 0, Repeated, Error)) ||
        !TestTrue(TEXT("Refused plan yields no proposal"), Repeated.WorldObjects.IsEmpty()))
        return false;

    Owner->InstanceConfig.InteractionOverrides.bOverrideReceptacleRules = false;
    ++Owner->CellX;
    if (!TestFalse(TEXT("Wrong owner cell refused"), FPlan::TryBuildLevel(*Copy.Get(), Registry.Get(), 0, Repeated, Error)))
        return false;
    --Owner->CellX;
    FGridReceptacleAcceptedItemConfig& Wrong = Owner->InstanceConfig.InteractionOverrides.ReceptacleRules.AcceptedItems.AddDefaulted_GetRef();
    Wrong.ItemId = TEXT("Gem_Blue");
    if (!TestFalse(TEXT("Unexpected authored rule refused"), FPlan::TryBuildLevel(*Copy.Get(), Registry.Get(), 0, Repeated, Error)))
        return false;

    AddInfo(FString::Printf(TEXT("[ARM-CLEAN06.4X59] three exact dormant owners; old=%d; effective behavior and canonical reactivation verified."), Old));
    return TestEqual(TEXT("Real ItemSystem unmodified"), Root->GetOutermost()->IsDirty(), bRootDirty);
}
#endif
