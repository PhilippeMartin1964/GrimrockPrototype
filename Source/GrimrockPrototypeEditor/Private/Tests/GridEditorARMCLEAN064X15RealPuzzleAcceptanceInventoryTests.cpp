#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Modules/ModuleManager.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN064X15
{
struct FReferenceCount
{
    int32 Total = 0;
    int32 CanonicalKnown = 0;
    int32 Unrecognized = 0;
    int32 Missing = 0;
    int32 Duplicates = 0;

    void Visit(FName ItemId, const UGridItemSubsystem* Registry)
    {
        ++Total;
        if (ItemId.IsNone())
        {
            ++Missing;
        }
        else if (Registry && Registry->ContainsItem(ItemId))
        {
            ++CanonicalKnown;
        }
        else
        {
            ++Unrecognized;
        }
    }
};

FString Summary(const FReferenceCount& C)
{
    return FString::Printf(TEXT("total=%d canonical=%d unknown=%d missing=%d"),
        C.Total, C.CanonicalKnown, C.Unrecognized, C.Missing);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorClean064X15RealAcceptanceInventory,
    "Grimrock.RPG.ARM.CLEAN06_4X15.RealPuzzleAcceptanceSourcesReadOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorClean064X15RealAcceptanceInventory::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
    TStrongObjectPtr<UGridObjectPaletteAsset> Palette(LoadObject<UGridObjectPaletteAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default")));
    if (!TestNotNull(TEXT("Actual ItemSystem"), Root.Get()) ||
        !TestNotNull(TEXT("Actual editor palette"), Palette.Get()) ||
        !TestTrue(TEXT("Exact 34/38 registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
    {
        return false;
    }

    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    const bool bPaletteDirty = Palette->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("One canonical read-only registry"),
        Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) {AddError(Error);}
        return false;
    }

    const TCHAR* LevelPaths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };

    ARMCLEAN064X15::FReferenceCount InstanceAccepted, InstanceKeys, DefinitionAccepted, DefinitionKeys;
    int32 OverrideReceptacleCount = 0, OverrideLockCount = 0;
    int32 ReceptacleInstances = 0, LevelWorldObjects = 0, InitialItems = 0;
    int32 DefinitionReceptacleCount = 0, DefinitionLockCount = 0;
    int32 ExplicitDefaultKeyIds = 0, ExplicitOverrideKeyIds = 0;
    TSet<FName> PlacedDefinitionIds;
    for (const TCHAR* Path : LevelPaths)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(LoadObject<UGridLevelAsset>(nullptr, Path));
        if (!TestNotNull(TEXT("Actual level exists"), Level.Get())) {return false;}
        const bool bLevelDirty = Level->GetOutermost()->IsDirty();

        for (const FGridWorldObjectInstance& Object : Level->WorldObjectInstances)
        {
            ++LevelWorldObjects;
            if (!Object.WorldObjectDefinitionId.IsNone())
                {PlacedDefinitionIds.Add(Object.WorldObjectDefinitionId);}
            if (Object.Type == EGridLevelObjectType::Receptacle)
                {++ReceptacleInstances;}
            InitialItems += Object.InstanceConfig.ReceptacleInitialContent.Num();

            const FGridWorldObjectInteractionOverrides& O = Object.InstanceConfig.InteractionOverrides;
            if (O.bOverrideReceptacleRules)
            {
                ++OverrideReceptacleCount;
                for (const FGridReceptacleAcceptedItemConfig& Rule : O.ReceptacleRules.AcceptedItems)
                    {InstanceAccepted.Visit(Rule.ItemId, Registry.Get());}
            }
            if (O.bOverrideAcceptedKeys)
            {
                ++OverrideLockCount;
                ExplicitOverrideKeyIds += O.AcceptedKeys.AcceptedKeyIds.Num();
            }
        }
        if (!TestEqual(TEXT("Read-only native level package"),
            Level->GetOutermost()->IsDirty(), bLevelDirty))
            {return false;}
    }
    if (!TestEqual(TEXT("Four real levels retain 18 initial receptacle items"),
        InitialItems, 18)) {return false;}

    // Scan the actual class assets instead of assuming all definition assets
    // are present in the direct-item palette. Never save or normalize them.
    FAssetRegistryModule& Module =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype")));
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    TArray<FAssetData> Assets;
    Module.Get().GetAssets(Filter, Assets);

    int32 DefinitionAssets = 0;
    TSet<FName> InspectedDefinitionIds;
    for (const FAssetData& AssetData : Assets)
    {
        UGridWorldObjectDefinitionAsset* Definition =
            Cast<UGridWorldObjectDefinitionAsset>(AssetData.GetAsset());
        if (!TestNotNull(TEXT("World definition asset can be loaded"), Definition))
            {return false;}
        ++DefinitionAssets;
        const bool bWasDirty = Definition->GetOutermost()->IsDirty();
        if (!Definition->DefinitionId.IsNone())
            {InspectedDefinitionIds.Add(Definition->DefinitionId);}
        if (Definition->SupportedType == EGridLevelObjectType::Receptacle)
            {++DefinitionReceptacleCount;}
        if (Definition->DefaultBehavior.Receptacle.bAcceptAnyItem == false ||
            !Definition->DefaultBehavior.Receptacle.AcceptedItems.IsEmpty())
        {
            for (const FGridReceptacleAcceptedItemConfig& Rule :
                Definition->DefaultBehavior.Receptacle.AcceptedItems)
            {
                DefinitionAccepted.Visit(Rule.ItemId, Registry.Get());
            }
        }

        // Keys are a distinct gameplay acceptance surface, not a receptacle
        // decoration. Record explicit IDs only.
        if (!Definition->DefaultBehavior.Lock.AcceptedKeyIds.IsEmpty())
            {++DefinitionLockCount;}
        ExplicitDefaultKeyIds += Definition->DefaultBehavior.Lock.AcceptedKeyIds.Num();

        if (!TestEqual(TEXT("Read-only definition package"),
            Definition->GetOutermost()->IsDirty(), bWasDirty))
            {return false;}
    }

    int32 UncoveredPlacedDefinitions = 0;
    for (FName Id : PlacedDefinitionIds)
    {
        if (!InspectedDefinitionIds.Contains(Id))
            {++UncoveredPlacedDefinitions;}
    }

    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X.15] real levels=4 worldObjects=%d receptacles=%d "
        "initialContent=%d overriddenReceptacles=%d overriddenLocks=%d "
        "nativeWorldDefinitions=%d definitionReceptacles=%d keyDefinitions=%d "
        "uncoveredPlacedDefinitionIds=%d"),
        LevelWorldObjects, ReceptacleInstances, InitialItems,
        OverrideReceptacleCount, OverrideLockCount, DefinitionAssets,
        DefinitionReceptacleCount, DefinitionLockCount,
        UncoveredPlacedDefinitions));
    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X.15] defaultAccepted{%s} instanceAccepted{%s} "
        "defaultKeyPointers{%s} instanceKeyPointers{%s} "
        "explicitDefaultKeyIds=%d explicitInstanceKeyIds=%d"),
        *ARMCLEAN064X15::Summary(DefinitionAccepted),
        *ARMCLEAN064X15::Summary(InstanceAccepted),
        *ARMCLEAN064X15::Summary(DefinitionKeys),
        *ARMCLEAN064X15::Summary(InstanceKeys),
        ExplicitDefaultKeyIds, ExplicitOverrideKeyIds));

    if (UncoveredPlacedDefinitions > 0)
    {
        AddWarning(FString::Printf(TEXT(
            "Some placed definitions are outside scanned native definition assets: %d. "
            "Review this coverage before any acceptance-rule rewrite."),
            UncoveredPlacedDefinitions));
    }
    if (DefinitionAccepted.Unrecognized + InstanceAccepted.Unrecognized +
        DefinitionKeys.Unrecognized + InstanceKeys.Unrecognized > 0)
    {
        AddWarning(TEXT("Some authored acceptance sources do not match the canonical ItemSystem. "
            "Do not perform automatic pointer-to-ID migration before identifying each source."));
    }

    return TestEqual(TEXT("Two explicit shared rules"), DefinitionAccepted.CanonicalKnown, 2) &&
        TestEqual(TEXT("No unresolved shared rules"), DefinitionAccepted.Unrecognized + DefinitionAccepted.Missing, 0) &&
        TestEqual(TEXT("No active instance rules"), InstanceAccepted.Total, 0) &&
        TestTrue(TEXT("Native world definition assets inspected"), DefinitionAssets > 0) &&
        TestEqual(TEXT("Registry package not modified"), Root->GetOutermost()->IsDirty(), bRootDirty) &&
        TestEqual(TEXT("Palette package not modified"), Palette->GetOutermost()->IsDirty(), bPaletteDirty);
}
#endif
