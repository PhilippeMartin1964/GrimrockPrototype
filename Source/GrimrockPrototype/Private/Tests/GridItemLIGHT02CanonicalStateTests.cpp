#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace GridItemLIGHT02Tests
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

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ITEM_LIGHT02_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
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

	UGridItemDefinitionAsset* MakeLightDefinition(UObject* Outer, FName Id, bool bDefaultEnabled, float Weight)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = Id;
		Definition->DisplayName = FText::FromName(Id);
		Definition->Weight = Weight;
		Definition->WorldMesh = NewObject<UStaticMesh>(Definition);
		Definition->LightEmitter.bUsePointLight = true;
		Definition->LightEmitter.bDefaultEnabled = bDefaultEnabled;
		Definition->LightEmitter.bEnableLightFlicker = false;
		Definition->LightEmitter.bEnableLightPositionFlicker = false;
		Definition->LightEmitter.bEnableLightColorFlicker = false;
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemLIGHT02CanonicalItemLightStateTest,
	"Grimrock.Items.LIGHT02.CanonicalItemLightState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemLIGHT02CanonicalItemLightStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	UGridItemDefinitionAsset* DefaultOn = GridItemLIGHT02Tests::MakeLightDefinition(Inventory, TEXT("LIGHT02_DefaultOn"), true, 2.5f);
	UGridItemDefinitionAsset* DefaultOff = GridItemLIGHT02Tests::MakeLightDefinition(Inventory, TEXT("LIGHT02_DefaultOff"), false, 1.5f);
	TestTrue(TEXT("Default-on definition registers"), Inventory->RegisterItemDefinition(DefaultOn));
	TestTrue(TEXT("Default-off definition registers"), Inventory->RegisterItemDefinition(DefaultOff));

	FGridItemInstance ExtinguishedItem;
	ExtinguishedItem.RuntimeObjectId = FGuid::NewGuid();
	ExtinguishedItem.ItemDefinitionId = DefaultOn->ItemDefinitionId;
	ExtinguishedItem.Quantity = 1;
	ExtinguishedItem.bLightsEnabled = false;
	TestTrue(TEXT("Definition metadata applies to an extinguished runtime item"), Inventory->ApplyItemDefinitionToInstance(ExtinguishedItem));
	TestFalse(TEXT("A default-on definition does not relight an extinguished runtime item"), ExtinguishedItem.bLightsEnabled);
	TestTrue(TEXT("Definition metadata still updates weight"), FMath::IsNearlyEqual(ExtinguishedItem.Weight, DefaultOn->Weight));
	TestTrue(TEXT("Definition metadata still fills display name"), ExtinguishedItem.DisplayName.EqualTo(DefaultOn->DisplayName));

	FGridItemInstance LitItem;
	LitItem.RuntimeObjectId = FGuid::NewGuid();
	LitItem.ItemDefinitionId = DefaultOff->ItemDefinitionId;
	LitItem.Quantity = 1;
	LitItem.bLightsEnabled = true;
	TestTrue(TEXT("Definition metadata applies to a lit runtime item"), Inventory->ApplyItemDefinitionToInstance(LitItem));
	TestTrue(TEXT("A default-off definition does not extinguish a lit runtime item"), LitItem.bLightsEnabled);

	GridItemLIGHT02Tests::FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("ITEM-LIGHT02 could not create a transient game world."));
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("Level runtime actor is spawned"), Runtime);
	if (!Runtime)
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 1;
	Level->Height = 1;
	Level->EnsureCellCount();
	Level->Cells[0].CellType = EGridCellType::Floor;
	Runtime->LevelAsset = Level;

	UGridItemDefinitionAsset* AuthoredTorch = GridItemLIGHT02Tests::MakeLightDefinition(Runtime, TEXT("LIGHT02_AuthoredTorch"), true, 1.0f);
	FGridLooseItemInstance Placement;
	Placement.InstanceId = FGuid::NewGuid();
	Placement.ItemDefinition = AuthoredTorch;
	Placement.Quantity = 1;
	Placement.CellX = 0;
	Placement.CellY = 0;
	Placement.SurfaceSide = EGridEdge::None;
	Level->LooseItemInstances.Add(Placement);

	Runtime->RebuildRuntimeObjects();
	TestTrue(TEXT("Fresh authored world state can be captured"), Runtime->CaptureCurrentLevelRuntimeState());
	const FGridLevelRuntimeState* RuntimeState = Runtime->FindRuntimeStateForCurrentLevel();
	TestNotNull(TEXT("Fresh authored world state exists"), RuntimeState);
	const FGridRuntimeItemState* WorldItemState = RuntimeState ? RuntimeState->Items.Find(Placement.InstanceId) : nullptr;
	TestNotNull(TEXT("Authored loose item is present in runtime state"), WorldItemState);
	TestTrue(TEXT("A freshly authored default-on loose item starts lit"), WorldItemState && WorldItemState->bLightsEnabled);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
