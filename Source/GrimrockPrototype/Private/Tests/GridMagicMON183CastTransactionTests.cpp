#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Magic/GridSpellCastTransaction.h"

namespace
{
	FGridSpellDefinition MakeValidSpell()
	{
		FGridSpellDefinition Definition;
		Definition.SpellId = TEXT("Spell_TestBolt");
		Definition.DisplayName = FText::FromString(TEXT("Test Bolt"));
		Definition.School = EGridSpellSchool::Arcane;
		Definition.ManaCost = 4;
		Definition.ActionPointCost = 2;
		Definition.MinRangeCells = 1;
		Definition.MaxRangeCells = 4;
		Definition.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;

		FGridSpellEffectDefinition Effect;
		Effect.Type = EGridSpellEffectType::Damage;
		Effect.Magnitude = 3;
		Definition.Effects.Add(Effect);
		return Definition;
	}

	FGridCharacterSpellbookState MakeSpellbook(const FGuid& CharacterId, bool bKnowsSpell = true)
	{
		FGridCharacterSpellbookState Spellbook;
		Spellbook.CharacterId = CharacterId;
		if (bKnowsSpell)
		{
			Spellbook.KnownSpellIds.Add(TEXT("Spell_TestBolt"));
		}
		return Spellbook;
	}

	FGridSpellCastRequest MakeRequest(const FGuid& CharacterId)
	{
		FGridSpellCastRequest Request;
		Request.CasterCharacterId = CharacterId;
		Request.SpellId = TEXT("Spell_TestBolt");
		return Request;
	}

