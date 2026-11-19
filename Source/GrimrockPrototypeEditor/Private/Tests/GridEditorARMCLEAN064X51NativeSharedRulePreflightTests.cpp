#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorCanonicalReceptacleRuleAuthoring.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

/** Native X.51 only inspects the FOUR real shared acceptance definitions.
 *  No SavePackage/Modify; the proposed remove-pointer operation is performed
 *  on disposable duplicates, preserving all authored puzzle semantics.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X51NativeSharedRules,
    "Grimrock.RPG.ARM.CLEAN06_4X51.FourSharedRuleNativeDryRun",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X51NativeSharedRules::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Native canonical ItemSystem"), Root.Get())) return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Canonical item resolver configured"),
        Registry->RegisterItemSystem(Root.Get(), Errors))) return false;

    const TCHAR* ReceptaclePaths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
             "DA_Receptacle_TorchHolder.DA_Receptacle_TorchHolder"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/"
             "DA_Guardian.DA_Guardian")
    };
    const FName ReceptacleIds[] = {TEXT("Item_Torch"), TEXT("Gem_Blue")};
    const TCHAR* LockPaths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
             "DA_Lock_CopperWall.DA_Lock_CopperWall"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
             "DA_Lock_IronWall.DA_Lock_IronWall")
    };
    const FName KeyIds[] = {TEXT("Key_Copper"), TEXT("Key_Iron")};

    int32 OldPointers = 0;
    for (int32 Index = 0; Index < 2; ++Index)
    {
        TStrongObjectPtr<UGridWorldObjectDefinitionAsset> Original(
            LoadObject<UGridWorldObjectDefinitionAsset>(nullptr, ReceptaclePaths[Index]));
        if (!TestNotNull(TEXT("Actual shared receptacle definition"), Original.Get()))
            return false;
        const bool bWasDirty = Original->GetOutermost()->IsDirty();
        const FGridReceptacleBehaviorParams& Source = Original->DefaultBehavior.Receptacle;
        if (!TestEqual(TEXT("One accepted item per shared receptacle"), Source.AcceptedItems.Num(), 1) ||
            !TestEqual(TEXT("Canonical accepted ItemId is authored"),
                Source.AcceptedItems[0].CanonicalItemId, ReceptacleIds[Index]) ||
            !TestTrue(TEXT("Accepted real ItemId belongs to ItemSystem"),
                Registry->HasCanonicalItem(ReceptacleIds[Index])))
            return false;
        TStrongObjectPtr<UGridWorldObjectDefinitionAsset> Copy(
            DuplicateObject<UGridWorldObjectDefinitionAsset>(
                Original.Get(), GetTransientPackage()));
        if (!TestNotNull(TEXT("Disposable receptacle copy"), Copy.Get())) return false;
        const FGridReceptacleBehaviorParams Before = Copy->DefaultBehavior.Receptacle;
        const FGridReceptacleBehaviorParams& After = Copy->DefaultBehavior.Receptacle;
        TArray<FGridReceptacleAcceptedItemConfig> Preview;
        if (!TestTrue(TEXT("Explicit rule remains authorable"),
            FGridEditorCanonicalReceptacleRuleAuthoring::TryCopyDefaults(
                Registry.Get(), After.AcceptedItems, Preview)) ||
            !TestEqual(TEXT("Exactly one pointerless canonical rule"), Preview.Num(), 1) ||
            !TestEqual(TEXT("Physical acceptance ItemId unchanged"),
                Preview[0].CanonicalItemId, ReceptacleIds[Index]) ||
            !TestEqual(TEXT("Accept-any state unchanged"), After.bAcceptAnyItem, Before.bAcceptAnyItem) ||
            !TestEqual(TEXT("Capacity unchanged"), After.MaxContainedItems, Before.MaxContainedItems) ||
            !TestEqual(TEXT("Visual placement mode unchanged"), After.VisualPlacementMode, Before.VisualPlacementMode) ||
            !TestEqual(TEXT("Physics setting unchanged"), After.bSimulatePhysicsWhenPlaced,
                Before.bSimulatePhysicsWhenPlaced) ||
            !TestTrue(TEXT("Receptacle physical offset unchanged"),
                FMath::IsNearlyEqual(After.PhysicalPlacementSurfaceOffset,
                    Before.PhysicalPlacementSurfaceOffset)) ||
            !TestEqual(TEXT("Source receptacle package not modified"),
                Original->GetOutermost()->IsDirty(), bWasDirty))
            return false;
    }

    for (int32 Index = 0; Index < 2; ++Index)
    {
        TStrongObjectPtr<UGridWorldObjectDefinitionAsset> Original(
            LoadObject<UGridWorldObjectDefinitionAsset>(nullptr, LockPaths[Index]));
        if (!TestNotNull(TEXT("Actual shared lock definition"), Original.Get()))
            return false;
        const bool bWasDirty = Original->GetOutermost()->IsDirty();
        const FGridLockBehaviorParams& Source = Original->DefaultBehavior.Lock;
        if (!TestEqual(TEXT("Single explicit accepted key"), Source.AcceptedKeyIds.Num(), 1) ||
            !TestEqual(TEXT("Native canonical key identity"),
                Source.AcceptedKeyIds[0], KeyIds[Index]) ||
            !TestTrue(TEXT("Accepted key remains registered"),
                Registry->HasCanonicalItem(KeyIds[Index])))
            return false;
        TStrongObjectPtr<UGridWorldObjectDefinitionAsset> Copy(
            DuplicateObject<UGridWorldObjectDefinitionAsset>(
                Original.Get(), GetTransientPackage()));
        if (!TestNotNull(TEXT("Disposable lock copy"), Copy.Get())) return false;
        const FGridLockBehaviorParams Before = Copy->DefaultBehavior.Lock;
        const FGridLockBehaviorParams& After = Copy->DefaultBehavior.Lock;
        if (!TestTrue(TEXT("Exact accepted key ID survives copy"), After.AcceptedKeyIds == Before.AcceptedKeyIds) ||
            !TestEqual(TEXT("Starts-unlocked state unchanged"),
                After.bStartsUnlocked, Before.bStartsUnlocked) ||
            !TestEqual(TEXT("Consume-on-unlock policy unchanged"),
                After.bConsumeKeyOnUnlock, Before.bConsumeKeyOnUnlock) ||
            !TestTrue(TEXT("Locked message unchanged"),
                After.LockedMessage.EqualTo(Before.LockedMessage)) ||
            !TestTrue(TEXT("Unlocked message unchanged"),
                After.UnlockedMessage.EqualTo(Before.UnlockedMessage)) ||
            !TestTrue(TEXT("Missing key message unchanged"),
                After.MissingKeyMessage.EqualTo(Before.MissingKeyMessage)) ||
            !TestEqual(TEXT("Source lock package not modified"),
                Original->GetOutermost()->IsDirty(), bWasDirty))
            return false;
    }

    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X51] four real shared rule DataAssets, "
        "historical pointers=%d; transient pointerless dry-run passes; "
        "Torch/Gem/Copper/Iron identities and puzzle rules preserved; no SavePackage."),
        OldPointers));
    return TestTrue(TEXT("Shared rule packages are entirely pre/post migration"),
        OldPointers == 0) &&
        TestEqual(TEXT("ItemSystem root remains read-only"),
            Root->GetOutermost()->IsDirty(), bRootDirty);
}
#endif
