#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridProgressiveReceptacleActor.h"

namespace GridPUZZLE01Tests
{
	struct FPUZZLE01TestWorld
	{
		UWorld* World = nullptr;

		FPUZZLE01TestWorld()
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
				FName(*FString::Printf(TEXT("PUZZLE01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FPUZZLE01TestWorld()
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

	FGridRuntimeWorldObjectData MakeGuardianData(
		UGridItemDefinitionAsset* GemDefinition, UMaterialInterface* BlueMaterial)
	{
		FGridRuntimeWorldObjectData Data;
		Data.ObjectId = FGuid(1, 1, 1, 1);
		Data.Type = EGridLevelObjectType::Receptacle;
		Data.CellX = 0;
		Data.CellY = 0;
		Data.Edge = EGridEdge::North;

		FGridReceptacleBehaviorParams& Receptacle = Data.Behavior.Receptacle;
		Receptacle.bAcceptAnyItem = false;
		FGridReceptacleAcceptedItemConfig AcceptedGem;
		AcceptedGem.ItemDefinition = GemDefinition;
		Receptacle.AcceptedItems.Add(AcceptedGem);
		Receptacle.MaxContainedItems = 2;
		Receptacle.VisualPlacementMode = EGridReceptacleVisualPlacementMode::AttachedSocket;
		Receptacle.ProgressiveConsume.bEnabled = true;

		FGridReceptacleProgressMaterialStep LeftEye;
		LeftEye.MaterialSlotName = TEXT("Eye_Left");
		LeftEye.Material = BlueMaterial;
		Receptacle.ProgressiveConsume.MaterialSteps.Add(LeftEye);

		FGridReceptacleProgressMaterialStep RightEye;
		RightEye.MaterialSlotName = TEXT("Eye_Right");
		RightEye.Material = BlueMaterial;
		Receptacle.ProgressiveConsume.MaterialSteps.Add(RightEye);
		return Data;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPUZZLE01ProgressiveConsumableReceptacleTest,
	"Grimrock.PUZZLE01.ProgressiveConsumableReceptacle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPUZZLE01ProgressiveConsumableReceptacleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPUZZLE01Tests;

	FPUZZLE01TestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create PUZZLE01 test world."));
		return false;
	}

	UMaterial* EmptyLeftMaterial = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* EmptyRightMaterial = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* BlueMaterial = NewObject<UMaterial>(GetTransientPackage());
	UStaticMesh* GuardianMesh = NewObject<UStaticMesh>(GetTransientPackage());
	GuardianMesh->GetStaticMaterials().Add(FStaticMaterial(EmptyLeftMaterial, FName(TEXT("Eye_Left"))));
	GuardianMesh->GetStaticMaterials().Add(FStaticMaterial(EmptyRightMaterial, FName(TEXT("Eye_Right"))));

	UGridItemDefinitionAsset* BlueGem = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
	BlueGem->ItemDefinitionId = TEXT("Gem_Blue");
	BlueGem->DisplayName = FText::FromString(TEXT("Blue Gem"));

	AGridProgressiveReceptacleActor* Guardian = TestWorld.World->SpawnActor<AGridProgressiveReceptacleActor>();
	if (!Guardian)
	{
		AddError(TEXT("Unable to spawn PUZZLE01 progressive receptacle."));
		return false;
	}

	const FGridRuntimeWorldObjectData GuardianData = MakeGuardianData(BlueGem, BlueMaterial);
	Guardian->InitializeRuntimeWorldObject(GuardianData, GuardianMesh, FTransform::Identity);

	TestTrue(TEXT("Progressive consume mode resolves from receptacle behavior"), Guardian->IsProgressiveConsumeEnabled());
	TestFalse(TEXT("Progressive charges cannot be removed by the player"), Guardian->bCanRemoveItem);
	TestEqual(TEXT("Guardian starts with zero charges"), Guardian->GetProgressiveItemCount(), 0);
	TestEqual(TEXT("Guardian completion threshold is MaxContainedItems"), Guardian->GetProgressiveRequiredItemCount(), 2);
	TestTrue(TEXT("Left eye starts on its authored material"), Guardian->MeshComponent->GetMaterial(0) == EmptyLeftMaterial);
	TestTrue(TEXT("Right eye starts on its authored material"), Guardian->MeshComponent->GetMaterial(1) == EmptyRightMaterial);

	TestFalse(TEXT("Non-accepted item is rejected"), Guardian->TryInsertItem(TEXT("Gem_Red"), nullptr, nullptr));
	TestEqual(TEXT("Rejected item does not change progress"), Guardian->GetProgressiveItemCount(), 0);

	TestTrue(TEXT("First blue gem is accepted"), Guardian->TryInsertItem(BlueGem->ItemDefinitionId, BlueGem, nullptr));
	TestEqual(TEXT("First blue gem becomes one logical progress charge"), Guardian->GetProgressiveItemCount(), 1);
	TestFalse(TEXT("One charge does not complete a two-gem guardian"), Guardian->IsFull());
	TestTrue(TEXT("First progress step lights the left eye"), Guardian->MeshComponent->GetMaterial(0) == BlueMaterial);
	TestTrue(TEXT("Right eye remains unlit after the first gem"), Guardian->MeshComponent->GetMaterial(1) == EmptyRightMaterial);
	TestTrue(TEXT("Consumed first gem has no world item actor"), Guardian->ContainedItems[0].ItemActor == nullptr);

	TestTrue(TEXT("Second blue gem is accepted"), Guardian->TryInsertItem(BlueGem->ItemDefinitionId, BlueGem, nullptr));
	TestEqual(TEXT("Second blue gem reaches two logical progress charges"), Guardian->GetProgressiveItemCount(), 2);
	TestTrue(TEXT("Two charges complete the guardian"), Guardian->IsFull());
	TestTrue(TEXT("Left eye stays lit at completion"), Guardian->MeshComponent->GetMaterial(0) == BlueMaterial);
	TestTrue(TEXT("Second progress step lights the right eye"), Guardian->MeshComponent->GetMaterial(1) == BlueMaterial);
	TestTrue(TEXT("Consumed second gem has no world item actor"), Guardian->ContainedItems[1].ItemActor == nullptr);

	FGridRuntimeReceptacleState SavedState;
	Guardian->CaptureRuntimeReceptacleState(SavedState);
	TestEqual(TEXT("Existing receptacle persistence stores both progressive charges"), SavedState.ContainedItems.Num(), 2);
	TestEqual(TEXT("First persisted charge keeps canonical item identity"), SavedState.ContainedItems[0].ItemDefinitionId, FName(TEXT("Gem_Blue")));
	TestEqual(TEXT("Second persisted charge keeps canonical item identity"), SavedState.ContainedItems[1].ItemDefinitionId, FName(TEXT("Gem_Blue")));

	TestFalse(TEXT("Completed guardian rejects a third gem"), Guardian->TryInsertItem(BlueGem->ItemDefinitionId, BlueGem, nullptr));
	TestEqual(TEXT("Rejected third gem does not exceed threshold"), Guardian->GetProgressiveItemCount(), 2);

	Guardian->ForceClearRuntimeContents(false);
	TestEqual(TEXT("Runtime clear removes logical progress charges"), Guardian->GetProgressiveItemCount(), 0);
	TestTrue(TEXT("Runtime clear restores left eye baseline material"), Guardian->MeshComponent->GetMaterial(0) == EmptyLeftMaterial);
	TestTrue(TEXT("Runtime clear restores right eye baseline material"), Guardian->MeshComponent->GetMaterial(1) == EmptyRightMaterial);

	for (const FGridRuntimeItemState& SavedItem : SavedState.ContainedItems)
	{
		TestTrue(TEXT("Saved progressive charge restores through the existing receptacle item path"), Guardian->RestoreRuntimeContainedItem(SavedItem, nullptr));
	}

	TestEqual(TEXT("Restore rebuilds progressive charge count"), Guardian->GetProgressiveItemCount(), 2);
	TestFalse(TEXT("Restore keeps progressive charges non-removable"), Guardian->bCanRemoveItem);
	TestTrue(TEXT("Restore relights left eye"), Guardian->MeshComponent->GetMaterial(0) == BlueMaterial);
	TestTrue(TEXT("Restore relights right eye"), Guardian->MeshComponent->GetMaterial(1) == BlueMaterial);
	TestTrue(TEXT("Restored first charge remains presentation-only"), Guardian->ContainedItems[0].ItemActor == nullptr);
	TestTrue(TEXT("Restored second charge remains presentation-only"), Guardian->ContainedItems[1].ItemActor == nullptr);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS