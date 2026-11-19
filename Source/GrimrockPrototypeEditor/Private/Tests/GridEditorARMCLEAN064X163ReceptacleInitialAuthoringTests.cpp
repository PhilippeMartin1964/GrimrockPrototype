#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "EditorTools/GridEditorCanonicalReceptacleInitialAuthoring.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
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

    UGridItemDefinitionAsset* Arrows = NewObject<UGridItemDefinitionAsset>();
    Arrows->ItemDefinitionId = TEXT("Item_Arrows_01");
    UGridItemDefinitionAsset* Sword = NewObject<UGridItemDefinitionAsset>();
    Sword->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
    Sword->DefaultWeaponItemLevel = 7;
    UGridItemDefinitionAsset* Unknown = NewObject<UGridItemDefinitionAsset>();
    Unknown->ItemDefinitionId = TEXT("X163_Unmigrated_Historical");

    FGridReceptacleInitialItemConfig Item;
    Item.Quantity = 5;
    if (!TestFalse(TEXT("Missing registry cannot change initial content"),
        FGridEditorCanonicalReceptacleInitialAuthoring::TrySelectDefinition(
            nullptr, Sword, Item)) ||
        !TestTrue(TEXT("Rejected change remains untouched"),
            Item.ItemDefinition == nullptr && Item.CanonicalItemId.IsNone() &&
            Item.ItemLevel == 0 && Item.Quantity == 5)) {return false;}

    if (!TestTrue(TEXT("Non-ARM authored at real N0"),
        FGridEditorCanonicalReceptacleInitialAuthoring::TrySelectDefinition(
            Registry.Get(), Arrows, Item)) ||
        !TestEqual(TEXT("Canonical arrow identity"), Item.CanonicalItemId,
            Arrows->ItemDefinitionId) ||
        !TestEqual(TEXT("Unlevelled item stays N0"), Item.ItemLevel, 0))
        {return false;}
    if (!TestFalse(TEXT("Persisted nonARM N1 rejected"),
        FGridEditorCanonicalReceptacleInitialAuthoring::TrySetLevel(
            Registry.Get(), 1, Item)) ||
        !TestEqual(TEXT("Arrow remains N0"), Item.ItemLevel, 0))
        {return false;}

    if (!TestTrue(TEXT("Changing to ARM stores an explicit physical N1"),
        FGridEditorCanonicalReceptacleInitialAuthoring::TrySelectDefinition(
            Registry.Get(), Sword, Item)) ||
        !TestEqual(TEXT("Canonical sword identity replaces arrow"),
            Item.CanonicalItemId, Sword->ItemDefinitionId) ||
        !TestEqual(TEXT("ARM N1 from canonical authoring, not old N7"),
            Item.ItemLevel, 1) ||
        !TestEqual(TEXT("Quantity unchanged"), Item.Quantity, 5))
        {return false;}
    if (!TestFalse(TEXT("Persisted ARM N0 cannot inherit arbitrary N1"),
        FGridEditorCanonicalReceptacleInitialAuthoring::TrySetLevel(
            Registry.Get(), 0, Item)) ||
        !TestEqual(TEXT("Refused N0 leaves N1 intact"), Item.ItemLevel, 1))
        {return false;}
    if (!TestTrue(TEXT("ARM N20 is an editable physical level"),
        FGridEditorCanonicalReceptacleInitialAuthoring::TrySetLevel(
            Registry.Get(), 20, Item)) ||
        !TestEqual(TEXT("N20 fixed"), Item.ItemLevel, 20))
        {return false;}

    Sword->DefaultWeaponItemLevel = 0;
    if (!TestTrue(TEXT("Old default cannot prevent selection of the same explicit ID"),
        FGridEditorCanonicalReceptacleInitialAuthoring::TrySelectDefinition(
            Registry.Get(), Sword, Item)) ||
        !TestEqual(TEXT("Same ID retains physical N20"),
            Item.ItemLevel, 20))
        {return false;}

    if (!TestTrue(TEXT("Switch to unregistered fixture leaves no canonical ID"),
        FGridEditorCanonicalReceptacleInitialAuthoring::TrySelectDefinition(
            Registry.Get(), Unknown, Item)) ||
        !TestTrue(TEXT("Historical fixture reset to old N0 behavior"),
            Item.CanonicalItemId.IsNone() && Item.ItemLevel == 0 &&
            Item.ItemDefinition == Unknown))
        {return false;}
    if (!TestTrue(TEXT("Removing initial source clears identity and level"),
        FGridEditorCanonicalReceptacleInitialAuthoring::TrySelectDefinition(
            Registry.Get(), nullptr, Item)) ||
        !TestTrue(TEXT("No previous canonical identity survives clearing"),
            Item.ItemDefinition == nullptr && Item.CanonicalItemId.IsNone() &&
            Item.ItemLevel == 0 && Item.Quantity == 5))
        {return false;}

    FGridReceptacleInitialItemConfig Contradiction;
    Contradiction.ItemDefinition = Arrows;
    Contradiction.CanonicalItemId = Sword->ItemDefinitionId;
    Contradiction.ItemLevel = 20;
    if (!TestFalse(TEXT("Contradictory pointer cannot set canonical level"),
        FGridEditorCanonicalReceptacleInitialAuthoring::TrySetLevel(
            Registry.Get(), 7, Contradiction)) ||
        !TestEqual(TEXT("Contradiction did not change level"),
            Contradiction.ItemLevel, 20))
        {return false;}

    FGridReceptacleInitialItemConfig Canonical;
    Canonical.Quantity = 3;
    using FAuthoring = FGridEditorCanonicalReceptacleInitialAuthoring;
    if (!TestTrue(TEXT("New canonical arrow needs no source asset"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Arrows->ItemDefinitionId, Canonical)) ||
        !TestNull(TEXT("No legacy pointer on new arrow"), Canonical.ItemDefinition.Get()) ||
        !TestEqual(TEXT("Arrow N0"), Canonical.ItemLevel, 0) ||
        !TestFalse(TEXT("Non-ARM level not editable"), FAuthoring::IsLevelEditable(Registry.Get(), Canonical)) ||
        !TestFalse(TEXT("Unknown registered identity refused"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Unknown->ItemDefinitionId, Canonical)) ||
        !TestEqual(TEXT("Refused identity preserves arrow"), Canonical.CanonicalItemId, Arrows->ItemDefinitionId) ||
        !TestTrue(TEXT("New ARM explicit physical N1"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Sword->ItemDefinitionId, Canonical)) ||
        !TestEqual(TEXT("New ARM N1"), Canonical.ItemLevel, 1) ||
        !TestTrue(TEXT("Pointerless ARM level editable"), FAuthoring::IsLevelEditable(Registry.Get(), Canonical)) ||
        !TestTrue(TEXT("Pointerless ARM can author N20"),
            FAuthoring::TrySetLevel(Registry.Get(), 20, Canonical)) ||
        !TestFalse(TEXT("Pointerless ARM N0 refused"),
            FAuthoring::TrySetLevel(Registry.Get(), 0, Canonical)) ||
        !TestTrue(TEXT("Recommit same ID preserves N20"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Sword->ItemDefinitionId, Canonical)) ||
        !TestEqual(TEXT("Physical level remains N20"), Canonical.ItemLevel, 20) ||
        !TestNull(TEXT("No historical pointer reintroduced"), Canonical.ItemDefinition.Get()) ||
        !TestTrue(TEXT("Change back to unlevelled resets to physical N0"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Arrows->ItemDefinitionId, Canonical)) ||
        !TestEqual(TEXT("Arrow now N0"), Canonical.ItemLevel, 0) ||
        !TestEqual(TEXT("Quantity preserved by canonical edits"), Canonical.Quantity, 3))
        return false;

    FGridReceptacleInitialItemConfig LegacyBacked;
    LegacyBacked.CanonicalItemId = Sword->ItemDefinitionId;
    LegacyBacked.ItemDefinition = Sword;
    LegacyBacked.ItemLevel = 7;
    if (!TestTrue(TEXT("Same ID does not implicitly migrate old row"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Sword->ItemDefinitionId, LegacyBacked)) ||
        !TestTrue(TEXT("Original source preserved until explicit native conversion"), LegacyBacked.ItemDefinition == Sword) ||
        !TestEqual(TEXT("Authored N7 preserved"), LegacyBacked.ItemLevel, 7) ||
        !TestFalse(TEXT("Contradictory identity cannot be silently repaired"),
            FAuthoring::TrySelectCanonicalId(Registry.Get(), Arrows->ItemDefinitionId, Contradiction)))
        return false;

    return TestEqual(TEXT("No mutation of real ItemSystem"),
        Root->GetOutermost()->IsDirty(), WasDirty);
}
#endif
