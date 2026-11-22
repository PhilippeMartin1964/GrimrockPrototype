#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Modules/ModuleManager.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorReceptacleRuleAuthoring.h"
#include "Runtime/Items/GridItemCatalogues.h"
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
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")))
        , GI(NewObject<UGameInstance>())
        , Registry(NewObject<UGridItemSubsystem>(GI.Get()))
    {
        TArray<FString> Errors;
        bReady = ItemSystem.IsValid() && Registry.IsValid() &&
            Registry->RegisterItemSystem(ItemSystem.Get(), Errors);
    }
};

bool VerifyPlanInvariance(FAutomationTestBase& Test, const UGridItemSubsystem* Registry,
    const TArray<FGridReceptacleAcceptedItemConfig>& Input, int32& SafeCount)
{
    TArray<FGridReceptacleAcceptedItemConfig> Copy;
    if (!Test.TestTrue(TEXT("Explicit rules copy atomically"),
        FGridEditorReceptacleRuleAuthoring::TryCopyDefaults(Registry, Input, Copy)) ||
        !Test.TestEqual(TEXT("Copy preserves exact rule count"), Copy.Num(), Input.Num())) return false;
    for (int32 Index = 0; Index < Input.Num(); ++Index)
        if (!Test.TestEqual(TEXT("Identity and order preserved"), Copy[Index].ItemId, Input[Index].ItemId)) return false;
    ++SafeCount;
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X164AtomicPreview,
    "Grimrock.RPG.ARM.CLEAN06_4X164.AtomicAcceptedItemMigrationPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X164AtomicPreview::RunTest(const FString&)
{
    ARMCLEAN064X164::FFixture F;
    if (!TestTrue(TEXT("Actual canonical registry"), F.bReady)) return false;
    const bool bDirtyBefore = F.ItemSystem->GetOutermost()->IsDirty();
    FGridReceptacleAcceptedItemConfig A, B;
    A.ItemId = TEXT("Item_Arrows_01");
    B.ItemId = TEXT("Weapon_StraightSword_01");
    TArray<FGridReceptacleAcceptedItemConfig> Rules = {A, B, A, A};
    int32 Safe = 0;
    if (!ARMCLEAN064X164::VerifyPlanInvariance(*this, F.Registry.Get(), Rules, Safe) ||
        !ARMCLEAN064X164::VerifyPlanInvariance(*this, F.Registry.Get(), {}, Safe)) return false;
    TArray<FGridReceptacleAcceptedItemConfig> Proposed;
    FGridReceptacleAcceptedItemConfig Invalid;
    Invalid.ItemId = TEXT("X164_Unregistered");
    Rules.Add(Invalid);
    if (!TestFalse(TEXT("Unknown identity blocks entire list"),
            FGridEditorReceptacleRuleAuthoring::TryCopyDefaults(F.Registry.Get(), Rules, Proposed)) ||
        !TestTrue(TEXT("No partial output"), Proposed.IsEmpty())) return false;
    Rules.Last().ItemId = NAME_None;
    if (!TestFalse(TEXT("Empty identity blocks entire list"),
            FGridEditorReceptacleRuleAuthoring::TryCopyDefaults(F.Registry.Get(), Rules, Proposed)) ||
        !TestTrue(TEXT("No partial output"), Proposed.IsEmpty())) return false;
    Rules.Pop();
    return TestFalse(TEXT("No registry never copies rules"),
            FGridEditorReceptacleRuleAuthoring::TryCopyDefaults(nullptr, Rules, Proposed)) &&
        TestTrue(TEXT("No partial output"), Proposed.IsEmpty()) &&
        TestEqual(TEXT("Native catalogue not modified"), F.ItemSystem->GetOutermost()->IsDirty(), bDirtyBefore);
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
                    Overrides.ReceptacleRules.AcceptedItems, SafeInstances)) return false;
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
                Receptacle.AcceptedItems, SafeDefaults))
                return false;
        }
        DefaultExplicitKeyIds += Definition->DefaultBehavior.Lock.AcceptedKeyIds.Num();
        if (!TestEqual(TEXT("Definition package unchanged by preview"),
            Definition->GetOutermost()->IsDirty(), bDirty))
            return false;
    }
    AddInfo(FString::Printf(TEXT("[ARM-CLEAN06.4X164] levels=%d definitions=%d ruleLists=%d/%d safe=%d/%d"),
        LevelCount, DefinitionCount, DefaultLists, InstanceLists, SafeDefaults, SafeInstances));
    if (!TestEqual(TEXT("Six actual receptacle lists copied"), SafeDefaults, 6) ||
        !TestEqual(TEXT("No active native receptacle overrides"), SafeInstances, 0)) return false;
    return TestEqual(TEXT("Four real levels checked"), LevelCount, 4) &&
        TestTrue(TEXT("At least one definition class asset scanned"), DefinitionCount > 0) &&
        TestEqual(TEXT("ItemSystem package never modified"),
            F.ItemSystem->GetOutermost()->IsDirty(), bItemSystemDirty);
}
#endif
