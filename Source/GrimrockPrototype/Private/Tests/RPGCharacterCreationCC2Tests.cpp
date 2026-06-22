#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/UObjectGlobals.h"

namespace
{
    URPGRaceAsset* CreateCC2HumanRace ()
    {
        URPGRaceAsset* Race = NewObject<URPGRaceAsset> ();
        Race->RaceId = TEXT ("Human");
        Race->DisplayName = FText::FromString (TEXT ("Human"));
        Race->AttributeBonuses = FRPGAttributes { 1, 1, 1, 1, 1, 1 };
        return Race;
    }

    URPGClassAsset* CreateCC2WarriorClass ()
    {
        URPGClassAsset* CharacterClass = NewObject<URPGClassAsset> ();
        CharacterClass->ClassId = TEXT ("Warrior");
        CharacterClass->DisplayName = FText::FromString (TEXT ("Warrior"));
        CharacterClass->BaseAttributes = FRPGAttributes { 15, 11, 13, 9, 9, 9 };
        CharacterClass->HealthAtLevelOne = 18;
        CharacterClass->HealthPerLevel = 8;
        return CharacterClass;
    }

    FRPGCharacterCreationRequest CreateValidCC2Request (const TCHAR* DisplayName = TEXT ("Elias"))
    {
        FRPGCharacterCreationRequest Request;
        Request.DisplayName = FText::FromString (DisplayName);
        Request.RaceDefinition = CreateCC2HumanRace ();
        Request.ClassDefinition = CreateCC2WarriorClass ();
        return Request;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGCreateInitialCharacterCC2Test,
    "Grimrock.CharacterCreation.CC2.CreateInitialCharacter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGCreateInitialCharacterCC2Test::RunTest (const FString& Parameters)
{
    UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent> ();
    Component->InitializeDefaultPartyIfNeeded ();

    FGridItemInstance CursorItem;
    CursorItem.RuntimeObjectId = FGuid::NewGuid ();
    CursorItem.ItemDefinitionId = TEXT ("CC2_PlaceholderItem");
    Component->SetCursorItem (CursorItem);

    const FRPGCharacterCreationRequest Request = CreateValidCC2Request (TEXT ("  Elias  "));
    FText Error;
    TestTrue (TEXT ("A valid initial character is created"), Component->CreateInitialCharacter (Request, Error));
    TestTrue (TEXT ("No creation error is returned"), Error.IsEmpty ());
    TestTrue (TEXT ("Creation is marked completed"), Component->HasCompletedInitialCharacterCreation ());
    TestEqual (TEXT ("Exactly one active character exists"), Component->GetActiveCharacterCount (), 1);
    TestEqual (TEXT ("Character zero is selected"), Component->GetSelectedCharacterIndex (), 0);
    TestEqual (TEXT ("Exactly one equipment state exists"), Component->PartyInventoryState.ActiveEquipment.Num (), 1);
    TestTrue (TEXT ("The cursor is cleared by initial creation"), !Component->HasCursorItem ());

    const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
    TestTrue (TEXT ("The character has a runtime id"), Character.CharacterId.IsValid ());
    TestEqual (TEXT ("The character name is trimmed"), Character.DisplayName.ToString (), FString (TEXT ("Elias")));
    TestTrue (TEXT ("Race id comes from the race DataAsset"), Character.RaceId == FName (TEXT ("Human")));
    TestTrue (TEXT ("Class id comes from the class DataAsset"), Character.ClassId == FName (TEXT ("Warrior")));
    TestEqual (TEXT ("The character starts at level one"), Character.Level, 1);
    TestEqual (TEXT ("The character starts with zero experience"), Character.Experience, 0);
    TestEqual (TEXT ("The character has 40 inventory slots"), Character.InventorySlots.Num (), 40);
    TestEqual (TEXT ("Final Strength is 16"), Character.Attributes.Strength, 16);
    TestEqual (TEXT ("Final Dexterity is 12"), Character.Attributes.Dexterity, 12);
    TestEqual (TEXT ("Final Constitution is 14"), Character.Attributes.Constitution, 14);
    TestEqual (TEXT ("Final Intelligence is 10"), Character.Attributes.Intelligence, 10);
    TestEqual (TEXT ("Final Wisdom is 10"), Character.Attributes.Wisdom, 10);
    TestEqual (TEXT ("Final Charisma is 10"), Character.Attributes.Charisma, 10);
    TestEqual (TEXT ("The character starts with 20 maximum health"), Character.DerivedStats.MaxHealth, 20);
    TestEqual (TEXT ("The character starts at full health"), Character.DerivedStats.CurrentHealth, 20);
    TestEqual (TEXT ("The warrior starts with zero mana"), Character.DerivedStats.MaxMana, 0);
    TestTrue (TEXT ("Strength 16 gives maximum carry weight 80"),
        FMath::IsNearlyEqual (Character.MaxCarryWeight, 80.0f));

    FString OwnershipError;
    TestTrue (TEXT ("The newly created party has valid ownership"),
        Component->ValidateInventoryOwnership (OwnershipError));

    Component->InitializeDefaultPartyIfNeeded ();
    TestEqual (TEXT ("Reinitialization preserves the created character"),
        Component->PartyInventoryState.ActiveCharacters[0].DisplayName.ToString (),
        FString (TEXT ("Elias")));
    TestTrue (TEXT ("Reinitialization preserves completion state"),
        Component->HasCompletedInitialCharacterCreation ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGRejectInvalidInitialCharacterCC2Test,
    "Grimrock.CharacterCreation.CC2.RejectInvalidRequestAtomically",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGRejectInvalidInitialCharacterCC2Test::RunTest (const FString& Parameters)
{
    UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent> ();
    Component->InitializeDefaultPartyIfNeeded ();

    const FGuid PlaceholderId = Component->PartyInventoryState.ActiveCharacters[0].CharacterId;
    const FString PlaceholderName = Component->PartyInventoryState.ActiveCharacters[0].DisplayName.ToString ();

    FRPGCharacterCreationRequest MissingRaceRequest = CreateValidCC2Request ();
    MissingRaceRequest.RaceDefinition = nullptr;
    FText Error;
    TestTrue (TEXT ("A request without race is rejected"),
        !Component->CreateInitialCharacter (MissingRaceRequest, Error));
    TestTrue (TEXT ("The invalid request returns an error"), !Error.IsEmpty ());

    FRPGCharacterCreationRequest InvalidAttributesRequest = CreateValidCC2Request ();
    InvalidAttributesRequest.ClassDefinition->BaseAttributes.Strength = 30;
    Error = FText::GetEmpty ();
    TestTrue (TEXT ("Attributes outside 6-20 are rejected"),
        !Component->CreateInitialCharacter (InvalidAttributesRequest, Error));
    TestTrue (TEXT ("The attribute error is returned"), !Error.IsEmpty ());

    TestTrue (TEXT ("Creation remains incomplete after invalid requests"),
        !Component->HasCompletedInitialCharacterCreation ());
    TestEqual (TEXT ("The placeholder count is unchanged"), Component->GetActiveCharacterCount (), 1);
    TestTrue (TEXT ("The placeholder runtime id is unchanged"),
        Component->PartyInventoryState.ActiveCharacters[0].CharacterId == PlaceholderId);
    TestEqual (TEXT ("The placeholder name is unchanged"),
        Component->PartyInventoryState.ActiveCharacters[0].DisplayName.ToString (),
        PlaceholderName);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGRejectSecondInitialCharacterCC2Test,
    "Grimrock.CharacterCreation.CC2.RejectSecondCreation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGRejectSecondInitialCharacterCC2Test::RunTest (const FString& Parameters)
{
    UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent> ();
    Component->InitializeDefaultPartyIfNeeded ();

    FText Error;
    TestTrue (TEXT ("The first creation succeeds"),
        Component->CreateInitialCharacter (CreateValidCC2Request (TEXT ("Elias")), Error));

    const FGuid FirstCharacterId = Component->PartyInventoryState.ActiveCharacters[0].CharacterId;
    Error = FText::GetEmpty ();
    TestTrue (TEXT ("The second creation is rejected"),
        !Component->CreateInitialCharacter (CreateValidCC2Request (TEXT ("Replacement")), Error));
    TestTrue (TEXT ("The second creation returns an error"), !Error.IsEmpty ());
    TestTrue (TEXT ("The original character id is preserved"),
        Component->PartyInventoryState.ActiveCharacters[0].CharacterId == FirstCharacterId);
    TestEqual (TEXT ("The original character name is preserved"),
        Component->PartyInventoryState.ActiveCharacters[0].DisplayName.ToString (),
        FString (TEXT ("Elias")));
    TestEqual (TEXT ("The party still contains one character"), Component->GetActiveCharacterCount (), 1);
    return true;
}

#endif
