#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
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

#endif
