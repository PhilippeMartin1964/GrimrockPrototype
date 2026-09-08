#include "Runtime/GridLevelRuntimeActor.h"

#include "Core/GridDirectionUtils.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridMonsterEncounterComponent.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterIdleVariationComponent.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
	const FName GridLevelRuntimeMonstersSingleLevelRuntimeStateId(TEXT("SingleLevel"));

	bool GridLevelRuntimeMonstersIsCardinalSpawnFacing(EGridEdge Facing)
	{
		return Facing == EGridEdge::North || Facing == EGridEdge::East || Facing == EGridEdge::South || Facing == EGridEdge::West;
	}

	FName GridLevelRuntimeMonstersResolveRuntimeStateLevelId(const UGridDungeonAsset* DungeonAsset, FName CurrentDungeonLevelId)
	{
		if (DungeonAsset && !CurrentDungeonLevelId.IsNone())
		{
			return CurrentDungeonLevelId;
		}

		return GridLevelRuntimeMonstersSingleLevelRuntimeStateId;
	}

	FString GridLevelRuntimeMonstersGetEdgeText(EGridEdge Edge)
	{
		if (const UEnum* EdgeEnum = StaticEnum<EGridEdge>())
		{
			return EdgeEnum->GetNameStringByValue(static_cast<int64>(Edge));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(Edge));
	}

	void GridLevelRuntimeMonstersGetWorldMonsters(const UWorld* World, TArray<AGridMonsterActor*>& OutMonsters)
	{
		OutMonsters.Reset();
		if (!World)
		{
			return;
		}

		for (TActorIterator<AGridMonsterActor> It(const_cast<UWorld*>(World)); It; ++It)
		{
			if (IsValid(*It))
			{
				OutMonsters.Add(*It);
			}
		}

		OutMonsters.Sort(
			[](const AGridMonsterActor& Left, const AGridMonsterActor& Right)
			{
				const FGuid LeftId = Left.ResolvePersistenceId();
				const FGuid RightId = Right.ResolvePersistenceId();
				if (LeftId.IsValid() != RightId.IsValid())
				{
					return LeftId.IsValid();
				}
				if (LeftId != RightId)
				{
					return LeftId.ToString(EGuidFormats::Digits) < RightId.ToString(EGuidFormats::Digits);
				}
				return Left.GetPathName() < Right.GetPathName();
			});
	}
}
void AGridLevelRuntimeActor::AbortActiveCombatAndMonsterActions()
{
	if (UGridTurnManagerComponent* TurnManager = FindComponentByClass<UGridTurnManagerComponent>())
	{
		TurnManager->AbortCombat();
	}

	TArray<AGridMonsterActor*> Monsters;
	GridLevelRuntimeMonstersGetWorldMonsters(GetWorld(), Monsters);
	for (AGridMonsterActor* Monster : Monsters)
	{
		if (Monster->CombatComponent)
		{
			Monster->CombatComponent->CancelAttackPresentation();
		}

		if (UGridMonsterMovementComponent* Movement = Monster->FindComponentByClass<UGridMonsterMovementComponent>())
		{
			Movement->CancelCurrentAction();
		}
	}

	if (UGridMonsterOccupancySubsystem* Occupancy = GetWorld() ? GetWorld()->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr)
	{
		for (AGridMonsterActor* Monster : Monsters)
		{
			Occupancy->CancelReservation(Monster);
		}
	}
}

