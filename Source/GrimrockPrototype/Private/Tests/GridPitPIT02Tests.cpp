#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalLootTestScope.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridPIT02
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
				FName(*FString::Printf(TEXT("PIT02TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
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

	UGridLevelAsset* MakeFloor(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 4;
		Level->Height = 4;
		Level->CellSize = 200.0f;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}

	FGridWorldObjectInstance MakePit(int32 X, int32 Y, FName TargetLevelId)
	{
		FGridWorldObjectInstance Pit;
		Pit.InstanceId = FGuid::NewGuid();
		Pit.Type = EGridLevelObjectType::Pit;
		Pit.WorldObjectDefinitionId = TEXT("Pit_Stone_01");
		Pit.CellX = X;
		Pit.CellY = Y;
		Pit.WallSide = EGridEdge::None;
		Pit.InstanceConfig.Pit.bInitiallyOpen = true;
		Pit.InstanceConfig.Pit.bUseSameCellCoordinates = true;
		Pit.InstanceConfig.Relocation.TargetLevelId = TargetLevelId;
		Pit.InstanceConfig.Relocation.TargetFacing = EGridEdge::North;
		return Pit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPIT02WorldItemsTest, "Grimrock.Pit.PIT02.WorldItemsFallingThroughPit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPIT02WorldItemsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPIT02;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("PIT02 world exists"), TestWorld.World))
	{
		return false;
	}

	FGridCanonicalLootTestScope RegistryScope;
	if (!TestTrue(TEXT("Real canonical ItemSystem attached"), RegistryScope.Attach(TestWorld.World)))
		return false;
	FGridResolvedItem CanonicalStone;
	if (!TestTrue(TEXT("Real N0 stone resolves"), RegistryScope.Registry->ResolveItem(TEXT("Stone"), 0, CanonicalStone)))
		return false;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("Runtime exists"), Runtime) || !TestNotNull(TEXT("Party exists"), Party))
	{
		return false;
	}

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
	UGridLevelAsset* Upper = MakeFloor(Dungeon);
	UGridLevelAsset* Lower = MakeFloor(Dungeon);
	const FName UpperId(TEXT("PIT02_Upper"));
	const FName LowerId(TEXT("PIT02_Lower"));

	FGridDungeonLevelEntry UpperEntry;
	UpperEntry.LevelId = UpperId;
	UpperEntry.DisplayName = FText::FromString(TEXT("Upper"));
	UpperEntry.LevelAsset = Upper;
	UpperEntry.LogicalPosition = FIntVector(0, 0, 1);
	UpperEntry.bEnabled = true;

	FGridDungeonLevelEntry LowerEntry;
	LowerEntry.LevelId = LowerId;
	LowerEntry.DisplayName = FText::FromString(TEXT("Lower"));
	LowerEntry.LevelAsset = Lower;
	LowerEntry.LogicalPosition = FIntVector(0, 0, 0);
	LowerEntry.bEnabled = true;

	Dungeon->DefaultLevelId = UpperId;
	Dungeon->Levels = { UpperEntry, LowerEntry };
	Upper->WorldObjectInstances.Add(MakePit(2, 2, LowerId));
	Lower->GetCellMutable(2, 2).CellType = EGridCellType::Empty;
	Lower->GetCellMutable(2, 2).bBlocksOccupancy = false;

	Runtime->DungeonAsset = Dungeon;
	Runtime->CurrentDungeonLevelId = UpperId;
	Runtime->LevelAsset = Upper;
	Party->SetGridStart(Runtime, 1, 2, EGridEdge::East);

	FGridItemInstance Stone;
	Stone.RuntimeObjectId = FGuid::NewGuid();
	Stone.ItemDefinitionId = TEXT("Stone");
	Stone.ItemLevel = 0;
	Stone.OwnerType = EGridItemOwnerType::World;
	Stone.Quantity = 1;
	Stone.bLightsEnabled = false;

	const FVector SourceLocalOffset(24.0f, -16.0f, 0.0f);
	AddExpectedError(TEXT("GridPit ItemTransfer landing fallback"), EAutomationExpectedErrorFlags::Contains, 1);
	TestTrue(TEXT("Dropping a normal World Item on an open pit routes it away from the source level"),
		Runtime->TryDropItemInstanceAtCell(Stone, nullptr, 2, 2, EGridEdge::None, SourceLocalOffset));
	TestEqual(TEXT("No World Item remains on the upper pit cell"), Runtime->GetWorldItemWeightAtCell(2, 2, false), 0.0f);

	const FGridLevelRuntimeState* LowerState = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
	if (!TestNotNull(TEXT("Lower level received a runtime state container"), LowerState))
	{
		return false;
	}
	TestFalse(TEXT("Queuing a falling item does not mark the lower level as visited"), LowerState->bHasBeenVisited);
	TestTrue(TEXT("The falling item is queued by its persistent RuntimeObjectId"), LowerState->PendingInboundItems.Contains(Stone.RuntimeObjectId));

	const FGridPendingWorldItemState* Pending = LowerState->PendingInboundItems.Find(Stone.RuntimeObjectId);
	if (!TestNotNull(TEXT("Pending item data exists"), Pending))
	{
		return false;
	}
	TestEqual(TEXT("Pending item resolves nearest usable destination X"), Pending->ItemState.CellX, 2);
	TestEqual(TEXT("Pending item resolves nearest usable destination Y"), Pending->ItemState.CellY, 1);
	TestEqual(TEXT("Pending item keeps its canonical ID"), Pending->ItemState.ItemDefinitionId, Stone.ItemDefinitionId);
	TestNull(TEXT("Pending definition field retired"), FGridPendingWorldItemState::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")));

	TestTrue(TEXT("Party can travel to the lower level"), Runtime->TravelToDungeonLevel(LowerId, 1, 1, EGridEdge::North, Party));
	TestEqual(TEXT("The queued item materializes at the resolved fallback cell on the lower level"),
		Runtime->GetWorldItemWeightAtCell(2, 1, false), CanonicalStone.WeightKg);

	const FGridLevelRuntimeState* LowerAfterArrival = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
	if (!TestNotNull(TEXT("Lower runtime state still exists"), LowerAfterArrival))
	{
		return false;
	}
	TestFalse(TEXT("Pending queue is consumed after materialization"), LowerAfterArrival->PendingInboundItems.Contains(Stone.RuntimeObjectId));

	TestTrue(TEXT("Capturing the active lower level succeeds"), Runtime->CaptureCurrentLevelRuntimeState());
	const FGridLevelRuntimeState* CapturedLower = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
	if (!TestNotNull(TEXT("Captured lower state exists"), CapturedLower))
	{
		return false;
	}
	TestTrue(TEXT("The fallen item now uses ordinary World Item persistence"), CapturedLower->Items.Contains(Stone.RuntimeObjectId));
	TestTrue(TEXT("The lower level becomes visited only when actually active/captured"), CapturedLower->bHasBeenVisited);
	return true;
}


namespace GridPIT02Canonical
{
    struct FScopedRegistry
    {
        UWorld* World = nullptr;
        UGameInstance* GameInstance = nullptr;
        UGridItemSubsystem* Registry = nullptr;

        explicit FScopedRegistry(UWorld* InWorld) : World(InWorld)
        {
            if (!World) return;
            GameInstance = NewObject<UGameInstance>(GetTransientPackage());
            if (!GameInstance) return;
            GameInstance->Init();
            World->SetGameInstance(GameInstance);
            Registry = GameInstance->GetSubsystem<UGridItemSubsystem>();
        }

        ~FScopedRegistry()
        {
            if (World) World->SetGameInstance(nullptr);
            if (GameInstance) GameInstance->Shutdown();
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGridClean064X1611CanonicalPitTransfer,
    "Grimrock.RPG.ARM.CLEAN06_4X1611.CanonicalPitSaveContinueAndRejectInvalid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X1611CanonicalPitTransfer::RunTest(const FString&)
{
    using namespace GridPIT02;
    FTestWorld TestWorld;
    GridPIT02Canonical::FScopedRegistry Scope(TestWorld.World);
    if (!TestNotNull(TEXT("Transient world"), TestWorld.World) ||
        !TestNotNull(TEXT("Registered canonical item subsystem"), Scope.Registry) ||
        !TestTrue(TEXT("Torch identity exists"), Scope.Registry->HasCanonicalItem(TEXT("Item_Torch"))) ||
        !TestTrue(TEXT("Sword identity exists"), Scope.Registry->HasCanonicalItem(TEXT("Weapon_StraightSword_01"))))
        return false;

    FGridResolvedItem CanonicalTorch;
    if (!TestTrue(TEXT("Torch N0 fully resolves"), Scope.Registry->ResolveItem(TEXT("Item_Torch"), 0, CanonicalTorch)) ||
        !TestTrue(TEXT("Torch carries real light/Niagara data"),
            CanonicalTorch.LightEmitter.bUsePointLight && !CanonicalTorch.VisualEffects.Effects.IsEmpty()))
        return false;

    AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
    AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Level runtime"), Runtime) || !TestNotNull(TEXT("Party pawn"), Party))
        return false;

    UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
    UGridLevelAsset* Upper = MakeFloor(Dungeon);
    UGridLevelAsset* Lower = MakeFloor(Dungeon);
    const FName UpperId(TEXT("ARM064X1611_Upper"));
    const FName LowerId(TEXT("ARM064X1611_Lower"));
    FGridDungeonLevelEntry UpperEntry;
    UpperEntry.LevelId = UpperId;
    UpperEntry.DisplayName = FText::FromString(TEXT("Upper"));
    UpperEntry.LevelAsset = Upper;
    UpperEntry.LogicalPosition = FIntVector(0, 0, 1);
    UpperEntry.bEnabled = true;
    FGridDungeonLevelEntry LowerEntry;
    LowerEntry.LevelId = LowerId;
    LowerEntry.DisplayName = FText::FromString(TEXT("Lower"));
    LowerEntry.LevelAsset = Lower;
    LowerEntry.LogicalPosition = FIntVector(0, 0, 0);
    LowerEntry.bEnabled = true;
    Dungeon->DefaultLevelId = UpperId;
    Dungeon->Levels = { UpperEntry, LowerEntry };
    Upper->WorldObjectInstances.Add(MakePit(1, 1, LowerId));

    Runtime->DungeonAsset = Dungeon;
    Runtime->CurrentDungeonLevelId = UpperId;
    Runtime->LevelAsset = Upper;
    Party->SetGridStart(Runtime, 0, 0, EGridEdge::North);

    // A contradicting legacy definition must never enter a canonical pit snapshot.
    UGridItemDefinitionAsset* ObsoleteTorch = NewObject<UGridItemDefinitionAsset>(Runtime);
    ObsoleteTorch->ItemDefinitionId = TEXT("Item_Torch");
    ObsoleteTorch->Weight = 999.0f;
    FGridItemInstance Torch;
    Torch.RuntimeObjectId = FGuid(11, 22, 33, 44);
    Torch.ItemDefinitionId = TEXT("Item_Torch");
    Torch.ItemLevel = 0;
    Torch.Quantity = 1;
    Torch.OwnerType = EGridItemOwnerType::World;
    Torch.OwnerCharacterIndex = INDEX_NONE;
    Torch.bLightsEnabled = true;

    if (!TestTrue(TEXT("Canonical lit torch routes through open pit"),
            Runtime->TryRouteWorldItemThroughOpenPit(Torch, ObsoleteTorch, 1, 1, FVector::ZeroVector)))
        return false;
    const FGridLevelRuntimeState* LowerState = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
    const FGridPendingWorldItemState* Pending = LowerState
        ? LowerState->PendingInboundItems.Find(Torch.RuntimeObjectId) : nullptr;
    if (!TestNotNull(TEXT("Torch queued on lower level"), Pending))
        return false;
    TestNull(TEXT("Non-ARM pit transfer has no reflected old definition"),
        FGridPendingWorldItemState::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")));
    TestEqual(TEXT("Queued canonical identity"), Pending->ItemState.ItemDefinitionId, Torch.ItemDefinitionId);
    TestEqual(TEXT("Queued N0 is stable"), Pending->ItemState.ItemLevel, 0);
    TestTrue(TEXT("Queued flame stays lit"), Pending->ItemState.bLightsEnabled);

    // The exact same call previously accepted ARM N0 when an old pointer was passed.
    FGridResolvedItem InvalidSword;
    TestFalse(TEXT("Persisted sword N0 is invalid in canonical catalogue"),
        Scope.Registry->ResolveItem(TEXT("Weapon_StraightSword_01"), 0, InvalidSword));
    UGridItemDefinitionAsset* ObsoleteSword = NewObject<UGridItemDefinitionAsset>(Runtime);
    ObsoleteSword->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
    FGridItemInstance Invalid;
    Invalid.RuntimeObjectId = FGuid(55, 66, 77, 88);
    Invalid.ItemDefinitionId = ObsoleteSword->ItemDefinitionId;
    Invalid.ItemLevel = 0;
    Invalid.Quantity = 1;
    Invalid.OwnerType = EGridItemOwnerType::World;
    Invalid.OwnerCharacterIndex = INDEX_NONE;
    AddExpectedError(TEXT("GridPit ItemTransfer rejected Item="),
        EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Known invalid ARM N0 cannot route by obsolete pointer"),
        Runtime->TryRouteWorldItemThroughOpenPit(Invalid, ObsoleteSword, 1, 1, FVector::ZeroVector));
    LowerState = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
    TestTrue(TEXT("Rejected invalid item never entered destination queue"),
        LowerState && LowerState->PendingInboundItems.Num() == 1 &&
        !LowerState->PendingInboundItems.Contains(Invalid.RuntimeObjectId));

    // Persist the real pending PIT02 snapshot to memory and restore it.
    UGrimrockPartySaveGame* Source = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
    Source->DungeonRuntimeState = Runtime->DungeonRuntimeState;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Pending PIT02 state serializes to RAM"),
            UGameplayStatics::SaveGameToMemory(Source, Bytes)))
        return false;
    UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(
        UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Pending PIT02 state reloads from RAM"), Loaded) ||
        !TestTrue(TEXT("Current SaveGame version remains compatible"),
            Loaded->IsLoadValid() && Loaded->IsCompatible()))
        return false;
    Runtime->DungeonRuntimeState = Loaded->DungeonRuntimeState;

    if (!TestTrue(TEXT("Party travels to pit destination"),
            Runtime->TravelToDungeonLevel(LowerId, 0, 0, EGridEdge::North, Party)))
        return false;

    int32 TorchCount = 0;
    AGridItemActor* Restored = nullptr;
    for (TActorIterator<AGridItemActor> It(TestWorld.World); It; ++It)
    {
        AGridItemActor* Item = *It;
        if (IsValid(Item) && !Item->IsActorBeingDestroyed() &&
            Item->GetRuntimeObjectId() == Torch.RuntimeObjectId)
        {
            ++TorchCount;
            Restored = Item;
        }
    }
    if (!TestEqual(TEXT("Original GUID materializes exactly once"), TorchCount, 1) ||
        !TestNotNull(TEXT("Materialized torch exists"), Restored))
        return false;
    TestEqual(TEXT("ItemId survives inter-level Save/Continue"),
        Restored->GetItemDefinitionId(), Torch.ItemDefinitionId);
    TestNull(TEXT("Landing has no legacy asset"), Restored->GetItemDefinitionAsset());
    TestTrue(TEXT("Landing retains lit state"), Restored->AreItemLightsEnabled());
    TestTrue(TEXT("Landing restores canonical PointLight"),
        Restored->LightEmitterComponent &&
        Restored->LightEmitterComponent->RuntimeConfig.bUsePointLight);
    TestTrue(TEXT("Landing restores canonical Niagara"),
        Restored->VisualEffectsComponent &&
        Restored->VisualEffectsComponent->GetConfiguredEffectCount() ==
            CanonicalTorch.VisualEffects.Effects.Num());

    // Direct inbound calls must refuse an unknown physical ID and
    // leave the queued source intact.
    FGridPendingWorldItemState Unknown;
    Unknown.ItemState.ObjectId = FGuid::NewGuid();
    Unknown.ItemState.ItemDefinitionId = TEXT("UnknownPitFixture_5G81");
    Unknown.ItemState.CellX = 1;
    Unknown.ItemState.CellY = 1;
    FGridLevelRuntimeState& Active = Runtime->DungeonRuntimeState.LevelStates.FindChecked(LowerId);
    Active.PendingInboundItems.Add(Unknown.ItemState.ObjectId, Unknown);
    AddExpectedError(TEXT("GridPit PendingItem rejected Item="), EAutomationExpectedErrorFlags::Contains, 1);
    TestEqual(TEXT("Unknown inbound identity is refused directly"), Runtime->ApplyPendingInboundItemsForCurrentLevel(), 0);
    TestTrue(TEXT("Rejected pending source remains queued"),
        Active.PendingInboundItems.Contains(Unknown.ItemState.ObjectId));
    Active.PendingInboundItems.Remove(Unknown.ItemState.ObjectId);

    const FGridLevelRuntimeState* After = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
    TestTrue(TEXT("Consumed inbound queue stays empty"), After &&
        !After->PendingInboundItems.Contains(Torch.RuntimeObjectId));
    TestEqual(TEXT("Repeated inbound apply cannot duplicate torch"),
        Runtime->ApplyPendingInboundItemsForCurrentLevel(), 0);
    return !HasAnyErrors();
}

#endif