	FGridPlayerCharacterTurnState MakeTurn(const FGuid& CharacterId, int32 ActionPoints = 3)
	{
		FGridPlayerCharacterTurnState Turn;
		Turn.CharacterIndex = 0;
		Turn.CharacterId = CharacterId;
		Turn.State = EGridCombatantTurnState::Active;
		Turn.MaximumActionPoints = 3;
		Turn.RemainingActionPoints = ActionPoints;
		return Turn;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON183SuccessfulCommitTest, "Grimrock.Magic.MON18.3.SuccessfulCommit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON183SuccessfulCommitTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeValidSpell();
	FGridCharacterSpellbookState Spellbook = MakeSpellbook(CharacterId);
	FGridSpellCastRequest Request = MakeRequest(CharacterId);
	FRPGDerivedStats Stats;
	Stats.MaxMana = 10;
	Stats.CurrentMana = 10;
	FGridPlayerCharacterTurnState Turn = MakeTurn(CharacterId);
	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastTransactionRejectReason RejectReason = EGridSpellCastTransactionRejectReason::InvalidRequest;

	TestTrue(
		TEXT("Valid cast costs commit"), FGridSpellCastTransactionService::TryCommitCosts(Definition, Request, Spellbook, Stats, Turn, Receipt, RejectReason));
	TestEqual(TEXT("Mana paid"), Stats.CurrentMana, 6);
	TestEqual(TEXT("PA paid"), Turn.RemainingActionPoints, 1);
	TestEqual(TEXT("No reject reason"), RejectReason, EGridSpellCastTransactionRejectReason::None);
	TestTrue(TEXT("Receipt valid"), Receipt.IsValid());
	TestEqual(TEXT("Receipt mana"), Receipt.ManaSpent, 4);
	TestEqual(TEXT("Receipt PA"), Receipt.ActionPointsSpent, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON183UnknownSpellNoMutationTest, "Grimrock.Magic.MON18.3.UnknownSpellNoMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON183UnknownSpellNoMutationTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeValidSpell();
	FGridCharacterSpellbookState Spellbook = MakeSpellbook(CharacterId, false);
	FGridSpellCastRequest Request = MakeRequest(CharacterId);
	FRPGDerivedStats Stats;
	Stats.CurrentMana = 10;
	FGridPlayerCharacterTurnState Turn = MakeTurn(CharacterId);
	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastTransactionRejectReason RejectReason = EGridSpellCastTransactionRejectReason::None;

	TestFalse(
		TEXT("Unknown spell rejected"), FGridSpellCastTransactionService::TryCommitCosts(Definition, Request, Spellbook, Stats, Turn, Receipt, RejectReason));
	TestEqual(TEXT("Mana unchanged"), Stats.CurrentMana, 10);
	TestEqual(TEXT("PA unchanged"), Turn.RemainingActionPoints, 3);
	TestEqual(TEXT("Reject reason"), RejectReason, EGridSpellCastTransactionRejectReason::SpellNotKnown);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON183InsufficientManaNoMutationTest, "Grimrock.Magic.MON18.3.InsufficientManaNoMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON183InsufficientManaNoMutationTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeValidSpell();
	FGridCharacterSpellbookState Spellbook = MakeSpellbook(CharacterId);
	FGridSpellCastRequest Request = MakeRequest(CharacterId);
	FRPGDerivedStats Stats;
	Stats.CurrentMana = 3;
	FGridPlayerCharacterTurnState Turn = MakeTurn(CharacterId);
	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastTransactionRejectReason RejectReason = EGridSpellCastTransactionRejectReason::None;

	TestFalse(TEXT("Insufficient mana rejected"),
		FGridSpellCastTransactionService::TryCommitCosts(Definition, Request, Spellbook, Stats, Turn, Receipt, RejectReason));
	TestEqual(TEXT("Mana unchanged"), Stats.CurrentMana, 3);
	TestEqual(TEXT("PA unchanged"), Turn.RemainingActionPoints, 3);
	TestEqual(TEXT("Reject reason"), RejectReason, EGridSpellCastTransactionRejectReason::InsufficientMana);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON183InsufficientActionPointsNoMutationTest, "Grimrock.Magic.MON18.3.InsufficientActionPointsNoMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON183InsufficientActionPointsNoMutationTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeValidSpell();
	FGridCharacterSpellbookState Spellbook = MakeSpellbook(CharacterId);
	FGridSpellCastRequest Request = MakeRequest(CharacterId);
	FRPGDerivedStats Stats;
	Stats.CurrentMana = 10;
	FGridPlayerCharacterTurnState Turn = MakeTurn(CharacterId, 1);
	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastTransactionRejectReason RejectReason = EGridSpellCastTransactionRejectReason::None;

	TestFalse(
		TEXT("Insufficient PA rejected"), FGridSpellCastTransactionService::TryCommitCosts(Definition, Request, Spellbook, Stats, Turn, Receipt, RejectReason));
	TestEqual(TEXT("Mana unchanged"), Stats.CurrentMana, 10);
	TestEqual(TEXT("PA unchanged"), Turn.RemainingActionPoints, 1);
	TestEqual(TEXT("Reject reason"), RejectReason, EGridSpellCastTransactionRejectReason::InsufficientActionPoints);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON183IdentityMismatchNoMutationTest, "Grimrock.Magic.MON18.3.IdentityMismatchNoMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON183IdentityMismatchNoMutationTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	const FGuid OtherCharacterId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeValidSpell();
	FGridCharacterSpellbookState Spellbook = MakeSpellbook(CharacterId);
	FGridSpellCastRequest Request = MakeRequest(OtherCharacterId);
	FRPGDerivedStats Stats;
	Stats.CurrentMana = 10;
	FGridPlayerCharacterTurnState Turn = MakeTurn(CharacterId);
	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastTransactionRejectReason RejectReason = EGridSpellCastTransactionRejectReason::None;

	TestFalse(TEXT("Character mismatch rejected"),
		FGridSpellCastTransactionService::TryCommitCosts(Definition, Request, Spellbook, Stats, Turn, Receipt, RejectReason));
	TestEqual(TEXT("Mana unchanged"), Stats.CurrentMana, 10);
	TestEqual(TEXT("PA unchanged"), Turn.RemainingActionPoints, 3);
	TestEqual(TEXT("Reject reason"), RejectReason, EGridSpellCastTransactionRejectReason::CharacterMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON183TargetingDeferredTest, "Grimrock.Magic.MON18.3.TargetingDeferred", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON183TargetingDeferredTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeValidSpell();
	Definition.TargetingPolicy = EGridCombatTargetingPolicy::Ally;
	FGridCharacterSpellbookState Spellbook = MakeSpellbook(CharacterId);
	FGridSpellCastRequest Request = MakeRequest(CharacterId);
	FRPGDerivedStats Stats;
	Stats.CurrentMana = 10;
	FGridPlayerCharacterTurnState Turn = MakeTurn(CharacterId);
	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastTransactionRejectReason RejectReason = EGridSpellCastTransactionRejectReason::InvalidRequest;

	TestTrue(TEXT("Cost transaction deliberately ignores target payload until MON18.4"),
		FGridSpellCastTransactionService::TryCommitCosts(Definition, Request, Spellbook, Stats, Turn, Receipt, RejectReason));
	TestEqual(TEXT("Mana paid"), Stats.CurrentMana, 6);
	TestEqual(TEXT("PA paid"), Turn.RemainingActionPoints, 1);
	return true;
}

#endif