void AGridLevelRuntimeActor::SetMonsterRuntimeLevelActive(AGridMonsterActor* Monster, bool bActive)
{
	if (!IsValid(Monster))
	{
		return;
	}

	UGridMonsterMovementComponent* Movement = Monster->FindComponentByClass<UGridMonsterMovementComponent>();
	UGridMonsterBehaviorComponent* Behavior = Monster->FindComponentByClass<UGridMonsterBehaviorComponent>();
	UGridMonsterOccupancySubsystem* Occupancy = GetWorld() ? GetWorld()->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr;

	if (!bActive)
	{
		if (Monster->IdleVariationComponent)
		{
			Monster->IdleVariationComponent->StopIdleVariations();
		}
		if (Monster->VFXComponent)
		{
			Monster->VFXComponent->StopAllMonsterVFX();
		}
		if (Monster->AudioComponent)
		{
			Monster->AudioComponent->StopAllMonsterAudio();
		}
		if (Monster->CombatComponent)
		{
			Monster->CombatComponent->CancelAttackPresentation();
			Monster->CombatComponent->Deactivate();
		}
		if (Movement)
		{
			Movement->CancelCurrentAction();
			Movement->ReleaseOccupancy();
			Movement->Deactivate();
		}
		else if (Occupancy)
		{
			Occupancy->UnregisterMonster(Monster);
		}
		if (Behavior)
		{
			Behavior->Deactivate();
		}

		Monster->ResetAnimationSignals();
		Monster->bRuntimeLevelActive = false;
		Monster->SetActorEnableCollision(false);
		if (Monster->CollisionComponent)
		{
			Monster->CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Monster->SetActorHiddenInGame(true);
		if (Monster->SkeletalMeshComponent)
		{
			Monster->SkeletalMeshComponent->SetVisibility(false, true);
		}

		UE_LOG(LogGridMonsterState, Log, TEXT("[GridMonsterState] DeactivateLevel Level=%s Monster=%s PersistenceId=%s"), *CurrentDungeonLevelId.ToString(),
			*GetNameSafe(Monster), *Monster->ResolvePersistenceId().ToString());
		return;
	}

	Monster->bRuntimeLevelActive = true;
	if (Monster->VFXComponent)
	{
		Monster->VFXComponent->InitializeMonsterVFX();
	}
	Monster->SetActorHiddenInGame(false);
	if (Monster->SkeletalMeshComponent)
	{
		Monster->SkeletalMeshComponent->SetVisibility(true, true);
	}

	const bool bStatsWereInitialized = Monster->bCombatStatsInitialized;
	Monster->EnsureInitialCombatState();
	const bool bInitializedStats = !bStatsWereInitialized && Monster->bCombatStatsInitialized;

	if (Monster->IsDead())
	{
		if (IsValidCell(Monster->CurrentCell.X, Monster->CurrentCell.Y) && IsWalkableCell(Monster->CurrentCell.X, Monster->CurrentCell.Y))
		{
			Monster->SetActorLocation(GetCellCenterWorld(Monster->CurrentCell.X, Monster->CurrentCell.Y));
			Monster->ApplyFacingRotation();
		}
		else
		{
			UE_LOG(LogGridMonsterState, Error,
				TEXT("[GridMonsterState] ActivateLevel Level=%s Monster=%s PersistenceId=%s Cell=(%d,%d) Result=InvalidDeadCell"),
				*CurrentDungeonLevelId.ToString(), *GetNameSafe(Monster), *Monster->ResolvePersistenceId().ToString(), Monster->CurrentCell.X,
				Monster->CurrentCell.Y);
		}

		if (Monster->DeathComponent)
		{
			Monster->DeathComponent->InitializeDeathComponent(this);
			Monster->DeathComponent->RestoreCommittedDeathState(Monster->CurrentCell);
		}
		else
		{
			Monster->SetActorEnableCollision(false);
		}
	}
	else if (Monster->bMonsterEnabled)
	{
		Monster->SetActorEnableCollision(true);
		if (Monster->CollisionComponent)
		{
			Monster->CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}

		bool bRegistered = false;
		if (Movement)
		{
			Movement->Activate();
			bRegistered = Movement->InitializeMovement(this);
		}
		else
		{
			int32 ResolvedX = INDEX_NONE;
			int32 ResolvedY = INDEX_NONE;
			FVector LocalOffset = FVector::ZeroVector;
			const FVector WorldLocation = Monster->GetActorLocation();
			if (TryResolveWorldCellFromImpactPoint(WorldLocation, ResolvedX, ResolvedY, LocalOffset))
			{
				Monster->CurrentCell = FIntPoint(ResolvedX, ResolvedY);
				Monster->SetActorLocation(GetCellCenterWorld(ResolvedX, ResolvedY));
				Monster->ApplyFacingRotation();
				bRegistered = Occupancy && Occupancy->RegisterMonster(Monster, Monster->CurrentCell);
			}
			else
			{
				UE_LOG(LogGridMonsterState, Error,
					TEXT("[GridMonsterState] ActivateLevel Level=%s Monster=%s PersistenceId=%s WorldLocation=%s Result=CellInferenceFailed"),
					*CurrentDungeonLevelId.ToString(), *GetNameSafe(Monster), *Monster->ResolvePersistenceId().ToString(), *WorldLocation.ToCompactString());
			}
		}

		if (!bRegistered)
		{
			if (Monster->CollisionComponent)
			{
				Monster->CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			UE_LOG(LogGridMonsterState, Error,
				TEXT("[GridMonsterState] ActivateLevel Level=%s Monster=%s PersistenceId=%s Cell=(%d,%d) Result=MovementInitializationOrOccupancyFailed"),
				*CurrentDungeonLevelId.ToString(), *GetNameSafe(Monster), *Monster->ResolvePersistenceId().ToString(), Monster->CurrentCell.X,
				Monster->CurrentCell.Y);
		}

		if (Behavior)
		{
			Behavior->Activate();
			Behavior->InitializeBehavior(this, nullptr);
		}
		if (Monster->CombatComponent)
		{
			Monster->CombatComponent->Activate();
			Monster->CombatComponent->InitializeCombat(nullptr);
		}
	}
	else
	{
		Monster->SetActorEnableCollision(false);
		if (Monster->CollisionComponent)
		{
			Monster->CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	UE_LOG(LogGridMonsterState, Log,
		TEXT(
			"[GridMonsterState] ActivateLevel Level=%s Monster=%s PersistenceId=%s Dead=%s Enabled=%s InitializedStats=%s CombatStatsInitialized=%s DeathCommitted=%s"),
		*CurrentDungeonLevelId.ToString(), *GetNameSafe(Monster), *Monster->ResolvePersistenceId().ToString(), Monster->IsDead() ? TEXT("true") : TEXT("false"),
		Monster->bMonsterEnabled ? TEXT("true") : TEXT("false"), bInitializedStats ? TEXT("true") : TEXT("false"),
		Monster->bCombatStatsInitialized ? TEXT("true") : TEXT("false"),
		Monster->DeathComponent && Monster->DeathComponent->bDeathCommitted ? TEXT("true") : TEXT("false"));
	if (Monster->AudioComponent)
	{
		Monster->AudioComponent->InitializeMonsterAudio();
		Monster->AudioComponent->RefreshIdleAmbienceScheduling();
	}
	if (Monster->IdleVariationComponent)
	{
		Monster->IdleVariationComponent->InitializeIdleVariations();
		Monster->IdleVariationComponent->RefreshIdleVariationScheduling();
	}
}

void AGridLevelRuntimeActor::ApplyInitialMonsterStateForCurrentLevel()
{
	AbortActiveCombatAndMonsterActions();

	TArray<AGridMonsterActor*> Monsters;
	GridLevelRuntimeMonstersGetWorldMonsters(GetWorld(), Monsters);
	for (AGridMonsterActor* Monster : Monsters)
	{
		SetMonsterRuntimeLevelActive(Monster, false);
	}

	if (UGridMonsterOccupancySubsystem* Occupancy = GetWorld() ? GetWorld()->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr)
	{
		Occupancy->ResetRegistry();
	}

	const FName RuntimeLevelId = GridLevelRuntimeMonstersResolveRuntimeStateLevelId(DungeonAsset, CurrentDungeonLevelId);
	for (AGridMonsterActor* Monster : Monsters)
	{
		if (Monster->ResolveRuntimeDungeonLevelId(RuntimeLevelId) == RuntimeLevelId)
		{
			Monster->EnsureInitialCombatState();
			SetMonsterRuntimeLevelActive(Monster, true);
		}
	}
}

void AGridLevelRuntimeActor::ClearSpawnedMonsterActors()
{
	if (SpawnedMonsterActors.IsEmpty())
	{
		return;
	}

	AbortActiveCombatAndMonsterActions();
	for (TPair<FGuid, TObjectPtr<AGridMonsterActor>>& Pair : SpawnedMonsterActors)
	{
		AGridMonsterActor* Monster = Pair.Value.Get();
		if (!IsValid(Monster))
		{
			continue;
		}

		SetMonsterRuntimeLevelActive(Monster, false);
		Monster->Destroy();
	}
	SpawnedMonsterActors.Empty();
}

void AGridLevelRuntimeActor::ApplyMonsterPlacementMetadata(AGridMonsterActor* Monster) const
{
	if (!LevelAsset || !IsValid(Monster) || !Monster->SpawnObjectId.IsValid())
	{
		return;
	}

	const FGridMonsterSpawnInstance* Placement = LevelAsset->MonsterSpawns.FindByPredicate(
		[Monster](const FGridMonsterSpawnInstance& Spawn)
		{
			return Spawn.SpawnId == Monster->SpawnObjectId;
		});
	if (Placement)
	{
		Monster->EncounterGroupId = Placement->EncounterGroupId;
		if (Monster->HomeDungeonLevelId.IsNone())
		{
			Monster->HomeDungeonLevelId = GridLevelRuntimeMonstersResolveRuntimeStateLevelId(DungeonAsset, CurrentDungeonLevelId);
		}
	}
}

AGridMonsterActor* AGridLevelRuntimeActor::FindSpawnedMonsterActor(const FGuid& SpawnId) const
{
	if (const TObjectPtr<AGridMonsterActor>* Monster = SpawnedMonsterActors.Find(SpawnId))
	{
		return IsValid(Monster->Get()) ? Monster->Get() : nullptr;
	}
	return nullptr;
}

int32 AGridLevelRuntimeActor::GetSpawnedMonsterActorCount() const
{
	int32 Count = 0;
	for (const TPair<FGuid, TObjectPtr<AGridMonsterActor>>& Pair : SpawnedMonsterActors)
	{
		Count += IsValid(Pair.Value.Get()) ? 1 : 0;
	}
	return Count;
}

bool AGridLevelRuntimeActor::StartMonsterEncounter(FGuid AnchorSpawnId)
{
	return MonsterEncounterComponent && MonsterEncounterComponent->StartEncounter(AnchorSpawnId);
}

bool AGridLevelRuntimeActor::IsMonsterEncounterCompleted(FName EncounterGroupId) const
{
	return MonsterEncounterComponent && MonsterEncounterComponent->IsEncounterCompleted(EncounterGroupId);
}

int32 AGridLevelRuntimeActor::GetMonsterEncounterActiveWave(FName EncounterGroupId) const
{
	return MonsterEncounterComponent ? MonsterEncounterComponent->GetActiveWaveIndex(EncounterGroupId) : INDEX_NONE;
}

void AGridLevelRuntimeActor::NotifyMonsterEncounterDeath(FGuid SpawnId)
{
	if (MonsterEncounterComponent)
	{
		MonsterEncounterComponent->NotifyMonsterDied(SpawnId);
	}
}

bool AGridLevelRuntimeActor::ExecuteMonsterSpawnCommand(FGuid SpawnId, EGridObjectCommand Command)
{
	if (!LevelAsset || !SpawnId.IsValid())
	{
		return false;
	}

	const FGridMonsterSpawnInstance* SpawnData = LevelAsset->MonsterSpawns.FindByPredicate(
		[SpawnId](const FGridMonsterSpawnInstance& Candidate)
		{
			return Candidate.SpawnId == SpawnId;
		});
	if (!SpawnData)
	{
		UE_LOG(LogGridMonsterState, Warning, TEXT("[GridMonsterLifecycle] CommandRejected SpawnId=%s Command=%s Reason=PlacementNotFound"),
			*SpawnId.ToString(EGuidFormats::DigitsWithHyphens), *UEnum::GetValueAsString(Command));
		return false;
	}

	const FGridLevelRuntimeState* ExistingState = FindRuntimeStateForCurrentLevel();
	if (!ExistingState || !ExistingState->bHasBeenVisited)
	{
		CaptureCurrentLevelRuntimeState();
	}

	const bool bIsCurrentlySpawned = FindSpawnedMonsterActor(SpawnId) != nullptr;
	switch (Command)
	{
		case EGridObjectCommand::Toggle:
			Command = bIsCurrentlySpawned ? EGridObjectCommand::Despawn : EGridObjectCommand::Spawn;
			break;

		case EGridObjectCommand::Activate:
		case EGridObjectCommand::Enable:
			Command = EGridObjectCommand::Spawn;
			break;

		case EGridObjectCommand::Deactivate:
		case EGridObjectCommand::Disable:
			Command = EGridObjectCommand::Despawn;
			break;

		default:
			break;
	}

	if (Command == EGridObjectCommand::Despawn)
	{
		return DespawnMonsterSpawnActor(*SpawnData, true, bIsCurrentlySpawned);
	}

	if (Command == EGridObjectCommand::Teleport)
	{
		return TeleportSpawnedMonster(SpawnId, SpawnData->CellX, SpawnData->CellY, SpawnData->Facing);
	}

	if (Command != EGridObjectCommand::Spawn)
	{
		return false;
	}

	if (bIsCurrentlySpawned)
	{
		return true;
	}

	const FGridRuntimeMonsterState* RestoreState = nullptr;
	if (const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel())
	{
		if (const FGridRuntimeMonsterPlacementState* PlacementState = State->MonsterPlacements.Find(SpawnId))
		{
			RestoreState = PlacementState->bHasMonsterState ? &PlacementState->MonsterState : nullptr;
		}
	}

	AGridMonsterActor* Monster = AddMonsterSpawnActor(*SpawnData, RestoreState);
	if (!Monster)
	{
		UE_LOG(LogGridMonsterState, Log, TEXT("[GridMonsterLifecycle] SpawnRejected SpawnId=%s Reason=AtomicSpawnFailed"),
			*SpawnId.ToString(EGuidFormats::DigitsWithHyphens));
		return false;
	}

	if (!StoreMonsterPlacementState(*SpawnData, Monster, true))
	{
		DespawnMonsterSpawnActor(*SpawnData, false, false);
		return false;
	}

	AbortActiveCombatAndMonsterActions();
	UE_LOG(LogGridMonsterState, Log, TEXT("[GridMonsterLifecycle] SpawnCommandCompleted SpawnId=%s Encounter=%s"),
		*SpawnId.ToString(EGuidFormats::DigitsWithHyphens), *Monster->EncounterGroupId.ToString());
	ExecuteLinksFromRuntimeObject(SpawnId, EGridObjectEvent::MonsterSpawned);
	return true;
}

bool AGridLevelRuntimeActor::TeleportSpawnedMonster(FGuid SpawnId, int32 TargetCellX, int32 TargetCellY, EGridEdge TargetFacing)
{
	if (!LevelAsset || !SpawnId.IsValid())
	{
		return false;
	}

	const FGridMonsterSpawnInstance* SpawnData = LevelAsset->MonsterSpawns.FindByPredicate(
		[SpawnId](const FGridMonsterSpawnInstance& Candidate)
		{
			return Candidate.SpawnId == SpawnId;
		});
	AGridMonsterActor* Monster = FindSpawnedMonsterActor(SpawnId);
	UGridMonsterMovementComponent* Movement = Monster ? Monster->FindComponentByClass<UGridMonsterMovementComponent>() : nullptr;
	UGridMonsterOccupancySubsystem* Occupancy = GetWorld() ? GetWorld()->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr;
	const FIntPoint TargetCell(TargetCellX, TargetCellY);

	bool bGeneratedMonsterOccupiesTarget = false;
	for (const TPair<FGuid, TObjectPtr<AGridMonsterActor>>& Pair : SpawnedMonsterActors)
	{
		const AGridMonsterActor* Other = Pair.Value.Get();
		if (IsValid(Other) && Other != Monster && Other->CurrentCell == TargetCell)
		{
			bGeneratedMonsterOccupiesTarget = true;
			break;
		}
	}

	if (!SpawnData || !Monster || (Movement && !Movement->IsInitialized()) || Monster->IsDead() ||
		!GridLevelRuntimeMonstersIsCardinalSpawnFacing(TargetFacing) || !IsValidCell(TargetCellX, TargetCellY) || !IsWalkableCell(TargetCellX, TargetCellY) ||
		IsPartyOnCell(TargetCellX, TargetCellY) || bGeneratedMonsterOccupiesTarget || !Occupancy || Occupancy->IsCellBlocked(TargetCell, Monster))
	{
		UE_LOG(LogGridMonsterState, Log, TEXT("[GridMonsterLifecycle] TeleportRejected SpawnId=%s Target=(%d,%d) Facing=%s Reason=InvalidOrOccupiedTarget"),
			*SpawnId.ToString(EGuidFormats::DigitsWithHyphens), TargetCellX, TargetCellY, *GridLevelRuntimeMonstersGetEdgeText(TargetFacing));
		return false;
	}

	if (Monster->CurrentCell == TargetCell && Monster->Facing == TargetFacing)
	{
		return true;
	}

	const FGridLevelRuntimeState* ExistingState = FindRuntimeStateForCurrentLevel();
	if (!ExistingState || !ExistingState->bHasBeenVisited)
	{
		CaptureCurrentLevelRuntimeState();
	}

	const FIntPoint PreviousCell = Monster->CurrentCell;
	const EGridEdge PreviousFacing = Monster->Facing;
	AbortActiveCombatAndMonsterActions();
	bool bTeleported = false;
	if (Movement)
	{
		bTeleported = Movement->TeleportToGridPose(TargetCell, TargetFacing);
	}
	else
	{
		Occupancy->UnregisterMonster(Monster);
		if (Occupancy->RegisterMonster(Monster, TargetCell))
		{
			Monster->CurrentCell = TargetCell;
			Monster->Facing = TargetFacing;
			Monster->SetActorLocation(GetCellCenterWorld(TargetCell.X, TargetCell.Y));
			Monster->ApplyFacingRotation();
			bTeleported = true;
		}
		else
		{
			Occupancy->RegisterMonster(Monster, PreviousCell);
		}
	}
	if (!bTeleported)
	{
		return false;
	}

	if (!StoreMonsterPlacementState(*SpawnData, Monster, true))
	{
		if (Movement)
		{
			Movement->TeleportToGridPose(PreviousCell, PreviousFacing);
		}
		else
		{
			Occupancy->UnregisterMonster(Monster);
			Occupancy->RegisterMonster(Monster, PreviousCell);
			Monster->CurrentCell = PreviousCell;
			Monster->Facing = PreviousFacing;
			Monster->SetActorLocation(GetCellCenterWorld(PreviousCell.X, PreviousCell.Y));
			Monster->ApplyFacingRotation();
		}
		return false;
	}

	UE_LOG(LogGridMonsterState, Log, TEXT("[GridMonsterLifecycle] Teleported SpawnId=%s From=(%d,%d) To=(%d,%d) Facing=%s Encounter=%s"),
		*SpawnId.ToString(EGuidFormats::DigitsWithHyphens), PreviousCell.X, PreviousCell.Y, TargetCellX, TargetCellY,
		*GridLevelRuntimeMonstersGetEdgeText(TargetFacing), *Monster->EncounterGroupId.ToString());
	ExecuteLinksFromRuntimeObject(SpawnId, EGridObjectEvent::MonsterTeleported);
	return true;
}
