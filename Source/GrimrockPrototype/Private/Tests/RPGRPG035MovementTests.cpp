#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatMovementResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace RPG035
{
	FGridCombatMovementEffectProfile MakeRetreatProfile(bool bForced = false)
	{
		FGridCombatMovementEffectProfile Profile;
		Profile.Subject = EGridCombatMovementSubject::PartyGroup;
		Profile.Direction = EGridCombatMovementDirection::BackwardFromFacing;
		Profile.DistanceCells = 1;
		Profile.MobilityActionPointCost = 1;
		Profile.bForced = bForced;
		return Profile;
	}

	FGridCombatActionDefinition MakeRetreatAction()
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("Action_Ranger_TacticalRetreat");
		Action.DisplayName = FText::FromString(TEXT("Tactical Retreat"));
		Action.Description = FText::FromString(TEXT("Move the whole party backward by one cell."));
		Action.ActionType = EGridCombatActionType::Retreat;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Self;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 1;
		Action.CooldownRounds = 3;
		Action.MovementEffects.Add(MakeRetreatProfile());
		return Action;
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
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("RPG035World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num, &Values);
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

	struct FFixture
	{
		FTestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		AActor* Owner = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		URPGClassAsset* Class = nullptr;
		FGuid CharacterId = FGuid(3, 5, 1, 1);

		FFixture()
		{
			if (!TestWorld.World)
			{
				return;
			}
			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			Owner = TestWorld.World->SpawnActor<AActor>();
			if (!Runtime || !Party || !Party->PartyInventoryComponent || !Owner)
			{
				return;
			}

			UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(Runtime);
			LevelAsset->Width = 3;
			LevelAsset->Height = 3;
			LevelAsset->EnsureCellCount();
			for (FGridLevelCellData& Cell : LevelAsset->Cells)
			{
				Cell.CellType = EGridCellType::Floor;
				Cell.bBlocksOccupancy = false;
			}
			Runtime->LevelAsset = LevelAsset;

			Class = NewObject<URPGClassAsset>(GetTransientPackage());
			Class->ClassId = TEXT("RPG035_Ranger");
			Class->HealthAtLevelOne = 10;
			Class->CombatActions = { MakeRetreatAction() };

			FGridCharacterInventoryState Character;
			Character.CharacterId = CharacterId;
			Character.DisplayName = FText::FromString(TEXT("RPG03.5 Ranger"));
			Character.ClassId = Class->ClassId;
			Character.ClassDefinition = TSoftObjectPtr<URPGClassAsset>(Class);
			Character.DerivedStats.MaxHealth = 10;
			Character.Resources.CurrentHealth = 10;
			Character.InventorySlots.SetNum(4);
			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(1);

			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 1;
			Party->CurrentCellY = 1;
			Party->Facing = EGridEdge::North;
			Party->SnapToCurrentCell();

			TurnManager = NewObject<UGridTurnManagerComponent>(Owner, TEXT("RPG035TurnManager"));
			if (!TurnManager || !TurnManager->InitializeTurnManager(Runtime, Party))
			{
				TurnManager = nullptr;
				return;
			}
			TurnManager->bCombatActive = true;
			TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
			TurnManager->RoundNumber = 1;
			TurnManager->PartyMobilityState.RoundNumber = 1;
			TurnManager->PartyMobilityState.MaximumMobilityActionPoints = 2;
			TurnManager->PartyMobilityState.RemainingMobilityActionPoints = 2;

			FGridCombatantInitiativeEntry Entry;
			Entry.CombatantId = CharacterId;
			Entry.Side = EGridCombatantSide::Party;
			Entry.CharacterIndex = 0;
			Entry.DisplayName = Character.DisplayName;
			Entry.InitiativeTotal = 20;
			Entry.CurrentHealth = 10;
			Entry.MaximumHealth = 10;
			Entry.State = EGridCombatantTurnState::Active;
			TurnManager->InitiativeOrder = { Entry };
			TurnManager->CurrentInitiativeIndex = 0;

			FGridPlayerCharacterTurnState TurnState;
			TurnState.CharacterIndex = 0;
			TurnState.CharacterId = CharacterId;
			TurnState.State = EGridCombatantTurnState::Active;
			TurnState.MaximumActionPoints = 4;
			TurnState.RemainingActionPoints = 4;
			TurnManager->PlayerCharacterTurnStates = { TurnState };
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG035ProfileValidationTest, "Grimrock.RPG.RPG03.5.ProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG035ProfileValidationTest::RunTest(const FString&)
{
	FGridCombatMovementEffectProfile Profile = RPG035::MakeRetreatProfile();
	TestTrue(TEXT("Party retreat profile is valid"), Profile.IsValid());
	Profile.Direction = EGridCombatMovementDirection::AwayFromSource;
	TestFalse(TEXT("Party-group AwayFromSource is rejected"), Profile.IsValid());

	Profile.Subject = EGridCombatMovementSubject::TargetCombatant;
	Profile.MobilityActionPointCost = 1;
	TestFalse(TEXT("Target forced movement cannot spend party PAM"), Profile.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG035DirectionResolutionTest, "Grimrock.RPG.RPG03.5.DirectionResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG035DirectionResolutionTest::RunTest(const FString&)
{
	const FIntPoint Cell(1, 1);
	TestEqual(TEXT("Backward from north is south"),
		FGridCombatMovementResolver::ResolveDirection(EGridCombatMovementDirection::BackwardFromFacing, EGridEdge::North, Cell, Cell), EGridEdge::South);
	TestEqual(TEXT("Left from north follows project canonical rotation"),
		FGridCombatMovementResolver::ResolveDirection(EGridCombatMovementDirection::LeftFromFacing, EGridEdge::North, Cell, Cell), EGridEdge::East);
	TestEqual(TEXT("Away from western source is east"),
		FGridCombatMovementResolver::ResolveDirection(EGridCombatMovementDirection::AwayFromSource, EGridEdge::North, FIntPoint(2, 1), FIntPoint(1, 1)),
		EGridEdge::East);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG035DestinationResolutionTest, "Grimrock.RPG.RPG03.5.DestinationResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG035DestinationResolutionTest::RunTest(const FString&)
{
	RPG035::FFixture Fixture;
	if (!Fixture.Runtime || !Fixture.TestWorld.World)
	{
		return false;
	}
	const UGridMonsterOccupancySubsystem* Occupancy = Fixture.TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	FGridCombatMovementResolution Resolution;
	TestTrue(TEXT("Retreat resolves through normal grid geometry"),
		FGridCombatMovementResolver::ResolveDestination(
			RPG035::MakeRetreatProfile(), FIntPoint(1, 1), EGridEdge::North, FIntPoint(1, 1), Fixture.Runtime, Occupancy, Resolution));
	TestEqual(TEXT("Retreat target is one southern cell"), Resolution.ToCell, FIntPoint(1, 0));
	TestEqual(TEXT("Resolved distance is one"), Resolution.DistanceCells, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG035BlockedGeometryTest, "Grimrock.RPG.RPG03.5.BlockedGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG035BlockedGeometryTest::RunTest(const FString&)
{
	RPG035::FFixture Fixture;
	if (!Fixture.Runtime || !Fixture.TestWorld.World)
	{
		return false;
	}
	FGridCombatMovementResolution Resolution;
	const UGridMonsterOccupancySubsystem* Occupancy = Fixture.TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	TestFalse(TEXT("Movement cannot leave the grid"),
		FGridCombatMovementResolver::ResolveDestination(
			RPG035::MakeRetreatProfile(), FIntPoint(1, 0), EGridEdge::North, FIntPoint(1, 0), Fixture.Runtime, Occupancy, Resolution));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG035CatalogMobilityGateTest, "Grimrock.RPG.RPG03.5.CatalogMobilityGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG035CatalogMobilityGateTest::RunTest(const FString&)
{
	FGridCombatActionContribution Contribution;
	Contribution.Definition = RPG035::MakeRetreatAction();
	Contribution.SourceDefinitionId = TEXT("RPG035_Ranger");
	Contribution.AvailableSourceQuantity = 1;

	FGridCombatActionCatalogContext Context;
	Context.CharacterIndex = 0;
	Context.CharacterId = FGuid::NewGuid();
	Context.bCombatActive = true;
	Context.bActiveCombatant = true;
	Context.bEnableClassActionExecutors = true;
	Context.RemainingActionPoints = 4;
	Context.RemainingMobilityActionPoints = 0;
	Context.CurrentHealth = 10;
	Context.MaximumHealth = 10;

	TArray<FGridAvailableCombatAction> Actions;
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestEqual(TEXT("Movement action stays visible"), Actions.Num(), 1);
	TestEqual(TEXT("No PAM has an explicit availability reason"),
		Actions.Num() == 1 ? Actions[0].AvailabilityReason : EGridCombatActionAvailabilityReason::None,
		EGridCombatActionAvailabilityReason::InsufficientMobilityActionPoints);

	Context.RemainingMobilityActionPoints = 1;
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestTrue(TEXT("One PAM enables Tactical Retreat"), Actions.Num() == 1 && Actions[0].bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG035ForcedControlBypassTest, "Grimrock.RPG.RPG03.5.ForcedControlBypass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG035ForcedControlBypassTest::RunTest(const FString&)
{
	RPG035::FFixture Fixture;
	if (!Fixture.TurnManager || !Fixture.Party || !Fixture.Party->PartyInventoryComponent)
	{
		return false;
	}

	UGridStatusEffectDefinitionAsset* Immobilized = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	Immobilized->EffectId = TEXT("Status_RPG035_Immobilized");
	Immobilized->DisplayName = FText::FromString(TEXT("Immobilized"));
	Immobilized->DurationUnit = EGridStatusEffectDurationUnit::Rounds;
	Immobilized->DefaultDuration = 1;
	Immobilized->Control.bBlockTranslation = true;
	FGridStatusEffectApplyResult ApplyResult;
	FString Error;
	FGridCharacterInventoryState& Character = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	TestTrue(TEXT("Immobilized status applies"), Character.StatusEffects.TryApply(*Immobilized, Character.CharacterId, ApplyResult, Error));

	FGridCombatMovementResolution Resolution;
	EGridPartyMovementRejectReason RejectReason = EGridPartyMovementRejectReason::None;
	TestFalse(TEXT("Normal tactical movement obeys Immobilize"),
		Fixture.TurnManager->CanResolvePartyActionMovement(0, RPG035::MakeRetreatProfile(false), Resolution, RejectReason));
	TestTrue(TEXT("Forced movement bypasses Immobilize"),
		Fixture.TurnManager->CanResolvePartyActionMovement(0, RPG035::MakeRetreatProfile(true), Resolution, RejectReason));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG035TacticalRetreatExecutionTest, "Grimrock.RPG.RPG03.5.TacticalRetreatExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG035TacticalRetreatExecutionTest::RunTest(const FString&)
{
	RPG035::FFixture Fixture;
	if (!Fixture.TurnManager || !Fixture.Party || !Fixture.Class)
	{
		return false;
	}

	FGridCombatActionRequestResult Result;
	TestTrue(TEXT("Tactical Retreat executes"),
		Fixture.TurnManager->RequestCharacterCombatAction(0, TEXT("Action_Ranger_TacticalRetreat"), EGridCombatActionSourcePolicy::Ability,
			Fixture.Class->ClassId, EGridEquipmentSlot::None, Result));
	TestTrue(TEXT("Movement result is exposed"), Result.ClassActionResult.bMovementStarted);
	TestEqual(TEXT("Movement starts from party anchor"), Result.ClassActionResult.MovementFromCell, FIntPoint(1, 1));
	TestEqual(TEXT("Whole-party anchor moves backward one cell"), Result.ClassActionResult.MovementToCell, FIntPoint(1, 0));
	TestEqual(TEXT("Logical party cell is updated by the normal interpolation path"), FIntPoint(Fixture.Party->CurrentCellX, Fixture.Party->CurrentCellY), FIntPoint(1, 0));
	TestEqual(TEXT("Facing is preserved"), Fixture.Party->Facing, EGridEdge::North);
	TestEqual(TEXT("Exactly one shared PAM is spent"), Fixture.TurnManager->PartyMobilityState.RemainingMobilityActionPoints, 1);

	FGridPlayerCharacterTurnState TurnState;
	TestTrue(TEXT("Turn state remains available"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, TurnState));
	TestEqual(TEXT("Exactly one personal AP is spent"), TurnState.RemainingActionPoints, 3);
	TestTrue(TEXT("Movement remains pending until interpolation completes"), Fixture.TurnManager->IsPartyMotionInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG035RejectedRetreatNoSpendTest, "Grimrock.RPG.RPG03.5.RejectedRetreatNoSpend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG035RejectedRetreatNoSpendTest::RunTest(const FString&)
{
	RPG035::FFixture Fixture;
	if (!Fixture.TurnManager || !Fixture.Party || !Fixture.Class)
	{
		return false;
	}
	Fixture.Party->CurrentCellX = 1;
	Fixture.Party->CurrentCellY = 0;
	Fixture.Party->SnapToCurrentCell();

	FGridPlayerCharacterTurnState Before;
	Fixture.TurnManager->GetPlayerCharacterTurnState(0, Before);
	const int32 PamBefore = Fixture.TurnManager->PartyMobilityState.RemainingMobilityActionPoints;

	FGridCombatActionRequestResult Result;
	TestFalse(TEXT("Retreat outside the grid is rejected without commit"),
		Fixture.TurnManager->RequestCharacterCombatAction(0, TEXT("Action_Ranger_TacticalRetreat"), EGridCombatActionSourcePolicy::Ability,
			Fixture.Class->ClassId, EGridEquipmentSlot::None, Result));

	FGridPlayerCharacterTurnState After;
	Fixture.TurnManager->GetPlayerCharacterTurnState(0, After);
	TestEqual(TEXT("Rejected movement spends no AP"), After.RemainingActionPoints, Before.RemainingActionPoints);
	TestEqual(TEXT("Rejected movement spends no PAM"), Fixture.TurnManager->PartyMobilityState.RemainingMobilityActionPoints, PamBefore);
	TestFalse(TEXT("Rejected movement starts no interpolation"), Fixture.TurnManager->IsPartyMotionInProgress());
	return true;
}

#endif
