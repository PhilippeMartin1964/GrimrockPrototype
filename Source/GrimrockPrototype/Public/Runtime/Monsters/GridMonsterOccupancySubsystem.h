#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridMonsterOccupancySubsystem.generated.h"

class AGridMonsterActor;

/**
 * Pure deterministic occupancy model used by the world subsystem and MON3 tests.
 * It deliberately stores only stable monster ids so the rules remain independent
 * from Unreal object lifetime and frame rate.
 */
class GRIMROCKPROTOTYPE_API FGridMonsterOccupancyRegistry
{
public:
    bool TryRegisterMonster (const FGuid& MonsterId, const FIntPoint& Cell);
    bool TryReserveCell (const FGuid& MonsterId, const FIntPoint& Cell);
    bool CommitReservation (const FGuid& MonsterId, const FIntPoint& FromCell, const FIntPoint& ToCell);

    void CancelReservation (const FGuid& MonsterId);
    void UnregisterMonster (const FGuid& MonsterId);
    void Reset ();

    bool IsCellOccupied (const FIntPoint& Cell, const FGuid& IgnoredMonsterId = FGuid ()) const;
    bool IsCellReserved (const FIntPoint& Cell, const FGuid& IgnoredMonsterId = FGuid ()) const;
    bool IsCellBlocked (const FIntPoint& Cell, const FGuid& IgnoredMonsterId = FGuid ()) const;

    bool TryGetMonsterCell (const FGuid& MonsterId, FIntPoint& OutCell) const;
    bool TryGetReservation (const FGuid& MonsterId, FIntPoint& OutCell) const;
    FGuid GetOccupantId (const FIntPoint& Cell) const;
    FGuid GetReservationOwnerId (const FIntPoint& Cell) const;

    int32 GetOccupiedCellCount () const { return OccupiedCells.Num (); }
    int32 GetReservedCellCount () const { return ReservedCells.Num (); }

private:
    TMap<FIntPoint, FGuid> OccupiedCells;
    TMap<FIntPoint, FGuid> ReservedCells;
    TMap<FGuid, FIntPoint> MonsterCells;
    TMap<FGuid, FIntPoint> MonsterReservations;
};

/**
 * One occupancy authority per UWorld. Geometry remains owned by
 * AGridLevelRuntimeActor; this subsystem only arbitrates dynamic monster cells.
 */
UCLASS ()
class GRIMROCKPROTOTYPE_API UGridMonsterOccupancySubsystem : public UWorldSubsystem
{
    GENERATED_BODY ()

public:
    virtual void Deinitialize () override;

    UFUNCTION (BlueprintCallable, Category = "Monster|Occupancy")
    bool RegisterMonster (AGridMonsterActor* Monster, FIntPoint Cell);

    UFUNCTION (BlueprintCallable, Category = "Monster|Occupancy")
    void UnregisterMonster (AGridMonsterActor* Monster);

    UFUNCTION (BlueprintCallable, Category = "Monster|Occupancy")
    bool TryReserveCell (AGridMonsterActor* Monster, FIntPoint Cell);

    UFUNCTION (BlueprintCallable, Category = "Monster|Occupancy")
    bool CommitMove (AGridMonsterActor* Monster, FIntPoint FromCell, FIntPoint ToCell);

    UFUNCTION (BlueprintCallable, Category = "Monster|Occupancy")
    void CancelReservation (AGridMonsterActor* Monster);

    UFUNCTION (BlueprintPure, Category = "Monster|Occupancy")
    bool IsCellOccupied (FIntPoint Cell, const AGridMonsterActor* IgnoredMonster = nullptr) const;

    UFUNCTION (BlueprintPure, Category = "Monster|Occupancy")
    bool IsCellReserved (FIntPoint Cell, const AGridMonsterActor* IgnoredMonster = nullptr) const;

    UFUNCTION (BlueprintPure, Category = "Monster|Occupancy")
    bool IsCellBlocked (FIntPoint Cell, const AGridMonsterActor* IgnoredMonster = nullptr) const;

    UFUNCTION (BlueprintPure, Category = "Monster|Occupancy")
    AGridMonsterActor* GetOccupantAtCell (FIntPoint Cell) const;

    UFUNCTION (BlueprintPure, Category = "Monster|Occupancy")
    int32 GetOccupiedCellCount () const { return Registry.GetOccupiedCellCount (); }

    UFUNCTION (BlueprintPure, Category = "Monster|Occupancy")
    int32 GetReservedCellCount () const { return Registry.GetReservedCellCount (); }

    UFUNCTION (BlueprintCallable, Category = "Monster|Occupancy")
    void ResetRegistry ();

    UFUNCTION (BlueprintCallable, Category = "Monster|Occupancy|Debug")
    void LogRegistry () const;

    const FGridMonsterOccupancyRegistry& GetRegistry () const { return Registry; }

private:
    FGridMonsterOccupancyRegistry Registry;
    TMap<FGuid, TWeakObjectPtr<AGridMonsterActor>> RegisteredActors;

    static FGuid ResolveMonsterId (const AGridMonsterActor* Monster);
};
