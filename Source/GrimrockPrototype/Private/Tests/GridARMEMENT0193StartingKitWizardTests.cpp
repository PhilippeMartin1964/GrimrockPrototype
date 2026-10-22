#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
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

    UGridPartyInventoryComponent* NewParty()
    {
        UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
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
    URPGCharacterCreationWizardWidget* Wizard = NewObject<URPGCharacterCreationWizardWidget>();
    URPGClassAsset* Ranger = ARMEMENT0193::Class(TEXT("Ranger"));
    URPGClassAsset* Warrior = ARMEMENT0193::Class(TEXT("Warrior"));
    if (!TestNotNull(TEXT("Wizard object exists"), Wizard) ||
        !TestNotNull(TEXT("Production Ranger exists"), Ranger) ||
        !TestNotNull(TEXT("Production Warrior exists"), Warrior)) return false;

    Wizard->ClassDefinition = Ranger;
    const TArray<FRPGStartingEquipmentKit> Kits = Wizard->GetAvailableStartingEquipmentKits();
    if (!TestEqual(TEXT("Ranger presents two starting kits"), Kits.Num(), 2)) return false;
    TestEqual(TEXT("Ranger default kit selected"), Wizard->GetSelectedStartingEquipmentKitId(),
        FName(TEXT("Ranger_Archer")));
    TestFalse(TEXT("Unknown ID refused without selecting"),
        Wizard->SelectStartingEquipmentKit(TEXT("Warrior_Defender")));
    TestEqual(TEXT("Rejected ID does not change default"), Wizard->GetSelectedStartingEquipmentKitId(),
        FName(TEXT("Ranger_Archer")));
    if (!TestTrue(TEXT("Ranger alternative can be selected"),
        Wizard->SelectStartingEquipmentKit(TEXT("Ranger_Pathfinder")))) return false;
    TestEqual(TEXT("Ranger alternative selection is retained"), Wizard->GetSelectedStartingEquipmentKitId(),
        FName(TEXT("Ranger_Pathfinder")));
    const FRPGStartingEquipmentKit* Pathfinder = Kits.FindByPredicate(
        [](const FRPGStartingEquipmentKit& Kit) { return Kit.KitId == FName(TEXT("Ranger_Pathfinder")); });
    if (!TestNotNull(TEXT("Pathfinder kit exists"), Pathfinder)) return false;
    const UGridItemDefinitionAsset* Pilot = Pathfinder->Items[0].ItemDefinition.Get();
    if (!TestNotNull(TEXT("Pathfinder item exists"), Pilot)) return false;
    const FString PilotName = Pilot->DisplayName.IsEmpty() ? Pilot->ItemDefinitionId.ToString() : Pilot->DisplayName.ToString();
    TestTrue(TEXT("Selection description includes authored display name"),
        Wizard->GetSelectedStartingEquipmentKitDescription().ToString().Contains(PilotName));

    Wizard->ClassDefinition = Warrior;
    Wizard->RefreshPreview();
    TestEqual(TEXT("Switching class resets to Warrior default"), Wizard->GetSelectedStartingEquipmentKitId(),
        FName(TEXT("Warrior_Defender")));
    TestFalse(TEXT("Old Ranger kit cannot be selected for Warrior"),
        Wizard->SelectStartingEquipmentKit(TEXT("Ranger_Pathfinder")));
    TestEqual(TEXT("Two Warrior options are offered"), Wizard->GetAvailableStartingEquipmentKits().Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0193RequestTest,
    "Grimrock.RPG.ARMEMENT01.9.WizardSelection.AlternativeKitNewGame",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0193RequestTest::RunTest(const FString&)
{
    URPGClassAsset* Ranger = ARMEMENT0193::Class(TEXT("Ranger"));
    if (!TestNotNull(TEXT("Ranger exists"), Ranger)) return false;
    UGridPartyInventoryComponent* Party = ARMEMENT0193::NewParty();
    FText Error;
    const FRPGCharacterCreationRequest NewGame =
        ARMEMENT0193::Request(Ranger, TEXT("Ranger_Pathfinder"));
    if (!TestTrue(TEXT("Explicit alternative kit accepted"),
        Party->CreateInitialCharacter(NewGame, Error))) return false;
    TestEqual(TEXT("Alternative kit ID retained on character"),
        Party->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId,
        FName(TEXT("Ranger_Pathfinder")));
    TestEqual(TEXT("Alternative kit grants exactly two physical objects"),
        ARMEMENT0193::CountPhysicalObjects(Party), 2);
    const FGridItemInstance& Main = Party->PartyInventoryState.ActiveEquipment[0].MainHand;
    TestEqual(TEXT("Pathfinder equips the dagger"), Main.ItemDefinitionId,
        FName(TEXT("Weapon_Dagger_01")));
    TestFalse(TEXT("Alternative kit leaves OffHand available"),
        Party->PartyInventoryState.ActiveEquipment[0].OffHand.IsValid());
    FString OwnershipError;
    TestTrue(TEXT("Ownership invariants hold"), Party->ValidateInventoryOwnership(OwnershipError));

    UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
    Save->PartyInventoryState = Party->PartyInventoryState;
    TestEqual(TEXT("Kit selection is in the SaveGame party snapshot"),
        Save->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId,
        FName(TEXT("Ranger_Pathfinder")));
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
    URPGClassAsset* Ranger = ARMEMENT0193::Class(TEXT("Ranger"));
    if (!TestNotNull(TEXT("Ranger exists"), Ranger)) return false;
    UGridPartyInventoryComponent* Party = ARMEMENT0193::NewParty();
    const FGuid PreviousId = Party->PartyInventoryState.ActiveCharacters[0].CharacterId;
    FText Error;
    const FRPGCharacterCreationRequest WrongClass =
        ARMEMENT0193::Request(Ranger, TEXT("Warrior_Defender"));
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
    URPGClassAsset* Warrior = ARMEMENT0193::Class(TEXT("Warrior"));
    if (!TestNotNull(TEXT("Warrior exists"), Warrior)) return false;
    UGridPartyInventoryComponent* Party = ARMEMENT0193::NewParty();
    FText Error;
    if (!TestTrue(TEXT("Omitting the kit selection preserves default New Game"),
        Party->CreateInitialCharacter(ARMEMENT0193::Request(Warrior, NAME_None), Error))) return false;
    TestEqual(TEXT("Warrior default kit is unchanged"),
        Party->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId,
        FName(TEXT("Warrior_Defender")));

    URPGCharacterCreationWizardWidget* Recruit = NewObject<URPGCharacterCreationWizardWidget>();
    Recruit->ClassDefinition = Warrior;
    Recruit->CreationContext = ERPGCharacterCreationContext::CustomRecruit;
    TestTrue(TEXT("Custom recruitment does not introduce starting-kit options"),
        Recruit->GetAvailableStartingEquipmentKits().IsEmpty());
    TestTrue(TEXT("Recruitment has no selected starting kit"),
        Recruit->GetSelectedStartingEquipmentKitId().IsNone());
    TestFalse(TEXT("Recruitment cannot choose an initial-hero kit"),
        Recruit->SelectStartingEquipmentKit(TEXT("Warrior_Defender")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0193KitPresentationTest,
    "Grimrock.RPG.ARMEMENT01.9.WizardSelection.PlayerFacingKitLabels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0193KitPresentationTest::RunTest(const FString&)
{
    URPGCharacterCreationWizardWidget* Wizard = NewObject<URPGCharacterCreationWizardWidget>();
    Wizard->ClassDefinition = ARMEMENT0193::Class(TEXT("Warrior"));
    if (!TestNotNull(TEXT("Production Warrior exists"), Wizard->ClassDefinition.Get()))
    {
        return false;
    }

    const TArray<FRPGStartingEquipmentKit> Kits = Wizard->GetAvailableStartingEquipmentKits();
    if (!TestEqual(TEXT("Two starting kit labels"), Kits.Num(), 2))
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

    if (!TestTrue(TEXT("Select second Warrior kit"), Wizard->SelectStartingEquipmentKit(TEXT("Warrior_Skirmisher"))))
    {
        return false;
    }
    const FString Label = Wizard->GetStartingKitOptionLabel(Kits[1]).ToString();
    const FString Details = Wizard->GetSelectedStartingEquipmentKitDescription().ToString();
    const FString Summary = Wizard->GetSelectedStartingEquipmentKitSummary().ToString();

    TestFalse(TEXT("Class description does not repeat the displayed title"), Details.StartsWith(Label));
    TestTrue(TEXT("Class description starts with an item quantity"), Details.StartsWith(TEXT("1 × ")));
    TestTrue(TEXT("Technical ID appears in the details rather than the ComboBox"),
        Details.Contains(TEXT("Référence technique : Warrior_Skirmisher")));
    TestTrue(TEXT("Details have both item rows"), Details.Contains(TEXT("\n1 × ")));
    TestTrue(TEXT("Summary retains kit name without a ComboBox"), Summary.StartsWith(Label + TEXT("\n")));
    TestTrue(TEXT("Summary retains technical reference"), Summary.Contains(TEXT("Warrior_Skirmisher")));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
