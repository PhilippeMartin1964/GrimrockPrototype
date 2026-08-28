#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "InputActionValue.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "UI/GridCombatActionPanelWidget.h"

namespace
{
	struct FGridMON12TestWorld
	{
		UWorld* World = nullptr;

		FGridMON12TestWorld()
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
																		  .AllowAudioPlayback(false)
																		  .RequiresHitProxies(false)
																		  .CreatePhysicsScene(false)
																		  .CreateNavigation(false)
																		  .CreateAISystem(false)
																		  .ShouldSimulatePhysics(false)
																		  .SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("MON12TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON12TestWorld()
		{
			if (!World)
			{
				return;
			}
			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};

	FGridCharacterInventoryState MakeCharacter(
		const FGuid& CharacterId, const TCHAR* DisplayName, int32 CurrentHealth, int32 MaxHealth, int32 CurrentMana, int32 MaxMana, UTexture2D* Portrait)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.DisplayName = FText::FromString(DisplayName);
		Character.Resources.CurrentHealth = CurrentHealth;
		Character.DerivedStats.MaxHealth = MaxHealth;
		Character.Resources.CurrentMana = CurrentMana;
		Character.DerivedStats.MaxMana = MaxMana;
		Character.RaceId = TEXT("MON12_TestRace");
		Character.RaceDisplayName = FText::FromString(TEXT("MON12 Test Race"));
		Character.PortraitGender = ERPGCharacterPortraitGender::Male;
		Character.PortraitVariantId = FName(*FString::Printf(TEXT("MON12_%s"), DisplayName));
		Character.Portrait = Portrait;
		FRPGAuthoringIdentityResolver::RememberPortraitVisual(
			Character.RaceId, Character.PortraitGender, Character.PortraitVariantId, TSoftObjectPtr<UTexture2D>(Portrait));
		Character.InventorySlots.SetNum(4);
		return Character;
	}

	FGridItemInstance MakeEquippedItem(
		const FGuid& RuntimeObjectId, FName ItemDefinitionId, const TCHAR* DisplayName, int32 Quantity, int32 CharacterIndex, EGridEquipmentSlot EquipmentSlot)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = RuntimeObjectId;
		Item.ItemDefinitionId = ItemDefinitionId;
		Item.DisplayName = FText::FromString(DisplayName);
		Item.Quantity = Quantity;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerCharacterIndex = CharacterIndex;
		Item.EquipmentSlot = EquipmentSlot;
		return Item;
	}

