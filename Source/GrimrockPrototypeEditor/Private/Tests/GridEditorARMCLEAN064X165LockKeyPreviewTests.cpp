#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Modules/ModuleManager.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorLockKeyMigrationPreview.h"
#include "Runtime/GridItemDefinitionAsset.h"
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

TArray<FName> CurrentRuntimeUnion(
    const TArray<TObjectPtr<UGridItemDefinitionAsset>>& Pointers,
    const TArray<FName>& ExplicitIds)
{
    // Mirror AGridWallLockActor::InitializeRuntimeWorldObject.
    TArray<FName> Accepted;
    for (const TObjectPtr<UGridItemDefinitionAsset>& Ptr : Pointers)
    {
        if (IsValid(Ptr.Get()) && !Ptr->ItemDefinitionId.IsNone())
            Accepted.AddUnique(Ptr->ItemDefinitionId);
    }
    for (FName KeyId : ExplicitIds)
    {
        if (!KeyId.IsNone()) Accepted.AddUnique(KeyId);
    }
    return Accepted;
}

bool VerifyPreview(FAutomationTestBase& Test, const UGridItemSubsystem* Registry,
    const TArray<TObjectPtr<UGridItemDefinitionAsset>>& Pointers,
    const TArray<FName>& ExplicitIds,
    FGridLockKeyPreviewStats& Totals, int32& SafeLists)
{
    const TArray<TObjectPtr<UGridItemDefinitionAsset>> OriginalPointers = Pointers;
    const TArray<FName> OriginalIds = ExplicitIds;
    const TArray<FName> BeforeUnion = CurrentRuntimeUnion(Pointers, ExplicitIds);
    TArray<FName> Proposed;
    FGridLockKeyPreviewStats Stats;
    const bool bSuccess = FGridLockKeyMigrationPreview::TryPreview(
        Registry, Pointers, ExplicitIds, Proposed, Stats);
    Totals.Accumulate(Stats);
    SafeLists += bSuccess ? 1 : 0;
    if (!Test.TestEqual(TEXT("Pointer count unchanged"), Pointers.Num(), OriginalPointers.Num()) ||
        !Test.TestEqual(TEXT("Explicit key count unchanged"),
            ExplicitIds.Num(), OriginalIds.Num()) ||
        !Test.TestEqual(TEXT("Blocked lock has no partial proposed IDs"),
            bSuccess ? 1 : Proposed.Num(), bSuccess ? 1 : 0))
        return false;

    for (int32 i=0; i < Pointers.Num(); ++i)
    {
        if (!Test.TestTrue(TEXT("Old key pointer was never rewritten"),
            Pointers[i] == OriginalPointers[i])) return false;
    }
    for (int32 i=0; i < ExplicitIds.Num(); ++i)
    {
        if (!Test.TestTrue(TEXT("User-authored explicit ID not edited"),
            ExplicitIds[i] == OriginalIds[i])) return false;
        if (bSuccess && !Test.TestTrue(TEXT("Explicit prefix identical"),
            Proposed[i] == OriginalIds[i])) return false;
    }

    if (bSuccess)
    {
        // Changing only the duplicated read path, not the accepted set.
        const TArray<FName> AfterUnion = CurrentRuntimeUnion(Pointers, Proposed);
        if (!Test.TestEqual(TEXT("Accepted key set cardinality conserved"),
            AfterUnion.Num(), BeforeUnion.Num())) return false;
        for (FName Id : BeforeUnion)
        {
            if (!Test.TestTrue(TEXT("Every old accepted key remains accepted"),
                AfterUnion.Contains(Id))) return false;
        }
        for (FName Id : AfterUnion)
        {
            if (!Test.TestTrue(TEXT("No new key is made acceptable"),
                BeforeUnion.Contains(Id))) return false;
        }
    }
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X165KeyAtomicPreview,
    "Grimrock.RPG.ARM.CLEAN06_4X165.AtomicLockKeyUnionPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X165KeyAtomicPreview::RunTest(const FString&)
{
    ARMCLEAN064X165::FFixture F;
    if (!TestTrue(TEXT("Registered canonical ItemSystem"), F.bReady))
        return false;
    const bool WasDirty = F.ItemSystem->GetOutermost()->IsDirty();
    UGridItemDefinitionAsset* A = NewObject<UGridItemDefinitionAsset>();
    A->ItemDefinitionId = TEXT("Item_Arrows_01");
    UGridItemDefinitionAsset* B = NewObject<UGridItemDefinitionAsset>();
    B->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
    UGridItemDefinitionAsset* Unknown = NewObject<UGridItemDefinitionAsset>();
    Unknown->ItemDefinitionId = TEXT("X165_UnmappedQuestKey");

    // A lock legitimately may accept explicit script-defined key IDs not
    // included in the 34-item catalogue. Do not erase/rename such IDs.
    TArray<TObjectPtr<UGridItemDefinitionAsset>> Pointers = {A, A, B};
    TArray<FName> Explicit = {
        TEXT("Key_Scripted_Quest"), A->ItemDefinitionId,
        TEXT("Key_Scripted_Quest")
    };
    TArray<FName> Proposed;
    FGridLockKeyPreviewStats Stats;
    if (!TestTrue(TEXT("Safe union proposal"),
        FGridLockKeyMigrationPreview::TryPreview(
            F.Registry.Get(), Pointers, Explicit, Proposed, Stats)) ||
        !TestEqual(TEXT("One additional physical key ID proposed"),
            Stats.NewlyProposed, 1) ||
        !TestEqual(TEXT("Two pointer identities already covered"), Stats.AlreadyCovered, 2) ||
        !TestEqual(TEXT("Explicit IDs remain exact prefix"), Proposed.Num(), 4) ||
        !TestEqual(TEXT("Appended ID matches B"),
            Proposed[3], B->ItemDefinitionId) ||
        !TestTrue(TEXT("Explicit unknown scripted ID remains unchanged"),
            Proposed[0] == Explicit[0] && Proposed[2] == Explicit[2]))
        return false;
    const TArray<FName> BeforeUnion =
        ARMCLEAN064X165::CurrentRuntimeUnion(Pointers, Explicit);
    const TArray<FName> AfterUnion =
        ARMCLEAN064X165::CurrentRuntimeUnion(Pointers, Proposed);
    if (!TestEqual(TEXT("Runtime membership unchanged"), BeforeUnion.Num(), AfterUnion.Num()))
        return false;
    for (FName Id : BeforeUnion)
    {
        if (!TestTrue(TEXT("All original allowed keys remain allowed"),
            AfterUnion.Contains(Id))) return false;
    }

    Pointers.Add(Unknown);
    if (!TestFalse(TEXT("Unknown pointer blocks entire list"),
        FGridLockKeyMigrationPreview::TryPreview(
            F.Registry.Get(), Pointers, Explicit, Proposed, Stats)) ||
        !TestEqual(TEXT("One unknown historical pointer recorded"),
            Stats.UnknownPointer, 1) ||
        !TestTrue(TEXT("No partial IDs on unknown"), Proposed.IsEmpty()))
        return false;
    Pointers.Pop();
    Pointers.Add(nullptr);
    if (!TestFalse(TEXT("Null pointer prevents silent source removal"),
        FGridLockKeyMigrationPreview::TryPreview(
            F.Registry.Get(), Pointers, Explicit, Proposed, Stats)) ||
        !TestEqual(TEXT("Null pointer recorded"), Stats.MissingPointer, 1) ||
        !TestTrue(TEXT("Blocked proposal cleared"), Proposed.IsEmpty()))
        return false;

    return TestFalse(TEXT("Unavailable registry blocks conversion"),
            FGridLockKeyMigrationPreview::TryPreview(
                nullptr, Pointers, Explicit, Proposed, Stats)) &&
        TestTrue(TEXT("Missing registry flagged"), Stats.bRegistryUnavailable) &&
        TestEqual(TEXT("Native canonical catalogue not modified"),
            F.ItemSystem->GetOutermost()->IsDirty(), WasDirty);
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
    FGridLockKeyPreviewStats InstanceStats, DefinitionStats;
    int32 InstanceLists = 0, DefaultLists = 0;
    int32 SafeInstances = 0, SafeDefaults = 0;
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
            if (!ARMCLEAN064X165::VerifyPreview(*this, F.Registry.Get(),
                Overrides.AcceptedKeys.AcceptedKeyItems,
                Overrides.AcceptedKeys.AcceptedKeyIds,
                InstanceStats, SafeInstances)) return false;
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
        if (!Lock.AcceptedKeyItems.IsEmpty() || !Lock.AcceptedKeyIds.IsEmpty())
        {
            ++DefaultLists;
            if (!ARMCLEAN064X165::VerifyPreview(*this, F.Registry.Get(),
                Lock.AcceptedKeyItems, Lock.AcceptedKeyIds,
                DefinitionStats, SafeDefaults)) return false;
        }
        if (!TestEqual(TEXT("Native definition read-only"),
            Definition->GetOutermost()->IsDirty(), WasDirty)) return false;
    }

    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X.16.5] definitionAssets=%d defaultKeyLists=%d safeDefault=%d {%s} "
        "instanceKeyLists=%d safeInstance=%d {%s}"),
        InspectedDefinitions, DefaultLists, SafeDefaults,
        *DefinitionStats.Describe(), InstanceLists, SafeInstances,
        *InstanceStats.Describe()));
    if (DefinitionStats.HasBlockers() || InstanceStats.HasBlockers())
    {
        AddWarning(TEXT("At least one real lock key list requires manual source "
            "qualification. Do not convert any native key acceptance assets."));
    }
    return TestTrue(TEXT("Native definitions were scanned"),
        InspectedDefinitions > 0) &&
        TestEqual(TEXT("Canonical ItemSystem remains read-only"),
            F.ItemSystem->GetOutermost()->IsDirty(), SystemWasDirty);
}
#endif
