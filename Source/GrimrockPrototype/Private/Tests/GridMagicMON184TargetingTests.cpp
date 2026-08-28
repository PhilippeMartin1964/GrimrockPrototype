#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Magic/GridSpellTargeting.h"

namespace
{
	FGridSpellDefinition MakeTargetingSpell()
	{
		FGridSpellDefinition Definition;
		Definition.SpellId = TEXT("Spell_TargetingTest");
		Definition.DisplayName = FText::FromString(TEXT("Targeting Test"));
		Definition.School = EGridSpellSchool::Arcane;
		Definition.ManaCost = 3;
		Definition.ActionPointCost = 2;
		Definition.MinRangeCells = 1;
		Definition.MaxRangeCells = 6;
		Definition.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
		Definition.bRequiresLineOfSight = true;

		FGridSpellEffectDefinition Effect;
		Effect.Type = EGridSpellEffectType::Damage;
		Effect.Magnitude = 4;
		Definition.Effects.Add(Effect);
		return Definition;
	}

	FGridSpellCastRequest MakeTargetingRequest(const FGuid& CharacterId, const FGuid& TargetId)
	{
		FGridSpellCastRequest Request;
		Request.CasterCharacterId = CharacterId;
		Request.SpellId = TEXT("Spell_TargetingTest");
		Request.Target.TargetId = TargetId;
		return Request;
	}

	FGridCharacterSpellbookState MakeTargetingSpellbook(const FGuid& CharacterId)
	{
		FGridCharacterSpellbookState Spellbook;
		Spellbook.CharacterId = CharacterId;
		Spellbook.KnownSpellIds.Add(TEXT("Spell_TargetingTest"));
		return Spellbook;
	}

	FGridPlayerCharacterTurnState MakeTargetingTurn(const FGuid& CharacterId, int32 ActionPoints = 3)
	{
		FGridPlayerCharacterTurnState Turn;
		Turn.CharacterIndex = 0;
		Turn.CharacterId = CharacterId;
		Turn.State = EGridCombatantTurnState::Active;
		Turn.MaximumActionPoints = 3;
		Turn.RemainingActionPoints = ActionPoints;
		return Turn;
	}

	FGridSpellTargetingContext MakeHostileContext(const FGuid& TargetId, const FIntPoint& TargetCell)
	{
		FGridSpellTargetingContext Context;
		Context.CasterCell = FIntPoint(2, 2);
		Context.ResolvedTargetId = TargetId;
		Context.ResolvedTargetCell = TargetCell;
		Context.bHasResolvedTargetCell = true;
		Context.bResolvedTargetIsHostile = true;
		Context.bLineOfSightClear = true;
		return Context;
	}

