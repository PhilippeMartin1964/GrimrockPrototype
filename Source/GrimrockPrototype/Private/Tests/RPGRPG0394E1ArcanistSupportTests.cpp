#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatMovementResolver.h"
#include "Runtime/Combat/GridCombatTargetingResolver.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace RPG0394E1
{
	FGridStatusEffectRuntimeState MakeStatus(
		UGridStatusEffectDefinitionAsset* Definition,
		FName EffectId,
		EGridStatusEffectDisposition Disposition,
		int32 Potency,
		const TArray<FName>& Tags)
	{
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromName(EffectId);
		Definition->Disposition = Disposition;
		Definition->DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		Definition->DefaultDuration = 2;
		Definition->StatusTags = Tags;

		FGridStatusEffectRuntimeState State;
		State.EffectId = EffectId;
		State.SourceId = FGuid::NewGuid();
		State.StackCount = 1;
		State.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		State.RemainingDuration = 2;
		State.Potency = Potency;
		State.DefinitionAsset = Definition;
		return State;
	}

	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);
			World = UWorld::CreateWorld(
				EWorldType::Game,
				false,
				FName(*FString::Printf(TEXT("RPG0394E1World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr,
				true,
				ERHIFeatureLevel::Num,
				&Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTestWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}
	};

	struct FMovementFixture
	{
		FTestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		UGridLevelAsset* Level = nullptr;
		UGridMonsterOccupancySubsystem* Occupancy = nullptr;

		FMovementFixture()
		{
			if (!TestWorld.World)
			{
				return;
			}
			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			if (!Runtime)
			{
				return;
			}

			Level = NewObject<UGridLevelAsset>(Runtime);
			Level->Width = 4;
			Level->Height = 4;
			Level->EnsureCellCount();
			for (FGridLevelCellData& Cell : Level->Cells)
			{
				Cell.CellType = EGridCellType::Floor;
				Cell.bBlocksOccupancy = false;
			}
			Runtime->LevelAsset = Level;
			Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPG0394E1StatusRemovalContractTest,
	"Grimrock.RPG.RPG03.9.4E1.StatusRemovalContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394E1StatusRemovalContractTest::RunTest(const FString&)
{
	using namespace RPG0394E1;

	FGridCombatStatusRemovalProfile AllyDebuff;
	AllyDebuff.AnyStatusTags = { TEXT("Dispel.Magical") };
	AllyDebuff.AllowedDispositions = { EGridStatusEffectDisposition::Debuff };
	AllyDebuff.TargetSide = EGridCombatStatusRemovalTargetSide::Party;
	AllyDebuff.MaximumRemovals = 1;

	FGridCombatStatusRemovalProfile HostileBuff;
	HostileBuff.AnyStatusTags = { TEXT("Dispel.Magical") };
	HostileBuff.AllowedDispositions = { EGridStatusEffectDisposition::Buff };
	HostileBuff.TargetSide = EGridCombatStatusRemovalTargetSide::Hostile;
	HostileBuff.MaximumRemovals = 1;

	FGridCombatActionDefinition Action;
	Action.ActionId = TEXT("Action_Test_AllyOrHostile");
	Action.DisplayName = FText::FromString(TEXT("Ally Or Hostile"));
	Action.ActionType = EGridCombatActionType::Ability;
	Action.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	Action.TargetingPolicy = EGridCombatTargetingPolicy::AllyOrHostile;
	Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
	Action.ActionPointCost = 2;
	Action.ResourceCosts.ManaCost = 6;
	Action.RangeCells = 4;
	Action.StatusRemovals = { AllyDebuff, HostileBuff };
	TestTrue(TEXT("One Effect action can legally target AllyOrHostile"), Action.IsValid());

	UGridStatusEffectDefinitionAsset* High = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	UGridStatusEffectDefinitionAsset* Alpha = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	UGridStatusEffectDefinitionAsset* Zeta = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	UGridStatusEffectDefinitionAsset* Buff = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());

	FGridStatusEffectCollection Effects;
	Effects.ActiveEffects = {
		MakeStatus(High, TEXT("Status_High"), EGridStatusEffectDisposition::Debuff, 9, { TEXT("Dispel.Magical") }),
		MakeStatus(Zeta, TEXT("Status_Zeta"), EGridStatusEffectDisposition::Debuff, 7, { TEXT("Dispel.Magical") }),
		MakeStatus(Alpha, TEXT("Status_Alpha"), EGridStatusEffectDisposition::Debuff, 7, { TEXT("Dispel.Magical") }),
		MakeStatus(Buff, TEXT("Status_Buff"), EGridStatusEffectDisposition::Buff, 12, { TEXT("Dispel.Magical") })
	};

	TArray<FName> Removed;
	FGridCombatTargetingResolver::CollectStatusRemovalIds(
		Effects, Action.StatusRemovals, Removed, EGridCombatStatusRemovalTargetSide::Party);
	TestEqual(TEXT("Party side removes exactly one matching Debuff"), Removed.Num(), 1);
	TestEqual(TEXT("Highest Potency wins first"), Removed.Num() == 1 ? Removed[0] : NAME_None, FName(TEXT("Status_High")));

	Effects.ActiveEffects[0].Potency = 7;
	FGridCombatTargetingResolver::CollectStatusRemovalIds(
		Effects, Action.StatusRemovals, Removed, EGridCombatStatusRemovalTargetSide::Party);
	TestEqual(TEXT("Equal Potency falls back to lexical EffectId"), Removed.Num() == 1 ? Removed[0] : NAME_None, FName(TEXT("Status_Alpha")));

	FGridCombatTargetingResolver::CollectStatusRemovalIds(
		Effects, Action.StatusRemovals, Removed, EGridCombatStatusRemovalTargetSide::Hostile);
	TestEqual(TEXT("Hostile side selects the Buff profile only"), Removed.Num(), 1);
	TestEqual(TEXT("Hostile side removes the magical Buff"), Removed.Num() == 1 ? Removed[0] : NAME_None, FName(TEXT("Status_Buff")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPG0394E1SemanticTargetTagsTest,
	"Grimrock.RPG.RPG03.9.4E1.SemanticTargetTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394E1SemanticTargetTagsTest::RunTest(const FString&)
{
	FGridCombatModifierProfile Profile;
	Profile.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
	Profile.RequiredSourceTags = { TEXT("Spell.School.Arcane") };
	Profile.AnyTargetSemanticTags = { TEXT("Rune"), TEXT("Construct") };
	Profile.OutgoingDamagePercentModifier = 20;
	TestTrue(TEXT("Semantic-target modifier profile is valid"), Profile.IsValid());

	FGridCombatActionDefinition Action;
	Action.ActionId = TEXT("Spell_Test_Arcane");
	Action.ActionType = EGridCombatActionType::Ability;
	Action.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	Action.SourceTags = { TEXT("Spell.School.Arcane") };

	FGridStatusEffectCollection NoStatuses;
	FGridCombatModifierContext Context =
		FGridCombatModifierResolver::MakeActionContext(Action, Action.ActionId);
	FGridCombatModifierResolver::AddTargetStatusContext(
		Context, NoStatuses, FGuid::NewGuid(), TEXT("Construct"));

	FGridResolvedCombatModifiers Resolved;
	FGridCombatModifierResolver::Resolve({ Profile }, Context, Resolved);
	TestEqual(TEXT("CategoryId is exposed as a semantic target tag"), Resolved.OutgoingDamagePercentModifier, 20);

	Context = FGridCombatModifierResolver::MakeActionContext(Action, Action.ActionId);
	FGridCombatModifierResolver::AddTargetStatusContext(
		Context, NoStatuses, FGuid::NewGuid(), TEXT("Other"), { FName(TEXT("Rune")) });
	FGridCombatModifierResolver::Resolve({ Profile }, Context, Resolved);
	TestEqual(TEXT("Explicit Rune semantic tag matches"), Resolved.OutgoingDamagePercentModifier, 20);

	Context = FGridCombatModifierResolver::MakeActionContext(Action, Action.ActionId);
	FGridCombatModifierResolver::AddTargetStatusContext(
		Context, NoStatuses, FGuid::NewGuid(), TEXT("Vermin"));
	FGridCombatModifierResolver::Resolve({ Profile }, Context, Resolved);
	TestEqual(TEXT("Unrelated target does not match"), Resolved.OutgoingDamagePercentModifier, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPG0394E1MinimumManaCostTest,
	"Grimrock.RPG.RPG03.9.4E1.MinimumManaCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394E1MinimumManaCostTest::RunTest(const FString&)
{
	FGridCombatModifierProfile Profile;
	Profile.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
	Profile.RequiredSourceTags = { TEXT("Spell.School.Arcane") };
	Profile.ManaCostModifier = -1;
	Profile.MinimumManaCost = 1;
	Profile.RangeCellsModifier = 1;
	TestTrue(TEXT("Arcane cost/range profile is valid"), Profile.IsValid());

	FGridCombatActionDefinition Spell;
	Spell.ActionId = TEXT("Spell_Test_Arcane");
	Spell.ActionType = EGridCombatActionType::Ability;
	Spell.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	Spell.SourceTags = { TEXT("Spell.School.Arcane") };
	Spell.ResourceCosts.ManaCost = 1;
	Spell.RangeCells = 32;

	FGridResolvedCombatModifiers Resolved;
	FGridCombatModifierResolver::Resolve(
		{ Profile }, FGridCombatModifierResolver::MakeActionContext(Spell, Spell.ActionId), Resolved);
	FGridCombatModifierResolver::ApplyToActionDefinitionProjection(Spell, Resolved);
	TestEqual(TEXT("Positive one-mana action stays at minimum one"), Spell.ResourceCosts.ManaCost, 1);
	TestEqual(TEXT("Range remains capped at 32"), Spell.RangeCells, 32);

	FGridCombatActionDefinition FreeSpell = Spell;
	FreeSpell.ResourceCosts.ManaCost = 0;
	FGridCombatModifierResolver::ApplyToActionDefinitionProjection(FreeSpell, Resolved);
	TestEqual(TEXT("Zero-cost action remains free"), FreeSpell.ResourceCosts.ManaCost, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPG0394E1SelectedCellRelocationTest,
	"Grimrock.RPG.RPG03.9.4E1.SelectedCellRelocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394E1SelectedCellRelocationTest::RunTest(const FString&)
{
	using namespace RPG0394E1;
	FMovementFixture Fixture;
	if (!Fixture.Runtime || !Fixture.Level)
	{
		return false;
	}

	FGridCombatActionDefinition Action;
	Action.ActionId = TEXT("Action_Test_Relocation");
	Action.DisplayName = FText::FromString(TEXT("Relocation"));
	Action.ActionType = EGridCombatActionType::Ability;
	Action.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	Action.TargetingPolicy = EGridCombatTargetingPolicy::Cell;
	Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
	Action.ActionPointCost = 3;
	Action.ResourceCosts.ManaCost = 8;
	Action.RangeCells = 2;
	Action.bRelocatePartyToTargetCell = true;
	TestTrue(TEXT("Selected-cell relocation action is structurally valid"), Action.IsValid());

	TestTrue(TEXT("Open destination two cells away is reachable"),
		FGridCombatMovementResolver::CanRelocatePartyToSelectedCell(
			FIntPoint(1, 1), FIntPoint(1, 3), 2, Fixture.Runtime, Fixture.Occupancy));

	Fixture.Level->GetCellMutable(1, 1).NorthWall = EGridWallType::Solid;
	TestFalse(TEXT("Solid wall blocks selected-cell relocation"),
		FGridCombatMovementResolver::CanRelocatePartyToSelectedCell(
			FIntPoint(1, 1), FIntPoint(1, 2), 2, Fixture.Runtime, Fixture.Occupancy));

	Fixture.Level->GetCellMutable(1, 1).NorthWall = EGridWallType::None;
	TestFalse(TEXT("Range limit is enforced"),
		FGridCombatMovementResolver::CanRelocatePartyToSelectedCell(
			FIntPoint(0, 0), FIntPoint(3, 0), 2, Fixture.Runtime, Fixture.Occupancy));
	return true;
}

#endif
