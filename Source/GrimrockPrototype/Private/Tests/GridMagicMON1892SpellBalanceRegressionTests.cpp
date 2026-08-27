#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellHotbarExecution.h"
#include "Magic/GridSpellbookUI.h"
#include "Misc/AutomationTest.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

namespace
{
	FGridCharacterSpellbookState MakeMON1892Spellbook(const FGuid& CharacterId, FName SpellId)
	{
		FGridCharacterSpellbookState Spellbook;
		Spellbook.CharacterId = CharacterId;
		Spellbook.KnownSpellIds.Add(SpellId);
		return Spellbook;
	}

	FRPGCharacterResources MakeMON1892CasterResources()
	{
		FRPGCharacterResources Resources;
		Resources.CurrentHealth = 20;
		Resources.CurrentMana = 10;
		return Resources;
	}

	FGridPlayerCharacterTurnState MakeMON1892TurnState(const FGuid& CharacterId)
	{
		FGridPlayerCharacterTurnState State;
		State.CharacterIndex = 0;
		State.CharacterId = CharacterId;
		State.MaximumActionPoints = 4;
		State.RemainingActionPoints = 4;
		State.State = EGridCombatantTurnState::Active;
		return State;
	}

	FGridSpellCastRequest MakeMON1892AllyRequest(const FGuid& CharacterId, FName SpellId)
	{
		FGridSpellCastRequest Request;
		Request.CasterCharacterId = CharacterId;
		Request.SpellId = SpellId;
		Request.Target.TargetId = CharacterId;
		Request.Target.GridCell = FIntPoint(1, 1);
		Request.Target.bHasGridCell = true;
		return Request;
	}

	FGridSpellTargetingContext MakeMON1892AllyContext(const FGuid& CharacterId)
	{
		FGridSpellTargetingContext Context;
		Context.CasterCell = FIntPoint(1, 1);
		Context.ResolvedTargetId = CharacterId;
		Context.ResolvedTargetCell = FIntPoint(1, 1);
		Context.bHasResolvedTargetCell = true;
		Context.bResolvedTargetIsAlly = true;
		Context.bLineOfSightClear = true;
		return Context;
	}

