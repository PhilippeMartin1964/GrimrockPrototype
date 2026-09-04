#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridSecretDoorActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"

namespace
{
	struct FGridAcousticTestWorld
	{
		UWorld* World = nullptr;

		FGridAcousticTestWorld()
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
				FName(*FString::Printf(TEXT("AcousticHearingWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridAcousticTestWorld()
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

	UGridLevelAsset* MakeLevel(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 6;
		Level->Height = 4;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
			Cell.NorthWall = EGridWallType::None;
			Cell.EastWall = EGridWallType::None;
			Cell.SouthWall = EGridWallType::None;
			Cell.WestWall = EGridWallType::None;
		}
		return Level;
	}

	FGridLevelObjectData MakeDoor(FIntPoint Cell, EGridEdge Edge, FName ArchetypeId)
	{
		FGridLevelObjectData Door;
		Door.ObjectId = FGuid::NewGuid();
		Door.Type = EGridLevelObjectType::Door;
		Door.CellX = Cell.X;
		Door.CellY = Cell.Y;
		Door.Edge = Edge;
		Door.ArchetypeId = ArchetypeId;
		Door.bInitiallyEnabled = true;
		Door.bInitiallyActive = false;
		Door.Behavior.DoorAnimation.OpenHeight = 180.f;
		Door.Behavior.DoorAnimation.MoveDuration = 1.f;
		return Door;
	}

	AGridDoorActor* RegisterDoor(UWorld* World, UGridDoorSystemComponent* Doors, const FGridLevelObjectData& Data, UClass* DoorClass)
	{
		AGridDoorActor* Door = World ? World->SpawnActor<AGridDoorActor>(DoorClass) : nullptr;
		if (!Door || !Doors)
		{
			return nullptr;
		}
		Door->InitializeDoor(Data, nullptr, nullptr, FVector::ZeroVector, FRotator::ZeroRotator, false);
		Doors->RegisterDoorObject(Data, Door);
		return Door;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterAcousticHearingTest,
	"Grimrock.Monsters.Perception.AcousticHearing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterAcousticHearingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const auto OpenGrid = [](const FIntPoint&, const FIntPoint&) { return true; };
	TestTrue(TEXT("Hearing can follow a two-step corner"),
		FGridMonsterPerception::CanHearThroughGrid(FIntPoint(0, 0), FIntPoint(1, 1), 2, OpenGrid));
	TestFalse(TEXT("The corner exceeds a one-cell hearing budget"),
		FGridMonsterPerception::CanHearThroughGrid(FIntPoint(0, 0), FIntPoint(1, 1), 1, OpenGrid));

	FGridAcousticTestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!Runtime || !Party)
	{
		return false;
	}

	Runtime->bApplyLevelStartOnBeginPlay = false;
	UGridLevelAsset* Level = MakeLevel(Runtime);
	Runtime->LevelAsset = Level;

	Level->GetCellMutable(1, 1).EastWall = EGridWallType::Solid;
	Level->GetCellMutable(2, 1).WestWall = EGridWallType::Solid;

	Party->LevelRuntimeActor = Runtime;
	Party->CurrentCellX = 2;
	Party->CurrentCellY = 1;
	Party->SetActorLocation(Runtime->GetCellCenterWorld(2, 1));

	UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Runtime);
	Definition->MonsterId = TEXT("MON_AcousticDormant");
	Definition->DisplayName = FText::FromString(TEXT("Acoustic Dormant"));
	Definition->CategoryId = TEXT("Test");
	Definition->MaxHealth = 10;
	Definition->ActionPointsPerTurn = 2;
	Definition->SightRangeCells = 0;
	Definition->HearingRangeCells = 1;
	Definition->DeathExpectedDuration = 1.f;

	const FGuid SpawnId = FGuid::NewGuid();
	FGridLevelObjectData Spawn;
	Spawn.ObjectId = SpawnId;
	Spawn.Type = EGridLevelObjectType::MonsterSpawn;
	Spawn.CellX = 1;
	Spawn.CellY = 1;
	Spawn.InitialFacing = EGridEdge::East;
	Spawn.InitialMonsterState = EGridMonsterState::Dormant;
	Spawn.MonsterDefinitionAsset = Definition;
	Spawn.MonsterDefinitionId = Definition->MonsterId;
	Spawn.bInitiallyEnabled = true;
	Level->Objects.Add(Spawn);

	FActorSpawnParameters Params;
	Params.Owner = Runtime;
	AGridMonsterActor* Monster = TestWorld.World->SpawnActor<AGridMonsterActor>(
		AGridMonsterActor::StaticClass(), Runtime->GetCellCenterWorld(1, 1), FRotator::ZeroRotator, Params);
	if (!Monster || !Monster->InitializeMonster(Definition, SpawnId, FIntPoint(1, 1), EGridEdge::East))
	{
		return false;
	}

	UGridMonsterBehaviorComponent* Behavior = NewObject<UGridMonsterBehaviorComponent>(Monster, TEXT("AcousticBehavior"));
	Behavior->bAutoInitialize = false;
	Monster->AddInstanceComponent(Behavior);
	Behavior->RegisterComponent();
	TestTrue(TEXT("Behavior initializes"), Behavior->InitializeBehavior(Runtime, Party));

	TestFalse(TEXT("A solid wall blocks adjacent hearing"), Behavior->RefreshPerception());
	TestFalse(TEXT("The party is not heard through the wall"), Behavior->bCanHearParty);
	TestEqual(TEXT("Dormant remains Dormant behind a wall"), Monster->MonsterState, EGridMonsterState::Dormant);

	UGridDoorSystemComponent* Doors = Runtime->FindComponentByClass<UGridDoorSystemComponent>();
	if (!Doors)
	{
		return false;
	}
	Doors->Initialize(Runtime);

	FGridLevelObjectData NormalDoorData = MakeDoor(FIntPoint(1, 1), EGridEdge::East, TEXT("Door_Wood"));
	Level->Objects.Add(NormalDoorData);
	Doors->RebuildIndexes();
	TestNotNull(TEXT("Normal door registers"), RegisterDoor(TestWorld.World, Doors, NormalDoorData, AGridDoorActor::StaticClass()));
	TestTrue(TEXT("A closed normal door transmits sound"), Runtime->CanSoundTraverse(1, 1, EGridEdge::East));

	Monster->SetMonsterState(EGridMonsterState::Dormant);
	Behavior->bCanSeeParty = false;
	Behavior->bCanHearParty = false;
	TestTrue(TEXT("The dormant monster hears through a normal closed door"), Behavior->RefreshPerception());
	TestTrue(TEXT("Normal door produces hearing"), Behavior->bCanHearParty);
	TestEqual(TEXT("Hearing wakes Dormant to Alert"), Monster->MonsterState, EGridMonsterState::Alert);

	Level->GetCellMutable(3, 1).EastWall = EGridWallType::Solid;
	Level->GetCellMutable(4, 1).WestWall = EGridWallType::Solid;
	FGridLevelObjectData SecretDoorData = MakeDoor(FIntPoint(3, 1), EGridEdge::East, TEXT("Door_Secret"));
	Level->Objects.Add(SecretDoorData);
	Doors->RebuildIndexes();
	TestNotNull(TEXT("Secret door registers"), RegisterDoor(TestWorld.World, Doors, SecretDoorData, AGridSecretDoorActor::StaticClass()));
	TestTrue(TEXT("Secret door is identified"), Doors->IsSecretDoorOnEdge(3, 1, EGridEdge::East));
	TestFalse(TEXT("A closed secret door blocks sound"), Runtime->CanSoundTraverse(3, 1, EGridEdge::East));

	TestTrue(TEXT("Secret door state can be restored open"), Doors->ApplyDoorState(SecretDoorData.ObjectId, true, false));
	TestTrue(TEXT("A fully open secret door transmits sound"), Runtime->CanSoundTraverse(3, 1, EGridEdge::East));

	return true;
}

#endif
