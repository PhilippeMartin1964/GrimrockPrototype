#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
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
        const TCHAR* DefaultKit;
        const TCHAR* ApprovedDefaultKit;
        int32 ApprovedItemCount;
    };

    const FClassSpec Classes[] =
    {
        {TEXT("Warrior"), TEXT("Warrior_Defender"), TEXT("Kit_Warrior_Guardian"), 2},
        {TEXT("Rogue"), TEXT("Rogue_Shadow"), TEXT("Kit_Rogue_Shadow"), 2},
        {TEXT("Ranger"), TEXT("Ranger_Archer"), TEXT("Kit_Ranger_Marksman"), 3},
        {TEXT("Mage"), TEXT("Mage_Scholar"), TEXT("Kit_Mage_Arcanist"), 2},
        {TEXT("Priest"), TEXT("Priest_Pilgrim"), TEXT("Kit_Priest_Protection"), 2},
        {TEXT("Alchemist"), TEXT("Alchemist_Apothecary"), TEXT("Kit_Alchemist_Apothecary"), 2}
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
    for (const ARMEMENT0192::FClassSpec& Spec : ARMEMENT0192::Classes)
    {
        URPGClassAsset* Original = ARMEMENT0192::LoadClass(Spec.Name);
        if (!TestNotNull(FString::Printf(TEXT("%s production class exists"), Spec.Name), Original)) return false;
        TestTrue(FString::Printf(TEXT("%s is recognized as production"), Spec.Name),
            FRPGStartingEquipmentPresetCatalog::IsProductionClassAsset(Original));
        const int32 OriginalCount = Original->StartingEquipmentKits.Num();
        const FName OriginalDefault = Original->DefaultStartingEquipmentKitId;

        URPGClassAsset* Preview = ARMEMENT0192::DuplicateForPresets(Original);
        if (!TestNotNull(TEXT("Transient class copy created"), Preview)) return false;
        FString Error;
        if (!TestTrue(FString::Printf(TEXT("%s starter recipes built: %s"), Spec.Name, *Error),
            FRPGStartingEquipmentPresetCatalog::TryPopulateTransientClass(*Preview, Error))) return false;
        if (!TestEqual(FString::Printf(TEXT("%s has two kits"), Spec.Name), Preview->StartingEquipmentKits.Num(), 2)) return false;
        TestEqual(FString::Printf(TEXT("%s default kit is deterministic"), Spec.Name),
            Preview->DefaultStartingEquipmentKitId, FName(Spec.DefaultKit));
        TSet<FName> KitIds;
        for (const FRPGStartingEquipmentKit& Kit : Preview->StartingEquipmentKits)
        {
            TestFalse(TEXT("Kit id not empty"), Kit.KitId.IsNone());
            TestFalse(TEXT("Kit ids unique"), KitIds.Contains(Kit.KitId));
            KitIds.Add(Kit.KitId);
            TestEqual(TEXT("Every kit defines two production items"), Kit.Items.Num(), 2);
            for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
            {
                if (!TestNotNull(TEXT("Item definition resolves"), Entry.ItemDefinition.Get())) return false;
                TestTrue(TEXT("Item definition has stable identity"), !Entry.ItemDefinition->ItemDefinitionId.IsNone());
                TestEqual(TEXT("Starting item uses default catalogue level"), Entry.ItemLevel, 0);
                TestEqual(TEXT("Starting quantity one"), Entry.Quantity, 1);
                TestTrue(TEXT("Equip compatibility or bag destination"),
                    Entry.TargetSlot == EGridEquipmentSlot::None || Entry.ItemDefinition->CanEquipToSlot(Entry.TargetSlot));
            }
        }
        TestTrue(TEXT("Built class remains valid"), Preview->IsValidDefinition());
        TestEqual(TEXT("Production class kits left untouched"), Original->StartingEquipmentKits.Num(), OriginalCount);
        TestEqual(TEXT("Production default left untouched"), Original->DefaultStartingEquipmentKitId, OriginalDefault);

        // Explicit authoring wins; a second call must be strictly idempotent.
        if (!TestTrue(TEXT("Second materialization preserves the authored kits"),
            FRPGStartingEquipmentPresetCatalog::TryPopulateTransientClass(*Preview, Error))) return false;
        TestEqual(TEXT("No duplicated kits"), Preview->StartingEquipmentKits.Num(), 2);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0192SixClassCreationTest,
    "Grimrock.RPG.ARMEMENT01.9.SixClassKits.ProductionNewGameGrants",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0192SixClassCreationTest::RunTest(const FString&)
{
    URPGRaceAsset* Human = ARMEMENT0192::LoadHuman();
    if (!TestNotNull(TEXT("Production human race exists"), Human)) return false;
    for (const ARMEMENT0192::FClassSpec& Spec : ARMEMENT0192::Classes)
    {
        URPGClassAsset* Class = ARMEMENT0192::LoadClass(Spec.Name);
        if (!TestNotNull(TEXT("Production class loads"), Class)) return false;
        UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
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
    URPGClassAsset* Ranger = ARMEMENT0192::LoadClass(TEXT("Ranger"));
    URPGRaceAsset* Human = ARMEMENT0192::LoadHuman();
    if (!TestNotNull(TEXT("Production Ranger exists"), Ranger) || !TestNotNull(TEXT("Production race exists"), Human)) return false;

    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
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
        Preview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(*Preview, Failure))) return false;
    const FRPGStartingEquipmentKit* Trail = Preview->FindStartingEquipmentKit(TEXT("Kit_Ranger_Hunter"));
    if (!TestNotNull(TEXT("Approved Hunter kit can be selected in wizard"), Trail)) return false;

    FGridCharacterInventoryState Candidate;
    Candidate.CharacterId = FGuid::NewGuid();
    Candidate.InventorySlots.SetNum(40);
    FGridCharacterEquipmentState CandidateEquipment;
    TArray<UGridItemDefinitionAsset*> Definitions;
    if (!TestTrue(TEXT("Alternative kit builds independently"),
        FRPGStartingEquipmentService::TryBuildKit(Preview, Trail->KitId,
            Candidate, CandidateEquipment, Definitions, Failure))) return false;
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
