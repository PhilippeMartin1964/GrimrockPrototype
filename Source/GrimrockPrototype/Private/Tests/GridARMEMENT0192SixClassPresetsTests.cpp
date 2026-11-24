#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalWeightTestWorld.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "RPG/RPGStartingEquipmentService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT0192
{
    struct FClassSpec
    {
        const TCHAR* Name;
        const TCHAR* ApprovedDefaultKit;
        int32 ApprovedItemCount;
    };

    const FClassSpec Classes[] =
    {
        {TEXT("Warrior"), TEXT("Kit_Warrior_Guardian"), 2},
        {TEXT("Rogue"), TEXT("Kit_Rogue_Shadow"), 2},
        {TEXT("Ranger"), TEXT("Kit_Ranger_Marksman"), 3},
        {TEXT("Mage"), TEXT("Kit_Mage_Arcanist"), 2},
        {TEXT("Priest"), TEXT("Kit_Priest_Protection"), 2},
        {TEXT("Alchemist"), TEXT("Kit_Alchemist_Apothecary"), 2}
    };

    URPGClassAsset* LoadClass(const TCHAR* ClassName)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            ClassName, ClassName);
        return LoadObject<URPGClassAsset>(nullptr, *Path);
    }

    URPGRaceAsset* LoadHuman()
    {
        return LoadObject<URPGRaceAsset>(
            nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    }

    FGridItemInstance* FindAnyEquipped(UGridPartyInventoryComponent* Inventory, EGridEquipmentSlot Slot)
    {
        if (!Inventory || Inventory->PartyInventoryState.ActiveEquipment.IsEmpty())
        {
            return nullptr;
        }
        return Inventory->PartyInventoryState.ActiveEquipment[0].GetMutableSlot(Slot);
    }

    int32 CountOwnedItems(UGridPartyInventoryComponent* Inventory)
    {
        if (!Inventory || Inventory->PartyInventoryState.ActiveCharacters.IsEmpty()) return 0;
        int32 Count = 0;
        for (const FGridInventorySlot& Slot : Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots)
        {
            if (!Slot.IsEmpty()) ++Count;
        }
        const FGridCharacterEquipmentState& Gear = Inventory->PartyInventoryState.ActiveEquipment[0];
        const EGridEquipmentSlot Physical[] = {
            EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand,
            EGridEquipmentSlot::Head, EGridEquipmentSlot::Chest, EGridEquipmentSlot::Legs,
            EGridEquipmentSlot::Feet, EGridEquipmentSlot::Amulet, EGridEquipmentSlot::Ring1,
            EGridEquipmentSlot::Ring2, EGridEquipmentSlot::Shoulders, EGridEquipmentSlot::Gloves,
            EGridEquipmentSlot::Belt, EGridEquipmentSlot::Cloak, EGridEquipmentSlot::Talisman,
            EGridEquipmentSlot::QuickSlot1, EGridEquipmentSlot::QuickSlot2,
            EGridEquipmentSlot::Shirt, EGridEquipmentSlot::Bracers
        };
        for (const EGridEquipmentSlot Slot : Physical)
        {
            const FGridItemInstance* Item = Gear.GetSlot(Slot);
            if (Item && Item->IsValid()) ++Count;
        }
        return Count;
    }

    URPGClassAsset* DuplicateForPresets(URPGClassAsset* Original)
    {
        return Original ? DuplicateObject<URPGClassAsset>(Original, GetTransientPackage()) : nullptr;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0192CatalogTest,
    "Grimrock.RPG.ARMEMENT01.9.SixClassKits.CatalogueAndOverride",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0192CatalogTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Actual native ItemSystem is configured"), Fixture.bReady))
        return false;
    for (const ARMEMENT0192::FClassSpec& Spec : ARMEMENT0192::Classes)
    {
        URPGClassAsset* Original = ARMEMENT0192::LoadClass(Spec.Name);
        if (!TestNotNull(FString::Printf(TEXT("%s production class exists"), Spec.Name), Original))
            return false;
        TestTrue(FString::Printf(TEXT("%s is a supported production class"), Spec.Name),
            FRPGStartingEquipmentPresetCatalog::IsProductionClassAsset(Original));
        const int32 OriginalCount = Original->StartingEquipmentKits.Num();
        const FName OriginalDefault = Original->DefaultStartingEquipmentKitId;

        URPGClassAsset* Preview = ARMEMENT0192::DuplicateForPresets(Original);
        if (!TestNotNull(TEXT("Transient class copy created"), Preview)) return false;
        FString Error;
        if (!TestTrue(TEXT("Three approved native recipes build without old DA_Items"),
            FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
                *Preview, Error, Fixture.Registry.Get())))
        {
            AddError(Error);
            return false;
        }
        if (!TestEqual(TEXT("Exactly three active kits per class"),
            Preview->StartingEquipmentKits.Num(), 3))
            return false;
        TestEqual(TEXT("Approved deterministic default preserved"),
            Preview->DefaultStartingEquipmentKitId, FName(Spec.ApprovedDefaultKit));
        TSet<FName> KitIds;
        for (const FRPGStartingEquipmentKit& Kit : Preview->StartingEquipmentKits)
        {
            TestFalse(TEXT("One non-empty canonical kit id"), Kit.KitId.IsNone());
            TestFalse(TEXT("No duplicate native kit ids per class"), KitIds.Contains(Kit.KitId));
            KitIds.Add(Kit.KitId);
            TestTrue(TEXT("Approved kit has physical starter entries"), !Kit.Items.IsEmpty());
            for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
            {
                TestFalse(TEXT("Canonical source ItemId is explicit"),
                    Entry.ItemId.IsNone());
                FGridResolvedItem Native;
                if (!TestTrue(TEXT("Registry resolves each exact source and level"),
                    Fixture.Resolve(Entry.ItemId, Entry.ItemLevel, Native)))
                    return false;
                TestTrue(TEXT("Stack count is within registered limits"),
                    Entry.Quantity >= 1 && Entry.Quantity <= Native.MaxStack);
                TestTrue(TEXT("Equipped canonical starter respects the hand policy"),
                    Entry.TargetSlot == EGridEquipmentSlot::None ||
                    (Native.bIsArmament && Native.Weapon.CanEquipInHand(Entry.TargetSlot)));
            }
        }
        TestTrue(TEXT("Native class preview remains valid"), Preview->IsValidDefinition());
        TestEqual(TEXT("Production kits remain untouched"),
            Original->StartingEquipmentKits.Num(), OriginalCount);
        TestEqual(TEXT("Production default remains untouched"),
            Original->DefaultStartingEquipmentKitId, OriginalDefault);

        // Explicit authoring wins; idempotence applies to all three native kits.
        if (!TestTrue(TEXT("Repeated canonical materialization is idempotent"),
            FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
                *Preview, Error, Fixture.Registry.Get())))
            return false;
        TestEqual(TEXT("No duplicated approved options"),
            Preview->StartingEquipmentKits.Num(), 3);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0192SixClassCreationTest,
    "Grimrock.RPG.ARMEMENT01.9.SixClassKits.ProductionNewGameGrants",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0192SixClassCreationTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld NativeWorld({});
    if (!TestNotNull(TEXT("World for six canonical NewGame classes"), NativeWorld.World) ||
        !TestNotNull(TEXT("Registered 48-item NewGame source"), NativeWorld.Scope.Registry)) return false;
    URPGRaceAsset* Human = ARMEMENT0192::LoadHuman();
    if (!TestNotNull(TEXT("Production human race exists"), Human)) return false;
    for (const ARMEMENT0192::FClassSpec& Spec : ARMEMENT0192::Classes)
    {
        URPGClassAsset* Class = ARMEMENT0192::LoadClass(Spec.Name);
        if (!TestNotNull(TEXT("Production class loads"), Class)) return false;
        UGridPartyInventoryComponent* Inventory = NativeWorld.MakeInventory();
        if (!TestNotNull(TEXT("NewGame inventory owned by actual world"), Inventory)) return false;
        Inventory->InitializeDefaultPartyIfNeeded();

        FRPGCharacterCreationRequest Request;
        Request.DisplayName = FText::FromString(TEXT("Elias"));
        Request.RaceDefinition = Human;
        Request.ClassDefinition = Class;
        Request.CombatActionSourceClassDefinition = Class;
        FText Error;
        if (!TestTrue(FString::Printf(TEXT("%s creation succeeds, error: %s"), Spec.Name, *Error.ToString()),
            Inventory->CreateInitialCharacter(Request, Error))) return false;
        TestTrue(TEXT("Initial creation marked complete"), Inventory->HasCompletedInitialCharacterCreation());
        TestEqual(FString::Printf(TEXT("%s kit id persisted"), Spec.Name),
            Inventory->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId, FName(Spec.ApprovedDefaultKit));
        TestEqual(FString::Printf(TEXT("%s owns approved starter instances"), Spec.Name),
            ARMEMENT0192::CountOwnedItems(Inventory), Spec.ApprovedItemCount);
        FString OwnershipError;
        TestTrue(TEXT("Exclusive ownership retained"), Inventory->ValidateInventoryOwnership(OwnershipError));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0192RangerTest,
    "Grimrock.RPG.ARMEMENT01.9.SixClassKits.RangerHandsAndAlternative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0192RangerTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld NativeWorld({});
    if (!TestNotNull(TEXT("Real Ranger test world"), NativeWorld.World) ||
        !TestNotNull(TEXT("World-bound Ranger ItemSystem"), NativeWorld.Scope.Registry)) return false;
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Native Ranger kit registry available"), Fixture.bReady)) return false;
    URPGClassAsset* Ranger = ARMEMENT0192::LoadClass(TEXT("Ranger"));
    URPGRaceAsset* Human = ARMEMENT0192::LoadHuman();
    if (!TestNotNull(TEXT("Production Ranger exists"), Ranger) || !TestNotNull(TEXT("Production race exists"), Human)) return false;

    UGridPartyInventoryComponent* Inventory = NativeWorld.MakeInventory();
    if (!TestNotNull(TEXT("Native Ranger party inventory"), Inventory)) return false;
    Inventory->InitializeDefaultPartyIfNeeded();
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString(TEXT("Archer"));
    Request.RaceDefinition = Human;
    Request.ClassDefinition = Ranger;
    Request.CombatActionSourceClassDefinition = Ranger;
    FText Error;
    if (!TestTrue(TEXT("Ranger creation succeeds"), Inventory->CreateInitialCharacter(Request, Error))) return false;
    TestTrue(TEXT("Starting archer kit reserves both hands"), Inventory->IsTwoHandedEquipped(0));
    const FGridItemInstance* Main = ARMEMENT0192::FindAnyEquipped(Inventory, EGridEquipmentSlot::MainHand);
    const FGridItemInstance* Off = ARMEMENT0192::FindAnyEquipped(Inventory, EGridEquipmentSlot::OffHand);
    if (!TestNotNull(TEXT("Physical bow present"), Main)) return false;
    TestEqual(TEXT("Bow is the production ARM045 item"), Main->ItemDefinitionId, FName(TEXT("Weapon_ShortBow_01")));
    TestTrue(TEXT("MainHand owns the item instance"), Main->IsValid());
    TestTrue(TEXT("Physical offhand remains empty"), Off && !Off->IsValid());
    TestEqual(TEXT("No duplicated virtual bow"), ARMEMENT0192::CountOwnedItems(Inventory), 3);

    URPGClassAsset* Preview = ARMEMENT0192::DuplicateForPresets(Ranger);
    FString Failure;
    if (!TestTrue(TEXT("Ranger alternate kits available"),
        Preview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(*Preview, Failure, Fixture.Registry.Get()))) return false;
    const FRPGStartingEquipmentKit* Trail = Preview->FindStartingEquipmentKit(TEXT("Kit_Ranger_Hunter"));
    if (!TestNotNull(TEXT("Approved Hunter kit can be selected in wizard"), Trail)) return false;

    FGridCharacterInventoryState Candidate;
    Candidate.CharacterId = FGuid::NewGuid();
    Candidate.InventorySlots.SetNum(40);
    FGridCharacterEquipmentState CandidateEquipment;

    if (!TestTrue(TEXT("Alternative kit builds independently"),
        FRPGStartingEquipmentService::TryBuildKit(Preview, Trail->KitId,
            Candidate, CandidateEquipment, Failure, Fixture.Registry.Get()))) return false;
    TestEqual(TEXT("Alternative kit records selected ID"), Candidate.StartingEquipmentKitId, Trail->KitId);
    TestTrue(TEXT("Alternative kit leaves OffHand available"), !CandidateEquipment.OffHand.IsValid());
    TestTrue(TEXT("Alternative kit has one-handed weapon"), CandidateEquipment.MainHand.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0192LegacyTest,
    "Grimrock.RPG.ARMEMENT01.9.SixClassKits.NonProductionUnchanged",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0192LegacyTest::RunTest(const FString&)
{
    // An unconfigured transient Warrior used by existing CC2 tests must not
    // suddenly acquire a production kit merely because ClassId matches.
    URPGClassAsset* Synthetic = NewObject<URPGClassAsset>();
    Synthetic->ClassId = TEXT("Warrior");
    Synthetic->DisplayName = FText::FromString(TEXT("Warrior"));
    Synthetic->BaseAttributes = FRPGAttributes{15, 11, 13, 9, 9, 9};
    Synthetic->HealthAtLevelOne = 18;
    Synthetic->HealthPerLevel = 8;
    TestFalse(TEXT("Synthetic class is not a production asset"),
        FRPGStartingEquipmentPresetCatalog::IsProductionClassAsset(Synthetic));
    URPGRaceAsset* Human = ARMEMENT0192::LoadHuman();
    if (!TestNotNull(TEXT("Human production race exists"), Human)) return false;

    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
    Inventory->InitializeDefaultPartyIfNeeded();
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString(TEXT("Legacy"));
    Request.RaceDefinition = Human;
    Request.ClassDefinition = Synthetic;
    FText Error;
    if (!TestTrue(TEXT("Legacy synthetic character creation succeeds"), Inventory->CreateInitialCharacter(Request, Error))) return false;
    TestTrue(TEXT("Synthetic class has no invented default kit"),
        Inventory->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId.IsNone());
    TestEqual(TEXT("No implicit items for legacy classes"), ARMEMENT0192::CountOwnedItems(Inventory), 0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
