#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"

namespace
{
    struct FTestGrid
    {
        int32 Width = 5;
        int32 Height = 5;
        TSet<FIntPoint> NonWalkableCells;
        TSet<FIntPoint> BlockedCells;
        TSet<FString> BlockedEdges;

        static FString MakeEdgeKey (const FIntPoint& A, const FIntPoint& B)
        {
            const bool bAFirst = A.X < B.X || (A.X == B.X && A.Y <= B.Y);
            const FIntPoint First = bAFirst ? A : B;
            const FIntPoint Second = bAFirst ? B : A;
            return FString::Printf (TEXT ("%d,%d-%d,%d"), First.X, First.Y, Second.X, Second.Y);
        }

        bool IsValid (const FIntPoint& Cell) const
        {
            return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Width && Cell.Y < Height;
        }

        bool IsWalkable (const FIntPoint& Cell) const
        {
            return IsValid (Cell) && !NonWalkableCells.Contains (Cell);
        }

        bool CanTraverse (const FIntPoint& From, const FIntPoint& To) const
        {
            return FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells (From, To) != EGridEdge::None &&
                !BlockedEdges.Contains (MakeEdgeKey (From, To));
        }

        FGridMonsterPathContext MakeContext () const
        {
            FGridMonsterPathContext Context;
            Context.IsValidCell = [this] (const FIntPoint& Cell) { return IsValid (Cell); };
            Context.IsWalkableCell = [this] (const FIntPoint& Cell) { return IsWalkable (Cell); };
            Context.CanTraverse = [this] (const FIntPoint& From, const FIntPoint& To)
            {
                return CanTraverse (From, To);
            };
            Context.IsCellBlocked = [this] (const FIntPoint& Cell)
            {
                return BlockedCells.Contains (Cell);
            };
            return Context;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON4PathfinderTest,
    "Grimrock.Monsters.MON4.Pathfinder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON4PathfinderTest::RunTest (const FString& Parameters)
{
    FTestGrid Grid;

    FGridMonsterPathQuery Query;
    Query.Start = FIntPoint (1, 1);
    Query.Goals = { FIntPoint (1, 4) };

    FGridMonsterPathResult Result;
    TestTrue (TEXT ("Direct path is found"),
        FGridMonsterPathfinder::FindPath (Query, Grid.MakeContext (), Result));
    TestEqual (TEXT ("Start cell is excluded"), Result.Cells.Num (), 3);
    if (Result.Cells.Num () >= 1)
    {
        TestTrue (TEXT ("First direct step is North"), Result.Cells[0] == FIntPoint (1, 2));
    }
    TestTrue (TEXT ("Reached direct goal"), Result.ReachedGoal == FIntPoint (1, 4));

    Grid.BlockedEdges.Add (FTestGrid::MakeEdgeKey (FIntPoint (1, 1), FIntPoint (1, 2)));
    Query.Goals = { FIntPoint (2, 2) };
    Result.Reset ();
    TestTrue (TEXT ("BFS routes around a blocked edge"),
        FGridMonsterPathfinder::FindPath (Query, Grid.MakeContext (), Result));
    if (Result.Cells.Num () >= 1)
    {
        TestTrue (TEXT ("Stable order chooses East when North is blocked"), Result.Cells[0] == FIntPoint (2, 1));
    }
    TestEqual (TEXT ("Detour reaches goal in two steps"), Result.Cells.Num (), 2);

    Grid.BlockedEdges.Reset ();
    Grid.BlockedCells.Add (FIntPoint (1, 2));
    Query.Goals = { FIntPoint (1, 3) };
    Result.Reset ();
    TestTrue (TEXT ("Occupied cell is avoided"),
        FGridMonsterPathfinder::FindPath (Query, Grid.MakeContext (), Result));
    TestFalse (TEXT ("Occupied cell is not in the path"), Result.Cells.Contains (FIntPoint (1, 2)));

    Grid.BlockedCells.Reset ();
    Grid.BlockedCells.Add (FIntPoint (2, 1));
    Query.Start = FIntPoint (1, 1);
    Query.Goals = { FIntPoint (3, 1) };
    Result.Reset ();
    TestTrue (TEXT ("Reserved cell is avoided like an occupied cell"),
        FGridMonsterPathfinder::FindPath (Query, Grid.MakeContext (), Result));
    TestFalse (TEXT ("Reserved cell is not in the path"), Result.Cells.Contains (FIntPoint (2, 1)));

    Grid.BlockedCells.Reset ();
    Grid.BlockedCells.Add (FIntPoint (1, 2));
    Query.Start = FIntPoint (1, 1);
    Query.Goals = { FIntPoint (1, 2) };
    Query.bAllowBlockedGoal = false;
    Result.Reset ();
    TestFalse (TEXT ("Blocked goal is rejected by default"),
        FGridMonsterPathfinder::FindPath (Query, Grid.MakeContext (), Result));

    Query.bAllowBlockedGoal = true;
    Result.Reset ();
    TestTrue (TEXT ("Explicitly allowed blocked goal can be reached"),
        FGridMonsterPathfinder::FindPath (Query, Grid.MakeContext (), Result));

    Grid.BlockedCells.Reset ();
    Query.Start = FIntPoint (0, 0);
    Query.Goals = { FIntPoint (0, 2), FIntPoint (2, 0) };
    Query.bAllowBlockedGoal = false;
    Result.Reset ();
    TestTrue (TEXT ("One of multiple goals is found"),
        FGridMonsterPathfinder::FindPath (Query, Grid.MakeContext (), Result));
    TestTrue (TEXT ("N,E,S,W order deterministically selects North goal"),
        Result.ReachedGoal == FIntPoint (0, 2));

    FTestGrid ClosedGrid;
    ClosedGrid.Width = 3;
    ClosedGrid.Height = 3;
    ClosedGrid.BlockedEdges.Add (FTestGrid::MakeEdgeKey (FIntPoint (1, 1), FIntPoint (1, 2)));
    ClosedGrid.BlockedEdges.Add (FTestGrid::MakeEdgeKey (FIntPoint (1, 1), FIntPoint (2, 1)));
    ClosedGrid.BlockedEdges.Add (FTestGrid::MakeEdgeKey (FIntPoint (1, 1), FIntPoint (1, 0)));
    ClosedGrid.BlockedEdges.Add (FTestGrid::MakeEdgeKey (FIntPoint (1, 1), FIntPoint (0, 1)));

    Query.Start = FIntPoint (1, 1);
    Query.Goals = { FIntPoint (2, 2) };
    Result.Reset ();
    TestFalse (TEXT ("No path is returned from a sealed cell"),
        FGridMonsterPathfinder::FindPath (Query, ClosedGrid.MakeContext (), Result));
    TestTrue (TEXT ("Failed query returns an empty path"), Result.Cells.IsEmpty ());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON4PerceptionTest,
    "Grimrock.Monsters.MON4.Perception",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON4PerceptionTest::RunTest (const FString& Parameters)
{
    FTestGrid Grid;
    const auto CanTraverse = [&Grid] (const FIntPoint& From, const FIntPoint& To)
    {
        return Grid.CanTraverse (From, To);
    };

    TestTrue (TEXT ("Straight unobstructed party is visible"),
        FGridMonsterPerception::HasStraightLineOfSight (
            FIntPoint (1, 1), FIntPoint (1, 4), 5, CanTraverse));

    Grid.BlockedEdges.Add (FTestGrid::MakeEdgeKey (FIntPoint (1, 2), FIntPoint (1, 3)));
    TestFalse (TEXT ("Wall or closed door blocks sight"),
        FGridMonsterPerception::HasStraightLineOfSight (
            FIntPoint (1, 1), FIntPoint (1, 4), 5, CanTraverse));

    Grid.BlockedEdges.Reset ();
    TestFalse (TEXT ("MON4 sight does not see around a corner"),
        FGridMonsterPerception::HasStraightLineOfSight (
            FIntPoint (1, 1), FIntPoint (3, 2), 5, CanTraverse));
    TestFalse (TEXT ("Sight range is enforced"),
        FGridMonsterPerception::HasStraightLineOfSight (
            FIntPoint (1, 1), FIntPoint (1, 7), 5, CanTraverse));

    TestTrue (TEXT ("Hearing works around a corner within Manhattan range"),
        FGridMonsterPerception::CanHear (FIntPoint (1, 1), FIntPoint (3, 2), 3));
    TestFalse (TEXT ("Hearing range is enforced"),
        FGridMonsterPerception::CanHear (FIntPoint (1, 1), FIntPoint (4, 2), 3));

    return true;
}

#endif
