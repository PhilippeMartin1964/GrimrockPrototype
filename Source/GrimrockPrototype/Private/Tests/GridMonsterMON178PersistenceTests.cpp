#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace GridMonsterMON178Persistence
{
	const FName TestLevelId(TEXT("MON178_Persistence"));

	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
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
				FName(*FString::Printf(TEXT("MON178Persistence_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTestWorld()
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

	UGridLevelAsset* MakeFloor(AGridLevelRuntimeActor* RuntimeActor)
	{
		if (!RuntimeActor)
		{
			return nullptr;
		}

		UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(RuntimeActor);
		LevelAsset->Width = 5;
		LevelAsset->Height = 5;
		LevelAsset->EnsureCellCount();
		for (FGridLevelCellData& Cell : LevelAsset->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		RuntimeActor->LevelAsset = LevelAsset;
		return LevelAsset;
	}

	UGridMonsterDefinitionAsset* MakeDefinition(UObject* Outer)
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Outer);
		Definition->MonsterId = TEXT("MON178_PersistenceMonster");
		Definition->DisplayName = FText::FromString(TEXT("MON17.8 Persistence Monster"));
		Definition->CategoryId = TEXT("Test");
		Definition->MaxHealth = 10;
		Definition->ActionPointsPerTurn = 2;
		Definition->DeathExpectedDuration = 1.0f;
		return Definition;
	}

	AGridMonsterActor* SpawnMonster(UWorld* World, UGridMonsterDefinitionAsset* Definition, const FGuid& PersistenceId, FIntPoint Cell, FName ActorName)
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
		Monster->HomeDungeonLevelId = TestLevelId;
		Monster->CurrentCell = Cell;
		Monster->Facing = EGridEdge::North;
		Monster->CurrentHealth = Definition->MaxHealth;
		Monster->CurrentPhysicalArmor = 0;
		Monster->CurrentMagicalArmor = 0;
		Monster->MonsterState = EGridMonsterState::Idle;
		Monster->bCombatStatsInitialized = true;
		Monster->bMonsterEnabled = true;
		Monster->SkeletalMeshComponent->SetVisibility(true, true);
		return Monster;
	}

	FGridRuntimeMonsterState MakeState(const AGridMonsterActor* Monster, FIntPoint Cell, bool bDead)
	{
		FGridRuntimeMonsterState State;
		State.PersistenceId = Monster ? Monster->ResolvePersistenceId() : FGuid();
		State.MonsterDefinitionId = Monster && Monster->MonsterDefinition ? Monster->MonsterDefinition->MonsterId : NAME_None;
		State.DungeonLevelId = TestLevelId;
		State.CellX = Cell.X;
		State.CellY = Cell.Y;
		State.Facing = EGridEdge::North;
		State.MonsterState = bDead ? EGridMonsterState::Dead : EGridMonsterState::Idle;
		State.CurrentHealth = bDead ? 0 : 7;
		State.CurrentPhysicalArmor = 0;
		State.CurrentMagicalArmor = 0;
		State.bMonsterEnabled = true;
		State.bIsDead = bDead;
		return State;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON178DeadRestorePresentationContractTest, "Grimrock.Monsters.MON17.8.DeadRestorePresentationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON178DeadRestorePresentationContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMonsterMON178Persistence;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Persistence test world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("Runtime actor exists"), Runtime);
	if (!Runtime || !MakeFloor(Runtime))
	{
		return false;
	}

	UGridMonsterDefinitionAsset* Definition = MakeDefinition(Runtime);
	const FGuid PersistenceId(1786, 1, 1, 1);
	const FIntPoint DeathCell(2, 2);
	AGridMonsterActor* Monster = SpawnMonster(TestWorld.World, Definition, PersistenceId, DeathCell, TEXT("MON178_DeadRestore"));
	if (!TestNotNull(TEXT("Monster exists"), Monster))
	{
		return false;
	}

	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	TestNotNull(TEXT("Occupancy subsystem exists"), Occupancy);
	TestTrue(TEXT("Test setup initially occupies the death cell"), Occupancy && Occupancy->RegisterMonster(Monster, DeathCell));

	const FGridRuntimeMonsterState DeadState = MakeState(Monster, DeathCell, true);

	TestTrue(TEXT("Dead runtime state restores"), Monster->RestoreRuntimeMonsterState(DeadState, Runtime));
	TestTrue(TEXT("Monster remains logically dead"), Monster->IsDead());
	TestEqual(TEXT("Restored dead health is zero"), Monster->CurrentHealth, 0);
	TestTrue(TEXT("Death commit remains restored"), Monster->DeathComponent->bDeathCommitted);
	TestTrue(TEXT("Loot generation remains committed"), Monster->DeathComponent->bLootGenerated);
	TestFalse(TEXT("Restored dead mesh is hidden immediately"), Monster->SkeletalMeshComponent->IsVisible());
	TestFalse(TEXT("Actor is retained instead of hidden/destroyed"), Monster->IsHidden());
	TestEqual(TEXT("Restored dead collision stays disabled"), Monster->CollisionComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Restored dead occupies no cell"), Occupancy && Occupancy->IsCellOccupied(DeathCell));
	TestFalse(TEXT("Death montage presentation is not active after restore"), Monster->DeathComponent->bDeathPresentationActive);
	TestFalse(TEXT("Death dissolve is not replayed after restore"), Monster->DeathComponent->bDeathDissolveActive);
	TestEqual(TEXT("Transient dissolve alpha is reset"), Monster->DeathComponent->DeathDissolveAlpha, 0.0f);
	TestEqual(TEXT("Restore emits no logical death event"), Monster->DeathComponent->LogicalDeathEventCount, 0);
	TestEqual(TEXT("Restore executes no MonsterDied links"), Monster->DeathComponent->LinkExecutionAttemptCount, 0);
	TestEqual(TEXT("Restore generates no transient loot metadata"), Monster->DeathComponent->GeneratedLoot.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON178LivingRestoreClearsDeathPresentationTest, "Grimrock.Monsters.MON17.8.LivingRestoreClearsDeathPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON178LivingRestoreClearsDeathPresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMonsterMON178Persistence;

	FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!Runtime || !MakeFloor(Runtime))
	{
		return false;
	}

	UGridMonsterDefinitionAsset* Definition = MakeDefinition(Runtime);
	const FGuid PersistenceId(1786, 2, 1, 1);
	const FIntPoint Cell(1, 3);
	AGridMonsterActor* Monster = SpawnMonster(TestWorld.World, Definition, PersistenceId, Cell, TEXT("MON178_LivingRestore"));
	if (!Monster)
	{
		return false;
	}

	const FGridRuntimeMonsterState DeadState = MakeState(Monster, Cell, true);
	TestTrue(TEXT("Dead precursor state restores"), Monster->RestoreRuntimeMonsterState(DeadState, Runtime));
	TestFalse(TEXT("Dead precursor hides the mesh"), Monster->SkeletalMeshComponent->IsVisible());

	const FGridRuntimeMonsterState LivingState = MakeState(Monster, Cell, false);
	TestTrue(TEXT("Living state restores after dead presentation state"), Monster->RestoreRuntimeMonsterState(LivingState, Runtime));
	TestFalse(TEXT("Living restore clears the death commit"), Monster->DeathComponent->bDeathCommitted);
	TestFalse(TEXT("Living restore clears loot generation guard"), Monster->DeathComponent->bLootGenerated);
	TestFalse(TEXT("Living restore has no active death presentation"), Monster->DeathComponent->bDeathPresentationActive);
	TestFalse(TEXT("Living restore has no active dissolve"), Monster->DeathComponent->bDeathDissolveActive);
	TestEqual(TEXT("Living restore resets dissolve alpha"), Monster->DeathComponent->DeathDissolveAlpha, 0.0f);
	TestTrue(TEXT("Living restore makes the mesh visible again"), Monster->SkeletalMeshComponent->IsVisible());
	TestFalse(TEXT("Living monster is not dead"), Monster->IsDead());
	TestEqual(TEXT("Living health restores"), Monster->CurrentHealth, 7);

	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	TestTrue(TEXT("Living restore re-establishes normal occupancy"), Occupancy && Occupancy->IsCellOccupied(Cell));
	return true;
}

#endif
