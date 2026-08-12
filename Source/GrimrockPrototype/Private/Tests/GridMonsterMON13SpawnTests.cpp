#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace
{
    UGridLevelAsset* MakeMON13Level (UObject* Outer)
    {
        UGridLevelAsset* Level = NewObject<UGridLevelAsset> (Outer);
        Level->Width = 4;
        Level->Height = 4;
        Level->EnsureCellCount ();
        for (FGridLevelCellData& Cell : Level->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
            Cell.bBlocksOccupancy = false;
        }
        return Level;
    }

    UGridMonsterDefinitionAsset* MakeMON13Definition (
        UObject* Outer,
        FName MonsterId = TEXT ("MON13_Rat"))
    {
        UGridMonsterDefinitionAsset* Definition =
            NewObject<UGridMonsterDefinitionAsset> (Outer);
        Definition->MonsterId = MonsterId;
        Definition->DisplayName =
            FText::FromString (TEXT ("Rat MON13"));
        Definition->CategoryId = TEXT ("Vermin");
        Definition->DangerLevel = 1;
        Definition->MaxHealth = 12;
        Definition->ActionPointsPerTurn = 2;
        Definition->GridFootprint = FIntPoint (1, 1);
        Definition->DeathExpectedDuration = 1.0f;
        return Definition;
    }

    FGridLevelObjectData MakeMON13Spawn (
        UGridMonsterDefinitionAsset* Definition,
        FGuid SpawnId,
        FIntPoint Cell = FIntPoint (1, 1))
    {
        FGridLevelObjectData Spawn;
        Spawn.ObjectId = SpawnId;
        Spawn.Type = EGridLevelObjectType::MonsterSpawn;
        Spawn.CellX = Cell.X;
        Spawn.CellY = Cell.Y;
        Spawn.Edge = EGridEdge::None;
        Spawn.InitialFacing = EGridEdge::North;
        Spawn.MonsterDefinitionAsset = Definition;
        Spawn.MonsterDefinitionId = Definition
            ? Definition->MonsterId
            : NAME_None;
        Spawn.EncounterGroupId = TEXT ("Encounter_MON13");
        Spawn.bInitiallyEnabled = true;
        return Spawn;
    }

    bool HasErrorContaining (
        const TArray<FString>& Errors,
        const TCHAR* ExpectedText)
    {
        return Errors.ContainsByPredicate (
            [ExpectedText] (const FString& Error)
        {
            return Error.Contains (ExpectedText);
        });
    }

    struct FGridMON132TestWorld
    {
        UWorld* World = nullptr;

        explicit FGridMON132TestWorld (
            EWorldType::Type WorldType)
        {
            const UWorld::InitializationValues InitializationValues =
                UWorld::InitializationValues ()
                    .AllowAudioPlayback (false)
                    .RequiresHitProxies (false)
                    .CreatePhysicsScene (false)
                    .CreateNavigation (false)
                    .CreateAISystem (false)
                    .ShouldSimulatePhysics (false)
                    .SetTransactional (false);
            World = UWorld::CreateWorld (
                WorldType,
                false,
                FName (*FString::Printf (
                    TEXT ("MON132TestWorld_%s"),
                    *FGuid::NewGuid ().ToString (
                        EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &InitializationValues);
            if (!World || !GEngine)
            {
                return;
            }

            FWorldContext& Context =
                GEngine->CreateNewWorldContext (WorldType);
            Context.SetCurrentWorld (World);
        }

        ~FGridMON132TestWorld ()
        {
            if (!World)
            {
                return;
            }
            World->DestroyWorld (false);
            if (GEngine)
            {
                GEngine->DestroyWorldContext (World);
            }
        }
    };

    int32 CountMON132WorldMonsters (UWorld* World)
    {
        int32 Count = 0;
        if (!World)
        {
            return Count;
        }
        for (TActorIterator<AGridMonsterActor> It (World); It; ++It)
        {
            Count += IsValid (*It) ? 1 : 0;
        }
        return Count;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON131PersistentModelTest,
    "Grimrock.Monsters.MON13.1.PersistentModel",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON131PersistentModelTest::RunTest (
    const FString& Parameters)
{
    UGridLevelAsset* Level = MakeMON13Level (GetTransientPackage ());
    UGridMonsterDefinitionAsset* Definition =
        MakeMON13Definition (Level);

    TestEqual (
        TEXT ("MonsterDefinition defaults to the native monster actor"),
        Definition->MonsterActorClass.Get (),
        AGridMonsterActor::StaticClass ());

    FGridLevelObjectData Spawn = MakeMON13Spawn (
        Definition,
        FGuid (),
        FIntPoint (2, 1));
    Spawn.MonsterDefinitionId = NAME_None;
    Spawn.InitialFacing = EGridEdge::None;
    Spawn.LocalYaw = 90.0f;

    const FGuid SpawnId = Level->AddObject (Spawn);
    const FGridLevelObjectData* StoredSpawn =
        Level->FindMonsterSpawnById (SpawnId);

    TestTrue (TEXT ("AddObject creates a stable SpawnId"),
        SpawnId.IsValid ());
    TestNotNull (TEXT ("Spawn is found by its persistent id"),
        StoredSpawn);
    if (!StoredSpawn)
    {
        return false;
    }

    TestEqual (TEXT ("Legacy yaw migrates to InitialFacing"),
        StoredSpawn->InitialFacing,
        EGridEdge::East);
    TestEqual (TEXT ("Definition id is synchronized from the DataAsset"),
        StoredSpawn->MonsterDefinitionId,
        Definition->MonsterId);
    TestEqual (TEXT ("Encounter id remains persistent"),
        StoredSpawn->EncounterGroupId,
        FName (TEXT ("Encounter_MON13")));
    TestTrue (TEXT ("Initial enabled state remains persistent"),
        StoredSpawn->bInitiallyEnabled);

    TArray<FString> Errors;
    TestTrue (TEXT ("Complete MonsterSpawn model validates"),
        Level->ValidateMonsterSpawns (Errors));
    TestEqual (TEXT ("Valid model has no errors"),
        Errors.Num (),
        0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON131ValidationTest,
    "Grimrock.Monsters.MON13.1.Validation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON131ValidationTest::RunTest (
    const FString& Parameters)
{
    UGridLevelAsset* Level = MakeMON13Level (GetTransientPackage ());
    UGridMonsterDefinitionAsset* Definition =
        MakeMON13Definition (Level);
    const FGuid SharedId (13, 1, 1, 1);

    FGridLevelObjectData Valid = MakeMON13Spawn (
        Definition,
        SharedId,
        FIntPoint (1, 1));
    Level->Objects.Add (Valid);

    FGridLevelObjectData DuplicateId = MakeMON13Spawn (
        Definition,
        SharedId,
        FIntPoint (2, 1));
    DuplicateId.bInitiallyEnabled = false;
    Level->Objects.Add (DuplicateId);

    FGridLevelObjectData DuplicateCell = MakeMON13Spawn (
        Definition,
        FGuid (13, 1, 1, 2),
        FIntPoint (1, 1));
    Level->Objects.Add (DuplicateCell);

    FGridLevelObjectData InvalidPlacement = MakeMON13Spawn (
        nullptr,
        FGuid (13, 1, 1, 3),
        FIntPoint (3, 3));
    InvalidPlacement.Edge = EGridEdge::North;
    InvalidPlacement.InitialFacing = EGridEdge::None;
    InvalidPlacement.MonsterDefinitionId = NAME_None;
    Level->Cells[Level->GetIndex (3, 3)].bBlocksOccupancy = true;
    Level->Objects.Add (InvalidPlacement);

    FGridLevelObjectData MismatchedDefinition = MakeMON13Spawn (
        Definition,
        FGuid (13, 1, 1, 4),
        FIntPoint (0, 1));
    MismatchedDefinition.MonsterDefinitionId = TEXT ("MON13_Other");
    Level->Objects.Add (MismatchedDefinition);

    FGridLevelObjectData Outside = MakeMON13Spawn (
        Definition,
        FGuid (13, 1, 1, 5),
        FIntPoint (8, 8));
    Level->Objects.Add (Outside);

    TArray<FString> Errors;
    TestFalse (TEXT ("Invalid MonsterSpawn set is rejected"),
        Level->ValidateMonsterSpawns (Errors));
    TestTrue (TEXT ("Duplicate SpawnId is reported"),
        HasErrorContaining (Errors, TEXT ("unique ObjectId/SpawnId")));
    TestTrue (TEXT ("Duplicate enabled cell is reported"),
        HasErrorContaining (Errors, TEXT ("shares initial cell")));
    TestTrue (TEXT ("Blocked cell is reported"),
        HasErrorContaining (Errors, TEXT ("allows occupancy")));
    TestTrue (TEXT ("Edge placement is rejected"),
        HasErrorContaining (Errors, TEXT ("requires Edge=None")));
    TestTrue (TEXT ("Non-cardinal facing is rejected"),
        HasErrorContaining (Errors, TEXT ("cardinal InitialFacing")));
    TestTrue (TEXT ("Missing definition is reported"),
        HasErrorContaining (Errors, TEXT ("requires MonsterDefinitionAsset")));
    TestTrue (TEXT ("Definition id mismatch is reported"),
        HasErrorContaining (Errors, TEXT ("asset resolves")));
    TestTrue (TEXT ("Out-of-bounds spawn is reported"),
        HasErrorContaining (Errors, TEXT ("outside grid bounds")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON131PaletteContractTest,
    "Grimrock.Monsters.MON13.1.PaletteContract",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON131PaletteContractTest::RunTest (
    const FString& Parameters)
{
    UGridObjectPaletteAsset* Palette =
        NewObject<UGridObjectPaletteAsset> (GetTransientPackage ());
    UGridObjectArchetypeAsset* Archetype =
        NewObject<UGridObjectArchetypeAsset> (Palette);
    Archetype->ArchetypeId = TEXT ("Monster_RatGiant");
    Archetype->SupportedType = EGridLevelObjectType::MonsterSpawn;
    Archetype->PlacementKind = EGridObjectPlacementKind::Center;

    FGridObjectPaletteEntry Entry;
    Entry.EntryId = TEXT ("MON_RatGiant");
    Entry.DefaultArchetype = Archetype;
    Palette->Entries.Add (Entry);

    TArray<FGridArchetypeValidationMessage> Messages;
    TestFalse (TEXT ("Monster palette entry requires a definition"),
        Palette->ValidatePalette (Messages));

    Palette->Entries[0].DefaultMonsterDefinition =
        MakeMON13Definition (Palette, TEXT ("MON_RatGiant"));
    TestTrue (TEXT ("Complete monster palette entry validates"),
        Palette->ValidatePalette (Messages));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON132RuntimePipelineTest,
    "Grimrock.Monsters.MON13.2.RuntimePipeline",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON132RuntimePipelineTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON132TestWorld TestWorld (EWorldType::Game);
    if (!TestWorld.World)
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    TestNotNull (TEXT ("Runtime actor is created"), Runtime);
    if (!Runtime)
    {
        return false;
    }

    UGridLevelAsset* Level = MakeMON13Level (Runtime);
    Runtime->LevelAsset = Level;
    UGridMonsterDefinitionAsset* Definition =
        MakeMON13Definition (Runtime, TEXT ("MON132_Rat"));
    const FGuid SpawnId (13, 2, 1, 1);
    FGridLevelObjectData Spawn = MakeMON13Spawn (
        Definition,
        SpawnId,
        FIntPoint (2, 1));
    Spawn.InitialFacing = EGridEdge::East;
    Spawn.LocalYaw = 0.0f;
    Spawn.EncounterGroupId = TEXT ("Encounter_MON132");
    Level->Objects.Add (Spawn);

    AddExpectedMessage (
        TEXT ("[GridMonsterSpawn] PresentationWarning"),
        EAutomationExpectedMessageFlags::Contains,
        2);
    Runtime->RebuildLevel ();
    AGridMonsterActor* FirstMonster =
        Runtime->FindSpawnedMonsterActor (SpawnId);
    TestNotNull (TEXT ("MonsterSpawn creates its Actor"),
        FirstMonster);
    if (!FirstMonster)
    {
        return false;
    }

    TestEqual (TEXT ("Exactly one placement Actor is tracked"),
        Runtime->GetSpawnedMonsterActorCount (),
        1);
    TestEqual (TEXT ("Valid spawn reports no failure"),
        Runtime->GetMonsterSpawnFailureCount (),
        0);
    TestEqual (TEXT ("Definition is transmitted"),
        FirstMonster->MonsterDefinition.Get (),
        Definition);
    TestEqual (TEXT ("Definition chooses the spawned Actor class"),
        FirstMonster->GetClass (),
        Definition->MonsterActorClass.Get ());
    TestEqual (TEXT ("SpawnId remains the persistence id"),
        FirstMonster->ResolvePersistenceId (),
        SpawnId);
    TestTrue (TEXT ("Actor identity comes from MonsterSpawn"),
        FirstMonster->HasMonsterSpawnIdentity ());
    TestEqual (TEXT ("Cell is transmitted"),
        FirstMonster->CurrentCell,
        FIntPoint (2, 1));
    TestEqual (TEXT ("InitialFacing is authoritative"),
        FirstMonster->Facing,
        EGridEdge::East);
    TestEqual (TEXT ("Encounter group is transmitted"),
        FirstMonster->EncounterGroupId,
        FName (TEXT ("Encounter_MON132")));
    TestEqual (TEXT ("Combat health is initialized"),
        FirstMonster->CurrentHealth,
        Definition->MaxHealth);
    TestTrue (TEXT ("Combat stats are initialized"),
        FirstMonster->bCombatStatsInitialized);
    TestTrue (TEXT ("Actor is centered on its cell"),
        FirstMonster->GetActorLocation ().Equals (
            Runtime->GetCellCenterWorld (2, 1),
            KINDA_SMALL_NUMBER));
    TestTrue (TEXT ("Actor rotation follows InitialFacing"),
        FMath::IsNearlyEqual (
            FirstMonster->GetActorRotation ().Yaw,
            GridDirectionUtils::ToYaw (
                EGridEdge::East)));

    Runtime->RebuildLevel ();
    AGridMonsterActor* RebuiltMonster =
        Runtime->FindSpawnedMonsterActor (SpawnId);
    TestNotNull (TEXT ("Rebuild recreates the placement Actor"),
        RebuiltMonster);
    TestTrue (TEXT ("Rebuild replaces rather than duplicates"),
        RebuiltMonster != FirstMonster);
    TestFalse (TEXT ("Previous generated Actor is destroyed"),
        IsValid (FirstMonster));
    TestEqual (TEXT ("Rebuild still tracks one Actor"),
        Runtime->GetSpawnedMonsterActorCount (),
        1);
    TestEqual (TEXT ("Only one live monster remains in the world"),
        CountMON132WorldMonsters (TestWorld.World),
        1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON132AtomicFailureTest,
    "Grimrock.Monsters.MON13.2.AtomicFailure",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON132AtomicFailureTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON132TestWorld TestWorld (EWorldType::Game);
    if (!TestWorld.World)
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    UGridLevelAsset* Level = MakeMON13Level (Runtime);
    Runtime->LevelAsset = Level;
    UGridMonsterDefinitionAsset* Definition =
        MakeMON13Definition (Runtime, TEXT ("MON132_AtomicRat"));

    const FGuid ValidId (13, 2, 2, 1);
    Level->Objects.Add (MakeMON13Spawn (
        Definition,
        ValidId,
        FIntPoint (0, 0)));

    FGridLevelObjectData Mismatched = MakeMON13Spawn (
        Definition,
        FGuid (13, 2, 2, 2),
        FIntPoint (1, 0));
    Mismatched.MonsterDefinitionId = TEXT ("MON132_WrongId");
    Level->Objects.Add (Mismatched);

    Level->Objects.Add (MakeMON13Spawn (
        Definition,
        ValidId,
        FIntPoint (2, 0)));

    FGridLevelObjectData DuplicateCell = MakeMON13Spawn (
        Definition,
        FGuid (13, 2, 2, 5),
        FIntPoint (0, 0));
    Level->Objects.Add (DuplicateCell);

    FGridLevelObjectData Blocked = MakeMON13Spawn (
        Definition,
        FGuid (13, 2, 2, 3),
        FIntPoint (3, 0));
    Level->Cells[Level->GetIndex (3, 0)].bBlocksOccupancy = true;
    Level->Objects.Add (Blocked);

    FGridLevelObjectData Disabled = MakeMON13Spawn (
        nullptr,
        FGuid (13, 2, 2, 4),
        FIntPoint (0, 1));
    Disabled.bInitiallyEnabled = false;
    Level->Objects.Add (Disabled);

    AddExpectedError (
        TEXT ("[GridMonsterSpawn] Skipped"),
        EAutomationExpectedErrorFlags::Contains,
        4);
    AddExpectedMessage (
        TEXT ("[GridMonsterSpawn] PresentationWarning"),
        EAutomationExpectedMessageFlags::Contains,
        1);
    Runtime->RebuildLevel ();
    TestEqual (TEXT ("Only the valid placement creates an Actor"),
        Runtime->GetSpawnedMonsterActorCount (),
        1);
    TestEqual (TEXT ("Every enabled invalid placement is counted"),
        Runtime->GetMonsterSpawnFailureCount (),
        4);
    TestNotNull (TEXT ("Valid placement remains available"),
        Runtime->FindSpawnedMonsterActor (ValidId));
    TestNull (TEXT ("Mismatched definition creates no partial Actor"),
        Runtime->FindSpawnedMonsterActor (
            Mismatched.ObjectId));
    TestNull (TEXT ("Blocked placement creates no partial Actor"),
        Runtime->FindSpawnedMonsterActor (
            Blocked.ObjectId));
    TestNull (TEXT ("Occupied placement creates no partial Actor"),
        Runtime->FindSpawnedMonsterActor (
            DuplicateCell.ObjectId));
    TestNull (TEXT ("Disabled placement creates no Actor"),
        Runtime->FindSpawnedMonsterActor (
            Disabled.ObjectId));
    TestEqual (TEXT ("Invalid placements leak no world Actor"),
        CountMON132WorldMonsters (TestWorld.World),
        1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON132EditorPreviewTest,
    "Grimrock.Monsters.MON13.2.EditorPreview",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON132EditorPreviewTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON132TestWorld TestWorld (EWorldType::Editor);
    if (!TestWorld.World)
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    UGridLevelAsset* Level = MakeMON13Level (Runtime);
    Runtime->LevelAsset = Level;
    UGridMonsterDefinitionAsset* Definition =
        MakeMON13Definition (Runtime, TEXT ("MON132_PreviewRat"));
    USkeletalMesh* PreviewMesh =
        NewObject<USkeletalMesh> (Definition);
    Definition->SkeletalMesh =
        TSoftObjectPtr<USkeletalMesh> (PreviewMesh);
    Definition->VisualOffset = FVector (1.0f, 2.0f, 3.0f);
    Definition->VisualScale = FVector (1.5f);

    const FGuid SpawnId (13, 2, 3, 1);
    FGridLevelObjectData Spawn = MakeMON13Spawn (
        Definition,
        SpawnId,
        FIntPoint (1, 2));
    Spawn.InitialFacing = EGridEdge::South;
    Level->Objects.Add (Spawn);

    Runtime->RebuildLevel ();
    AGridEditorPreviewObjectActor* PreviewActor = nullptr;
    for (TActorIterator<AGridEditorPreviewObjectActor> It (
        TestWorld.World); It; ++It)
    {
        if (IsValid (*It) &&
            It->GetOwner () == Runtime &&
            It->ObjectId == SpawnId)
        {
            PreviewActor = *It;
            break;
        }
    }

    TestNotNull (TEXT ("Editor rebuild creates a monster preview"),
        PreviewActor);
    if (!PreviewActor)
    {
        return false;
    }
    TestNotNull (TEXT ("Monster preview has a skeletal component"),
        PreviewActor->SkeletalMeshComponent);
    TestEqual (TEXT ("Preview resolves the definition skeletal mesh"),
        PreviewActor->SkeletalMeshComponent->GetSkeletalMeshAsset (),
        PreviewMesh);
    TestTrue (TEXT ("Preview is centered on the placement cell"),
        PreviewActor->GetActorLocation ().Equals (
            Runtime->GetCellCenterWorld (1, 2),
            KINDA_SMALL_NUMBER));
    TestTrue (TEXT ("Preview rotation follows InitialFacing"),
        FMath::IsNearlyEqual (
            PreviewActor->GetActorRotation ().Yaw,
            GridDirectionUtils::ToYaw (
                EGridEdge::South)));
    TestEqual (TEXT ("Preview receives the visual offset"),
        PreviewActor->SkeletalMeshComponent->GetRelativeLocation (),
        Definition->VisualOffset);
    TestEqual (TEXT ("Preview receives the visual scale"),
        PreviewActor->SkeletalMeshComponent->GetRelativeScale3D (),
        Definition->VisualScale);
    Runtime->SetEditorSelectedObject (SpawnId);
    TestTrue (TEXT ("Selection stencil reaches the skeletal preview"),
        PreviewActor->SkeletalMeshComponent->bRenderCustomDepth);
    TestEqual (TEXT ("Editor preview creates no gameplay monster"),
        CountMON132WorldMonsters (TestWorld.World),
        0);
    return true;
}

#endif