	bool TryPipeline(const FGridSpellDefinition& Definition, const FGridSpellCastRequest& Request, const FGridSpellTargetingContext& Context,
		const FGridCharacterSpellbookState& Spellbook, FRPGCharacterResources& Resources, FGridPlayerCharacterTurnState& Turn,
		FGridSpellResolvedTarget& ResolvedTarget, FGridSpellCastCostReceipt& Receipt, EGridSpellCastPipelineRejectStage& RejectStage,
		EGridSpellTargetingRejectReason& TargetReject, EGridSpellCastTransactionRejectReason& TransactionReject)
	{
		return FGridSpellCastPipelineService::TryValidateTargetAndCommitCosts(
			Definition, Request, Context, Spellbook, Resources, Turn, ResolvedTarget, Receipt, RejectStage, TargetReject, TransactionReject);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON184AxialTargetSuccessTest, "Grimrock.Magic.MON18.4.AxialTargetSuccess", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON184AxialTargetSuccessTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	const FGuid TargetId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeTargetingSpell();
	FGridSpellCastRequest Request = MakeTargetingRequest(CharacterId, TargetId);
	FGridSpellTargetingContext Context = MakeHostileContext(TargetId, FIntPoint(2, 5));
	FGridCharacterSpellbookState Spellbook = MakeTargetingSpellbook(CharacterId);
	FRPGCharacterResources Resources;
	Resources.CurrentMana = 8;
	FGridPlayerCharacterTurnState Turn = MakeTargetingTurn(CharacterId);
	FGridSpellResolvedTarget ResolvedTarget;
	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastPipelineRejectStage RejectStage;
	EGridSpellTargetingRejectReason TargetReject;
	EGridSpellCastTransactionRejectReason TransactionReject;

	TestTrue(TEXT("Valid axial target commits"),
		TryPipeline(Definition, Request, Context, Spellbook, Resources, Turn, ResolvedTarget, Receipt, RejectStage, TargetReject, TransactionReject));
	TestEqual(TEXT("Mana paid"), Resources.CurrentMana, 5);
	TestEqual(TEXT("PA paid"), Turn.RemainingActionPoints, 1);
	TestTrue(TEXT("Target resolved"), ResolvedTarget.TargetId == TargetId);
	TestTrue(TEXT("Resolved cell retained"), ResolvedTarget.GridCell == FIntPoint(2, 5));
	TestEqual(TEXT("No reject stage"), RejectStage, EGridSpellCastPipelineRejectStage::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON184OutOfRangeNoMutationTest, "Grimrock.Magic.MON18.4.OutOfRangeNoMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON184OutOfRangeNoMutationTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	const FGuid TargetId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeTargetingSpell();
	Definition.MaxRangeCells = 2;
	FGridSpellCastRequest Request = MakeTargetingRequest(CharacterId, TargetId);
	FGridSpellTargetingContext Context = MakeHostileContext(TargetId, FIntPoint(2, 5));
	FGridCharacterSpellbookState Spellbook = MakeTargetingSpellbook(CharacterId);
	FRPGCharacterResources Resources;
	Resources.CurrentMana = 8;
	FGridPlayerCharacterTurnState Turn = MakeTargetingTurn(CharacterId);
	FGridSpellResolvedTarget ResolvedTarget;
	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastPipelineRejectStage RejectStage;
	EGridSpellTargetingRejectReason TargetReject;
	EGridSpellCastTransactionRejectReason TransactionReject;

	TestFalse(TEXT("Out of range rejected"),
		TryPipeline(Definition, Request, Context, Spellbook, Resources, Turn, ResolvedTarget, Receipt, RejectStage, TargetReject, TransactionReject));
	TestEqual(TEXT("Mana unchanged"), Resources.CurrentMana, 8);
	TestEqual(TEXT("PA unchanged"), Turn.RemainingActionPoints, 3);
	TestEqual(TEXT("Targeting stage"), RejectStage, EGridSpellCastPipelineRejectStage::Targeting);
	TestEqual(TEXT("Range reason"), TargetReject, EGridSpellTargetingRejectReason::TargetOutOfRange);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON184NonAxialNoMutationTest, "Grimrock.Magic.MON18.4.NonAxialNoMutation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON184NonAxialNoMutationTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	const FGuid TargetId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeTargetingSpell();
	FGridSpellCastRequest Request = MakeTargetingRequest(CharacterId, TargetId);
	FGridSpellTargetingContext Context = MakeHostileContext(TargetId, FIntPoint(4, 4));
	FGridCharacterSpellbookState Spellbook = MakeTargetingSpellbook(CharacterId);
	FRPGCharacterResources Resources;
	Resources.CurrentMana = 8;
	FGridPlayerCharacterTurnState Turn = MakeTargetingTurn(CharacterId);
	FGridSpellResolvedTarget ResolvedTarget;
	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastPipelineRejectStage RejectStage;
	EGridSpellTargetingRejectReason TargetReject;
	EGridSpellCastTransactionRejectReason TransactionReject;

	TestFalse(TEXT("Non axial target rejected"),
		TryPipeline(Definition, Request, Context, Spellbook, Resources, Turn, ResolvedTarget, Receipt, RejectStage, TargetReject, TransactionReject));
	TestEqual(TEXT("Mana unchanged"), Resources.CurrentMana, 8);
	TestEqual(TEXT("PA unchanged"), Turn.RemainingActionPoints, 3);
	TestEqual(TEXT("Axial reason"), TargetReject, EGridSpellTargetingRejectReason::TargetNotAxial);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON184LineOfSightNoMutationTest, "Grimrock.Magic.MON18.4.LineOfSightNoMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON184LineOfSightNoMutationTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	const FGuid TargetId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeTargetingSpell();
	FGridSpellCastRequest Request = MakeTargetingRequest(CharacterId, TargetId);
	FGridSpellTargetingContext Context = MakeHostileContext(TargetId, FIntPoint(2, 4));
	Context.bLineOfSightClear = false;
	FGridCharacterSpellbookState Spellbook = MakeTargetingSpellbook(CharacterId);
	FRPGCharacterResources Resources;
	Resources.CurrentMana = 8;
	FGridPlayerCharacterTurnState Turn = MakeTargetingTurn(CharacterId);
	FGridSpellResolvedTarget ResolvedTarget;
	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastPipelineRejectStage RejectStage;
	EGridSpellTargetingRejectReason TargetReject;
	EGridSpellCastTransactionRejectReason TransactionReject;

	TestFalse(TEXT("Blocked LOS rejected"),
		TryPipeline(Definition, Request, Context, Spellbook, Resources, Turn, ResolvedTarget, Receipt, RejectStage, TargetReject, TransactionReject));
	TestEqual(TEXT("Mana unchanged"), Resources.CurrentMana, 8);
	TestEqual(TEXT("PA unchanged"), Turn.RemainingActionPoints, 3);
	TestEqual(TEXT("LOS reason"), TargetReject, EGridSpellTargetingRejectReason::LineOfSightBlocked);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON184AllyRelationTest, "Grimrock.Magic.MON18.4.AllyRelation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON184AllyRelationTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	const FGuid TargetId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeTargetingSpell();
	Definition.TargetingPolicy = EGridCombatTargetingPolicy::Ally;
	Definition.bRequiresLineOfSight = false;
	FGridSpellCastRequest Request = MakeTargetingRequest(CharacterId, TargetId);
	FGridSpellTargetingContext Context = MakeHostileContext(TargetId, FIntPoint(3, 2));
	Context.bResolvedTargetIsHostile = false;
	Context.bResolvedTargetIsAlly = true;
	FGridSpellResolvedTarget ResolvedTarget;

	TestEqual(TEXT("Ally accepted"), FGridSpellTargetingService::ValidateAndResolveTarget(Definition, Request, Context, ResolvedTarget),
		EGridSpellTargetingRejectReason::None);
	TestTrue(TEXT("Ally id retained"), ResolvedTarget.TargetId == TargetId);

	Context.bResolvedTargetIsAlly = false;
	TestEqual(TEXT("Non ally rejected"), FGridSpellTargetingService::ValidateAndResolveTarget(Definition, Request, Context, ResolvedTarget),
		EGridSpellTargetingRejectReason::InvalidTargetRelation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON184CellAreaResolutionTest, "Grimrock.Magic.MON18.4.CellAreaResolution", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON184CellAreaResolutionTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeTargetingSpell();
	Definition.TargetingPolicy = EGridCombatTargetingPolicy::Area;
	Definition.bRequiresLineOfSight = false;
	FGridSpellCastRequest Request;
	Request.CasterCharacterId = CharacterId;
	Request.SpellId = Definition.SpellId;
	Request.Target.bHasGridCell = true;
	Request.Target.GridCell = FIntPoint(5, 2);
	FGridSpellTargetingContext Context;
	Context.CasterCell = FIntPoint(2, 2);
	FGridSpellResolvedTarget ResolvedTarget;

	TestEqual(TEXT("Area cell accepted"), FGridSpellTargetingService::ValidateAndResolveTarget(Definition, Request, Context, ResolvedTarget),
		EGridSpellTargetingRejectReason::None);
	TestTrue(TEXT("Area cell retained"), ResolvedTarget.GridCell == FIntPoint(5, 2));
	TestTrue(TEXT("Area has cell"), ResolvedTarget.bHasGridCell);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON184SelfResolutionTest, "Grimrock.Magic.MON18.4.SelfResolution", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON184SelfResolutionTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeTargetingSpell();
	Definition.TargetingPolicy = EGridCombatTargetingPolicy::Self;
	Definition.MinRangeCells = 0;
	Definition.MaxRangeCells = 0;
	Definition.bRequiresLineOfSight = false;
	FGridSpellCastRequest Request;
	Request.CasterCharacterId = CharacterId;
	Request.SpellId = Definition.SpellId;
	FGridSpellTargetingContext Context;
	Context.CasterCell = FIntPoint(7, 9);
	FGridSpellResolvedTarget ResolvedTarget;

	TestEqual(TEXT("Self target accepted"), FGridSpellTargetingService::ValidateAndResolveTarget(Definition, Request, Context, ResolvedTarget),
		EGridSpellTargetingRejectReason::None);
	TestTrue(TEXT("Caster is target"), ResolvedTarget.TargetId == CharacterId);
	TestTrue(TEXT("Caster cell retained"), ResolvedTarget.GridCell == FIntPoint(7, 9));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON184TransactionFailureAfterTargetTest, "Grimrock.Magic.MON18.4.TransactionFailureAfterTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON184TransactionFailureAfterTargetTest::RunTest(const FString& Parameters)
{
	const FGuid CharacterId = FGuid::NewGuid();
	const FGuid TargetId = FGuid::NewGuid();
	FGridSpellDefinition Definition = MakeTargetingSpell();
	FGridSpellCastRequest Request = MakeTargetingRequest(CharacterId, TargetId);
	FGridSpellTargetingContext Context = MakeHostileContext(TargetId, FIntPoint(2, 4));
	FGridCharacterSpellbookState Spellbook = MakeTargetingSpellbook(CharacterId);
	FRPGCharacterResources Resources;
	Resources.CurrentMana = 1;
	FGridPlayerCharacterTurnState Turn = MakeTargetingTurn(CharacterId);
	FGridSpellResolvedTarget ResolvedTarget;
	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastPipelineRejectStage RejectStage;
	EGridSpellTargetingRejectReason TargetReject;
	EGridSpellCastTransactionRejectReason TransactionReject;

	TestFalse(TEXT("Transaction rejection returned"),
		TryPipeline(Definition, Request, Context, Spellbook, Resources, Turn, ResolvedTarget, Receipt, RejectStage, TargetReject, TransactionReject));
	TestEqual(TEXT("Mana unchanged"), Resources.CurrentMana, 1);
	TestEqual(TEXT("PA unchanged"), Turn.RemainingActionPoints, 3);
	TestEqual(TEXT("Transaction stage"), RejectStage, EGridSpellCastPipelineRejectStage::Transaction);
	TestEqual(TEXT("Targeting succeeded first"), TargetReject, EGridSpellTargetingRejectReason::None);
	TestEqual(TEXT("Mana rejection"), TransactionReject, EGridSpellCastTransactionRejectReason::InsufficientMana);
	TestFalse(TEXT("Resolved target cleared on transaction failure"), ResolvedTarget.bHasGridCell);
	return true;
}

#endif
