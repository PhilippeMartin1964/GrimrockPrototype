#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPGMON155TestHelpers.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatReactionResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"
#include "Runtime/Monsters/GridMonsterRangedAttackPlanner.h"
#include "RPG/RPGResourceRules.h"
#include "RPG/RPGSelectionRules.h"
#include "Save/GridPartySaveProjections.h"
#include "Runtime/Combat/GridCombatTurnPlan.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridInvariantRotations, "Grimrock.Invariants.Rotations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridInvariantRotations::RunTest(const FString&)
{
	const EGridEdge Directions[] = { EGridEdge::North, EGridEdge::East, EGridEdge::South, EGridEdge::West };
	const FIntPoint Cells[] = { { 0, 3 }, { 3, 0 }, { 0, -3 }, { -3, 0 } };
	const FGuid Source = FGuid::NewGuid();
	FGridMonsterAttackDefinition Attack;
	Attack.AttackId = TEXT("InvariantShot");
	Attack.DisplayName = FText::FromString(TEXT("Shot"));
	Attack.MinDamage = Attack.MaxDamage = 2;
	Attack.MinRangeCells = 2;
	Attack.MaxRangeCells = 6;
	Attack.Delivery = EGridMonsterAttackDelivery::Projectile;
	Attack.ActionPointCost = 1;
	for (int32 From = 0; From < 5; ++From)
	{
		for (int32 To = 0; To < 4; ++To)
		{
			const int32 Start = From == 4 ? 0 : From;
			const int32 Delta = (To - Start + 4) % 4;
			const int32 Turns = Delta == 3 ? 1 : Delta;
			TArray<FGridCombatAction> Actions;
			TestTrue(TEXT("Every cardinal shot is legal"),
				FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(
					Source, FIntPoint::ZeroValue, From == 4 ? EGridEdge::None : Directions[From], Cells[To], 1, Attack, true, Actions));
			TestEqual(TEXT("Shortest turn count; None starts North"), Actions.Num(), Turns + 1);
			for (int32 I = 0; I < Turns && Actions.IsValidIndex(I); ++I)
			{
				const int32 Facing = Delta == 3 ? (Start + 3) % 4 : (Start + I + 1) % 4;
				TestEqual(TEXT("Turn order including left-left half turn"), Actions[I].TargetCell,
					FGridMonsterPathfinder::GetNeighborCell(FIntPoint::ZeroValue, Directions[Facing]));
				TestEqual(TEXT("Turns are free"), Actions[I].ActionPointCost, 0);
				TestEqual(TEXT("Source identity is preserved"), Actions[I].SourceActorId, Source);
				TestFalse(TEXT("Stationary turns are not repositioning"), Actions[I].bIsRepositioningAction);
			}
			if (!Actions.IsEmpty())
				TestEqual(TEXT("Attack follows every turn"), Actions.Last().Type, EGridCombatActionType::RangedAttack);
		}
	}
	TArray<FGridCombatAction> Actions;
	TestFalse(TEXT("Same-cell target is rejected"),
		FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(
			Source, FIntPoint::ZeroValue, EGridEdge::North, FIntPoint::ZeroValue, 1, Attack, true, Actions));
	TestTrue(TEXT("Rejected plan is empty"), Actions.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridInvariantResistance, "Grimrock.Invariants.Resistances", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridInvariantResistance::RunTest(const FString&)
{
	const EGridDamageType Types[] = { EGridDamageType::Physical, EGridDamageType::Fire, EGridDamageType::Ice, EGridDamageType::Lightning,
		EGridDamageType::Poison, EGridDamageType::Holy, EGridDamageType::Necrotic, EGridDamageType::Arcane };
	FGridDamageResistanceSet R;
	int32* Fields[] = { &R.PhysicalResistance, &R.FireResistance, &R.IceResistance, &R.LightningResistance, &R.PoisonResistance, &R.HolyResistance,
		&R.NecroticResistance, &R.ArcaneResistance };
	for (int32 I = 0; I < 8; ++I)
		*Fields[I] = I + 11;
	for (int32 I = 0; I < 8; ++I)
		TestEqual(TEXT("Eight distinct resistance mappings"), FGridCombatResolver::GetResistancePercent(R, Types[I]), I + 11);
	for (const int32 Value : { MIN_int32, -100, -1, 0, 1, 50, 100, 101, MAX_int32 })
	{
		for (int32 I = 0; I < 8; ++I)
		{
			*Fields[I] = Value;
			TestEqual(TEXT("Read preserves raw bounds; downstream damage owns clamping"), FGridCombatResolver::GetResistancePercent(R, Types[I]), Value);
		}
	}
	TestEqual(TEXT("Unknown type has no resistance"), FGridCombatResolver::GetResistancePercent(R, static_cast<EGridDamageType>(255)), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridInvariantRequirements, "Grimrock.Invariants.OwnerRequirements", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridInvariantRequirements::RunTest(const FString&)
{
	FMON155RuntimeStateGuard Guard;
	URPGClassAsset* Class = nullptr;
	auto* Inventory = MakeMON155Inventory(3, 3000, Class);
	auto& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	TArray<FGridCombatModifierProfile> Modifiers;
	TArray<FGridCombatReactionBinding> Reactions;
	const TArray<TArray<FName>> Selections = { {}, { TEXT("Choice_A") }, { NAME_None }, { TEXT("Choice_A"), TEXT("Choice_A") }, { TEXT("MissingChoice") },
		{ TEXT("Choice_B") } };
	for (int32 I = 0; I < Selections.Num(); ++I)
	{
		Character.SelectedClassProgressionChoiceIds = Selections[I];
		const bool Expected = I < 2;
		TestEqual(
			TEXT("Modifiers reject invalid owner selection"), FGridCombatModifierResolver::CollectCharacterChoiceModifiers(Character, Modifiers), Expected);
		TestEqual(
			TEXT("Reactions apply the same owner prerequisite gate"), FGridCombatReactionResolver::CollectCharacterBindings(Character, Reactions), Expected);
	}
	Character.ClassDefinition = nullptr;
	Character.SelectedClassProgressionChoiceIds.Reset();
	TestTrue(TEXT("Missing class without choices remains allowed"), FGridCombatReactionResolver::CollectCharacterBindings(Character, Reactions));
	Character.SelectedClassProgressionChoiceIds = { TEXT("Choice_A") };
	TestFalse(TEXT("Missing class with choices is refused"), FGridCombatModifierResolver::CollectCharacterChoiceModifiers(Character, Modifiers));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridInvariantSelection, "Grimrock.Invariants.SelectionAtomicity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridInvariantSelection::RunTest(const FString&)
{
	FMON155RuntimeStateGuard Guard;
	URPGClassAsset* Class = nullptr;
	auto* Inventory = MakeMON155Inventory(3, 3000, Class);
	const FGuid Id = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
	for (const TArray<FName>& Request : TArray<TArray<FName>>{ { NAME_None }, { TEXT("Choice_A"), TEXT("Choice_A") }, { TEXT("MissingChoice") } })
	{
		FRPGClassProgressionCommitResult Result;
		TestFalse(TEXT("Invalid request refuses before mutation"), FRPGClassProgressionTransactionService::TryCommitChoices(Inventory, 0, Request, Result));
		TestTrue(TEXT("No partial selection"), Inventory->PartyInventoryState.ActiveCharacters[0].SelectedClassProgressionChoiceIds.IsEmpty());
		TestEqual(TEXT("Persistent identity unchanged"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId, Id);
	}
	Inventory->PartyInventoryState.ActiveCharacters[0].SelectedClassProgressionChoiceIds = { TEXT("Choice_A"), TEXT("Choice_A") };
	FText Error;
	TestFalse(
		TEXT("Duplicate persistent choices reject projection rebuild"), FRPGClassProgressionTransactionService::RebuildRuntimeProjection(Inventory, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridInvariantResources, "Grimrock.Invariants.ResourceDeficits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridInvariantResources::RunTest(const FString&)
{
	FMON155RuntimeStateGuard Guard;
	// New level-two maxima are 20 HP / 10 mana. Rows include decreasing maxima,
	// defeated characters and out-of-range previous values accepted by the existing path.
	struct FCase
	{
		int32 Max, Current, Health, Mana;
	};
	const FCase Cases[] = { { 10, 10, 20, 10 }, { 10, 7, 17, 7 }, { 10, 0, 0, 0 }, { 30, 25, 15, 5 }, { 30, 5, 0, 0 }, { 10, 99, 20, 10 }, { 10, -1, 0, 0 },
		{ 0, 0, 0, 10 } };
	for (const auto& Row : Cases)
	{
		URPGClassAsset* Class = nullptr;
		auto* Inventory = MakeMON155Inventory(1, 1000, Class);
		Class->HealthAtLevelOne = 13; // Constitution 12: +1, level-two gain 5+1.
		Class->ManaAtLevelOne = 8;
		auto& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
		Character.DerivedStats.MaxHealth = Character.DerivedStats.MaxMana = Row.Max;
		Character.Resources.CurrentHealth = Character.Resources.CurrentMana = Row.Current;
		TestTrue(TEXT("Existing level-up commits"), FRPGLevelUpService::ApplyPendingLevelUp(Inventory, 0, false));
		TestEqual(TEXT("New HP maximum"), Character.DerivedStats.MaxHealth, 20);
		TestEqual(TEXT("Health deficit and defeated state preserved"), Character.Resources.CurrentHealth, Row.Health);
		TestEqual(TEXT("Mana spent preserved without refund"), Character.Resources.CurrentMana, Row.Mana);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridInvariantResourceBounds, "Grimrock.Invariants.ResourceBounds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridInvariantResourceBounds::RunTest(const FString&)
{
	struct FCase
	{
		int32 Max, Current, NewMax, Health, Mana;
	};
	const FCase Cases[] = { { 10, 7, 20, 17, 17 }, { 20, 17, 10, 7, 7 }, { 20, 3, 10, 0, 0 }, { 10, 0, 20, 0, 10 }, { 10, 99, 5, 5, 5 }, { 10, -1, 5, 0, 0 },
		{ 0, 0, 0, 0, 0 }, { -1, -1, -1, 0, 0 }, { 10, 10, -1, 1, 0 } };
	for (const auto& C : Cases)
	{
		FRPGDerivedStats Stats;
		Stats.MaxHealth = Stats.MaxMana = C.Max;
		FRPGCharacterResources Resources;
		Resources.CurrentHealth = Resources.CurrentMana = C.Current;
		TestEqual(TEXT("HP deficit, floor and death"), RPGResourceRules::PreserveHealthDeficit(Stats, Resources, C.NewMax), C.Health);
		TestEqual(TEXT("Mana deficit and zero floor"), RPGResourceRules::PreserveManaDeficit(Stats, Resources, C.NewMax), C.Mana);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridInvariantSelectionProjection, "Grimrock.Invariants.SelectionProjection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridInvariantSelectionProjection::RunTest(const FString&)
{
	TSet<FName> Output = { TEXT("Stale") };
	TestTrue(TEXT("Empty selection succeeds and clears output"), RPGSelectionRules::BuildSelectionSet({}, Output));
	TestTrue(TEXT("Empty output"), Output.IsEmpty());
	TestTrue(TEXT("Distinct selection succeeds"), RPGSelectionRules::BuildSelectionSet({ TEXT("B"), TEXT("A") }, Output));
	TestEqual(TEXT("Two selected identities"), Output.Num(), 2);
	TestFalse(TEXT("Duplicate fails"), RPGSelectionRules::BuildSelectionSet({ TEXT("A"), TEXT("B"), TEXT("A") }, Output));
	TestTrue(TEXT("Duplicate clears partial output"), Output.IsEmpty());
	TestFalse(TEXT("Empty identity fails"), RPGSelectionRules::BuildSelectionSet({ TEXT("A"), NAME_None }, Output));
	TestTrue(TEXT("Empty identity clears partial output"), Output.IsEmpty());
	FGridPartyInventoryState Party;
	Party.ActiveCharacters.SetNum(2);
	Party.CharacterPool.SetNum(2);
	Party.ActiveCharacters[1].KnownSpellIds = { TEXT("Spell_A"), TEXT("Spell_B") };
	Party.CharacterPool[0].KnownSpellIds = { TEXT("Spell_A") };
	TestEqual(TEXT("Projection counts characters, including pool, rather than spells"), GridPartySaveProjections::CountCharactersWithKnownSpells(Party), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridInvariantTurnBoundaries, "Grimrock.Invariants.TurnBoundaries", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridInvariantTurnBoundaries::RunTest(const FString&)
{
	const FGuid Id = FGuid::NewGuid();
	TArray<FGridCombatAction> Actions;
	EGridEdge Facing = EGridEdge::None;
	const auto Factory = [](const FGuid&, const FIntPoint&, EGridEdge Target)
	{
		FGridCombatAction Action;
		Action.TargetCell = { static_cast<int32>(Target), 0 };
		Action.bIsRepositioningAction = true;
		return Action;
	};
	TestFalse(
		TEXT("No destination rejects without normalizing source"), GridCombatTurnPlan::AppendTurnsToward(Id, {}, EGridEdge::None, Facing, Actions, Factory));
	TestEqual(TEXT("Source remains None"), Facing, EGridEdge::None);
	TestTrue(TEXT("No actions on rejection"), Actions.IsEmpty());
	TestTrue(TEXT("Missing source defaults North"), GridCombatTurnPlan::AppendTurnsToward(Id, {}, EGridEdge::South, Facing, Actions, Factory));
	TestEqual(TEXT("Two left turns"), Actions.Num(), 2);
	if (Actions.Num() == 2)
	{
		TestEqual(TEXT("East before South"), Actions[0].TargetCell.X, static_cast<int32>(EGridEdge::East));
		TestTrue(TEXT("Caller policy survives"), Actions[0].bIsRepositioningAction && Actions[1].bIsRepositioningAction);
	}
	TestFalse(TEXT("Unknown destination preserves former failure semantics"),
		GridCombatTurnPlan::AppendTurnsToward(Id, {}, static_cast<EGridEdge>(255), Facing, Actions, Factory));
	TestEqual(TEXT("Unknown destination still appends two turns"), Actions.Num(), 4);
	return true;
}

#endif
