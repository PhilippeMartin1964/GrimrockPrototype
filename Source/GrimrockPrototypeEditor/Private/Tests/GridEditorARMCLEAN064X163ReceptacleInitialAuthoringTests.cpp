#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "EditorTools/GridEditorReceptacleInitialAuthoring.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X163InitialAuthoring,
    "Grimrock.RPG.ARM.CLEAN06_4X163.PhysicalReceptacleInitialItemAuthoring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X163InitialAuthoring::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real canonical registry"), Root.Get())) {return false;}
    const bool WasDirty = Root->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Canonical ItemSystem configured"),
        Registry->RegisterItemSystem(Root.Get(), Errors))) {return false;}

    const FName Arrows = TEXT("Item_Arrows_01");
    const FName Sword = TEXT("Weapon_StraightSword_01");
    const FName Unknown = TEXT("X163_Unknown");
    using FAuthoring = FGridEditorReceptacleInitialAuthoring;
    FGridReceptacleInitialItemConfig Draft;
    Draft.Quantity = 5;
    if (!TestFalse(TEXT("Missing registry cannot change initial content"),
        FAuthoring::TrySelectCanonicalId(nullptr, Sword, Draft)) ||
        !TestTrue(TEXT("Rejected draft unchanged"), Draft.ItemId.IsNone() &&
            Draft.ItemLevel == 0 && Draft.Quantity == 5))
        return false;

    FGridReceptacleInitialItemConfig Canonical;
    Canonical.Quantity = 3;
    if (!TestTrue(TEXT("New canonical arrow needs no source asset"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Arrows, Canonical)) ||
        !TestEqual(TEXT("Arrow N0"), Canonical.ItemLevel, 0) ||
        !TestFalse(TEXT("Non-ARM level not editable"), FAuthoring::IsLevelEditable(Registry.Get(), Canonical)) ||
        !TestFalse(TEXT("Unknown registered identity refused"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Unknown, Canonical)) ||
        !TestEqual(TEXT("Refused identity preserves arrow"), Canonical.ItemId, Arrows) ||
        !TestTrue(TEXT("New ARM explicit physical N1"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Sword, Canonical)) ||
        !TestEqual(TEXT("New ARM N1"), Canonical.ItemLevel, 1) ||
        !TestTrue(TEXT("Pointerless ARM level editable"), FAuthoring::IsLevelEditable(Registry.Get(), Canonical)) ||
        !TestTrue(TEXT("Pointerless ARM can author N20"),
            FAuthoring::TrySetLevel(Registry.Get(), 20, Canonical)) ||
        !TestFalse(TEXT("Pointerless ARM N0 refused"),
            FAuthoring::TrySetLevel(Registry.Get(), 0, Canonical)) ||
        !TestTrue(TEXT("Recommit same ID preserves N20"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Sword, Canonical)) ||
        !TestEqual(TEXT("Physical level remains N20"), Canonical.ItemLevel, 20) ||
        !TestTrue(TEXT("Change back to unlevelled resets to physical N0"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Arrows, Canonical)) ||
        !TestEqual(TEXT("Arrow now N0"), Canonical.ItemLevel, 0) ||
        !TestEqual(TEXT("Quantity preserved by canonical edits"), Canonical.Quantity, 3))
        return false;

    FGridReceptacleInitialItemConfig Invalid;
    Invalid.ItemId = Sword;
    Invalid.ItemLevel = 0;
    if (!TestFalse(TEXT("Same ID does not repair a persisted invalid ARM N0"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Sword, Invalid)) ||
        !TestFalse(TEXT("Invalid physical level cannot be committed"),
            FAuthoring::TrySetLevel(Registry.Get(), 0, Invalid)) ||
        !TestEqual(TEXT("Rejected level remains unchanged"), Invalid.ItemLevel, 0) ||
        !TestTrue(TEXT("Explicit valid edit repairs the draft"),
            FAuthoring::TrySetLevel(Registry.Get(), 7, Invalid)) ||
        !TestEqual(TEXT("Explicit N7 stored"), Invalid.ItemLevel, 7))
        return false;

    return TestEqual(TEXT("No mutation of real ItemSystem"),
        Root->GetOutermost()->IsDirty(), WasDirty);
}
#endif
