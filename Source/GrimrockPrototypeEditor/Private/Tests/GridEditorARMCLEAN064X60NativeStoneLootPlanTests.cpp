#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorNativeStoneLootPlan.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X60StonePlan,
    "Grimrock.RPG.ARM.CLEAN06_4X60.NativeStoneLootPlan",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X60StonePlan::RunTest(const FString&)
{
    using FPlan = FGridEditorNativeStoneLootPlan;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    TStrongObjectPtr<UGridMonsterDefinitionAsset> Goblin(LoadObject<UGridMonsterDefinitionAsset>(nullptr, FPlan::MonsterPath(0)));
    TStrongObjectPtr<UGridMonsterDefinitionAsset> Rat(LoadObject<UGridMonsterDefinitionAsset>(nullptr, FPlan::MonsterPath(1)));
    if (!TestNotNull(TEXT("Canonical root"), Root.Get()) ||
        !TestNotNull(TEXT("Goblin source"), Goblin.Get()) ||
        !TestNotNull(TEXT("Rat source"), Rat.Get())) return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    const bool bGoblinDirty = Goblin->GetOutermost()->IsDirty();
    const bool bRatDirty = Rat->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Real registry configured"), Registry->RegisterItemSystem(Root.Get(), Errors))) return false;
    TArray<FGridMonsterLootEntry> Plan;
    int32 Old = INDEX_NONE;
    FString Error;
    const bool bPlanned = FPlan::TryBuild(*Goblin.Get(), *Rat.Get(), Registry.Get(), Plan, Old, Error);
    if (!TestTrue(*FString::Printf(TEXT("Exact native Stone projection: %s"), *Error), bPlanned) ||
        !TestEqual(TEXT("No historical pointer state remains"), Old, 0) ||
        !TestEqual(TEXT("Three goblin entries"), Plan.Num(), 3)) return false;
    auto Expected = Goblin->LootTable;
    Expected[1].ItemDefinitionId = TEXT("Stone");
    if (!TestTrue(TEXT("All reflected loot fields except approved identity pair unchanged"),
            FPlan::SameTable(Expected, Plan)) ||
        !TestTrue(TEXT("128 deterministic seeds preserve every roll"), FPlan::SameRolls(Goblin->LootTable, Plan)) ||
        !TestFalse(TEXT("Normalized full monster manifest exists"), FPlan::Export(*Goblin.Get(), &Plan).IsEmpty()))
        return false;
    auto Altered = Plan;
    Altered[1].DropChance = 0.f;
    if (!TestFalse(TEXT("Probability modification detected"), FPlan::SameTable(Altered, Plan))) return false;
    int32 Level = INDEX_NONE;
    Altered = Plan;
    Altered[1].ItemLevel = 1;
    if (!TestFalse(TEXT("Stone N1 refused"),
            FGridMonsterLootResolver::TryResolveItemLevel(Altered[1], Level, Registry.Get()))) return false;
    // Source evidence is read-only; the remaining four can only be registered as one complete X.61 cutover.
    for (int32 Index = 0; Index < 5; ++Index)
    {
        const auto* Source = FPlan::LoadSource(Index);
        if (!TestNotNull(TEXT("Exact legacy evidence source"), Source) ||
            !TestFalse(TEXT("Complete source export"), FPlan::Export(*Source).IsEmpty())) return false;
    }
    return TestEqual(TEXT("Registry read-only"), Root->GetOutermost()->IsDirty(), bRootDirty) &&
        TestEqual(TEXT("Goblin read-only"), Goblin->GetOutermost()->IsDirty(), bGoblinDirty) &&
        TestEqual(TEXT("Rat read-only"), Rat->GetOutermost()->IsDirty(), bRatDirty);
}
#endif
