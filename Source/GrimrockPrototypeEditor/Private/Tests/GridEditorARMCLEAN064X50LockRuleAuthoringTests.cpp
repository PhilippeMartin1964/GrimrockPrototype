#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorCanonicalLockRuleAuthoring.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
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

    UGridItemDefinitionAsset* Copper = NewObject<UGridItemDefinitionAsset>();
    Copper->ItemDefinitionId = TEXT("Key_Copper");
    UGridItemDefinitionAsset* Iron = NewObject<UGridItemDefinitionAsset>();
    Iron->ItemDefinitionId = TEXT("Key_Iron");
    UGridItemDefinitionAsset* Unknown = NewObject<UGridItemDefinitionAsset>();
    Unknown->ItemDefinitionId = TEXT("X50_HistoricUnregisteredKey");

    FGridLockBehaviorParams Shared;
    Shared.bStartsUnlocked = false;
    Shared.bConsumeKeyOnUnlock = true;
    Shared.AcceptedKeyItems = {Copper, Iron};
    Shared.AcceptedKeyIds = {TEXT("Key_Scripted"), Copper->ItemDefinitionId};

    FGridLockAcceptedKeyInstanceRuleValues Out;
    Out.AcceptedKeyItems.Add(Copper);
    Out.AcceptedKeyIds.Add(TEXT("X50_KeepOnFailure"));
    if (!TestFalse(TEXT("Missing registry cannot author or modify override"),
        FGridEditorCanonicalLockRuleAuthoring::TryCopyDefaults(nullptr, Shared, Out)) ||
        !TestEqual(TEXT("Rejected draft keeps its previous ItemId"),
            Out.AcceptedKeyIds[0], FName(TEXT("X50_KeepOnFailure"))) ||
        !TestEqual(TEXT("Rejected draft keeps its historical source"),
            Out.AcceptedKeyItems[0].Get(), Copper))
        return false;

    if (!TestTrue(TEXT("Valid mixed explicit/old lock list converts atomically"),
        FGridEditorCanonicalLockRuleAuthoring::TryCopyDefaults(Registry.Get(), Shared, Out)) ||
        !TestEqual(TEXT("Existing explicit IDs and converted sources kept"), Out.AcceptedKeyIds.Num(), 3) ||
        !TestEqual(TEXT("Script-owned non-registry key preserved"), Out.AcceptedKeyIds[0],
            FName(TEXT("Key_Scripted"))) ||
        !TestEqual(TEXT("Registered copper kept"), Out.AcceptedKeyIds[1], Copper->ItemDefinitionId) ||
        !TestEqual(TEXT("Registered iron appended"), Out.AcceptedKeyIds[2], Iron->ItemDefinitionId) ||
        !TestEqual(TEXT("No obsolete key source stored in new override"), Out.AcceptedKeyItems.Num(), 0) ||
        !TestEqual(TEXT("Shared definition remains unmodified"), Shared.AcceptedKeyItems.Num(), 2))
    {
        return false;
    }

    UGridWorldObjectDefinitionAsset* Definition =
        NewObject<UGridWorldObjectDefinitionAsset>();
    Definition->DefaultBehavior.Lock = Shared;
    FGridWorldObjectInstance Placed;
    Placed.InstanceConfig.InteractionOverrides.bOverrideAcceptedKeys = true;
    Placed.InstanceConfig.InteractionOverrides.AcceptedKeys = Out;
    const FGridObjectBehaviorParams Effective =
        GridObjectInstanceBehavior::Resolve(Placed, Definition);
    if (!TestEqual(TEXT("Runtime receives canonical-only overrides"),
        Effective.Lock.AcceptedKeyItems.Num(), 0) ||
        !TestEqual(TEXT("Runtime sees the three original accepted key IDs"),
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
    if (!TestEqual(TEXT("Empty override does not inherit historical key assets"),
            NoKeys.Lock.AcceptedKeyItems.Num(), 0) ||
        !TestEqual(TEXT("Empty override does not inherit default accepted ItemIds"),
            NoKeys.Lock.AcceptedKeyIds.Num(), 0) ||
        !TestEqual(TEXT("Consume-key policy survives empty override"),
            NoKeys.Lock.bConsumeKeyOnUnlock, Shared.bConsumeKeyOnUnlock))
        return false;

    // Already-authored legacy-only instances stay readable and unchanged.
    FGridWorldObjectInstance LegacyKeys = Placed;
    LegacyKeys.InstanceConfig.InteractionOverrides.AcceptedKeys.AcceptedKeyIds.Reset();
    LegacyKeys.InstanceConfig.InteractionOverrides.AcceptedKeys.AcceptedKeyItems = {Copper};
    const FGridObjectBehaviorParams LegacyEffective =
        GridObjectInstanceBehavior::Resolve(LegacyKeys, Definition);
    if (!TestEqual(TEXT("Legacy-only instance retains one historical key"),
            LegacyEffective.Lock.AcceptedKeyItems.Num(), 1) ||
        !TestEqual(TEXT("Existing legacy key identity is not rewritten"),
            LegacyEffective.Lock.AcceptedKeyItems[0].Get(), Copper) ||
        !TestEqual(TEXT("Legacy-only instance does not fabricate ItemIds"),
            LegacyEffective.Lock.AcceptedKeyIds.Num(), 0))
        return false;

    Shared.AcceptedKeyItems.Add(Unknown);
    if (!TestFalse(TEXT("Unregistered historical source rejects entire conversion"),
        FGridEditorCanonicalLockRuleAuthoring::TryCopyDefaults(Registry.Get(), Shared, Out)) ||
        !TestEqual(TEXT("Failed projection keeps accepted key set"), Out.AcceptedKeyIds.Num(), 3) ||
        !TestEqual(TEXT("Failed projection cannot reintroduce old pointers"),
            Out.AcceptedKeyItems.Num(), 0))
        return false;
    Shared.AcceptedKeyItems.Pop();
    Shared.AcceptedKeyItems.Add(nullptr);
    if (!TestFalse(TEXT("Null historical source also rejects conversion"),
        FGridEditorCanonicalLockRuleAuthoring::TryCopyDefaults(Registry.Get(), Shared, Out)) ||
        !TestEqual(TEXT("Null source does not mutate prior accepted ID set"),
            Out.AcceptedKeyIds.Num(), 3))
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
            !TestEqual(TEXT("No old key pointers copied from shared definition"),
                Projection.AcceptedKeyItems.Num(), 0) ||
            !TestTrue(TEXT("Shared source intact or fully pointerless after native migration"),
                RealLock.AcceptedKeyItems.IsEmpty() ||
                (RealLock.AcceptedKeyItems.Num() == 1 &&
                 RealLock.AcceptedKeyItems[0] &&
                 RealLock.AcceptedKeyItems[0]->ItemDefinitionId == ExpectedIds[Index])) ||
            !TestEqual(TEXT("Original package not dirty"), Real->GetOutermost()->IsDirty(), bDirty))
        {
            return false;
        }
    }

    AddInfo(TEXT("[ARM-CLEAN06.4X50] new lock overrides are canonical-only; "
        "real Copper/Iron lock defaults projected without modifying either package; "
        "script IDs preserved and unknown/null old sources fail atomically."));
    return TestEqual(TEXT("ItemSystem package unchanged"),
        Root->GetOutermost()->IsDirty(), bRootDirty);
}
#endif
