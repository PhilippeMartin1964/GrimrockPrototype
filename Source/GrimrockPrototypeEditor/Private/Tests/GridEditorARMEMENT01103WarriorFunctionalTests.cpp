#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridEditorCanonicalStarterTestWorld.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace ARMEMENT01103
{
    constexpr TCHAR PalettePath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");
    constexpr TCHAR DungeonPath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00");

    struct FWeapon
    {
        const TCHAR* Archetype;
        const TCHAR* ItemId;
        const TCHAR* VisualId;
        const TCHAR* AttackId;
    };
    constexpr FWeapon Weapons[] =
    {
        {TEXT("ARM014"), TEXT("Weapon_StraightSword_01"),
            TEXT("VIS_StraightSword_01"), TEXT("Attack_StraightSword_01")},
        {TEXT("ARM015"), TEXT("Weapon_LongSword_01"),
            TEXT("VIS_LongSword_01"), TEXT("Attack_LongSword_01")},
        {TEXT("ARM025"), TEXT("Weapon_WarPick_01"),
            TEXT("VIS_WarPick_01"), TEXT("Attack_WarPick_01")},
        {TEXT("ARM031"), TEXT("Weapon_GreatAxe_01"),
            TEXT("VIS_GreatAxe_01"), TEXT("Attack_GreatAxe_01")}
    };

    bool Mint(UGridPartyInventoryComponent* Inventory, FName ItemId, FGridItemInstance& Out)
    {
        if (!Inventory) return false;
        Out = FGridItemInstance();
        Out.RuntimeObjectId = FGuid::NewGuid();
        Out.ItemDefinitionId = ItemId;
        Out.ItemLevel = 1;
        Out.Quantity = 1;
        return Inventory->ApplyItemDefinitionToInstance(Out) && Out.ItemLevel == 1;
    }

    int32 BagSlot(const UGridPartyInventoryComponent* Inventory, FGuid Id)
    {
        if (!Inventory || Inventory->PartyInventoryState.ActiveCharacters.IsEmpty())
            return INDEX_NONE;
        const TArray<FGridInventorySlot>& Slots =
            Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
        for (int32 Index = 0; Index < Slots.Num(); ++Index)
            if (!Slots[Index].IsEmpty() && Slots[Index].Item.RuntimeObjectId == Id)
                return Index;
        return INDEX_NONE;
    }

    int32 PhysicalCount(const UGridPartyInventoryComponent* Inventory, FGuid Id)
    {
        if (!Inventory || Inventory->PartyInventoryState.ActiveCharacters.IsEmpty()) return 0;
        int32 Count = 0;
        for (const FGridInventorySlot& Slot : Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots)
            if (!Slot.IsEmpty() && Slot.Item.RuntimeObjectId == Id) ++Count;
        const FGridCharacterEquipmentState& Gear = Inventory->PartyInventoryState.ActiveEquipment[0];
        if (Gear.MainHand.IsValid() && Gear.MainHand.RuntimeObjectId == Id) ++Count;
        if (Gear.OffHand.IsValid() && Gear.OffHand.RuntimeObjectId == Id) ++Count;
        return Count;
    }

    struct FPreviewWorld
    {
        UWorld* World = nullptr;
        FPreviewWorld()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
                FName(*FString::Printf(TEXT("ARM01103_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
                Context.SetCurrentWorld(World);
            }
        }
        ~FPreviewWorld()
        {
            if (World) World->DestroyWorld(false);
            if (World && GEngine) GEngine->DestroyWorldContext(World);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01103ProductionBindings,
    "Grimrock.RPG.ARMEMENT01.10.3.ProductionBindings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01103ProductionBindings::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    const UGridObjectPaletteAsset* Palette =
        LoadObject<UGridObjectPaletteAsset>(nullptr, ARMEMENT01103::PalettePath);
    const UGridLevelAsset* Dungeon =
        LoadObject<UGridLevelAsset>(nullptr, ARMEMENT01103::DungeonPath);
    if (!TestTrue(TEXT("Canonical production registry"), Registry.bReady) ||
        !TestNotNull(TEXT("Real weapon palette"), Palette) ||
        !TestNotNull(TEXT("Real dungeon level"), Dungeon)) return false;
    for (const ARMEMENT01103::FWeapon& Spec : ARMEMENT01103::Weapons)
    {
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("One of four canonical warrior weapons"),
            Registry.Resolve(FName(Spec.ItemId), 1, Item))) continue;
        TestTrue(TEXT("Valid non-stackable ARM N1"),
            Item.IsValid() && Item.bIsArmament && Item.MaxStack == 1);
        TestEqual(TEXT("Canonical archetype"), Item.ArchetypeId, FName(Spec.Archetype));
        TestEqual(TEXT("Canonical visual id"), Item.VisualId, FName(Spec.VisualId));
        TestTrue(TEXT("Authored MainHand attack bound in ItemSystem"),
            Item.AvailableActions.Num() == 1 &&
            Item.AvailableActions[0].ActionId == FName(Spec.AttackId) &&
            Item.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand));
        TestTrue(TEXT("Icon and world mesh actually load"),
            Item.ItemIcon.LoadSynchronous() && Item.WorldMesh.LoadSynchronous());
        TestTrue(TEXT("N1 weight is finite and positive"),
            FMath::IsFinite(Item.WeightKg) && Item.WeightKg > 0.f);
        TestEqual(TEXT("Only Great Axe has two-handed policy"),
            Item.Weapon.HandPolicy == EGridHandPolicy::TwoHanded,
            Item.ArchetypeId == FName(TEXT("ARM031")));

        const FName EntryId(Spec.ItemId);
        const FGridObjectPaletteEntry* Entry = Palette->FindEntryById(EntryId);
        if (TestNotNull(TEXT("Direct collectible palette entry exists"), Entry))
        {
            TestEqual(TEXT("Weapon category"), Entry->PaletteCategory, FName(TEXT("Weapon")));
            TestEqual(TEXT("Canonical item identity"), Entry->ItemId, Item.ItemId);
            TestNull(TEXT("Old pointer property absent"),
                FGridObjectPaletteEntry::StaticStruct()->FindPropertyByName(TEXT("DefaultItemDefinition")));
            TestNull(TEXT("No duplicate icon"), Entry->Icon.Get());
            TestTrue(TEXT("No duplicate world-object definition"),
                Entry->DefaultWorldObjectDefinition == nullptr);
            TestTrue(TEXT("Palette entry valid"), Entry->IsValidEntry());
            TestEqual(TEXT("No duplicate palette entry"),
                Palette->Entries.FilterByPredicate(
                    [EntryId](const FGridObjectPaletteEntry& Candidate)
                    { return Candidate.EntryId == EntryId; }).Num(), 1);
        }
        int32 DungeonCount = 0;
        for (const FGridLooseItemInstance& Placed : Dungeon->LooseItemInstances)
            if (Placed.ItemId == Item.ItemId &&
                Placed.ItemLevel >= 1 && Placed.ItemLevel <= 20) ++DungeonCount;
        TestTrue(TEXT("Canonical weapon still placed in production dungeon"),
            DungeonCount > 0);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01103TransientPlacement,
    "Grimrock.RPG.ARMEMENT01.10.3.TransientEditorPlacement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01103TransientPlacement::RunTest(const FString&)
{
    ARMEMENT01103::FPreviewWorld Fixture;
    if (!TestNotNull(TEXT("Transient editor preview world"), Fixture.World)) return false;
    UGridObjectPaletteAsset* Palette =
        LoadObject<UGridObjectPaletteAsset>(nullptr, ARMEMENT01103::PalettePath);
    AGridLevelEditorActor* Editor = Fixture.World->SpawnActor<AGridLevelEditorActor>();
    AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Actual palette"), Palette) ||
        !TestNotNull(TEXT("Grid editor actor"), Editor) ||
        !TestNotNull(TEXT("Grid preview runtime"), Runtime)) return false;
    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
    Level->Width = 5;
    Level->Height = 4;
    Level->EnsureCellCount();
    for (FGridLevelCellData& Cell : Level->Cells)
    {
        Cell.CellType = EGridCellType::Floor;
        Cell.bBlocksOccupancy = false;
    }
    Editor->LevelAsset = Level;
    Editor->ObjectPalette = Palette;
    Runtime->LevelAsset = Level;
    const UGridItemSubsystem* Registry = Editor->ResolveEditorItemRegistry();
    if (!TestNotNull(TEXT("Real ItemSystem for editor preview"), Registry)) return false;

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ARMEMENT01103::Weapons); ++Index)
    {
        const ARMEMENT01103::FWeapon& Spec = ARMEMENT01103::Weapons[Index];
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Canonical N1 item for palette placement"),
            Registry->ResolveItem(FName(Spec.ItemId), 1, Item))) continue;
        Editor->SelectedCellX = Index;
        Editor->SelectedCellY = 1;
        Editor->SelectedEdge = EGridEdge::None;
        if (!TestTrue(TEXT("Palette selection"), Editor->ApplyPaletteEntry(Item.ItemId))) continue;
        TestEqual(TEXT("Collectible item paint mode"), Editor->PaintObjectType, EGridLevelObjectType::Item);
        Editor->PlaceSelectedObject();
        const FGridLooseItemInstance* Placed = Level->LooseItemInstances.FindByPredicate(
            [ItemId = Item.ItemId, Index](const FGridLooseItemInstance& Candidate)
            {
                return Candidate.ItemId == ItemId &&
                    Candidate.ItemLevel >= 1 && Candidate.ItemLevel <= 20 &&
                    Candidate.CellX == Index && Candidate.CellY == 1;
            });
        if (TestNotNull(TEXT("Pointerless placed item"), Placed))
        {
            TestTrue(TEXT("Unique item GUID persists"), Placed->InstanceId.IsValid());
            TestEqual(TEXT("Palette identity persists"), Placed->PaletteEntryId, Item.ItemId);
        }
    }
    TestEqual(TEXT("All four weapons painted"), Level->LooseItemInstances.Num(), 4);
    TestTrue(TEXT("No companion world-objects"), Level->WorldObjectInstances.IsEmpty());
    UGridEditorPreviewComponent* Preview =
        Runtime->FindComponentByClass<UGridEditorPreviewComponent>();
    if (!TestNotNull(TEXT("3D preview component"), Preview)) return false;
    Preview->Initialize(Runtime);
    Preview->RebuildPreviewObjects();
    for (const FGridLooseItemInstance& Placed : Level->LooseItemInstances)
    {
        TestNull(TEXT("No old placed definition property"),
            FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
        FGridResolvedItem Canonical;
        if (!TestTrue(TEXT("Preview item resolves exact physical level"),
            Registry->ResolveItem(Placed.ItemId, Placed.ItemLevel, Canonical))) continue;
        UStaticMesh* Expected = Canonical.WorldMesh.LoadSynchronous();
        if (!TestNotNull(TEXT("Actual canonical world mesh"), Expected)) continue;
        AGridEditorPreviewObjectActor* Actor = nullptr;
        for (TActorIterator<AGridEditorPreviewObjectActor> It(Fixture.World); It; ++It)
            if ((*It)->ObjectId == Placed.InstanceId) { Actor = *It; break; }
        if (!TestNotNull(TEXT("Placed 3D preview actor"), Actor)) continue;
        TestEqual(TEXT("Correct canonical preview mesh"),
            Actor->MeshComponent->GetStaticMesh().Get(), Expected);
        TestTrue(TEXT("Correct canonical visual offset"),
            Actor->MeshComponent->GetRelativeTransform().Equals(Canonical.WorldVisualTransform));
    }
    Preview->ClearPreviewObjects();
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01103RealInventory,
    "Grimrock.RPG.ARMEMENT01.10.3.RealInventoryEquipUnequip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01103RealInventory::RunTest(const FString&)
{
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("Game-world ItemSystem configured"), Fixture.Ready())) return false;
    for (const ARMEMENT01103::FWeapon& Spec : ARMEMENT01103::Weapons)
    {
        FGridResolvedItem Definition;
        if (!TestTrue(TEXT("Canonical N1 warrior weapon"),
            Fixture.Registry->ResolveItem(FName(Spec.ItemId), 1, Definition))) continue;
        UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
        if (!TestNotNull(TEXT("World-bound inventory"), Inventory) ||
            !TestTrue(TEXT("Test character"), Inventory->IsValidCharacterIndex(0))) continue;
        FGridItemInstance Instance;
        if (!TestTrue(TEXT("Mint canonical N1"),
            ARMEMENT01103::Mint(Inventory, Definition.ItemId, Instance))) continue;
        if (!TestTrue(TEXT("Add to bag"), Inventory->AddItemToCharacterInventory(0, Instance))) continue;
        const int32 Index = ARMEMENT01103::BagSlot(Inventory, Instance.RuntimeObjectId);
        if (!TestTrue(TEXT("Physical bag item"), Index != INDEX_NONE)) continue;
        if (!TestTrue(TEXT("Equip MainHand"), Inventory->EquipItemFromInventorySlot(
            0, Index, EGridEquipmentSlot::MainHand))) continue;
        TestEqual(TEXT("Exactly one physical GUID"),
            ARMEMENT01103::PhysicalCount(Inventory, Instance.RuntimeObjectId), 1);
        const FGridCharacterEquipmentState& Gear = Inventory->PartyInventoryState.ActiveEquipment[0];
        TestEqual(TEXT("MainHand preserves GUID"), Gear.MainHand.RuntimeObjectId, Instance.RuntimeObjectId);
        const bool bTwo = Definition.Weapon.HandPolicy == EGridHandPolicy::TwoHanded;
        TestEqual(TEXT("Two-hand policy from ItemSystem"), Inventory->IsTwoHandedEquipped(0), bTwo);
        if (bTwo)
        {
            TestFalse(TEXT("No duplicate serialized OffHand"), Gear.OffHand.IsValid());
            TestTrue(TEXT("OffHand reserved"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
            FGridItemInstance Virtual;
            TestTrue(TEXT("Virtual OffHand resolves same physical item"),
                Inventory->GetEquippedItem(0, EGridEquipmentSlot::OffHand, Virtual));
            TestEqual(TEXT("One shared GUID"), Virtual.RuntimeObjectId, Instance.RuntimeObjectId);
        }
        if (!TestTrue(TEXT("Unequip into bag"), Inventory->UnequipItemToInventory(
            0, bTwo ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand))) continue;
        TestFalse(TEXT("MainHand free"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));
        TestFalse(TEXT("OffHand free"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
        TestTrue(TEXT("Original item returned"),
            ARMEMENT01103::BagSlot(Inventory, Instance.RuntimeObjectId) != INDEX_NONE);
        TestEqual(TEXT("No item duplication/loss"),
            ARMEMENT01103::PhysicalCount(Inventory, Instance.RuntimeObjectId), 1);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01103GreatAxeTwoHands,
    "Grimrock.RPG.ARMEMENT01.10.3.GreatAxeTwoHandedTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01103GreatAxeTwoHands::RunTest(const FString&)
{
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("Real ItemSystem world"), Fixture.Ready())) return false;
    UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
    if (!TestNotNull(TEXT("Canonical inventory"), Inventory)) return false;
    FGridItemInstance Sword, Axe, Shield;
    if (!TestTrue(TEXT("Mint sword"), ARMEMENT01103::Mint(Inventory, TEXT("Weapon_StraightSword_01"), Sword)) ||
        !TestTrue(TEXT("Mint great axe"), ARMEMENT01103::Mint(Inventory, TEXT("Weapon_GreatAxe_01"), Axe)) ||
        !TestTrue(TEXT("Mint round shield"), ARMEMENT01103::Mint(Inventory, TEXT("Weapon_RoundShield_01"), Shield)))
        return false;
    if (!TestTrue(TEXT("Sword added"), Inventory->AddItemToCharacterInventory(0, Sword)) ||
        !TestTrue(TEXT("Shield added"), Inventory->AddItemToCharacterInventory(0, Shield)) ||
        !TestTrue(TEXT("Axe added"), Inventory->AddItemToCharacterInventory(0, Axe)))
        return false;
    const int32 SwordSlot = ARMEMENT01103::BagSlot(Inventory, Sword.RuntimeObjectId);
    if (!TestTrue(TEXT("Equip sword"), SwordSlot != INDEX_NONE &&
        Inventory->EquipItemFromInventorySlot(0, SwordSlot, EGridEquipmentSlot::MainHand))) return false;
    const int32 ShieldSlot = ARMEMENT01103::BagSlot(Inventory, Shield.RuntimeObjectId);
    if (!TestTrue(TEXT("Equip shield"), ShieldSlot != INDEX_NONE &&
        Inventory->EquipItemFromInventorySlot(0, ShieldSlot, EGridEquipmentSlot::OffHand))) return false;
    const int32 AxeSlot = ARMEMENT01103::BagSlot(Inventory, Axe.RuntimeObjectId);
    if (!TestTrue(TEXT("Axe atomically releases both equipped items"), AxeSlot != INDEX_NONE &&
        Inventory->EquipItemFromInventorySlot(0, AxeSlot, EGridEquipmentSlot::MainHand))) return false;
    TestTrue(TEXT("Axe is two-handed"), Inventory->IsTwoHandedEquipped(0));
    const FGridCharacterEquipmentState& Gear = Inventory->PartyInventoryState.ActiveEquipment[0];
    TestEqual(TEXT("GreatAxe in MainHand"), Gear.MainHand.RuntimeObjectId, Axe.RuntimeObjectId);
    TestFalse(TEXT("No physical OffHand duplicate"), Gear.OffHand.IsValid());
    TestTrue(TEXT("OffHand virtually reserved"),
        Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
    TestTrue(TEXT("Sword returned"), ARMEMENT01103::BagSlot(Inventory, Sword.RuntimeObjectId) != INDEX_NONE);
    TestTrue(TEXT("Shield returned"), ARMEMENT01103::BagSlot(Inventory, Shield.RuntimeObjectId) != INDEX_NONE);
    TestEqual(TEXT("Sword unique"), ARMEMENT01103::PhysicalCount(Inventory, Sword.RuntimeObjectId), 1);
    TestEqual(TEXT("Shield unique"), ARMEMENT01103::PhysicalCount(Inventory, Shield.RuntimeObjectId), 1);
    TestEqual(TEXT("Axe unique"), ARMEMENT01103::PhysicalCount(Inventory, Axe.RuntimeObjectId), 1);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01103CombatProfiles,
    "Grimrock.RPG.ARMEMENT01.10.3.RealCatalogueCombatProfiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01103CombatProfiles::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Canonical production ItemSystem"), Registry.bReady)) return false;
    for (const ARMEMENT01103::FWeapon& Spec : ARMEMENT01103::Weapons)
    {
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Actual N1 warrior ARM profile"),
            Registry.Resolve(FName(Spec.ItemId), 1, Item))) continue;
        FGridResolvedBoundAction Action;
        if (!TestTrue(TEXT("MainHand canonical attack is bound"),
            Registry.Registry->ResolveBoundAttackForHand(
                Item.ItemId, 1, FName(Spec.AttackId), EGridEquipmentSlot::MainHand, Action)))
            continue;
        TestTrue(TEXT("Canonical action resolves"), Action.IsValid());
        TestEqual(TEXT("Authored attack identity"), Action.ActionId, FName(Spec.AttackId));
        TestTrue(TEXT("Weapon owns its positive AP cost and range"),
            Item.Weapon.ActionPointCost > 0 && Item.Weapon.RangeCells > 0);
        TestTrue(TEXT("N1 retains actual damage dice"),
            Item.Weapon.DamageDice.bEnabled &&
            Item.Weapon.DamageDice.DiceCount > 0 && Item.Weapon.DamageDice.DiceSides > 0);
        FGridResolvedItem N20;
        if (TestTrue(TEXT("Real progression to N20"), Registry.Resolve(Item.ItemId, 20, N20)))
        {
            TestEqual(TEXT("Same archetype at N20"), N20.ArchetypeId, Item.ArchetypeId);
            TestTrue(TEXT("N20 still has authored damage dice"),
                N20.Weapon.DamageDice.bEnabled &&
                N20.Weapon.DamageDice.DiceCount > 0);
        }
    }
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
