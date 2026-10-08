#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatReactionResolver.h"
#include "Runtime/Combat/GridCombatSurfaceResolver.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace RPG0396
{
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
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("RPG0396World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
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

	AGridLevelRuntimeActor* MakeRuntime(FTestWorld& TestWorld)
	{
		if (!TestWorld.World)
		{
			return nullptr;
		}
		AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
		if (!Runtime)
		{
			return nullptr;
		}
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
		Level->Width = 3;
		Level->Height = 3;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		Runtime->LevelAsset = Level;
		Runtime->CurrentDungeonLevelId = TEXT("RPG0396_Level");
		return Runtime;
	}

	FGridCombatActionDefinition MakeCatalyst()
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("Action_RPG0396_Catalyst");
		Action.DisplayName = FText::FromString(TEXT("Catalyst"));
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Cell;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 1;
		Action.RangeCells = 4;
		Action.SurfaceInteraction = EGridCombatSurfaceInteraction::AnyCanonical;
		return Action;
	}

	FGridCombatReactionEvent MakeSurfaceEvent(const FGuid& CharacterId, const FGuid& ActionInstanceId, bool bReactionGenerated = false)
	{
		FGridCombatReactionEvent Event;
		Event.EventId = FGuid::NewGuid();
		Event.ActionInstanceId = ActionInstanceId;
		Event.RoundNumber = 2;
		Event.Trigger = EGridCombatReactionTrigger::SurfaceReaction;
		Event.SourceCombatantId = CharacterId;
		Event.TargetCombatantId = CharacterId;
		Event.ActionId = TEXT("Use_Item_Bomb_Fire");
		Event.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
		Event.ActionType = EGridCombatActionType::RangedAttack;
		Event.SourceTags = { TEXT("QuickItem.Alchemy"), TEXT("QuickItem.Bomb") };
		Event.bOffensiveAction = true;
		Event.bReactionGenerated = bReactionGenerated;
		return Event;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396SurfaceTraversalStateTest,
	"Grimrock.RPG.RPG03.9.6A.SurfaceTraversalState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396SurfaceTraversalStateTest::RunTest(const FString&)
{
	FGridCombatSurfaceEffectProfile Oil;
	Oil.SurfaceType = EGridCombatSurfaceType::Oil;
	Oil.DurationRounds = 4;
	Oil.TraversalCostModifier = 1;
	FGridCombatSurfaceState State;
	TestTrue(TEXT("Oil-style traversal profile builds"), FGridCombatSurfaceResolver::BuildState(
		Oil, FGuid::NewGuid(), TEXT("Action_RPG0396_Oil"), FGridResolvedCombatModifiers(), State));
	TestEqual(TEXT("Surface state snapshots +1 traversal cost"), State.TraversalCostModifier, 1);

	FGridCombatSurfaceReactionResult FireReaction;
	TestTrue(TEXT("Oil reacts to Fire"), FGridCombatSurfaceResolver::ResolveReaction(
		State, EGridCombatSurfaceInteraction::Fire, FGridResolvedCombatModifiers(), FireReaction));
	FGridCombatSurfaceResolver::ApplyReactionToState(FireReaction, State);
	TestEqual(TEXT("Reaction output does not inherit Oil traversal penalty"), State.TraversalCostModifier, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396RuntimeTraversalQueryTest,
	"Grimrock.RPG.RPG03.9.6A.RuntimeTraversalQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396RuntimeTraversalQueryTest::RunTest(const FString&)
{
	RPG0396::FTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = RPG0396::MakeRuntime(TestWorld);
	if (!TestNotNull(TEXT("Runtime exists"), Runtime))
	{
		return false;
	}
	FGridCombatSurfaceEffectProfile Oil;
	Oil.SurfaceType = EGridCombatSurfaceType::Oil;
	Oil.DurationRounds = 4;
	Oil.TraversalCostModifier = 1;
	TestTrue(TEXT("Oil surface applies"), Runtime->ApplyCombatSurfaceAtCell(
		1, 1, Oil, FGuid::NewGuid(), TEXT("Action_RPG0396_Oil"), FGridResolvedCombatModifiers()));
	TestEqual(TEXT("Runtime exposes surface traversal budget"), Runtime->GetCombatSurfaceTraversalCostModifierAtCell(1, 1), 1);
	TestEqual(TEXT("Empty cell has no traversal surcharge"), Runtime->GetCombatSurfaceTraversalCostModifierAtCell(0, 0), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396AnyCanonicalReactionTest,
	"Grimrock.RPG.RPG03.9.6A.AnyCanonicalReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396AnyCanonicalReactionTest::RunTest(const FString&)
{
	FGridCombatSurfaceState Water;
	Water.SurfaceType = EGridCombatSurfaceType::Water;
	Water.RemainingRounds = 3;
	Water.SourceCombatantId = FGuid::NewGuid();
	Water.SourceActionId = TEXT("Action_Water");

	FGridCombatSurfaceReactionResult Reaction;
	TestTrue(TEXT("AnyCanonical finds a Water reaction"), FGridCombatSurfaceResolver::ResolveReaction(
		Water, EGridCombatSurfaceInteraction::AnyCanonical, FGridResolvedCombatModifiers(), Reaction));
	TestEqual(TEXT("Deterministic priority resolves Water through Ice before Lightning"),
		Reaction.ResolvedInteraction, EGridCombatSurfaceInteraction::Ice);
	TestEqual(TEXT("Water becomes Ice"), Reaction.OutputSurfaceType, EGridCombatSurfaceType::Ice);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396CatalystActionValidationTest,
	"Grimrock.RPG.RPG03.9.6A.CatalystActionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396CatalystActionValidationTest::RunTest(const FString&)
{
	FGridCombatActionDefinition Catalyst = RPG0396::MakeCatalyst();
	TestTrue(TEXT("Cell Effect with AnyCanonical surface interaction is structurally valid"), Catalyst.IsValid());
	Catalyst.TargetingPolicy = EGridCombatTargetingPolicy::Self;
	TestFalse(TEXT("Surface interaction cannot be authored on Self targeting"), Catalyst.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396CatalystCatalogueTest,
	"Grimrock.RPG.RPG03.9.6A.CatalystCatalogue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396CatalystCatalogueTest::RunTest(const FString&)
{
	FGridCombatActionContribution Contribution;
	Contribution.Definition = RPG0396::MakeCatalyst();
	Contribution.SourceDefinitionId = TEXT("Alchemist");

	FGridCombatActionCatalogContext Context;
	Context.CharacterIndex = 0;
	Context.CharacterId = FGuid::NewGuid();
	Context.bCombatActive = true;
	Context.bActiveCombatant = true;
	Context.bEnableClassActionExecutors = true;
	Context.RemainingActionPoints = 4;
	Context.CurrentHealth = 10;
	Context.MaximumHealth = 10;

	TArray<FGridAvailableCombatAction> Actions;
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestEqual(TEXT("Catalyst-style interaction action remains in the generic catalogue"), Actions.Num(), 1);
	TestTrue(TEXT("Catalogue recognizes the generic surface-interaction executor"), Actions.Num() == 1 && Actions[0].bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396ChainReactionLedgerTest,
	"Grimrock.RPG.RPG03.9.6A.ChainReactionLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396ChainReactionLedgerTest::RunTest(const FString&)
{
	FGridCombatReactionProfile Profile;
	Profile.ReactionId = TEXT("Reaction_RPG0396_Chain");
	Profile.Trigger = EGridCombatReactionTrigger::SurfaceReaction;
	Profile.Limit = EGridCombatReactionLimit::OncePerAction;
	Profile.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
	Profile.RequiredSourceTags = { TEXT("QuickItem.Bomb") };
	Profile.SurfaceReactionDamagePercentModifier = 25;
	Profile.SurfaceReactionAreaRadiusModifier = 1;
	TestTrue(TEXT("Surface-reaction response profile is structurally valid"), Profile.IsValid());

	FGridCombatReactionBinding Binding;
	Binding.Profile = Profile;
	const FGuid Owner = FGuid::NewGuid();
	const FGuid ActionInstance = FGuid::NewGuid();
	FGridCombatReactionLedger Ledger;
	TArray<FGridCombatReactionMatch> Matches;

	FGridCombatReactionEvent First = RPG0396::MakeSurfaceEvent(Owner, ActionInstance);
	FGridCombatReactionResolver::ResolveMatches({ Binding }, Owner, First, Ledger, true, Matches);
	TestEqual(TEXT("First surface reaction in action consumes the once-per-action slot"), Matches.Num(), 1);
	TestTrue(TEXT("Response carries +25 percent and +1 radius"), Matches.Num() == 1 &&
		Matches[0].SurfaceReactionDamagePercentModifier == 25 &&
		Matches[0].SurfaceReactionAreaRadiusModifier == 1);

	FGridCombatReactionEvent Second = RPG0396::MakeSurfaceEvent(Owner, ActionInstance);
	FGridCombatReactionResolver::ResolveMatches({ Binding }, Owner, Second, Ledger, true, Matches);
	TestEqual(TEXT("Second surface reaction in same action is not bonused"), Matches.Num(), 0);

	FGridCombatReactionEvent NextAction = RPG0396::MakeSurfaceEvent(Owner, FGuid::NewGuid());
	FGridCombatReactionResolver::ResolveMatches({ Binding }, Owner, NextAction, Ledger, true, Matches);
	TestEqual(TEXT("A new action may trigger the response again"), Matches.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396ReactionRecursionGuardTest,
	"Grimrock.RPG.RPG03.9.6A.ReactionRecursionGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396ReactionRecursionGuardTest::RunTest(const FString&)
{
	FGridCombatReactionProfile Profile;
	Profile.ReactionId = TEXT("Reaction_RPG0396_NoRecursiveChain");
	Profile.Trigger = EGridCombatReactionTrigger::SurfaceReaction;
	Profile.Limit = EGridCombatReactionLimit::OncePerAction;
	Profile.SurfaceReactionDamagePercentModifier = 25;

	const FGuid Owner = FGuid::NewGuid();
	const FGridCombatReactionEvent Generated = RPG0396::MakeSurfaceEvent(Owner, FGuid::NewGuid(), true);
	TestFalse(TEXT("Reaction-generated surface events are rejected by default"), FGridCombatReactionResolver::Matches(Profile, Generated));
	Profile.bAllowReactionGeneratedEvents = true;
	TestTrue(TEXT("Generic opt-in still exists for future explicit recursive designs"), FGridCombatReactionResolver::Matches(Profile, Generated));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396ConversionTraversalTest,
	"Grimrock.RPG.RPG03.9.6A.ConversionTraversal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396ConversionTraversalTest::RunTest(const FString&)
{
	FGridCombatSurfaceConversionProfile ToOil;
	ToOil.bAllowEmptyCell = true;
	ToOil.OutputSurfaceType = EGridCombatSurfaceType::Oil;
	ToOil.EmptyCellDurationRounds = 4;
	ToOil.OutputTraversalCostModifier = 1;
	FGridCombatSurfaceState State;
	TestTrue(TEXT("Converted Oil may author the same traversal surcharge"), FGridCombatSurfaceResolver::ResolveConversion(
		ToOil, nullptr, FGuid::NewGuid(), TEXT("Action_RPG0396_Transmute"), FGridResolvedCombatModifiers(), State));
	TestEqual(TEXT("Conversion snapshots output traversal cost"), State.TraversalCostModifier, 1);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396SurfaceEffectReactionBridgeTest,
	"Grimrock.RPG.RPG03.9.6A.SurfaceEffectReactionBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396SurfaceEffectReactionBridgeTest::RunTest(const FString&)
{
	FGridCombatSurfaceState Poison;
	Poison.SurfaceType = EGridCombatSurfaceType::Poison;
	Poison.RemainingRounds = 3;
	Poison.SourceCombatantId = FGuid::NewGuid();
	Poison.SourceActionId = TEXT("Action_Poison");

	FGridCombatSurfaceEffectProfile Fire;
	Fire.SurfaceType = EGridCombatSurfaceType::Fire;
	Fire.DurationRounds = 2;

	FGridResolvedCombatModifiers Modifiers;
	Modifiers.SurfaceReactionDamagePercentModifier = 25;
	Modifiers.SurfaceReactionAreaRadiusModifier = 1;

	FGridCombatSurfaceReactionResult Reaction;
	TestTrue(TEXT("Incoming Fire surface resolves the canonical Poison + Fire reaction"),
		FGridCombatSurfaceResolver::ResolveAppliedSurfaceReaction(Poison, Fire, Modifiers, Reaction));
	TestEqual(TEXT("Incoming Fire maps to the canonical Fire interaction"),
		Reaction.ResolvedInteraction, EGridCombatSurfaceInteraction::Fire);
	TestTrue(TEXT("Poison plus incoming Fire is explosive"), Reaction.bExplosive);
	TestEqual(TEXT("Reaction carries the Chain Reaction damage modifier"),
		Reaction.ExplosionDamagePercentModifier, 25);
	TestEqual(TEXT("Reaction carries the Chain Reaction radius modifier"),
		Reaction.ExplosionAreaRadiusModifier, 1);

	FGridCombatSurfaceState Ice = Poison;
	Ice.SurfaceType = EGridCombatSurfaceType::Ice;
	TestTrue(TEXT("Incoming Fire also resolves Ice + Fire"),
		FGridCombatSurfaceResolver::ResolveAppliedSurfaceReaction(Ice, Fire, FGridResolvedCombatModifiers(), Reaction));
	TestEqual(TEXT("Ice plus Fire canonically outputs Water"),
		Reaction.OutputSurfaceType, EGridCombatSurfaceType::Water);

	FGridCombatSurfaceEffectProfile Toxic;
	Toxic.SurfaceType = EGridCombatSurfaceType::Poison;
	Toxic.DurationRounds = 3;
	TestFalse(TEXT("Poison surface placement does not invent a non-canonical Poison interaction"),
		FGridCombatSurfaceResolver::ResolveAppliedSurfaceReaction(Ice, Toxic, FGridResolvedCombatModifiers(), Reaction));
	return true;
}

#endif
