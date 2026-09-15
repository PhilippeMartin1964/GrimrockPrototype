#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GridLuaScriptTypes.h"
#include "Materials/Material.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLevelVariableStore.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
	struct FPUZZLE01Lua01IntegrationWorld
	{
		UWorld* World = nullptr;

		FPUZZLE01Lua01IntegrationWorld()
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
				FName(*FString::Printf(TEXT("PUZZLE01_LUA01_Integration_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FPUZZLE01Lua01IntegrationWorld()
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

	FGridLevelVariableDefinition MakeIntVariable(FName Id, int32 DefaultValue)
	{
		FGridLevelVariableDefinition Variable;
		Variable.VariableId = Id;
		Variable.Type = EGridLevelVariableType::Int32;
		Variable.DefaultInt32Value = DefaultValue;
		return Variable;
	}

	FGridLevelVariableDefinition MakeBoolVariable(FName Id, bool bDefaultValue)
	{
		FGridLevelVariableDefinition Variable;
		Variable.VariableId = Id;
		Variable.Type = EGridLevelVariableType::Bool;
		Variable.bDefaultBoolValue = bDefaultValue;
		return Variable;
	}

	int32 FindInventorySlot(const FGridPartyInventoryState& State, FName ItemDefinitionId)
	{
		if (!State.ActiveCharacters.IsValidIndex(0))
		{
			return INDEX_NONE;
		}
		const TArray<FGridInventorySlot>& Slots = State.ActiveCharacters[0].InventorySlots;
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (!Slots[Index].IsEmpty() && Slots[Index].Item.ItemDefinitionId == ItemDefinitionId)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPUZZLE01Lua01GuardianPuzzleIntegrationTest,
	"Grimrock.PUZZLE01.LUA01.GuardianPuzzleIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPUZZLE01Lua01GuardianPuzzleIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FPUZZLE01Lua01IntegrationWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create integration world."));
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!Runtime || !Party || !Party->PartyInventoryComponent)
	{
		AddError(TEXT("Unable to spawn level runtime actor and party pawn."));
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 1;
	Level->Height = 1;
	Level->EnsureCellCount();
	Level->Cells[0].CellType = EGridCellType::Floor;
	Level->LevelVariables.Add(MakeIntVariable(TEXT("GuardianGemCount"), 0));
	Level->LevelVariables.Add(MakeIntVariable(TEXT("GuardianInsertionCallbacks"), 0));
	Level->LevelVariables.Add(MakeBoolVariable(TEXT("GuardianComplete"), false));
	Runtime->LevelAsset = Level;
	Runtime->CurrentDungeonLevelId = TEXT("PUZZLE01_LUA01");

	const UGridLevelAsset* ProductionLevel = LoadObject<UGridLevelAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"));
	TestNotNull(TEXT("Production level asset loads"), ProductionLevel);
	const FGridLuaScriptSource* ProductionGuardianScript = ProductionLevel
		? ProductionLevel->LuaScripts.FindByPredicate(
			  [](const FGridLuaScriptSource& Candidate) { return Candidate.ScriptId == TEXT("puzzle1_lvl1"); })
		: nullptr;
	TestNotNull(TEXT("Production Guardian Lua script exists"), ProductionGuardianScript);
	if (!ProductionGuardianScript)
	{
		return false;
	}
	TestTrue(TEXT("Production Lua consumes accepted Guardian gems"),
		ProductionGuardianScript->Source.Contains(TEXT("ReceptacleConsumeItem")));
	TestTrue(TEXT("Production Lua disables Guardian insertion after completion"),
		ProductionGuardianScript->Source.Contains(TEXT("ReceptacleDisableInsertion")));

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
	GuardianDefinition->DefinitionId = TEXT("Guardian");
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

	const FGuid GuardianId(1, 2, 3, 4);
	FGridWorldObjectInstance GuardianPlacement;
	GuardianPlacement.InstanceId = GuardianId;
	GuardianPlacement.LogicId = TEXT("Guardian");
	GuardianPlacement.Type = EGridLevelObjectType::Receptacle;
	GuardianPlacement.WorldObjectDefinitionId = GuardianDefinition->DefinitionId;
	GuardianPlacement.CellX = 0;
	GuardianPlacement.CellY = 0;
	GuardianPlacement.WallSide = EGridEdge::North;
	Level->WorldObjectInstances.Add(GuardianPlacement);

	FGridLuaScriptSource Script;
	Script.ScriptId = TEXT("GuardianGemDoor");
	Script.bEnabled = true;
	Script.Source =
		TEXT("persistent = { GuardianGemCount = 0, GuardianInsertionCallbacks = 0, GuardianComplete = false }\n")
		TEXT("local function must(ok, err) assert(ok, err) end\n")
		TEXT("function on_gem_inserted(event)\n")
		TEXT("  if persistent.GuardianGemCount >= 2 then return end\n")
		TEXT("  persistent.GuardianInsertionCallbacks = persistent.GuardianInsertionCallbacks + 1\n")
		TEXT("  must(grid.command('Guardian', 'ReceptacleConsumeItem'))\n")
		TEXT("  must(grid.command('Guardian', 'ReceptacleDisableRemoval'))\n")
		TEXT("  local next_count = persistent.GuardianGemCount + 1\n")
		TEXT("  if next_count == 1 then\n")
		TEXT("    must(grid.visual.set_material('Guardian', 'EyesLeft', 'BlueGem'))\n")
		TEXT("  else\n")
		TEXT("    must(grid.visual.set_material('Guardian', 'EyesRight', 'BlueGem'))\n")
		TEXT("    persistent.GuardianComplete = true\n")
		TEXT("    must(grid.command('Guardian', 'ReceptacleDisableInsertion'))\n")
		TEXT("  end\n")
		TEXT("  persistent.GuardianGemCount = next_count\n")
		TEXT("end\n");
	Level->LuaScripts.Add(Script);

	FGridObjectLink Binding;
	Binding.SourceObjectId = GuardianId;
	Binding.SourceEvent = EGridObjectEvent::ItemInserted;
	Binding.Command = EGridObjectCommand::LuaCallback;
	Binding.LuaScriptId = Script.ScriptId;
	Binding.LuaCallbackName = TEXT("on_gem_inserted");
	Level->Links.Add(Binding);

	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	if (!Activation)
	{
		AddError(TEXT("Activation component is missing."));
		return false;
	}
	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();
	FString Error;
	if (!Activation->ReloadLuaRuntime(&Error))
	{
		AddError(FString::Printf(TEXT("Lua runtime failed to load: %s"), *Error));
		return false;
	}

	Runtime->AddRuntimeObjectActor(GuardianPlacement);
	AGridReceptacleActor* Guardian = Runtime->FindRuntimeObjectActor<AGridReceptacleActor>(GuardianId);
	if (!Guardian)
	{
		AddError(TEXT("Guardian receptacle failed to spawn/register."));
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
	AGridReceptacleActor* GuardianBasePointer = Guardian;
	TestTrue(TEXT("Wrong item reaches the cursor"), Party->PartyInventoryComponent->SetCursorItem(MakeItem(WrongItem->ItemDefinitionId, 1)));
	TestFalse(TEXT("Wrong item is rejected through the real mouse receptacle path"), GuardianBasePointer->TryPlaceCursorItemFromHit(Party, GuardianHit));
	TestTrue(TEXT("Wrong item remains on the cursor"), Party->PartyInventoryComponent->HasCursorItem());
	TestEqual(TEXT("Wrong item creates no contained entry"), Guardian->GetContainedItemCount(), 0);
	Party->PartyInventoryComponent->ClearCursorItem();

	const FGuid OrdinaryId(5, 6, 7, 8);
	UGridWorldObjectDefinitionAsset* OrdinaryDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	OrdinaryDefinition->DefinitionId = TEXT("OrdinaryReceptacle");
	OrdinaryDefinition->SupportedType = EGridLevelObjectType::Receptacle;
	OrdinaryDefinition->PlacementSurface = EGridObjectPlacementKind::Wall;
	OrdinaryDefinition->StaticPart.Mesh = NewObject<UStaticMesh>(Runtime);
	OrdinaryDefinition->RuntimeActorClass = AGridReceptacleActor::StaticClass();
	OrdinaryDefinition->bIsInteractable = true;
	OrdinaryDefinition->DefaultBehavior.Receptacle.bAcceptAnyItem = true;
	Runtime->WorldObjectDefinitions.Add(OrdinaryDefinition);
	FGridWorldObjectInstance OrdinaryPlacement;
	OrdinaryPlacement.InstanceId = OrdinaryId;
	OrdinaryPlacement.Type = EGridLevelObjectType::Receptacle;
	OrdinaryPlacement.WorldObjectDefinitionId = OrdinaryDefinition->DefinitionId;
	OrdinaryPlacement.CellX = 0;
	OrdinaryPlacement.CellY = 0;
	OrdinaryPlacement.WallSide = EGridEdge::North;
	Level->WorldObjectInstances.Add(OrdinaryPlacement);
	Runtime->AddRuntimeObjectActor(OrdinaryPlacement);
	Activation->RebuildIndexes();
	AGridReceptacleActor* Ordinary = Runtime->FindRuntimeObjectActor<AGridReceptacleActor>(OrdinaryId);
	TestNotNull(TEXT("Ordinary receptacle fixture spawns"), Ordinary);
	if (!Ordinary)
	{
		return false;
	}
	TestTrue(TEXT("Ordinary item reaches cursor"), Party->PartyInventoryComponent->SetCursorItem(MakeItem(WrongItem->ItemDefinitionId, 1)));
	FHitResult OrdinaryHit;
	OrdinaryHit.Component = Ordinary->MeshComponent;
	TestTrue(TEXT("Ordinary receptacle still accepts through canonical cursor transfer"), Ordinary->TryPlaceCursorItemFromHit(Party, OrdinaryHit));
	TestFalse(TEXT("Ordinary deposit clears cursor"), Party->PartyInventoryComponent->HasCursorItem());
	TestEqual(TEXT("Ordinary receptacle keeps its item"), Ordinary->GetContainedItemCount(), 1);
	FName TakenItemId;
	TestTrue(TEXT("Ordinary receptacle item remains removable"), Ordinary->TryTakeFirstItem(Party, TakenItemId));
	TestEqual(TEXT("Ordinary receptacle returns the deposited item"), TakenItemId, WrongItem->ItemDefinitionId);

	const FGridItemInstance GemStack = MakeItem(BlueGem->ItemDefinitionId, 3);
	TestTrue(TEXT("Three blue gems enter party inventory"), Party->PartyInventoryComponent->AddItemToCharacterInventory(0, GemStack));
	const int32 GemSlot = FindInventorySlot(Party->PartyInventoryComponent->PartyInventoryState, BlueGem->ItemDefinitionId);
	TestTrue(TEXT("Blue gem stack inventory slot exists"), GemSlot != INDEX_NONE);
	TestTrue(TEXT("Blue gem stack moves to cursor"), GemSlot != INDEX_NONE && Party->PartyInventoryComponent->TryTakeInventorySlotToCursor(0, GemSlot));

	TestTrue(TEXT("First blue gem uses real cursor-to-receptacle path"), GuardianBasePointer->TryPlaceCursorItemFromHit(Party, GuardianHit));
	TestTrue(TEXT("Cursor retains the two unconsumed stack units"), Party->PartyInventoryComponent->HasCursorItem());
	TestEqual(TEXT("First deposit decrements stack from three to two"), Party->PartyInventoryComponent->GetCursorItem().Quantity, 2);
	TestEqual(TEXT("Lua consumes the first inserted gem"), Guardian->GetContainedItemCount(), 0);
	TestTrue(TEXT("Lua lights EyesLeft after the first gem"), Guardian->MeshComponent->GetMaterial(0) == BlueMaterial);
	TestTrue(TEXT("EyesRight remains dark after the first gem"), Guardian->MeshComponent->GetMaterial(1) == EmptyRight);

	FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	int32 GemCount = 0;
	bool bComplete = false;
	TestTrue(TEXT("GuardianGemCount is readable after first insertion"),
		State && GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("GuardianGemCount"), GemCount, Error));
	TestEqual(TEXT("First insertion commits persistent count 1"), GemCount, 1);
	TestTrue(TEXT("GuardianComplete is readable after first insertion"),
		State && GridLevelVariableStore::TryGetBool(*Level, *State, TEXT("GuardianComplete"), bComplete, Error));
	TestFalse(TEXT("Guardian is not complete after one gem"), bComplete);

	TestTrue(TEXT("Second blue gem uses the same real cursor transfer"), GuardianBasePointer->TryPlaceCursorItemFromHit(Party, GuardianHit));
	TestEqual(TEXT("Second deposit decrements stack from two to one"), Party->PartyInventoryComponent->GetCursorItem().Quantity, 1);
	TestEqual(TEXT("Lua consumes the second inserted gem"), Guardian->GetContainedItemCount(), 0);
	TestTrue(TEXT("EyesLeft remains lit"), Guardian->MeshComponent->GetMaterial(0) == BlueMaterial);
	TestTrue(TEXT("Lua lights EyesRight after the second gem"), Guardian->MeshComponent->GetMaterial(1) == BlueMaterial);

	TestTrue(TEXT("GuardianGemCount is readable after second insertion"),
		GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("GuardianGemCount"), GemCount, Error));
	TestEqual(TEXT("Second insertion commits persistent count 2"), GemCount, 2);
	TestTrue(TEXT("GuardianComplete is readable after second insertion"),
		GridLevelVariableStore::TryGetBool(*Level, *State, TEXT("GuardianComplete"), bComplete, Error));
	TestTrue(TEXT("Guardian is complete after two gems"), bComplete);
	TestFalse(TEXT("Lua disables Guardian insertion after the second gem"), Guardian->bCanInsertItems);

	const UMaterialInterface* LeftAfterTwo = Guardian->MeshComponent->GetMaterial(0);
	const UMaterialInterface* RightAfterTwo = Guardian->MeshComponent->GetMaterial(1);
	TestFalse(TEXT("Third gem is rejected before cursor transfer"), GuardianBasePointer->TryPlaceCursorItemFromHit(Party, GuardianHit));
	TestTrue(TEXT("Third gem stays on cursor"), Party->PartyInventoryComponent->HasCursorItem());
	TestEqual(TEXT("Rejected third gem keeps stack quantity one"), Party->PartyInventoryComponent->GetCursorItem().Quantity, 1);
	TestEqual(TEXT("Third attempt creates no contained item"), Guardian->GetContainedItemCount(), 0);
	TestTrue(TEXT("Third attempt leaves left material unchanged"), Guardian->MeshComponent->GetMaterial(0) == LeftAfterTwo);
	TestTrue(TEXT("Third attempt leaves right material unchanged"), Guardian->MeshComponent->GetMaterial(1) == RightAfterTwo);
	TestTrue(TEXT("GuardianGemCount remains readable after third attempt"),
		GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("GuardianGemCount"), GemCount, Error));
	TestEqual(TEXT("GuardianGemCount never exceeds two"), GemCount, 2);
	int32 CallbackCount = 0;
	TestTrue(TEXT("Guardian callback count is readable"),
		GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("GuardianInsertionCallbacks"), CallbackCount, Error));
	TestEqual(TEXT("Third attempt emits no ItemInserted callback"), CallbackCount, 2);

	const FGridRuntimeObjectVisualState* VisualState = State ? State->ObjectVisuals.Find(GuardianId) : nullptr;
	TestNotNull(TEXT("Lua visual changes are persisted generically"), VisualState);
	if (VisualState)
	{
		TestEqual(TEXT("Both eye material overrides are persisted"), VisualState->MaterialAliasesBySlot.Num(), 2);
	}
	return true;
}

#endif
