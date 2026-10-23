#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/Combat/GridWorldItemVisualResolver.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace ARMEMENT01103
{
    constexpr const TCHAR* ItemFolder = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons");
    constexpr const TCHAR* PalettePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");
    constexpr const TCHAR* DungeonPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00");

    struct FWeapon
    {
        const TCHAR* Archetype;
        const TCHAR* Asset;
        const TCHAR* ItemId;
        const TCHAR* VisualId;
        const TCHAR* AttackId;
    };

    constexpr FWeapon Weapons[] =
    {
        { TEXT("ARM014"), TEXT("DA_Weapon_StraightSword_01"), TEXT("Weapon_StraightSword_01"),
          TEXT("VIS_StraightSword_01"), TEXT("Attack_StraightSword_01") },
        { TEXT("ARM015"), TEXT("DA_Weapon_LongSword_01"), TEXT("Weapon_LongSword_01"),
          TEXT("VIS_LongSword_01"), TEXT("Attack_LongSword_01") },
        { TEXT("ARM025"), TEXT("DA_Weapon_WarPick_01"), TEXT("Weapon_WarPick_01"),
          TEXT("VIS_WarPick_01"), TEXT("Attack_WarPick_01") },
        { TEXT("ARM031"), TEXT("DA_Weapon_GreatAxe_01"), TEXT("Weapon_GreatAxe_01"),
          TEXT("VIS_GreatAxe_01"), TEXT("Attack_GreatAxe_01") }
    };

    UGridItemDefinitionAsset* LoadWeapon(const FWeapon& Spec)
    {
        const FString Path = FString::Printf(TEXT("%s/%s.%s"), ItemFolder, Spec.Asset, Spec.Asset);
        return LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
    }

    UGridPartyInventoryComponent* MakeInventory()
    {
        UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
        Inventory->InitializeDefaultPartyIfNeeded();
        return Inventory;
    }

    bool Mint(UGridPartyInventoryComponent* Inventory, const UGridItemDefinitionAsset* Definition, FGridItemInstance& Out)
    {
        if (!Inventory || !Definition) return false;
        Out = FGridItemInstance();
        Out.RuntimeObjectId = FGuid::NewGuid();
        Out.ItemDefinitionId = Definition->ItemDefinitionId;
        Out.Quantity = 1;
        return Inventory->ApplyItemDefinitionToInstance(Out) && Out.ItemLevel == 1;
    }

    int32 BagSlot(const UGridPartyInventoryComponent* Inventory, const FGuid Id)
    {
        if (!Inventory || Inventory->PartyInventoryState.ActiveCharacters.IsEmpty()) return INDEX_NONE;
        const TArray<FGridInventorySlot>& Slots =
            Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
        for (int32 Index = 0; Index < Slots.Num(); ++Index)
        {
            if (!Slots[Index].IsEmpty() && Slots[Index].Item.RuntimeObjectId == Id) return Index;
        }
        return INDEX_NONE;
    }

    int32 PhysicalCount(const UGridPartyInventoryComponent* Inventory, const FGuid Id)
    {
        if (!Inventory || Inventory->PartyInventoryState.ActiveCharacters.IsEmpty()) return 0;
        int32 Count = 0;
        for (const FGridInventorySlot& Slot : Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots)
        {
            if (!Slot.IsEmpty() && Slot.Item.RuntimeObjectId == Id) ++Count;
        }
        const FGridCharacterEquipmentState& Equipment = Inventory->PartyInventoryState.ActiveEquipment[0];
        if (Equipment.MainHand.IsValid() && Equipment.MainHand.RuntimeObjectId == Id) ++Count;
        if (Equipment.OffHand.IsValid() && Equipment.OffHand.RuntimeObjectId == Id) ++Count;
        return Count;
    }

    struct FPreviewWorld
    {
        UWorld* World = nullptr;

        FPreviewWorld()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
                FName(*FString::Printf(TEXT("ARM01103_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
                Context.SetCurrentWorld(World);
            }
        }

        ~FPreviewWorld()
        {
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine) GEngine->DestroyWorldContext(World);
            }
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01103ProductionBindings,
    "Grimrock.RPG.ARMEMENT01.10.3.ProductionBindings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01103ProductionBindings::RunTest(const FString&)
{
    const UGridObjectPaletteAsset* Palette =
        LoadObject<UGridObjectPaletteAsset>(nullptr, ARMEMENT01103::PalettePath);
    const UGridLevelAsset* Dungeon = LoadObject<UGridLevelAsset>(nullptr, ARMEMENT01103::DungeonPath);
    if (!TestNotNull(TEXT("Real Weapon palette"), Palette) ||
        !TestNotNull(TEXT("Real dungeon level asset"), Dungeon)) return false;

    for (const ARMEMENT01103::FWeapon& Spec : ARMEMENT01103::Weapons)
    {
        UGridItemDefinitionAsset* Item = ARMEMENT01103::LoadWeapon(Spec);
        if (!TestNotNull(Spec.ItemId, Item)) continue;
        TestTrue(TEXT("Real definition is valid"), Item->IsValidDefinition());
        TestEqual(TEXT("Canonical item identity"), Item->ItemDefinitionId, FName(Spec.ItemId));
        TestEqual(TEXT("Catalogue archetype identity"), Item->WeaponArchetypeId, FName(Spec.Archetype));
        TestEqual(TEXT("Fixed initial item level"), Item->DefaultWeaponItemLevel, 1);
        TestTrue(TEXT("Real catalogue bindings enabled"), Item->bUseWeaponCatalogue && Item->bUseWeaponVisualCatalogue);
        TestFalse(TEXT("Weapon is never stackable"), Item->bStackable);
        TestEqual(TEXT("Exactly one authored attack"), Item->CombatActions.Num(), 1);

        const UGridWeaponCatalogueAsset* Rpg = Item->WeaponCatalogue.LoadSynchronous();
        const UGridWeaponVisualCatalogueAsset* Visuals = Item->WeaponVisualCatalogue.LoadSynchronous();
        if (!TestNotNull(TEXT("RPG catalogue"), Rpg) ||
            !TestNotNull(TEXT("Visual catalogue"), Visuals)) continue;
        const FGridWeaponLevelProfile* Profile = Rpg->FindProfile(FName(Spec.Archetype), 1);
        const FGridWeaponVisualProfile* Visual = Visuals->FindProfileForArchetype(FName(Spec.Archetype));
        if (!TestNotNull(TEXT("Real N1 profile"), Profile) ||
            !TestNotNull(TEXT("Real visual profile"), Visual)) continue;
        TestEqual(TEXT("Visual id"), Visual->VisualId, FName(Spec.VisualId));
        TestTrue(TEXT("Item weight matches RPG catalogue"), FMath::IsNearlyEqual(Item->Weight, Profile->WeightKg));
        TestTrue(TEXT("World mesh loads"), Visual->WorldMesh.LoadSynchronous() != nullptr);
        TestTrue(TEXT("Icon loads"), Visual->BaseIcon.LoadSynchronous() != nullptr);
        const EGridItemHandUsage ExpectedHands =
            Profile->HandUsage == TEXT("2 mains") ? EGridItemHandUsage::TwoHanded
            : EGridItemHandUsage::OneHanded;
        TestTrue(TEXT("Workbook contains a supported hand usage"),
            Profile->HandUsage == TEXT("1 main") || Profile->HandUsage == TEXT("2 mains"));
        TestEqual(TEXT("Physical hand usage follows catalogue"), Item->GetEffectiveHandUsage(), ExpectedHands);
        if (FName(Spec.Archetype) == FName(TEXT("ARM031")))
        {
            TestEqual(TEXT("ARM031 is a two-handed Great Axe"), Item->GetEffectiveHandUsage(), EGridItemHandUsage::TwoHanded);
        }

        const FName EntryId(Spec.ItemId);
        const FGridObjectPaletteEntry* Entry = Palette->FindEntryById(EntryId);
        if (TestNotNull(TEXT("Real collectible palette entry"), Entry))
        {
            TestEqual(TEXT("Weapon palette category"), Entry->PaletteCategory, FName(TEXT("Weapon")));
            TestEqual(TEXT("Palette points to actual item definition"), Entry->DefaultItemDefinition.Get(), Item);
            TestTrue(TEXT("No second WorldObject definition"), Entry->DefaultWorldObjectDefinition == nullptr);
            TestTrue(TEXT("No duplicate palette icon"), Entry->Icon == nullptr);
            TestTrue(TEXT("Palette entry valid"), Entry->IsValidEntry());
            const int32 Duplicates = Palette->Entries.FilterByPredicate(
                [EntryId](const FGridObjectPaletteEntry& Candidate) { return Candidate.EntryId == EntryId; }).Num();
            TestEqual(TEXT("No duplicate palette entries"), Duplicates, 1);
        }

        int32 DungeonCount = 0;
        for (const FGridLooseItemInstance& Placed : Dungeon->LooseItemInstances)
        {
            if (Placed.ItemDefinition.Get() == Item) ++DungeonCount;
        }
        TestTrue(TEXT("Real dungeon contains this weapon as a loose item"), DungeonCount > 0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01103TransientPlacement,
    "Grimrock.RPG.ARMEMENT01.10.3.TransientEditorPlacement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01103TransientPlacement::RunTest(const FString&)
{
    ARMEMENT01103::FPreviewWorld Fixture;
    if (!TestNotNull(TEXT("Temporary editor world"), Fixture.World)) return false;
    UGridObjectPaletteAsset* Palette = LoadObject<UGridObjectPaletteAsset>(nullptr, ARMEMENT01103::PalettePath);
    AGridLevelEditorActor* Editor = Fixture.World->SpawnActor<AGridLevelEditorActor>();
    AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Real palette"), Palette) ||
        !TestNotNull(TEXT("Editor actor"), Editor) ||
        !TestNotNull(TEXT("Preview runtime actor"), Runtime)) return false;

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

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ARMEMENT01103::Weapons); ++Index)
    {
        const ARMEMENT01103::FWeapon& Spec = ARMEMENT01103::Weapons[Index];
        UGridItemDefinitionAsset* Item = ARMEMENT01103::LoadWeapon(Spec);
        if (!TestNotNull(TEXT("Weapon to paint"), Item)) continue;
        Editor->SelectedCellX = Index;
        Editor->SelectedCellY = 1;
        Editor->SelectedEdge = EGridEdge::None;
        if (!TestTrue(TEXT("Select weapon from real palette"), Editor->ApplyPaletteEntry(FName(Spec.ItemId))))
            continue;
        TestEqual(TEXT("Palette selection is a direct loose item"), Editor->PaintObjectType, EGridLevelObjectType::Item);
        Editor->PlaceSelectedObject();
        const FGridLooseItemInstance* Placed = Level->LooseItemInstances.FindByPredicate(
            [Item, Index](const FGridLooseItemInstance& Candidate)
            {
                return Candidate.ItemDefinition.Get() == Item && Candidate.CellX == Index && Candidate.CellY == 1;
            });
        if (TestNotNull(TEXT("Correct item saved in temporary level"), Placed))
        {
            TestTrue(TEXT("Placed instance has a stable ID"), Placed->InstanceId.IsValid());
            TestEqual(TEXT("Palette source identity retained"), Placed->PaletteEntryId, FName(Spec.ItemId));
        }
    }
    TestEqual(TEXT("Four real weapons painted"), Level->LooseItemInstances.Num(), 4);
    TestTrue(TEXT("No companion world objects"), Level->WorldObjectInstances.IsEmpty());

    UGridEditorPreviewComponent* Preview = Runtime->FindComponentByClass<UGridEditorPreviewComponent>();
    if (!TestNotNull(TEXT("Preview component exists"), Preview)) return false;
    Preview->Initialize(Runtime);
    Preview->RebuildPreviewObjects();

    for (const FGridLooseItemInstance& Placed : Level->LooseItemInstances)
    {
        if (!TestNotNull(TEXT("Preview item definition"), Placed.ItemDefinition.Get())) continue;
        const FGridResolvedWorldItemMesh Resolved = FGridWorldItemVisualResolver::Resolve(*Placed.ItemDefinition);
        if (!TestNotNull(TEXT("Resolved real 3D world mesh"), Resolved.Mesh)) continue;
        AGridEditorPreviewObjectActor* PreviewActor = nullptr;
        for (TActorIterator<AGridEditorPreviewObjectActor> It(Fixture.World); It; ++It)
        {
            if ((*It)->ObjectId == Placed.InstanceId)
            {
                PreviewActor = *It;
                break;
            }
        }
        if (!TestNotNull(TEXT("Actual preview actor"), PreviewActor)) continue;
        TestEqual(TEXT("Preview shows the catalogue mesh"),
            PreviewActor->MeshComponent->GetStaticMesh().Get(), Resolved.Mesh);
        TestTrue(TEXT("Preview keeps the visual transform"),
            PreviewActor->MeshComponent->GetRelativeTransform().Equals(Resolved.RelativeTransform));
    }
    Preview->ClearPreviewObjects();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01103RealInventory,
    "Grimrock.RPG.ARMEMENT01.10.3.RealInventoryEquipUnequip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01103RealInventory::RunTest(const FString&)
{
    for (const ARMEMENT01103::FWeapon& Spec : ARMEMENT01103::Weapons)
    {
        UGridItemDefinitionAsset* Definition = ARMEMENT01103::LoadWeapon(Spec);
        if (!TestNotNull(TEXT("Inventory weapon asset"), Definition)) continue;
        UGridPartyInventoryComponent* Inventory = ARMEMENT01103::MakeInventory();
        if (!TestTrue(TEXT("Register real definition"), Inventory->RegisterItemDefinition(Definition)) ||
            !TestTrue(TEXT("Default test character exists"), Inventory->IsValidCharacterIndex(0))) continue;
        FGridItemInstance Instance;
        if (!TestTrue(TEXT("Mint real level-one instance"), ARMEMENT01103::Mint(Inventory, Definition, Instance))) continue;
        if (!TestTrue(TEXT("Add real weapon to inventory"), Inventory->AddItemToCharacterInventory(0, Instance))) continue;
        const int32 Index = ARMEMENT01103::BagSlot(Inventory, Instance.RuntimeObjectId);
        if (!TestTrue(TEXT("Item is physically in the bag"), Index != INDEX_NONE)) continue;
        if (!TestTrue(TEXT("Equip real weapon from the bag"),
            Inventory->EquipItemFromInventorySlot(0, Index, EGridEquipmentSlot::MainHand))) continue;
        TestEqual(TEXT("Physical item remains unique"), ARMEMENT01103::PhysicalCount(Inventory, Instance.RuntimeObjectId), 1);
        const FGridCharacterEquipmentState& Equipment = Inventory->PartyInventoryState.ActiveEquipment[0];
        TestEqual(TEXT("MainHand keeps the same runtime GUID"), Equipment.MainHand.RuntimeObjectId, Instance.RuntimeObjectId);
        const bool bTwoHanded = Definition->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded;
        TestEqual(TEXT("Equipment occupancy matches hand usage"), Inventory->IsTwoHandedEquipped(0), bTwoHanded);
        if (bTwoHanded)
        {
            TestFalse(TEXT("No duplicate serialized OffHand instance"), Equipment.OffHand.IsValid());
            TestTrue(TEXT("OffHand virtually reserved"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
            FGridItemInstance VirtualOff;
            TestTrue(TEXT("OffHand UI resolves same physical item"),
                Inventory->GetEquippedItem(0, EGridEquipmentSlot::OffHand, VirtualOff));
            TestEqual(TEXT("Virtual hand keeps one GUID"), VirtualOff.RuntimeObjectId, Instance.RuntimeObjectId);
        }
        if (!TestTrue(TEXT("Unequip from real or virtual hand"),
            Inventory->UnequipItemToInventory(0, bTwoHanded ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand)))
            continue;
        TestFalse(TEXT("MainHand is now free"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));
        TestFalse(TEXT("OffHand is now free"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
        TestTrue(TEXT("Same item returned to the bag"), ARMEMENT01103::BagSlot(Inventory, Instance.RuntimeObjectId) != INDEX_NONE);
        TestEqual(TEXT("No duplication or loss on equip/unequip"), ARMEMENT01103::PhysicalCount(Inventory, Instance.RuntimeObjectId), 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01103GreatAxeTwoHands,
    "Grimrock.RPG.ARMEMENT01.10.3.GreatAxeTwoHandedTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01103GreatAxeTwoHands::RunTest(const FString&)
{
    UGridItemDefinitionAsset* Sword = ARMEMENT01103::LoadWeapon(ARMEMENT01103::Weapons[0]);
    UGridItemDefinitionAsset* GreatAxe = ARMEMENT01103::LoadWeapon(ARMEMENT01103::Weapons[3]);
    UGridItemDefinitionAsset* Shield = LoadObject<UGridItemDefinitionAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
    if (!TestNotNull(TEXT("Real straight sword"), Sword) ||
        !TestNotNull(TEXT("Real great axe"), GreatAxe) ||
        !TestNotNull(TEXT("Real round shield"), Shield)) return false;

    UGridPartyInventoryComponent* Inventory = ARMEMENT01103::MakeInventory();
    for (UGridItemDefinitionAsset* Definition : { Sword, GreatAxe, Shield })
    {
        if (!TestTrue(TEXT("Register real equipment definition"), Inventory->RegisterItemDefinition(Definition))) return false;
    }
    FGridItemInstance SwordInstance, AxeInstance, ShieldInstance;
    if (!TestTrue(TEXT("Mint sword"), ARMEMENT01103::Mint(Inventory, Sword, SwordInstance)) ||
        !TestTrue(TEXT("Mint great axe"), ARMEMENT01103::Mint(Inventory, GreatAxe, AxeInstance)) ||
        !TestTrue(TEXT("Mint shield"), ARMEMENT01103::Mint(Inventory, Shield, ShieldInstance))) return false;

    if (!TestTrue(TEXT("Add sword"), Inventory->AddItemToCharacterInventory(0, SwordInstance)) ||
        !TestTrue(TEXT("Add shield"), Inventory->AddItemToCharacterInventory(0, ShieldInstance)) ||
        !TestTrue(TEXT("Add axe"), Inventory->AddItemToCharacterInventory(0, AxeInstance))) return false;
    const int32 SwordSlot = ARMEMENT01103::BagSlot(Inventory, SwordInstance.RuntimeObjectId);
    if (!TestTrue(TEXT("Equip sword MainHand"), SwordSlot != INDEX_NONE &&
        Inventory->EquipItemFromInventorySlot(0, SwordSlot, EGridEquipmentSlot::MainHand))) return false;
    const int32 ShieldSlot = ARMEMENT01103::BagSlot(Inventory, ShieldInstance.RuntimeObjectId);
    if (!TestTrue(TEXT("Equip shield OffHand"), ShieldSlot != INDEX_NONE &&
        Inventory->EquipItemFromInventorySlot(0, ShieldSlot, EGridEquipmentSlot::OffHand))) return false;

    const int32 AxeSlot = ARMEMENT01103::BagSlot(Inventory, AxeInstance.RuntimeObjectId);
    if (!TestTrue(TEXT("Equip Great Axe and release both previous hand items"), AxeSlot != INDEX_NONE &&
        Inventory->EquipItemFromInventorySlot(0, AxeSlot, EGridEquipmentSlot::MainHand))) return false;
    TestTrue(TEXT("Great Axe is two-handed"), Inventory->IsTwoHandedEquipped(0));
    const FGridCharacterEquipmentState& Equipment = Inventory->PartyInventoryState.ActiveEquipment[0];
    TestEqual(TEXT("Great Axe physically in MainHand"), Equipment.MainHand.RuntimeObjectId, AxeInstance.RuntimeObjectId);
    TestFalse(TEXT("No physical OffHand duplicate"), Equipment.OffHand.IsValid());
    TestTrue(TEXT("OffHand is reserved"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
    TestTrue(TEXT("Sword returned to inventory"), ARMEMENT01103::BagSlot(Inventory, SwordInstance.RuntimeObjectId) != INDEX_NONE);
    TestTrue(TEXT("Shield returned to inventory"), ARMEMENT01103::BagSlot(Inventory, ShieldInstance.RuntimeObjectId) != INDEX_NONE);
    TestEqual(TEXT("Sword GUID count exactly one"), ARMEMENT01103::PhysicalCount(Inventory, SwordInstance.RuntimeObjectId), 1);
    TestEqual(TEXT("Shield GUID count exactly one"), ARMEMENT01103::PhysicalCount(Inventory, ShieldInstance.RuntimeObjectId), 1);
    TestEqual(TEXT("Axe GUID count exactly one"), ARMEMENT01103::PhysicalCount(Inventory, AxeInstance.RuntimeObjectId), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01103CombatProfiles,
    "Grimrock.RPG.ARMEMENT01.10.3.RealCatalogueCombatProfiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01103CombatProfiles::RunTest(const FString&)
{
    for (const ARMEMENT01103::FWeapon& Spec : ARMEMENT01103::Weapons)
    {
        UGridItemDefinitionAsset* Item = ARMEMENT01103::LoadWeapon(Spec);
        if (!TestNotNull(TEXT("Real combat item"), Item)) continue;
        const UGridWeaponCatalogueAsset* Catalogue = Item->WeaponCatalogue.LoadSynchronous();
        if (!TestNotNull(TEXT("Real RPG catalogue"), Catalogue)) continue;
        const FGridWeaponLevelProfile* Profile = Catalogue->FindProfile(FName(Spec.Archetype), 1);
        if (!TestNotNull(TEXT("Real fixed N1 offensive profile"), Profile)) continue;
        if (!TestTrue(TEXT("Weapon offers one valid equipment attack"),
            Item->CombatActions.Num() == 1 && Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand))) continue;
        const FGridCombatActionDefinition& Action = Item->CombatActions[0];
        TestTrue(TEXT("Authored action is valid"), Action.IsValid());
        TestEqual(TEXT("Stable attack ID"), Action.ActionId, FName(Spec.AttackId));
        TestEqual(TEXT("Catalogue AP cost"), Action.ActionPointCost, Profile->ActionPointCost);
        TestEqual(TEXT("Catalogue targeting range"), Action.RangeCells, Profile->RangeCells);

        UGridPartyInventoryComponent* Inventory = ARMEMENT01103::MakeInventory();
        if (!TestTrue(TEXT("Register combat definition"), Inventory->RegisterItemDefinition(Item))) continue;
        FGridItemInstance Instance;
        if (!TestTrue(TEXT("Mint real combat instance"), ARMEMENT01103::Mint(Inventory, Item, Instance))) continue;
        FGridOffensiveEquipmentProfile Offensive;
        if (!TestTrue(TEXT("Resolve equipped attack from runtime item level"),
            Item->ResolveEquippedOffensiveProfile(Instance, Action, Offensive))) continue;
        TestEqual(TEXT("Fixed level governs dice count"),
            Offensive.AttackDefinition.DamageDice.DiceCount, Profile->DamageDice.DiceCount);
        TestEqual(TEXT("Fixed level governs dice faces"),
            Offensive.AttackDefinition.DamageDice.DiceSides, Profile->DamageDice.DiceSides);
        TestEqual(TEXT("Fixed level governs flat dice bonus"),
            Offensive.AttackDefinition.DamageDice.FlatBonus, Profile->DamageDice.FlatBonus);
        TestEqual(TEXT("Catalogue governs attack accuracy"),
            Offensive.AttackDefinition.AccuracyBonus, Profile->AccuracyBonus);
        TestEqual(TEXT("Catalogue governs damage type"),
            Offensive.AttackDefinition.DamageType, Profile->DamageType);
        TestEqual(TEXT("Catalogue governs physical subtype"),
            Offensive.AttackDefinition.PhysicalSubtype, Profile->PhysicalSubtype);
        TestEqual(TEXT("Catalogue governs scaling attribute"),
            Offensive.DamageScalingAttribute, Profile->ScalingAttribute);
        TestEqual(TEXT("Catalogue governs actual attack range"), Offensive.RangeCells, Profile->RangeCells);

        FGridAttackSourceStats Source;
        FGridAttackTargetStats Target;
        Target.CurrentHealth = 10000;
        TArray<int32> MinimumDieRolls;
        MinimumDieRolls.Init(1, Profile->DamageDice.DiceCount);
        const FGridAttackResult DamageResult = FGridCombatResolver::ResolveAttackFromDiceRolls(
            Source, Target, Offensive.AttackDefinition, 20, MinimumDieRolls);
        TestTrue(TEXT("A legal maximum accuracy roll hits an unarmored target"), DamageResult.bHit);
        TestEqual(TEXT("Deterministic multi-dice trace agrees with N1 data"),
            DamageResult.Trace.Damage.Roll.FinalValue,
            Profile->DamageDice.DiceCount + Profile->DamageDice.FlatBonus);
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
