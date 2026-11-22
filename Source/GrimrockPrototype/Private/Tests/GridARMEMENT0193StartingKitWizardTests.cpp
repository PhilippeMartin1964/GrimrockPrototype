#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalWeightTestWorld.h"
#include "Blueprint/UserWidget.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGCharacterTypes.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UI/RPGCharacterCreationWizardWidget.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace ARMEMENT0193
{
    URPGClassAsset* Class(const TCHAR* Id)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"), Id, Id);
        return LoadObject<URPGClassAsset>(nullptr, *Path);
    }

    URPGRaceAsset* Human()
    {
        return LoadObject<URPGRaceAsset>(
            nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    }

    UGridPartyInventoryComponent* NewParty(FGridCanonicalWeightTestWorld& Fixture)
    {
        UGridPartyInventoryComponent* Party = Fixture.MakeInventory();
        Party->InitializeDefaultPartyIfNeeded();
        return Party;
    }

    FRPGCharacterCreationRequest Request(URPGClassAsset* Definition, FName KitId)
    {
        FRPGCharacterCreationRequest Result;
        Result.DisplayName = FText::FromString(TEXT("Elias"));
        Result.RaceDefinition = Human();
        Result.ClassDefinition = Definition;
        Result.CombatActionSourceClassDefinition = Definition;
        Result.StartingEquipmentKitId = KitId;
        return Result;
    }

    int32 CountPhysicalObjects(const UGridPartyInventoryComponent* Party)
    {
        int32 Total = 0;
        const FGridCharacterInventoryState& Char = Party->PartyInventoryState.ActiveCharacters[0];
        for (const FGridInventorySlot& Slot : Char.InventorySlots)
        {
            if (!Slot.IsEmpty()) ++Total;
        }
        const FGridCharacterEquipmentState& Equipped = Party->PartyInventoryState.ActiveEquipment[0];
        const EGridEquipmentSlot Slots[] = {
            EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand,
            EGridEquipmentSlot::Head, EGridEquipmentSlot::Chest, EGridEquipmentSlot::Legs,
            EGridEquipmentSlot::Feet, EGridEquipmentSlot::Amulet,
            EGridEquipmentSlot::Ring1, EGridEquipmentSlot::Ring2,
            EGridEquipmentSlot::Shoulders, EGridEquipmentSlot::Gloves, EGridEquipmentSlot::Belt,
            EGridEquipmentSlot::Cloak, EGridEquipmentSlot::Talisman,
            EGridEquipmentSlot::QuickSlot1, EGridEquipmentSlot::QuickSlot2,
            EGridEquipmentSlot::Shirt, EGridEquipmentSlot::Bracers
        };
        for (const EGridEquipmentSlot Slot : Slots)
        {
            const FGridItemInstance* Item = Equipped.GetSlot(Slot);
            if (Item && Item->IsValid()) ++Total;
        }
        return Total;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0193WizardReadModelTest,
    "Grimrock.RPG.ARMEMENT01.9.WizardSelection.ReadModelAndClassSwitch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0193WizardReadModelTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    if (!TestNotNull(TEXT("Canonical NewGame GameInstance world"), Fixture.World) ||
        !TestNotNull(TEXT("Configured canonical ItemSystem"), Fixture.Scope.Registry))
        return false;
    URPGClassAsset* Ranger = ARMEMENT0193::Class(TEXT("Ranger"));
    if (!TestNotNull(TEXT("Production Ranger exists"), Ranger))
        return false;
    URPGClassAsset* RangerPreview =
        DuplicateObject<URPGClassAsset>(Ranger, GetTransientPackage());
    FString Error;
    if (!TestTrue(TEXT("Canonical Ranger preview materializes"),
        RangerPreview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
            *RangerPreview, Error, Fixture.Scope.Registry)))
    {
        AddError(Error);
        return false;
    }
    URPGCharacterCreationWizardWidget* Wizard =
        CreateWidget<URPGCharacterCreationWizardWidget>(
            Fixture.World, URPGCharacterCreationWizardWidget::StaticClass());
    if (!TestNotNull(TEXT("World-bound wizard exists"), Wizard))
        return false;
    Wizard->ClassDefinition = RangerPreview;
    const TArray<FRPGStartingEquipmentKit> Kits =
        Wizard->GetAvailableStartingEquipmentKits();
    if (!TestEqual(TEXT("Ranger presents three approved starting kits"), Kits.Num(), 3))
        return false;
    TestEqual(TEXT("Ranger default kit selected"),
        Wizard->GetSelectedStartingEquipmentKitId(),
        FName(TEXT("Kit_Ranger_Marksman")));
    TestFalse(TEXT("Unknown cross-class ID refused without selecting"),
        Wizard->SelectStartingEquipmentKit(TEXT("Kit_Warrior_Guardian")));
    TestEqual(TEXT("Rejected ID does not change Ranger default"),
        Wizard->GetSelectedStartingEquipmentKitId(),
        FName(TEXT("Kit_Ranger_Marksman")));
    return !HasAnyErrors();
}

