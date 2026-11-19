#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Modules/ModuleManager.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorReceptacleAcceptanceMigrationPreview.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN064X164
{
struct FFixture
{
    TStrongObjectPtr<UGridItemSystemAsset> ItemSystem;
    TStrongObjectPtr<UGameInstance> GI;
    TStrongObjectPtr<UGridItemSubsystem> Registry;
    bool bReady = false;
    FFixture()
        : ItemSystem(LoadObject<UGridItemSystemAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")))
        , GI(NewObject<UGameInstance>())
        , Registry(NewObject<UGridItemSubsystem>(GI.Get()))
    {
        TArray<FString> Errors;
        bReady = ItemSystem.IsValid() && Registry.IsValid() &&
            Registry->RegisterItemSystem(ItemSystem.Get(), Errors);
    }
};

bool VerifyPlanInvariance(FAutomationTestBase& Test, const UGridItemSubsystem* Registry,
    const TArray<FGridReceptacleAcceptedItemConfig>& Input,
    FGridReceptacleAcceptancePlanStats& Total, int32& bPreviewSafeCount)
{
    const TArray<FGridReceptacleAcceptedItemConfig> Source = Input;
    TArray<FGridReceptacleAcceptedItemConfig> Preview;
    FGridReceptacleAcceptancePlanStats Stats;
    const bool bSafe = FGridReceptacleAcceptanceMigrationPreview::TryPreview(
        Registry, Input, Preview, Stats);
    Total.Accumulate(Stats);
    bPreviewSafeCount += bSafe ? 1 : 0;
    if (!Test.TestEqual(TEXT("Preview did not edit source count"), Input.Num(), Source.Num()) ||
        !Test.TestEqual(TEXT("Stats count exact"), Stats.Rules, Input.Num()) ||
        !Test.TestEqual(TEXT("Blocked list produces no partial proposal"),
            Preview.Num(), bSafe ? Input.Num() : 0))
        return false;
    for (int32 Index = 0; Index < Source.Num(); ++Index)
    {
        if (!Test.TestTrue(TEXT("Preview cannot modify serialized rule fields"),
            Input[Index].ItemDefinition == Source[Index].ItemDefinition &&
            Input[Index].CanonicalItemId == Source[Index].CanonicalItemId))
            return false;
        if (bSafe)
        {
            const FGridReceptacleAcceptedItemConfig& Before = Source[Index];
            const FGridReceptacleAcceptedItemConfig& After = Preview[Index];
            const FName OldId = Before.CanonicalItemId.IsNone()
                ? (IsValid(Before.ItemDefinition.Get())
                    ? Before.ItemDefinition->ItemDefinitionId : NAME_None)
                : Before.CanonicalItemId;
            if (!Test.TestTrue(TEXT("Strict identity order/pointer parity"),
                OldId == After.CanonicalItemId &&
                After.ItemDefinition == Before.ItemDefinition))
                return false;
        }
    }
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X164AtomicPreview,
    "Grimrock.RPG.ARM.CLEAN06_4X164.AtomicAcceptedItemMigrationPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X164AtomicPreview::RunTest(const FString&)
{
    ARMCLEAN064X164::FFixture F;
    if (!TestTrue(TEXT("Actual canonical registry"), F.bReady))
        return false;
    const bool bDirtyBefore = F.ItemSystem->GetOutermost()->IsDirty();
    UGridItemDefinitionAsset* KnownA = NewObject<UGridItemDefinitionAsset>();
    KnownA->ItemDefinitionId = TEXT("Item_Arrows_01");
    UGridItemDefinitionAsset* KnownB = NewObject<UGridItemDefinitionAsset>();
    KnownB->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
    UGridItemDefinitionAsset* Unknown = NewObject<UGridItemDefinitionAsset>();
    Unknown->ItemDefinitionId = TEXT("X164_Unregistered_Historical");

    FGridReceptacleAcceptedItemConfig LegacyA;
    LegacyA.ItemDefinition = KnownA;
    FGridReceptacleAcceptedItemConfig LegacyB;
    LegacyB.ItemDefinition = KnownB;
    FGridReceptacleAcceptedItemConfig Explicit;
    Explicit.CanonicalItemId = KnownA->ItemDefinitionId;
    TArray<FGridReceptacleAcceptedItemConfig> Rules = {
        LegacyA, LegacyB, LegacyA, Explicit
    };
    TArray<FGridReceptacleAcceptedItemConfig> Proposed;
    FGridReceptacleAcceptancePlanStats Stats;
    if (!TestTrue(TEXT("Safe duplicate/order-preserving preview"),
            FGridReceptacleAcceptanceMigrationPreview::TryPreview(
                F.Registry.Get(), Rules, Proposed, Stats)) ||
        !TestEqual(TEXT("All 4 rules preserved"), Proposed.Num(), 4) ||
        !TestEqual(TEXT("3 eligible legacy sources"), Stats.EligibleLegacy, 3) ||
        !TestEqual(TEXT("1 existing canonical rule"), Stats.AlreadyCanonical, 1) ||
        !TestEqual(TEXT("No item level inferred from acceptance rules"),
            Proposed[1].CanonicalItemId, KnownB->ItemDefinitionId) ||
        !TestTrue(TEXT("Shared first rule remains unmodified"),
            Rules[0].CanonicalItemId.IsNone()) ||
        !TestTrue(TEXT("Duplicate rule order intact"),
            Proposed[0].CanonicalItemId == Proposed[2].CanonicalItemId &&
            Proposed[0].ItemDefinition == Proposed[2].ItemDefinition))
        return false;

    FGridReceptacleAcceptedItemConfig Unmapped;
    Unmapped.ItemDefinition = Unknown;
    Rules.Add(Unmapped);
    if (!TestFalse(TEXT("Unknown historical family blocks entire list"),
            FGridReceptacleAcceptanceMigrationPreview::TryPreview(
                F.Registry.Get(), Rules, Proposed, Stats)) ||
        !TestEqual(TEXT("Unknown legacy source counted"), Stats.UnknownLegacy, 1) ||
        !TestTrue(TEXT("No partial migration can escape"), Proposed.IsEmpty()))
        return false;

    Rules.Pop();
    FGridReceptacleAcceptedItemConfig Conflict = Explicit;
    Conflict.ItemDefinition = KnownB;
    Rules.Add(Conflict);
    if (!TestFalse(TEXT("Persisted contradictory identity blocks entire list"),
            FGridReceptacleAcceptanceMigrationPreview::TryPreview(
                F.Registry.Get(), Rules, Proposed, Stats)) ||
        !TestEqual(TEXT("Contradictory source count"), Stats.ContradictoryCanonical, 1) ||
        !TestTrue(TEXT("Contradiction leaves preview empty"), Proposed.IsEmpty()))
        return false;

    Rules.Pop();
    FGridReceptacleAcceptedItemConfig Missing;
    Rules.Add(Missing);
    if (!TestFalse(TEXT("Missing authoring is not silently converted"),
            FGridReceptacleAcceptanceMigrationPreview::TryPreview(
                F.Registry.Get(), Rules, Proposed, Stats)) ||
        !TestEqual(TEXT("Missing source counted"), Stats.MissingSource, 1))
        return false;

    return TestFalse(TEXT("No registry never plans migration"),
            FGridReceptacleAcceptanceMigrationPreview::TryPreview(
                nullptr, Rules, Proposed, Stats)) &&
        TestTrue(TEXT("No registry flagged"), Stats.bRegistryUnavailable) &&
        TestEqual(TEXT("Native canonical asset not dirtied"),
            F.ItemSystem->GetOutermost()->IsDirty(), bDirtyBefore);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X164RealSourcesPreview,
    "Grimrock.RPG.ARM.CLEAN06_4X164.RealReceptacleRulesDryRun",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X164RealSourcesPreview::RunTest(const FString&)
{
    ARMCLEAN064X164::FFixture F;
    if (!TestTrue(TEXT("Real 34-definition ItemSystem available"), F.bReady))
        return false;
    const bool bItemSystemDirty = F.ItemSystem->GetOutermost()->IsDirty();
    const TCHAR* LevelPaths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };
    FGridReceptacleAcceptancePlanStats InstanceStats, DefaultStats;
    int32 InstanceLists = 0, DefaultLists = 0;
    int32 SafeInstances = 0, SafeDefaults = 0, LevelCount = 0;
    int32 ExplicitKeysInOverrides = 0, KeyPointersInOverrides = 0;
    for (const TCHAR* Path : LevelPaths)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(LoadObject<UGridLevelAsset>(nullptr, Path));
        if (!TestNotNull(TEXT("One of four native levels"), Level.Get()))
            return false;
        ++LevelCount;
        const bool bDirty = Level->GetOutermost()->IsDirty();
        for (const FGridWorldObjectInstance& Object : Level->WorldObjectInstances)
        {
            const FGridWorldObjectInteractionOverrides& Overrides =
                Object.InstanceConfig.InteractionOverrides;
            if (Overrides.bOverrideReceptacleRules)
            {
                ++InstanceLists;
                if (!ARMCLEAN064X164::VerifyPlanInvariance(*this, F.Registry.Get(),
                    Overrides.ReceptacleRules.AcceptedItems, InstanceStats,
                    SafeInstances)) return false;
            }
            if (Overrides.bOverrideAcceptedKeys)
            {
                ExplicitKeysInOverrides += Overrides.AcceptedKeys.AcceptedKeyIds.Num();
            }
        }
        if (!TestEqual(TEXT("Real level package remains read-only"),
            Level->GetOutermost()->IsDirty(), bDirty))
            return false;
    }

    FAssetRegistryModule& Module =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype")));
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    TArray<FAssetData> Assets;
    Module.Get().GetAssets(Filter, Assets);
    int32 DefinitionCount = 0;
    int32 DefaultKeyPointers = 0, DefaultExplicitKeyIds = 0;
    for (const FAssetData& Entry : Assets)
    {
        UGridWorldObjectDefinitionAsset* Definition =
            Cast<UGridWorldObjectDefinitionAsset>(Entry.GetAsset());
        if (!TestNotNull(TEXT("Native definition loads"), Definition))
            return false;
        ++DefinitionCount;
        const bool bDirty = Definition->GetOutermost()->IsDirty();
        const FGridReceptacleBehaviorParams& Receptacle =
            Definition->DefaultBehavior.Receptacle;
        if (Definition->SupportedType == EGridLevelObjectType::Receptacle ||
            !Receptacle.AcceptedItems.IsEmpty())
        {
            ++DefaultLists;
            if (!ARMCLEAN064X164::VerifyPlanInvariance(*this, F.Registry.Get(),
                Receptacle.AcceptedItems, DefaultStats, SafeDefaults))
                return false;
        }
        DefaultExplicitKeyIds += Definition->DefaultBehavior.Lock.AcceptedKeyIds.Num();
        if (!TestEqual(TEXT("Definition package unchanged by preview"),
            Definition->GetOutermost()->IsDirty(), bDirty))
            return false;
    }
    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X.16.4] levels=%d definitions=%d "
        "instanceRuleLists=%d safeInstanceLists=%d {%s} "
        "definitionRuleLists=%d safeDefinitionLists=%d {%s} "
        "lockDefaultPointers=%d lockDefaultExplicitIds=%d "
        "lockInstancePointers=%d lockInstanceExplicitIds=%d"),
        LevelCount, DefinitionCount, InstanceLists, SafeInstances,
        *InstanceStats.Describe(), DefaultLists, SafeDefaults,
        *DefaultStats.Describe(), DefaultKeyPointers, DefaultExplicitKeyIds,
        KeyPointersInOverrides, ExplicitKeysInOverrides));
    if (InstanceStats.HasBlockers() || DefaultStats.HasBlockers())
    {
        AddWarning(TEXT("Some acceptance lists have blockers. Keep all native assets "
            "unchanged and resolve each unknown/contradictory source before conversion."));
    }
    return TestEqual(TEXT("Four real levels checked"), LevelCount, 4) &&
        TestTrue(TEXT("At least one definition class asset scanned"), DefinitionCount > 0) &&
        TestEqual(TEXT("ItemSystem package never modified"),
            F.ItemSystem->GetOutermost()->IsDirty(), bItemSystemDirty);
}
#endif
