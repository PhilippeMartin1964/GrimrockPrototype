#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "GridMonsterPathfinder.generated.h"

USTRUCT (BlueprintType)
struct FGridMonsterPathQuery
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Pathfinding")
    FIntPoint Start = FIntPoint::ZeroValue;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Pathfinding")
    TArray<FIntPoint> Goals;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Pathfinding", meta = (ClampMin = "1"))
    int32 MaxVisitedCells = 1024;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Pathfinding")
    bool bAllowBlockedGoal = false;
};

USTRUCT (BlueprintType)
struct FGridMonsterPathResult
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Monster|Pathfinding")
    bool bFound = false;

    UPROPERTY (BlueprintReadOnly, Category = "Monster|Pathfinding")
    FIntPoint ReachedGoal = FIntPoint::ZeroValue;

    /** Ordered cells from the first step to the goal. The start cell is never included. */
    UPROPERTY (BlueprintReadOnly, Category = "Monster|Pathfinding")
    TArray<FIntPoint> Cells;

    UPROPERTY (BlueprintReadOnly, Category = "Monster|Pathfinding")
    int32 VisitedCellCount = 0;

    void Reset ()
    {
        bFound = false;
        ReachedGoal = FIntPoint::ZeroValue;
        Cells.Reset ();
        VisitedCellCount = 0;
    }
};

/**
 * Runtime-independent callbacks used by the deterministic BFS.
 * Tests can provide a small synthetic grid, while gameplay supplies the real
 * AGridLevelRuntimeActor and monster occupancy rules.
 */
struct GRIMROCKPROTOTYPE_API FGridMonsterPathContext
{
    TFunction<bool (const FIntPoint&)> IsValidCell;
    TFunction<bool (const FIntPoint&)> IsWalkableCell;
    TFunction<bool (const FIntPoint&, const FIntPoint&)> CanTraverse;
    TFunction<bool (const FIntPoint&)> IsCellBlocked;

    bool IsComplete () const
    {
        return IsValidCell && IsWalkableCell && CanTraverse && IsCellBlocked;
    }
};

/** Pure, deterministic breadth-first pathfinder for the 32 x 32 dungeon grid. */
class GRIMROCKPROTOTYPE_API FGridMonsterPathfinder
{
public:
    static bool FindPath (
        const FGridMonsterPathQuery& Query,
        const FGridMonsterPathContext& Context,
        FGridMonsterPathResult& OutResult);

    static const TArray<EGridEdge>& GetOrderedDirections ();
    static FIntPoint GetNeighborCell (const FIntPoint& Cell, EGridEdge Direction);
    static EGridEdge GetDirectionBetweenAdjacentCells (const FIntPoint& From, const FIntPoint& To);
    static int32 ManhattanDistance (const FIntPoint& A, const FIntPoint& B);
};

/** Pure grid perception helpers used by UGridMonsterBehaviorComponent and tests. */
class GRIMROCKPROTOTYPE_API FGridMonsterPerception
{
public:
    /**
     * MON4 sight is deliberately orthogonal: the target must share X or Y with
     * the observer and every crossed edge must be open. This helper remains as
     * the geometry-only contract for legacy tests and diagnostics.
     */
    static bool HasStraightLineOfSight (
        const FIntPoint& ObserverCell,
        const FIntPoint& TargetCell,
        int32 SightRangeCells,
        const TFunction<bool (const FIntPoint&, const FIntPoint&)>& CanTraverse);

    /** True only when TargetCell lies on the cardinal ray emitted by Facing. */
    static bool IsTargetInFacingDirection (
        const FIntPoint& ObserverCell,
        EGridEdge Facing,
        const FIntPoint& TargetCell);

    /** MON14.2 sight: straight-line geometry constrained by monster Facing. */
    static bool HasDirectionalLineOfSight (
        const FIntPoint& ObserverCell,
        EGridEdge Facing,
        const FIntPoint& TargetCell,
        int32 SightRangeCells,
        const TFunction<bool (const FIntPoint&, const FIntPoint&)>& CanTraverse);

    /** Hearing remains omnidirectional and Manhattan-distance based. */
    static bool CanHear (
        const FIntPoint& ObserverCell,
        const FIntPoint& TargetCell,
        int32 HearingRangeCells);
};
