#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTRAP016Highlight, "Grimrock.TRAP01.6.HighlightTransitionsAndRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP016Highlight::RunTest(const FString&)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues()
													.AllowAudioPlayback(false)
													.RequiresHitProxies(false)
													.CreatePhysicsScene(false)
													.CreateNavigation(false)
													.CreateAISystem(false)
													.ShouldSimulatePhysics(false)
													.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName(*FString::Printf(TEXT("TRAP016_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
		nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Transient world"), World))
		return false;
	if (GEngine)
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto Cleanup = [&]()
	{
		World->DestroyWorld(false);
		if (GEngine)
			GEngine->DestroyWorldContext(World);
	};

	AGridLevelRuntimeActor* Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("Runtime"), Runtime) || !TestNotNull(TEXT("Party"), Party))
	{
		Cleanup();
		return false;
	}
	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->EnsureCellCount();
	Level->GetCellMutable(1, 1).CellType = EGridCellType::Floor;
	Runtime->LevelAsset = Level;
	Party->LevelRuntimeActor = Runtime;
	Party->CurrentCellX = Party->CurrentCellY = 1;

	UGridWorldObjectDefinitionAsset* Def = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	Def->DefinitionId = TEXT("TRAP016_Visual");
	Def->SupportedType = EGridLevelObjectType::Trigger;
	Def->DungeonTrap.bIsDungeonTrap = true;
	Def->RuntimeActorClass = AGridGenericObjectActor::StaticClass();
	Def->StaticPart.Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	Runtime->WorldObjectDefinitions.Add(Def);
	// Transient material instance exercises assignment/removal without authoring an asset.
	UMaterialInstanceDynamic* Overlay = UMaterialInstanceDynamic::Create(UMaterial::GetDefaultMaterial(MD_Surface), World);
	if (!TestNotNull(TEXT("Test mesh"), Def->StaticPart.Mesh.Get()) || !TestNotNull(TEXT("Test overlay instance"), Overlay))
	{
		Cleanup();
		return false;
	}
	FGridWorldObjectInstance Trap;
	Trap.InstanceId = FGuid::NewGuid();
	Trap.Type = EGridLevelObjectType::Trigger;
	Trap.WorldObjectDefinitionId = Def->DefinitionId;
	Trap.CellX = Trap.CellY = 1;
	Level->WorldObjectInstances.Add(Trap);
	Runtime->RestoreDungeonTrapRevealVisuals();
	TestNull(TEXT("Hidden trap is not materialized by restore"), Runtime->FindRuntimeObjectActor<AGridRuntimeObjectActor>(Trap.InstanceId));
	TestTrue(TEXT("Detection creates the visual"), Runtime->TryRecordDungeonTrapDetection(Trap.InstanceId));
	AGridRuntimeObjectActor* Visual = Runtime->FindRuntimeObjectActor<AGridRuntimeObjectActor>(Trap.InstanceId);
	FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	if (!TestNotNull(TEXT("Visual"), Visual) || !TestNotNull(TEXT("Mesh"), Visual->MeshComponent.Get()) || !TestNotNull(TEXT("Persistent state"), State))
	{
		Cleanup();
		return false;
	}

	auto SetState = [&](EGridDungeonTrapState Target)
	{
		State->DetectedTriggerTrapIds.Reset();
		State->TriggeredTriggerTrapIds.Reset();
		State->DisarmedTriggerTrapIds.Reset();
		State->DisabledTriggerTrapIds.Reset();
		if (Target != EGridDungeonTrapState::Hidden)
			State->DetectedTriggerTrapIds.Add(Trap.InstanceId);
		if (Target == EGridDungeonTrapState::Triggered)
			State->TriggeredTriggerTrapIds.Add(Trap.InstanceId);
		if (Target == EGridDungeonTrapState::Disarmed)
			State->DisarmedTriggerTrapIds.Add(Trap.InstanceId);
		if (Target == EGridDungeonTrapState::Disabled)
			State->DisabledTriggerTrapIds.Add(Trap.InstanceId);
	};
	auto CheckVisual = [&](AGridRuntimeObjectActor* Actor, bool bVisible, bool bDanger, UMaterialInterface* ExpectedOverlay)
	{
		if (!TestNotNull(TEXT("Checked visual"), Actor))
			return;
		const UStaticMeshComponent* Mesh = Actor->MeshComponent;
		TestEqual(TEXT("Visibility follows persistent state"), !Actor->IsHidden(), bVisible);
		TestEqual(TEXT("CustomDepth follows danger and settings"), Mesh->bRenderCustomDepth != 0, bDanger);
		TestEqual(TEXT("Stencil clears or follows definition"), Mesh->CustomDepthStencilValue, bDanger ? Def->DungeonTrap.DangerStencilValue : 0);
		TestTrue(TEXT("Overlay clears or follows definition"), Mesh->GetOverlayMaterial() == ExpectedOverlay);
		TestEqual(TEXT("Mouse collision follows visibility, not highlight"), Mesh->GetCollisionEnabled(),
			bVisible ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		TestEqual(TEXT("Revealed mouse ray is retained"), Mesh->GetCollisionResponseToChannel(ECC_Visibility), ECR_Block);
		TestEqual(TEXT("Status interaction follows revelation"), IGridInteractableInterface::Execute_CanInteract(Actor, Party, Actor->MeshComponent), bVisible);
	};

	Def->DungeonTrap.DangerStencilValue = 37;
	for (const bool bOneShot : { true, false })
	{
		Def->DungeonTrap.bOneShot = bOneShot;
		for (const bool bWithOverlay : { true, false })
		{
			Def->DungeonTrap.RevealedOverlayMaterial = bWithOverlay ? Overlay : nullptr;
			for (const EGridDungeonTrapState Target : { EGridDungeonTrapState::Hidden, EGridDungeonTrapState::Detected, EGridDungeonTrapState::Disarmed,
					 EGridDungeonTrapState::Disabled, EGridDungeonTrapState::Triggered })
			{
				AddInfo(FString::Printf(TEXT("State=%d oneShot=%d overlay=%d"), static_cast<int32>(Target), bOneShot, bWithOverlay));
				// Start every transition with an applied effect, so stale effects cannot pass.
				SetState(EGridDungeonTrapState::Detected);
				Runtime->RefreshDungeonTrapRevealVisual(Trap.InstanceId);
				if (Target == EGridDungeonTrapState::Triggered)
				{
					TestTrue(TEXT("Gameplay trigger refreshes the danger effect"), Runtime->TryRecordDungeonTrapTriggered(Trap.InstanceId, nullptr));
					CheckVisual(Visual, true, !bOneShot, !bOneShot && bWithOverlay ? Overlay : nullptr);
				}
				else
					SetState(Target);
				Runtime->RestoreDungeonTrapRevealVisuals();
				const bool bVisible = Target != EGridDungeonTrapState::Hidden;
				const bool bDanger = Target == EGridDungeonTrapState::Detected || (Target == EGridDungeonTrapState::Triggered && !bOneShot);
				UMaterialInterface* ExpectedOverlay = bDanger && bWithOverlay ? Overlay : nullptr;
				CheckVisual(Visual, bVisible, bDanger, ExpectedOverlay);

				UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
				Save->DungeonRuntimeState = Runtime->DungeonRuntimeState;
				TArray<uint8> Bytes;
				TestTrue(TEXT("Trap state serializes through UE SaveGame"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
				UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
				if (!TestNotNull(TEXT("Trap save reloads"), Loaded))
					continue;

				AGridLevelRuntimeActor* Restored = World->SpawnActor<AGridLevelRuntimeActor>();
				if (TestNotNull(TEXT("Restored runtime"), Restored))
				{
					Restored->LevelAsset = Level;
					Restored->WorldObjectDefinitions.Add(Def);
					Restored->DungeonRuntimeState = Loaded->DungeonRuntimeState;
					Restored->RestoreDungeonTrapRevealVisuals();
					Party->LevelRuntimeActor = Restored;
					AGridRuntimeObjectActor* RestoredVisual = Restored->FindRuntimeObjectActor<AGridRuntimeObjectActor>(Trap.InstanceId);
					TestEqual(TEXT("Snapshot restores logical state"), Restored->GetDungeonTrapState(Trap.InstanceId), Target);
					if (bVisible)
						CheckVisual(RestoredVisual, true, bDanger, ExpectedOverlay);
					else
						TestNull(TEXT("Restored hidden trap has no model"), RestoredVisual);
					Party->LevelRuntimeActor = Runtime;
				}
			}
		}
	}

	SetState(EGridDungeonTrapState::Detected);
	Def->DungeonTrap.RevealedOverlayMaterial = Overlay;
	Runtime->RefreshDungeonTrapRevealVisual(Trap.InstanceId);
	Def->DungeonTrap.bEnableDangerHighlight = false;
	Runtime->RefreshDungeonTrapRevealVisual(Trap.InstanceId);
	CheckVisual(Visual, true, false, nullptr);
	Def->DungeonTrap.bEnableDangerHighlight = true;
	Def->DungeonTrap.bUseDangerCustomDepth = false;
	Runtime->RefreshDungeonTrapRevealVisual(Trap.InstanceId);
	CheckVisual(Visual, true, false, Overlay);
	Def->DungeonTrap.RevealedOverlayMaterial = nullptr;
	Runtime->RefreshDungeonTrapRevealVisual(Trap.InstanceId);
	CheckVisual(Visual, true, false, nullptr);
	Def->DungeonTrap.bUseDangerCustomDepth = true;
	Runtime->RefreshDungeonTrapRevealVisual(Trap.InstanceId);
	CheckVisual(Visual, true, true, nullptr);
	for (const int32 InvalidStencil : { 0, 1, 2, 256 })
	{
		Def->DungeonTrap.DangerStencilValue = InvalidStencil;
		Runtime->RefreshDungeonTrapRevealVisual(Trap.InstanceId);
		TestEqual(TEXT("Stencil never takes editor-reserved values or exceeds eight bits"), Visual->MeshComponent->CustomDepthStencilValue,
			InvalidStencil < 3 ? 3 : 255);
		TestEqual(TEXT("Presentation settings never invalidate trap gameplay"), Runtime->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Detected);
	}

	Cleanup();
	return true;
}

#endif
