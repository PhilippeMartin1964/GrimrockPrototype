#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Sound/SoundWave.h"

namespace
{
	struct FSemanticAudioRoutingWorld
	{
		UWorld* World = nullptr;

		FSemanticAudioRoutingWorld()
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
				FName(*FString::Printf(TEXT("SemanticAudioRouting_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FSemanticAudioRoutingWorld()
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

	FGridObjectAudioEvent MakeTwoVariantEvent(UObject* Outer, USoundWave*& OutFirst, USoundWave*& OutSecond)
	{
		OutFirst = NewObject<USoundWave>(Outer);
		OutSecond = NewObject<USoundWave>(Outer);
		FGridObjectAudioEvent Event;
		Event.PitchVariation = 0.0f;
		Event.Sounds.Add(OutFirst);
		Event.Sounds.Add(OutSecond);
		return Event;
	}

	void ConfigureMechanismDefinition(UGridWorldObjectDefinitionAsset& Definition, FName DefinitionId, EGridLevelObjectType Type, TSubclassOf<AGridRuntimeObjectActor> ActorClass)
	{
		Definition.DefinitionId = DefinitionId;
		Definition.SupportedType = Type;
		Definition.RuntimeActorClass = ActorClass;
		Definition.MovingParts.SetNum(1);
		Definition.MovingParts[0].Mesh = NewObject<UStaticMesh>(&Definition);
		Definition.MovingParts[0].Motion.Type = EGridWorldObjectMotionType::Translation;
		Definition.MovingParts[0].Motion.Axis = EGridWorldObjectMotionAxis::Z;
		Definition.MovingParts[0].Motion.Amount = 5.0f;
		Definition.MovingParts[0].Motion.Duration = 0.1f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridSemanticObjectAudioRoutingTest,
	"Grimrock.Runtime.Objects.SemanticAudioRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridSemanticObjectAudioRoutingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FSemanticAudioRoutingWorld TestWorld;
	if (!TestNotNull(TEXT("Semantic-audio transient world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Semantic-audio runtime exists"), Runtime))
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 1;
	Level->Height = 1;
	Level->EnsureCellCount();
	Level->Cells[0].CellType = EGridCellType::Floor;
	Runtime->LevelAsset = Level;

	UGridWorldObjectDefinitionAsset* LeverDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	ConfigureMechanismDefinition(*LeverDefinition, TEXT("Lever_SemanticAudio"), EGridLevelObjectType::Lever, AGridLeverActor::StaticClass());
	USoundWave* LeverActivatedA = nullptr;
	USoundWave* LeverActivatedB = nullptr;
	USoundWave* LeverDeactivatedA = nullptr;
	USoundWave* LeverDeactivatedB = nullptr;
	LeverDefinition->AudioEvents.Add(TEXT("Activated"), MakeTwoVariantEvent(LeverDefinition, LeverActivatedA, LeverActivatedB));
	LeverDefinition->AudioEvents.Add(TEXT("Deactivated"), MakeTwoVariantEvent(LeverDefinition, LeverDeactivatedA, LeverDeactivatedB));

	UGridWorldObjectDefinitionAsset* PlateDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	ConfigureMechanismDefinition(*PlateDefinition, TEXT("Plate_SemanticAudio"), EGridLevelObjectType::PressurePlate, AGridPressurePlateActor::StaticClass());
	USoundWave* PlateActivatedA = nullptr;
	USoundWave* PlateActivatedB = nullptr;
	PlateDefinition->AudioEvents.Add(TEXT("Activated"), MakeTwoVariantEvent(PlateDefinition, PlateActivatedA, PlateActivatedB));

	UGridWorldObjectDefinitionAsset* ReceptacleDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	ReceptacleDefinition->DefinitionId = TEXT("Receptacle_SemanticAudio");
	ReceptacleDefinition->SupportedType = EGridLevelObjectType::Receptacle;
	ReceptacleDefinition->RuntimeActorClass = AGridReceptacleActor::StaticClass();
	ReceptacleDefinition->StaticPart.Mesh = NewObject<UStaticMesh>(ReceptacleDefinition);
	USoundWave* InsertedA = nullptr;
	USoundWave* InsertedB = nullptr;
	USoundWave* RemovedA = nullptr;
	USoundWave* RemovedB = nullptr;
	USoundWave* ChangedA = nullptr;
	USoundWave* ChangedB = nullptr;
	ReceptacleDefinition->AudioEvents.Add(TEXT("ItemInserted"), MakeTwoVariantEvent(ReceptacleDefinition, InsertedA, InsertedB));
	ReceptacleDefinition->AudioEvents.Add(TEXT("ItemRemoved"), MakeTwoVariantEvent(ReceptacleDefinition, RemovedA, RemovedB));
	ReceptacleDefinition->AudioEvents.Add(TEXT("ItemChanged"), MakeTwoVariantEvent(ReceptacleDefinition, ChangedA, ChangedB));

	Runtime->WorldObjectDefinitions.Add(LeverDefinition);
	Runtime->WorldObjectDefinitions.Add(PlateDefinition);
	Runtime->WorldObjectDefinitions.Add(ReceptacleDefinition);

	const FGuid LeverId(1, 1, 1, 1);
	const FGuid PlateId(2, 2, 2, 2);
	const FGuid ReceptacleId(3, 3, 3, 3);

	FGridWorldObjectInstance LeverPlacement;
	LeverPlacement.InstanceId = LeverId;
	LeverPlacement.Type = EGridLevelObjectType::Lever;
	LeverPlacement.WorldObjectDefinitionId = LeverDefinition->DefinitionId;
	LeverPlacement.CellX = 0;
	LeverPlacement.CellY = 0;
	LeverPlacement.WallSide = EGridEdge::North;
	Level->WorldObjectInstances.Add(LeverPlacement);

	FGridWorldObjectInstance PlatePlacement;
	PlatePlacement.InstanceId = PlateId;
	PlatePlacement.Type = EGridLevelObjectType::PressurePlate;
	PlatePlacement.WorldObjectDefinitionId = PlateDefinition->DefinitionId;
	PlatePlacement.CellX = 0;
	PlatePlacement.CellY = 0;
	Level->WorldObjectInstances.Add(PlatePlacement);

	FGridWorldObjectInstance ReceptaclePlacement;
	ReceptaclePlacement.InstanceId = ReceptacleId;
	ReceptaclePlacement.Type = EGridLevelObjectType::Receptacle;
	ReceptaclePlacement.WorldObjectDefinitionId = ReceptacleDefinition->DefinitionId;
	ReceptaclePlacement.CellX = 0;
	ReceptaclePlacement.CellY = 0;
	ReceptaclePlacement.WallSide = EGridEdge::East;
	Level->WorldObjectInstances.Add(ReceptaclePlacement);

	Runtime->AddRuntimeObjectActor(LeverPlacement);
	Runtime->AddRuntimeObjectActor(PlatePlacement);
	Runtime->AddRuntimeObjectActor(ReceptaclePlacement);

	AGridLeverActor* Lever = Runtime->FindRuntimeObjectActor<AGridLeverActor>(LeverId);
	AGridPressurePlateActor* Plate = Runtime->FindRuntimeObjectActor<AGridPressurePlateActor>(PlateId);
	AGridReceptacleActor* Receptacle = Runtime->FindRuntimeObjectActor<AGridReceptacleActor>(ReceptacleId);
	if (!TestNotNull(TEXT("Lever runtime actor exists"), Lever) ||
		!TestNotNull(TEXT("PressurePlate runtime actor exists"), Plate) ||
		!TestNotNull(TEXT("Receptacle runtime actor exists"), Receptacle))
	{
		return false;
	}

	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	if (!TestNotNull(TEXT("Activation component exists"), Activation))
	{
		return false;
	}
	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();

	Runtime->ExecuteLinksFromRuntimeObject(LeverId, EGridObjectEvent::Activated);
	const FGridObjectAudioPlaybackResult LeverAfterActivated = Lever->PlayObjectAudioEventDetailed(TEXT("Activated"), false);
	TestTrue(TEXT("Lever Activated gameplay event consumes its first audio variant"), LeverAfterActivated.Sound == LeverActivatedB);

	Runtime->ExecuteLinksFromRuntimeObject(LeverId, EGridObjectEvent::Deactivated);
	const FGridObjectAudioPlaybackResult LeverAfterDeactivated = Lever->PlayObjectAudioEventDetailed(TEXT("Deactivated"), false);
	TestTrue(TEXT("Lever Deactivated gameplay event consumes its first audio variant"), LeverAfterDeactivated.Sound == LeverDeactivatedB);

	Runtime->ExecuteLinksFromRuntimeObject(PlateId, EGridObjectEvent::Activated);
	const FGridObjectAudioPlaybackResult PlateAfterActivated = Plate->PlayObjectAudioEventDetailed(TEXT("Activated"), false);
	TestTrue(TEXT("PressurePlate Activated gameplay event consumes its first audio variant"), PlateAfterActivated.Sound == PlateActivatedB);

	Runtime->ExecuteLinksFromRuntimeObject(ReceptacleId, EGridObjectEvent::ItemInserted);
	const FGridObjectAudioPlaybackResult AfterInserted = Receptacle->PlayObjectAudioEventDetailed(TEXT("ItemInserted"), false);
	TestTrue(TEXT("Receptacle ItemInserted gameplay event consumes its first audio variant"), AfterInserted.Sound == InsertedB);

	Runtime->ExecuteLinksFromRuntimeObject(ReceptacleId, EGridObjectEvent::ItemRemoved);
	const FGridObjectAudioPlaybackResult AfterRemoved = Receptacle->PlayObjectAudioEventDetailed(TEXT("ItemRemoved"), false);
	TestTrue(TEXT("Receptacle ItemRemoved gameplay event consumes its first audio variant"), AfterRemoved.Sound == RemovedB);

	Runtime->ExecuteLinksFromRuntimeObject(ReceptacleId, EGridObjectEvent::ItemChanged);
	const FGridObjectAudioPlaybackResult AfterChanged = Receptacle->PlayObjectAudioEventDetailed(TEXT("ItemChanged"), false);
	TestTrue(TEXT("Receptacle ItemChanged gameplay event consumes its first audio variant"), AfterChanged.Sound == ChangedB);

	return true;
}

#endif
