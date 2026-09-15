#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
	struct FPUZZLE01Lua01World
	{
		UWorld* World = nullptr;

		FPUZZLE01Lua01World()
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
				FName(*FString::Printf(TEXT("PUZZLE01_LUA01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FPUZZLE01Lua01World()
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPUZZLE01Lua01RuntimeMaterialAliasTest,
	"Grimrock.PUZZLE01.LUA01.RuntimeMaterialAliasPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPUZZLE01Lua01RuntimeMaterialAliasTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FPUZZLE01Lua01World TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create PUZZLE01-LUA01 test world."));
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!Runtime)
	{
		AddError(TEXT("Unable to spawn level runtime actor."));
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 1;
	Level->Height = 1;
	Level->EnsureCellCount();
	Level->Cells[0].CellType = EGridCellType::Floor;
	Runtime->LevelAsset = Level;
	Runtime->CurrentDungeonLevelId = TEXT("PUZZLE01_LUA01");

	UMaterial* EmptyLeft = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* EmptyRight = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* BlueGem = NewObject<UMaterial>(GetTransientPackage());
	UStaticMesh* GuardianMesh = NewObject<UStaticMesh>(GetTransientPackage());
	GuardianMesh->GetStaticMaterials().Add(FStaticMaterial(EmptyLeft, FName(TEXT("EyesLeft"))));
	GuardianMesh->GetStaticMaterials().Add(FStaticMaterial(EmptyRight, FName(TEXT("EyesRight"))));

	UGridWorldObjectDefinitionAsset* GuardianDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	GuardianDefinition->DefinitionId = TEXT("Guardian");
	GuardianDefinition->SupportedType = EGridLevelObjectType::Decoration;
	GuardianDefinition->PlacementSurface = EGridObjectPlacementKind::Center;
	GuardianDefinition->StaticPart.Mesh = GuardianMesh;
	GuardianDefinition->RuntimeActorClass = AGridRuntimeObjectActor::StaticClass();
	GuardianDefinition->RuntimeMaterialAliases.Add(TEXT("BlueGem"), BlueGem);
	Runtime->WorldObjectDefinitions.Add(GuardianDefinition);

	const FGuid GuardianId(1, 1, 1, 1);
	FGridWorldObjectInstance GuardianPlacement;
	GuardianPlacement.InstanceId = GuardianId;
	GuardianPlacement.Type = EGridLevelObjectType::Decoration;
	GuardianPlacement.CellX = 0;
	GuardianPlacement.CellY = 0;
	GuardianPlacement.WorldObjectDefinitionId = GuardianDefinition->DefinitionId;
	GuardianPlacement.LogicId = TEXT("Guardian");
	Level->WorldObjectInstances.Add(GuardianPlacement);

	Runtime->RebuildLevel();
	AGridRuntimeObjectActor* Guardian = Runtime->FindRuntimeObjectActor<AGridRuntimeObjectActor>(GuardianId);
	TestNotNull(TEXT("Guardian runtime object is spawned and registered"), Guardian);
	if (!Guardian || !Guardian->MeshComponent)
	{
		return false;
	}

	TestTrue(TEXT("EyesLeft starts with its authored material"), Guardian->MeshComponent->GetMaterial(0) == EmptyLeft);
	TestTrue(TEXT("EyesRight starts with its authored material"), Guardian->MeshComponent->GetMaterial(1) == EmptyRight);

	FString Error;
	TestTrue(TEXT("Definition-owned BlueGem alias can be applied to EyesLeft"),
		Guardian->SetRuntimeMaterialAlias(TEXT("EyesLeft"), TEXT("BlueGem"), true, Error));
	TestTrue(TEXT("EyesLeft now uses the BlueGem material"), Guardian->MeshComponent->GetMaterial(0) == BlueGem);
	TestTrue(TEXT("EyesRight remains unchanged"), Guardian->MeshComponent->GetMaterial(1) == EmptyRight);

	const FGridLevelRuntimeState* SavedState = Runtime->FindRuntimeStateForCurrentLevel();
	TestNotNull(TEXT("Persistent level runtime state exists"), SavedState);
	const FGridRuntimeObjectVisualState* SavedVisual = SavedState ? SavedState->ObjectVisuals.Find(GuardianId) : nullptr;
	TestNotNull(TEXT("Material alias override is stored by ObjectId"), SavedVisual);
	if (SavedVisual)
	{
		const FName* SavedAlias = SavedVisual->MaterialAliasesBySlot.Find(TEXT("EyesLeft"));
		TestNotNull(TEXT("EyesLeft override is stored by semantic slot"), SavedAlias);
		if (SavedAlias)
		{
			TestEqual(TEXT("Only the alias is persisted"), *SavedAlias, FName(TEXT("BlueGem")));
		}
	}

	TestFalse(TEXT("Unknown material alias is rejected"), Guardian->SetRuntimeMaterialAlias(TEXT("EyesRight"), TEXT("MissingAlias"), true, Error));
	TestFalse(TEXT("Unknown material slot is rejected"), Guardian->SetRuntimeMaterialAlias(TEXT("MissingSlot"), TEXT("BlueGem"), true, Error));

	TestTrue(TEXT("Runtime state captures before SaveGame round-trip"), Runtime->CaptureCurrentLevelRuntimeState());
	UGrimrockPartySaveGame* SourceSave = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
	SourceSave->DungeonRuntimeState = Runtime->DungeonRuntimeState;

	TArray<uint8> SaveBytes;
	TestTrue(TEXT("Visual override serializes through the real UE SaveGame archive"), UGameplayStatics::SaveGameToMemory(SourceSave, SaveBytes));
	UGrimrockPartySaveGame* LoadedSave = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
	TestNotNull(TEXT("Visual override deserializes through the real UE SaveGame archive"), LoadedSave);
	if (!LoadedSave)
	{
		return false;
	}

	const FGridLevelRuntimeState* LoadedState = LoadedSave->DungeonRuntimeState.LevelStates.Find(TEXT("PUZZLE01_LUA01"));
	TestNotNull(TEXT("Round-tripped level runtime state exists"), LoadedState);
	const FGridRuntimeObjectVisualState* LoadedVisual = LoadedState ? LoadedState->ObjectVisuals.Find(GuardianId) : nullptr;
	TestNotNull(TEXT("Round-tripped visual override exists"), LoadedVisual);
	if (LoadedVisual)
	{
		const FName* LoadedAlias = LoadedVisual->MaterialAliasesBySlot.Find(TEXT("EyesLeft"));
		TestNotNull(TEXT("Round-tripped EyesLeft alias exists"), LoadedAlias);
		if (LoadedAlias)
		{
			TestEqual(TEXT("Round-tripped EyesLeft alias remains BlueGem"), *LoadedAlias, FName(TEXT("BlueGem")));
		}
	}

	// Reproduce the real load bug: the level actor and Guardian already exist with authored visuals
	// when the SaveGame DungeonRuntimeState is injected. ApplyCurrentLevelRuntimeState must restore
	// ObjectVisuals explicitly instead of relying on a fresh actor initialization/rebuild.
	Guardian->MeshComponent->SetMaterial(0, EmptyLeft);
	TestTrue(TEXT("Existing Guardian is reset to authored EyesLeft before load apply"), Guardian->MeshComponent->GetMaterial(0) == EmptyLeft);
	Runtime->DungeonRuntimeState = LoadedSave->DungeonRuntimeState;
	TestTrue(TEXT("Loaded runtime state applies to the existing level"), Runtime->ApplyCurrentLevelRuntimeState());
	TestTrue(TEXT("SaveGame load reapplies persisted EyesLeft material to existing Guardian"), Guardian->MeshComponent->GetMaterial(0) == BlueGem);
	TestTrue(TEXT("SaveGame load leaves unmodified EyesRight authored material intact"), Guardian->MeshComponent->GetMaterial(1) == EmptyRight);
	return true;
}

#endif