#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPitTrapdoorActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridPIT03
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
				FName(*FString::Printf(TEXT("PIT03TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
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
		Level->Width = 5;
		Level->Height = 5;
		Level->CellSize = 200.0f;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}

	FGridWorldObjectInstance MakeControlledPit(int32 X, int32 Y, FName TargetLevelId, bool bInitiallyOpen)
	{
		FGridWorldObjectInstance Pit;
		Pit.InstanceId = FGuid::NewGuid();
		Pit.Type = EGridLevelObjectType::Pit;
		Pit.WorldObjectDefinitionId = TEXT("Pit_Stone_01");
		Pit.CellX = X;
		Pit.CellY = Y;
		Pit.WallSide = EGridEdge::None;
		Pit.InstanceConfig.Pit.bInitiallyOpen = bInitiallyOpen;
		Pit.InstanceConfig.Pit.bUseSameCellCoordinates = true;
		Pit.InstanceConfig.Relocation.TargetLevelId = TargetLevelId;
		Pit.InstanceConfig.Relocation.TargetFacing = EGridEdge::North;
		return Pit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPIT03ControlledStateTest, "Grimrock.Pit.PIT03.ControlledStateAndLinks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPIT03ControlledStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPIT03;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("PIT03 world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("Runtime exists"), Runtime) || !TestNotNull(TEXT("Party exists"), Party))
	{
		return false;
	}

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
	UGridLevelAsset* Upper = MakeFloor(Dungeon);
	UGridLevelAsset* Lower = MakeFloor(Dungeon);
	const FName UpperId(TEXT("PIT03_Upper"));
	const FName LowerId(TEXT("PIT03_Lower"));

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

	FGridWorldObjectInstance PitA = MakeControlledPit(2, 2, LowerId, false);
	FGridWorldObjectInstance PitB = MakeControlledPit(3, 2, LowerId, false);
	FGridWorldObjectInstance Button;
	Button.InstanceId = FGuid::NewGuid();
	Button.Type = EGridLevelObjectType::Button;
	Button.CellX = 1;
	Button.CellY = 2;
	Button.WallSide = EGridEdge::East;

	const FGuid PitAId = PitA.InstanceId;
	const FGuid PitBId = PitB.InstanceId;
	const FGuid ButtonId = Button.InstanceId;
	Upper->WorldObjectInstances = { Button, PitA, PitB };

	FGridObjectLink OpenPitA;
	OpenPitA.SourceObjectId = ButtonId;
	OpenPitA.TargetObjectId = PitAId;
	OpenPitA.SourceEvent = EGridObjectEvent::Activated;
	OpenPitA.Command = EGridObjectCommand::Open;

	FGridObjectLink ChainPitB;
	ChainPitB.SourceObjectId = PitAId;
	ChainPitB.TargetObjectId = PitBId;
	ChainPitB.SourceEvent = EGridObjectEvent::Opened;
	ChainPitB.Command = EGridObjectCommand::Open;
	Upper->Links = { OpenPitA, ChainPitB };

	UGridWorldObjectDefinitionAsset* PitDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	PitDefinition->DefinitionId = TEXT("Pit_Stone_01");
	PitDefinition->SupportedType = EGridLevelObjectType::Pit;
	PitDefinition->PlacementSurface = EGridObjectPlacementKind::Floor;
	PitDefinition->StaticPart.Mesh = NewObject<UStaticMesh>(Runtime);
	PitDefinition->bHideCellFloor = true;
	PitDefinition->MovingParts.SetNum(2);
	PitDefinition->MovingParts[0].Mesh = NewObject<UStaticMesh>(Runtime);
	PitDefinition->MovingParts[1].Mesh = NewObject<UStaticMesh>(Runtime);
	PitDefinition->RuntimeActorClass = AGridPitTrapdoorActor::StaticClass();

	Runtime->DungeonAsset = Dungeon;
	Runtime->CurrentDungeonLevelId = UpperId;
	Runtime->LevelAsset = Upper;
	Runtime->WorldObjectDefinitions.Add(PitDefinition);
	Party->SetGridStart(Runtime, 1, 1, EGridEdge::North);

	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	if (!TestNotNull(TEXT("Activation component exists"), Activation))
	{
		return false;
	}
	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();

	TestFalse(TEXT("Pit A starts closed"), Runtime->IsPitOpen(PitAId));
	TestFalse(TEXT("Pit B starts closed"), Runtime->IsPitOpen(PitBId));

	FGridRelocationBehaviorParams Relocation;
	TestFalse(TEXT("Closed Pit A is not a fall-through cell"), Runtime->FindOpenPitAtCell(2, 2, Relocation));

	UGridItemDefinitionAsset* StoneDefinition = NewObject<UGridItemDefinitionAsset>(Runtime);
	StoneDefinition->ItemDefinitionId = TEXT("Item_PIT03_Stone");
	StoneDefinition->Weight = 1.5f;

	FGridItemInstance Stone;
	Stone.RuntimeObjectId = FGuid::NewGuid();
	Stone.ItemDefinitionId = StoneDefinition->ItemDefinitionId;
	Stone.Quantity = 1;

	TestTrue(TEXT("World Item can rest on a closed trapdoor"),
		Runtime->TryDropItemInstanceAtCell(Stone, StoneDefinition, 2, 2, EGridEdge::None, FVector(12.0f, 8.0f, 0.0f)));
	TestEqual(TEXT("Closed trapdoor keeps item weight on upper level"), Runtime->GetWorldItemWeightAtCell(2, 2, false), StoneDefinition->Weight);

	TestTrue(TEXT("Button Activated -> Pit A Open executes"), Runtime->ExecuteLinksFromRuntimeObject(ButtonId, EGridObjectEvent::Activated));
	TestTrue(TEXT("Pit A is open after command"), Runtime->IsPitOpen(PitAId));
	TestTrue(TEXT("Pit A Opened event chained to Pit B Open"), Runtime->IsPitOpen(PitBId));
	TestTrue(TEXT("Open Pit A is now a fall-through cell"), Runtime->FindOpenPitAtCell(2, 2, Relocation));
	TestEqual(TEXT("Opening under a World Item removes its upper-level weight"), Runtime->GetWorldItemWeightAtCell(2, 2, false), 0.0f);

	const FGridLevelRuntimeState* LowerState = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
	if (!TestNotNull(TEXT("Lower level received falling item state"), LowerState))
	{
		return false;
	}
	TestTrue(TEXT("Opening the trapdoor routed the resting item through PIT02"), LowerState->PendingInboundItems.Contains(Stone.RuntimeObjectId));

	const FGridLevelRuntimeState* UpperState = Runtime->DungeonRuntimeState.LevelStates.Find(UpperId);
	if (!TestNotNull(TEXT("Upper runtime state exists"), UpperState))
	{
		return false;
	}
	const FGridRuntimePitState* PitAState = UpperState->Pits.Find(PitAId);
	TestTrue(TEXT("Pit A runtime state persists Open"), PitAState && PitAState->bIsOpen);

	TestTrue(TEXT("Close command API succeeds"), Runtime->SetPitOpen(PitAId, false, false));
	TestFalse(TEXT("Pit A is closed"), Runtime->IsPitOpen(PitAId));
	TestTrue(TEXT("Toggle reopens Pit A"), Runtime->TogglePit(PitAId, false));
	TestTrue(TEXT("Pit A is open after Toggle"), Runtime->IsPitOpen(PitAId));

	TestTrue(TEXT("Travel to lower level succeeds"), Runtime->TravelToDungeonLevel(LowerId, 1, 1, EGridEdge::North, Party));
	TestEqual(TEXT("Fallen item materialized normally below"), Runtime->GetWorldItemWeightAtCell(2, 2, false), StoneDefinition->Weight);
	TestTrue(TEXT("Travel back to upper level succeeds"), Runtime->TravelToDungeonLevel(UpperId, 1, 1, EGridEdge::North, Party));
	TestTrue(TEXT("Pit A open state survives level round-trip"), Runtime->IsPitOpen(PitAId));
	TestTrue(TEXT("Pit B open state survives level round-trip"), Runtime->IsPitOpen(PitBId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPIT03PresentationActorTest, "Grimrock.Pit.PIT03.PresentationActorState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPIT03PresentationActorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPIT03;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Presentation test world exists"), TestWorld.World))
	{
		return false;
	}

	AGridPitTrapdoorActor* Actor = TestWorld.World->SpawnActor<AGridPitTrapdoorActor>();
	if (!TestNotNull(TEXT("Pit trapdoor actor exists"), Actor))
	{
		return false;
	}

	FGridWorldObjectInstance Pit = MakeControlledPit(1, 1, TEXT("Lower"), false);
	const FVector LeftHinge(-85.0f, 0.0f, -5.0f);
	const FVector RightHinge(85.0f, 0.0f, -5.0f);
	constexpr float OpenAngleDegrees = 80.0f;
	constexpr float MoveDuration = 0.0f;

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Actor);
	Definition->SupportedType = EGridLevelObjectType::Pit;
	Definition->MovingParts.SetNum(2);
	Definition->MovingParts[0].Mesh = NewObject<UStaticMesh>(Actor);
	Definition->MovingParts[0].Motion.Type = EGridWorldObjectMotionType::Rotation;
	Definition->MovingParts[0].Motion.Axis = EGridWorldObjectMotionAxis::Y;
	Definition->MovingParts[0].Motion.Pivot = LeftHinge;
	// Positive quaternion rotation around Y produces negative Unreal Pitch.
	Definition->MovingParts[0].Motion.Amount = OpenAngleDegrees;
	Definition->MovingParts[0].Motion.Duration = MoveDuration;
	Definition->MovingParts[1].Mesh = NewObject<UStaticMesh>(Actor);
	Definition->MovingParts[1].Motion.Type = EGridWorldObjectMotionType::Rotation;
	Definition->MovingParts[1].Motion.Axis = EGridWorldObjectMotionAxis::Y;
	Definition->MovingParts[1].Motion.Pivot = RightHinge;
	Definition->MovingParts[1].Motion.Amount = -OpenAngleDegrees;
	Definition->MovingParts[1].Motion.Duration = MoveDuration;

	Actor->InitializeRuntimeMechanismVisuals(FGridRuntimeWorldObjectData(Pit), Definition, FTransform::Identity);
	Actor->InitializeRuntimeWorldObject(FGridRuntimeWorldObjectData(Pit), nullptr, FTransform::Identity);

	TestTrue(TEXT("Presentation actor requires and sees both leaves"), Actor->HasCompleteTrapdoorCover());
	TestFalse(TEXT("Presentation actor starts Closed"), Actor->IsPitOpenVisualState());
	Actor->SetPitOpenVisualState(true, false);
	TestTrue(TEXT("Presentation actor accepts Open state"), Actor->IsPitOpenVisualState());
	TestTrue(TEXT("Left leaf reaches -80 degrees"), FMath::IsNearlyEqual(Actor->GetLeftLeafPitch(), -80.0f, 0.1f));
	TestTrue(TEXT("Right leaf reaches +80 degrees"), FMath::IsNearlyEqual(Actor->GetRightLeafPitch(), 80.0f, 0.1f));
	return true;
}



