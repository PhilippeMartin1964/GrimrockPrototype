#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorCanonicalReceptacleRuleAuthoring.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
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

    UGridItemDefinitionAsset* Arrow = NewObject<UGridItemDefinitionAsset>();
    Arrow->ItemDefinitionId = TEXT("Item_Arrows_01");
    UGridItemDefinitionAsset* Sword = NewObject<UGridItemDefinitionAsset>();
    Sword->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
    UGridItemDefinitionAsset* Unknown = NewObject<UGridItemDefinitionAsset>();
    Unknown->ItemDefinitionId = TEXT("X162_Historical_Unknown");
    FGridReceptacleAcceptedItemConfig Rule;
    if (!TestFalse(TEXT("No registry cannot rewrite rule"),
        FGridEditorCanonicalReceptacleRuleAuthoring::TryAssign(nullptr, Arrow, Rule)) ||
        !TestTrue(TEXT("Rejected rule was not modified"), Rule.ItemDefinition == nullptr &&
            Rule.CanonicalItemId.IsNone())) {return false;}
    if (!TestTrue(TEXT("Known ItemId written by picker"),
        FGridEditorCanonicalReceptacleRuleAuthoring::TryAssign(Registry.Get(), Arrow, Rule)) ||
        !TestEqual(TEXT("Arrow identity written"), Rule.CanonicalItemId, Arrow->ItemDefinitionId) ||
        !TestNull(TEXT("New registered accepted rule has no old pointer"), Rule.ItemDefinition.Get()))
        {return false;}
    if (!TestTrue(TEXT("New selection updates both fields"),
        FGridEditorCanonicalReceptacleRuleAuthoring::TryAssign(Registry.Get(), Sword, Rule)) ||
        !TestEqual(TEXT("No stale previous ItemId"), Rule.CanonicalItemId, Sword->ItemDefinitionId) ||
        !TestNull(TEXT("Switching registered rules never copies a legacy pointer"),
            Rule.ItemDefinition.Get()))
        {return false;}
    if (!TestTrue(TEXT("Unknown historic source stays unmigrated"),
        FGridEditorCanonicalReceptacleRuleAuthoring::TryAssign(Registry.Get(), Unknown, Rule)) ||
        !TestTrue(TEXT("No false canonical ItemId"), Rule.CanonicalItemId.IsNone() &&
            Rule.ItemDefinition == Unknown)) {return false;}
    if (!TestTrue(TEXT("Clearing picker resets both sources"),
        FGridEditorCanonicalReceptacleRuleAuthoring::TryAssign(Registry.Get(), nullptr, Rule)) ||
        !TestTrue(TEXT("No stale accepted identity survives"), Rule.CanonicalItemId.IsNone() &&
            Rule.ItemDefinition == nullptr)) {return false;}

    FGridReceptacleAcceptedItemConfig DefaultArrow;
    DefaultArrow.ItemDefinition = Arrow;
    FGridReceptacleAcceptedItemConfig DefaultUnknown;
    DefaultUnknown.ItemDefinition = Unknown;
    FGridReceptacleAcceptedItemConfig IdOnly;
    IdOnly.CanonicalItemId = Sword->ItemDefinitionId;
    TArray<FGridReceptacleAcceptedItemConfig> Defaults = {DefaultArrow, DefaultUnknown, IdOnly};
    TArray<FGridReceptacleAcceptedItemConfig> Copies;
    if (!TestFalse(TEXT("Missing registry never copies unvalidated rules"),
        FGridEditorCanonicalReceptacleRuleAuthoring::TryCopyDefaults(nullptr, Defaults, Copies)) ||
        !TestTrue(TEXT("No partial copy on failure"), Copies.IsEmpty())) {return false;}
    if (!TestTrue(TEXT("Ordered instance rule copy succeeds"),
        FGridEditorCanonicalReceptacleRuleAuthoring::TryCopyDefaults(
            Registry.Get(), Defaults, Copies)) ||
        !TestEqual(TEXT("All three rules kept"), Copies.Num(), 3) ||
        !TestEqual(TEXT("Known legacy default upgraded in copy only"),
            Copies[0].CanonicalItemId, Arrow->ItemDefinitionId) ||
        !TestNull(TEXT("Known shared default copied pointerlessly"), Copies[0].ItemDefinition.Get()) ||
        !TestTrue(TEXT("Unknown historical rule preserved"), Copies[1].CanonicalItemId.IsNone() &&
            Copies[1].ItemDefinition == Unknown) ||
        !TestTrue(TEXT("Canonical ID-only rule remains source independent"),
            Copies[2].CanonicalItemId == Sword->ItemDefinitionId && Copies[2].ItemDefinition == nullptr) ||
        !TestTrue(TEXT("Shared definition default unmodified"),
            Defaults[0].CanonicalItemId.IsNone() && Defaults[0].ItemDefinition == Arrow)) {return false;}

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
        !TestEqual(TEXT("Override gains canonical identity"), Effective.Receptacle.AcceptedItems[0].CanonicalItemId,
            Arrow->ItemDefinitionId) ||
        !TestNull(TEXT("Instance accepted override remains pointerless"),
            Effective.Receptacle.AcceptedItems[0].ItemDefinition.Get()) ||
        !TestEqual(TEXT("Override capacity remains 4"), Effective.Receptacle.MaxContainedItems, 4))
        {return false;}

    FGridReceptacleAcceptedItemConfig Contradictory;
    Contradictory.CanonicalItemId = Arrow->ItemDefinitionId;
    Contradictory.ItemDefinition = Sword;
    Defaults.Add(Contradictory);
    return TestFalse(TEXT("Contradictory inherited canonical rule rejected atomically"),
            FGridEditorCanonicalReceptacleRuleAuthoring::TryCopyDefaults(
                Registry.Get(), Defaults, Copies)) &&
        TestTrue(TEXT("No partial new rule set after failure"), Copies.IsEmpty()) &&
        TestEqual(TEXT("No mutation of native catalogue"), Root->GetOutermost()->IsDirty(), WasDirty);
}
#endif
