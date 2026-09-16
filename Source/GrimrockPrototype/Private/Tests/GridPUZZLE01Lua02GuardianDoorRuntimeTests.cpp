#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "GridDoorTestUtils.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLevelVariableStore.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridPUZZLE01Lua02RuntimeTests
{
	constexpr float DoorMotionDuration = 0.10f;
	const FName ProductionScriptId(TEXT("puzzle1_lvl1"));
	const FName GuardianDoorLogicId(TEXT("GuardianDoor"));

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
				FName(*FString::Printf(TEXT("PUZZLE01_LUA02_Runtime_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

	FGridItemInstance MakeItem(FName ItemDefinitionId, int32 Quantity)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = ItemDefinitionId;
		Item.Quantity = Quantity;
		Item.OwnerType = EGridItemOwnerType::World;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPUZZLE01Lua02RuntimeGuardianDoorCompletionTest,
	"Grimrock.PUZZLE01.LUA02.RuntimeGuardianDoorCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPUZZLE01Lua02RuntimeGuardianDoorCompletionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPUZZLE01Lua02RuntimeTests;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("PUZZLE01-LUA02 runtime world exists"), TestWorld.World))
	{
		return false;
	}

	const UGridLevelAsset* ProductionLevel = LoadObject<UGridLevelAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"));
	if (!TestNotNull(TEXT("Production DA_GridLevel_00 loads"), ProductionLevel))
	{
		return false;
	}

	const FGridLuaScriptSource* ProductionScript = ProductionLevel->LuaScripts.FindByPredicate(
		[](const FGridLuaScriptSource& Candidate)
		{
			return Candidate.ScriptId == ProductionScriptId;
		});
	if (!TestNotNull(TEXT("Production puzzle1_lvl1 script exists"), ProductionScript))
	{
		return false;
	}
	TestTrue(TEXT("Production puzzle1_lvl1 script is enabled"), ProductionScript->bEnabled);

	const FGridObjectLink* ProductionBinding = ProductionLevel->Links.FindByPredicate(
		[](const FGridObjectLink& Link)
		{
			return Link.Command == EGridObjectCommand::LuaCallback && Link.SourceEvent == EGridObjectEvent::ItemInserted &&
				Link.LuaScriptId == ProductionScriptId && !Link.LuaCallbackName.IsNone();
		});
	if (!TestNotNull(TEXT("Production Guardian ItemInserted -> puzzle1_lvl1 binding exists"), ProductionBinding))
	{
		return false;
	}

	const FGridWorldObjectInstance* ProductionGuardian = ProductionLevel->WorldObjectInstances.FindByPredicate(
		[ProductionBinding](const FGridWorldObjectInstance& Placement)
		{
			return Placement.InstanceId == ProductionBinding->SourceObjectId;
		});
	if (!TestNotNull(TEXT("Production Guardian placement exists"), ProductionGuardian) || ProductionGuardian->LogicId.IsNone())
	{
		AddError(TEXT("Production Guardian placement must expose a LogicId."));
		return false;
	}

	TArray<FGuid> GuardianDoorIds;
	const int32 GuardianDoorCount = ProductionLevel->FindTypedPlacementIdsByLogicId(GuardianDoorLogicId, GuardianDoorIds);
	TestEqual(TEXT("Production level contains exactly one GuardianDoor"), GuardianDoorCount, 1);
	if (GuardianDoorCount != 1 || GuardianDoorIds.Num() != 1)
	{
		return false;
	}

	const FGridWorldObjectInstance* ProductionDoor = ProductionLevel->WorldObjectInstances.FindByPredicate(
		[DoorId = GuardianDoorIds[0]](const FGridWorldObjectInstance& Placement)
		{
			return Placement.InstanceId == DoorId;
		});
	if (!TestNotNull(TEXT("Production GuardianDoor placement exists"), ProductionDoor))
	{
		return false;
	}
	TestTrue(TEXT("Production GuardianDoor is a Door"), ProductionDoor->Type == EGridLevelObjectType::Door);

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime) || !TestNotNull(TEXT("Party pawn exists"), Party) ||
		!TestNotNull(TEXT("Party inventory exists"), Party ? Party->PartyInventoryComponent.Get() : nullptr))
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 2;
	Level->Height = 1;
	Level->CellSize = ProductionLevel->CellSize;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.NorthWall = EGridWallType::None;
		Cell.EastWall = EGridWallType::None;
		Cell.SouthWall = EGridWallType::None;
		Cell.WestWall = EGridWallType::None;
		Cell.bBlocksOccupancy = false;
	}
	Level->LevelVariables = ProductionLevel->LevelVariables;
	Level->LuaScripts.Add(*ProductionScript);
	Runtime->LevelAsset = Level;
	Runtime->CurrentDungeonLevelId = TEXT("PUZZLE01_LUA02");

	UMaterial* EmptyLeft = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* EmptyRight = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* BlueMaterial = NewObject<UMaterial>(GetTransientPackage());
	UStaticMesh* GuardianMesh = NewObject<UStaticMesh>(GetTransientPackage());
	GuardianMesh->GetStaticMaterials().Add(FStaticMaterial(EmptyLeft, FName(TEXT("EyesLeft"))));
	GuardianMesh->GetStaticMaterials().Add(FStaticMaterial(EmptyRight, FName(TEXT("EyesRight"))));

	UGridItemDefinitionAsset* BlueGem = NewObject<UGridItemDefinitionAsset>(Runtime);
	BlueGem->ItemDefinitionId = TEXT("Gem_Blue");
	BlueGem->DisplayName = FText::FromString(TEXT("Blue Gem"));
	BlueGem->bStackable = true;
	BlueGem->MaxStackSize = 10;

	UGridItemDefinitionAsset* WrongItem = NewObject<UGridItemDefinitionAsset>(Runtime);
	WrongItem->ItemDefinitionId = TEXT("Gem_Red");
	WrongItem->DisplayName = FText::FromString(TEXT("Red Gem"));

	UGridWorldObjectDefinitionAsset* GuardianDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	GuardianDefinition->DefinitionId = TEXT("Guardian_LUA02_Runtime");
	GuardianDefinition->SupportedType = EGridLevelObjectType::Receptacle;
	GuardianDefinition->PlacementSurface = EGridObjectPlacementKind::Wall;
	GuardianDefinition->StaticPart.Mesh = GuardianMesh;
	GuardianDefinition->RuntimeActorClass = AGridReceptacleActor::StaticClass();
	GuardianDefinition->bIsInteractable = true;
	GuardianDefinition->RuntimeMaterialAliases.Add(TEXT("BlueGem"), BlueMaterial);
	GuardianDefinition->DefaultBehavior.Receptacle.bAcceptAnyItem = false;
	GuardianDefinition->DefaultBehavior.Receptacle.MaxContainedItems = 1;
	FGridReceptacleAcceptedItemConfig AcceptedGem;
	AcceptedGem.ItemDefinition = BlueGem;
	GuardianDefinition->DefaultBehavior.Receptacle.AcceptedItems.Add(AcceptedGem);
	Runtime->WorldObjectDefinitions.Add(GuardianDefinition);

	FGridWorldObjectInstance GuardianPlacement;
	GuardianPlacement.InstanceId = ProductionGuardian->InstanceId;
	GuardianPlacement.LogicId = ProductionGuardian->LogicId;
	GuardianPlacement.Type = EGridLevelObjectType::Receptacle;
	GuardianPlacement.WorldObjectDefinitionId = GuardianDefinition->DefinitionId;
	GuardianPlacement.CellX = 0;
	GuardianPlacement.CellY = 0;
	GuardianPlacement.WallSide = EGridEdge::North;
	Level->WorldObjectInstances.Add(GuardianPlacement);

	FGridWorldObjectInstance DoorPlacement;
	DoorPlacement.InstanceId = ProductionDoor->InstanceId;
	DoorPlacement.LogicId = ProductionDoor->LogicId;
	DoorPlacement.Type = EGridLevelObjectType::Door;
	DoorPlacement.CellX = 0;
	DoorPlacement.CellY = 0;
	DoorPlacement.WallSide = EGridEdge::East;
	DoorPlacement.InstanceConfig.bDoorInitiallyOpen = false;
	Level->WorldObjectInstances.Add(DoorPlacement);

	FGridObjectLink Binding = *ProductionBinding;
	Binding.SourceObjectId = GuardianPlacement.InstanceId;
	Level->Links.Add(Binding);

	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	UGridDoorSystemComponent* DoorSystem = Runtime->FindComponentByClass<UGridDoorSystemComponent>();
	if (!TestNotNull(TEXT("Activation component exists"), Activation) || !TestNotNull(TEXT("Door system exists"), DoorSystem))
	{
		return false;
	}
	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();
	FString Error;
	if (!Activation->ReloadLuaRuntime(&Error))
	{
		AddError(FString::Printf(TEXT("Production Guardian Lua runtime failed to load: %s"), *Error));
		return false;
	}

	DoorSystem->Initialize(Runtime);
	DoorSystem->RebuildIndexes();
	AGridDoorActor* DoorActor = TestWorld.World->SpawnActor<AGridDoorActor>();
	if (!TestNotNull(TEXT("GuardianDoor runtime actor exists"), DoorActor))
	{
		return false;
	}
	DoorActor->bNativeDoorAudioPlaybackEnabled = false;
	GridDoorTestUtils::InitializeDoorFromMotion(DoorActor, DoorPlacement, TestWorld.World, DoorMotionDuration, 180.0f);
	DoorSystem->RegisterDoorObject(FGridRuntimeWorldObjectData(DoorPlacement), DoorActor);
	TestTrue(TEXT("GuardianDoor starts fully closed"), DoorActor->IsFullyClosed());

	Runtime->AddRuntimeObjectActor(GuardianPlacement);
	AGridReceptacleActor* Guardian = Runtime->FindRuntimeObjectActor<AGridReceptacleActor>(GuardianPlacement.InstanceId);
	if (!TestNotNull(TEXT("Guardian runtime receptacle exists"), Guardian))
	{
		return false;
	}

	Party->SetGridStart(Runtime, 0, 0, EGridEdge::North);
	Party->PartyInventoryComponent->InitializeDefaultPartyIfNeeded();
	TestTrue(TEXT("Blue gem definition registers"), Party->PartyInventoryComponent->RegisterItemDefinition(BlueGem));
	TestTrue(TEXT("Wrong item definition registers"), Party->PartyInventoryComponent->RegisterItemDefinition(WrongItem));

	AddExpectedError(TEXT("Receptacle cursor insert refused"), EAutomationExpectedErrorFlags::Contains, 2);
	AddExpectedError(TEXT("GridInventory Cursor Place ToReceptacle Failed"), EAutomationExpectedErrorFlags::Contains, 2);
	AddExpectedError(TEXT("cyclic link dispatch"), EAutomationExpectedErrorFlags::Contains, 2);

	FHitResult GuardianHit;
	GuardianHit.Component = Guardian->MeshComponent;

	TestTrue(TEXT("Wrong item reaches the cursor"), Party->PartyInventoryComponent->SetCursorItem(MakeItem(WrongItem->ItemDefinitionId, 1)));
	TestFalse(TEXT("Wrong item is rejected by the Guardian receptacle"), Guardian->TryPlaceCursorItemFromHit(Party, GuardianHit));
	TestTrue(TEXT("Wrong item remains on the cursor"), Party->PartyInventoryComponent->HasCursorItem());
	TestEqual(TEXT("Wrong item creates no contained entry"), Guardian->GetContainedItemCount(), 0);
	Party->PartyInventoryComponent->ClearCursorItem();

	TestTrue(TEXT("Three blue gems are placed on the cursor"),
		Party->PartyInventoryComponent->SetCursorItem(MakeItem(BlueGem->ItemDefinitionId, 3)));

	TestTrue(TEXT("First blue gem executes production puzzle1_lvl1"), Guardian->TryPlaceCursorItemFromHit(Party, GuardianHit));
	TestTrue(TEXT("Cursor retains two unconsumed blue gems"), Party->PartyInventoryComponent->HasCursorItem());
	TestEqual(TEXT("Cursor quantity is two after first gem"), Party->PartyInventoryComponent->GetCursorItem().Quantity, 2);
	TestEqual(TEXT("Production Lua consumes the first inserted gem"), Guardian->GetContainedItemCount(), 0);
	TestTrue(TEXT("Production Lua lights EyesLeft after the first gem"), Guardian->MeshComponent->GetMaterial(0) == BlueMaterial);
	TestTrue(TEXT("EyesRight remains dark after the first gem"), Guardian->MeshComponent->GetMaterial(1) == EmptyRight);
	TestTrue(TEXT("GuardianDoor remains fully closed after first gem"), DoorActor->IsFullyClosed());
	TestFalse(TEXT("First gem does not start GuardianDoor animation"), DoorActor->IsAnimating());

	FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	int32 GemCount = 0;
	TestTrue(TEXT("GuardianGemCount reads after first gem"),
		State && GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("GuardianGemCount"), GemCount, Error));
	TestEqual(TEXT("First gem commits GuardianGemCount=1"), GemCount, 1);

	TestTrue(TEXT("Second blue gem executes production puzzle1_lvl1"), Guardian->TryPlaceCursorItemFromHit(Party, GuardianHit));
	TestTrue(TEXT("Cursor retains the third unconsumed blue gem"), Party->PartyInventoryComponent->HasCursorItem());
	TestEqual(TEXT("Cursor quantity is one after second gem"), Party->PartyInventoryComponent->GetCursorItem().Quantity, 1);
	TestEqual(TEXT("Production Lua consumes the second inserted gem"), Guardian->GetContainedItemCount(), 0);
	TestTrue(TEXT("EyesLeft remains lit after the second gem"), Guardian->MeshComponent->GetMaterial(0) == BlueMaterial);
	TestTrue(TEXT("Production Lua lights EyesRight after the second gem"), Guardian->MeshComponent->GetMaterial(1) == BlueMaterial);
	TestFalse(TEXT("Production Lua disables further Guardian insertion"), Guardian->bCanInsertItems);
	TestTrue(TEXT("GuardianGemCount reads after second gem"),
		GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("GuardianGemCount"), GemCount, Error));
	TestEqual(TEXT("Second gem commits GuardianGemCount=2"), GemCount, 2);

	// bIsOpen is an endpoint state. The second gem must start the physical opening,
	// while passage remains blocked until the animation reaches its endpoint.
	TestFalse(TEXT("GuardianDoor endpoint open state remains false while opening"), DoorActor->bIsOpen);
	TestTrue(TEXT("Second gem starts GuardianDoor physical opening"), DoorActor->IsAnimating());
	TestFalse(TEXT("GuardianDoor passage remains closed while animation is running"),
		Runtime->IsDoorOpenOnEdge(DoorPlacement.CellX, DoorPlacement.CellY, DoorPlacement.WallSide));

	const UMaterialInterface* LeftAfterTwo = Guardian->MeshComponent->GetMaterial(0);
	const UMaterialInterface* RightAfterTwo = Guardian->MeshComponent->GetMaterial(1);
	TestFalse(TEXT("Third gem is rejected before cursor transfer"), Guardian->TryPlaceCursorItemFromHit(Party, GuardianHit));
	TestTrue(TEXT("Third gem stays on cursor"), Party->PartyInventoryComponent->HasCursorItem());
	TestEqual(TEXT("Rejected third gem keeps stack quantity one"), Party->PartyInventoryComponent->GetCursorItem().Quantity, 1);
	TestEqual(TEXT("Third attempt creates no contained item"), Guardian->GetContainedItemCount(), 0);
	TestTrue(TEXT("Third attempt leaves left material unchanged"), Guardian->MeshComponent->GetMaterial(0) == LeftAfterTwo);
	TestTrue(TEXT("Third attempt leaves right material unchanged"), Guardian->MeshComponent->GetMaterial(1) == RightAfterTwo);
	TestTrue(TEXT("GuardianGemCount remains readable after third attempt"),
		GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("GuardianGemCount"), GemCount, Error));
	TestEqual(TEXT("GuardianGemCount never exceeds two"), GemCount, 2);

	const FGridRuntimeObjectVisualState* VisualState = State ? State->ObjectVisuals.Find(GuardianPlacement.InstanceId) : nullptr;
	TestNotNull(TEXT("Production Lua visual changes are persisted generically"), VisualState);
	if (VisualState)
	{
		TestEqual(TEXT("Both eye material overrides are persisted"), VisualState->MaterialAliasesBySlot.Num(), 2);
	}

	DoorActor->Tick(DoorMotionDuration + 0.01f);
	TestTrue(TEXT("GuardianDoor commits endpoint open state after animation"), DoorActor->bIsOpen);
	TestTrue(TEXT("GuardianDoor reaches fully open state"), DoorActor->IsFullyOpen());
	TestTrue(TEXT("GuardianDoor passage opens after animation completion"),
		Runtime->IsDoorOpenOnEdge(DoorPlacement.CellX, DoorPlacement.CellY, DoorPlacement.WallSide));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
