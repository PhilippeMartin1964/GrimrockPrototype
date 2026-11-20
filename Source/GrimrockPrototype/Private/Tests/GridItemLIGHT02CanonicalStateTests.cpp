#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "GridCanonicalLootTestScope.h"
#include "../UI/GridInventoryItemMetadataReadModel.h"
#include "GameFramework/Actor.h"
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


}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemLIGHT02CanonicalItemLightStateTest,
	"Grimrock.Items.LIGHT02.CanonicalItemLightState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemLIGHT02CanonicalItemLightStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

    GridItemLIGHT02Tests::FTestWorld TestWorld;
    if (!TestNotNull(TEXT("Transient light state world"), TestWorld.World)) return false;

    FGridCanonicalLootTestScope RegistryScope;
    if (!TestTrue(TEXT("Canonical light fixture configured"),
        RegistryScope.AttachWithWeights(TestWorld.World, {}, [](UGridItemSystemAsset& Root)
        {
            for (bool bDefaultOn : {true, false})
            {
                auto* Def = NewObject<UGridNonArmamentDefinitionAsset>(&Root);
                Def->ItemId = bDefaultOn ? FName(TEXT("LIGHT02_DefaultOn")) : FName(TEXT("LIGHT02_DefaultOff"));
                Def->DisplayName = FText::FromName(Def->ItemId);
                Def->WeightKg = bDefaultOn ? 2.5f : 1.5f;
                Def->LightEmitter.bUsePointLight = true;
                Def->LightEmitter.bDefaultEnabled = bDefaultOn;
                Def->LightEmitter.bEnableLightFlicker = false;
                Def->LightEmitter.bEnableLightPositionFlicker = false;
                Def->LightEmitter.bEnableLightColorFlicker = false;
                Root.ItemDefinitions.Add(Def);
            }
        }))) return false;

    AActor* Owner = TestWorld.World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("World-scoped inventory owner"), Owner)) return false;
    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>(Owner);
    if (!TestNotNull(TEXT("World-scoped inventory"), Inventory)) return false;

    FGridItemInstance ExtinguishedItem;
    ExtinguishedItem.RuntimeObjectId = FGuid::NewGuid();
    ExtinguishedItem.ItemDefinitionId = TEXT("LIGHT02_DefaultOn");
    ExtinguishedItem.Quantity = 1;
    ExtinguishedItem.bLightsEnabled = false;
    TestTrue(TEXT("Canonical materialization accepts extinguished item"),
        Inventory->ApplyItemDefinitionToInstance(ExtinguishedItem));
    TestFalse(TEXT("A default-on definition does not relight a previously extinguished physical torch"),
        ExtinguishedItem.bLightsEnabled);

    FGridResolvedItem DefaultOn;
    if (!TestTrue(TEXT("Canonical default-on definition resolves"),
        RegistryScope.Registry->ResolveItem(ExtinguishedItem.ItemDefinitionId, 0, DefaultOn))) return false;
    const auto ExtinguishedMetadata =
        FGridInventoryItemMetadataReadModel::Resolve(RegistryScope.Registry, nullptr, ExtinguishedItem);
    TestTrue(TEXT("Extinguished item name and unit mass are projected canonically"),
        ExtinguishedMetadata.bResolved && ExtinguishedMetadata.DisplayName.EqualTo(DefaultOn.DisplayName) &&
        FMath::IsNearlyEqual(ExtinguishedMetadata.UnitWeightKg, 2.5f));
    TestTrue(TEXT("Source default-on light is present"), DefaultOn.LightEmitter.bDefaultEnabled);

    FGridItemInstance LitItem;
    LitItem.RuntimeObjectId = FGuid::NewGuid();
    LitItem.ItemDefinitionId = TEXT("LIGHT02_DefaultOff");
    LitItem.Quantity = 1;
    LitItem.bLightsEnabled = true;
    TestTrue(TEXT("Canonical materialization accepts lit item"),
        Inventory->ApplyItemDefinitionToInstance(LitItem));
    TestTrue(TEXT("A default-off definition does not extinguish an already lit physical torch"),
        LitItem.bLightsEnabled);

    FGridResolvedItem DefaultOff;
    if (!TestTrue(TEXT("Canonical default-off definition resolves"),
        RegistryScope.Registry->ResolveItem(LitItem.ItemDefinitionId, 0, DefaultOff))) return false;
    TestFalse(TEXT("Source default-off light remains disabled by default"),
        DefaultOff.LightEmitter.bDefaultEnabled);

    AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Level runtime actor is spawned"), Runtime)) return false;
    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
    Level->Width = 1;
    Level->Height = 1;
    Level->EnsureCellCount();
    Level->Cells[0].CellType = EGridCellType::Floor;
    Runtime->LevelAsset = Level;

    FGridResolvedItem AuthoredTorch;
    if (!TestTrue(TEXT("Real torch resolves at N0"),
        RegistryScope.Registry->ResolveItem(TEXT("Item_Torch"), 0, AuthoredTorch)) ||
        !TestTrue(TEXT("Real torch authored default is lit"), AuthoredTorch.LightEmitter.bDefaultEnabled))
        return false;
	FGridLooseItemInstance Placement;
	Placement.InstanceId = FGuid::NewGuid();
	Placement.CanonicalItemId = AuthoredTorch.ItemId;
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
