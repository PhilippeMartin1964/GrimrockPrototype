#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
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

	const UGridLevelAsset* ProductionLevel = LoadObject<UGridLevelAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"));
	TestNotNull(TEXT("Production level asset loads"), ProductionLevel);
	if (!ProductionLevel)
	{
		return false;
	}

	const FGridLuaScriptSource* ProductionGuardianScript = ProductionLevel->LuaScripts.FindByPredicate(
		[](const FGridLuaScriptSource& Candidate) { return Candidate.ScriptId == TEXT("puzzle1_lvl1"); });
	TestNotNull(TEXT("Production Guardian Lua script exists"), ProductionGuardianScript);
	if (!ProductionGuardianScript)
	{
		return false;
	}
	TestTrue(TEXT("Production Guardian Lua script is enabled"), ProductionGuardianScript->bEnabled);
	TestTrue(TEXT("Production Lua consumes accepted Guardian gems"), ProductionGuardianScript->Source.Contains(TEXT("ReceptacleConsumeItem")));
	TestTrue(TEXT("Production Lua disables Guardian insertion after completion"),
		ProductionGuardianScript->Source.Contains(TEXT("ReceptacleDisableInsertion")));

	const FGridObjectLink* ProductionBinding = ProductionLevel->Links.FindByPredicate(
		[ProductionGuardianScript](const FGridObjectLink& Link)
		{
			return Link.Command == EGridObjectCommand::LuaCallback && Link.SourceEvent == EGridObjectEvent::ItemInserted &&
				Link.LuaScriptId == ProductionGuardianScript->ScriptId && !Link.LuaCallbackName.IsNone();
		});
	TestNotNull(TEXT("Production Guardian ItemInserted Lua binding exists"), ProductionBinding);
	if (!ProductionBinding)
	{
		return false;
	}

	const FGridWorldObjectInstance* ProductionGuardianPlacement = ProductionLevel->WorldObjectInstances.FindByPredicate(
		[ProductionBinding](const FGridWorldObjectInstance& Placement) { return Placement.InstanceId == ProductionBinding->SourceObjectId; });
	TestNotNull(TEXT("Production Guardian binding source exists"), ProductionGuardianPlacement);
	if (!ProductionGuardianPlacement || ProductionGuardianPlacement->LogicId.IsNone())
	{
		AddError(TEXT("Production Guardian binding source must expose a LogicId."));
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
	Level->LevelVariables = ProductionLevel->LevelVariables;
	Level->LuaScripts.Add(*ProductionGuardianScript);
	Runtime->LevelAsset = Level;
	Runtime->CurrentDungeonLevelId = TEXT("PUZZLE01_CLEAN01");

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
	GuardianDefinition->DefinitionId = TEXT("Guardian_CLEAN01");
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
	GuardianPlacement.LogicId = ProductionGuardianPlacement->LogicId;
	GuardianPlacement.Type = EGridLevelObjectType::Receptacle;
	GuardianPlacement.WorldObjectDefinitionId = GuardianDefinition->DefinitionId;
	GuardianPlacement.CellX = 0;
	GuardianPlacement.CellY = 0;
	GuardianPlacement.WallSide = EGridEdge::North;
	Level->WorldObjectInstances.Add(GuardianPlacement);

	FGridObjectLink Binding;
	Binding.SourceObjectId = GuardianId;
	Binding.SourceEvent = ProductionBinding->SourceEvent;
	Binding.Command = ProductionBinding->Command;
	Binding.LuaScriptId = ProductionBinding->LuaScriptId;
	Binding.LuaCallbackName = ProductionBinding->LuaCallbackName;
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
		AddError(FString::Printf(TEXT("Production Guardian Lua runtime failed to load: %s"), *Error));
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
	TestTrue(TEXT("Wrong item reaches the cursor"), Party->PartyInventoryComponent->SetCursorItem(MakeItem(WrongItem->ItemDefinitionId, 1)));
	TestFalse(TEXT("Wrong item is rejected through the real mouse receptacle path"), Guardian->TryPlaceCursorItemFromHit(Party, GuardianHit));
	TestTrue(TEXT("Wrong item remains on the cursor"), Party->PartyInventoryComponent->HasCursorItem());
	TestEqual(TEXT("Wrong item creates no contained entry"), Guardian->GetContainedItemCount(), 0);
	Party->PartyInventoryComponent->ClearCursorItem();

	const FGridItemInstance GemStack = MakeItem(BlueGem->ItemDefinitionId, 3);
	TestTrue(TEXT("Three blue gems enter party inventory"), Party->PartyInventoryComponent->AddItemToCharacterInventory(0, GemStack));
	const int32 GemSlot = FindInventorySlot(Party->PartyInventoryComponent->PartyInventoryState, BlueGem->ItemDefinitionId);
	TestTrue(TEXT("Blue gem stack inventory slot exists"), GemSlot != INDEX_NONE);
	TestTrue(TEXT("Blue gem stack moves to cursor"), GemSlot != INDEX_NONE && Party->PartyInventoryComponent->TryTakeInventorySlotToCursor(0, GemSlot));

	TestTrue(TEXT("First blue gem executes the production Guardian script"), Guardian->TryPlaceCursorItemFromHit(Party, GuardianHit));
	TestTrue(TEXT("Cursor retains the two unconsumed stack units"), Party->PartyInventoryComponent->HasCursorItem());
	TestEqual(TEXT("First deposit decrements stack from three to two"), Party->PartyInventoryComponent->GetCursorItem().Quantity, 2);
	TestEqual(TEXT("Production Lua consumes the first inserted gem"), Guardian->GetContainedItemCount(), 0);
	TestTrue(TEXT("Production Lua lights EyesLeft after the first gem"), Guardian->MeshComponent->GetMaterial(0) == BlueMaterial);
	TestTrue(TEXT("EyesRight remains dark after the first gem"), Guardian->MeshComponent->GetMaterial(1) == EmptyRight);

	FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	int32 GemCount = 0;
	TestTrue(TEXT("GuardianGemCount is readable after first insertion"),
		State && GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("GuardianGemCount"), GemCount, Error));
	TestEqual(TEXT("First insertion commits persistent count 1"), GemCount, 1);

	TestTrue(TEXT("Second blue gem executes the same production script"), Guardian->TryPlaceCursorItemFromHit(Party, GuardianHit));
	TestEqual(TEXT("Second deposit decrements stack from two to one"), Party->PartyInventoryComponent->GetCursorItem().Quantity, 1);
	TestEqual(TEXT("Production Lua consumes the second inserted gem"), Guardian->GetContainedItemCount(), 0);
	TestTrue(TEXT("EyesLeft remains lit"), Guardian->MeshComponent->GetMaterial(0) == BlueMaterial);
	TestTrue(TEXT("Production Lua lights EyesRight after the second gem"), Guardian->MeshComponent->GetMaterial(1) == BlueMaterial);
	TestTrue(TEXT("GuardianGemCount is readable after second insertion"),
		GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("GuardianGemCount"), GemCount, Error));
	TestEqual(TEXT("Second insertion commits persistent count 2"), GemCount, 2);
	TestFalse(TEXT("Production Lua disables Guardian insertion after the second gem"), Guardian->bCanInsertItems);

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

	const FGridRuntimeObjectVisualState* VisualState = State ? State->ObjectVisuals.Find(GuardianId) : nullptr;
	TestNotNull(TEXT("Production Lua visual changes are persisted generically"), VisualState);
	if (VisualState)
	{
		TestEqual(TEXT("Both eye material overrides are persisted"), VisualState->MaterialAliasesBySlot.Num(), 2);
	}
	return true;
}

#endif
