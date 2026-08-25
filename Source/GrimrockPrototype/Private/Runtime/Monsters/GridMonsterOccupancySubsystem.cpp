#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

#include "Runtime/Monsters/GridMonsterActor.h"

DEFINE_LOG_CATEGORY(LogGridMonsterOccupancy);

bool FGridMonsterOccupancyRegistry::TryRegisterMonster(const FGuid& MonsterId, const FIntPoint& Cell)
{
	if (!MonsterId.IsValid())
	{
		return false;
	}

	if (const FIntPoint* ExistingCell = MonsterCells.Find(MonsterId))
	{
		return *ExistingCell == Cell;
	}

	if (IsCellBlocked(Cell, MonsterId))
	{
		return false;
	}

	OccupiedCells.Add(Cell, MonsterId);
	MonsterCells.Add(MonsterId, Cell);
	return true;
}

bool FGridMonsterOccupancyRegistry::TryReserveCell(const FGuid& MonsterId, const FIntPoint& Cell)
{
	if (!MonsterId.IsValid() || !MonsterCells.Contains(MonsterId))
	{
		return false;
	}

	if (const FIntPoint* ExistingReservation = MonsterReservations.Find(MonsterId))
	{
		return *ExistingReservation == Cell;
	}

	if (IsCellBlocked(Cell, MonsterId))
	{
		return false;
	}

	ReservedCells.Add(Cell, MonsterId);
	MonsterReservations.Add(MonsterId, Cell);
	return true;
}

bool FGridMonsterOccupancyRegistry::CommitReservation(const FGuid& MonsterId, const FIntPoint& FromCell, const FIntPoint& ToCell)
{
	if (!MonsterId.IsValid())
	{
		return false;
	}

	const FIntPoint* RegisteredCell = MonsterCells.Find(MonsterId);
	const FIntPoint* ReservedCell = MonsterReservations.Find(MonsterId);
	if (!RegisteredCell || !ReservedCell || *RegisteredCell != FromCell || *ReservedCell != ToCell)
	{
		return false;
	}

	if (IsCellOccupied(ToCell, MonsterId) || IsCellReserved(ToCell, MonsterId))
	{
		return false;
	}

	OccupiedCells.Remove(FromCell);
	ReservedCells.Remove(ToCell);
	MonsterReservations.Remove(MonsterId);

	OccupiedCells.Add(ToCell, MonsterId);
	MonsterCells.Add(MonsterId, ToCell);
	return true;
}

void FGridMonsterOccupancyRegistry::CancelReservation(const FGuid& MonsterId)
{
	FIntPoint ReservedCell;
	if (MonsterReservations.RemoveAndCopyValue(MonsterId, ReservedCell))
	{
		if (const FGuid* ReservationOwner = ReservedCells.Find(ReservedCell))
		{
			if (*ReservationOwner == MonsterId)
			{
				ReservedCells.Remove(ReservedCell);
			}
		}
	}
}

void FGridMonsterOccupancyRegistry::UnregisterMonster(const FGuid& MonsterId)
{
	CancelReservation(MonsterId);

	FIntPoint OccupiedCell;
	if (MonsterCells.RemoveAndCopyValue(MonsterId, OccupiedCell))
	{
		if (const FGuid* Occupant = OccupiedCells.Find(OccupiedCell))
		{
			if (*Occupant == MonsterId)
			{
				OccupiedCells.Remove(OccupiedCell);
			}
		}
	}
}

void FGridMonsterOccupancyRegistry::Reset()
{
	OccupiedCells.Reset();
	ReservedCells.Reset();
	MonsterCells.Reset();
	MonsterReservations.Reset();
}

bool FGridMonsterOccupancyRegistry::IsCellOccupied(const FIntPoint& Cell, const FGuid& IgnoredMonsterId) const
{
	const FGuid* Occupant = OccupiedCells.Find(Cell);
	return Occupant && (!IgnoredMonsterId.IsValid() || *Occupant != IgnoredMonsterId);
}

bool FGridMonsterOccupancyRegistry::IsCellReserved(const FIntPoint& Cell, const FGuid& IgnoredMonsterId) const
{
	const FGuid* ReservationOwner = ReservedCells.Find(Cell);
	return ReservationOwner && (!IgnoredMonsterId.IsValid() || *ReservationOwner != IgnoredMonsterId);
}

bool FGridMonsterOccupancyRegistry::IsCellBlocked(const FIntPoint& Cell, const FGuid& IgnoredMonsterId) const
{
	return IsCellOccupied(Cell, IgnoredMonsterId) || IsCellReserved(Cell, IgnoredMonsterId);
}

bool FGridMonsterOccupancyRegistry::CanReserveCell(const FGuid& MonsterId, const FIntPoint& Cell) const
{
	if (!MonsterId.IsValid() || !MonsterCells.Contains(MonsterId))
	{
		return false;
	}

	if (const FIntPoint* ExistingReservation = MonsterReservations.Find(MonsterId))
	{
		return *ExistingReservation == Cell;
	}

	return !IsCellBlocked(Cell, MonsterId);
}

