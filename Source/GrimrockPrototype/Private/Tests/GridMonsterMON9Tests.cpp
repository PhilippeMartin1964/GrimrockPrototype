#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
	const FName MON9SingleLevelId(TEXT("SingleLevel"));

	struct FGridMON9TestWorld
	{
		UWorld* World = nullptr;

		FGridMON9TestWorld()
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
				FName(*FString::Printf(TEXT("MON9TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FGridMON9TestWorld()
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

	struct FGridMON9TestSaveSlot
	{
		FString SlotName;
		int32 UserIndex = 0;

		explicit FGridMON9TestSaveSlot(const FString& InSlotName)
			: SlotName(InSlotName)
		{
			UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
		}

		~FGridMON9TestSaveSlot()
		{
			UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
		}
	};

	UGridLevelAsset* MakeMON9Floor(UObject* Outer, int32 Width = 6, int32 Height = 6)
	{
		UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(Outer);
		LevelAsset->Width = Width;
		LevelAsset->Height = Height;
		LevelAsset->StartCellX = Width - 1;
		LevelAsset->StartCellY = Height - 1;
		LevelAsset->StartFacing = EGridEdge::North;
		LevelAsset->EnsureCellCount();
		for (FGridLevelCellData& Cell : LevelAsset->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return LevelAsset;
	}

	UGridMonsterDefinitionAsset* MakeMON9Definition(UObject* Outer, FName MonsterId = TEXT("MON9_Rat"))
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Outer);
		Definition->MonsterId = MonsterId;
		Definition->DisplayName = FText::FromString(TEXT("Rat MON9"));
		Definition->CategoryId = TEXT("Vermin");
		Definition->DangerLevel = 1;
		Definition->MaxHealth = 12;
		Definition->PhysicalArmor = 5;
		Definition->MagicalArmor = 4;
		Definition->ActionPointsPerTurn = 2;
		Definition->GridFootprint = FIntPoint(1, 1);
		Definition->DeathExpectedDuration = 1.0f;
		return Definition;
	}

	void AddGuaranteedMON9Loot(UGridMonsterDefinitionAsset* MonsterDefinition, FName ItemDefinitionId)
	{
		UGridItemDefinitionAsset* ItemDefinition = NewObject<UGridItemDefinitionAsset>(MonsterDefinition);
		ItemDefinition->ItemDefinitionId = ItemDefinitionId;
		ItemDefinition->DisplayName = FText::FromName(ItemDefinitionId);
		ItemDefinition->Weight = 1.0f;

		FGridMonsterLootEntry Entry;
		Entry.ItemDefinitionId = ItemDefinitionId;
		Entry.ItemDefinitionAsset = ItemDefinition;
		Entry.DropChance = 1.0f;
		Entry.MinQuantity = 1;
		Entry.MaxQuantity = 1;
		MonsterDefinition->LootTable.Add(Entry);
	}

	AGridMonsterActor* SpawnMON9Monster(UWorld* World, UGridMonsterDefinitionAsset* Definition, const FGuid& PersistenceId, FName HomeLevelId, FIntPoint Cell,
		FName ActorName, bool bAddRuntimeComponents = true)
	{
		if (!World || !Definition)
		{
			return nullptr;
		}

		FActorSpawnParameters Params;
		Params.Name = ActorName;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AGridMonsterActor* Monster = World->SpawnActor<AGridMonsterActor>(AGridMonsterActor::StaticClass(), FTransform::Identity, Params);
		if (!Monster)
		{
			return nullptr;
		}

		Monster->MonsterDefinition = Definition;
		Monster->PersistentMonsterId = PersistenceId;
		Monster->HomeDungeonLevelId = HomeLevelId;
		Monster->CurrentCell = Cell;
		Monster->Facing = EGridEdge::North;
		Monster->MonsterState = EGridMonsterState::Idle;
		Monster->CurrentHealth = Definition->MaxHealth;
		Monster->CurrentPhysicalArmor = Definition->PhysicalArmor;
		Monster->CurrentMagicalArmor = Definition->MagicalArmor;
		Monster->bCombatStatsInitialized = true;
		Monster->bMonsterEnabled = true;

		if (bAddRuntimeComponents)
		{
			UGridMonsterMovementComponent* Movement =
				NewObject<UGridMonsterMovementComponent>(Monster, *FString::Printf(TEXT("%s_Movement"), *ActorName.ToString()));
			Movement->bAutoInitialize = false;
			Movement->bInferCellFromActorLocation = false;
			Monster->AddInstanceComponent(Movement);
			Movement->RegisterComponent();

			UGridMonsterBehaviorComponent* Behavior =
				NewObject<UGridMonsterBehaviorComponent>(Monster, *FString::Printf(TEXT("%s_Behavior"), *ActorName.ToString()));
			Behavior->bAutoInitialize = false;
			Behavior->bRefreshPerceptionOnBeginPlay = false;
			Monster->AddInstanceComponent(Behavior);
			Behavior->RegisterComponent();
		}

		return Monster;
	}

	AGridMonsterActor* SpawnMON9DirectPlacedMonster(
		UWorld* World, UGridMonsterDefinitionAsset* Definition, const FGuid& PersistenceId, FName HomeLevelId, const FVector& WorldLocation, FName ActorName)
	{
		if (!World || !Definition)
		{
			return nullptr;
		}

		FActorSpawnParameters Params;
		Params.Name = ActorName;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AGridMonsterActor* Monster =
			World->SpawnActor<AGridMonsterActor>(AGridMonsterActor::StaticClass(), FTransform(FRotator::ZeroRotator, WorldLocation), Params);
		if (!Monster)
		{
			return nullptr;
		}

		Monster->MonsterDefinition = Definition;
		Monster->PersistentMonsterId = PersistenceId;
		Monster->HomeDungeonLevelId = HomeLevelId;
		Monster->Facing = EGridEdge::North;
		Monster->MonsterState = EGridMonsterState::Idle;
		Monster->CurrentHealth = Definition->MaxHealth;
		Monster->CurrentPhysicalArmor = Definition->PhysicalArmor;
		Monster->CurrentMagicalArmor = Definition->MagicalArmor;
		Monster->bCombatStatsInitialized = true;
		Monster->bMonsterEnabled = true;

		UGridMonsterMovementComponent* Movement =
			NewObject<UGridMonsterMovementComponent>(Monster, *FString::Printf(TEXT("%s_Movement"), *ActorName.ToString()));
		Movement->bAutoInitialize = false;
		Movement->bInferCellFromActorLocation = true;
		Movement->bSnapToCellOnInitialize = true;
		Monster->AddInstanceComponent(Movement);
		Movement->RegisterComponent();
		return Monster;
	}

	AGrimrockPartyPawn* SpawnMON9Party(UWorld* World, AGridLevelRuntimeActor* RuntimeActor, FIntPoint Cell = FIntPoint(5, 5))
	{
		if (!World || !RuntimeActor)
		{
			return nullptr;
		}

		AGrimrockPartyPawn* Party = World->SpawnActor<AGrimrockPartyPawn>();
		if (Party)
		{
			Party->LevelRuntimeActor = RuntimeActor;
			Party->CurrentCellX = Cell.X;
			Party->CurrentCellY = Cell.Y;
			Party->Facing = EGridEdge::North;
			Party->SetActorLocation(RuntimeActor->GetCellCenterWorld(Cell.X, Cell.Y, Party->EyeHeight));
		}
		return Party;
	}

	bool InitializeMON9Movement(AGridMonsterActor* Monster, AGridLevelRuntimeActor* RuntimeActor)
	{
		UGridMonsterMovementComponent* Movement = Monster ? Monster->FindComponentByClass<UGridMonsterMovementComponent>() : nullptr;
		return Movement && Movement->InitializeMovement(RuntimeActor);
	}

	void ResetMON9CombatStateForPreBeginPlay(AGridMonsterActor* Monster)
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON9DirectPlacedInitialCellInferenceTest, "Grimrock.Monsters.MON9.DirectPlacedInitialCellInference",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9DirectPlacedInitialCellInferenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("[GridMonsterCombat] Initialization failed."), EAutomationExpectedErrorFlags::Contains, 1);
	FGridMON9TestWorld TestWorld;
	if (!TestNotNull(TEXT("The direct-placement test world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* LevelAsset = MakeMON9Floor(Runtime);
	LevelAsset->GetCellMutable(0, 0).CellType = EGridCellType::Empty;
	LevelAsset->GetCellMutable(0, 0).bBlocksOccupancy = true;
	Runtime->LevelAsset = LevelAsset;

	UGridMonsterDefinitionAsset* Definition = MakeMON9Definition(Runtime, TEXT("MON9_DirectPlacedRat"));
	const FGuid PersistenceId(98, 1, 1, 1);
	const FIntPoint ExpectedCell(2, 3);
	const FVector ExpectedLocation = Runtime->GetCellCenterWorld(ExpectedCell.X, ExpectedCell.Y);
	AGridMonsterActor* Monster =
		SpawnMON9DirectPlacedMonster(TestWorld.World, Definition, PersistenceId, MON9SingleLevelId, ExpectedLocation, TEXT("MON9_DirectPlacedRat"));
	if (!TestNotNull(TEXT("The directly placed rat exists"), Monster))
	{
		return false;
	}

	TestTrue(TEXT("CurrentCell starts at its default value"), Monster->CurrentCell == FIntPoint::ZeroValue);
	TestFalse(TEXT("An unvisited level uses its initial-state path"), Runtime->ApplyCurrentLevelRuntimeState());
	TestTrue(TEXT("The initial world position is inferred as cell (2,3)"), Monster->CurrentCell == ExpectedCell);
	TestTrue(TEXT("The directly placed rat snaps to cell (2,3)"), Monster->GetActorLocation().Equals(ExpectedLocation, KINDA_SMALL_NUMBER));

	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	TestNotNull(TEXT("The occupancy subsystem exists"), Occupancy);
	TestTrue(TEXT("The directly placed rat occupies cell (2,3)"), Occupancy && Occupancy->GetRegistry().GetOccupantId(ExpectedCell) == PersistenceId);
	TestFalse(TEXT("The invalid default cell is not occupied"), Occupancy && Occupancy->IsCellOccupied(FIntPoint::ZeroValue));

	TestTrue(TEXT("The inferred monster is captured"), Runtime->CaptureCurrentLevelRuntimeState());
	const FGridLevelRuntimeState* LevelState = Runtime->FindRuntimeStateForCurrentLevel();
	const FGridRuntimeMonsterState* MonsterState = LevelState ? LevelState->Monsters.Find(PersistenceId) : nullptr;
	TestNotNull(TEXT("The inferred monster has a saved state"), MonsterState);
	if (MonsterState)
	{
		TestEqual(TEXT("The saved cell X is inferred correctly"), MonsterState->CellX, ExpectedCell.X);
		TestEqual(TEXT("The saved cell Y is inferred correctly"), MonsterState->CellY, ExpectedCell.Y);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON9RestoredCellNotReinferredTest, "Grimrock.Monsters.MON9.RestoredCellNotReinferredFromStaleLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9RestoredCellNotReinferredTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON9TestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	Runtime->LevelAsset = MakeMON9Floor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON9Definition(Runtime, TEXT("MON9_RestoredCellRat"));
	const FGuid PersistenceId(98, 1, 1, 2);
	const FIntPoint StaleCell(1, 1);
	const FIntPoint RestoredCell(4, 2);
	AGridMonsterActor* Monster = SpawnMON9DirectPlacedMonster(
		TestWorld.World, Definition, PersistenceId, MON9SingleLevelId, Runtime->GetCellCenterWorld(StaleCell.X, StaleCell.Y), TEXT("MON9_RestoredCellRat"));
	if (!Monster)
	{
		return false;
	}

	FGridRuntimeMonsterState SavedMonsterState;
	SavedMonsterState.PersistenceId = PersistenceId;
	SavedMonsterState.MonsterDefinitionId = Definition->MonsterId;
	SavedMonsterState.DungeonLevelId = MON9SingleLevelId;
	SavedMonsterState.CellX = RestoredCell.X;
	SavedMonsterState.CellY = RestoredCell.Y;
	SavedMonsterState.Facing = EGridEdge::East;
	SavedMonsterState.MonsterState = EGridMonsterState::Idle;
	SavedMonsterState.CurrentHealth = 9;
	SavedMonsterState.CurrentPhysicalArmor = 3;
	SavedMonsterState.CurrentMagicalArmor = 2;
	SavedMonsterState.bMonsterEnabled = true;

	FGridLevelRuntimeState LevelState;
	LevelState.LevelId = MON9SingleLevelId;
	LevelState.bHasBeenVisited = true;
	LevelState.Monsters.Add(PersistenceId, SavedMonsterState);
	Runtime->DungeonRuntimeState.LevelStates.Add(MON9SingleLevelId, LevelState);

	TestTrue(TEXT("The saved MON9 state applies"), Runtime->ApplyCurrentLevelRuntimeState());
	TestTrue(TEXT("The saved cell overrides the stale actor location"), Monster->CurrentCell == RestoredCell);
	TestTrue(TEXT("The restored actor is centered on cell (4,2)"),
		Monster->GetActorLocation().Equals(Runtime->GetCellCenterWorld(RestoredCell.X, RestoredCell.Y), KINDA_SMALL_NUMBER));

	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	TestTrue(TEXT("The restored cell contains the rat"), Occupancy && Occupancy->GetRegistry().GetOccupantId(RestoredCell) == PersistenceId);
	TestFalse(TEXT("The stale initial cell is not occupied"), Occupancy && Occupancy->IsCellOccupied(StaleCell));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON9DirectPlacedNewGameStartsAliveTest, "Grimrock.Monsters.MON9.DirectPlacedNewGameStartsAlive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9DirectPlacedNewGameStartsAliveTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("[GridMonsterCombat] Initialization failed."), EAutomationExpectedErrorFlags::Contains, 1);
	FGridMON9TestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	Runtime->LevelAsset = MakeMON9Floor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON9Definition(Runtime, TEXT("MON9_NewGameRat"));
	const FGuid PersistenceId(98, 2, 1, 1);
	const FIntPoint ExpectedCell(3, 2);
	AGridMonsterActor* Monster = SpawnMON9DirectPlacedMonster(
		TestWorld.World, Definition, PersistenceId, MON9SingleLevelId, Runtime->GetCellCenterWorld(ExpectedCell.X, ExpectedCell.Y), TEXT("MON9_NewGameRat"));
	if (!Monster)
	{
		return false;
	}
	ResetMON9CombatStateForPreBeginPlay(Monster);

	TestFalse(TEXT("Zero uninitialized health is not a logical death"), Monster->IsDead());
	TestFalse(TEXT("The new level takes the initial-state path"), Runtime->ApplyCurrentLevelRuntimeState());
	TestFalse(TEXT("The directly placed rat starts alive"), Monster->IsDead());
	TestTrue(TEXT("The rat does not enter the Dead state"), Monster->MonsterState != EGridMonsterState::Dead);
	TestEqual(TEXT("Fresh health is initialized to MaxHealth"), Monster->CurrentHealth, Definition->MaxHealth);
	TestTrue(TEXT("Fresh combat statistics are committed"), Monster->bCombatStatsInitialized);
	TestFalse(TEXT("Fresh activation does not commit death"), Monster->DeathComponent->bDeathCommitted);
	TestFalse(TEXT("Fresh activation does not generate loot"), Monster->DeathComponent->bLootGenerated);
	TestTrue(TEXT("The fresh rat cell is inferred"), Monster->CurrentCell == ExpectedCell);

	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	TestTrue(TEXT("The fresh rat occupies its inferred cell"), Occupancy && Occupancy->GetRegistry().GetOccupantId(ExpectedCell) == PersistenceId);

	TestTrue(TEXT("The fresh living rat is captured"), Runtime->CaptureCurrentLevelRuntimeState());
	const FGridLevelRuntimeState* LevelState = Runtime->FindRuntimeStateForCurrentLevel();
	const FGridRuntimeMonsterState* SavedState = LevelState ? LevelState->Monsters.Find(PersistenceId) : nullptr;
	TestNotNull(TEXT("The fresh rat has a MON9 state"), SavedState);
	if (SavedState)
	{
		TestFalse(TEXT("The fresh rat is captured alive"), SavedState->bIsDead);
		TestEqual(TEXT("The fresh rat keeps MaxHealth in MON9"), SavedState->CurrentHealth, Definition->MaxHealth);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON9BeginPlayOrderIndependenceTest, "Grimrock.Monsters.MON9.BeginPlayOrderIndependence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9BeginPlayOrderIndependenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("[GridMonsterCombat] Initialization failed."), EAutomationExpectedErrorFlags::Contains, 4);

	const auto RunOrder = [this](bool bMonsterBeginsFirst, uint32 IdSuffix)
	{
		FGridMON9TestWorld TestWorld;
		if (!TestWorld.World)
		{
			return false;
		}

		AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
		Runtime->LevelAsset = MakeMON9Floor(Runtime);
		UGridMonsterDefinitionAsset* Definition =
			MakeMON9Definition(Runtime, bMonsterBeginsFirst ? TEXT("MON9_OrderMonsterFirst") : TEXT("MON9_OrderRuntimeFirst"));
		const FGuid PersistenceId(98, 2, 2, IdSuffix);
		const FIntPoint ExpectedCell(2, 4);
		AGridMonsterActor* Monster = SpawnMON9DirectPlacedMonster(TestWorld.World, Definition, PersistenceId, MON9SingleLevelId,
			Runtime->GetCellCenterWorld(ExpectedCell.X, ExpectedCell.Y), bMonsterBeginsFirst ? TEXT("MON9_OrderMonsterFirst") : TEXT("MON9_OrderRuntimeFirst"));
		if (!Monster)
		{
			return false;
		}
		ResetMON9CombatStateForPreBeginPlay(Monster);

		if (bMonsterBeginsFirst)
		{
			Monster->DispatchBeginPlay();
			Runtime->ApplyCurrentLevelRuntimeState();
		}
		else
		{
			Runtime->ApplyCurrentLevelRuntimeState();
			Monster->DispatchBeginPlay();
		}

		UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
		const FString OrderText = bMonsterBeginsFirst ? TEXT("MonsterFirst") : TEXT("RuntimeFirst");
		TestFalse(*FString::Printf(TEXT("%s remains alive"), *OrderText), Monster->IsDead());
		TestEqual(*FString::Printf(TEXT("%s initializes MaxHealth"), *OrderText), Monster->CurrentHealth, Definition->MaxHealth);
		TestTrue(*FString::Printf(TEXT("%s infers the same cell"), *OrderText), Monster->CurrentCell == ExpectedCell);
		TestTrue(*FString::Printf(TEXT("%s registers the same occupation"), *OrderText),
			Occupancy && Occupancy->GetRegistry().GetOccupantId(ExpectedCell) == PersistenceId);
		TestEqual(*FString::Printf(TEXT("%s emits no death"), *OrderText), Monster->DeathComponent->LogicalDeathEventCount, 0);
		TestFalse(*FString::Printf(TEXT("%s generates no loot"), *OrderText), Monster->DeathComponent->bLootGenerated);
		return true;
	};

	TestTrue(TEXT("Monster BeginPlay before runtime activation is stable"), RunOrder(true, 1));
	TestTrue(TEXT("Runtime activation before Monster BeginPlay is stable"), RunOrder(false, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON9RestoredDeadRemainsDeadTest, "Grimrock.Monsters.MON9.RestoredDeadRemainsDead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9RestoredDeadRemainsDeadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON9TestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	Runtime->LevelAsset = MakeMON9Floor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON9Definition(Runtime, TEXT("MON9_RestoredDeadRat"));
	AddGuaranteedMON9Loot(Definition, TEXT("Item_MON9RestoredDeadLoot"));
	const FGuid PersistenceId(98, 2, 3, 1);
	const FIntPoint DeathCell(4, 1);
	AGridMonsterActor* Monster = SpawnMON9DirectPlacedMonster(
		TestWorld.World, Definition, PersistenceId, MON9SingleLevelId, Runtime->GetCellCenterWorld(1, 1), TEXT("MON9_RestoredDeadRat"));
	if (!Monster)
	{
		return false;
	}

	FGridRuntimeMonsterState SavedState;
	SavedState.PersistenceId = PersistenceId;
	SavedState.MonsterDefinitionId = Definition->MonsterId;
	SavedState.DungeonLevelId = MON9SingleLevelId;
	SavedState.CellX = DeathCell.X;
	SavedState.CellY = DeathCell.Y;
	SavedState.Facing = EGridEdge::South;
	SavedState.MonsterState = EGridMonsterState::Dead;
	SavedState.CurrentHealth = 0;
	SavedState.CurrentPhysicalArmor = 2;
	SavedState.CurrentMagicalArmor = 1;
	SavedState.bMonsterEnabled = true;
	SavedState.bIsDead = true;

	FGridLevelRuntimeState LevelState;
	LevelState.LevelId = MON9SingleLevelId;
	LevelState.bHasBeenVisited = true;
	LevelState.Monsters.Add(PersistenceId, SavedState);
	Runtime->DungeonRuntimeState.LevelStates.Add(MON9SingleLevelId, LevelState);

	TestTrue(TEXT("The committed dead state applies"), Runtime->ApplyCurrentLevelRuntimeState());
	TestTrue(TEXT("The restored rat remains dead"), Monster->IsDead());
	TestEqual(TEXT("The restored dead rat keeps zero health"), Monster->CurrentHealth, 0);
	TestTrue(TEXT("Death commitment remains true"), Monster->DeathComponent->bDeathCommitted);
	TestTrue(TEXT("Loot generation remains committed"), Monster->DeathComponent->bLootGenerated);
	TestEqual(TEXT("Restoration creates no new loot"), Monster->DeathComponent->GeneratedLoot.Num(), 0);
	TestEqual(TEXT("Restoration emits no new death"), Monster->DeathComponent->LogicalDeathEventCount, 0);
	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	TestFalse(TEXT("The restored corpse occupies no cell"), Occupancy && Occupancy->IsCellOccupied(DeathCell));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON9MovementAutoInitSkipsDeadTest, "Grimrock.Monsters.MON9.MovementAutoInitSkipsDead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9MovementAutoInitSkipsDeadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON9TestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	Runtime->LevelAsset = MakeMON9Floor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON9Definition(Runtime, TEXT("MON9_AutoInitDeadRat"));
	const FIntPoint DeathCell(3, 3);
	AGridMonsterActor* Monster = SpawnMON9Monster(TestWorld.World, Definition, FGuid(98, 2, 4, 1), MON9SingleLevelId, DeathCell, TEXT("MON9_AutoInitDeadRat"));
	if (!Monster)
	{
		return false;
	}

	UGridMonsterMovementComponent* Movement = Monster->FindComponentByClass<UGridMonsterMovementComponent>();
	if (!Movement)
	{
		return false;
	}
	Movement->bAutoInitialize = true;
	Monster->SetActorLocation(Runtime->GetCellCenterWorld(DeathCell.X, DeathCell.Y));
	Monster->DeathComponent->InitializeDeathComponent(Runtime);
	Monster->MarkDead();

	Movement->BeginPlay();

	TestFalse(TEXT("Late auto-initialization leaves dead movement uninitialized"), Movement->IsInitialized());
	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	TestFalse(TEXT("Late auto-initialization does not occupy the death cell"), Occupancy && Occupancy->IsCellOccupied(DeathCell));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON9StateRoundTripTest, "Grimrock.Monsters.MON9.StateRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9StateRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON9TestWorld TestWorld;
	if (!TestNotNull(TEXT("The MON9 state world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	Runtime->LevelAsset = MakeMON9Floor(Runtime);
	SpawnMON9Party(TestWorld.World, Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON9Definition(Runtime);
	const FGuid PersistenceId(91, 1, 1, 1);
	AGridMonsterActor* Monster = SpawnMON9Monster(TestWorld.World, Definition, PersistenceId, MON9SingleLevelId, FIntPoint(2, 3), TEXT("MON9_StateRat"));
	if (!TestNotNull(TEXT("The state rat exists"), Monster) || !TestTrue(TEXT("The state rat occupies its cell"), InitializeMON9Movement(Monster, Runtime)))
	{
		return false;
	}

	Monster->Facing = EGridEdge::East;
	Monster->CurrentHealth = 7;
	Monster->CurrentPhysicalArmor = 3;
	Monster->CurrentMagicalArmor = 2;
	Monster->MonsterState = EGridMonsterState::Pursuing;
	Monster->EncounterGroupId = TEXT("Encounter_MON9");
	UGridMonsterBehaviorComponent* Behavior = Monster->FindComponentByClass<UGridMonsterBehaviorComponent>();
	Behavior->bHasLastKnownPartyCell = true;
	Behavior->LastKnownPartyCell = FIntPoint(4, 4);

	FGridRuntimeMonsterState SavedState;
	TestTrue(TEXT("The living state is captured"), Monster->CaptureRuntimeMonsterState(SavedState, MON9SingleLevelId));

	UGridMonsterMovementComponent* Movement = Monster->FindComponentByClass<UGridMonsterMovementComponent>();
	Movement->ReleaseOccupancy();
	Monster->CurrentCell = FIntPoint(1, 1);
	Monster->Facing = EGridEdge::South;
	Monster->CurrentHealth = 1;
	Monster->CurrentPhysicalArmor = 0;
	Monster->CurrentMagicalArmor = 0;
	Monster->MonsterState = EGridMonsterState::Hurt;
	Monster->EncounterGroupId = NAME_None;
	Behavior->bHasLastKnownPartyCell = false;
	Behavior->LastKnownPartyCell = FIntPoint::ZeroValue;

	TestTrue(TEXT("The living state restores"), Monster->RestoreRuntimeMonsterState(SavedState, Runtime));
	TestTrue(TEXT("The persistence id survives"), Monster->ResolvePersistenceId() == PersistenceId);
	TestTrue(TEXT("The moved cell survives"), Monster->CurrentCell == FIntPoint(2, 3));
	TestEqual(TEXT("The facing survives"), Monster->Facing, EGridEdge::East);
	TestEqual(TEXT("The health survives"), Monster->CurrentHealth, 7);
	TestEqual(TEXT("Physical armor survives"), Monster->CurrentPhysicalArmor, 3);
	TestEqual(TEXT("Magical armor survives"), Monster->CurrentMagicalArmor, 2);
	TestEqual(TEXT("The stable logical state survives"), Monster->MonsterState, EGridMonsterState::Pursuing);
	TestEqual(TEXT("The encounter group survives"), Monster->EncounterGroupId, FName(TEXT("Encounter_MON9")));
	TestTrue(TEXT("The last-known flag survives"), Behavior->bHasLastKnownPartyCell);
	TestTrue(TEXT("The last-known cell survives"), Behavior->LastKnownPartyCell == FIntPoint(4, 4));

	FGridRuntimeMonsterState TransientState = SavedState;
	TransientState.MonsterState = EGridMonsterState::Attacking;
	TestTrue(TEXT("An attacking snapshot restores safely"), Monster->RestoreRuntimeMonsterState(TransientState, Runtime));
	TestEqual(TEXT("Attacking is normalized to Pursuing"), Monster->MonsterState, EGridMonsterState::Pursuing);
	TestFalse(TEXT("No movement animation is restored"), Monster->bIsMoving);
	TestFalse(TEXT("No turn animation is restored"), Monster->bIsTurning);
	TestFalse(TEXT("No movement action is restored"), Movement->IsBusy());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON9DeadRoundTripTest, "Grimrock.Monsters.MON9.DeadRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9DeadRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON9TestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	Runtime->LevelAsset = MakeMON9Floor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON9Definition(Runtime, TEXT("MON9_DeadRat"));
	AddGuaranteedMON9Loot(Definition, TEXT("Item_MON9LootA"));
	AddGuaranteedMON9Loot(Definition, TEXT("Item_MON9LootB"));
	AGridMonsterActor* Monster = SpawnMON9Monster(TestWorld.World, Definition, FGuid(92, 1, 1, 1), MON9SingleLevelId, FIntPoint(2, 2), TEXT("MON9_DeadRat"));
	if (!Monster || !InitializeMON9Movement(Monster, Runtime))
	{
		return false;
	}

	Monster->DeathComponent->InitializeDeathComponent(Runtime);
	Monster->MarkDead();
	TestEqual(TEXT("The death places both guaranteed loot entries"), Monster->DeathComponent->PlacedLootCount, 2);
	TestEqual(TEXT("The death tracks both generated items"), Monster->DeathComponent->GeneratedLoot.Num(), 2);
	TestTrue(TEXT("The two generated runtime ids are distinct"),
		Monster->DeathComponent->GeneratedLoot.Num() == 2 &&
			Monster->DeathComponent->GeneratedLoot[0].RuntimeObjectId != Monster->DeathComponent->GeneratedLoot[1].RuntimeObjectId);
	TestTrue(TEXT("The level captures both world items"), Runtime->CaptureCurrentLevelRuntimeState());
	const FGridLevelRuntimeState* LevelStateBeforeRestore = Runtime->FindRuntimeStateForCurrentLevel();
	const int32 RuntimeItemCountBeforeRestore = LevelStateBeforeRestore ? LevelStateBeforeRestore->Items.Num() : 0;
	TestEqual(TEXT("Both loot items are persisted independently"), RuntimeItemCountBeforeRestore, 2);

	FGridRuntimeMonsterState SavedState;
	TestTrue(TEXT("The dead state is captured"), Monster->CaptureRuntimeMonsterState(SavedState, MON9SingleLevelId));
	const int32 LogicalDeathEvents = Monster->DeathComponent->LogicalDeathEventCount;
	const int32 LinkAttempts = Monster->DeathComponent->LinkExecutionAttemptCount;
	const int32 LootCount = Monster->DeathComponent->GeneratedLoot.Num();

	TestTrue(TEXT("The dead state restores"), Monster->RestoreRuntimeMonsterState(SavedState, Runtime));
	TestTrue(TEXT("The rat remains dead"), Monster->IsDead());
	TestEqual(TEXT("Dead health is zero"), Monster->CurrentHealth, 0);
	TestEqual(TEXT("The logical state remains Dead"), Monster->MonsterState, EGridMonsterState::Dead);
	TestTrue(TEXT("The corpse remains visible"), !Monster->IsHidden());
	TestEqual(TEXT("Corpse collision remains disabled"), Monster->CollisionComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);

	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	TestFalse(TEXT("The death cell remains free"), Occupancy->IsCellOccupied(FIntPoint(2, 2)));
	TestEqual(TEXT("No new logical death is emitted"), Monster->DeathComponent->LogicalDeathEventCount, LogicalDeathEvents);
	TestEqual(TEXT("No link is executed again"), Monster->DeathComponent->LinkExecutionAttemptCount, LinkAttempts);
	TestEqual(TEXT("Existing loot metadata is preserved"), Monster->DeathComponent->GeneratedLoot.Num(), LootCount);
	TestTrue(TEXT("Loot generation stays committed"), Monster->DeathComponent->bLootGenerated);
	TestTrue(TEXT("Death commitment stays restored"), Monster->DeathComponent->bDeathCommitted);
	const FGridLevelRuntimeState* LevelStateAfterRestore = Runtime->FindRuntimeStateForCurrentLevel();
	TestEqual(TEXT("Restoring the dead monster does not change runtime items"), LevelStateAfterRestore ? LevelStateAfterRestore->Items.Num() : 0,
		RuntimeItemCountBeforeRestore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON9OccupancyRestoreTest, "Grimrock.Monsters.MON9.OccupancyRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9OccupancyRestoreTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON9TestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	Runtime->LevelAsset = MakeMON9Floor(Runtime);
	SpawnMON9Party(TestWorld.World, Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON9Definition(Runtime, TEXT("MON9_OccupancyRat"));
	const FGuid IdA(93, 1, 1, 1);
	const FGuid IdB(93, 1, 1, 2);
	AGridMonsterActor* RatA = SpawnMON9Monster(TestWorld.World, Definition, IdA, MON9SingleLevelId, FIntPoint(1, 2), TEXT("MON9_OccupancyA"));
	AGridMonsterActor* RatB = SpawnMON9Monster(TestWorld.World, Definition, IdB, MON9SingleLevelId, FIntPoint(3, 2), TEXT("MON9_OccupancyB"));
	if (!RatA || !RatB || !InitializeMON9Movement(RatA, Runtime) || !InitializeMON9Movement(RatB, Runtime))
	{
		return false;
	}

	FGridRuntimeMonsterState StateA;
	FGridRuntimeMonsterState StateB;
	RatA->CaptureRuntimeMonsterState(StateA, MON9SingleLevelId);
	RatB->CaptureRuntimeMonsterState(StateB, MON9SingleLevelId);

	UGridMonsterMovementComponent* MoveA = RatA->FindComponentByClass<UGridMonsterMovementComponent>();
	UGridMonsterMovementComponent* MoveB = RatB->FindComponentByClass<UGridMonsterMovementComponent>();
	MoveA->ReleaseOccupancy();
	MoveB->ReleaseOccupancy();
	RatA->CurrentCell = FIntPoint(4, 1);
	RatB->CurrentCell = FIntPoint(4, 3);

	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	Occupancy->ResetRegistry();
	TestTrue(TEXT("Rat A restores its occupied cell"), RatA->RestoreRuntimeMonsterState(StateA, Runtime));
	TestTrue(TEXT("Rat B restores its occupied cell"), RatB->RestoreRuntimeMonsterState(StateB, Runtime));
	TestTrue(TEXT("Cell A is registered with the exact id"), Occupancy->GetRegistry().GetOccupantId(FIntPoint(1, 2)) == IdA);
	TestTrue(TEXT("Cell B is registered with the exact id"), Occupancy->GetRegistry().GetOccupantId(FIntPoint(3, 2)) == IdB);
	TestEqual(TEXT("Exactly two cells are occupied"), Occupancy->GetOccupiedCellCount(), 2);
	TestEqual(TEXT("No reservation survives restoration"), Occupancy->GetReservedCellCount(), 0);

	FGridRuntimeMonsterState ConflictState = StateB;
	ConflictState.CellX = StateA.CellX;
	ConflictState.CellY = StateA.CellY;
	AddExpectedError(TEXT("OccupancyConflict"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("is already occupied or reserved"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("A saved-cell conflict is detected"), RatB->RestoreRuntimeMonsterState(ConflictState, Runtime));
	TestTrue(TEXT("The first occupant is not displaced"), Occupancy->GetRegistry().GetOccupantId(FIntPoint(1, 2)) == IdA);
	TestEqual(TEXT("No reservation is leaked by the conflict"), Occupancy->GetReservedCellCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON9LevelTransitionTest, "Grimrock.Monsters.MON9.LevelTransition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9LevelTransitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON9TestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* LevelA = MakeMON9Floor(Runtime);
	UGridLevelAsset* LevelB = MakeMON9Floor(Runtime);
	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
	FGridDungeonLevelEntry EntryA;
	EntryA.LevelId = TEXT("Level_A");
	EntryA.LevelAsset = LevelA;
	FGridDungeonLevelEntry EntryB;
	EntryB.LevelId = TEXT("Level_B");
	EntryB.LevelAsset = LevelB;
	Dungeon->Levels = { EntryA, EntryB };
	Dungeon->DefaultLevelId = EntryA.LevelId;
	Runtime->DungeonAsset = Dungeon;
	Runtime->CurrentDungeonLevelId = EntryA.LevelId;
	Runtime->LevelAsset = LevelA;

	AGrimrockPartyPawn* Party = SpawnMON9Party(TestWorld.World, Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON9Definition(Runtime, TEXT("MON9_TransitionRat"));
	AGridMonsterActor* RatA = SpawnMON9Monster(TestWorld.World, Definition, FGuid(94, 1, 1, 1), EntryA.LevelId, FIntPoint(1, 1), TEXT("MON9_LevelARat"));
	AGridMonsterActor* RatB = SpawnMON9Monster(TestWorld.World, Definition, FGuid(94, 1, 1, 2), EntryB.LevelId, FIntPoint(2, 2), TEXT("MON9_LevelBRat"));
	if (!Party || !RatA || !RatB)
	{
		return false;
	}

	Runtime->ApplyCurrentLevelRuntimeState();
	RatA->DeathComponent->InitializeDeathComponent(Runtime);
	RatA->MarkDead();
	const int32 LogicalEvents = RatA->DeathComponent->LogicalDeathEventCount;
	const int32 LootAttempts = RatA->DeathComponent->PlacedLootCount + RatA->DeathComponent->FailedLootCount;

	TestTrue(TEXT("Travel from A to B succeeds"), Runtime->TravelToDungeonLevel(EntryB.LevelId, 5, 5, EGridEdge::East, Party));
	TestFalse(TEXT("The level-A rat is inactive on B"), RatA->IsRuntimeLevelActive());
	TestTrue(TEXT("The level-A rat is hidden on B"), RatA->IsHidden());
	TestFalse(
		TEXT("The level-A death cell is not occupied on B"), TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>()->IsCellOccupied(FIntPoint(1, 1)));
	TestTrue(TEXT("The level-B rat is active"), RatB->IsRuntimeLevelActive());

	TestTrue(TEXT("Travel back from B to A succeeds"), Runtime->TravelToDungeonLevel(EntryA.LevelId, 5, 5, EGridEdge::North, Party));
	TestTrue(TEXT("The level-A rat is active again"), RatA->IsRuntimeLevelActive());
	TestTrue(TEXT("The level-A rat remains dead"), RatA->IsDead());
	TestFalse(TEXT("The restored corpse is not hidden"), RatA->IsHidden());
	TestEqual(TEXT("No death event is duplicated by travel"), RatA->DeathComponent->LogicalDeathEventCount, LogicalEvents);
	TestEqual(TEXT("No loot attempt is duplicated by travel"), RatA->DeathComponent->PlacedLootCount + RatA->DeathComponent->FailedLootCount, LootAttempts);
	TestFalse(TEXT("The level-B rat is inactive after returning"), RatB->IsRuntimeLevelActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON9SaveVersionExactMatchTest, "Grimrock.Monsters.MON9.SaveVersionExactMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9SaveVersionExactMatchTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();

	TestTrue(TEXT("MON9 observes a prototype schema at least as new as v13"), UGrimrockPartySaveGame::CurrentSaveVersion >= 13);
	Save->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion;
	TestTrue(TEXT("The current save version is accepted"), Save->IsCompatible());

	Save->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion - 1;
	TestFalse(TEXT("The previous prototype save version is rejected"), Save->IsCompatible());

	Save->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion + 1;
	TestFalse(TEXT("A future save version is rejected"), Save->IsCompatible());

	UGrimrockPartySaveGame* NextSave = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("A new save defaults to the current save version"), NextSave->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON9DiskSaveRoundTripTest, "Grimrock.Monsters.MON9.DiskSaveRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9DiskSaveRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("Reason=MissingMonsterMovement; continuing death."), EAutomationExpectedErrorFlags::Contains, 1);
	const FString SlotName = FString::Printf(TEXT("MON9_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	FGridMON9TestSaveSlot TestSlot(SlotName);

	FGridMON9TestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	Runtime->LevelAsset = MakeMON9Floor(Runtime);
	SpawnMON9Party(TestWorld.World, Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON9Definition(Runtime, TEXT("MON9_DiskRat"));
	AGridMonsterActor* LivingRat =
		SpawnMON9Monster(TestWorld.World, Definition, FGuid(96, 1, 1, 1), MON9SingleLevelId, FIntPoint(1, 3), TEXT("MON9_DiskLiving"), false);
	AGridMonsterActor* DeadRat =
		SpawnMON9Monster(TestWorld.World, Definition, FGuid(96, 1, 1, 2), MON9SingleLevelId, FIntPoint(3, 3), TEXT("MON9_DiskDead"), false);
	if (!LivingRat || !DeadRat)
	{
		return false;
	}

	LivingRat->CurrentHealth = 5;
	LivingRat->Facing = EGridEdge::West;
	DeadRat->DeathComponent->InitializeDeathComponent(Runtime);
	DeadRat->MarkDead();
	TestTrue(TEXT("The dungeon state is captured before disk save"), Runtime->CaptureCurrentLevelRuntimeState());

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion;
	Save->DungeonRuntimeState = Runtime->DungeonRuntimeState;
	TestTrue(TEXT("The MON9 save is written to a real slot"), UGameplayStatics::SaveGameToSlot(Save, TestSlot.SlotName, TestSlot.UserIndex));

	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromSlot(TestSlot.SlotName, TestSlot.UserIndex));
	if (!TestNotNull(TEXT("The MON9 save loads from disk"), Loaded))
	{
		return false;
	}

	LivingRat->CurrentHealth = 1;
	LivingRat->Facing = EGridEdge::South;
	DeadRat->DeathComponent->RestoreLivingState();
	DeadRat->CurrentHealth = Definition->MaxHealth;
	DeadRat->MonsterState = EGridMonsterState::Idle;

	Runtime->DungeonRuntimeState = Loaded->DungeonRuntimeState;
	TestTrue(TEXT("The loaded dungeon state applies"), Runtime->ApplyCurrentLevelRuntimeState());
	TestEqual(TEXT("The disk-loaded living health restores"), LivingRat->CurrentHealth, 5);
	TestEqual(TEXT("The disk-loaded facing restores"), LivingRat->Facing, EGridEdge::West);
	TestTrue(TEXT("The disk-loaded dead rat remains dead"), DeadRat->IsDead());
	TestEqual(TEXT("Disk death restore emits no logical death"), DeadRat->DeathComponent->LogicalDeathEventCount, 0);
	TestEqual(TEXT("Disk death restore executes no link"), DeadRat->DeathComponent->LinkExecutionAttemptCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON9StableIdentityTest, "Grimrock.Monsters.MON9.StableIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON9StableIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON9TestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	Runtime->LevelAsset = MakeMON9Floor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON9Definition(Runtime, TEXT("MON9_IdentityRat"));
	const FGuid StableId(97, 1, 1, 1);
	const FGuid OtherId(97, 1, 1, 2);
	AGridMonsterActor* First =
		SpawnMON9Monster(TestWorld.World, Definition, StableId, MON9SingleLevelId, FIntPoint(1, 1), TEXT("MON9_IdentityOriginal"), false);
	AGridMonsterActor* Other = SpawnMON9Monster(TestWorld.World, Definition, OtherId, MON9SingleLevelId, FIntPoint(3, 3), TEXT("MON9_IdentityOther"), false);
	if (!First || !Other)
	{
		return false;
	}

	TestTrue(TEXT("The persistent id is stable"), First->ResolvePersistenceId() == StableId);
	TestTrue(TEXT("Two monsters have different ids"), First->ResolvePersistenceId() != Other->ResolvePersistenceId());

	AGridMonsterActor* Duplicate =
		SpawnMON9Monster(TestWorld.World, Definition, StableId, MON9SingleLevelId, FIntPoint(4, 4), TEXT("MON9_IdentityDuplicate"), false);
	FString ValidationError;
	TestFalse(TEXT("A duplicate id is detected"), First->ValidatePersistenceSetup(ValidationError));
	TestTrue(TEXT("Duplicate validation explains the id"), ValidationError.Contains(TEXT("duplicated")));
	TestWorld.World->DestroyActor(Duplicate);

	First->CurrentHealth = 4;
	First->Facing = EGridEdge::East;
	FGridRuntimeMonsterState SavedState;
	TestTrue(TEXT("The original identity captures state"), First->CaptureRuntimeMonsterState(SavedState, MON9SingleLevelId));
	TestWorld.World->DestroyActor(First);

	AGridMonsterActor* Recreated =
		SpawnMON9Monster(TestWorld.World, Definition, StableId, MON9SingleLevelId, FIntPoint(5, 4), TEXT("MON9_CompletelyDifferentName"), false);
	TestTrue(TEXT("The recreated actor finds state by id"), Recreated->RestoreRuntimeMonsterState(SavedState, Runtime));
	TestTrue(TEXT("Restoration does not use the current cell"), Recreated->CurrentCell == FIntPoint(1, 1));
	TestEqual(TEXT("Restoration does not use the actor name"), Recreated->CurrentHealth, 4);
	TestTrue(TEXT("The recreated actor keeps the same id"), Recreated->ResolvePersistenceId() == StableId);
	return true;
}

#endif
