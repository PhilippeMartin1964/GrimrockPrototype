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
			Monster->DeathComponent->RestoreCommittedDeathState(Monster->CurrentCell, true);
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

	const FGridLevelObjectData* Placement = LevelAsset->Objects.FindByPredicate(
		[Monster](const FGridLevelObjectData& ObjectData)
		{
			return ObjectData.Type == EGridLevelObjectType::MonsterSpawn && ObjectData.ObjectId == Monster->SpawnObjectId;
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

bool AGridLevelRuntimeActor::ResolveMonsterSpawn(
	const FGridLevelObjectData& ObjectData, UGridMonsterDefinitionAsset*& OutDefinition, TSubclassOf<AGridMonsterActor>& OutActorClass, FString& OutError) const
{
	OutDefinition = nullptr;
	OutActorClass = nullptr;
	OutError.Empty();

	TArray<FString> Errors;
	if (!LevelAsset)
	{
		Errors.Add(TEXT("LevelAsset is missing."));
	}
	if (ObjectData.Type != EGridLevelObjectType::MonsterSpawn)
	{
		Errors.Add(TEXT("Object type is not MonsterSpawn."));
	}
	if (!ObjectData.ObjectId.IsValid())
	{
		Errors.Add(TEXT("SpawnId/ObjectId is invalid."));
	}
	if (!LevelAsset || !LevelAsset->IsValidCoord(ObjectData.CellX, ObjectData.CellY))
	{
		Errors.Add(FString::Printf(TEXT("Cell=(%d,%d) is outside the level."), ObjectData.CellX, ObjectData.CellY));
	}
	else if (!IsWalkableCell(ObjectData.CellX, ObjectData.CellY))
	{
		Errors.Add(FString::Printf(TEXT("Cell=(%d,%d) does not allow monster occupancy."), ObjectData.CellX, ObjectData.CellY));
	}
	if (ObjectData.Edge != EGridEdge::None)
	{
		Errors.Add(TEXT("MonsterSpawn requires Edge=None."));
	}
	if (!GridLevelRuntimeMonstersIsCardinalSpawnFacing(ObjectData.InitialFacing))
	{
		Errors.Add(TEXT("InitialFacing is not cardinal."));
	}

	UGridMonsterDefinitionAsset* Definition = ObjectData.MonsterDefinitionAsset.Get();
	if (!Definition)
	{
		Errors.Add(ObjectData.MonsterDefinitionId.IsNone()
				? TEXT("MonsterDefinitionAsset and MonsterDefinitionId are missing.")
				: FString::Printf(TEXT("MonsterDefinitionId '%s' cannot be resolved without MonsterDefinitionAsset in MON13.2."),
					  *ObjectData.MonsterDefinitionId.ToString()));
	}
	else
	{
		if (ObjectData.MonsterDefinitionId.IsNone())
		{
			Errors.Add(TEXT("MonsterDefinitionId is missing."));
		}
		else if (Definition->MonsterId != ObjectData.MonsterDefinitionId)
		{
			Errors.Add(FString::Printf(TEXT("MonsterDefinitionId '%s' differs from asset MonsterId '%s'."), *ObjectData.MonsterDefinitionId.ToString(),
				*Definition->MonsterId.ToString()));
		}

		FString DefinitionError;
		if (!Definition->ValidateDefinition(DefinitionError))
		{
			Errors.Add(FString::Printf(TEXT("MonsterDefinition '%s' is invalid: %s"), *GetPathNameSafe(Definition), *DefinitionError));
		}
		else
		{
			UClass* ActorClass = Definition->MonsterActorClass.Get();
			if (!ActorClass)
			{
				Errors.Add(TEXT("MonsterActorClass is missing."));
			}
			else if (!ActorClass->IsChildOf(AGridMonsterActor::StaticClass()))
			{
				Errors.Add(FString::Printf(TEXT("MonsterActorClass '%s' is not an AGridMonsterActor."), *ActorClass->GetPathName()));
			}
			else if (ActorClass->HasAnyClassFlags(CLASS_Abstract))
			{
				Errors.Add(FString::Printf(TEXT("MonsterActorClass '%s' is abstract."), *ActorClass->GetPathName()));
			}
			else
			{
				OutActorClass = Definition->MonsterActorClass;
			}
		}
	}

	if (!Errors.IsEmpty())
	{
		OutError = FString::Join(Errors, TEXT(" "));
		return false;
	}

	OutDefinition = Definition;
	return true;
}

bool AGridLevelRuntimeActor::GetMonsterSpawnTransform(const FGridLevelObjectData& ObjectData, FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;
	if (!LevelAsset || ObjectData.Type != EGridLevelObjectType::MonsterSpawn || !LevelAsset->IsValidCoord(ObjectData.CellX, ObjectData.CellY) ||
		!GridLevelRuntimeMonstersIsCardinalSpawnFacing(ObjectData.InitialFacing))
	{
		return false;
	}

	OutTransform = FTransform(
		FRotator(0.0f, GridDirectionUtils::ToYaw(ObjectData.InitialFacing), 0.0f), GetCellCenterWorld(ObjectData.CellX, ObjectData.CellY), FVector::OneVector);
	return IsSafeRuntimeRenderTransform(OutTransform);
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

bool AGridLevelRuntimeActor::StoreMonsterPlacementState(const FGridLevelObjectData& ObjectData, AGridMonsterActor* Monster, bool bIsSpawned)
{
	if (!ObjectData.ObjectId.IsValid() || ObjectData.Type != EGridLevelObjectType::MonsterSpawn)
	{
		return false;
	}

	FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
	if (!State)
	{
		return false;
	}

	FGridRuntimeMonsterState CapturedState;
	bool bCapturedMonsterState = false;
	if (IsValid(Monster))
	{
		if (!Monster->CaptureRuntimeMonsterState(CapturedState, State->LevelId))
		{
			return false;
		}
		bCapturedMonsterState = true;
	}

	FGridRuntimeMonsterPlacementState& PlacementState = State->MonsterPlacements.FindOrAdd(ObjectData.ObjectId);
	PlacementState.SpawnId = ObjectData.ObjectId;
	PlacementState.bIsSpawned = bIsSpawned;

	if (bCapturedMonsterState)
	{
		PlacementState.bHasMonsterState = true;
		PlacementState.MonsterState = CapturedState;
		if (bIsSpawned)
		{
			State->Monsters.Add(CapturedState.PersistenceId, CapturedState);
		}
		else
		{
			State->Monsters.Remove(CapturedState.PersistenceId);
		}
	}
	else if (!bIsSpawned)
	{
		State->Monsters.Remove(ObjectData.ObjectId);
	}

	State->bHasBeenVisited = true;
	return true;
}

bool AGridLevelRuntimeActor::DespawnMonsterSpawnActor(const FGridLevelObjectData& ObjectData, bool bRememberState, bool bEmitEvent)
{
	AGridMonsterActor* Monster = FindSpawnedMonsterActor(ObjectData.ObjectId);
	if (!Monster)
	{
		return !bRememberState || StoreMonsterPlacementState(ObjectData, nullptr, false);
	}

	if (bRememberState && !StoreMonsterPlacementState(ObjectData, Monster, false))
	{
		UE_LOG(LogGridMonsterState, Warning, TEXT("[GridMonsterLifecycle] DespawnRejected SpawnId=%s Reason=StateCaptureFailed"),
			*ObjectData.ObjectId.ToString(EGuidFormats::DigitsWithHyphens));
		return false;
	}

	AbortActiveCombatAndMonsterActions();
	SetMonsterRuntimeLevelActive(Monster, false);
	SpawnedMonsterActors.Remove(ObjectData.ObjectId);
	Monster->Destroy();

	UE_LOG(LogGridMonsterState, Log, TEXT("[GridMonsterLifecycle] Despawned SpawnId=%s Encounter=%s"),
		*ObjectData.ObjectId.ToString(EGuidFormats::DigitsWithHyphens), *ObjectData.EncounterGroupId.ToString());

	if (bEmitEvent)
	{
		ExecuteLinksFromRuntimeObject(ObjectData.ObjectId, EGridObjectEvent::MonsterDespawned);
	}
	return true;
}

bool AGridLevelRuntimeActor::ExecuteMonsterSpawnCommand(FGuid SpawnId, EGridObjectCommand Command)
{
	if (!LevelAsset || !SpawnId.IsValid())
	{
		return false;
	}

	const FGridLevelObjectData* ObjectData = LevelAsset->FindMonsterSpawnById(SpawnId);
	if (!ObjectData)
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
		return DespawnMonsterSpawnActor(*ObjectData, true, bIsCurrentlySpawned);
	}

	if (Command == EGridObjectCommand::Teleport)
	{
		return TeleportSpawnedMonster(SpawnId, ObjectData->CellX, ObjectData->CellY, ObjectData->InitialFacing);
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

	AGridMonsterActor* Monster = AddMonsterSpawnActor(*ObjectData, RestoreState);
	if (!Monster)
	{
		UE_LOG(LogGridMonsterState, Log, TEXT("[GridMonsterLifecycle] SpawnRejected SpawnId=%s Reason=AtomicSpawnFailed"),
			*SpawnId.ToString(EGuidFormats::DigitsWithHyphens));
		return false;
	}

	if (!StoreMonsterPlacementState(*ObjectData, Monster, true))
	{
		DespawnMonsterSpawnActor(*ObjectData, false, false);
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

	const FGridLevelObjectData* ObjectData = LevelAsset->FindMonsterSpawnById(SpawnId);
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

	if (!ObjectData || !Monster || (Movement && !Movement->IsInitialized()) || Monster->IsDead() ||
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

	if (!StoreMonsterPlacementState(*ObjectData, Monster, true))
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

AGridMonsterActor* AGridLevelRuntimeActor::AddMonsterSpawnActor(const FGridLevelObjectData& ObjectData, const FGridRuntimeMonsterState* RestoreState)
{
	FGridLevelObjectData SpawnData = ObjectData;
	if (RestoreState)
	{
		SpawnData.CellX = RestoreState->CellX;
		SpawnData.CellY = RestoreState->CellY;
		SpawnData.InitialFacing = GridLevelRuntimeMonstersIsCardinalSpawnFacing(RestoreState->Facing) ? RestoreState->Facing : ObjectData.InitialFacing;
		SpawnData.EncounterGroupId = RestoreState->EncounterGroupId;
	}

	const bool bRestoreDead =
		RestoreState && (RestoreState->bIsDead || RestoreState->CurrentHealth <= 0 || RestoreState->MonsterState == EGridMonsterState::Dead);

	const FString SpawnIdText = SpawnData.ObjectId.ToString(EGuidFormats::DigitsWithHyphens);
	if (SpawnedMonsterActors.Contains(SpawnData.ObjectId))
	{
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Reason=DuplicateSpawnId"), *SpawnIdText, SpawnData.CellX,
			SpawnData.CellY);
		return nullptr;
	}

	UGridMonsterDefinitionAsset* Definition = nullptr;
	TSubclassOf<AGridMonsterActor> MonsterActorClass;
	FString ResolutionError;
	if (!ResolveMonsterSpawn(SpawnData, Definition, MonsterActorClass, ResolutionError))
	{
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Reason=%s"), *SpawnIdText, SpawnData.CellX,
			SpawnData.CellY, *SpawnData.MonsterDefinitionId.ToString(), *ResolutionError);
		return nullptr;
	}

	FTransform SpawnTransform;
	if (!GetMonsterSpawnTransform(SpawnData, SpawnTransform))
	{
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Reason=InvalidTransform"), *SpawnIdText,
			SpawnData.CellX, SpawnData.CellY, *SpawnData.MonsterDefinitionId.ToString());
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Reason=MissingWorld"), *SpawnIdText);
		return nullptr;
	}

	for (TActorIterator<AGridMonsterActor> It(World); It; ++It)
	{
		AGridMonsterActor* ExistingMonster = *It;
		if (IsValid(ExistingMonster) && ExistingMonster->ResolvePersistenceId() == SpawnData.ObjectId)
		{
			UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Reason=PersistenceIdAlreadyExists ExistingActor=%s"),
				*SpawnIdText, SpawnData.CellX, SpawnData.CellY, *GetNameSafe(ExistingMonster));
			return nullptr;
		}
	}

	const FIntPoint SpawnCell(SpawnData.CellX, SpawnData.CellY);
	if (!bRestoreDead)
	{
		for (const TPair<FGuid, TObjectPtr<AGridMonsterActor>>& Pair : SpawnedMonsterActors)
		{
			const AGridMonsterActor* SpawnedMonster = Pair.Value.Get();
			if (IsValid(SpawnedMonster) && !SpawnedMonster->IsDead() && SpawnedMonster->CurrentCell == SpawnCell)
			{
				UE_LOG(LogGridMonsterState, Error,
					TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Reason=GeneratedMonsterCellConflict OccupantSpawnId=%s"), *SpawnIdText,
					SpawnCell.X, SpawnCell.Y, *Pair.Key.ToString(EGuidFormats::DigitsWithHyphens));
				return nullptr;
			}
		}
		if (IsPartyOnCell(SpawnCell.X, SpawnCell.Y))
		{
			UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Reason=PartyOccupiesCell"), *SpawnIdText, SpawnCell.X,
				SpawnCell.Y);
			return nullptr;
		}
		if (const UGridMonsterOccupancySubsystem* Occupancy = World->GetSubsystem<UGridMonsterOccupancySubsystem>())
		{
			if (Occupancy->IsCellBlocked(SpawnCell))
			{
				UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Reason=MonsterOccupancyConflict"), *SpawnIdText,
					SpawnCell.X, SpawnCell.Y);
				return nullptr;
			}
		}
	}

	AGridMonsterActor* Monster =
		World->SpawnActorDeferred<AGridMonsterActor>(MonsterActorClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Monster)
	{
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Class=%s Reason=DeferredSpawnFailed"),
			*SpawnIdText, SpawnCell.X, SpawnCell.Y, *Definition->MonsterId.ToString(), *GetPathNameSafe(MonsterActorClass.Get()));
		return nullptr;
	}

	Monster->HomeDungeonLevelId = GridLevelRuntimeMonstersResolveRuntimeStateLevelId(DungeonAsset, CurrentDungeonLevelId);
	Monster->bMonsterEnabled = true;
	Monster->bRuntimeLevelActive = true;
	if (!Monster->InitializeMonster(Definition, SpawnData.ObjectId, SpawnCell, SpawnData.InitialFacing, SpawnData.EncounterGroupId))
	{
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Class=%s Reason=InitializationFailed"),
			*SpawnIdText, SpawnCell.X, SpawnCell.Y, *Definition->MonsterId.ToString(), *GetPathNameSafe(MonsterActorClass.Get()));
		Monster->Destroy();
		return nullptr;
	}

	UGameplayStatics::FinishSpawningActor(Monster, SpawnTransform);
	if (!IsValid(Monster))
	{
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Reason=FinishSpawningFailed"), *SpawnIdText,
			SpawnCell.X, SpawnCell.Y, *Definition->MonsterId.ToString());
		return nullptr;
	}

	Monster->SetActorLocation(SpawnTransform.GetLocation());
	Monster->ApplyFacingRotation();
	ApplyMonsterPlacementMetadata(Monster);
	if (Monster->DeathComponent)
	{
		Monster->DeathComponent->InitializeDeathComponent(this);
	}

	bool bOccupancyInitialized = bRestoreDead;
	if (!bRestoreDead)
	{
		if (UGridMonsterMovementComponent* Movement = Monster->FindComponentByClass<UGridMonsterMovementComponent>())
		{
			bOccupancyInitialized = Movement->IsInitialized() || Movement->InitializeMovement(this);
		}
		else if (UGridMonsterOccupancySubsystem* Occupancy = World->GetSubsystem<UGridMonsterOccupancySubsystem>())
		{
			bOccupancyInitialized = Occupancy->RegisterMonster(Monster, SpawnCell);
		}
	}
	if (!bOccupancyInitialized)
	{
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Reason=OccupancyInitializationFailed"),
			*SpawnIdText, SpawnCell.X, SpawnCell.Y, *Definition->MonsterId.ToString());
		Monster->Destroy();
		return nullptr;
	}

	FString PresentationError;
	if (!Monster->ValidatePresentationSetup(PresentationError))
	{
		PresentationError.ReplaceInline(TEXT("\n"), TEXT(" | "));
		UE_LOG(LogGridMonsterState, Warning, TEXT("[GridMonsterSpawn] PresentationWarning SpawnId=%s DefinitionId=%s Actor=%s Reason=%s"), *SpawnIdText,
			*Definition->MonsterId.ToString(), *GetNameSafe(Monster), *PresentationError);
	}
	SpawnedMonsterActors.Add(SpawnData.ObjectId, Monster);

	if (RestoreState && !Monster->RestoreRuntimeMonsterState(*RestoreState, this))
	{
		SpawnedMonsterActors.Remove(SpawnData.ObjectId);
		SetMonsterRuntimeLevelActive(Monster, false);
		Monster->Destroy();
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Reason=RestoreStateFailed"), *SpawnIdText,
			SpawnCell.X, SpawnCell.Y, *Definition->MonsterId.ToString());
		return nullptr;
	}

	UE_LOG(LogGridMonsterState, Log, TEXT("[GridMonsterSpawn] Spawned SpawnId=%s DefinitionId=%s Class=%s Cell=(%d,%d) Facing=%s Encounter=%s RuntimeLevel=%s"),
		*SpawnIdText, *Definition->MonsterId.ToString(), *Monster->GetClass()->GetPathName(), SpawnCell.X, SpawnCell.Y,
		*GridLevelRuntimeMonstersGetEdgeText(SpawnData.InitialFacing), *SpawnData.EncounterGroupId.ToString(), *Monster->HomeDungeonLevelId.ToString());
	return Monster;
}
