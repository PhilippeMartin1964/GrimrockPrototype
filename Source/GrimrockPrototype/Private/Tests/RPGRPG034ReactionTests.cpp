#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatReactionResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace RPG034
{
	FGridCombatReactionProfile MakeReaction(FName ReactionId, EGridCombatReactionTrigger Trigger, EGridCombatReactionLimit Limit)
	{
		FGridCombatReactionProfile Profile;
		Profile.ReactionId = ReactionId;
		Profile.Trigger = Trigger;
		Profile.Limit = Limit;
		return Profile;
	}

	FGridCombatReactionEvent MakeEvent(EGridCombatReactionTrigger Trigger, const FGuid& ActionId, int32 Round = 1)
	{
		FGridCombatReactionEvent Event;
		Event.EventId = FGuid::NewGuid();
		Event.ActionInstanceId = ActionId;
		Event.RoundNumber = Round;
		Event.Trigger = Trigger;
		Event.SourceCombatantId = FGuid(3, 4, 1, 1);
		Event.TargetCombatantId = FGuid(3, 4, 1, 2);
		Event.ActionId = TEXT("Action_RPG034");
		Event.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Event.ActionType = EGridCombatActionType::MeleeAttack;
		Event.DamageType = EGridDamageType::Physical;
		return Event;
	}

	UGridStatusEffectDefinitionAsset* MakeConsumableStatus(UObject* Outer, FName EffectId, FName ActionId)
	{
		UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>(Outer);
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromName(EffectId);
		Definition->Disposition = EGridStatusEffectDisposition::Buff;
		Definition->DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		Definition->DefaultDuration = 2;
		Definition->StackPolicy = EGridStatusEffectStackPolicy::NoStack;

		FGridCombatReactionProfile Reaction = MakeReaction(
			TEXT("Reaction_RPG034_Consume"), EGridCombatReactionTrigger::ActionResolved, EGridCombatReactionLimit::OncePerAction);
		Reaction.ActionIds.Add(ActionId);
		Reaction.bConsumeOwningStatus = true;
		Definition->CombatReactions.Add(Reaction);
		return Definition;
	}

	FGridCombatActionDefinition MakeArmorAction(FName ActionId)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromName(ActionId);
		Action.Description = FText::FromName(ActionId);
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Self;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 1;
		FGridCombatArmorEffectProfile Armor;
		Armor.Pool = EGridCombatArmorPool::Magical;
		Armor.Operation = EGridCombatArmorEffectOperation::Restore;
		Armor.Magnitude = EGridCombatArmorEffectMagnitude::Flat;
		Armor.Trigger = EGridCombatArmorEffectTrigger::AfterResolution;
		Armor.Amount = 1;
		Action.ArmorEffects.Add(Armor);
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
				FName(*FString::Printf(TEXT("RPG034World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num, &Values);
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

	struct FIntegrationFixture
	{
		FTestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		AActor* Owner = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		URPGClassAsset* Class = nullptr;
		UGridStatusEffectDefinitionAsset* Status = nullptr;
		FGuid CharacterId = FGuid(3, 4, 9, 1);

		FIntegrationFixture()
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

			const FName ActionId(TEXT("Action_RPG034_ConsumeStatus"));
			Class = NewObject<URPGClassAsset>(GetTransientPackage());
			Class->ClassId = TEXT("RPG034_Class");
			Class->HealthAtLevelOne = 10;
			Class->BaseMagicalArmor = 10;
			Class->CombatActions = { MakeArmorAction(ActionId) };

			FGridCharacterInventoryState Character;
			Character.CharacterId = CharacterId;
			Character.DisplayName = FText::FromString(TEXT("RPG03.4 Hero"));
			Character.ClassId = Class->ClassId;
			Character.ClassDefinition = TSoftObjectPtr<URPGClassAsset>(Class);
			Character.DerivedStats.MaxHealth = 10;
			Character.Resources.CurrentHealth = 10;
			Character.Resources.CurrentMagicalArmor = 1;
			Character.InventorySlots.SetNum(4);

			Status = MakeConsumableStatus(GetTransientPackage(), TEXT("Status_RPG034_Consume"), ActionId);
			FGridStatusEffectApplyResult ApplyResult;
			FString Error;
			Character.StatusEffects.TryApply(*Status, CharacterId, ApplyResult, Error);

			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(1);
			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 1;
			Party->CurrentCellY = 1;
			Party->Facing = EGridEdge::North;
			Party->SnapToCurrentCell();

			TurnManager = NewObject<UGridTurnManagerComponent>(Owner, TEXT("RPG034TurnManager"));
			if (!TurnManager || !TurnManager->InitializeTurnManager(Runtime, Party))
			{
				TurnManager = nullptr;
				return;
			}
			TurnManager->bCombatActive = true;
			TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
			TurnManager->RoundNumber = 1;

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG034ProfileValidationTest, "Grimrock.RPG.RPG03.4.ProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG034ProfileValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatReactionProfile Profile;
	TestFalse(TEXT("Empty reaction profile is invalid"), Profile.IsValid());
	Profile = RPG034::MakeReaction(TEXT("Reaction_Test"), EGridCombatReactionTrigger::AttackMiss, EGridCombatReactionLimit::OncePerRound);
	TestTrue(TEXT("Minimal reaction profile is valid"), Profile.IsValid());
	Profile.ActionIds.Add(NAME_None);
	TestFalse(TEXT("None action filter is rejected"), Profile.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG034FilterMatchingTest, "Grimrock.RPG.RPG03.4.FilterMatching",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG034FilterMatchingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatReactionProfile Profile =
		RPG034::MakeReaction(TEXT("Reaction_Riposte"), EGridCombatReactionTrigger::AttackMiss, EGridCombatReactionLimit::OncePerRound);
	Profile.SourcePolicies.Add(EGridCombatActionSourcePolicy::Universal);
	Profile.ActionTypes.Add(EGridCombatActionType::MeleeAttack);
	Profile.DamageTypes.Add(EGridDamageType::Physical);

	FGridCombatReactionEvent Event = RPG034::MakeEvent(EGridCombatReactionTrigger::AttackMiss, FGuid::NewGuid());
	Event.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	TestTrue(TEXT("Matching incoming melee miss resolves"), FGridCombatReactionResolver::Matches(Profile, Event));
	Event.ActionType = EGridCombatActionType::RangedAttack;
	TestFalse(TEXT("Ranged miss is filtered out"), FGridCombatReactionResolver::Matches(Profile, Event));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG034RecursiveIsolationTest, "Grimrock.RPG.RPG03.4.RecursiveIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG034RecursiveIsolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatReactionProfile Profile =
		RPG034::MakeReaction(TEXT("Reaction_NoRecursion"), EGridCombatReactionTrigger::AttackHit, EGridCombatReactionLimit::Unlimited);
	FGridCombatReactionEvent Event = RPG034::MakeEvent(EGridCombatReactionTrigger::AttackHit, FGuid::NewGuid());
	Event.bReactionGenerated = true;
	TestFalse(TEXT("Reaction-generated events are blocked by default"), FGridCombatReactionResolver::Matches(Profile, Event));
	Profile.bAllowReactionGeneratedEvents = true;
	TestTrue(TEXT("Explicit opt-in permits reaction-generated events"), FGridCombatReactionResolver::Matches(Profile, Event));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG034OncePerRoundLedgerTest, "Grimrock.RPG.RPG03.4.OncePerRoundLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG034OncePerRoundLedgerTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid Owner = FGuid::NewGuid();
	FGridCombatReactionProfile Profile =
		RPG034::MakeReaction(TEXT("Reaction_OnceRound"), EGridCombatReactionTrigger::AttackMiss, EGridCombatReactionLimit::OncePerRound);
	FGridCombatReactionLedger Ledger;
	FGridCombatReactionEvent Event = RPG034::MakeEvent(EGridCombatReactionTrigger::AttackMiss, FGuid::NewGuid(), 3);
	TestTrue(TEXT("First round use commits"), Ledger.Commit(Profile, Owner, Event));
	Event.EventId = FGuid::NewGuid();
	Event.ActionInstanceId = FGuid::NewGuid();
	TestFalse(TEXT("Second use in same round is rejected"), Ledger.Commit(Profile, Owner, Event));
	Event.RoundNumber = 4;
	TestTrue(TEXT("Next round is available again"), Ledger.Commit(Profile, Owner, Event));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG034OncePerActionLedgerTest, "Grimrock.RPG.RPG03.4.OncePerActionLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG034OncePerActionLedgerTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid Owner = FGuid::NewGuid();
	const FGuid RootAction = FGuid::NewGuid();
	FGridCombatReactionProfile Profile =
		RPG034::MakeReaction(TEXT("Reaction_OnceAction"), EGridCombatReactionTrigger::AttackHit, EGridCombatReactionLimit::OncePerAction);
	FGridCombatReactionLedger Ledger;
	FGridCombatReactionEvent Event = RPG034::MakeEvent(EGridCombatReactionTrigger::AttackHit, RootAction);
	TestTrue(TEXT("First target of action consumes allowance"), Ledger.Commit(Profile, Owner, Event));
	Event.EventId = FGuid::NewGuid();
	Event.TargetCombatantId = FGuid::NewGuid();
	TestFalse(TEXT("Second target of same action is rejected"), Ledger.Commit(Profile, Owner, Event));
	Event.ActionInstanceId = FGuid::NewGuid();
	TestTrue(TEXT("Different action gets a fresh allowance"), Ledger.Commit(Profile, Owner, Event));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG034ProjectionTest, "Grimrock.RPG.RPG03.4.Projection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG034ProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	URPGClassAsset* Class = NewObject<URPGClassAsset>(GetTransientPackage());
	Class->ClassId = TEXT("RPG034_ProjectionClass");
	Class->HealthAtLevelOne = 10;
	FRPGClassProgressionChoiceDefinition Choice;
	Choice.ChoiceId = TEXT("Talent_RPG034");
	Choice.DisplayName = FText::FromString(TEXT("Reaction talent"));
	Choice.MinimumLevel = 2;
	Choice.PointCost = 1;
	Choice.CombatReactions.Add(
		RPG034::MakeReaction(TEXT("Reaction_Talent"), EGridCombatReactionTrigger::TargetDefeated, EGridCombatReactionLimit::OncePerRound));
	Class->ProgressionChoices.Add(Choice);
	TestTrue(TEXT("Class accepts generic C4 reaction"), Class->IsValidDefinition());

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.ClassId = Class->ClassId;
	Character.ClassDefinition = TSoftObjectPtr<URPGClassAsset>(Class);
	Character.SelectedClassProgressionChoiceIds.Add(Choice.ChoiceId);

	UGridStatusEffectDefinitionAsset* Status =
		RPG034::MakeConsumableStatus(GetTransientPackage(), TEXT("Status_RPG034_Projection"), TEXT("Action_RPG034"));
	FGridStatusEffectApplyResult ApplyResult;
	FString Error;
	TestTrue(TEXT("Reaction status applies"), Character.StatusEffects.TryApply(*Status, Character.CharacterId, ApplyResult, Error));

	TArray<FGridCombatReactionBinding> Bindings;
	TestTrue(TEXT("Talent and status reactions project"), FGridCombatReactionResolver::CollectCharacterBindings(Character, Bindings));
	TestEqual(TEXT("Exactly two reaction bindings"), Bindings.Num(), 2);
	TestTrue(TEXT("Status binding keeps owner EffectId"),
		Bindings.ContainsByPredicate(
			[Status](const FGridCombatReactionBinding& Binding)
			{
				return Binding.OwningStatusEffectId == Status->EffectId && Binding.Profile.bConsumeOwningStatus;
			}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG034ResolveMatchesTest, "Grimrock.RPG.RPG03.4.ResolveMatches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG034ResolveMatchesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatReactionBinding Binding;
	Binding.Profile =
		RPG034::MakeReaction(TEXT("Reaction_Match"), EGridCombatReactionTrigger::ActionResolved, EGridCombatReactionLimit::OncePerAction);
	Binding.Profile.bConsumeOwningStatus = true;
	Binding.OwningStatusEffectId = TEXT("Status_Consume");

	FGridCombatReactionLedger Ledger;
	TArray<FGridCombatReactionMatch> Matches;
	const FGuid Owner = FGuid::NewGuid();
	const FGuid Action = FGuid::NewGuid();
	const FGridCombatReactionEvent Event = RPG034::MakeEvent(EGridCombatReactionTrigger::ActionResolved, Action);
	FGridCombatReactionResolver::ResolveMatches({ Binding }, Owner, Event, Ledger, true, Matches);
	TestEqual(TEXT("One matching reaction resolves"), Matches.Num(), 1);
	TestTrue(TEXT("Resolved match requests owning-status consumption"), Matches.Num() == 1 && Matches[0].bConsumeOwningStatus);
	FGridCombatReactionResolver::ResolveMatches({ Binding }, Owner, Event, Ledger, true, Matches);
	TestEqual(TEXT("Committed once-per-action match cannot repeat"), Matches.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG034RuntimeStatusConsumptionTest, "Grimrock.RPG.RPG03.4.RuntimeStatusConsumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG034RuntimeStatusConsumptionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	RPG034::FIntegrationFixture Fixture;
	if (!Fixture.TurnManager || !Fixture.Party || !Fixture.Party->PartyInventoryComponent || !Fixture.Class || !Fixture.Status)
	{
		return false;
	}

	FGridCharacterInventoryState& Character = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	TestTrue(TEXT("Consumable status is active before action"), Character.StatusEffects.Contains(Fixture.Status->EffectId));

	FGridCombatActionRequestResult Result;
	TestTrue(TEXT("Action executes"),
		Fixture.TurnManager->RequestCharacterCombatAction(0, TEXT("Action_RPG034_ConsumeStatus"), EGridCombatActionSourcePolicy::Ability,
			Fixture.Class->ClassId, EGridEquipmentSlot::None, Result));
	TestTrue(TEXT("Action accepted"), Result.bAccepted);
	TestFalse(TEXT("ActionResolved reaction consumes owning status"), Character.StatusEffects.Contains(Fixture.Status->EffectId));
	TestEqual(TEXT("Armor action still resolves normally"), Character.Resources.CurrentMagicalArmor, 2);
	return true;
}

#endif
