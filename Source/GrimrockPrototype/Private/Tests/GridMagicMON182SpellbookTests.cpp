#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GridPartySpellbookComponent.h"
#include "Magic/GridProductionSpellLibrary.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/UnrealType.h"

namespace GridMON182Tests
{
	FGridCharacterInventoryState MakeMON182Character(const FGuid& CharacterId)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.Experience = 0;
		Character.Level = 1;
		return Character;
	}

	UGridPartySpellbookComponent* MakeMON182Spellbook(const TArray<FGuid>& CharacterIds, UGridPartyInventoryComponent*& OutInventory)
	{
		OutInventory = NewObject<UGridPartyInventoryComponent>();
		OutInventory->PartyInventoryState = FGridPartyInventoryState();
		for (const FGuid& CharacterId : CharacterIds)
		{
			OutInventory->PartyInventoryState.ActiveCharacters.Add(MakeMON182Character(CharacterId));
		}
		UGridPartySpellbookComponent* Spellbook = NewObject<UGridPartySpellbookComponent>();
		Spellbook->InitializeSpellbookComponent(OutInventory);
		return Spellbook;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON182CharacterRegistrationTest, "Grimrock.Magic.MON18.2.CharacterRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON182CharacterRegistrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON182Tests;
	const FGuid CharacterId(18, 2, 1, 1);
	UGridPartyInventoryComponent* Inventory = nullptr;
	UGridPartySpellbookComponent* Spellbook = MakeMON182Spellbook({ CharacterId }, Inventory);

	TestTrue(TEXT("Existing character implicitly owns a Spellbook"), Spellbook->EnsureCharacterSpellbook(CharacterId));
	TestFalse(TEXT("Unknown CharacterId is rejected"), Spellbook->EnsureCharacterSpellbook(FGuid(18, 2, 1, 99)));
	TestFalse(TEXT("Invalid CharacterId is rejected"), Spellbook->EnsureCharacterSpellbook(FGuid()));

	FGridCharacterSpellbookState View;
	TestTrue(TEXT("Durable character can project a Spellbook view"), Spellbook->GetCharacterSpellbookState(CharacterId, View));
	TestTrue(TEXT("New character Spellbook is empty"), View.KnownSpellIds.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON182LearnForgetTest, "Grimrock.Magic.MON18.2.LearnForget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON182LearnForgetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON182Tests;
	const FGuid CharacterId(18, 2, 2, 1);
	UGridPartyInventoryComponent* Inventory = nullptr;
	UGridPartySpellbookComponent* Spellbook = MakeMON182Spellbook({ CharacterId }, Inventory);
	const FName SpellId = FGridProductionSpellLibrary::ArcaneBoltId();

	TestEqual(TEXT("First canonical learn succeeds"), Spellbook->LearnSpell(CharacterId, SpellId), EGridSpellbookMutationResult::Success);
	TestTrue(TEXT("Spell becomes known"), Spellbook->KnowsSpell(CharacterId, SpellId));
	TestEqual(TEXT("Duplicate learn is rejected"), Spellbook->LearnSpell(CharacterId, SpellId), EGridSpellbookMutationResult::AlreadyKnown);
	TestEqual(TEXT("NAME_None cannot be learned"), Spellbook->LearnSpell(CharacterId, NAME_None), EGridSpellbookMutationResult::InvalidSpell);
	TestEqual(TEXT("Non-canonical SpellId cannot be learned"), Spellbook->LearnSpell(CharacterId, TEXT("Spell_RemovedContent")),
		EGridSpellbookMutationResult::InvalidSpell);
	TestEqual(TEXT("Forget succeeds"), Spellbook->ForgetSpell(CharacterId, SpellId), EGridSpellbookMutationResult::Success);
	TestFalse(TEXT("Forgotten spell is no longer known"), Spellbook->KnowsSpell(CharacterId, SpellId));
	TestEqual(TEXT("Forgetting unknown spell is explicit"), Spellbook->ForgetSpell(CharacterId, SpellId), EGridSpellbookMutationResult::NotKnown);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON182CharacterIsolationTest, "Grimrock.Magic.MON18.2.CharacterIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON182CharacterIsolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON182Tests;
	const FGuid MageId(18, 2, 3, 1);
	const FGuid ClericId(18, 2, 3, 2);
	UGridPartyInventoryComponent* Inventory = nullptr;
	UGridPartySpellbookComponent* Spellbook = MakeMON182Spellbook({ MageId, ClericId }, Inventory);
	const FName ArcaneBolt = FGridProductionSpellLibrary::ArcaneBoltId();
	const FName Heal = FGridProductionSpellLibrary::LesserHealId();

	Spellbook->LearnSpell(MageId, ArcaneBolt);
	Spellbook->LearnSpell(ClericId, Heal);

	TestTrue(TEXT("Mage knows Arcane Bolt"), Spellbook->KnowsSpell(MageId, ArcaneBolt));
	TestFalse(TEXT("Mage does not inherit Heal"), Spellbook->KnowsSpell(MageId, Heal));
	TestTrue(TEXT("Cleric knows Heal"), Spellbook->KnowsSpell(ClericId, Heal));
	TestFalse(TEXT("Cleric does not inherit Arcane Bolt"), Spellbook->KnowsSpell(ClericId, ArcaneBolt));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON182StableIdentityTest, "Grimrock.Magic.MON18.2.StableIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON182StableIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON182Tests;
	const FGuid CharacterId(18, 2, 4, 1);
	UGridPartyInventoryComponent* Inventory = nullptr;
	UGridPartySpellbookComponent* Spellbook = MakeMON182Spellbook({ CharacterId }, Inventory);
	Spellbook->LearnSpell(CharacterId, FGridProductionSpellLibrary::HasteId());
	Spellbook->LearnSpell(CharacterId, FGridProductionSpellLibrary::ArcaneBoltId());

	const TArray<FName> KnownSpellIds = Spellbook->GetKnownSpellIds(CharacterId);
	TestEqual(TEXT("Spellbook stores two stable ids"), KnownSpellIds.Num(), 2);
	if (KnownSpellIds.Num() == 2)
	{
		TestEqual(TEXT("Durable identities are sorted deterministically"), KnownSpellIds[0], FGridProductionSpellLibrary::ArcaneBoltId());
		TestEqual(TEXT("Second identity is Haste"), KnownSpellIds[1], FGridProductionSpellLibrary::HasteId());
	}

	FString ValidationError;
	TestTrue(TEXT("Durable Spellbook state validates"), Spellbook->ValidateSpellbookState(ValidationError));
	TestTrue(TEXT("No validation error"), ValidationError.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON182DurableContractTest, "Grimrock.Magic.MON18.2.DurableContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON182DurableContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FProperty* KnownSpellsProperty = FindFProperty<FProperty>(FGridCharacterInventoryState::StaticStruct(), TEXT("KnownSpellIds"));
	TestNotNull(TEXT("KnownSpellIds is reflected on durable character state"), KnownSpellsProperty);
	TestTrue(TEXT("KnownSpellIds is not transient"), KnownSpellsProperty && !KnownSpellsProperty->HasAnyPropertyFlags(CPF_Transient));
	TestNull(TEXT("Component no longer exposes a parallel SpellbookState property"),
		FindFProperty<FProperty>(UGridPartySpellbookComponent::StaticClass(), TEXT("SpellbookState")));
	return true;
}

#endif
