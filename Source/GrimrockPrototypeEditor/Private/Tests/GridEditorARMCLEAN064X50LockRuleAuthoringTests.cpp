#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorCanonicalLockRuleAuthoring.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X50LockRuleAuthoring,
    "Grimrock.RPG.ARM.CLEAN06_4X50.PointerlessLockOverrideAuthoring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X50LockRuleAuthoring::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real ItemSystem root"), Root.Get())) return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Canonical item registry configured"),
        Registry->RegisterItemSystem(Root.Get(), Errors))) return false;

    const FName Copper = TEXT("Key_Copper");
    const FName Iron = TEXT("Key_Iron");
    FGridLockBehaviorParams Shared;
    Shared.bStartsUnlocked = false;
    Shared.bConsumeKeyOnUnlock = true;
    Shared.AcceptedKeyIds = {TEXT("Key_Scripted"), Copper, Iron};
    FGridLockAcceptedKeyInstanceRuleValues Out;
    Out.AcceptedKeyIds.Add(TEXT("X50_KeepOnFailure"));
    if (!TestFalse(TEXT("Missing registry cannot author or modify override"),
        FGridEditorCanonicalLockRuleAuthoring::TryCopyDefaults(nullptr, Shared, Out)) ||
        !TestEqual(TEXT("Rejected draft keeps previous ID"), Out.AcceptedKeyIds[0], FName(TEXT("X50_KeepOnFailure"))) ||
        !TestTrue(TEXT("Explicit IDs copy atomically"),
            FGridEditorCanonicalLockRuleAuthoring::TryCopyDefaults(Registry.Get(), Shared, Out)) ||
        !TestTrue(TEXT("Explicit IDs including script IDs and order are preserved"), Out.AcceptedKeyIds == Shared.AcceptedKeyIds))
        return false;

    UGridWorldObjectDefinitionAsset* Definition =
        NewObject<UGridWorldObjectDefinitionAsset>();
    Definition->DefaultBehavior.Lock = Shared;
    FGridWorldObjectInstance Placed;
    Placed.InstanceConfig.InteractionOverrides.bOverrideAcceptedKeys = true;
    Placed.InstanceConfig.InteractionOverrides.AcceptedKeys = Out;
    const FGridObjectBehaviorParams Effective =
        GridObjectInstanceBehavior::Resolve(Placed, Definition);
    if (!TestEqual(TEXT("Runtime sees the three original accepted key IDs"),
            Effective.Lock.AcceptedKeyIds.Num(), 3) ||
        !TestTrue(TEXT("Lock does not gain an unrelated accepted key"),
            !Effective.Lock.AcceptedKeyIds.Contains(TEXT("Key_Gold"))) ||
        !TestEqual(TEXT("Other authored lock fields survive override"),
            Effective.Lock.bConsumeKeyOnUnlock, Shared.bConsumeKeyOnUnlock))
    {
        return false;
    }

    // X.53: clearing the final ID is a deliberate empty instance override,
    // not permission to rediscover the shared historical key pointers.
    FGridWorldObjectInstance EmptyKeys = Placed;
    EmptyKeys.InstanceConfig.InteractionOverrides.AcceptedKeys.AcceptedKeyIds.Reset();
    const FGridObjectBehaviorParams NoKeys =
        GridObjectInstanceBehavior::Resolve(EmptyKeys, Definition);
    if (!TestEqual(TEXT("Empty override does not inherit default accepted ItemIds"),
            NoKeys.Lock.AcceptedKeyIds.Num(), 0) ||
        !TestEqual(TEXT("Consume-key policy survives empty override"),
            NoKeys.Lock.bConsumeKeyOnUnlock, Shared.bConsumeKeyOnUnlock))
        return false;

    // Check the two ACTUAL shared assets; do not mutate either one.
    const TCHAR* Paths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
             "DA_Lock_CopperWall.DA_Lock_CopperWall"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
             "DA_Lock_IronWall.DA_Lock_IronWall")
    };
    const FName ExpectedIds[] = {TEXT("Key_Copper"), TEXT("Key_Iron")};
    for (int32 Index = 0; Index < 2; ++Index)
    {
        TStrongObjectPtr<UGridWorldObjectDefinitionAsset> Real(
            LoadObject<UGridWorldObjectDefinitionAsset>(nullptr, Paths[Index]));
        if (!TestNotNull(TEXT("Real original lock definition"), Real.Get())) return false;
        const bool bDirty = Real->GetOutermost()->IsDirty();
        const FGridLockBehaviorParams& RealLock = Real->DefaultBehavior.Lock;
        FGridLockAcceptedKeyInstanceRuleValues Projection;
        if (!TestTrue(TEXT("Real definition projects exact canonical-only key set"),
            FGridEditorCanonicalLockRuleAuthoring::TryCopyDefaults(
                Registry.Get(), RealLock, Projection)) ||
            !TestEqual(TEXT("Real projected accepted key count"), Projection.AcceptedKeyIds.Num(), 1) ||
            !TestEqual(TEXT("Real projected key retains ID"), Projection.AcceptedKeyIds[0], ExpectedIds[Index]) ||
            !TestEqual(TEXT("Original package not dirty"), Real->GetOutermost()->IsDirty(), bDirty))
        {
            return false;
        }
    }

    AddInfo(TEXT("[ARM-CLEAN06.4X50] new lock overrides are canonical-only; "
        "real Copper/Iron lock defaults projected without modifying either package; "
        "script IDs and empty overrides preserved; missing registry fails atomically."));
    return TestEqual(TEXT("ItemSystem package unchanged"),
        Root->GetOutermost()->IsDirty(), bRootDirty);
}
#endif
