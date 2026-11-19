#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorLootDefinitionProjection.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X61LootProjection,
    "Grimrock.RPG.ARM.CLEAN06_4X61.FourLootDefinitionProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X61LootProjection::RunTest(const FString&)
{
    using FPlan = FGridEditorLootDefinitionProjection;
    using FStone = FPlan::FStone;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    TStrongObjectPtr<UGridMonsterDefinitionAsset> Goblin(LoadObject<UGridMonsterDefinitionAsset>(nullptr, FStone::MonsterPath(0)));
    TStrongObjectPtr<UGridMonsterDefinitionAsset> Rat(LoadObject<UGridMonsterDefinitionAsset>(nullptr, FStone::MonsterPath(1)));
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
    if (!TestNotNull(TEXT("Real root"), Root.Get()) ||
        !TestNotNull(TEXT("Real Goblin"), Goblin.Get()) ||
        !TestNotNull(TEXT("Real Rat"), Rat.Get()) ||
        !TestTrue(TEXT("Exact registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
        return false;
    const auto OriginalDefinitions = Root->ItemDefinitions;
    const FString OriginalRoot = FPlan::RootManifest(*Root.Get());
    const FString OriginalGoblin = FStone::Export(*Goblin.Get());
    const FString OriginalRat = FStone::Export(*Rat.Get());

    FString Error;
    const bool bProjection = FPlan::BuildPreview(*Root.Get(), *Preview.Get(), Error, true);
    if (!TestTrue(*FString::Printf(TEXT("Exact four real definitions: %s"), *Error), bProjection) ||
        !TestEqual(TEXT("Exact projected identities"), Preview->ItemDefinitions.Num(), FMath::Max(FPlan::FinalCount, OriginalDefinitions.Num())) ||
        !TestTrue(TEXT("Original 34 and exact four appended identities"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Preview.Get())) ||
        !TestTrue(TEXT("Four catalogue references identical"),
            Preview->WeaponCatalogue == Root->WeaponCatalogue &&
            Preview->WeaponVisualCatalogue == Root->WeaponVisualCatalogue &&
            Preview->ActionCatalogue == Root->ActionCatalogue &&
            Preview->ActionPresentationCatalogue == Root->ActionPresentationCatalogue))
        return false;

    TArray<FGridMonsterLootEntry> GoblinPlan, RatPlan;
    const bool bMigrated = OriginalDefinitions.Num() >= FPlan::FinalCount;
    if (!TestTrue(TEXT("Native loot cutover must already be complete"), bMigrated)) return false;
    TestNull(TEXT("Legacy loot pointer property removed"), FGridMonsterLootEntry::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")));
    if (!TestTrue(TEXT("Both tables are entirely in one native state"),
        FPlan::BuildLootTables(*Goblin.Get(), *Rat.Get(), bMigrated, GoblinPlan, RatPlan, Error)))
    {
        AddError(Error);
        return false;
    }
    for (const auto& Row : GoblinPlan)
    {
        TestTrue(TEXT("Goblin physical N0"), Row.ItemLevel == 0 && !Row.ItemDefinitionId.IsNone());
    }
    for (const auto& Row : RatPlan)
    {
        TestTrue(TEXT("Rat physical N0"), Row.ItemLevel == 0 && !Row.ItemDefinitionId.IsNone());
    }
    TestTrue(TEXT("Goblin 128 unchanged RNG sequences"), FStone::SameRolls(Goblin->LootTable, GoblinPlan));
    TestTrue(TEXT("Rat 128 unchanged RNG sequences"), FStone::SameRolls(Rat->LootTable, RatPlan));

    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Register complete preview"), Registry->RegisterItemSystem(Preview.Get(), Errors))) return false;
    for (int32 I = 0; I < FPlan::AddedCount; ++I)
    {
        const UGridItemDefinitionAsset* Source = FStone::LoadSource(FPlan::SourceIndex(I));
        if (!TestNotNull(TEXT("Native source"), Source) ||
            !TestTrue(TEXT("Exact gameplay and presentation parity"), FPlan::ResolveParity(*Registry.Get(), *Source, I)))
            return false;
        auto* Definition = Cast<UGridNonArmamentDefinitionAsset>(Preview->ItemDefinitions[FPlan::BaseCount+I].Get());
        if (!TestNotNull(TEXT("Projected nonarm definition"), Definition)) return false;
        auto* Counterfeit = Cast<UGridNonArmamentDefinitionAsset>(
            NewObject<UObject>(GetTransientPackage(), FPlan::ClassFor(I), NAME_None, RF_Transient));
        if (!TestNotNull(TEXT("Conflicting transient definition"), Counterfeit) ||
            !TestTrue(TEXT("Projection re-created exactly"), FPlan::Project(*Source, I, *Counterfeit)) ||
            !TestTrue(TEXT("Every reflected canonical property matches"), FPlan::SameDefinition(*Definition, *Counterfeit)))
            return false;
        Counterfeit->WeightKg += 1.f;
        TestFalse(TEXT("Conflicting canonical mass is detected"), FPlan::SameDefinition(*Definition, *Counterfeit));
        if (bMigrated)
            TestEqual(TEXT("Persisted canonical asset exact path"), Definition->GetPathName(), FPlan::TargetPath(I));
    }

    // The builder refuses an inconsistent serialized root without writing it.
    TStrongObjectPtr<UGridItemSystemAsset> Partial(NewObject<UGridItemSystemAsset>());
    Partial->ItemDefinitions = Preview->ItemDefinitions;
    Partial->ItemDefinitions.Pop();
    TStrongObjectPtr<UGridItemSystemAsset> Refused(NewObject<UGridItemSystemAsset>());
    TestFalse(TEXT("Partial cutover refused"), FPlan::BuildPreview(*Partial.Get(), *Refused.Get(), Error));
    TestTrue(TEXT("Ordered source root untouched"), Root->ItemDefinitions == OriginalDefinitions &&
        FPlan::RootManifest(*Root.Get()) == OriginalRoot);
    TestTrue(TEXT("Goblin unchanged"), FStone::Export(*Goblin.Get()) == OriginalGoblin);
    TestTrue(TEXT("Rat unchanged"), FStone::Export(*Rat.Get()) == OriginalRat);
    TestFalse(TEXT("No package writes"), Root->GetOutermost()->IsDirty() ||
        Goblin->GetOutermost()->IsDirty() || Rat->GetOutermost()->IsDirty());
    AddInfo(TEXT("[ARM-CLEAN06.4X61] Four exact N0 definitions, original 34 kept, both tables and all RNG rolls preserved, no native writes."));
    return !HasAnyErrors();
}
#endif