	UGridStatusEffectDefinitionAsset* MakeMON1892StatusDefinition(FName EffectId)
	{
		UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>();
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromName(EffectId);
		Definition->DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		Definition->DefaultDuration = 2;
		Definition->DefaultPotency = 0;
		Definition->StackPolicy = EGridStatusEffectStackPolicy::RefreshDuration;
		Definition->MaxStacks = 1;
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON1892BalanceContractTest, "Grimrock.Magic.MON18.9.2.BalanceContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1892BalanceContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridSpellDefinition ArcaneBolt = FGridProductionSpellLibrary::MakeArcaneBolt();
	const FGridSpellDefinition LesserHeal = FGridProductionSpellLibrary::MakeLesserHeal();
	const FGridSpellDefinition Haste = FGridProductionSpellLibrary::MakeHaste();
	const FGridSpellDefinition CurePoison = FGridProductionSpellLibrary::MakeCurePoison();

	TestEqual(TEXT("Arcane Bolt mana"), ArcaneBolt.ManaCost, 3);
	TestEqual(TEXT("Arcane Bolt AP"), ArcaneBolt.ActionPointCost, 2);
	TestEqual(TEXT("Arcane Bolt min range"), ArcaneBolt.MinRangeCells, 1);
	TestEqual(TEXT("Arcane Bolt max range"), ArcaneBolt.MaxRangeCells, 5);
	TestEqual(TEXT("Arcane Bolt cooldown"), ArcaneBolt.CooldownRounds, 0);
	TestEqual(TEXT("Arcane Bolt damage"), ArcaneBolt.Effects[0].Magnitude, 4);

	TestEqual(TEXT("Lesser Heal mana"), LesserHeal.ManaCost, 4);
	TestEqual(TEXT("Lesser Heal AP"), LesserHeal.ActionPointCost, 2);
	TestEqual(TEXT("Lesser Heal range"), LesserHeal.MaxRangeCells, 3);
	TestEqual(TEXT("Lesser Heal cooldown"), LesserHeal.CooldownRounds, 0);
	TestEqual(TEXT("Lesser Heal magnitude"), LesserHeal.Effects[0].Magnitude, 5);

	TestEqual(TEXT("Haste mana"), Haste.ManaCost, 5);
	TestEqual(TEXT("Haste AP"), Haste.ActionPointCost, 2);
	TestEqual(TEXT("Haste range"), Haste.MaxRangeCells, 3);
	TestEqual(TEXT("Haste cooldown"), Haste.CooldownRounds, 0);
	TestEqual(TEXT("Haste status"), Haste.Effects[0].StatusEffectId, FName(TEXT("Status_Haste")));

	TestEqual(TEXT("Cure Poison mana"), CurePoison.ManaCost, 4);
	TestEqual(TEXT("Cure Poison AP"), CurePoison.ActionPointCost, 2);
	TestEqual(TEXT("Cure Poison range"), CurePoison.MaxRangeCells, 3);
	TestEqual(TEXT("Cure Poison cooldown"), CurePoison.CooldownRounds, 0);
	TestEqual(TEXT("Cure Poison status"), CurePoison.Effects[0].StatusEffectId, FName(TEXT("Status_Poison")));

	const FGridCombatActionDefinition Action = UGridSpellbookUILibrary::MakeSpellCombatActionDefinition(ArcaneBolt);
	TestEqual(TEXT("Combat action AP mirrors spell"), Action.ActionPointCost, 2);
	TestEqual(TEXT("Combat action mana mirrors spell"), Action.ResourceCosts.ManaCost, 3);
	TestEqual(TEXT("Combat action range mirrors spell"), Action.RangeCells, 5);
	TestEqual(TEXT("Combat action cooldown mirrors spell"), Action.CooldownRounds, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1892FullHealthHealNoCostTest, "Grimrock.Magic.MON18.9.2.FullHealthHealNoCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1892FullHealthHealNoCostTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid CharacterId = FGuid::NewGuid();
	const FGridSpellDefinition Definition = FGridProductionSpellLibrary::MakeLesserHeal();
	const FRPGCharacterResources CasterResources = MakeMON1892CasterResources();
	const FGridPlayerCharacterTurnState TurnState = MakeMON1892TurnState(CharacterId);
	FGridSpellHotbarExecutionResult Result;

	TestFalse(TEXT("Lesser Heal rejects a full-health target"),
		FGridSpellHotbarExecutionService::TryExecute(
			Definition, MakeMON1892AllyRequest(CharacterId, Definition.SpellId), MakeMON1892AllyContext(CharacterId),
			MakeMON1892Spellbook(CharacterId, Definition.SpellId), CasterResources, TurnState, 20, 20, FGridStatusEffectCollection(),
			[](FName) -> const UGridStatusEffectDefinitionAsset*
			{
				return nullptr;
			},
			Result));
	TestEqual(TEXT("No-effect rejection"), Result.EffectRejectReason, EGridSpellEffectResolutionRejectReason::NoEffectWouldApply);
	TestEqual(TEXT("Authoritative input mana unchanged"), CasterResources.CurrentMana, 10);
	TestEqual(TEXT("Authoritative input AP unchanged"), TurnState.RemainingActionPoints, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1892CleanCureNoCostTest, "Grimrock.Magic.MON18.9.2.CurePoisonCleanTargetNoCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1892CleanCureNoCostTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid CharacterId = FGuid::NewGuid();
	const FGridSpellDefinition Definition = FGridProductionSpellLibrary::MakeCurePoison();
	const FRPGCharacterResources CasterResources = MakeMON1892CasterResources();
	const FGridPlayerCharacterTurnState TurnState = MakeMON1892TurnState(CharacterId);
	FGridSpellHotbarExecutionResult Result;

	TestFalse(TEXT("Cure Poison rejects a clean target"),
		FGridSpellHotbarExecutionService::TryExecute(
			Definition, MakeMON1892AllyRequest(CharacterId, Definition.SpellId), MakeMON1892AllyContext(CharacterId),
			MakeMON1892Spellbook(CharacterId, Definition.SpellId), CasterResources, TurnState, 20, 20, FGridStatusEffectCollection(),
			[](FName) -> const UGridStatusEffectDefinitionAsset*
			{
				return nullptr;
			},
			Result));
	TestEqual(TEXT("No-effect rejection"), Result.EffectRejectReason, EGridSpellEffectResolutionRejectReason::NoEffectWouldApply);
	TestEqual(TEXT("Authoritative input mana unchanged"), CasterResources.CurrentMana, 10);
	TestEqual(TEXT("Authoritative input AP unchanged"), TurnState.RemainingActionPoints, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON1892CurePoisonCommitTest, "Grimrock.Magic.MON18.9.2.CurePoisonCommit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1892CurePoisonCommitTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid CharacterId = FGuid::NewGuid();
	UGridStatusEffectDefinitionAsset* Poison = MakeMON1892StatusDefinition(TEXT("Status_Poison"));
	FGridStatusEffectCollection Statuses;
	FString AddError;
	TestTrue(TEXT("Poison fixture added"), Statuses.TryAdd(*Poison, FGuid::NewGuid(), AddError));

	const FGridSpellDefinition Definition = FGridProductionSpellLibrary::MakeCurePoison();
	FGridSpellHotbarExecutionResult Result;
	TestTrue(TEXT("Cure Poison executes when poison is present"),
		FGridSpellHotbarExecutionService::TryExecute(
			Definition, MakeMON1892AllyRequest(CharacterId, Definition.SpellId), MakeMON1892AllyContext(CharacterId),
			MakeMON1892Spellbook(CharacterId, Definition.SpellId), MakeMON1892CasterResources(), MakeMON1892TurnState(CharacterId), 20, 20, Statuses,
			[](FName) -> const UGridStatusEffectDefinitionAsset*
			{
				return nullptr;
			},
			Result));
	TestEqual(TEXT("Cure Poison spends four mana"), Result.CasterResources.CurrentMana, 6);
	TestEqual(TEXT("Cure Poison spends two AP"), Result.CasterTurnState.RemainingActionPoints, 2);
	TestFalse(TEXT("Poison removed"), Result.TargetStatusEffects.Contains(TEXT("Status_Poison")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON1892HasteCommitTest, "Grimrock.Magic.MON18.9.2.HasteCommit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1892HasteCommitTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid CharacterId = FGuid::NewGuid();
	UGridStatusEffectDefinitionAsset* Haste = MakeMON1892StatusDefinition(TEXT("Status_Haste"));
	const FGridSpellDefinition Definition = FGridProductionSpellLibrary::MakeHaste();
	FGridSpellHotbarExecutionResult Result;

	TestTrue(TEXT("Haste executes through the MON16 bridge"),
		FGridSpellHotbarExecutionService::TryExecute(
			Definition, MakeMON1892AllyRequest(CharacterId, Definition.SpellId), MakeMON1892AllyContext(CharacterId),
			MakeMON1892Spellbook(CharacterId, Definition.SpellId), MakeMON1892CasterResources(), MakeMON1892TurnState(CharacterId), 20, 20,
			FGridStatusEffectCollection(),
			[Haste](FName EffectId) -> const UGridStatusEffectDefinitionAsset*
			{
				return EffectId == Haste->EffectId ? Haste : nullptr;
			},
			Result));
	TestEqual(TEXT("Haste spends five mana"), Result.CasterResources.CurrentMana, 5);
	TestEqual(TEXT("Haste spends two AP"), Result.CasterTurnState.RemainingActionPoints, 2);
	TestTrue(TEXT("Haste status committed"), Result.TargetStatusEffects.Contains(TEXT("Status_Haste")));
	return true;
}

#endif