// Dedicated test names allow the isolated Windows agent to report whether
// a failure is in the UI's canonical text read model or class-switch behavior.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0193WizardHunterDescriptionTest,
    "Grimrock.RPG.ARMEMENT01.9.WizardSelection.HunterCanonicalDescription",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0193WizardHunterDescriptionTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    if (!TestNotNull(TEXT("Canonical game world"), Fixture.World) ||
        !TestNotNull(TEXT("Configured game-world ItemSystem"), Fixture.Scope.Registry))
        return false;
    URPGClassAsset* Ranger = ARMEMENT0193::Class(TEXT("Ranger"));
    if (!TestNotNull(TEXT("Actual production Ranger"), Ranger)) return false;
    URPGClassAsset* Preview =
        DuplicateObject<URPGClassAsset>(Ranger, GetTransientPackage());
    FString Error;
    if (!TestTrue(TEXT("Hunter source recipes are canonical"),
        Preview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
            *Preview, Error, Fixture.Scope.Registry)))
    {
        AddError(Error);
        return false;
    }
    URPGCharacterCreationWizardWidget* Wizard =
        CreateWidget<URPGCharacterCreationWizardWidget>(
            Fixture.World, URPGCharacterCreationWizardWidget::StaticClass());
    if (!TestNotNull(TEXT("World-bound NewGame widget"), Wizard))
        return false;
    Wizard->ClassDefinition = Preview;
    const TArray<FRPGStartingEquipmentKit> Kits =
        Wizard->GetAvailableStartingEquipmentKits();
    if (!TestEqual(TEXT("Three Ranger kits"), Kits.Num(), 3)) return false;
    if (!TestTrue(TEXT("Ranger Hunter alternative can be selected"),
        Wizard->SelectStartingEquipmentKit(TEXT("Kit_Ranger_Hunter"))))
        return false;
    TestEqual(TEXT("Selected Hunter kit ID retained"),
        Wizard->GetSelectedStartingEquipmentKitId(),
        FName(TEXT("Kit_Ranger_Hunter")));
    const FRPGStartingEquipmentKit* Hunter = Kits.FindByPredicate(
        [](const FRPGStartingEquipmentKit& Kit)
        { return Kit.KitId == FName(TEXT("Kit_Ranger_Hunter")); });
    if (!TestNotNull(TEXT("Hunter recipe exists"), Hunter) ||
        !TestTrue(TEXT("Hunter recipe has physical items"), !Hunter->Items.IsEmpty()))
        return false;
    TestNull(TEXT("Hunter item has no historic ItemDefinition"),
        Hunter->Items[0].ItemDefinition.Get());
    FGridResolvedItem Item;
    if (!TestTrue(TEXT("Hunter spear N1 resolves canonically"),
        Fixture.Scope.Registry->ResolveItem(
            Hunter->Items[0].CanonicalItemId, Hunter->Items[0].ItemLevel, Item)))
        return false;
    const FString AuthoredName = Item.DisplayName.IsEmpty()
        ? Item.ItemId.ToString() : Item.DisplayName.ToString();
    TestTrue(TEXT("Real selected-kit description contains canonical authored item name"),
        Wizard->GetSelectedStartingEquipmentKitDescription().ToString().Contains(AuthoredName));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0193WizardClassSwitchTest,
    "Grimrock.RPG.ARMEMENT01.9.WizardSelection.CrossClassReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0193WizardClassSwitchTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    if (!TestNotNull(TEXT("World for class switch"), Fixture.World) ||
        !TestNotNull(TEXT("Canonical registry for class switch"), Fixture.Scope.Registry))
        return false;
    URPGClassAsset* Ranger = ARMEMENT0193::Class(TEXT("Ranger"));
    URPGClassAsset* Warrior = ARMEMENT0193::Class(TEXT("Warrior"));
    if (!TestNotNull(TEXT("Ranger class exists"), Ranger) ||
        !TestNotNull(TEXT("Warrior class exists"), Warrior))
        return false;
    URPGClassAsset* RangerPreview =
        DuplicateObject<URPGClassAsset>(Ranger, GetTransientPackage());
    URPGClassAsset* WarriorPreview =
        DuplicateObject<URPGClassAsset>(Warrior, GetTransientPackage());
    FString Error;
    if (!TestTrue(TEXT("Ranger native recipes loaded"),
        RangerPreview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
            *RangerPreview, Error, Fixture.Scope.Registry)) ||
        !TestTrue(TEXT("Warrior native recipes loaded"),
        WarriorPreview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
            *WarriorPreview, Error, Fixture.Scope.Registry)))
    {
        AddError(Error);
        return false;
    }
    URPGCharacterCreationWizardWidget* Wizard =
        CreateWidget<URPGCharacterCreationWizardWidget>(
            Fixture.World, URPGCharacterCreationWizardWidget::StaticClass());
    if (!TestNotNull(TEXT("World-bound wizard for class switch"), Wizard))
        return false;
    Wizard->ClassDefinition = RangerPreview;
    if (!TestTrue(TEXT("Hunter alternative is selectable"),
        Wizard->SelectStartingEquipmentKit(TEXT("Kit_Ranger_Hunter"))))
        return false;
    TestEqual(TEXT("Hunter alternative was selected"),
        Wizard->GetSelectedStartingEquipmentKitId(), FName(TEXT("Kit_Ranger_Hunter")));

    Wizard->ClassDefinition = WarriorPreview;
    Wizard->RefreshPreview();
    TestEqual(TEXT("Switching to Warrior resets to Guardian default"),
        Wizard->GetSelectedStartingEquipmentKitId(), FName(TEXT("Kit_Warrior_Guardian")));
    TestFalse(TEXT("Old Ranger Hunter kit cannot be chosen for Warrior"),
        Wizard->SelectStartingEquipmentKit(TEXT("Kit_Ranger_Hunter")));
    TestEqual(TEXT("Three Warrior starter kits remain available"),
        Wizard->GetAvailableStartingEquipmentKits().Num(), 3);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0193RequestTest,
    "Grimrock.RPG.ARMEMENT01.9.WizardSelection.AlternativeKitNewGame",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0193RequestTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    if (!TestNotNull(TEXT("Canonical NewGame GameInstance world"), Fixture.World) ||
        !TestNotNull(TEXT("Configured canonical ItemSystem"), Fixture.Scope.Registry))
        return false;
    URPGClassAsset* Ranger = ARMEMENT0193::Class(TEXT("Ranger"));
    if (!TestNotNull(TEXT("Ranger exists"), Ranger)) return false;
    UGridPartyInventoryComponent* Party = ARMEMENT0193::NewParty(Fixture);
    FText Error;
    const FRPGCharacterCreationRequest NewGame =
        ARMEMENT0193::Request(Ranger, TEXT("Kit_Ranger_Hunter"));
    if (!TestTrue(TEXT("Explicit alternative kit accepted"),
        Party->CreateInitialCharacter(NewGame, Error))) return false;
    TestEqual(TEXT("Alternative kit ID retained on character"),
        Party->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId,
        FName(TEXT("Kit_Ranger_Hunter")));
    TestEqual(TEXT("Alternative kit grants exactly two physical objects"),
        ARMEMENT0193::CountPhysicalObjects(Party), 2);
    const FGridItemInstance& Main = Party->PartyInventoryState.ActiveEquipment[0].MainHand;
    TestEqual(TEXT("Hunter equips the short spear"), Main.ItemDefinitionId,
        FName(TEXT("Weapon_ShortSpear_01")));
    TestFalse(TEXT("Alternative kit leaves OffHand available"),
        Party->PartyInventoryState.ActiveEquipment[0].OffHand.IsValid());
    FString OwnershipError;
    TestTrue(TEXT("Ownership invariants hold"), Party->ValidateInventoryOwnership(OwnershipError));

    UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
    Save->PartyInventoryState = Party->PartyInventoryState;
    TestEqual(TEXT("Kit selection is in the SaveGame party snapshot"),
        Save->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId,
        FName(TEXT("Kit_Ranger_Hunter")));
    const FProperty* SavedState = UGrimrockPartySaveGame::StaticClass()->FindPropertyByName(TEXT("PartyInventoryState"));
    TestTrue(TEXT("Inventory snapshot property has SaveGame flag"),
        SavedState && SavedState->HasAnyPropertyFlags(CPF_SaveGame));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0193InvalidSelectionTest,
    "Grimrock.RPG.ARMEMENT01.9.WizardSelection.InvalidKitRollback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0193InvalidSelectionTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    if (!TestNotNull(TEXT("Canonical NewGame GameInstance world"), Fixture.World) ||
        !TestNotNull(TEXT("Configured canonical ItemSystem"), Fixture.Scope.Registry))
        return false;
    URPGClassAsset* Ranger = ARMEMENT0193::Class(TEXT("Ranger"));
    if (!TestNotNull(TEXT("Ranger exists"), Ranger)) return false;
    UGridPartyInventoryComponent* Party = ARMEMENT0193::NewParty(Fixture);
    const FGuid PreviousId = Party->PartyInventoryState.ActiveCharacters[0].CharacterId;
    FText Error;
    const FRPGCharacterCreationRequest WrongClass =
        ARMEMENT0193::Request(Ranger, TEXT("Kit_Warrior_Guardian"));
    TestFalse(TEXT("Kit belonging to another class cannot be created"),
        Party->CreateInitialCharacter(WrongClass, Error));
    TestFalse(TEXT("Failure includes an error"), Error.IsEmpty());
    TestFalse(TEXT("Failure keeps character creation incomplete"),
        Party->HasCompletedInitialCharacterCreation());
    TestEqual(TEXT("Previous hero identity retained"),
        Party->PartyInventoryState.ActiveCharacters[0].CharacterId, PreviousId);
    TestNull(TEXT("No kit definition registered after failure"),
        Party->FindItemDefinition(TEXT("Weapon_Dagger_01")));

    Error = FText::GetEmpty();
    FRPGCharacterCreationRequest Missing = ARMEMENT0193::Request(Ranger, TEXT("Unknown_Kit"));
    TestFalse(TEXT("Unknown kit rejected atomically"), Party->CreateInitialCharacter(Missing, Error));
    TestFalse(TEXT("Unknown ID returns an error"), Error.IsEmpty());
    TestEqual(TEXT("No mutation to party"), Party->PartyInventoryState.ActiveCharacters[0].CharacterId, PreviousId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0193CompatibilityTest,
    "Grimrock.RPG.ARMEMENT01.9.WizardSelection.DefaultAndRecruitmentUnchanged",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0193CompatibilityTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    if (!TestNotNull(TEXT("Canonical NewGame GameInstance world"), Fixture.World) ||
        !TestNotNull(TEXT("Configured canonical ItemSystem"), Fixture.Scope.Registry))
        return false;
    URPGClassAsset* Warrior = ARMEMENT0193::Class(TEXT("Warrior"));
    if (!TestNotNull(TEXT("Warrior exists"), Warrior)) return false;
    UGridPartyInventoryComponent* Party = ARMEMENT0193::NewParty(Fixture);
    FText Error;
    if (!TestTrue(TEXT("Omitting the kit selection preserves default New Game"),
        Party->CreateInitialCharacter(ARMEMENT0193::Request(Warrior, NAME_None), Error))) return false;
    TestEqual(TEXT("Warrior default is now Guardian"),
        Party->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId,
        FName(TEXT("Kit_Warrior_Guardian")));

    URPGCharacterCreationWizardWidget* Recruit = CreateWidget<URPGCharacterCreationWizardWidget>(Fixture.World,
        URPGCharacterCreationWizardWidget::StaticClass());
    Recruit->ClassDefinition = Warrior;
    Recruit->CreationContext = ERPGCharacterCreationContext::CustomRecruit;
    TestTrue(TEXT("Custom recruitment does not introduce starting-kit options"),
        Recruit->GetAvailableStartingEquipmentKits().IsEmpty());
    TestTrue(TEXT("Recruitment has no selected starting kit"),
        Recruit->GetSelectedStartingEquipmentKitId().IsNone());
    TestFalse(TEXT("Recruitment cannot choose an initial-hero kit"),
        Recruit->SelectStartingEquipmentKit(TEXT("Kit_Warrior_Guardian")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0193KitPresentationTest,
    "Grimrock.RPG.ARMEMENT01.9.WizardSelection.PlayerFacingKitLabels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0193KitPresentationTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    if (!TestNotNull(TEXT("Canonical NewGame GameInstance world"), Fixture.World) ||
        !TestNotNull(TEXT("Configured canonical ItemSystem"), Fixture.Scope.Registry))
        return false;
    URPGCharacterCreationWizardWidget* Wizard = CreateWidget<URPGCharacterCreationWizardWidget>(Fixture.World,
        URPGCharacterCreationWizardWidget::StaticClass());
    Wizard->ClassDefinition = ARMEMENT0193::Class(TEXT("Warrior"));
    if (!TestNotNull(TEXT("Production Warrior exists"), Wizard->ClassDefinition.Get()))
    {
        return false;
    }

    const TArray<FRPGStartingEquipmentKit> Kits = Wizard->GetAvailableStartingEquipmentKits();
    if (!TestEqual(TEXT("Three approved kit labels"), Kits.Num(), 3))
    {
        return false;
    }
    for (const FRPGStartingEquipmentKit& Kit : Kits)
    {
        const FString Label = Wizard->GetStartingKitOptionLabel(Kit).ToString();
        const FString ExpectedLabel = Kit.DisplayName.IsEmpty()
            ? Kit.KitId.ToString() : Kit.DisplayName.ToString();
        TestEqual(TEXT("ComboBox label is the display name only"), Label, ExpectedLabel);
        TestFalse(TEXT("ComboBox does not append technical brackets"), Label.Contains(TEXT("[")));
        TestFalse(TEXT("ComboBox does not expose technical ID"), Label.Contains(Kit.KitId.ToString()));
    }

    if (!TestTrue(TEXT("Select third Warrior kit"), Wizard->SelectStartingEquipmentKit(TEXT("Kit_Warrior_WeaponMaster"))))
    {
        return false;
    }
    const FString Label = Wizard->GetStartingKitOptionLabel(Kits[2]).ToString();
    const FString Details = Wizard->GetSelectedStartingEquipmentKitDescription().ToString();
    const FString Summary = Wizard->GetSelectedStartingEquipmentKitSummary().ToString();

    TestFalse(TEXT("Class description does not repeat the displayed title"), Details.StartsWith(Label));
    TestTrue(TEXT("Class description starts with an item quantity"), Details.StartsWith(TEXT("1 × ")));
    TestTrue(TEXT("Technical ID appears in the details rather than the ComboBox"),
        Details.Contains(TEXT("Référence technique : Kit_Warrior_WeaponMaster")));
    TestTrue(TEXT("Details have both item rows"), Details.Contains(TEXT("\n1 × ")));
    TestTrue(TEXT("Summary retains kit name without a ComboBox"), Summary.StartsWith(Label + TEXT("\n")));
    TestTrue(TEXT("Summary retains technical reference"), Summary.Contains(TEXT("Kit_Warrior_WeaponMaster")));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
