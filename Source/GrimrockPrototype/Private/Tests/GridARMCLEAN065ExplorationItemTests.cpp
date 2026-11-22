#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Runtime/GridExplorationItemReadModel.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMCLEAN065ExplorationItemReaders,
    "Grimrock.RPG.ARM.CLEAN06_5D.PointerlessExplorationItems",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMCLEAN065ExplorationItemReaders::RunTest(const FString&)
{
    using Read = FGridExplorationItemReadModel;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
    TArray<FString> Errors;
    if (!TestNotNull(TEXT("Real registry"), Root.Get()) ||
        !TestTrue(TEXT("Register actual native definitions"), Registry->RegisterItemSystem(Root.Get(), Errors))) return false;
    FGridCharacterInventoryState Character;
    Character.CharacterId = FGuid::NewGuid();
    Character.InventorySlots.SetNum(1);
    auto& Slot = Character.InventorySlots[0];
    Slot.bOccupied = true;
    Slot.Item.RuntimeObjectId = FGuid::NewGuid();
    Slot.Item.OwnerGuid = Character.CharacterId;
    Slot.Item.OwnerType = EGridItemOwnerType::CharacterInventory;
    Slot.Item.OwnerCharacterIndex = 0;
    Slot.Item.ItemDefinitionId = TEXT("Item_DisarmTools_01");
    Slot.Item.ItemLevel = 0;
    Slot.Item.Quantity = 1;
    TestTrue(TEXT("Actual pointerless Saboteur tools qualify"),
        Read::HasDisarmTool(Registry.Get(), Character, TEXT("Tool.Disarm")));
    TestTrue(TEXT("Same canonical instance authorizes the context reader"),
        Read::IsDisarmToolInstance(Registry.Get(), Slot.Item, TEXT("Tool.Disarm")));
    TestFalse(TEXT("Missing registry cannot authorize a tool"), Read::HasDisarmTool(nullptr, Character, TEXT("Tool.Disarm")));
    Slot.Item.Quantity = MAX_int32;
    TestFalse(TEXT("Over-capacity tool stack cannot authorize disarm"), Read::HasDisarmTool(Registry.Get(), Character, TEXT("Tool.Disarm")));
    Slot.Item.Quantity = 1;
    TestFalse(TEXT("Wrong tag rejected"), Read::HasDisarmTool(Registry.Get(), Character, TEXT("Wrong.Tag")));
    Slot.Item.ItemLevel = 1;
    TestFalse(TEXT("Wrong physical level rejected"), Read::HasDisarmTool(Registry.Get(), Character, TEXT("Tool.Disarm")));
    Slot.Item.ItemLevel = 0;
    Slot.bOccupied = false;
    TestFalse(TEXT("Tool must be owned"), Read::HasDisarmTool(Registry.Get(), Character, TEXT("Tool.Disarm")));
    Slot.bOccupied = true;

    const FName Items[] = {TEXT("Item_Flask_Oil"), TEXT("Item_SurfaceCatalyst_01")};
    const FName Actions[] = {TEXT("Action_Alchemist_OilSlick"), TEXT("Action_Mage_SurfaceCatalyst_01")};
    for (int32 I = 0; I < 2; ++I)
    {
        Slot.Item.ItemDefinitionId = Items[I];
        const auto Source = Read::ResolveSurface(Registry.Get(), Character, Items[I], Actions[I], true);
        if (!TestTrue(TEXT("Owned canonical surface QuickItem resolves without old asset"),
            Source.bResolved && Source.bOwned)) return false;
        TestTrue(TEXT("Existing exploration policy remains valid"), I == 0
            ? UGridTurnManagerComponent::IsExplorationOilSlickAction(Source.Action)
            : UGridTurnManagerComponent::IsExplorationCatalystAction(Source.Action));
        TestFalse(TEXT("Authored projectile mesh from canonical item"), Source.Mesh.IsNull());
        TestTrue(TEXT("Post-consumption mesh comes from same registered identity"),
            Read::ResolveSurfaceMesh(Registry.Get(), Items[I]) == Source.Mesh);
        TestFalse(TEXT("Missing registry cannot authorize owned surface"),
            Read::ResolveSurface(static_cast<const UGridItemSubsystem*>(nullptr), Character, Items[I], Actions[I], true).bResolved);
        TestTrue(TEXT("Missing registry has no projectile mesh"),
            Read::ResolveSurfaceMesh(static_cast<const UGridItemSubsystem*>(nullptr), Items[I]).IsNull());
        TestFalse(TEXT("Unapproved identity cannot authorize surface"),
            Read::ResolveSurface(Registry.Get(), Character, TEXT("Item_Torch"), Actions[I], false).bResolved);
        Slot.Item.Quantity = MAX_int32;
        TestFalse(TEXT("Over-capacity physical surface stack is rejected"),
            Read::ResolveSurface(Registry.Get(), Character, Items[I], Actions[I], true).bResolved);
        Slot.Item.Quantity = 1;
        TestEqual(TEXT("Exact action identity"), Source.Action.ActionId, Actions[I]);
        TestEqual(TEXT("One item consumed by authored action"), Source.Action.ResourceCosts.SourceItemQuantityCost, 1);
        TestFalse(TEXT("Stale binding cannot substitute action"),
            Read::ResolveSurface(Registry.Get(), Character, Items[I], Actions[1-I], true).bResolved);
        Slot.Item.ItemLevel = 1;
        TestFalse(TEXT("Invalid owned N1 fails closed"),
            Read::ResolveSurface(Registry.Get(), Character, Items[I], Actions[I], true).bResolved);
        Slot.Item.ItemLevel = 0;
        Slot.bOccupied = false;
        const auto Empty = Read::ResolveSurface(Registry.Get(), Character, Items[I], Actions[I], false);
        TestTrue(TEXT("Empty hotbar may display disabled static action"), Empty.bResolved && !Empty.bOwned);
        TestFalse(TEXT("Empty hotbar never executes"),
            Read::ResolveSurface(Registry.Get(), Character, Items[I], Actions[I], true).bResolved);
        Slot.bOccupied = true;
    }
    return !HasAnyErrors();
}
#endif