	UGridItemDefinitionAsset* MakeItemDefinition(UObject* Outer, FName ItemDefinitionId, const TCHAR* DisplayName, bool bStackable, UTexture2D* Icon)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = ItemDefinitionId;
		Definition->DisplayName = FText::FromString(DisplayName);
		Definition->bStackable = bStackable;
		Definition->MaxStackSize = bStackable ? 20 : 1;
		Definition->Icon = Icon;
		return Definition;
	}

	void ConfigureOffensiveDefinition(UGridItemDefinitionAsset* Definition, FName AttackId, EGridEquipmentSlot EquipmentSlot, int32 RangeCells = 1)
	{
		if (!Definition)
		{
			return;
		}

		Definition->CompatibleEquipmentSlots.AddUnique(EquipmentSlot);
		FGridCombatActionDefinition Action;
		Action.ActionId = AttackId;
		Action.DisplayName = Definition->DisplayName;
		Action.ActionType = RangeCells > 1 ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = 2;
		Action.RangeCells = RangeCells;
		Action.PresentationProfileId = AttackId;
		Action.OffensiveProfile.AttackId = AttackId;
		Action.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
		Action.OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
		Action.OffensiveProfile.AttackDefinition.MinDamage = 1;
		Action.OffensiveProfile.AttackDefinition.MaxDamage = 2;
		Action.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::Dexterity;
		Action.OffensiveProfile.RangeCells = RangeCells;
		Definition->CombatActions = { Action };
	}

	struct FGridMON12Fixture
	{
		FGridMON12TestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		UGridLevelAsset* LevelAsset = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridMonsterDefinitionAsset* MonsterDefinition = nullptr;
		AGridMonsterActor* Monster = nullptr;
		UGridMonsterOccupancySubsystem* Occupancy = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		UGridCombatActionPanelWidget* Panel = nullptr;
		UTexture2D* EliasPortrait = nullptr;
		UTexture2D* MinaPortrait = nullptr;
		FGuid EliasId = FGuid(12, 1, 0, 1);
		FGuid MinaId = FGuid(12, 1, 0, 2);

		FGridMON12Fixture()
		{
			if (!TestWorld.World)
			{
				return;
			}

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			LevelAsset = NewObject<UGridLevelAsset>(Runtime);
			LevelAsset->Width = 3;
			LevelAsset->Height = 3;
			LevelAsset->EnsureCellCount();
			for (FGridLevelCellData& Cell : LevelAsset->Cells)
			{
				Cell.CellType = EGridCellType::Floor;
				Cell.bBlocksOccupancy = false;
			}
			Runtime->LevelAsset = LevelAsset;
			Runtime->CurrentDungeonLevelId = TEXT("MON12_Test");

			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 1;
			Party->CurrentCellY = 1;
			Party->Facing = EGridEdge::North;
			Party->SetActorLocation(Runtime->GetCellCenterWorld(1, 1, Party->EyeHeight));
			Party->SetActorRotation(FRotator(0.0f, GridDirectionUtils::ToYaw(Party->Facing), 0.0f));

			EliasPortrait = NewObject<UTexture2D>(Party);
			MinaPortrait = NewObject<UTexture2D>(Party);

			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { MakeCharacter(EliasId, TEXT("Elias"), 18, 24, 7, 12, EliasPortrait),
				MakeCharacter(MinaId, TEXT("Mina"), 15, 20, 3, 9, MinaPortrait) };
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(2);
			Party->PartyInventoryComponent->PartyInventoryState.SelectedCharacterIndex = 0;

			FGridCharacterEquipmentState& EliasEquipment = Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0];
			EliasEquipment.MainHand = MakeEquippedItem(FGuid(12, 2, 0, 1), TEXT("Shuriken"), TEXT("Shuriken"), 3, 0, EGridEquipmentSlot::MainHand);
			EliasEquipment.OffHand = MakeEquippedItem(FGuid(12, 2, 0, 2), TEXT("Item_Torch"), TEXT("Torche"), 1, 0, EGridEquipmentSlot::OffHand);

			UGridItemDefinitionAsset* ShurikenDefinition = MakeItemDefinition(Party, TEXT("Shuriken"), TEXT("Shuriken"), true, nullptr);
			ConfigureOffensiveDefinition(ShurikenDefinition, TEXT("Attack_Shuriken"), EGridEquipmentSlot::MainHand, 2);
			Party->PartyInventoryComponent->RegisterItemDefinition(ShurikenDefinition);
			Party->PartyInventoryComponent->RegisterItemDefinition(MakeItemDefinition(Party, TEXT("Item_Torch"), TEXT("Torche"), false, nullptr));

			MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>(Runtime);
			MonsterDefinition->MonsterId = TEXT("MON12_Rat");
			MonsterDefinition->DisplayName = FText::FromString(TEXT("Rat MON12"));
			MonsterDefinition->CategoryId = TEXT("Vermin");
			MonsterDefinition->MaxHealth = 1000;
			MonsterDefinition->Evasion = 0;
			MonsterDefinition->ActionPointsPerTurn = 2;
			Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
			Monster = TestWorld.World->SpawnActor<AGridMonsterActor>();
			if (Monster && Occupancy)
			{
				Monster->InitializeMonster(MonsterDefinition, FGuid(12, 3, 0, 1), FIntPoint(1, 2), EGridEdge::South);
				Occupancy->RegisterMonster(Monster, FIntPoint(1, 2));

				UGridMonsterMovementComponent* Movement = NewObject<UGridMonsterMovementComponent>(Monster, TEXT("MON12Movement"));
				Movement->bAutoInitialize = false;
				Movement->bInferCellFromActorLocation = false;
				Monster->AddInstanceComponent(Movement);
				Movement->RegisterComponent();

				UGridMonsterBehaviorComponent* Behavior = NewObject<UGridMonsterBehaviorComponent>(Monster, TEXT("MON12Behavior"));
				Behavior->bAutoInitialize = false;
				Monster->AddInstanceComponent(Behavior);
				Behavior->RegisterComponent();
			}

			TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("MON12TurnManager"));
			TurnManager->bAutoInitialize = false;
			Runtime->AddInstanceComponent(TurnManager);
			TurnManager->RegisterComponent();
			TurnManager->InitializeTurnManager(Runtime, Party);
			TurnManager->bCombatActive = true;
			TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
			TurnManager->RoundNumber = 1;
			if (Monster)
			{
				TurnManager->CombatMonsters = { Monster };
			}

			Panel = NewObject<UGridCombatActionPanelWidget>(Party);
		}

		bool IsReady() const
		{
			return TestWorld.World && Runtime && LevelAsset && Party && Party->PartyInventoryComponent && MonsterDefinition && Monster &&
				Monster->FindComponentByClass<UGridMonsterMovementComponent>() && Monster->FindComponentByClass<UGridMonsterBehaviorComponent>() && Occupancy &&
				TurnManager && Panel && EliasPortrait && MinaPortrait;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12CombatActionPanelLiveDataTest, "Grimrock.Monsters.MON12.CombatActionPanel.LiveData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CombatActionPanelLiveDataTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The MON12 fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	Fixture.Panel->InitializeCombatActionPanel(Fixture.Party, 0, Fixture.TurnManager);

	const FGridCombatActionPanelView& View = Fixture.Panel->View;
	TestTrue(TEXT("The panel resolves its assigned character"), View.bHasValidCharacter);
	TestEqual(TEXT("The panel keeps the assigned member index"), View.CharacterIndex, 0);
	TestEqual(TEXT("The panel reads the real character name"), View.DisplayName.ToString(), FString(TEXT("Elias")));
	TestEqual(TEXT("The panel reads current health"), View.CurrentHealth, 18);
	TestEqual(TEXT("The panel reads maximum health"), View.MaxHealth, 24);
	TestEqual(TEXT("The panel reads current mana"), View.CurrentMana, 7);
	TestEqual(TEXT("The panel reads maximum mana"), View.MaxMana, 12);
	TestTrue(TEXT("The panel reads the real portrait"), View.Portrait.Get() == Fixture.EliasPortrait);
	TestEqual(TEXT("The initial turn state is Active"), View.TurnState, EGridCombatantTurnState::Active);
	TestEqual(TEXT("The initial action-point budget is full"), View.RemainingActionPoints, 4);
	TestEqual(TEXT("The panel exposes the maximum action points"), View.MaximumActionPoints, 4);
	TestTrue(TEXT("The turn manager authorizes the living ready member"), View.bCanAct);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12CombatActionPanelTurnAuthorityTest, "Grimrock.Monsters.MON12.CombatActionPanel.TurnAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CombatActionPanelTurnAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The MON12 fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	Fixture.Panel->InitializeCombatActionPanel(Fixture.Party, 1, Fixture.TurnManager);
	TestEqual(TEXT("The panel keeps its assigned member index"), Fixture.Panel->View.CharacterIndex, 1);
	TestEqual(TEXT("The assigned panel reads Mina"), Fixture.Panel->View.DisplayName.ToString(), FString(TEXT("Mina")));
	TestTrue(TEXT("The second living member is initially actionable"), Fixture.Panel->View.bCanAct);

	Fixture.TurnManager->CurrentPhase = EGridCombatPhase::EnemyPhase;
	Fixture.TurnManager->OnPhaseChanged.Broadcast(EGridCombatPhase::EnemyPhase);
	Fixture.Panel->RefreshFromSources();
	TestFalse(TEXT("The status panel reflects the enemy phase"), Fixture.Panel->View.bCanAct);

	Fixture.TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
	Fixture.TurnManager->OnPhaseChanged.Broadcast(EGridCombatPhase::PlayerPhase);
	Fixture.Panel->RefreshFromSources();
	TestTrue(TEXT("The status panel reflects the player phase"), Fixture.Panel->View.bCanAct);

	FGridPlayerCharacterTurnState CompletedState;
	CompletedState.CharacterIndex = 1;
	CompletedState.CharacterId = Fixture.MinaId;
	CompletedState.State = EGridCombatantTurnState::Completed;
	CompletedState.MaximumActionPoints = 4;
	CompletedState.RemainingActionPoints = 0;
	Fixture.TurnManager->PlayerCharacterTurnStates = { CompletedState };
	Fixture.TurnManager->OnPlayerCharacterTurnStateChanged.Broadcast(CompletedState);
	Fixture.Panel->RefreshFromSources();

	TestEqual(TEXT("The status panel reflects Completed"), Fixture.Panel->View.TurnState, EGridCombatantTurnState::Completed);
	TestFalse(TEXT("Completed disables the panel"), Fixture.Panel->View.bCanAct);

	Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[1].Resources.CurrentHealth = 9;
	FGridAttackResult Result;
	Fixture.TurnManager->OnAttackResolved.Broadcast(nullptr, 1, Result);
	Fixture.Panel->RefreshFromSources();
	TestEqual(TEXT("The status panel refreshes current health"), Fixture.Panel->View.CurrentHealth, 9);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12CharacterActionPointLifecycleTest, "Grimrock.Monsters.MON12.CharacterActionPoints.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CharacterActionPointLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The MON12 action-point fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	Fixture.Panel->InitializeCombatActionPanel(Fixture.Party, 0, Fixture.TurnManager);
	Fixture.TurnManager->BeginPlayerCharacterPhase();
	Fixture.Panel->RefreshFromSources();

	FGridPlayerCharacterTurnState EliasTurn;
	FGridPlayerCharacterTurnState MinaTurn;
	TestTrue(TEXT("Elias has an authoritative turn state"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, EliasTurn));
	TestEqual(TEXT("Elias begins the round Active"), EliasTurn.State, EGridCombatantTurnState::Active);
	TestEqual(TEXT("Elias begins with four action points"), EliasTurn.RemainingActionPoints, 4);
	TestTrue(TEXT("Mina has an independent turn state"), Fixture.TurnManager->GetPlayerCharacterTurnState(1, MinaTurn));
	TestEqual(TEXT("Mina also begins with four action points"), MinaTurn.RemainingActionPoints, 4);

	FGridPlayerAttackRequest Request;
	FGridAttackResult Result;
	EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
	TestTrue(TEXT("Elias can make a first two-AP attack"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
	Fixture.Panel->RefreshFromSources();
	TestEqual(TEXT("The request records its authoritative AP cost"), Request.ActionPointCost, 2);
	TestEqual(TEXT("The status panel projects two remaining AP"), Fixture.Panel->View.RemainingActionPoints, 2);
	TestEqual(TEXT("Elias remains Active after the first attack"), Fixture.Panel->View.TurnState, EGridCombatantTurnState::Active);
	TestTrue(TEXT("Elias can still afford a second attack"), Fixture.TurnManager->CanCharacterSpendActionPoints(0, 2));

	TestTrue(TEXT("Elias can make a second two-AP attack"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
	Fixture.Panel->RefreshFromSources();
	TestEqual(TEXT("The second attack exhausts Elias action points"), Fixture.Panel->View.RemainingActionPoints, 0);
	TestEqual(TEXT("An exhausted character becomes Completed"), Fixture.Panel->View.TurnState, EGridCombatantTurnState::Completed);
	TestFalse(TEXT("A Completed character cannot act"), Fixture.Panel->View.bCanAct);

	const int32 SeedBeforeRefusal = Fixture.TurnManager->CombatRandomStream.GetCurrentSeed();
	const int32 HealthBeforeRefusal = Fixture.Monster->CurrentHealth;
	TestFalse(TEXT("A third attack is refused"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
	TestEqual(TEXT("The refusal reports insufficient action points"), RejectReason, EGridPlayerAttackRejectReason::InsufficientActionPoints);
	TestEqual(TEXT("The AP refusal consumes no random draw"), Fixture.TurnManager->CombatRandomStream.GetCurrentSeed(), SeedBeforeRefusal);
	TestEqual(TEXT("The AP refusal applies no damage"), Fixture.Monster->CurrentHealth, HealthBeforeRefusal);

	TestTrue(TEXT("Mina remains independently actionable"), Fixture.TurnManager->CanCharacterSpendActionPoints(1, 2));
	TestTrue(TEXT("Mina still retains her full budget"), Fixture.TurnManager->GetPlayerCharacterTurnState(1, MinaTurn));
	TestEqual(TEXT("Elias spending does not change Mina AP"), MinaTurn.RemainingActionPoints, 4);

	Fixture.TurnManager->CompletePlayerCharacterPhase();
	TestTrue(TEXT("Mina state remains available after ending the phase"), Fixture.TurnManager->GetPlayerCharacterTurnState(1, MinaTurn));
	TestEqual(TEXT("Ending the player phase marks unspent characters Completed"), MinaTurn.State, EGridCombatantTurnState::Completed);
	TestEqual(TEXT("Ending a phase does not rewrite the diagnostic AP remainder"), MinaTurn.RemainingActionPoints, 4);

	Fixture.TurnManager->BeginPlayerCharacterPhase();
	TestTrue(TEXT("Elias state is restored for the next round"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, EliasTurn));
	TestEqual(TEXT("The next round restores four action points"), EliasTurn.RemainingActionPoints, 4);
	TestEqual(TEXT("The next round restores Active"), EliasTurn.State, EGridCombatantTurnState::Active);

	Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[1].Resources.CurrentHealth = 0;
	Fixture.TurnManager->RefreshPlayerCharacterVitalState(1);
	TestTrue(TEXT("The defeated Mina state remains queryable"), Fixture.TurnManager->GetPlayerCharacterTurnState(1, MinaTurn));
	TestEqual(TEXT("Zero health produces Defeated"), MinaTurn.State, EGridCombatantTurnState::Defeated);
	TestEqual(TEXT("A defeated character has no action points"), MinaTurn.RemainingActionPoints, 0);
	TestFalse(TEXT("A defeated character cannot act"), Fixture.TurnManager->CanCharacterAct(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12InitiativeOrderRulesTest, "Grimrock.Monsters.MON12.GlobalInitiative.OrderRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12InitiativeOrderRulesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCombatantInitiativeEntry LowTotal;
	LowTotal.CombatantId = FGuid(12, 4, 0, 3);
	LowTotal.Side = EGridCombatantSide::Monster;
	LowTotal.InitiativeBase = 20;
	LowTotal.InitiativeTotal = 21;

	FGridCombatantInitiativeEntry LowBase;
	LowBase.CombatantId = FGuid(12, 4, 0, 2);
	LowBase.CharacterIndex = 1;
	LowBase.InitiativeBase = 14;
	LowBase.InitiativeTotal = 25;
	LowBase.Dexterity = 18;

	FGridCombatantInitiativeEntry LowDexterity;
	LowDexterity.CombatantId = FGuid(12, 4, 0, 4);
	LowDexterity.CharacterIndex = 2;
	LowDexterity.InitiativeBase = 15;
	LowDexterity.InitiativeTotal = 25;
	LowDexterity.Dexterity = 12;

	FGridCombatantInitiativeEntry HighDexterity;
	HighDexterity.CombatantId = FGuid(12, 4, 0, 1);
	HighDexterity.CharacterIndex = 0;
	HighDexterity.InitiativeBase = 15;
	HighDexterity.InitiativeTotal = 25;
	HighDexterity.Dexterity = 16;

	TArray<FGridCombatantInitiativeEntry> Entries = { LowTotal, LowBase, LowDexterity, HighDexterity };
	FGridInitiativeOrderBuilder::Sort(Entries);

	TestTrue(TEXT("The highest total and Dexterity wins the first tie"), Entries[0].CombatantId == HighDexterity.CombatantId);
	TestTrue(TEXT("The same total and base then keeps lower Dexterity second"), Entries[1].CombatantId == LowDexterity.CombatantId);
	TestTrue(TEXT("Initiative base precedes Dexterity in tie-breaking"), Entries[2].CombatantId == LowBase.CombatantId);
	TestTrue(TEXT("A lower total always acts last"), Entries[3].CombatantId == LowTotal.CombatantId);

	TArray<FGridCombatantInitiativeEntry> FirstRoll = { HighDexterity, LowTotal };
	TArray<FGridCombatantInitiativeEntry> SecondRoll = FirstRoll;
	FRandomStream FirstStream(1204);
	FRandomStream SecondStream(1204);
	FGridInitiativeOrderBuilder::RollAndSort(FirstRoll, FirstStream);
	FGridInitiativeOrderBuilder::RollAndSort(SecondRoll, SecondStream);
	TestTrue(TEXT("The same encounter seed produces the same first id"), FirstRoll[0].CombatantId == SecondRoll[0].CombatantId);
	TestEqual(TEXT("The same encounter seed produces the same first roll"), FirstRoll[0].InitiativeRoll, SecondRoll[0].InitiativeRoll);
	TestTrue(TEXT("Every initiative roll stays in d20 bounds"),
		FirstRoll[0].InitiativeRoll >= 1 && FirstRoll[0].InitiativeRoll <= 20 && FirstRoll[1].InitiativeRoll >= 1 && FirstRoll[1].InitiativeRoll <= 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12GlobalInitiativeLifecycleTest, "Grimrock.Monsters.MON12.GlobalInitiative.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12GlobalInitiativeLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The MON12 initiative fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	Fixture.TurnManager->bCollectRuntimeMetrics = true;
	TestTrue(TEXT("The initiative test enters StartingCombat"), Fixture.TurnManager->PhaseState.StartCombat());
	TestTrue(TEXT("The initiative test starts round one"), Fixture.TurnManager->PhaseState.BeginRound());
	Fixture.TurnManager->RoundNumber = 1;
	Fixture.TurnManager->BuildGlobalInitiativeOrder();

	for (FGridCombatantInitiativeEntry& Entry : Fixture.TurnManager->InitiativeOrder)
	{
		if (Entry.Side == EGridCombatantSide::Party)
		{
			Entry.InitiativeTotal = Entry.CharacterIndex == 0 ? 30 : 10;
		}
		else
		{
			Entry.InitiativeTotal = 20;
		}
	}
	FGridInitiativeOrderBuilder::Sort(Fixture.TurnManager->InitiativeOrder);
	Fixture.TurnManager->ResetInitiativeRound();
	Fixture.TurnManager->BeginNextCombatantTurn();

	FGridCombatantInitiativeEntry Active;
	TestTrue(TEXT("The first global combatant is active"), Fixture.TurnManager->GetActiveCombatant(Active));
	TestEqual(TEXT("Elias acts first in the forced deterministic order"), Active.CharacterIndex, 0);
	TestEqual(TEXT("The active party member is selected automatically"), Fixture.Party->PartyInventoryComponent->GetSelectedCharacterIndex(), 0);

	FGridPlayerCharacterTurnState EliasTurn;
	FGridPlayerCharacterTurnState MinaTurn;
	TestTrue(TEXT("Elias turn state is available"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, EliasTurn));
	TestEqual(TEXT("The active character receives four action points"), EliasTurn.RemainingActionPoints, 4);
	TestTrue(TEXT("Mina waiting state is available"), Fixture.TurnManager->GetPlayerCharacterTurnState(1, MinaTurn));
	TestEqual(TEXT("A future character remains Waiting"), MinaTurn.State, EGridCombatantTurnState::Waiting);
	TestEqual(TEXT("A future character receives no AP before her turn"), MinaTurn.RemainingActionPoints, 0);

	TArray<FGridCombatantInitiativeEntry> Upcoming;
	Fixture.TurnManager->GetUpcomingInitiativeOrder(Upcoming);
	TestEqual(TEXT("The upcoming view contains active and two later turns"), Upcoming.Num(), 3);

	FGridPlayerAttackRequest Request;
	FGridAttackResult Result;
	EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
	TestFalse(TEXT("A waiting character cannot attack out of turn"), Fixture.TurnManager->RequestCharacterAttack(1, Request, Result, RejectReason));
	TestEqual(TEXT("The refusal identifies the non-active combatant"), RejectReason, EGridPlayerAttackRejectReason::NotActiveCombatant);

	TestTrue(TEXT("Ending Elias turn advances the global order"), Fixture.TurnManager->EndActivePlayerTurn());
	TestTrue(TEXT("A later global combatant becomes active"), Fixture.TurnManager->GetActiveCombatant(Active));
	TestEqual(TEXT("Mina acts after the interleaved monster"), Active.CharacterIndex, 1);
	TestEqual(TEXT("The interleaved monster receives exactly one turn"), Fixture.TurnManager->RuntimeMetrics.MonsterTurnsStarted, 1);
	TestEqual(TEXT("Mina is selected for the transitional MON12 panel"), Fixture.Party->PartyInventoryComponent->GetSelectedCharacterIndex(), 1);
	TestTrue(TEXT("Mina turn state is refreshed"), Fixture.TurnManager->GetPlayerCharacterTurnState(1, MinaTurn));
	TestEqual(TEXT("Mina receives her own full budget on activation"), MinaTurn.RemainingActionPoints, 4);

	const TArray<FGuid> FirstRoundOrder = { Fixture.TurnManager->InitiativeOrder[0].CombatantId, Fixture.TurnManager->InitiativeOrder[1].CombatantId,
		Fixture.TurnManager->InitiativeOrder[2].CombatantId };
	TestTrue(TEXT("Ending Mina turn completes the mixed round"), Fixture.TurnManager->EndActivePlayerTurn());
	TestEqual(TEXT("Completing all entries starts round two"), Fixture.TurnManager->RoundNumber, 2);
	TestTrue(TEXT("Round two immediately activates its first combatant"), Fixture.TurnManager->GetActiveCombatant(Active));
	TestEqual(TEXT("The same first character acts in round two"), Active.CharacterIndex, 0);
	TestTrue(TEXT("The initiative order is not rerolled between rounds"), Fixture.TurnManager->InitiativeOrder[0].CombatantId == FirstRoundOrder[0]);
	TestTrue(TEXT("The second initiative position is also stable"), Fixture.TurnManager->InitiativeOrder[1].CombatantId == FirstRoundOrder[1]);
	TestTrue(TEXT("The third initiative position is also stable"), Fixture.TurnManager->InitiativeOrder[2].CombatantId == FirstRoundOrder[2]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12PartyMobilityLifecycleTest, "Grimrock.Monsters.MON12.PartyMobility.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12PartyMobilityLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The MON12 party-mobility fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	TestTrue(TEXT("The mobility test enters StartingCombat"), Fixture.TurnManager->PhaseState.StartCombat());
	TestTrue(TEXT("The mobility test starts round one"), Fixture.TurnManager->PhaseState.BeginRound());
	Fixture.TurnManager->RoundNumber = 1;
	Fixture.TurnManager->BuildGlobalInitiativeOrder();

	for (FGridCombatantInitiativeEntry& Entry : Fixture.TurnManager->InitiativeOrder)
	{
		if (Entry.Side == EGridCombatantSide::Party)
		{
			Entry.InitiativeTotal = Entry.CharacterIndex == 0 ? 30 : 10;
		}
		else
		{
			Entry.InitiativeTotal = 20;
		}
	}
	FGridInitiativeOrderBuilder::Sort(Fixture.TurnManager->InitiativeOrder);
	Fixture.TurnManager->ResetInitiativeRound();
	Fixture.TurnManager->BeginNextCombatantTurn();

	FGridPartyMobilityState MobilityState = Fixture.TurnManager->GetPartyMobilityState();
	TestEqual(TEXT("Round one starts with two maximum PAM"), MobilityState.MaximumMobilityActionPoints, 2);
	TestEqual(TEXT("Round one starts with two remaining PAM"), MobilityState.RemainingMobilityActionPoints, 2);

	FGridPlayerCharacterTurnState EliasTurn;
	TestTrue(TEXT("Elias starts the mobility test with a turn state"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, EliasTurn));
	TestEqual(TEXT("Elias starts with four personal AP"), EliasTurn.RemainingActionPoints, 4);

	FIntPoint TargetCell;
	EGridPartyMovementRejectReason RejectReason = EGridPartyMovementRejectReason::None;
	TestFalse(
		TEXT("The party cannot translate into the monster cell"), Fixture.TurnManager->RequestPartyTranslation(EGridEdge::North, TargetCell, RejectReason));
	TestEqual(TEXT("The occupied destination has an explicit rejection"), RejectReason, EGridPartyMovementRejectReason::TargetCellOccupied);
	TestTrue(TEXT("The rejected movement keeps Elias turn state"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, EliasTurn));
	TestEqual(TEXT("An occupied cell consumes no personal AP"), EliasTurn.RemainingActionPoints, 4);
	TestEqual(TEXT("An occupied cell consumes no PAM"), Fixture.TurnManager->PartyMobilityState.RemainingMobilityActionPoints, 2);

	TestTrue(TEXT("Elias starts a valid east translation"), Fixture.Party->TryStartMove(EGridEdge::East));
	TestTrue(TEXT("The translation remains pending during interpolation"), Fixture.TurnManager->IsPartyMotionInProgress());
	TestTrue(
		TEXT("The first translation moves the logical party cell"), FIntPoint(Fixture.Party->CurrentCellX, Fixture.Party->CurrentCellY) == FIntPoint(2, 1));
	TestTrue(TEXT("Elias state is updated after the first translation starts"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, EliasTurn));
	TestEqual(TEXT("The first translation costs one personal AP"), EliasTurn.RemainingActionPoints, 3);
	TestEqual(TEXT("The first translation costs one shared PAM"), Fixture.TurnManager->PartyMobilityState.RemainingMobilityActionPoints, 1);
	TestFalse(TEXT("The turn cannot end while the camera is between cells"), Fixture.TurnManager->EndActivePlayerTurn());

	Fixture.Party->UpdateMove(Fixture.Party->MoveDuration);
	TestFalse(TEXT("The first interpolation clears the pending motion"), Fixture.TurnManager->IsPartyMotionInProgress());
	FGridCombatantInitiativeEntry ActiveCombatant;
	TestTrue(TEXT("A combatant remains active after the first movement"), Fixture.TurnManager->GetActiveCombatant(ActiveCombatant));
	TestEqual(TEXT("Elias keeps his turn after spending only one AP"), ActiveCombatant.CharacterIndex, 0);

	TestTrue(TEXT("Elias starts a valid west translation"), Fixture.Party->TryStartMove(EGridEdge::West));
	Fixture.Party->UpdateMove(Fixture.Party->MoveDuration);
	TestTrue(TEXT("Elias state remains available after two translations"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, EliasTurn));
	TestEqual(TEXT("Two translations leave Elias with two AP"), EliasTurn.RemainingActionPoints, 2);
	TestEqual(TEXT("Two translations exhaust the shared PAM"), Fixture.TurnManager->PartyMobilityState.RemainingMobilityActionPoints, 0);

	RejectReason = EGridPartyMovementRejectReason::None;
	TestFalse(
		TEXT("A third translation is refused in the same round"), Fixture.TurnManager->RequestPartyTranslation(EGridEdge::East, TargetCell, RejectReason));
	TestEqual(TEXT("The third translation reports exhausted PAM"), RejectReason, EGridPartyMovementRejectReason::InsufficientMobilityActionPoints);
	TestTrue(TEXT("Elias state survives the PAM refusal"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, EliasTurn));
	TestEqual(TEXT("The PAM refusal consumes no personal AP"), EliasTurn.RemainingActionPoints, 2);

	Fixture.Party->HandleTurnRight(FInputActionValue());
	TestTrue(TEXT("The TurnRight input starts a free rotation with zero PAM"), Fixture.Party->bIsTurning);
	TestTrue(TEXT("The TurnRight input is tracked during interpolation"), Fixture.TurnManager->IsPartyMotionInProgress());
	Fixture.Party->UpdateTurn(Fixture.Party->TurnDuration);
	TestEqual(TEXT("The TurnRight input follows the validated grid convention"), Fixture.Party->Facing, EGridEdge::West);

	Fixture.Party->HandleTurnLeft(FInputActionValue());
	TestTrue(TEXT("The TurnLeft input starts the opposite free rotation"), Fixture.Party->bIsTurning);
	Fixture.Party->UpdateTurn(Fixture.Party->TurnDuration);
	TestEqual(TEXT("TurnLeft reverses the preceding TurnRight input"), Fixture.Party->Facing, EGridEdge::North);
	TestTrue(TEXT("Elias state remains available after both rotations"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, EliasTurn));
	TestEqual(TEXT("The rotation consumes no personal AP"), EliasTurn.RemainingActionPoints, 2);
	TestEqual(TEXT("The rotation consumes no PAM"), Fixture.TurnManager->PartyMobilityState.RemainingMobilityActionPoints, 0);

	TestTrue(TEXT("Ending Elias turn advances through the actionless monster"), Fixture.TurnManager->EndActivePlayerTurn());
	TestTrue(TEXT("Mina becomes active after the interleaved monster"), Fixture.TurnManager->GetActiveCombatant(ActiveCombatant));
	TestEqual(TEXT("Mina is the final combatant of round one"), ActiveCombatant.CharacterIndex, 1);
	TestTrue(TEXT("Ending Mina turn completes round one"), Fixture.TurnManager->EndActivePlayerTurn());
	TestEqual(TEXT("The global sequencer starts round two"), Fixture.TurnManager->RoundNumber, 2);

	MobilityState = Fixture.TurnManager->GetPartyMobilityState();
	TestEqual(TEXT("Round two restores the shared PAM"), MobilityState.RemainingMobilityActionPoints, 2);
	TestEqual(TEXT("The PAM snapshot follows the current round"), MobilityState.RoundNumber, 2);
	TestTrue(TEXT("Elias is active again in round two"), Fixture.TurnManager->GetActiveCombatant(ActiveCombatant));
	TestEqual(TEXT("The stable initiative order starts round two with Elias"), ActiveCombatant.CharacterIndex, 0);

	FGridPlayerCharacterTurnState* MutableEliasTurn = Fixture.TurnManager->FindPlayerCharacterTurnState(Fixture.EliasId);
	TestNotNull(TEXT("The test can prepare Elias final movement AP"), MutableEliasTurn);
	if (!MutableEliasTurn)
	{
		return false;
	}
	MutableEliasTurn->RemainingActionPoints = 1;
	TestTrue(TEXT("Elias starts a translation with his final AP"), Fixture.Party->TryStartMove(EGridEdge::East));
	TestTrue(TEXT("Elias remains active until the final interpolation ends"), Fixture.TurnManager->GetActiveCombatant(ActiveCombatant));
	TestEqual(TEXT("The moving combatant is still Elias"), ActiveCombatant.CharacterIndex, 0);
	Fixture.Party->BufferMoveCommand(EGridEdge::West);

	Fixture.Party->UpdateMove(Fixture.Party->MoveDuration);
	TestTrue(TEXT("Completing the final-AP movement advances the turn"), Fixture.TurnManager->GetActiveCombatant(ActiveCombatant));
	TestEqual(TEXT("The actionless monster is skipped and Mina becomes active"), ActiveCombatant.CharacterIndex, 1);
	TestFalse(TEXT("A buffered command never carries into Mina turn"), Fixture.Party->TryConsumeBufferedCommand());
	TestTrue(
		TEXT("The discarded buffered command does not move the party"), FIntPoint(Fixture.Party->CurrentCellX, Fixture.Party->CurrentCellY) == FIntPoint(2, 1));
	TestTrue(TEXT("Elias completed state remains queryable"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, EliasTurn));
	TestEqual(TEXT("The final movement marks Elias Completed"), EliasTurn.State, EGridCombatantTurnState::Completed);
	TestEqual(TEXT("The final movement leaves one shared PAM"), Fixture.TurnManager->PartyMobilityState.RemainingMobilityActionPoints, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12ActionCatalogContributionsTest, "Grimrock.Monsters.MON12.ActionCatalog.Contributions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12ActionCatalogContributionsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The MON12 action-catalogue fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	UGridItemDefinitionAsset* Shuriken = Fixture.Party->PartyInventoryComponent->FindItemDefinition(TEXT("Shuriken"));
	TestNotNull(TEXT("The shuriken definition is registered"), Shuriken);
	if (!Shuriken)
	{
		return false;
	}

	TestTrue(TEXT("The Shuriken fixture exposes a current CombatAction"), !Shuriken->CombatActions.IsEmpty());
	if (Shuriken->CombatActions.IsEmpty())
	{
		return false;
	}
	FGridCombatActionDefinition QuickThrow = Shuriken->CombatActions[0];
	QuickThrow.ActionId = TEXT("Action_QuickThrow");
	QuickThrow.DisplayName = FText::FromString(TEXT("Lancer rapide"));
	QuickThrow.OffensiveProfile.AttackId = TEXT("Attack_QuickThrow");

	FGridCombatActionDefinition PreciseThrow = QuickThrow;
	PreciseThrow.ActionId = TEXT("Action_PreciseThrow");
	PreciseThrow.DisplayName = FText::FromString(TEXT("Lancer précis"));
	PreciseThrow.ActionPointCost = 3;
	PreciseThrow.OffensiveProfile.AttackId = TEXT("Attack_PreciseThrow");
	PreciseThrow.OffensiveProfile.AttackDefinition.AccuracyBonus = 2;
	Shuriken->CombatActions = { QuickThrow, PreciseThrow };

	URPGClassAsset* MageClass = NewObject<URPGClassAsset>(Fixture.Party);
	MageClass->ClassId = TEXT("Mage");
	MageClass->DisplayName = FText::FromString(TEXT("Mage"));
	MageClass->HealthAtLevelOne = 6;

	FGridCombatActionDefinition FocusAbility;
	FocusAbility.ActionId = TEXT("Ability_Focus");
	FocusAbility.DisplayName = FText::FromString(TEXT("Concentration"));
	FocusAbility.ActionType = EGridCombatActionType::Ability;
	FocusAbility.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
	FocusAbility.TargetingPolicy = EGridCombatTargetingPolicy::Self;
	FocusAbility.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
	FocusAbility.ActionPointCost = 1;

	FGridCombatActionDefinition FireballSpell;
	FireballSpell.ActionId = TEXT("Spell_Fireball");
	FireballSpell.DisplayName = FText::FromString(TEXT("Boule de feu"));
	FireballSpell.ActionType = EGridCombatActionType::Ability;
	FireballSpell.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	FireballSpell.TargetingPolicy = EGridCombatTargetingPolicy::Area;
	FireballSpell.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
	FireballSpell.ActionPointCost = 3;
	FireballSpell.ResourceCosts.ManaCost = 8;
	FireballSpell.RangeCells = 3;
	FireballSpell.AreaRadiusCells = 1;
	MageClass->CombatActions = { FocusAbility, FireballSpell };

	FGridCharacterInventoryState& Elias = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	Elias.ClassId = MageClass->ClassId;
	Elias.ClassDefinition = MageClass;

	TArray<FGridAvailableCombatAction> Actions;
	Fixture.TurnManager->GetAvailableCombatActions(0, Actions);
	TestEqual(TEXT("Weapon, class and manual unarmed actions contribute"), Actions.Num(), 5);

	const FGridAvailableCombatAction* Quick = Actions.FindByPredicate(
		[](const FGridAvailableCombatAction& Action)
		{
			return Action.Definition.ActionId == TEXT("Action_QuickThrow");
		});
	const FGridAvailableCombatAction* Precise = Actions.FindByPredicate(
		[](const FGridAvailableCombatAction& Action)
		{
			return Action.Definition.ActionId == TEXT("Action_PreciseThrow");
		});
	const FGridAvailableCombatAction* Ability = Actions.FindByPredicate(
		[](const FGridAvailableCombatAction& Action)
		{
			return Action.Definition.ActionId == TEXT("Ability_Focus");
		});
	const FGridAvailableCombatAction* Spell = Actions.FindByPredicate(
		[](const FGridAvailableCombatAction& Action)
		{
			return Action.Definition.ActionId == TEXT("Spell_Fireball");
		});
	TestNotNull(TEXT("The first weapon action exists"), Quick);
	TestNotNull(TEXT("The second weapon action exists"), Precise);
	TestNotNull(TEXT("The class ability exists"), Ability);
	TestNotNull(TEXT("The class spell exists"), Spell);
	if (!Quick || !Precise || !Ability || !Spell)
	{
		return false;
	}

	TestEqual(TEXT("The quick throw keeps MainHand provenance"), Quick->SourceEquipmentSlot, EGridEquipmentSlot::MainHand);
	TestEqual(TEXT("The quick throw keeps the concrete item source"), Quick->SourceDefinitionId, FName(TEXT("Shuriken")));
	TestEqual(TEXT("The precise throw carries its own AP cost"), Precise->CurrentActionPointCost, 3);
	TestTrue(TEXT("The current axial monster is suggested to both throws"),
		Quick->SuggestedTargetId == Fixture.Monster->ResolvePersistenceId() && Precise->SuggestedTargetId == Fixture.Monster->ResolvePersistenceId());
	TestEqual(TEXT("An affordable future ability is catalogued but not executed yet"), Ability->AvailabilityReason,
		EGridCombatActionAvailabilityReason::ExecutionNotImplemented);
	TestEqual(TEXT("Mana is checked before the future spell executor"), Spell->AvailabilityReason, EGridCombatActionAvailabilityReason::InsufficientMana);
	TestFalse(TEXT("A torch without a declared action contributes nothing"),
		Actions.ContainsByPredicate(
			[](const FGridAvailableCombatAction& Action)
			{
				return Action.SourceDefinitionId == TEXT("Item_Torch");
			}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12GenericActionAttackLifecycleTest, "Grimrock.Monsters.MON12.ActionCatalog.GenericAttackLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12GenericActionAttackLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The MON12 generic-action fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	TArray<FGridAvailableCombatAction> Actions;
	Fixture.TurnManager->GetAvailableCombatActions(0, Actions);
	TestEqual(TEXT("The current shuriken and manual unarmed action are catalogued"), Actions.Num(), 2);
	const FGridAvailableCombatAction* ShurikenActionPtr = Actions.FindByPredicate(
		[](const FGridAvailableCombatAction& Candidate)
		{
			return Candidate.Definition.ActionId == FName(TEXT("Attack_Shuriken"));
		});
	if (!TestNotNull(TEXT("The current shuriken CombatAction is present"), ShurikenActionPtr))
	{
		return false;
	}
	const FGridAvailableCombatAction ShurikenAction = *ShurikenActionPtr;
	TestEqual(TEXT("The current CombatAction preserves the MON11 attack id"), ShurikenAction.Definition.ActionId, FName(TEXT("Attack_Shuriken")));
	TestTrue(TEXT("The shuriken CombatAction is currently enabled"), ShurikenAction.bEnabled);

	FGridCombatActionRequestResult Execution;
	TestTrue(TEXT("The generic action entry point executes the current MON11 attack"),
		Fixture.TurnManager->RequestCharacterCombatAction(0, ShurikenAction.Definition.ActionId, ShurikenAction.Definition.SourcePolicy,
			ShurikenAction.SourceDefinitionId, ShurikenAction.SourceEquipmentSlot, Execution));
	TestTrue(TEXT("The generic result reports acceptance"), Execution.bAccepted);
	TestEqual(TEXT("The resolved request keeps the offensive item"), Execution.AttackRequest.OffensiveItemDefinitionId, FName(TEXT("Shuriken")));
	TestEqual(TEXT("The generic action pays its definition AP cost"), Execution.AttackRequest.ActionPointCost, 2);

	FGridPlayerCharacterTurnState TurnState;
	TestTrue(TEXT("The turn state remains available after the generic attack"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, TurnState));
	TestEqual(TEXT("The generic attack leaves two of four AP"), TurnState.RemainingActionPoints, 2);

	UGridItemDefinitionAsset* ShurikenDefinition = Fixture.Party->PartyInventoryComponent->FindItemDefinition(FName(TEXT("Shuriken")));
	TestNotNull(TEXT("The current shuriken definition remains registered"), ShurikenDefinition);
	if (ShurikenDefinition)
	{
		if (FGridCombatActionDefinition* CurrentShurikenAction = ShurikenDefinition->CombatActions.FindByPredicate(
				[](const FGridCombatActionDefinition& Candidate)
				{
					return Candidate.ActionId == FName(TEXT("Attack_Shuriken"));
				}))
		{
			CurrentShurikenAction->ActionPointCost = 3;
		}
	}

	FGridCombatActionRequestResult RejectedExecution;
	TestFalse(TEXT("The catalogue is revalidated from the current CombatAction before a second request"),
		Fixture.TurnManager->RequestCharacterCombatAction(0, ShurikenAction.Definition.ActionId, ShurikenAction.Definition.SourcePolicy,
			ShurikenAction.SourceDefinitionId, ShurikenAction.SourceEquipmentSlot, RejectedExecution));
	TestEqual(TEXT("The unavailable request has a generic rejection"), RejectedExecution.RejectReason, EGridCombatActionRequestRejectReason::ActionUnavailable);
	TestEqual(TEXT("The precise disabled reason is insufficient AP"), RejectedExecution.Action.AvailabilityReason,
		EGridCombatActionAvailabilityReason::InsufficientActionPoints);
	TestTrue(TEXT("The turn state remains queryable after refusal"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, TurnState));
	TestEqual(TEXT("A refused generic action consumes no AP"), TurnState.RemainingActionPoints, 2);
	return true;
}

namespace
{
	void ResetMON12MonsterToFreshPlacedState(AGridMonsterActor* Monster)
	{
		if (!Monster)
		{
			return;
		}

		Monster->CurrentHealth = 0;
		Monster->CurrentPhysicalArmor = 0;
		Monster->CurrentMagicalArmor = 0;
		Monster->bCombatStatsInitialized = false;
		Monster->MonsterState = EGridMonsterState::Idle;
		Monster->ResetAnimationSignals();
		if (Monster->DeathComponent)
		{
			Monster->DeathComponent->RestoreLivingState();
		}
	}

	FGridRuntimeMonsterState MakeMON12RestoredMonsterState(const FGridMON12Fixture& Fixture, int32 CurrentHealth, bool bDead)
	{
		FGridRuntimeMonsterState State;
		State.PersistenceId = Fixture.Monster->ResolvePersistenceId();
		State.MonsterDefinitionId = Fixture.MonsterDefinition->MonsterId;
		State.DungeonLevelId = Fixture.Runtime->CurrentDungeonLevelId;
		State.CellX = Fixture.Monster->CurrentCell.X;
		State.CellY = Fixture.Monster->CurrentCell.Y;
		State.Facing = Fixture.Monster->Facing;
		State.MonsterState = bDead ? EGridMonsterState::Dead : EGridMonsterState::Alert;
		State.CurrentHealth = CurrentHealth;
		State.CurrentPhysicalArmor = 0;
		State.CurrentMagicalArmor = 0;
		State.bMonsterEnabled = true;
		State.bIsDead = bDead;
		return State;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12FreshPlacedCombatInitializationTest, "Grimrock.Monsters.MON12.InitializationRegression.FreshPlacedMonster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12FreshPlacedCombatInitializationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The fresh-monster fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	ResetMON12MonsterToFreshPlacedState(Fixture.Monster);
	Fixture.TurnManager->bCombatActive = false;
	Fixture.TurnManager->CombatMonsters.Reset();

	TArray<AGridMonsterActor*> Candidates = { Fixture.Monster, Fixture.Monster };
	TestTrue(TEXT("Combat starts with a fresh directly placed monster"), Fixture.TurnManager->StartCombatInternal(Candidates));
	TestEqual(TEXT("Fresh health is initialized to MaxHealth"), Fixture.Monster->CurrentHealth, Fixture.MonsterDefinition->MaxHealth);
	TestTrue(TEXT("Fresh combat statistics are initialized"), Fixture.Monster->bCombatStatsInitialized);
	TestFalse(TEXT("The fresh monster is alive"), Fixture.Monster->IsDead());
	TestEqual(TEXT("The duplicate candidate is admitted once"), Fixture.TurnManager->CombatMonsters.Num(), 1);

	const FGridCombatantInitiativeEntry* MonsterEntry = Fixture.TurnManager->InitiativeOrder.FindByPredicate(
		[&Fixture](const FGridCombatantInitiativeEntry& Entry)
		{
			return Entry.Side == EGridCombatantSide::Monster && Entry.CombatantId == Fixture.Monster->ResolvePersistenceId();
		});
	TestNotNull(TEXT("The fresh monster enters initiative"), MonsterEntry);
	if (MonsterEntry)
	{
		TestTrue(TEXT("The fresh initiative entry is not Defeated"), MonsterEntry->State != EGridCombatantTurnState::Defeated);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12RestoredInjuredCombatInitializationTest,
	"Grimrock.Monsters.MON12.InitializationRegression.RestoredInjuredMonster", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12RestoredInjuredCombatInitializationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The injured-monster fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	const int32 SavedHealth = 375;
	const FGridRuntimeMonsterState State = MakeMON12RestoredMonsterState(Fixture, SavedHealth, false);
	TestTrue(TEXT("The injured state is restored"), Fixture.Monster->RestoreRuntimeMonsterState(State, Fixture.Runtime));
	TestTrue(TEXT("The restored state is combat-ready"), Fixture.TurnManager->PrepareMonsterForCombat(Fixture.Monster));
	TestEqual(TEXT("Preparation preserves restored injured health"), Fixture.Monster->CurrentHealth, SavedHealth);
	TestTrue(TEXT("Restored injured statistics stay initialized"), Fixture.Monster->bCombatStatsInitialized);
	TestFalse(TEXT("The restored injured monster stays alive"), Fixture.Monster->IsDead());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12RestoredDeadCombatInitializationTest, "Grimrock.Monsters.MON12.InitializationRegression.RestoredDeadMonster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12RestoredDeadCombatInitializationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The dead-monster fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	const FGridRuntimeMonsterState State = MakeMON12RestoredMonsterState(Fixture, 0, true);
	TestTrue(TEXT("The dead state is restored"), Fixture.Monster->RestoreRuntimeMonsterState(State, Fixture.Runtime));
	TestFalse(TEXT("A restored corpse is refused from combat"), Fixture.TurnManager->PrepareMonsterForCombat(Fixture.Monster));
	TestEqual(TEXT("The restored corpse keeps zero health"), Fixture.Monster->CurrentHealth, 0);
	TestTrue(TEXT("The restored corpse remains initialized"), Fixture.Monster->bCombatStatsInitialized);
	TestTrue(TEXT("The restored corpse remains dead"), Fixture.Monster->IsDead());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12InvalidDefinitionAdmissionTest, "Grimrock.Monsters.MON12.InitializationRegression.InvalidDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12InvalidDefinitionAdmissionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The invalid-definition fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	Fixture.MonsterDefinition->ActionPointsPerTurn = 0;
	FString ValidationError;
	TestFalse(TEXT("The deliberately invalid definition is refused"), Fixture.MonsterDefinition->ValidateDefinition(ValidationError));
	TestTrue(TEXT("The precise invalid field is reported"), ValidationError.Contains(TEXT("ActionPointsPerTurn must be at least 1.")));

	Fixture.TurnManager->bCombatActive = false;
	Fixture.TurnManager->CombatMonsters.Reset();
	AddExpectedError(TEXT("Reason=InvalidDefinition"), EAutomationExpectedErrorFlags::Contains, 1);
	TArray<AGridMonsterActor*> Candidates = { Fixture.Monster };
	TestFalse(TEXT("Combat cannot start with only an invalid monster"), Fixture.TurnManager->StartCombatInternal(Candidates));
	TestTrue(TEXT("The invalid monster never enters CombatMonsters"), Fixture.TurnManager->CombatMonsters.IsEmpty());
	TestTrue(TEXT("The invalid monster never enters initiative"), Fixture.TurnManager->InitiativeOrder.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12UninitializedInitiativeStateTest, "Grimrock.Monsters.MON12.InitializationRegression.UninitializedInitiative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12UninitializedInitiativeStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The initiative guard fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	ResetMON12MonsterToFreshPlacedState(Fixture.Monster);
	Fixture.TurnManager->InitiativeOrder.Reset();
	AddExpectedError(TEXT("Reason=UninitializedCombatState"), EAutomationExpectedErrorFlags::Contains, 1);
	Fixture.TurnManager->BuildGlobalInitiativeOrder();
	const FGuid MonsterId = Fixture.Monster->ResolvePersistenceId();
	TestFalse(TEXT("An uninitialized monster is excluded from initiative"),
		Fixture.TurnManager->InitiativeOrder.ContainsByPredicate(
			[&MonsterId](const FGridCombatantInitiativeEntry& Entry)
			{
				return Entry.Side == EGridCombatantSide::Monster && Entry.CombatantId == MonsterId;
			}));

	FGridCombatantInitiativeEntry UninitializedEntry;
	UninitializedEntry.CombatantId = MonsterId;
	UninitializedEntry.Side = EGridCombatantSide::Monster;
	UninitializedEntry.CurrentHealth = 0;
	UninitializedEntry.MaximumHealth = Fixture.MonsterDefinition->MaxHealth;
	UninitializedEntry.State = EGridCombatantTurnState::Waiting;
	Fixture.TurnManager->InitiativeOrder.Add(UninitializedEntry);
	Fixture.TurnManager->ResetInitiativeRound();
	const FGridCombatantInitiativeEntry* Refreshed = Fixture.TurnManager->FindInitiativeEntry(EGridCombatantSide::Monster, MonsterId);
	TestNotNull(TEXT("The defensive initiative entry remains queryable"), Refreshed);
	if (Refreshed)
	{
		TestTrue(TEXT("Raw zero health does not imply Defeated"), Refreshed->State != EGridCombatantTurnState::Defeated);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12InitializedPersistenceCaptureTest, "Grimrock.Monsters.MON12.InitializationRegression.ValidPersistenceCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12InitializedPersistenceCaptureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON12Fixture Fixture;
	if (!TestTrue(TEXT("The capture fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	ResetMON12MonsterToFreshPlacedState(Fixture.Monster);
	TestTrue(TEXT("Fresh preparation succeeds before capture"), Fixture.TurnManager->PrepareMonsterForCombat(Fixture.Monster));
	FGridRuntimeMonsterState CapturedState;
	TestTrue(
		TEXT("A valid initialized monster is captured"), Fixture.Monster->CaptureRuntimeMonsterState(CapturedState, Fixture.Runtime->CurrentDungeonLevelId));
	TestEqual(TEXT("The captured monster keeps MaxHealth"), CapturedState.CurrentHealth, Fixture.MonsterDefinition->MaxHealth);
	TestFalse(TEXT("The captured fresh monster is alive"), CapturedState.bIsDead);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12RatGiantAssetCompatibilityTest, "Grimrock.Monsters.MON12.InitializationRegression.RatGiantAssetCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12RatGiantAssetCompatibilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridMonsterDefinitionAsset* RatDefinition =
		LoadObject<UGridMonsterDefinitionAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"));
	if (!TestNotNull(TEXT("DA_MON_RatGiant loads"), RatDefinition))
	{
		return false;
	}

	FString ValidationError;
	TestTrue(TEXT("DA_MON_RatGiant is valid after load normalization"), RatDefinition->ValidateDefinition(ValidationError));
	TestTrue(TEXT("DA_MON_RatGiant has no validation error"), ValidationError.IsEmpty());
	return true;
}

#endif
