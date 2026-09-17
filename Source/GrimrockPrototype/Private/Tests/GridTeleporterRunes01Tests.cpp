#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
	struct FTeleporterRunes01World
	{
		UWorld* World = nullptr;

		FTeleporterRunes01World()
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
				FName(*FString::Printf(TEXT("TELEPORTER_RUNES01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTeleporterRunes01World()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTeleporterRunes01ActiveMaterialPresentationTest,
	"Grimrock.Teleporter.RUNES01.ActiveMaterialPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTeleporterRunes01ActiveMaterialPresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FTeleporterRunes01World TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create TELEPORTER-RUNES01 test world."));
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

	UMaterial* Authored = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* Active = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* Inactive = NewObject<UMaterial>(GetTransientPackage());
	UStaticMesh* TeleporterMesh = NewObject<UStaticMesh>(GetTransientPackage());
	TeleporterMesh->GetStaticMaterials().Add(FStaticMaterial(Authored, FName(TEXT("Teleporter"))));

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	Definition->DefinitionId = TEXT("Teleporter_Test");
	Definition->SupportedType = EGridLevelObjectType::Relocation;
	Definition->PlacementSurface = EGridObjectPlacementKind::Floor;
	Definition->StaticPart.Mesh = TeleporterMesh;
	Definition->StaticPart.StateMaterialSlot = TEXT("Teleporter");
	Definition->StaticPart.ActiveMaterialAlias = TEXT("Active");
	Definition->StaticPart.InactiveMaterialAlias = TEXT("Inactive");
	Definition->RuntimeMaterialAliases.Add(TEXT("Active"), Active);
	Definition->RuntimeMaterialAliases.Add(TEXT("Inactive"), Inactive);
	Definition->RuntimeActorClass = AGridGenericObjectActor::StaticClass();
	Runtime->WorldObjectDefinitions.Add(Definition);

	const FGuid TeleporterId(9, 8, 7, 6);
	FGridWorldObjectInstance Placement;
	Placement.InstanceId = TeleporterId;
	Placement.Type = EGridLevelObjectType::Relocation;
	Placement.CellX = 0;
	Placement.CellY = 0;
	Placement.WorldObjectDefinitionId = Definition->DefinitionId;
	Placement.InstanceConfig.bRelocationInitiallyEnabled = false;
	Level->WorldObjectInstances.Add(Placement);

	Runtime->AddRuntimeObjectActor(Placement);
	AGridGenericObjectActor* Teleporter = Runtime->FindRuntimeObjectActor<AGridGenericObjectActor>(TeleporterId);
	TestNotNull(TEXT("Teleporter generic actor exists"), Teleporter);
	if (!Teleporter || !Teleporter->MeshComponent)
	{
		return false;
	}

	TestTrue(TEXT("Inactive relocation uses inactive material alias at initialization"), Teleporter->MeshComponent->GetMaterial(0) == Inactive);
	Teleporter->SetRuntimeActivePresentation(true);
	TestTrue(TEXT("Active presentation uses active material alias"), Teleporter->MeshComponent->GetMaterial(0) == Active);
	Teleporter->SetRuntimeActivePresentation(false);
	TestTrue(TEXT("Inactive presentation restores inactive material alias"), Teleporter->MeshComponent->GetMaterial(0) == Inactive);

	const FGridLevelRuntimeState* RuntimeState = Runtime->FindRuntimeStateForCurrentLevel();
	TestTrue(TEXT("Presentation-only material switching does not create persisted visual state"),
		!RuntimeState || !RuntimeState->ObjectVisuals.Contains(TeleporterId));
	return true;
}

#endif