bool FGridMonsterOccupancyRegistry::TryGetMonsterCell(const FGuid& MonsterId, FIntPoint& OutCell) const
{
	if (const FIntPoint* Cell = MonsterCells.Find(MonsterId))
	{
		OutCell = *Cell;
		return true;
	}
	return false;
}

bool FGridMonsterOccupancyRegistry::TryGetReservation(const FGuid& MonsterId, FIntPoint& OutCell) const
{
	if (const FIntPoint* Cell = MonsterReservations.Find(MonsterId))
	{
		OutCell = *Cell;
		return true;
	}
	return false;
}

FGuid FGridMonsterOccupancyRegistry::GetOccupantId(const FIntPoint& Cell) const
{
	if (const FGuid* Occupant = OccupiedCells.Find(Cell))
	{
		return *Occupant;
	}
	return FGuid();
}

FGuid FGridMonsterOccupancyRegistry::GetReservationOwnerId(const FIntPoint& Cell) const
{
	if (const FGuid* ReservationOwner = ReservedCells.Find(Cell))
	{
		return *ReservationOwner;
	}
	return FGuid();
}

void UGridMonsterOccupancySubsystem::Deinitialize()
{
	ResetRegistry();
	Super::Deinitialize();
}

bool UGridMonsterOccupancySubsystem::RegisterMonster(AGridMonsterActor* Monster, FIntPoint Cell)
{
	if (!IsValid(Monster))
	{
		return false;
	}

	const FGuid MonsterId = Monster->ResolvePersistenceId();
	if (!MonsterId.IsValid())
	{
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterState] Occupancy registration skipped Monster=%s Reason=InvalidPersistenceId"),
			*GetNameSafe(Monster));
		return false;
	}
	if (!Registry.TryRegisterMonster(MonsterId, Cell))
	{
		return false;
	}

	RegisteredActors.Add(MonsterId, Monster);
	return true;
}

void UGridMonsterOccupancySubsystem::UnregisterMonster(AGridMonsterActor* Monster)
{
	const FGuid MonsterId = ResolveMonsterId(Monster);
	if (!MonsterId.IsValid())
	{
		return;
	}

	Registry.UnregisterMonster(MonsterId);
	RegisteredActors.Remove(MonsterId);
}

bool UGridMonsterOccupancySubsystem::TryReserveCell(AGridMonsterActor* Monster, FIntPoint Cell)
{
	return Registry.TryReserveCell(ResolveMonsterId(Monster), Cell);
}

bool UGridMonsterOccupancySubsystem::CommitMove(AGridMonsterActor* Monster, FIntPoint FromCell, FIntPoint ToCell)
{
	return Registry.CommitReservation(ResolveMonsterId(Monster), FromCell, ToCell);
}

void UGridMonsterOccupancySubsystem::CancelReservation(AGridMonsterActor* Monster)
{
	Registry.CancelReservation(ResolveMonsterId(Monster));
}

bool UGridMonsterOccupancySubsystem::IsCellOccupied(FIntPoint Cell, const AGridMonsterActor* IgnoredMonster) const
{
	return Registry.IsCellOccupied(Cell, ResolveMonsterId(IgnoredMonster));
}

bool UGridMonsterOccupancySubsystem::IsCellReserved(FIntPoint Cell, const AGridMonsterActor* IgnoredMonster) const
{
	return Registry.IsCellReserved(Cell, ResolveMonsterId(IgnoredMonster));
}

bool UGridMonsterOccupancySubsystem::IsCellBlocked(FIntPoint Cell, const AGridMonsterActor* IgnoredMonster) const
{
	return Registry.IsCellBlocked(Cell, ResolveMonsterId(IgnoredMonster));
}

bool UGridMonsterOccupancySubsystem::CanReserveCell(const AGridMonsterActor* Monster, FIntPoint Cell) const
{
	return Registry.CanReserveCell(ResolveMonsterId(Monster), Cell);
}

AGridMonsterActor* UGridMonsterOccupancySubsystem::GetOccupantAtCell(FIntPoint Cell) const
{
	const FGuid OccupantId = Registry.GetOccupantId(Cell);
	if (const TWeakObjectPtr<AGridMonsterActor>* Actor = RegisteredActors.Find(OccupantId))
	{
		return Actor->Get();
	}
	return nullptr;
}

void UGridMonsterOccupancySubsystem::ResetRegistry()
{
	Registry.Reset();
	RegisteredActors.Reset();
}

void UGridMonsterOccupancySubsystem::LogRegistry() const
{
	UE_LOG(
		LogGridMonsterOccupancy, Log, TEXT("[GridMonsterOccupancy] Occupied=%d Reserved=%d"), Registry.GetOccupiedCellCount(), Registry.GetReservedCellCount());
}

FGuid UGridMonsterOccupancySubsystem::ResolveMonsterId(const AGridMonsterActor* Monster)
{
	return IsValid(Monster) ? Monster->ResolvePersistenceId() : FGuid();
}
