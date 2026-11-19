#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Modules/ModuleManager.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorCanonicalLockRuleAuthoring.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN064X165
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

bool VerifyCopy(FAutomationTestBase& Test, const UGridItemSubsystem* Registry,
    const TArray<FName>& ExplicitIds)
{
    FGridLockBehaviorParams Source;
    Source.AcceptedKeyIds = ExplicitIds;
    Source.bConsumeKeyOnUnlock = true;
    FGridLockAcceptedKeyInstanceRuleValues Proposed;
    if (!Test.TestTrue(TEXT("Explicit key list copies atomically"),
        FGridEditorCanonicalLockRuleAuthoring::TryCopyDefaults(Registry, Source, Proposed)) ||
        !Test.TestTrue(TEXT("Exact order, duplicates and explicit IDs preserved"),
            Proposed.AcceptedKeyIds == ExplicitIds) ||
        !Test.TestTrue(TEXT("Source remains unchanged"), Source.AcceptedKeyIds == ExplicitIds))
        return false;
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X165KeyAtomicPreview,
    "Grimrock.RPG.ARM.CLEAN06_4X165.AtomicLockKeyUnionPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X165KeyAtomicPreview::RunTest(const FString&)
{
    ARMCLEAN064X165::FFixture F;
    if (!TestTrue(TEXT("Registered canonical ItemSystem"), F.bReady)) return false;
    const bool WasDirty = F.ItemSystem->GetOutermost()->IsDirty();
    const TArray<FName> Explicit = {TEXT("Key_Scripted_Quest"), TEXT("Key_Copper"),
        TEXT("Key_Scripted_Quest"), NAME_None, TEXT("Key_Iron")};
    if (!ARMCLEAN064X165::VerifyCopy(*this, F.Registry.Get(), Explicit) ||
        !ARMCLEAN064X165::VerifyCopy(*this, F.Registry.Get(), {})) return false;

    FGridLockBehaviorParams Source;
    Source.AcceptedKeyIds = Explicit;
    FGridLockAcceptedKeyInstanceRuleValues Out;
    Out.AcceptedKeyIds = {TEXT("KeepOnFailure")};
    return TestFalse(TEXT("Unavailable registry blocks change"),
            FGridEditorCanonicalLockRuleAuthoring::TryCopyDefaults(nullptr, Source, Out)) &&
        TestTrue(TEXT("Failure keeps previous output"), Out.AcceptedKeyIds == TArray<FName>{TEXT("KeepOnFailure")}) &&
        TestEqual(TEXT("Native catalogue not modified"), F.ItemSystem->GetOutermost()->IsDirty(), WasDirty);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X165RealKeyPreview,
    "Grimrock.RPG.ARM.CLEAN06_4X165.RealLockKeyListsReadOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X165RealKeyPreview::RunTest(const FString&)
{
    ARMCLEAN064X165::FFixture F;
    if (!TestTrue(TEXT("Canonical registry ready"), F.bReady)) return false;
    const bool SystemWasDirty = F.ItemSystem->GetOutermost()->IsDirty();
    const TCHAR* LevelPaths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };
    int32 InstanceLists = 0, DefaultLists = 0;
    for (const TCHAR* Path : LevelPaths)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(LoadObject<UGridLevelAsset>(nullptr, Path));
        if (!TestNotNull(TEXT("Actual level asset"), Level.Get())) return false;
        const bool WasDirty = Level->GetOutermost()->IsDirty();
        for (const FGridWorldObjectInstance& Object : Level->WorldObjectInstances)
        {
            const FGridWorldObjectInteractionOverrides& Overrides =
                Object.InstanceConfig.InteractionOverrides;
            if (!Overrides.bOverrideAcceptedKeys) continue;
            ++InstanceLists;
            if (!ARMCLEAN064X165::VerifyCopy(*this, F.Registry.Get(),
                Overrides.AcceptedKeys.AcceptedKeyIds)) return false;
        }
        if (!TestEqual(TEXT("No level package writes"),
            Level->GetOutermost()->IsDirty(), WasDirty)) return false;
    }

    FAssetRegistryModule& Module =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype")));
    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = true;
    TArray<FAssetData> Assets;
    Module.Get().GetAssets(Filter, Assets);
    int32 InspectedDefinitions = 0;
    for (const FAssetData& Entry : Assets)
    {
        UGridWorldObjectDefinitionAsset* Definition =
            Cast<UGridWorldObjectDefinitionAsset>(Entry.GetAsset());
        if (!TestNotNull(TEXT("Actual definition loads"), Definition)) return false;
        ++InspectedDefinitions;
        const bool WasDirty = Definition->GetOutermost()->IsDirty();
        const FGridLockBehaviorParams& Lock = Definition->DefaultBehavior.Lock;
        if (!Lock.AcceptedKeyIds.IsEmpty())
        {
            ++DefaultLists;
            if (!ARMCLEAN064X165::VerifyCopy(*this, F.Registry.Get(), Lock.AcceptedKeyIds)) return false;
        }
        if (!TestEqual(TEXT("Native definition read-only"),
            Definition->GetOutermost()->IsDirty(), WasDirty)) return false;
    }

    AddInfo(FString::Printf(TEXT("[ARM-CLEAN06.4X165] definitions=%d explicitDefaultLists=%d explicitInstanceLists=%d"),
        InspectedDefinitions, DefaultLists, InstanceLists));
    if (!TestEqual(TEXT("Two real shared lock lists"), DefaultLists, 2) ||
        !TestEqual(TEXT("No active native instance lock overrides"), InstanceLists, 0)) return false;
    return TestTrue(TEXT("Native definitions were scanned"),
        InspectedDefinitions > 0) &&
        TestEqual(TEXT("Canonical ItemSystem remains read-only"),
            F.ItemSystem->GetOutermost()->IsDirty(), SystemWasDirty);
}
#endif