namespace GridPIT03CanonicalARM
{
    struct FRegistry
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Items = nullptr;
        explicit FRegistry(UWorld* InWorld) : World(InWorld)
        {
            if (!World) return;
            GI = NewObject<UGameInstance>(GetTransientPackage());
            if (!GI) return;
            GI->Init();
            World->SetGameInstance(GI);
            Items = GI->GetSubsystem<UGridItemSubsystem>();
        }
        ~FRegistry()
        {
            if (World) World->SetGameInstance(nullptr);
            if (GI) GI->Shutdown();
        }
    };

    AGridItemActor* FindLive(UWorld* World, const FGuid Guid, int32& OutCount)
    {
        OutCount = 0;
        AGridItemActor* Result = nullptr;
        if (!World) return nullptr;
        for (TActorIterator<AGridItemActor> It(World); It; ++It)
        {
            AGridItemActor* Actor = *It;
            if (IsValid(Actor) && !Actor->IsActorBeingDestroyed() &&
                Actor->GetRuntimeObjectId() == Guid)
            {
                ++OutCount;
                Result = Actor;
            }
        }
        return Result;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPit03CanonicalARMPersistedFall,
    "Grimrock.RPG.ARM.CLEAN06_4X1612.TrapOpensUnderTwoWeaponsRealLevels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPit03CanonicalARMPersistedFall::RunTest(const FString&)
{
    using namespace GridPIT03;
    using namespace GridPIT03CanonicalARM;
    FTestWorld F;
    FRegistry Registry(F.World);
    if (!TestNotNull(TEXT("Transient world"), F.World) ||
        !TestNotNull(TEXT("Canonical game instance"), Registry.Items))
        return false;

    struct FWeapon
    {
        FName Id;
        int32 Level;
        FGuid Guid;
        FGridResolvedItem Resolved;
    };
    TArray<FWeapon> Weapons;
    Weapons.Add({ FName(TEXT("Weapon_StraightSword_01")), 1, FGuid(101, 102, 103, 104), FGridResolvedItem() });
    Weapons.Add({ FName(TEXT("Weapon_ThrowingKnife_01")), 20, FGuid(201, 202, 203, 204), FGridResolvedItem() });
    for (FWeapon& W : Weapons)
    {
        if (!TestTrue(TEXT("Real ARM resolves at its fixed N-level"),
                Registry.Items->ResolveItem(W.Id, W.Level, W.Resolved)) ||
            !TestTrue(TEXT("Real ARM has a world mesh"),
                W.Resolved.bIsArmament && !W.Resolved.WorldMesh.IsNull()))
            return false;
    }

    AGridLevelRuntimeActor* Runtime = F.World->SpawnActor<AGridLevelRuntimeActor>();
    AGrimrockPartyPawn* Party = F.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Runtime"), Runtime) || !TestNotNull(TEXT("Party"), Party))
        return false;
    UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
    UGridLevelAsset* Upper = MakeFloor(Dungeon);
    UGridLevelAsset* Lower = MakeFloor(Dungeon);
    // Real trap layout: the vertical target is a void; items must land on
    // the same nearest usable neighboring cell as the falling party.
    Lower->GetCellMutable(2, 2).CellType = EGridCellType::Empty;
    Lower->GetCellMutable(2, 2).bBlocksOccupancy = false;
    const FIntPoint ExpectedLanding(2, 1);
    const FName UpperId(TEXT("ARM064X1612_Upper"));
    const FName LowerId(TEXT("ARM064X1612_Lower"));
    FGridDungeonLevelEntry U;
    U.LevelId = UpperId;
    U.LevelAsset = Upper;
    U.LogicalPosition = FIntVector(0, 0, 1);
    U.bEnabled = true;
    FGridDungeonLevelEntry L;
    L.LevelId = LowerId;
    L.LevelAsset = Lower;
    L.LogicalPosition = FIntVector(0, 0, 0);
    L.bEnabled = true;
    Dungeon->DefaultLevelId = UpperId;
    Dungeon->Levels = {U, L};
    const FGridWorldObjectInstance Pit = MakeControlledPit(2, 2, LowerId, false);
    const FGuid PitGuid = Pit.InstanceId;
    Upper->WorldObjectInstances.Add(Pit);

    UGridWorldObjectDefinitionAsset* Def = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    Def->DefinitionId = TEXT("Pit_Stone_01");
    Def->SupportedType = EGridLevelObjectType::Pit;
    Def->PlacementSurface = EGridObjectPlacementKind::Floor;
    Def->StaticPart.Mesh = NewObject<UStaticMesh>(Runtime);
    Def->bHideCellFloor = true;
    Def->MovingParts.SetNum(2);
    Def->MovingParts[0].Mesh = NewObject<UStaticMesh>(Runtime);
    Def->MovingParts[1].Mesh = NewObject<UStaticMesh>(Runtime);
    Def->RuntimeActorClass = AGridPitTrapdoorActor::StaticClass();
    Runtime->DungeonAsset = Dungeon;
    Runtime->CurrentDungeonLevelId = UpperId;
    Runtime->LevelAsset = Upper;
    Runtime->WorldObjectDefinitions.Add(Def);
    Party->SetGridStart(Runtime, 1, 1, EGridEdge::North);

    TestFalse(TEXT("Trap starts closed"), Runtime->IsPitOpen(PitGuid));
    for (int32 i=0; i<Weapons.Num(); ++i)
    {
        const FWeapon& W = Weapons[i];
        FGridItemInstance Placed;
        Placed.RuntimeObjectId = W.Guid;
        Placed.ItemDefinitionId = W.Id;
        Placed.ItemLevel = W.Level;
        Placed.Quantity = 1;
        Placed.OwnerType = EGridItemOwnerType::World;
        const FVector Offset(i == 0 ? -25.f : 25.f, 0.f, 0.f);
        // Second weapon's recorded cell is stale: the Chaos mesh has moved
        // onto the trap. It must be collected rather than dropped on rebuild.
        const int32 PlacedX = i == 0 ? 2 : 1;
        if (!TestTrue(TEXT("Weapon can be physically placed near CLOSED trap"),
                Runtime->TryDropItemInstanceAtCell(Placed, PlacedX, 2, EGridEdge::None, Offset)))
            return false;
        int32 Count = 0;
        AGridItemActor* Actor = FindLive(F.World, W.Guid, Count);
        if (!TestNotNull(TEXT("Weapon actor exists above trap"), Actor) ||
            !TestEqual(TEXT("One actor per weapon"), Count, 1) ||
            !TestTrue(TEXT("Weapon starts with complete canonical presentation"),
                Actor->CanonicalWorldPresentation.IsValid() && Actor->MeshComponent &&
                Actor->MeshComponent->GetStaticMesh() == W.Resolved.WorldMesh.LoadSynchronous()) ||
            !TestNull(TEXT("No legacy weapon definition on ground actor"),
                Actor->GetItemDefinitionAsset()))
            return false;
        if (i == 1 && Actor->MeshComponent)
        {
            const FVector TrapCenter = Runtime->GetCellCenterWorld(2, 2, 12.f);
            Actor->MeshComponent->SetWorldLocation(TrapCenter,
                false, nullptr, ETeleportType::TeleportPhysics);
        }
    }

    if (!TestTrue(TEXT("Opening a controlled pit routes the resting weapons"),
            Runtime->SetPitOpen(PitGuid, true, false)))
        return false;
    const FGridLevelRuntimeState* PendingLevel =
        Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
    if (!TestNotNull(TEXT("Lower-level runtime exists"), PendingLevel))
        return false;
    for (const FWeapon& W : Weapons)
    {
        const FGridPendingWorldItemState* P = PendingLevel->PendingInboundItems.Find(W.Guid);
        if (!TestNotNull(TEXT("Same GUID queued downstairs"), P) ||
            !TestEqual(TEXT("Weapon lands at adjacent resolved X"), P->ItemState.CellX, ExpectedLanding.X) ||
            !TestEqual(TEXT("Weapon lands at adjacent resolved Y"), P->ItemState.CellY, ExpectedLanding.Y) ||
            !TestEqual(TEXT("Weapon N-level preserved"), P->ItemState.ItemLevel, W.Level) ||
            !TestEqual(TEXT("Weapon ID preserved"), P->ItemState.ItemDefinitionId, W.Id) ||
            !TestNull(TEXT("No legacy DA_Weapon pointer in PIT02 queue"),
                P->ItemDefinitionAsset.Get()))
            return false;
        int32 Count = 0;
        FindLive(F.World, W.Guid, Count);
        if (!TestEqual(TEXT("Upper weapon actor destroyed at pit opening"), Count, 0))
            return false;
    }

    FGridItemInstance Duplicate;
    Duplicate.RuntimeObjectId = Weapons[0].Guid;
    Duplicate.ItemDefinitionId = Weapons[0].Id;
    Duplicate.ItemLevel = Weapons[0].Level;
    Duplicate.Quantity = 1;
    Duplicate.OwnerType = EGridItemOwnerType::World;
    TestFalse(TEXT("A second arrival cannot replace an existing queued GUID"),
        Runtime->TryDropItemInstanceAtCell(Duplicate, 2, 2, EGridEdge::None, FVector::ZeroVector));
    FGridItemInstance InvalidQuantity = Duplicate;
    InvalidQuantity.RuntimeObjectId = FGuid::NewGuid();
    InvalidQuantity.Quantity = MAX_int32;
    TestFalse(TEXT("Oversized physical stack cannot enter pit queue"),
        Runtime->TryDropItemInstanceAtCell(InvalidQuantity, 2, 2, EGridEdge::None, FVector::ZeroVector));
    PendingLevel = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
    if (!TestNotNull(TEXT("Original destination state retained"), PendingLevel) ||
        !TestEqual(TEXT("Exactly two original arrivals remain"), PendingLevel->PendingInboundItems.Num(), 2))
        return false;
    const FGridPendingWorldItemState* Original = PendingLevel->PendingInboundItems.Find(Weapons[0].Guid);
    if (!TestNotNull(TEXT("Original queued identity retained"), Original) ||
        !TestEqual(TEXT("Original queued quantity retained"), Original->ItemState.Quantity, 1))
        return false;

    // Physical save round-trip, *in memory*, before travelling downstairs.
    UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
    Save->DungeonRuntimeState = Runtime->DungeonRuntimeState;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("PIT02 armament state serializes to RAM"),
            UGameplayStatics::SaveGameToMemory(Save, Bytes)))
        return false;
    UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(
        UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("PIT02 armament state reloads from RAM"), Loaded) ||
        !TestTrue(TEXT("Current SaveGame schema is compatible"), Loaded->IsCompatible()))
        return false;
    Runtime->DungeonRuntimeState = Loaded->DungeonRuntimeState;

    if (!TestTrue(TEXT("Party can travel to lower dungeon level"),
            Runtime->TravelToDungeonLevel(LowerId, ExpectedLanding.X, ExpectedLanding.Y, EGridEdge::North, Party)))
        return false;
    for (const FWeapon& W : Weapons)
    {
        int32 Count = 0;
        AGridItemActor* Actor = FindLive(F.World, W.Guid, Count);
        const FVector ExpectedCenter = Runtime->GetCellCenterWorld(ExpectedLanding.X, ExpectedLanding.Y, 12.f);
        if (!TestNotNull(TEXT("Weapon materialized below"), Actor) ||
            !TestEqual(TEXT("Each weapon appears exactly once downstairs"), Count, 1) ||
            !TestTrue(TEXT("Weapon root is in the actual adjacent arrival cell"),
                Actor->GetActorLocation().Equals(ExpectedCenter, 80.f)) ||
            !TestEqual(TEXT("ItemId below"), Actor->GetItemDefinitionId(), W.Id) ||
            !TestEqual(TEXT("Exact N-level below"), Actor->CanonicalWorldItemLevel, W.Level) ||
            !TestNull(TEXT("No obsolete item pointer below"), Actor->GetItemDefinitionAsset()) ||
            !TestTrue(TEXT("Canonical physical weapon mesh below"),
                Actor->CanonicalWorldPresentation.IsValid() && Actor->MeshComponent &&
                Actor->MeshComponent->GetStaticMesh() == W.Resolved.WorldMesh.LoadSynchronous()))
            return false;
    }
    const float ExpectedWeight = Weapons[0].Resolved.WeightKg + Weapons[1].Resolved.WeightKg;
    if (!TestTrue(TEXT("Both armaments have their canonical weight on the adjacent target cell"),
            FMath::IsNearlyEqual(
                Runtime->GetWorldItemWeightAtCell(ExpectedLanding.X, ExpectedLanding.Y, false),
                ExpectedWeight, 0.1f)))
        return false;
    const FGridLevelRuntimeState* After = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
    for (const FWeapon& W : Weapons)
        if (!TestTrue(TEXT("Each queued item consumed"), After &&
                !After->PendingInboundItems.Contains(W.Guid))) return false;

    if (!TestTrue(TEXT("Return to upper level"),
            Runtime->TravelToDungeonLevel(UpperId, 1, 1, EGridEdge::North, Party)) ||
        !TestTrue(TEXT("Return to lower level"),
            Runtime->TravelToDungeonLevel(LowerId, 1, 1, EGridEdge::North, Party)))
        return false;
    for (const FWeapon& W : Weapons)
    {
        int32 Count = 0;
        AGridItemActor* Actor = FindLive(F.World, W.Guid, Count);
        const FVector ExpectedCenter = Runtime->GetCellCenterWorld(ExpectedLanding.X, ExpectedLanding.Y, 12.f);
        if (!TestNotNull(TEXT("Weapon survived second level load"), Actor) ||
            !TestEqual(TEXT("No duplication after level round-trip"), Count, 1) ||
            !TestTrue(TEXT("Weapon root stays on neighboring arrival cell after reload"),
                FMath::Abs(Actor->GetActorLocation().X - ExpectedCenter.X) <= 80.f &&
                FMath::Abs(Actor->GetActorLocation().Y - ExpectedCenter.Y) <= 80.f) ||
            !TestEqual(TEXT("Level remains fixed after second load"),
                Actor->CanonicalWorldItemLevel, W.Level) ||
            !TestTrue(TEXT("Canonical mesh survives second level load"),
                Actor->CanonicalWorldPresentation.IsValid() && Actor->MeshComponent &&
                Actor->MeshComponent->GetStaticMesh() == W.Resolved.WorldMesh.LoadSynchronous()))
            return false;
    }
    return !HasAnyErrors();
}

#endif
