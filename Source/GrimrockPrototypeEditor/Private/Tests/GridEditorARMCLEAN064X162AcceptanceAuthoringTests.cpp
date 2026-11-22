#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorReceptacleRuleAuthoring.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X162Authoring,
    "Grimrock.RPG.ARM.CLEAN06_4X162.CanonicalAcceptanceAuthoring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X162Authoring::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real canonical registry asset"), Root.Get())) {return false;}
    const bool WasDirty = Root->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Single validated registry"), Registry->RegisterItemSystem(Root.Get(), Errors)))
        {return false;}

    TestNull(TEXT("Removed historical acceptance pointer schema"), FGridReceptacleAcceptedItemConfig::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));

    const FName Arrow = TEXT("Item_Arrows_01");
    const FName Sword = TEXT("Weapon_StraightSword_01");
    FGridReceptacleAcceptedItemConfig Rule;
    if (!TestFalse(TEXT("No registry cannot rewrite rule"),
        FGridEditorReceptacleRuleAuthoring::TrySelectCanonicalId(nullptr, Arrow, Rule)) ||
        !TestTrue(TEXT("Rejected draft remains empty"), Rule.ItemId.IsNone()) ||
        !TestTrue(TEXT("Known ItemId accepted"),
            FGridEditorReceptacleRuleAuthoring::TrySelectCanonicalId(Registry.Get(), Arrow, Rule)) ||
        !TestEqual(TEXT("Arrow identity persisted"), Rule.ItemId, Arrow) ||
        !TestTrue(TEXT("Switch to canonical sword"),
            FGridEditorReceptacleRuleAuthoring::TrySelectCanonicalId(Registry.Get(), Sword, Rule)) ||
        !TestEqual(TEXT("No stale identity"), Rule.ItemId, Sword) ||
        !TestFalse(TEXT("Unknown ID rejected atomically"),
            FGridEditorReceptacleRuleAuthoring::TrySelectCanonicalId(Registry.Get(), TEXT("X162_Unknown"), Rule)) ||
        !TestEqual(TEXT("Rejected edit keeps prior ID"), Rule.ItemId, Sword) ||
        !TestTrue(TEXT("Clear draft"),
            FGridEditorReceptacleRuleAuthoring::TrySelectCanonicalId(Registry.Get(), NAME_None, Rule)) ||
        !TestTrue(TEXT("Cleared draft has no identity"), Rule.ItemId.IsNone()))
        return false;

    FGridReceptacleAcceptedItemConfig DefaultArrow;
    DefaultArrow.ItemId = Arrow;
    FGridReceptacleAcceptedItemConfig DefaultSword;
    DefaultSword.ItemId = Sword;
    TArray<FGridReceptacleAcceptedItemConfig> Defaults = {DefaultArrow, DefaultSword, DefaultArrow};
    TArray<FGridReceptacleAcceptedItemConfig> Copies;
    if (!TestFalse(TEXT("Missing registry refuses defaults"),
        FGridEditorReceptacleRuleAuthoring::TryCopyDefaults(nullptr, Defaults, Copies)) ||
        !TestTrue(TEXT("No partial copy on failure"), Copies.IsEmpty()) ||
        !TestTrue(TEXT("Ordered canonical copy succeeds"),
            FGridEditorReceptacleRuleAuthoring::TryCopyDefaults(Registry.Get(), Defaults, Copies)) ||
        !TestEqual(TEXT("Copies preserve count"), Copies.Num(), 3) ||
        !TestEqual(TEXT("First identity preserved"), Copies[0].ItemId, Arrow) ||
        !TestEqual(TEXT("Second identity preserved"), Copies[1].ItemId, Sword) ||
        !TestEqual(TEXT("Duplicate identity preserved"), Copies[2].ItemId, Arrow))
        return false;

    UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>();
    Definition->DefaultBehavior.Receptacle.bAcceptAnyItem = false;
    Definition->DefaultBehavior.Receptacle.AcceptedItems = Defaults;
    FGridWorldObjectInstance Instance;
    Instance.InstanceConfig.InteractionOverrides.bOverrideReceptacleRules = true;
    Instance.InstanceConfig.InteractionOverrides.ReceptacleRules.AcceptedItems = Copies;
    Instance.InstanceConfig.InteractionOverrides.ReceptacleRules.bAcceptAnyItem = false;
    Instance.InstanceConfig.InteractionOverrides.ReceptacleRules.MaxContainedItems = 4;
    const FGridObjectBehaviorParams Effective = GridObjectInstanceBehavior::Resolve(Instance, Definition);
    if (!TestEqual(TEXT("Override rule count"), Effective.Receptacle.AcceptedItems.Num(), 3) ||
        !TestEqual(TEXT("Override gains canonical identity"), Effective.Receptacle.AcceptedItems[0].ItemId,
            Arrow) ||
        !TestEqual(TEXT("Override capacity remains 4"), Effective.Receptacle.MaxContainedItems, 4))
        {return false;}

    FGridReceptacleAcceptedItemConfig Invalid;
    Invalid.ItemId = TEXT("X162_Unknown");
    Defaults.Add(Invalid);
    if (!TestFalse(TEXT("Unknown default rejects entire copy"),
            FGridEditorReceptacleRuleAuthoring::TryCopyDefaults(Registry.Get(), Defaults, Copies)) ||
        !TestTrue(TEXT("No partial copy of unknown default"), Copies.IsEmpty())) return false;
    Defaults.Last().ItemId = NAME_None;
    return TestFalse(TEXT("Empty default rejects entire copy"),
            FGridEditorReceptacleRuleAuthoring::TryCopyDefaults(Registry.Get(), Defaults, Copies)) &&
        TestTrue(TEXT("No partial new rule set after failure"), Copies.IsEmpty()) &&
        TestEqual(TEXT("No mutation of native catalogue"), Root->GetOutermost()->IsDirty(), WasDirty);
}
#endif
