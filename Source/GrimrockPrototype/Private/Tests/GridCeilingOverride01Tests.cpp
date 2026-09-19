#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "UObject/UnrealType.h"

namespace GridCeilingOverride01
{
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

			World = UWorld::CreateWorld(
				EWorldType::Game, false,
				FName(*FString::Printf(TEXT("CEILING_OVERRIDE01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);

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

	UInstancedStaticMeshComponent* FindCeilingISM(AGridLevelRuntimeActor* Runtime)
	{
		if (!Runtime)
		{
			return nullptr;
		}

		TArray<UInstancedStaticMeshComponent*> Components;
		Runtime->GetComponents<UInstancedStaticMeshComponent>(Components);
		for (UInstancedStaticMeshComponent* Component : Components)
		{
			if (Component && Component->GetFName() == TEXT("CeilingISM"))
			{
				return Component;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridCeilingOverride01Test,
	"Grimrock.WorldObjects.CEILING_OVERRIDE01.HideBaseCeiling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridCeilingOverride01Test::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* DefinitionClass = UGridWorldObjectDefinitionAsset::StaticClass();
	FProperty* HideCeilingProperty = DefinitionClass ? DefinitionClass->FindPropertyByName(TEXT("bHideCellCeiling")) : nullptr;
	if (!TestNotNull(TEXT("Hide Cell Ceiling authoring property exists"), HideCeilingProperty))
	{
		return false;
	}
	TestEqual(TEXT("Hide Cell Ceiling display name"), HideCeilingProperty->GetMetaData(TEXT("DisplayName")), FString(TEXT("Hide Cell Ceiling")));
	TestEqual(TEXT("Hide Cell Ceiling belongs to Rendering|Cell Override"), HideCeilingProperty->GetMetaData(TEXT("Category")),
		FString(TEXT("Rendering|Cell Override")));
	TestTrue(TEXT("Hide Cell Ceiling is editable"), HideCeilingProperty->HasAnyPropertyFlags(CPF_Edit));

	GridCeilingOverride01::FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Ceiling override transient world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Ceiling override runtime exists"), Runtime))
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 1;
	Level->Height = 1;
	Level->CellSize = 200.0f;
	Level->EnsureCellCount();
	Level->Cells[0].CellType = EGridCellType::Floor;
	Level->Cells[0].bHasCeiling = true;
	Runtime->LevelAsset = Level;
	Runtime->FloorMesh = NewObject<UStaticMesh>(Runtime);
	Runtime->WallMesh = NewObject<UStaticMesh>(Runtime);
	Runtime->CeilingMesh = NewObject<UStaticMesh>(Runtime);

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	Definition->DefinitionId = TEXT("Ceiling_Shaft_Test");
	Definition->SupportedType = EGridLevelObjectType::Decoration;
	Definition->PlacementSurface = EGridObjectPlacementKind::Ceiling;
	Runtime->WorldObjectDefinitions.Add(Definition);

	FGridWorldObjectInstance Shaft;
	Shaft.InstanceId = FGuid::NewGuid();
	Shaft.Type = EGridLevelObjectType::Decoration;
	Shaft.WorldObjectDefinitionId = Definition->DefinitionId;
	Shaft.CellX = 0;
	Shaft.CellY = 0;
	Shaft.WallSide = EGridEdge::None;
	Level->WorldObjectInstances.Add(Shaft);

	TestFalse(TEXT("Ceiling override defaults to disabled"), Definition->bHideCellCeiling);
	TestFalse(TEXT("Runtime keeps the base ceiling when override is disabled"), Runtime->ShouldHideCellCeiling(0, 0));

	Runtime->RebuildLevel(EGridRuntimeRebuildMode::GeometryOnly);
	UInstancedStaticMeshComponent* CeilingISM = GridCeilingOverride01::FindCeilingISM(Runtime);
	if (!TestNotNull(TEXT("Ceiling ISM exists"), CeilingISM))
	{
		return false;
	}
	TestEqual(TEXT("Base ceiling is generated without the override"), CeilingISM->GetInstanceCount(), 1);

	Definition->bHideCellCeiling = true;
	TestTrue(TEXT("Runtime detects the ceiling override on its cell"), Runtime->ShouldHideCellCeiling(0, 0));
	TestFalse(TEXT("Ceiling override does not leak to another cell"), Runtime->ShouldHideCellCeiling(1, 0));

	Runtime->RebuildLevel(EGridRuntimeRebuildMode::GeometryOnly);
	TestEqual(TEXT("Base ceiling is suppressed when Hide Cell Ceiling is enabled"), CeilingISM->GetInstanceCount(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
