#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridDungeonTrapPitUtils.h"
#include "Core/GridDungeonAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTRAPPIT01Pairing,
    "Grimrock.TRAP_PIT01.UniqueSameCellPairing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTRAPPIT01Pairing::RunTest(const FString&)
{
    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(GetTransientPackage());
    Level->EnsureCellCount();
    FGridWorldObjectInstance Trap;
    Trap.InstanceId = FGuid::NewGuid();
    Trap.Type = EGridLevelObjectType::Trigger;
    Trap.CellX = 2;
    Trap.CellY = 3;
    FGridWorldObjectInstance Pit;
    Pit.InstanceId = FGuid::NewGuid();
    Pit.Type = EGridLevelObjectType::Pit;
    Pit.CellX = 2;
    Pit.CellY = 3;
    FGuid Found;
    TestFalse(TEXT("No colocated Pit fails closed"),
        GridDungeonTrapPit::FindUniqueSameCellPit(*Level, Trap, Found));
    Level->WorldObjectInstances.Add(Pit);
    TestTrue(TEXT("Exactly one colocated Pit resolves"),
        GridDungeonTrapPit::FindUniqueSameCellPit(*Level, Trap, Found));
    TestEqual(TEXT("Correct target GUID"), Found, Pit.InstanceId);
    FGridWorldObjectInstance AnotherPit = Pit;
    AnotherPit.InstanceId = FGuid::NewGuid();
    Level->WorldObjectInstances.Add(AnotherPit);
    TestFalse(TEXT("Two colocated Pit placements fail closed"),
        GridDungeonTrapPit::FindUniqueSameCellPit(*Level, Trap, Found));
    TestFalse(TEXT("Ambiguity clears returned ID"), Found.IsValid());
    Level->WorldObjectInstances.RemoveAt(1);
    Level->WorldObjectInstances[0].CellY = 4;
    TestFalse(TEXT("Pit on another cell never matches"),
        GridDungeonTrapPit::FindUniqueSameCellPit(*Level, Trap, Found));
    Level->WorldObjectInstances[0].CellY = 3;
    Level->WorldObjectInstances[0].InstanceId.Invalidate();
    TestFalse(TEXT("Invalid Pit GUID is rejected"),
        GridDungeonTrapPit::FindUniqueSameCellPit(*Level, Trap, Found));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTRAPPIT01Runtime,
    "Grimrock.TRAP_PIT01.FloorTrapOpensPitAndPersists",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTRAPPIT01Runtime::RunTest(const FString&)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("TRAPPIT01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Transient runtime world"), World)) return false;
    if (GEngine)
    {
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
    }
    auto Cleanup = [&]()
    {
        World->DestroyWorld(false);
        if (GEngine) GEngine->DestroyWorldContext(World);
    };
    World->InitializeActorsForPlay(FURL());
    AGridLevelRuntimeActor* Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
    AGrimrockPartyPawn* Party = World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Runtime"), Runtime) ||
        !TestNotNull(TEXT("Party"), Party) ||
        !TestNotNull(TEXT("Inventory"), Party ? Party->PartyInventoryComponent.Get() : nullptr))
    {
        Cleanup();
        return false;
    }

    UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
    UGridLevelAsset* Upper = NewObject<UGridLevelAsset>(Dungeon);
    UGridLevelAsset* Lower = NewObject<UGridLevelAsset>(Dungeon);
    Upper->EnsureCellCount();
    Lower->EnsureCellCount();
    for (FGridLevelCellData& Cell : Upper->Cells)
    {
        Cell.CellType = EGridCellType::Floor;
        Cell.bBlocksOccupancy = false;
    }
    for (FGridLevelCellData& Cell : Lower->Cells)
    {
        Cell.CellType = EGridCellType::Floor;
        Cell.bBlocksOccupancy = false;
    }
    const FName UpperId(TEXT("TRAPPIT_Upper"));
    const FName LowerId(TEXT("TRAPPIT_Lower"));
    FGridDungeonLevelEntry TopEntry;
    TopEntry.LevelId = UpperId;
    TopEntry.LevelAsset = Upper;
    TopEntry.LogicalPosition = FIntVector(0, 0, 1);
    FGridDungeonLevelEntry BottomEntry;
    BottomEntry.LevelId = LowerId;
    BottomEntry.LevelAsset = Lower;
    BottomEntry.LogicalPosition = FIntVector(0, 0, 0);
    Dungeon->Levels = { TopEntry, BottomEntry };
    Dungeon->DefaultLevelId = UpperId;

    UGridWorldObjectDefinitionAsset* TrapDef = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    TrapDef->DefinitionId = TEXT("TRAPPIT_Floor");
    TrapDef->SupportedType = EGridLevelObjectType::Trigger;
    TrapDef->PlacementSurface = EGridObjectPlacementKind::Floor;
    TrapDef->DungeonTrap.bIsDungeonTrap = true;
    TrapDef->DungeonTrap.MinimumDamage = 2;
    TrapDef->DungeonTrap.MaximumDamage = 2;
    TrapDef->DungeonTrap.bOneShot = true;
    TrapDef->RuntimeActorClass = AGridGenericObjectActor::StaticClass();
    TrapDef->StaticPart.Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!TestNotNull(TEXT("Existing test trap mesh"), TrapDef->StaticPart.Mesh.Get()))
    {
        Cleanup();
        return false;
    }
    UGridWorldObjectDefinitionAsset* PitDef = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    PitDef->DefinitionId = TEXT("TRAPPIT_Covered");
    PitDef->SupportedType = EGridLevelObjectType::Pit;
    PitDef->PlacementSurface = EGridObjectPlacementKind::Floor;
    PitDef->bHideCellFloor = true;
    PitDef->MovingParts.SetNum(2);
    PitDef->MovingParts[0].Mesh = NewObject<UStaticMesh>(Runtime);
    PitDef->MovingParts[1].Mesh = NewObject<UStaticMesh>(Runtime);
    Runtime->DungeonAsset = Dungeon;
    Runtime->CurrentDungeonLevelId = UpperId;
    Runtime->LevelAsset = Upper;
    Runtime->WorldObjectDefinitions.Add(TrapDef);
    Runtime->WorldObjectDefinitions.Add(PitDef);

    FGridWorldObjectInstance Trap;
    Trap.InstanceId = FGuid::NewGuid();
    Trap.Type = EGridLevelObjectType::Trigger;
    Trap.WorldObjectDefinitionId = TrapDef->DefinitionId;
    Trap.CellX = 2;
    Trap.CellY = 2;
    Trap.InstanceConfig.DungeonTrap.bOpenPitOnTrigger = true;
    Trap.InstanceConfig.DungeonTrap.bNoDamage = true;
    FGridWorldObjectInstance Pit;
    Pit.InstanceId = FGuid::NewGuid();
    Pit.Type = EGridLevelObjectType::Pit;
    Pit.WorldObjectDefinitionId = PitDef->DefinitionId;
    Pit.CellX = 2;
    Pit.CellY = 2;
    Pit.InstanceConfig.Pit.bInitiallyOpen = false;
    Pit.InstanceConfig.Pit.bUseSameCellCoordinates = true;
    Pit.InstanceConfig.Relocation.TargetLevelId = LowerId;
    // Ordinary traps and unpaired Pit triggers must never be hidden.
    FGridWorldObjectInstance Ordinary = Trap;
    Ordinary.InstanceId = FGuid::NewGuid();
    Ordinary.CellX = 4;
    Ordinary.CellY = 4;
    Ordinary.InstanceConfig.DungeonTrap.bOpenPitOnTrigger = false;
    FGridWorldObjectInstance Unpaired = Trap;
    Unpaired.InstanceId = FGuid::NewGuid();
    Unpaired.CellX = 5;
    Unpaired.CellY = 5;

    Upper->WorldObjectInstances = { Trap, Pit, Ordinary, Unpaired };
    const FGuid TrapId = Trap.InstanceId;
    const FGuid PitId = Pit.InstanceId;

    Party->SetGridStart(Runtime, 1, 2, EGridEdge::North);
    Party->CurrentCellX = 2;
    Party->CurrentCellY = 2;
    FGridCharacterInventoryState& Character =
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.AddDefaulted_GetRef();
    Character.CharacterId = FGuid::NewGuid();
    Character.Resources.CurrentHealth = 30;

    // A detected, still-closed floor trap retains its normal mouse hit.
    TestTrue(TEXT("Detected trap can materialize before opening the Pit"),
        Runtime->TryRecordDungeonTrapDetection(TrapId));
    AGridRuntimeObjectActor* TrapVisual =
        Runtime->FindRuntimeObjectActor<AGridRuntimeObjectActor>(TrapId);
    if (!TestNotNull(TEXT("Runtime floor trap visual"), TrapVisual) ||
        !TestNotNull(TEXT("Runtime trap mesh"), TrapVisual ? TrapVisual->MeshComponent.Get() : nullptr))
    {
        Cleanup();
        return false;
    }
    TestFalse(TEXT("Detected trap remains visible while Pit is closed"), TrapVisual->IsHidden());
    TestEqual(TEXT("Detected trap accepts mouse hit while Pit is closed"),
        TrapVisual->MeshComponent->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);

    FRandomStream Dice(91);
    TestFalse(TEXT("Pit starts closed"), Runtime->IsPitOpen(PitId));
    TestFalse(TEXT("Hidden closed Pit preserves regular floor"), Runtime->ShouldHideCellFloor(2, 2));
    TestEqual(TEXT("Floor trap activates once"),
        Runtime->TriggerDungeonTrapsOnPartyEnterCell(Party, 2, 2, Dice), 1);
    TestTrue(TEXT("Floor trap opened colocated PIT03"), Runtime->IsPitOpen(PitId));
    TestTrue(TEXT("Opened Pit removes ordinary floor"), Runtime->ShouldHideCellFloor(2, 2));
    TestEqual(TEXT("Trap triggered state retained"), Runtime->GetDungeonTrapState(TrapId),
        EGridDungeonTrapState::Triggered);
    TestTrue(TEXT("Opened associated Pit hides the triggered trap mesh"), TrapVisual->IsHidden());
    TestEqual(TEXT("Hidden trigger trap cannot intercept mouse on top of open Pit"),
        TrapVisual->MeshComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    TestFalse(TEXT("Hidden trap removes obsolete danger outline"),
        TrapVisual->MeshComponent->bRenderCustomDepth);
    TestEqual(TEXT("Aucun: floor trap opens pit without direct damage"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth, 30);
    TestTrue(TEXT("Opened Pit is fall-through"),
        Party->IsPitFalling());
    const FGridLevelRuntimeState* State = Runtime->FindRuntimeStateForCurrentLevel();
    TestTrue(TEXT("Pit open state persisted"), State && State->Pits.Contains(PitId) &&
        State->Pits.Find(PitId)->bIsOpen);
    TestEqual(TEXT("Triggered one shot does not activate again"),
        Runtime->TriggerDungeonTrapsOnPartyEnterCell(Party, 2, 2, Dice), 0);
    TestEqual(TEXT("Pit open state remains unchanged on repeat"), Runtime->IsPitOpen(PitId), true);

    const FGridDungeonRuntimeState SavedState = Runtime->DungeonRuntimeState;
    AGridLevelRuntimeActor* Restored = World->SpawnActor<AGridLevelRuntimeActor>();
    if (TestNotNull(TEXT("Restore target"), Restored))
    {
        Restored->DungeonAsset = Dungeon;
        Restored->CurrentDungeonLevelId = UpperId;
        Restored->LevelAsset = Upper;
        Restored->WorldObjectDefinitions.Add(TrapDef);
        Restored->WorldObjectDefinitions.Add(PitDef);
        Restored->DungeonRuntimeState = SavedState;
        TestTrue(TEXT("Open Pit survives runtime state restoration"), Restored->IsPitOpen(PitId));
        TestTrue(TEXT("Restored opened Pit removes ordinary floor"), Restored->ShouldHideCellFloor(2, 2));
        TestEqual(TEXT("Triggered trap survives runtime state restoration"),
            Restored->GetDungeonTrapState(TrapId), EGridDungeonTrapState::Triggered);

        Restored->RestoreDungeonTrapRevealVisuals();
        AGridRuntimeObjectActor* RestoredTrapVisual =
            Restored->FindRuntimeObjectActor<AGridRuntimeObjectActor>(TrapId);
        if (TestNotNull(TEXT("Triggered trap visual reconstituted after restore"), RestoredTrapVisual) &&
            TestNotNull(TEXT("Restored triggered trap mesh"), RestoredTrapVisual ? RestoredTrapVisual->MeshComponent.Get() : nullptr))
        {
            TestTrue(TEXT("Save/Continue keeps trap mesh hidden above opened Pit"), RestoredTrapVisual->IsHidden());
            TestEqual(TEXT("Save/Continue keeps trap mouse collision disabled"),
                RestoredTrapVisual->MeshComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);

            TestTrue(TEXT("Closing Pit restores the triggered trap visual"), Restored->SetPitOpen(PitId, false));
            TestFalse(TEXT("Trap appears again when associated Pit is closed"), RestoredTrapVisual->IsHidden());
            TestEqual(TEXT("Trap regains mouse query after Pit closes"),
                RestoredTrapVisual->MeshComponent->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);

            TestTrue(TEXT("Reopening Pit hides the same trap again"), Restored->SetPitOpen(PitId, true));
            TestTrue(TEXT("Trap hides again when Pit opens via normal API"), RestoredTrapVisual->IsHidden());
            TestEqual(TEXT("Reopening Pit removes mouse collision again"),
                RestoredTrapVisual->MeshComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
        }

        for (const FGuid OtherTrapId : { Ordinary.InstanceId, Unpaired.InstanceId })
        {
            TestTrue(TEXT("Other trap can be triggered independently"), Restored->TryRecordDungeonTrapTriggered(OtherTrapId, nullptr));
            AGridRuntimeObjectActor* OtherVisual =
                Restored->FindRuntimeObjectActor<AGridRuntimeObjectActor>(OtherTrapId);
            if (TestNotNull(TEXT("Other trap keeps a materialized visual"), OtherVisual) &&
                TestNotNull(TEXT("Other trap keeps its mesh"), OtherVisual ? OtherVisual->MeshComponent.Get() : nullptr))
            {
                TestFalse(TEXT("Ordinary / unpaired triggered trap remains visible"), OtherVisual->IsHidden());
                TestEqual(TEXT("Ordinary / unpaired trap remains mouse-queryable"),
                    OtherVisual->MeshComponent->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
            }
        }
    }

    Cleanup();
    return true;
}
#endif
