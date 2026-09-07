#include "Runtime/GridLevelRuntimeActor.h"

#include "Core/GridDirectionUtils.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
	const FName GridTypedMonsterSingleLevelRuntimeStateId(TEXT("SingleLevel"));

	bool GridTypedMonsterIsCardinalFacing(EGridEdge Facing)
	{
		return Facing == EGridEdge::North || Facing == EGridEdge::East || Facing == EGridEdge::South || Facing == EGridEdge::West;
	}

	FName GridTypedMonsterResolveRuntimeStateLevelId(const UGridDungeonAsset* DungeonAsset, FName CurrentDungeonLevelId)
	{
		return DungeonAsset && !CurrentDungeonLevelId.IsNone() ? CurrentDungeonLevelId : GridTypedMonsterSingleLevelRuntimeStateId;
	}

	FString GridTypedMonsterGetEdgeText(EGridEdge Edge)
	{
		if (const UEnum* EdgeEnum = StaticEnum<EGridEdge>())
		{
			return EdgeEnum->GetNameStringByValue(static_cast<int64>(Edge));
		}
		return FString::Printf(TEXT("%d"), static_cast<int32>(Edge));
	}
}

bool AGridLevelRuntimeActor::ResolveMonsterSpawn(
	const FGridMonsterSpawnInstance& SpawnData, UGridMonsterDefinitionAsset*& OutDefinition, TSubclassOf<AGridMonsterActor>& OutActorClass, FString& OutError) const
{
	OutDefinition = nullptr;
	OutActorClass = nullptr;
	OutError.Empty();

	TArray<FString> Errors;
	if (!LevelAsset)
	{
		Errors.Add(TEXT("LevelAsset is missing."));
	}
	if (!SpawnData.SpawnId.IsValid())
	{
		Errors.Add(TEXT("SpawnId is invalid."));
	}
	if (!LevelAsset || !LevelAsset->IsValidCoord(SpawnData.CellX, SpawnData.CellY))
	{
		Errors.Add(FString::Printf(TEXT("Cell=(%d,%d) is outside the level."), SpawnData.CellX, SpawnData.CellY));
	}
	else if (!IsWalkableCell(SpawnData.CellX, SpawnData.CellY))
	{
		Errors.Add(FString::Printf(TEXT("Cell=(%d,%d) does not allow monster occupancy."), SpawnData.CellX, SpawnData.CellY));
	}
	if (!GridTypedMonsterIsCardinalFacing(SpawnData.Facing))
	{
		Errors.Add(TEXT("Facing is not cardinal."));
	}

	UGridMonsterDefinitionAsset* Definition = SpawnData.MonsterDefinition.Get();
	if (!Definition)
	{
		Errors.Add(TEXT("MonsterDefinition is missing."));
	}
	else
	{
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

bool AGridLevelRuntimeActor::GetMonsterSpawnTransform(const FGridMonsterSpawnInstance& SpawnData, FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;
	if (!LevelAsset || !LevelAsset->IsValidCoord(SpawnData.CellX, SpawnData.CellY) || !GridTypedMonsterIsCardinalFacing(SpawnData.Facing))
	{
		return false;
	}

	OutTransform = FTransform(
		FRotator(0.0f, GridDirectionUtils::ToYaw(SpawnData.Facing), 0.0f), GetCellCenterWorld(SpawnData.CellX, SpawnData.CellY), FVector::OneVector);
	return IsSafeRuntimeRenderTransform(OutTransform);
}

bool AGridLevelRuntimeActor::StoreMonsterPlacementState(const FGridMonsterSpawnInstance& SpawnData, AGridMonsterActor* Monster, bool bIsSpawned)
{
	if (!SpawnData.SpawnId.IsValid())
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

	FGridRuntimeMonsterPlacementState& PlacementState = State->MonsterPlacements.FindOrAdd(SpawnData.SpawnId);
	PlacementState.SpawnId = SpawnData.SpawnId;
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
		State->Monsters.Remove(SpawnData.SpawnId);
	}

	State->bHasBeenVisited = true;
	return true;
}

bool AGridLevelRuntimeActor::DespawnMonsterSpawnActor(const FGridMonsterSpawnInstance& SpawnData, bool bRememberState, bool bEmitEvent)
{
	AGridMonsterActor* Monster = FindSpawnedMonsterActor(SpawnData.SpawnId);
	if (!Monster)
	{
		return !bRememberState || StoreMonsterPlacementState(SpawnData, nullptr, false);
	}

	if (bRememberState && !StoreMonsterPlacementState(SpawnData, Monster, false))
	{
		UE_LOG(LogGridMonsterState, Warning, TEXT("[GridMonsterLifecycle] DespawnRejected SpawnId=%s Reason=StateCaptureFailed"),
			*SpawnData.SpawnId.ToString(EGuidFormats::DigitsWithHyphens));
		return false;
	}

	AbortActiveCombatAndMonsterActions();
	SetMonsterRuntimeLevelActive(Monster, false);
	SpawnedMonsterActors.Remove(SpawnData.SpawnId);
	Monster->Destroy();

	UE_LOG(LogGridMonsterState, Log, TEXT("[GridMonsterLifecycle] Despawned SpawnId=%s Encounter=%s"),
		*SpawnData.SpawnId.ToString(EGuidFormats::DigitsWithHyphens), *SpawnData.EncounterGroupId.ToString());

	if (bEmitEvent)
	{
		ExecuteLinksFromRuntimeObject(SpawnData.SpawnId, EGridObjectEvent::MonsterDespawned);
	}
	return true;
}

AGridMonsterActor* AGridLevelRuntimeActor::AddMonsterSpawnActor(const FGridMonsterSpawnInstance& Placement, const FGridRuntimeMonsterState* RestoreState)
{
	FGridMonsterSpawnInstance SpawnData = Placement;
	if (RestoreState)
	{
		SpawnData.CellX = RestoreState->CellX;
		SpawnData.CellY = RestoreState->CellY;
		SpawnData.Facing = GridTypedMonsterIsCardinalFacing(RestoreState->Facing) ? RestoreState->Facing : Placement.Facing;
		SpawnData.EncounterGroupId = RestoreState->EncounterGroupId;
	}

	const bool bRestoreDead =
		RestoreState && (RestoreState->bIsDead || RestoreState->CurrentHealth <= 0 || RestoreState->MonsterState == EGridMonsterState::Dead);
	const FString SpawnIdText = SpawnData.SpawnId.ToString(EGuidFormats::DigitsWithHyphens);

	if (SpawnedMonsterActors.Contains(SpawnData.SpawnId))
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
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Definition=%s Reason=%s"), *SpawnIdText, SpawnData.CellX,
			SpawnData.CellY, *GetPathNameSafe(SpawnData.MonsterDefinition.Get()), *ResolutionError);
		return nullptr;
	}

	FTransform SpawnTransform;
	if (!GetMonsterSpawnTransform(SpawnData, SpawnTransform))
	{
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Definition=%s Reason=InvalidTransform"), *SpawnIdText,
			SpawnData.CellX, SpawnData.CellY, *GetPathNameSafe(SpawnData.MonsterDefinition.Get()));
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
		if (IsValid(ExistingMonster) && ExistingMonster->ResolvePersistenceId() == SpawnData.SpawnId)
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

	Monster->HomeDungeonLevelId = GridTypedMonsterResolveRuntimeStateLevelId(DungeonAsset, CurrentDungeonLevelId);
	Monster->bMonsterEnabled = true;
	Monster->bRuntimeLevelActive = true;
	if (!Monster->InitializeMonster(Definition, SpawnData.SpawnId, SpawnCell, SpawnData.Facing, SpawnData.EncounterGroupId))
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
	SpawnedMonsterActors.Add(SpawnData.SpawnId, Monster);

	if (RestoreState && !Monster->RestoreRuntimeMonsterState(*RestoreState, this))
	{
		SpawnedMonsterActors.Remove(SpawnData.SpawnId);
		SetMonsterRuntimeLevelActive(Monster, false);
		Monster->Destroy();
		UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Reason=RestoreStateFailed"), *SpawnIdText,
			SpawnCell.X, SpawnCell.Y, *Definition->MonsterId.ToString());
		return nullptr;
	}

	UE_LOG(LogGridMonsterState, Log, TEXT("[GridMonsterSpawn] Spawned SpawnId=%s DefinitionId=%s Class=%s Cell=(%d,%d) Facing=%s Encounter=%s RuntimeLevel=%s"),
		*SpawnIdText, *Definition->MonsterId.ToString(), *Monster->GetClass()->GetPathName(), SpawnCell.X, SpawnCell.Y,
		*GridTypedMonsterGetEdgeText(SpawnData.Facing), *SpawnData.EncounterGroupId.ToString(), *Monster->HomeDungeonLevelId.ToString());
	return Monster;
}
