#include "Runtime/GridLevelRuntimeActor.h"

#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GrimrockGameMode.h"
#include "Runtime/Monsters/GridMonsterActor.h"

namespace
{
	const FName GridLevelRuntimeDiagnosticsSingleLevelRuntimeStateId(TEXT("SingleLevel"));

	FName GridLevelRuntimeDiagnosticsResolveRuntimeStateLevelId(const UGridDungeonAsset* DungeonAsset, FName CurrentDungeonLevelId)
	{
		if (DungeonAsset && !CurrentDungeonLevelId.IsNone())
		{
			return CurrentDungeonLevelId;
		}

		return GridLevelRuntimeDiagnosticsSingleLevelRuntimeStateId;
	}

	FString GridLevelRuntimeDiagnosticsGetRuntimeWorldTypeText(const UWorld* World)
	{
		if (!World)
		{
			return TEXT("None");
		}

		switch (World->WorldType)
		{
			case EWorldType::Game:
				return TEXT("Game");

			case EWorldType::PIE:
				return TEXT("PIE");

			case EWorldType::Editor:
				return TEXT("Editor");

			case EWorldType::EditorPreview:
				return TEXT("EditorPreview");

			case EWorldType::GamePreview:
				return TEXT("GamePreview");

			case EWorldType::Inactive:
				return TEXT("Inactive");

			default:
				return FString::Printf(TEXT("Unknown(%d)"), static_cast<int32>(World->WorldType));
		}
	}

	FString GridLevelRuntimeDiagnosticsGetRuntimeEdgeText(EGridEdge Edge)
	{
		if (const UEnum* EdgeEnum = StaticEnum<EGridEdge>())
		{
			return EdgeEnum->GetNameStringByValue(static_cast<int64>(Edge));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(Edge));
	}

	FString GridLevelRuntimeDiagnosticsGetRuntimeBoolText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	int32 GridLevelRuntimeDiagnosticsCountRuntimeTransitionObjects(const UGridLevelAsset* InLevelAsset)
	{
		if (!InLevelAsset)
		{
			return 0;
		}

		int32 TransitionCount = 0;
		for (const FGridLevelObjectData& ObjectData : InLevelAsset->Objects)
		{
			if (ObjectData.Behavior.Transition.bIsTransition)
			{
				++TransitionCount;
			}
		}

		return TransitionCount;
	}

	int32 GridLevelRuntimeDiagnosticsCountRemovedRuntimeObjects(const FGridLevelRuntimeState* RuntimeState)
	{
		if (!RuntimeState)
		{
			return 0;
		}

		int32 RemovedCount = 0;
		for (const TPair<FGuid, FGridRuntimeObjectPresenceState>& Pair : RuntimeState->ObjectPresence)
		{
			if (Pair.Value.bRemovedFromInitialPlacement)
			{
				++RemovedCount;
			}
		}
		return RemovedCount;
	}

	int32 GridLevelRuntimeDiagnosticsCountHiddenFloorCells(const UGridLevelAsset* InLevelAsset, const AGridLevelRuntimeActor* RuntimeActor)
	{
		if (!InLevelAsset || !RuntimeActor)
		{
			return 0;
		}

		int32 HiddenFloorCells = 0;
		for (int32 Y = 0; Y < InLevelAsset->Height; ++Y)
		{
			for (int32 X = 0; X < InLevelAsset->Width; ++X)
			{
				if (RuntimeActor->ShouldHideCellFloor(X, Y))
				{
					++HiddenFloorCells;
				}
			}
		}

		return HiddenFloorCells;
	}

