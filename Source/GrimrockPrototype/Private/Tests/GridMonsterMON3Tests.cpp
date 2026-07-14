#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON3OccupancyRegistryTest,
    "Grimrock.Monsters.MON3.OccupancyRegistry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON3OccupancyRegistryTest::RunTest (const FString& Parameters)
{
    FGridMonsterOccupancyRegistry Registry;
    const FGuid RatA = FGuid::NewGuid ();
    const FGuid RatB = FGuid::NewGuid ();
    const FIntPoint CellA (4, 4);
    const FIntPoint CellB (5, 4);
    const FIntPoint CellC (6, 4);

    TestTrue (TEXT ("The first rat occupies its initial cell"), Registry.TryRegisterMonster (RatA, CellA));
    TestFalse (TEXT ("A second rat cannot occupy the same cell"), Registry.TryRegisterMonster (RatB, CellA));
    TestTrue (TEXT ("The second rat occupies another cell"), Registry.TryRegisterMonster (RatB, CellC));
    TestEqual (TEXT ("Two occupied cells are registered"), Registry.GetOccupiedCellCount (), 2);

    TestTrue (TEXT ("The first rat reserves its destination"), Registry.TryReserveCell (RatA, CellB));
    TestFalse (TEXT ("The second rat cannot reserve the same destination"), Registry.TryReserveCell (RatB, CellB));
    TestTrue (TEXT ("The reserved cell is blocked for another rat"), Registry.IsCellBlocked (CellB, RatB));
    TestFalse (TEXT ("A rat ignores its own reservation when queried"), Registry.IsCellBlocked (CellB, RatA));

    TestTrue (TEXT ("The first rat commits its reserved move"), Registry.CommitReservation (RatA, CellA, CellB));
    TestFalse (TEXT ("The old cell becomes free"), Registry.IsCellOccupied (CellA));
    TestTrue (TEXT ("The destination becomes occupied"), Registry.IsCellOccupied (CellB));
    TestFalse (TEXT ("The committed reservation is removed"), Registry.IsCellReserved (CellB));

    TestTrue (TEXT ("The second rat can reserve the released old cell"), Registry.TryReserveCell (RatB, CellA));
    Registry.CancelReservation (RatB);
    TestFalse (TEXT ("Cancelling a move releases the reservation"), Registry.IsCellReserved (CellA));

    Registry.UnregisterMonster (RatA);
    TestFalse (TEXT ("Unregistering a monster releases its occupied cell"), Registry.IsCellOccupied (CellB));
    TestEqual (TEXT ("Only the second rat remains registered"), Registry.GetOccupiedCellCount (), 1);

    Registry.Reset ();
    TestEqual (TEXT ("A reset clears all occupied cells"), Registry.GetOccupiedCellCount (), 0);
    TestEqual (TEXT ("A reset clears all reserved cells"), Registry.GetReservedCellCount (), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON3InvalidTransitionsTest,
    "Grimrock.Monsters.MON3.InvalidTransitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON3InvalidTransitionsTest::RunTest (const FString& Parameters)
{
    FGridMonsterOccupancyRegistry Registry;
    const FGuid Rat = FGuid::NewGuid ();
    const FIntPoint Start (2, 2);
    const FIntPoint Target (3, 2);

    TestFalse (TEXT ("An unregistered monster cannot reserve a cell"), Registry.TryReserveCell (Rat, Target));
    TestTrue (TEXT ("The monster can be registered"), Registry.TryRegisterMonster (Rat, Start));
    TestFalse (TEXT ("A move cannot be committed without a reservation"), Registry.CommitReservation (Rat, Start, Target));
    TestTrue (TEXT ("The destination can be reserved"), Registry.TryReserveCell (Rat, Target));
    TestFalse (TEXT ("A reservation cannot be committed from the wrong source"),
        Registry.CommitReservation (Rat, FIntPoint (1, 2), Target));

    Registry.UnregisterMonster (Rat);
    TestFalse (TEXT ("Unregistering also removes a pending reservation"), Registry.IsCellReserved (Target));
    TestFalse (TEXT ("Unregistering releases the original occupied cell"), Registry.IsCellOccupied (Start));

    return true;
}

#endif
