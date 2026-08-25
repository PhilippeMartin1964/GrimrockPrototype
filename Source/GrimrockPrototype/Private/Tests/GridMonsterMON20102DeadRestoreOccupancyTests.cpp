#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimInstance.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace GridMonsterMON20102DeadRestoreOccupancy
{
	const FName RuntimeLevelId(TEXT("SingleLevel"));

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
				FName(*FString::Printf(TEXT("MON20102DeadRestore_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
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

	UGridLevelAsset* MakeLevel(AGridLevelRuntimeActor* Runtime)
	{
		if (!Runtime)
		{
			return nullptr;
		}

		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
		Level->Width = 4;
		Level->Height = 4;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		Runtime->LevelAsset = Level;
		return Level;
	}

	UGridMonsterDefinitionAsset* MakeDefinition(FAutomationTestBase& Test, UObject* Outer, FName MonsterId)
	{
		USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Meshes/SK_RatGiant.SK_RatGiant"));
		UClass* AnimationClass =
			LoadClass<UAnimInstance>(nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Animation/ABP_MON_RatGiant.ABP_MON_RatGiant_C"));
		UClass* MonsterActorClass =
			LoadClass<AGridMonsterActor>(nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Blueprints/BP_MON_RatGiant.BP_MON_RatGiant_C"));

		Test.TestNotNull(TEXT("MON20.10.2 fixture loads Rat Giant skeletal mesh"), SkeletalMesh);
		Test.TestNotNull(TEXT("MON20.10.2 fixture loads Rat Giant animation class"), AnimationClass);
		Test.TestNotNull(TEXT("MON20.10.2 fixture loads Rat Giant actor class"), MonsterActorClass);
		if (!SkeletalMesh || !AnimationClass || !MonsterActorClass)
		{
			return nullptr;
		}

		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Outer);
		Definition->MonsterId = MonsterId;
		Definition->DisplayName = FText::FromString(TEXT("MON20.10.2 Restore Monster"));
		Definition->CategoryId = TEXT("Test");
		Definition->MaxHealth = 10;
		Definition->ActionPointsPerTurn = 2;
		Definition->DeathExpectedDuration = 1.0f;
		Definition->SkeletalMesh = TSoftObjectPtr<USkeletalMesh>(SkeletalMesh);
		Definition->AnimationClass = AnimationClass;
		Definition->MonsterActorClass = MonsterActorClass;
		return Definition;
	}

	FGridLevelObjectData MakeSpawn(UGridMonsterDefinitionAsset* Definition, const FGuid& SpawnId, FIntPoint Cell)
	{
		FGridLevelObjectData Spawn;
		Spawn.ObjectId = SpawnId;
		Spawn.Type = EGridLevelObjectType::MonsterSpawn;
		Spawn.CellX = Cell.X;
		Spawn.CellY = Cell.Y;
		Spawn.Edge = EGridEdge::None;
		Spawn.InitialFacing = EGridEdge::North;
		Spawn.MonsterDefinitionAsset = Definition;
		Spawn.MonsterDefinitionId = Definition ? Definition->MonsterId : NAME_None;
		Spawn.EncounterGroupId = TEXT("MON20102_Encounter");
		Spawn.bInitiallyEnabled = true;
		return Spawn;
	}

	FGridRuntimeMonsterState MakeRestoreState(const FGuid& SpawnId, FName MonsterDefinitionId, FIntPoint Cell, bool bDead)
	{
		FGridRuntimeMonsterState State;
		State.PersistenceId = SpawnId;
		State.SpawnObjectId = SpawnId;
		State.MonsterDefinitionId = MonsterDefinitionId;
		State.DungeonLevelId = RuntimeLevelId;
		State.CellX = Cell.X;
		State.CellY = Cell.Y;
		State.Facing = EGridEdge::North;
		State.MonsterState = bDead ? EGridMonsterState::Dead : EGridMonsterState::Idle;
		State.CurrentHealth = bDead ? 0 : 7;
		State.CurrentPhysicalArmor = 0;
		State.CurrentMagicalArmor = 0;
		State.bMonsterEnabled = true;
		State.EncounterGroupId = TEXT("MON20102_Encounter");
		State.bIsDead = bDead;
		return State;
	}

	void InstallRestoreState(AGridLevelRuntimeActor* Runtime, const FGuid& SpawnId, const FGridRuntimeMonsterState& MonsterState)
	{
		FGridLevelRuntimeState& LevelState = Runtime->DungeonRuntimeState.LevelStates.FindOrAdd(RuntimeLevelId);
		LevelState.LevelId = RuntimeLevelId;
		LevelState.bHasBeenVisited = true;
		LevelState.Monsters.Add(SpawnId, MonsterState);

		FGridRuntimeMonsterPlacementState PlacementState;
		PlacementState.SpawnId = SpawnId;
		PlacementState.bIsSpawned = true;
		PlacementState.bHasMonsterState = true;
		PlacementState.MonsterState = MonsterState;
		LevelState.MonsterPlacements.Add(SpawnId, PlacementState);
	}

	AGrimrockPartyPawn* SpawnParty(UWorld* World, AGridLevelRuntimeActor* Runtime, FIntPoint Cell)
	{
		if (!World || !Runtime)
		{
			return nullptr;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AGrimrockPartyPawn* Party = World->SpawnActor<AGrimrockPartyPawn>(AGrimrockPartyPawn::StaticClass(), FTransform::Identity, Params);
		if (Party)
		{
			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = Cell.X;
			Party->CurrentCellY = Cell.Y;
		}
		return Party;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON20102DeadRestoreOverPartyCellTest, "Grimrock.MON20.10.2.DeadRestoreOverPartyCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON20102DeadRestoreOverPartyCellTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMonsterMON20102DeadRestoreOccupancy;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("MON20.10.2 test world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime exists"), Runtime))
	{
		return false;
	}

	UGridLevelAsset* Level = MakeLevel(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeDefinition(*this, Runtime, TEXT("MON20102_DeadRestore"));
	if (!Level || !Definition)
	{
		return false;
	}

	const FGuid SpawnId(20, 10, 2, 1);
	const FIntPoint SharedCell(2, 2);
	Level->Objects.Add(MakeSpawn(Definition, SpawnId, SharedCell));
	InstallRestoreState(Runtime, SpawnId, MakeRestoreState(SpawnId, Definition->MonsterId, SharedCell, true));

	AGrimrockPartyPawn* Party = SpawnParty(TestWorld.World, Runtime, SharedCell);
	TestNotNull(TEXT("Party exists"), Party);
	TestTrue(TEXT("Party occupies the saved monster cell"), Runtime->IsPartyOnCell(SharedCell.X, SharedCell.Y));

	Runtime->RebuildLevel();

	TestEqual(TEXT("Dead restore does not count as a spawn failure"), Runtime->GetMonsterSpawnFailureCount(), 0);
	AGridMonsterActor* Monster = Runtime->FindSpawnedMonsterActor(SpawnId);
	if (!TestNotNull(TEXT("Dead restored monster actor is retained"), Monster))
	{
		return false;
	}

	TestTrue(TEXT("Restored monster is logically dead"), Monster->IsDead());
	TestEqual(TEXT("Restored dead health is zero"), Monster->CurrentHealth, 0);
	TestFalse(TEXT("Restored dead mesh is hidden"), Monster->SkeletalMeshComponent->IsVisible());
	TestEqual(TEXT("Restored dead collision is disabled"), Monster->CollisionComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);

	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	TestNotNull(TEXT("Occupancy subsystem exists"), Occupancy);
	TestFalse(TEXT("Restored dead monster does not occupy the shared cell"), Occupancy && Occupancy->IsCellOccupied(SharedCell));

	TestTrue(TEXT("Full runtime-state application accepts the retained dead actor"), Runtime->ApplyCurrentLevelRuntimeState());
	TestTrue(TEXT("Dead actor remains available after full state application"), Runtime->FindSpawnedMonsterActor(SpawnId) == Monster);
	TestFalse(TEXT("Dead actor remains non-occupying after full state application"), Occupancy && Occupancy->IsCellOccupied(SharedCell));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON20102LivingRestoreStillRejectsPartyCellTest, "Grimrock.MON20.10.2.LivingRestoreStillRejectsPartyCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON20102LivingRestoreStillRejectsPartyCellTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMonsterMON20102DeadRestoreOccupancy;

	FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!Runtime)
	{
		return false;
	}

	UGridLevelAsset* Level = MakeLevel(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeDefinition(*this, Runtime, TEXT("MON20102_LivingRestore"));
	if (!Level || !Definition)
	{
		return false;
	}

	const FGuid SpawnId(20, 10, 2, 2);
	const FIntPoint SharedCell(1, 2);
	Level->Objects.Add(MakeSpawn(Definition, SpawnId, SharedCell));
	InstallRestoreState(Runtime, SpawnId, MakeRestoreState(SpawnId, Definition->MonsterId, SharedCell, false));

	if (!SpawnParty(TestWorld.World, Runtime, SharedCell))
	{
		return false;
	}

	AddExpectedError(TEXT("Reason=PartyOccupiesCell"), EAutomationExpectedErrorFlags::Contains, 1);

	Runtime->RebuildLevel();

	TestEqual(TEXT("Living restore on the party cell remains a spawn failure"), Runtime->GetMonsterSpawnFailureCount(), 1);
	TestNull(TEXT("Living restore does not create an overlapping actor"), Runtime->FindSpawnedMonsterActor(SpawnId));
	return true;
}

#endif