	void GridLevelRuntimeDiagnosticsGetWorldMonsters(const UWorld* World, TArray<AGridMonsterActor*>& OutMonsters)
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
FString AGridLevelRuntimeActor::GetRuntimeDebugSummary() const
{
	FString Result;

	Result += FString::Printf(TEXT("Grid Runtime | Level=%s\n"), LevelAsset ? *LevelAsset->GetName() : TEXT("None"));
	Result += FString::Printf(TEXT("Runtime Actors=%d\n"), SpawnedRuntimeObjectActors.Num());
	Result += FString::Printf(TEXT("Spawned Items=%d\n"), SpawnedItemActors.Num());
	Result += FString::Printf(TEXT("Spawned Monsters=%d Failures=%d\n"), GetSpawnedMonsterActorCount(), RuntimeMonsterSpawnFailureCount);
	Result += ActivationComponent ? ActivationComponent->GetDebugSummary() : TEXT("Activation | Missing");
	Result += TEXT("\n");
	Result += DoorSystemComponent ? DoorSystemComponent->GetDebugSummary() : TEXT("Doors | Missing");
	return Result;
}

void AGridLevelRuntimeActor::LogRuntimeDebugSummary() const
{
	const FString Summary = GetRuntimeDebugSummary();
	UE_LOG(LogTemp, Log, TEXT("%s"), *Summary);
	if (ActivationComponent)
	{
		ActivationComponent->LogDebugSummary();
	}
	if (DoorSystemComponent)
	{
		DoorSystemComponent->LogDebugSummary();
	}
}

FString AGridLevelRuntimeActor::GetLevelAssetDiagnostics() const
{
	const UWorld* World = GetWorld();
	FString WorldType = TEXT("None");
	FString MapName = TEXT("None");

	if (World)
	{
		MapName = World->GetMapName();
		switch (World->WorldType)
		{
			case EWorldType::Game:
				WorldType = TEXT("Game");
				break;

			case EWorldType::PIE:
				WorldType = TEXT("PIE");
				break;

			case EWorldType::Editor:
				WorldType = TEXT("Editor");
				break;

			case EWorldType::EditorPreview:
				WorldType = TEXT("EditorPreview");
				break;

			case EWorldType::GamePreview:
				WorldType = TEXT("GamePreview");
				break;

			case EWorldType::Inactive:
				WorldType = TEXT("Inactive");
				break;

			default:
				WorldType = FString::Printf(TEXT("Unknown(%d)"), static_cast<int32>(World->WorldType));
				break;
		}
	}

	FString Result;
	Result += TEXT("Grid LevelAsset Diagnostics\n");
	Result += FString::Printf(TEXT("RuntimeActor=%s\n"), *GetPathName());
#if WITH_EDITOR
	Result += FString::Printf(TEXT("ActorLabel=%s\n"), *GetActorLabel());
#else
	Result += FString::Printf(TEXT("ActorName=%s\n"), *GetName());
#endif
	Result += FString::Printf(TEXT("World=%s\n"), *GetNameSafe(World));
	Result += FString::Printf(TEXT("WorldType=%s\n"), *WorldType);
	Result += FString::Printf(TEXT("Map=%s\n"), *MapName);
	Result += FString::Printf(TEXT("DungeonAsset=%s\n"), DungeonAsset ? *DungeonAsset->GetPathName() : TEXT("None"));
	Result += FString::Printf(TEXT("CurrentDungeonLevelId=%s\n"), *CurrentDungeonLevelId.ToString());
	Result += FString::Printf(TEXT("LevelAsset=%s\n"), LevelAsset ? *LevelAsset->GetPathName() : TEXT("None"));

	const FGridLevelRuntimeState* RuntimeState = FindRuntimeStateForCurrentLevel();
	Result += FString::Printf(TEXT("HasRuntimeState=%s\n"), RuntimeState && RuntimeState->bHasBeenVisited ? TEXT("true") : TEXT("false"));
	Result += FString::Printf(TEXT("RuntimeRemovedObjects=%d\n"), GridLevelRuntimeDiagnosticsCountRemovedRuntimeObjects(RuntimeState));
	Result += FString::Printf(TEXT("RuntimeDoors=%d\n"), RuntimeState ? RuntimeState->Doors.Num() : 0);
	Result += FString::Printf(TEXT("RuntimeItems=%d\n"), RuntimeState ? RuntimeState->Items.Num() : 0);
	Result += FString::Printf(TEXT("RuntimeReceptacles=%d\n"), RuntimeState ? RuntimeState->Receptacles.Num() : 0);

	const FName RuntimeLevelId = GridLevelRuntimeDiagnosticsResolveRuntimeStateLevelId(DungeonAsset, CurrentDungeonLevelId);
	TArray<AGridMonsterActor*> DiagnosticMonsters;
	GridLevelRuntimeDiagnosticsGetWorldMonsters(World, DiagnosticMonsters);
	int32 AssociatedMonsterCount = 0;
	int32 InvalidMonsterIdCount = 0;
	TMap<FGuid, int32> MonsterIdCounts;
	for (AGridMonsterActor* Monster : DiagnosticMonsters)
	{
		if (Monster->ResolveRuntimeDungeonLevelId(RuntimeLevelId) != RuntimeLevelId)
		{
			continue;
		}

		++AssociatedMonsterCount;
		const FGuid PersistenceId = Monster->ResolvePersistenceId();
		if (!PersistenceId.IsValid())
		{
			++InvalidMonsterIdCount;
			continue;
		}
		++MonsterIdCounts.FindOrAdd(PersistenceId);
	}

	int32 DuplicateMonsterIdCount = 0;
	for (const TPair<FGuid, int32>& Pair : MonsterIdCounts)
	{
		DuplicateMonsterIdCount += Pair.Value > 1 ? Pair.Value : 0;
	}

	int32 SavedDeadMonsterCount = 0;
	if (RuntimeState)
	{
		for (const TPair<FGuid, FGridRuntimeMonsterState>& Pair : RuntimeState->Monsters)
		{
			SavedDeadMonsterCount += Pair.Value.bIsDead ? 1 : 0;
		}
	}

	Result += FString::Printf(TEXT("MonstersAssociated=%d RuntimeMonsters=%d RuntimeDeadMonsters=%d InvalidMonsterIds=%d DuplicateMonsterIds=%d\n"),
		AssociatedMonsterCount, RuntimeState ? RuntimeState->Monsters.Num() : 0, SavedDeadMonsterCount, InvalidMonsterIdCount, DuplicateMonsterIdCount);

	if (!LevelAsset)
	{
		Result += TEXT("Status=ERROR: missing LevelAsset reference.\n");
		return Result;
	}

	const int32 ExpectedCellCount = LevelAsset->Width * LevelAsset->Height;
	int32 NonEmptyCellCount = 0;
	int32 BlockingCellCount = 0;
	int32 CeilingCellCount = 0;
	const int32 TransitionObjectCount = GridLevelRuntimeDiagnosticsCountRuntimeTransitionObjects(LevelAsset);
	const int32 HiddenFloorCellCount = GridLevelRuntimeDiagnosticsCountHiddenFloorCells(LevelAsset, this);

	for (const FGridLevelCellData& Cell : LevelAsset->Cells)
	{
		if (Cell.CellType != EGridCellType::Empty)
		{
			++NonEmptyCellCount;
		}
		if (Cell.bBlocksOccupancy)
		{
			++BlockingCellCount;
		}
		if (Cell.bHasCeiling)
		{
			++CeilingCellCount;
		}
	}

	Result += FString::Printf(TEXT("AssetPackage=%s\n"), *GetNameSafe(LevelAsset->GetOutermost()));
	Result += FString::Printf(TEXT("GridSize=%dx%d\n"), LevelAsset->Width, LevelAsset->Height);
	Result += FString::Printf(TEXT("CellSize=%.2f\n"), LevelAsset->CellSize);
	Result += FString::Printf(TEXT("StartCell=(%d,%d) StartFacing=%s StartCellValid=%s\n"), LevelAsset->StartCellX, LevelAsset->StartCellY,
		StaticEnum<EGridEdge>() ? *StaticEnum<EGridEdge>()->GetNameStringByValue(static_cast<int64>(LevelAsset->StartFacing)) : TEXT("Unknown"),
		LevelAsset->IsStartCellValid() ? TEXT("true") : TEXT("false"));
	Result += FString::Printf(TEXT("Cells=%d ExpectedCells=%d\n"), LevelAsset->Cells.Num(), ExpectedCellCount);
	Result += FString::Printf(TEXT("NonEmptyCells=%d BlockingCells=%d CeilingCells=%d\n"), NonEmptyCellCount, BlockingCellCount, CeilingCellCount);
	Result += FString::Printf(TEXT("Objects=%d Links=%d TransitionObjects=%d HiddenFloorCells=%d\n"), LevelAsset->Objects.Num(), LevelAsset->Links.Num(),
		TransitionObjectCount, HiddenFloorCellCount);
	Result += FString::Printf(TEXT("ObjectArchetypesOnRuntimeActor=%d\n"), ObjectArchetypes.Num());
	Result += FString::Printf(TEXT("FloorMesh=%s WallMesh=%s CeilingMesh=%s\n"), *GetNameSafe(FloorMesh), *GetNameSafe(WallMesh), *GetNameSafe(CeilingMesh));

	if (LevelAsset->Cells.Num() != ExpectedCellCount)
	{
		Result += TEXT("Status=WARNING: Cells.Num does not match Width*Height. Run EnsureLevelReady from the editor actor.\n");
	}
	else
	{
		Result += TEXT("Status=OK\n");
	}

	return Result;
}

void AGridLevelRuntimeActor::LogLevelAssetDiagnostics() const
{
	const FString Diagnostics = GetLevelAssetDiagnostics();
	UE_LOG(LogTemp, Log, TEXT("%s"), *Diagnostics);
}

FString AGridLevelRuntimeActor::GetPIEReadinessDiagnostics() const
{
	const UWorld* World = GetWorld();
	const bool bHasGameWorld = World && World->IsGameWorld();
	const bool bHasRequiredMeshes = FloorMesh && WallMesh && CeilingMesh;
	const bool bHasValidStart = LevelAsset && LevelAsset->IsStartCellValid();

	int32 NullArchetypeCount = 0;
	for (const TObjectPtr<UGridObjectArchetypeAsset>& Archetype : ObjectArchetypes)
	{
		if (!Archetype)
		{
			++NullArchetypeCount;
		}
	}

	AGrimrockPartyPawn* FoundPartyPawn = nullptr;
	AGameModeBase* ActiveGameMode = nullptr;
	if (World)
	{
		ActiveGameMode = World->GetAuthGameMode();
		for (TActorIterator<AGrimrockPartyPawn> It(World); It; ++It)
		{
			FoundPartyPawn = *It;
			break;
		}
	}

	FString Result;
	Result += TEXT("GridLevelRuntimeActor PIE Readiness\n");
	Result += FString::Printf(TEXT("RuntimeActor: %s\n"), *GetName());
	Result += FString::Printf(TEXT("World: %s\n"), World ? *World->GetMapName() : TEXT("None"));
	Result += FString::Printf(TEXT("WorldType: %s\n"), *GridLevelRuntimeDiagnosticsGetRuntimeWorldTypeText(World));
	Result += FString::Printf(TEXT("IsGameWorld: %s\n"), *GridLevelRuntimeDiagnosticsGetRuntimeBoolText(bHasGameWorld));
	Result += FString::Printf(TEXT("DungeonAsset: %s\n"), DungeonAsset ? *DungeonAsset->GetPathName() : TEXT("None"));
	Result += FString::Printf(TEXT("CurrentDungeonLevelId: %s\n"), *CurrentDungeonLevelId.ToString());
	Result += FString::Printf(TEXT("LevelAsset: %s\n"), LevelAsset ? *LevelAsset->GetPathName() : TEXT("None"));
	Result += FString::Printf(TEXT("ApplyLevelStartOnBeginPlay: %s\n"), *GridLevelRuntimeDiagnosticsGetRuntimeBoolText(bApplyLevelStartOnBeginPlay));

	if (LevelAsset)
	{
		Result += FString::Printf(TEXT("Start: Cell=(%d,%d) Facing=%s Valid=%s\n"), LevelAsset->StartCellX, LevelAsset->StartCellY,
			*GridLevelRuntimeDiagnosticsGetRuntimeEdgeText(LevelAsset->StartFacing), *GridLevelRuntimeDiagnosticsGetRuntimeBoolText(bHasValidStart));
		Result += FString::Printf(TEXT("Asset Stats: Cells=%d Objects=%d Links=%d TransitionObjects=%d\n"), LevelAsset->Cells.Num(), LevelAsset->Objects.Num(),
			LevelAsset->Links.Num(), GridLevelRuntimeDiagnosticsCountRuntimeTransitionObjects(LevelAsset));
	}
	else
	{
		Result += TEXT("Start: Cell=None Facing=None Valid=false\n");
		Result += TEXT("Asset Stats: Cells=0 Objects=0 Links=0 TransitionObjects=0\n");
	}

	Result += FString::Printf(TEXT("Meshes: Floor=%s Wall=%s Ceiling=%s\n"), *GetNameSafe(FloorMesh), *GetNameSafe(WallMesh), *GetNameSafe(CeilingMesh));
	Result += FString::Printf(TEXT("ObjectArchetypes: Count=%d NullEntries=%d\n"), ObjectArchetypes.Num(), NullArchetypeCount);
	Result += FString::Printf(TEXT("Components: Activation=%s Doors=%s EditorPreview=%s\n"),
		*GridLevelRuntimeDiagnosticsGetRuntimeBoolText(ActivationComponent != nullptr),
		*GridLevelRuntimeDiagnosticsGetRuntimeBoolText(DoorSystemComponent != nullptr),
		*GridLevelRuntimeDiagnosticsGetRuntimeBoolText(EditorPreviewComponent != nullptr));
	Result += FString::Printf(TEXT("EditorPreviewInGameWorld: %s\n"),
		bHasGameWorld ? TEXT("Runtime objects are used; editor preview rebuild is skipped.") : TEXT("Editor preview may be used outside PIE/game world."));
	Result += FString::Printf(TEXT("PartyPawnInCurrentWorld: %s\n"), FoundPartyPawn ? *FoundPartyPawn->GetName() : TEXT("None"));
	Result += FString::Printf(TEXT("ActiveGameMode: %s\n"), ActiveGameMode ? *ActiveGameMode->GetClass()->GetName() : TEXT("None"));
	Result += FString::Printf(TEXT("GameModeDefaultPawnClass: %s\n"),
		ActiveGameMode && ActiveGameMode->DefaultPawnClass ? *ActiveGameMode->DefaultPawnClass->GetName() : TEXT("None"));
	Result += FString::Printf(TEXT("GameModePlayerControllerClass: %s\n"),
		ActiveGameMode && ActiveGameMode->PlayerControllerClass ? *ActiveGameMode->PlayerControllerClass->GetName() : TEXT("None"));

	if (ActiveGameMode && ActiveGameMode->GetClass() == AGrimrockGameMode::StaticClass())
	{
		Result += TEXT("GameModeNote: Native AGrimrockGameMode spawns the native C++ pawn unless a Blueprint override is used.\n");
	}
	else if (ActiveGameMode && ActiveGameMode->GetClass()->IsChildOf(AGrimrockGameMode::StaticClass()))
	{
		Result += TEXT("GameModeNote: Blueprint GameMode is active; its Default Pawn Class and Player Controller Class overrides are used.\n");
	}
	else if (ActiveGameMode)
	{
		Result += TEXT("GameModeNote: Active GameMode is not derived from AGrimrockGameMode.\n");
	}
	else
	{
		Result += TEXT("GameModeNote: No active GameMode in this world yet. Run PIE to inspect the spawned GameMode.\n");
	}

	if (!LevelAsset)
	{
		Result += TEXT("Status: ERROR - LevelAsset is null.");
	}
	else if (!bHasValidStart)
	{
		Result += TEXT("Status: ERROR - LevelAsset start cell is invalid.");
	}
	else if (!bHasRequiredMeshes)
	{
		Result += TEXT("Status: ERROR - FloorMesh, WallMesh or CeilingMesh is missing.");
	}
	else if (!ActivationComponent || !DoorSystemComponent)
	{
		Result += TEXT("Status: ERROR - Runtime activation or door component is missing.");
	}
	else if (!FoundPartyPawn)
	{
		Result += TEXT("Status: WARNING - No AGrimrockPartyPawn exists in the current world. PIE can still work if GameMode spawns one.");
	}
	else
	{
		Result += TEXT("Status: OK - Runtime actor is ready for PIE with this LevelAsset.");
	}

	return Result;
}

void AGridLevelRuntimeActor::LogPIEReadinessDiagnostics() const
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *GetPIEReadinessDiagnostics());
}

void AGridLevelRuntimeActor::ShowRuntimeDebugSummary(float Duration) const
{
	if (!GEngine)
	{
		return;
	}
	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Green, GetRuntimeDebugSummary());
}
