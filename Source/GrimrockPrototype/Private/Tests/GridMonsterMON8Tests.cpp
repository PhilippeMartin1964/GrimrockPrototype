#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterLootResolver.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
	struct FGridMON8TestWorld
	{
		UWorld* World = nullptr;

		FGridMON8TestWorld()
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
				FName(*FString::Printf(TEXT("MON8TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FGridMON8TestWorld()
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

	UGridLevelAsset* ConfigureMON8Floor(AGridLevelRuntimeActor* RuntimeActor, int32 Width = 5, int32 Height = 5)
	{
		if (!RuntimeActor)
		{
			return nullptr;
		}

		UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(RuntimeActor);
		LevelAsset->Width = Width;
		LevelAsset->Height = Height;
		LevelAsset->EnsureCellCount();
		for (FGridLevelCellData& Cell : LevelAsset->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		RuntimeActor->LevelAsset = LevelAsset;
		return LevelAsset;
	}

	UGridMonsterDefinitionAsset* MakeMON8MonsterDefinition(UObject* Outer, FName MonsterId)
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Outer);
		Definition->MonsterId = MonsterId;
		Definition->DisplayName = FText::FromString(TEXT("MON8 Test Rat"));
		Definition->CategoryId = TEXT("Vermin");
		Definition->MaxHealth = 8;
		Definition->ActionPointsPerTurn = 2;
		Definition->DeathExpectedDuration = 1.0f;
		return Definition;
	}

	UGridItemDefinitionAsset* MakeMON8ItemDefinition(UObject* Outer, FName ItemDefinitionId)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = ItemDefinitionId;
		Definition->DisplayName = FText::FromName(ItemDefinitionId);
		Definition->Weight = 1.0f;
		return Definition;
	}

	FGridMonsterLootEntry MakeMON8LootEntry(FName ItemDefinitionId, float DropChance, int32 MinQuantity = 1, int32 MaxQuantity = 1)
	{
		FGridMonsterLootEntry Entry;
		Entry.ItemDefinitionId = ItemDefinitionId;
		Entry.DropChance = DropChance;
		Entry.MinQuantity = MinQuantity;
		Entry.MaxQuantity = MaxQuantity;
		return Entry;
	}

	const FGridMonsterLootRollResult* FindMON8LootResult(const TArray<FGridMonsterLootRollResult>& Results, FName ItemDefinitionId)
	{
		return Results.FindByPredicate(
			[ItemDefinitionId](const FGridMonsterLootRollResult& Result)
			{
				return Result.ItemDefinitionId == ItemDefinitionId;
			});
	}

	AGridMonsterActor* SpawnMON8Monster(UWorld* World, UGridMonsterDefinitionAsset* Definition, FGuid SpawnId, FIntPoint Cell, bool bAddMovementAndBehavior)
	{
		if (!World || !Definition)
		{
			return nullptr;
		}

		AGridMonsterActor* Monster = World->SpawnActor<AGridMonsterActor>();
		if (!Monster || !Monster->InitializeMonster(Definition, SpawnId, Cell, EGridEdge::North))
		{
			return nullptr;
		}

		if (bAddMovementAndBehavior)
		{
			UGridMonsterMovementComponent* Movement = NewObject<UGridMonsterMovementComponent>(Monster, TEXT("MON8TestMovement"));
			Movement->bAutoInitialize = false;
			Movement->bInferCellFromActorLocation = false;
			Monster->AddInstanceComponent(Movement);
			Movement->RegisterComponent();

			UGridMonsterBehaviorComponent* Behavior = NewObject<UGridMonsterBehaviorComponent>(Monster, TEXT("MON8TestBehavior"));
			Behavior->bAutoInitialize = false;
			Monster->AddInstanceComponent(Behavior);
			Behavior->RegisterComponent();
		}
		return Monster;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON8LootResolverTest, "Grimrock.Monsters.MON8.LootResolver", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8LootResolverTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TArray<FGridMonsterLootEntry> LootTable;
	TestEqual(TEXT("An empty table produces no evaluations"), FGridMonsterLootResolver::ResolveLoot(LootTable, 123).Num(), 0);

	FGridMonsterLootEntry NeverEntry = MakeMON8LootEntry(TEXT("Item_Never"), 0.0f);
	FGridMonsterLootRollResult Result = FGridMonsterLootResolver::ResolveEntryFromRolls(NeverEntry, 0, -1.0f, 0);
	TestFalse(TEXT("A zero chance never drops"), Result.bDropped);
	TestEqual(TEXT("A failed entry has no quantity"), Result.Quantity, 0);

	FGridMonsterLootEntry AlwaysEntry = MakeMON8LootEntry(TEXT("Item_Always"), 1.0f, 2, 4);
	Result = FGridMonsterLootResolver::ResolveEntryFromRolls(AlwaysEntry, 0, 2.0f, 7);
	TestTrue(TEXT("A full chance always drops"), Result.bDropped);
	TestTrue(TEXT("The resolved quantity remains inside its range"), Result.Quantity >= 2 && Result.Quantity <= 4);

	const FGridMonsterLootEntry Key = MakeMON8LootEntry(TEXT("Key_Iron"), 1.0f);
	const FGridMonsterLootEntry Tooth = MakeMON8LootEntry(TEXT("Item_RatTooth"), 0.4f);
	const FGridMonsterLootEntry Meat = MakeMON8LootEntry(TEXT("Item_RatMeat"), 0.8f, 1, 2);
	LootTable = { Key, Tooth, Meat };

	TArray<FGridMonsterLootRollResult> ForcedAllDrops;
	ForcedAllDrops.Add(FGridMonsterLootResolver::ResolveEntryFromRolls(Key, 0, 0.99f, 0));
	ForcedAllDrops.Add(FGridMonsterLootResolver::ResolveEntryFromRolls(Tooth, 1, 0.20f, 0));
	ForcedAllDrops.Add(FGridMonsterLootResolver::ResolveEntryFromRolls(Meat, 2, 0.70f, 1));
	TestTrue(TEXT("The key succeeds independently"), ForcedAllDrops[0].bDropped);
	TestTrue(TEXT("The tooth succeeds independently"), ForcedAllDrops[1].bDropped);
	TestTrue(TEXT("The meat succeeds independently"), ForcedAllDrops[2].bDropped);

	TArray<FGridMonsterLootRollResult> ForcedMixedDrops;
	ForcedMixedDrops.Add(FGridMonsterLootResolver::ResolveEntryFromRolls(Key, 0, 0.99f, 0));
	ForcedMixedDrops.Add(FGridMonsterLootResolver::ResolveEntryFromRolls(Tooth, 1, 0.60f, 0));
	ForcedMixedDrops.Add(FGridMonsterLootResolver::ResolveEntryFromRolls(Meat, 2, 0.70f, 1));
	TestTrue(TEXT("The guaranteed key still succeeds"), ForcedMixedDrops[0].bDropped);
	TestFalse(TEXT("The tooth can fail independently"), ForcedMixedDrops[1].bDropped);
	TestTrue(TEXT("A failed tooth does not prevent the meat"), ForcedMixedDrops[2].bDropped);

	UGridMonsterDefinitionAsset* ValidatedDefinition = MakeMON8MonsterDefinition(GetTransientPackage(), TEXT("MON8_IndependentLootValidation"));
	ValidatedDefinition->LootTable = LootTable;
	FString ValidationError;
	TestTrue(TEXT("A 2.20 total chance is valid"), ValidatedDefinition->ValidateDefinition(ValidationError));
	TestTrue(TEXT("Independent chances produce no validation error"), ValidationError.IsEmpty());

	const int32 BaseSeed = 8675309;
	const TArray<FGridMonsterLootRollResult> First = FGridMonsterLootResolver::ResolveLoot(LootTable, BaseSeed);
	const TArray<FGridMonsterLootRollResult> Second = FGridMonsterLootResolver::ResolveLoot(LootTable, BaseSeed);
	TestEqual(TEXT("Every valid entry produces an evaluation"), First.Num(), 3);
	TestEqual(TEXT("An identical seed produces the same result count"), First.Num(), Second.Num());
	for (int32 Index = 0; Index < First.Num(); ++Index)
	{
		TestEqual(TEXT("An identical seed preserves each item id"), First[Index].ItemDefinitionId, Second[Index].ItemDefinitionId);
		TestEqual(TEXT("An identical seed preserves each drop roll"), First[Index].DropRoll, Second[Index].DropRoll);
		TestEqual(TEXT("An identical seed preserves each drop outcome"), First[Index].bDropped, Second[Index].bDropped);
		TestEqual(TEXT("An identical seed preserves each quantity"), First[Index].Quantity, Second[Index].Quantity);
	}

	FGridMonsterLootEntry InvalidEntry;
	InvalidEntry.ItemDefinitionId = NAME_None;
	InvalidEntry.DropChance = 1.0f;
	TArray<FGridMonsterLootEntry> WithInvalidEntry = LootTable;
	WithInvalidEntry.Insert(InvalidEntry, 1);
	const TArray<FGridMonsterLootRollResult> WithInvalidResults = FGridMonsterLootResolver::ResolveLoot(WithInvalidEntry, BaseSeed);
	TestEqual(TEXT("An invalid entry is omitted"), WithInvalidResults.Num(), First.Num());
	for (const FGridMonsterLootRollResult& Original : First)
	{
		const FGridMonsterLootRollResult* WithInvalid = FindMON8LootResult(WithInvalidResults, Original.ItemDefinitionId);
		TestNotNull(TEXT("Each original result survives an invalid entry"), WithInvalid);
		if (WithInvalid)
		{
			TestEqual(TEXT("An invalid entry does not change another roll"), WithInvalid->DropRoll, Original.DropRoll);
			TestEqual(TEXT("An invalid entry does not change another quantity"), WithInvalid->Quantity, Original.Quantity);
		}
	}

	const TArray<FGridMonsterLootEntry> ReorderedTable = { Meat, Key, Tooth };
	const TArray<FGridMonsterLootRollResult> ReorderedResults = FGridMonsterLootResolver::ResolveLoot(ReorderedTable, BaseSeed);
	for (const FGridMonsterLootRollResult& Original : First)
	{
		const FGridMonsterLootRollResult* Reordered = FindMON8LootResult(ReorderedResults, Original.ItemDefinitionId);
		TestNotNull(TEXT("Each item exists after table reordering"), Reordered);
		if (Reordered)
		{
			TestEqual(TEXT("Reordering preserves the item drop roll"), Reordered->DropRoll, Original.DropRoll);
			TestEqual(TEXT("Reordering preserves the item quantity"), Reordered->Quantity, Original.Quantity);
		}
	}

	TArray<FGridMonsterLootEntry> ExtendedTable = LootTable;
	ExtendedTable.Add(MakeMON8LootEntry(TEXT("Item_RatTail"), 0.5f));
	const TArray<FGridMonsterLootRollResult> ExtendedResults = FGridMonsterLootResolver::ResolveLoot(ExtendedTable, BaseSeed);
	for (const FGridMonsterLootRollResult& Original : First)
	{
		const FGridMonsterLootRollResult* Extended = FindMON8LootResult(ExtendedResults, Original.ItemDefinitionId);
		TestNotNull(TEXT("Each original item survives table extension"), Extended);
		if (Extended)
		{
			TestEqual(TEXT("Adding an entry preserves an existing roll"), Extended->DropRoll, Original.DropRoll);
			TestEqual(TEXT("Adding an entry preserves an existing quantity"), Extended->Quantity, Original.Quantity);
		}
	}

	const TArray<FGridMonsterLootRollResult> DifferentSeedResults = FGridMonsterLootResolver::ResolveLoot(LootTable, BaseSeed + 1);
	bool bFoundDifferentRoll = false;
	for (const FGridMonsterLootRollResult& Original : First)
	{
		const FGridMonsterLootRollResult* Different = FindMON8LootResult(DifferentSeedResults, Original.ItemDefinitionId);
		bFoundDifferentRoll |= Different && Different->DropRoll != Original.DropRoll;
	}
	TestTrue(TEXT("Different base seeds can produce different rolls"), bFoundDifferentRoll);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON8MultipleIndependentDropsTest, "Grimrock.Monsters.MON8.MultipleIndependentDrops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8MultipleIndependentDropsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("Reason=MissingMonsterMovement; continuing death."), EAutomationExpectedErrorFlags::Contains, 1);
	FGridMON8TestWorld TestWorld;
	if (!TestNotNull(TEXT("The transient multiple-loot world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("The loot runtime exists"), Runtime) || !TestNotNull(TEXT("The loot floor exists"), ConfigureMON8Floor(Runtime)))
	{
		return false;
	}

	UGridMonsterDefinitionAsset* Definition = MakeMON8MonsterDefinition(Runtime, TEXT("MON8_MultipleLootRat"));
	const FName ItemIds[] = { TEXT("Key_Iron"), TEXT("Item_RatTooth"), TEXT("Item_RatMeat") };
	for (const FName ItemId : ItemIds)
	{
		UGridItemDefinitionAsset* ItemDefinition = MakeMON8ItemDefinition(Definition, ItemId);
		FGridMonsterLootEntry Entry = MakeMON8LootEntry(ItemId, 1.0f);
		Entry.ItemDefinitionAsset = ItemDefinition;
		Definition->LootTable.Add(Entry);
	}

	AGridMonsterActor* Monster = SpawnMON8Monster(TestWorld.World, Definition, FGuid(8, 8, 8, 8), FIntPoint(2, 2), false);
	if (!TestNotNull(TEXT("The multiple-loot monster exists"), Monster) || !Monster->DeathComponent)
	{
		return false;
	}
	Monster->DeathComponent->InitializeDeathComponent(Runtime);
	Monster->MarkDead();

	TestEqual(TEXT("All guaranteed entries are placed"), Monster->DeathComponent->PlacedLootCount, 3);
	TestEqual(TEXT("No guaranteed placement fails"), Monster->DeathComponent->FailedLootCount, 0);
	TestEqual(TEXT("GeneratedLoot contains every placed item"), Monster->DeathComponent->GeneratedLoot.Num(), 3);

	TSet<FGuid> RuntimeObjectIds;
	for (const FGridItemInstance& Item : Monster->DeathComponent->GeneratedLoot)
	{
		TestTrue(TEXT("Each placed item has a runtime id"), Item.RuntimeObjectId.IsValid());
		RuntimeObjectIds.Add(Item.RuntimeObjectId);
	}
	TestEqual(TEXT("Every placed item has a distinct runtime id"), RuntimeObjectIds.Num(), 3);

	TestTrue(TEXT("The three world items are captured for persistence"), Runtime->CaptureCurrentLevelRuntimeState());
	const FGridLevelRuntimeState* RuntimeState = Runtime->FindRuntimeStateForCurrentLevel();
	TestNotNull(TEXT("The captured runtime state exists"), RuntimeState);
	TestEqual(TEXT("All placed items are captured independently"), RuntimeState ? RuntimeState->Items.Num() : 0, 3);

	const int32 PlacedBeforeSecondDeath = Monster->DeathComponent->PlacedLootCount;
	const int32 GeneratedBeforeSecondDeath = Monster->DeathComponent->GeneratedLoot.Num();
	Monster->MarkDead();
	TestEqual(TEXT("A second MarkDead places no additional item"), Monster->DeathComponent->PlacedLootCount, PlacedBeforeSecondDeath);
	TestEqual(TEXT("A second MarkDead generates no additional item"), Monster->DeathComponent->GeneratedLoot.Num(), GeneratedBeforeSecondDeath);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON8DeathExactlyOnceTest, "Grimrock.Monsters.MON8.DeathExactlyOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8DeathExactlyOnceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("Reason=MissingMonsterMovement; continuing death."), EAutomationExpectedErrorFlags::Contains, 1);
	FGridMON8TestWorld TestWorld;
	TestNotNull(TEXT("The transient test world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	UGridMonsterDefinitionAsset* Definition = MakeMON8MonsterDefinition(TestWorld.World, TEXT("MON8_DeathRat"));
	AGridMonsterActor* Monster = SpawnMON8Monster(TestWorld.World, Definition, FGuid(1, 2, 3, 4), FIntPoint(2, 2), false);
	TestNotNull(TEXT("The death-test monster exists"), Monster);
	if (!Monster || !Monster->DeathComponent)
	{
		return false;
	}

	Monster->MarkDead();
	TestEqual(TEXT("CurrentHealth becomes zero"), Monster->CurrentHealth, 0);
	TestEqual(TEXT("MonsterState becomes Dead"), Monster->MonsterState, EGridMonsterState::Dead);
	TestTrue(TEXT("The logical death is committed"), Monster->DeathComponent->bDeathCommitted);
	TestTrue(TEXT("The loot attempt is guarded even for an empty table"), Monster->DeathComponent->bLootGenerated);

	const int32 LogicalEvents = Monster->DeathComponent->LogicalDeathEventCount;
	const int32 PlacedLoot = Monster->DeathComponent->PlacedLootCount;
	const int32 FailedLoot = Monster->DeathComponent->FailedLootCount;
	Monster->MarkDead();
	TestEqual(TEXT("A second MarkDead does not broadcast another logical event"), Monster->DeathComponent->LogicalDeathEventCount, LogicalEvents);
	TestEqual(TEXT("A second MarkDead does not place more loot"), Monster->DeathComponent->PlacedLootCount, PlacedLoot);
	TestEqual(TEXT("A second MarkDead does not retry failed loot"), Monster->DeathComponent->FailedLootCount, FailedLoot);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON8OccupancyReleaseTest, "Grimrock.Monsters.MON8.OccupancyRelease", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8OccupancyReleaseTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON8TestWorld TestWorld;
	if (!TestNotNull(TEXT("The transient occupancy world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON8Floor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON8MonsterDefinition(Runtime, TEXT("MON8_OccupancyRat"));
	AGridMonsterActor* Monster = SpawnMON8Monster(TestWorld.World, Definition, FGuid(10, 0, 0, 1), FIntPoint(2, 2), true);
	UGridMonsterMovementComponent* Movement = Monster ? Monster->FindComponentByClass<UGridMonsterMovementComponent>() : nullptr;
	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	TestNotNull(TEXT("MonsterMovement exists"), Movement);
	TestNotNull(TEXT("The MON3 occupancy subsystem exists"), Occupancy);
	if (!Monster || !Movement || !Occupancy)
	{
		return false;
	}

	TestTrue(TEXT("The monster registers its occupied cell"), Movement->InitializeMovement(Runtime));
	TestTrue(TEXT("A pending reservation can be created"), Occupancy->TryReserveCell(Monster, FIntPoint(3, 2)));
	Monster->MarkDead();
	TestFalse(TEXT("Death immediately releases occupation"), Occupancy->IsCellOccupied(FIntPoint(2, 2)));
	TestFalse(TEXT("Death immediately releases reservation"), Occupancy->IsCellReserved(FIntPoint(3, 2)));

	AGridMonsterActor* Other = SpawnMON8Monster(TestWorld.World, Definition, FGuid(10, 0, 0, 2), FIntPoint(1, 2), false);
	TestTrue(TEXT("Another monster can register beside the released cell"), Occupancy->RegisterMonster(Other, FIntPoint(1, 2)));
	TestTrue(TEXT("Another monster can reserve the released death cell"), Occupancy->TryReserveCell(Other, FIntPoint(2, 2)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON8VictoryOnLastDeathTest, "Grimrock.Monsters.MON8.VictoryOnLastDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8VictoryOnLastDeathTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON8TestWorld TestWorld;
	if (!TestNotNull(TEXT("The transient combat world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON8Floor(Runtime);
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	Party->LevelRuntimeActor = Runtime;
	Party->CurrentCellX = 4;
	Party->CurrentCellY = 4;
	FGridCharacterInventoryState LivingCharacter;
	LivingCharacter.CharacterId = FGuid(25, 1, 0, 1);
	LivingCharacter.Resources.CurrentHealth = 10;
	LivingCharacter.DerivedStats.Initiative = 100;
	Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { LivingCharacter };

	UGridMonsterDefinitionAsset* Definition = MakeMON8MonsterDefinition(Runtime, TEXT("MON8_VictoryRat"));
	AGridMonsterActor* First = SpawnMON8Monster(TestWorld.World, Definition, FGuid(20, 0, 0, 1), FIntPoint(1, 1), true);
	AGridMonsterActor* Last = SpawnMON8Monster(TestWorld.World, Definition, FGuid(20, 0, 0, 2), FIntPoint(2, 1), true);

	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("MON8TestTurnManager"));
	TurnManager->bAutoInitialize = false;
	Runtime->AddInstanceComponent(TurnManager);
	TurnManager->RegisterComponent();

	TestTrue(TEXT("The TurnManager initializes"), TurnManager->InitializeTurnManager(Runtime, Party));
	TestTrue(TEXT("The encounter starts with both rats"), TurnManager->StartCombatWithAllMonsters());
	First->MarkDead();
	TestTrue(TEXT("The encounter remains active after the first death"), TurnManager->bCombatActive);
	Last->MarkDead();
	TestFalse(TEXT("The encounter ends immediately after the last death"), TurnManager->bCombatActive);
	TestEqual(TEXT("The terminal phase is Victory"), TurnManager->CurrentPhase, EGridCombatPhase::Victory);
	TestTrue(TEXT("No pending enemy action remains"), TurnManager->PendingActions.IsEmpty());
	TestFalse(TEXT("Party input is re-enabled"), TurnManager->IsPartyInputLocked());

	Last->MarkDead();
	TestEqual(TEXT("A duplicate death leaves Victory unchanged"), TurnManager->CurrentPhase, EGridCombatPhase::Victory);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON8VictoryAfterEnemyPhaseStartedTest, "Grimrock.Monsters.MON8.VictoryAfterEnemyPhaseStarted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8VictoryAfterEnemyPhaseStartedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON8TestWorld TestWorld;
	if (!TestNotNull(TEXT("The transient enemy-phase world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	TestNotNull(TEXT("The runtime actor exists"), Runtime);
	TestNotNull(TEXT("The party pawn exists"), Party);
	if (!Runtime || !ConfigureMON8Floor(Runtime) || !Party || !Party->PartyInventoryComponent)
	{
		return false;
	}

	Party->LevelRuntimeActor = Runtime;
	Party->CurrentCellX = 4;
	Party->CurrentCellY = 4;
	Party->Facing = EGridEdge::North;
	Party->MoveDuration = 0.0f;
	Party->TurnDuration = 0.0f;
	Party->InputBufferMaxAge = 0.0f;
	Party->SetActorLocation(Runtime->GetCellCenterWorld(Party->CurrentCellX, Party->CurrentCellY, Party->EyeHeight));
	Party->SetActorRotation(FRotator(0.0f, GridDirectionUtils::ToYaw(Party->Facing), 0.0f));

	FGridCharacterInventoryState LivingCharacter;
	LivingCharacter.CharacterId = FGuid(25, 1, 0, 2);
	LivingCharacter.Resources.CurrentHealth = 10;
	LivingCharacter.DerivedStats.Initiative = 100;
	Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { LivingCharacter };

	UGridMonsterDefinitionAsset* Definition = MakeMON8MonsterDefinition(Runtime, TEXT("MON8_EnemyPhaseVictoryRat"));
	Definition->HearingRangeCells = 20;
	AGridMonsterActor* First = SpawnMON8Monster(TestWorld.World, Definition, FGuid(25, 0, 0, 1), FIntPoint(1, 1), true);
	AGridMonsterActor* Second = SpawnMON8Monster(TestWorld.World, Definition, FGuid(25, 0, 0, 2), FIntPoint(2, 1), true);
	TestNotNull(TEXT("The first enemy-phase rat exists"), First);
	TestNotNull(TEXT("The second enemy-phase rat exists"), Second);
	if (!First || !Second)
	{
		return false;
	}

	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("MON8EnemyPhaseTurnManager"));
	TurnManager->bAutoInitialize = false;
	TurnManager->CombatStartSafetyPadding = 0.0f;
	Runtime->AddInstanceComponent(TurnManager);
	TurnManager->RegisterComponent();

	if (!TestTrue(TEXT("The enemy-phase TurnManager initializes"), TurnManager->InitializeTurnManager(Runtime, Party)) ||
		!TestTrue(TEXT("The enemy-phase encounter starts"), TurnManager->StartCombatWithAllMonsters()) ||
		!TestTrue(TEXT("Ending the active player turn starts the enemy"), TurnManager->EndActivePlayerTurn()))
	{
		return false;
	}

	TestEqual(TEXT("The encounter is in EnemyPhase"), TurnManager->CurrentPhase, EGridCombatPhase::EnemyPhase);
	AGridMonsterActor* ActiveMonster = TurnManager->CurrentMonster;
	TestNotNull(TEXT("A monster turn is active after BeginNextMonsterTurn"), ActiveMonster);
	if (!ActiveMonster)
	{
		return false;
	}

	AGridMonsterActor* RemainingMonster = ActiveMonster == First ? Second : First;
	ActiveMonster->MarkDead();
	TestTrue(TEXT("Combat remains active after the active monster dies"), TurnManager->bCombatActive);
	TestEqual(TEXT("The next living monster immediately receives its turn"), TurnManager->CurrentMonster.Get(), RemainingMonster);

	RemainingMonster->MarkDead();
	TestFalse(TEXT("The last EnemyPhase death immediately ends combat"), TurnManager->bCombatActive);
	TestEqual(TEXT("The terminal phase is Victory"), TurnManager->CurrentPhase, EGridCombatPhase::Victory);
	TestTrue(TEXT("No pending action remains after Victory"), TurnManager->PendingActions.IsEmpty());
	TestFalse(TEXT("No active action remains after Victory"), TurnManager->bHasActiveAction);
	TestEqual(TEXT("ActiveAction is reset after Victory"), TurnManager->ActiveAction.Type, EGridCombatActionType::None);
	TestFalse(TEXT("Party input is unlocked after Victory"), TurnManager->IsPartyInputLocked());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON8MonsterDiedEventTest, "Grimrock.Monsters.MON8.MonsterDiedEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8MonsterDiedEventTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("[GridMonsterSpawn] PresentationWarning"), EAutomationExpectedErrorFlags::Contains, 2);
	AddExpectedError(TEXT("Reason=MissingMonsterMovement; continuing death."), EAutomationExpectedErrorFlags::Contains, 2);
	AddExpectedError(TEXT("Grid link failed:"), EAutomationExpectedErrorFlags::Contains, 1);
	TestEqual(TEXT("Activated keeps its historic numeric value"), static_cast<uint8>(EGridObjectEvent::Activated), static_cast<uint8>(0));
	TestEqual(TEXT("Disabled keeps its historic numeric value"), static_cast<uint8>(EGridObjectEvent::Disabled), static_cast<uint8>(11));
	TestEqual(TEXT("MonsterDied is appended after all historic events"), static_cast<uint8>(EGridObjectEvent::MonsterDied), static_cast<uint8>(12));

	FGridMON8TestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* LevelAsset = ConfigureMON8Floor(Runtime);
	const FGuid SourceId(30, 0, 0, 1);
	const FGuid TargetId(30, 0, 0, 2);
	const FGuid UnlinkedSourceId(30, 0, 0, 3);
	UGridMonsterDefinitionAsset* Definition = MakeMON8MonsterDefinition(Runtime, TEXT("MON8_EventRat"));

	FGridLevelObjectData Source;
	Source.ObjectId = SourceId;
	Source.Type = EGridLevelObjectType::MonsterSpawn;
	Source.CellX = 1;
	Source.CellY = 1;
	Source.Edge = EGridEdge::None;
	Source.InitialFacing = EGridEdge::North;
	Source.MonsterDefinitionAsset = Definition;
	Source.MonsterDefinitionId = Definition->MonsterId;
	LevelAsset->Objects.Add(Source);

	FGridLevelObjectData UnlinkedSource = Source;
	UnlinkedSource.ObjectId = UnlinkedSourceId;
	UnlinkedSource.CellX = 2;
	UnlinkedSource.CellY = 2;
	LevelAsset->Objects.Add(UnlinkedSource);

	FGridLevelObjectData Target;
	Target.ObjectId = TargetId;
	Target.Type = EGridLevelObjectType::Trigger;
	Target.CellX = 3;
	Target.CellY = 3;
	LevelAsset->Objects.Add(Target);

	FGridObjectLink Link;
	Link.SourceObjectId = SourceId;
	Link.TargetObjectId = TargetId;
	Link.SourceEvent = EGridObjectEvent::MonsterDied;
	Link.Command = EGridObjectCommand::Activate;
	LevelAsset->Links.Add(Link);
	Runtime->RebuildLevel();

	AGridMonsterActor* Monster = Runtime->FindSpawnedMonsterActor(SourceId);
	TestNotNull(TEXT("The linked MonsterSpawn creates its runtime monster"), Monster);
	if (!Monster)
	{
		return false;
	}
	Monster->MarkDead();
	TestEqual(TEXT("The MonsterDied link is attempted once"), Monster->DeathComponent->LinkExecutionAttemptCount, 1);
	Monster->MarkDead();
	TestEqual(TEXT("A second MarkDead does not re-execute the link"), Monster->DeathComponent->LinkExecutionAttemptCount, 1);

	AGridMonsterActor* UnlinkedMonster = Runtime->FindSpawnedMonsterActor(UnlinkedSourceId);
	TestNotNull(TEXT("The unlinked MonsterSpawn creates its runtime monster"), UnlinkedMonster);
	if (!UnlinkedMonster)
	{
		return false;
	}
	UnlinkedMonster->MarkDead();
	TestTrue(TEXT("A missing LevelAsset link does not prevent death"), UnlinkedMonster->DeathComponent->bDeathCommitted);
	return true;
}

#endif
