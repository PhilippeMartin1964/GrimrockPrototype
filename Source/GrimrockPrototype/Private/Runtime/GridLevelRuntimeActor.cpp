#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPIEPlaytestRequest.h"
#include "Runtime/GridMonsterEncounterComponent.h"
#include "Core/GridTypes.h"
#include "Core/GridDirectionUtils.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridPlacementTransformResolver.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GrimrockGameMode.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridMechanismActor.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/Combat/GridPlayerAttackPresentationComponent.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterIdleVariationComponent.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridPitTrapdoorActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridWallLockActor.h"
#include "UI/ReadableMessageWidget.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName SingleLevelRuntimeStateId(TEXT("SingleLevel"));

	FName ResolveRuntimeStateLevelId(const UGridDungeonAsset* DungeonAsset, FName CurrentDungeonLevelId)
	{
		if (DungeonAsset && !CurrentDungeonLevelId.IsNone())
		{
			return CurrentDungeonLevelId;
		}

		return SingleLevelRuntimeStateId;
	}

	FString GetRuntimeEdgeText(EGridEdge Edge)
	{
		if (const UEnum* EdgeEnum = StaticEnum<EGridEdge>())
		{
			return EdgeEnum->GetNameStringByValue(static_cast<int64>(Edge));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(Edge));
	}

	FString GetRuntimeBoolText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	void GetWorldMonsters(const UWorld* World, TArray<AGridMonsterActor*>& OutMonsters)
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

bool AGridLevelRuntimeActor::IsSafeRuntimeRenderTransform(const FTransform& Transform)
{
	constexpr float MinAbsScale = 0.001f;
	const FVector Scale = Transform.GetScale3D();

	return Transform.IsValid() && !Transform.GetLocation().ContainsNaN() && !Scale.ContainsNaN() && Transform.GetRotation().IsNormalized() &&
		FMath::Abs(Scale.X) >= MinAbsScale && FMath::Abs(Scale.Y) >= MinAbsScale && FMath::Abs(Scale.Z) >= MinAbsScale;
}

void AGridLevelRuntimeActor::LogUnsafeInstanceTransform(
	const TCHAR* FunctionName, const UInstancedStaticMeshComponent* Component, int32 X, int32 Y, EGridEdge Edge, const FTransform& Transform) const
{
	UE_LOG(LogTemp, Error,
		TEXT("Unsafe runtime render transform skipped: Function=%s Component=%s StaticMesh=%s Cell=(%d,%d) Edge=%d Location=%s Rotation=%s Scale=%s"),
		FunctionName, *GetNameSafe(Component), *GetNameSafe(Component ? Component->GetStaticMesh() : nullptr), X, Y, static_cast<int32>(Edge),
		*Transform.GetLocation().ToCompactString(), *Transform.GetRotation().ToString(), *Transform.GetScale3D().ToCompactString());
}

void AGridLevelRuntimeActor::LogUnsafeObjectTransform(
	const TCHAR* FunctionName, const FGridWorldObjectInstance& ObjectData, const UStaticMesh* StaticMesh, const FTransform& Transform) const
{
	UE_LOG(LogTemp, Error,
		TEXT(
			"Unsafe runtime render transform skipped: Function=%s ObjectId=%s WorldObjectDefinitionId=%s Tag=%s Cell=(%d,%d) Edge=%d StaticMesh=%s Location=%s Rotation=%s Scale=%s"),
		FunctionName, *ObjectData.InstanceId.ToString(), *ObjectData.WorldObjectDefinitionId.ToString(), *ObjectData.Tag.ToString(), ObjectData.CellX, ObjectData.CellY,
		static_cast<int32>(ObjectData.WallSide), *GetNameSafe(StaticMesh), *Transform.GetLocation().ToCompactString(), *Transform.GetRotation().ToString(),
		*Transform.GetScale3D().ToCompactString());
}

void AGridLevelRuntimeActor::LogUnsafeItemTransform(const TCHAR* FunctionName, FName WorldObjectDefinitionId, const AActor* OwnerActor, const USceneComponent* AttachParent,
	const UStaticMesh* StaticMesh, const FTransform& Transform) const
{
	UE_LOG(LogTemp, Error,
		TEXT("Unsafe runtime item transform skipped: Function=%s WorldObjectDefinitionId=%s Owner=%s AttachParent=%s StaticMesh=%s Location=%s Rotation=%s Scale=%s"),
		FunctionName, *WorldObjectDefinitionId.ToString(), *GetNameSafe(OwnerActor), *GetNameSafe(AttachParent), *GetNameSafe(StaticMesh),
		*Transform.GetLocation().ToCompactString(), *Transform.GetRotation().ToString(), *Transform.GetScale3D().ToCompactString());
}

AGridLevelRuntimeActor::AGridLevelRuntimeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	FloorISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorISM"));
	FloorISM->SetupAttachment(SceneRoot);

	WallISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallISM"));
	WallISM->SetupAttachment(SceneRoot);

	CeilingISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CeilingISM"));
	CeilingISM->SetupAttachment(SceneRoot);

	ActivationComponent = CreateDefaultSubobject<UGridActivationComponent>(TEXT("ActivationComponent"));

	MonsterEncounterComponent = CreateDefaultSubobject<UGridMonsterEncounterComponent>(TEXT("MonsterEncounterComponent"));

	DoorSystemComponent = CreateDefaultSubobject<UGridDoorSystemComponent>(TEXT("DoorSystemComponent"));

	EditorPreviewComponent = CreateDefaultSubobject<UGridEditorPreviewComponent>(TEXT("EditorPreviewComponent"));

	PlayerAttackPresentationComponent = CreateDefaultSubobject<UGridPlayerAttackPresentationComponent>(TEXT("PlayerAttackPresentationComponent"));
}

void AGridLevelRuntimeActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!bRebuildInConstruction)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	RebuildLevel(EGridRuntimeRebuildMode::GeometryOnly);
}

void AGridLevelRuntimeActor::BeginPlay()
{
	Super::BeginPlay();

	if (GridPIEPlaytestRequest::Matches(this))
	{
		DungeonRuntimeState = FGridDungeonRuntimeState();
		UE_LOG(LogTemp, Log, TEXT("GridLevelRuntimeActor: fresh PIE dungeon state initialized without modifying save data."));
	}

	if (DungeonAsset)
	{
		if (!CurrentDungeonLevelId.IsNone())
		{
			if (UGridLevelAsset* ConfiguredDungeonLevel = DungeonAsset->GetLevelAssetById(CurrentDungeonLevelId))
			{
				LevelAsset = ConfiguredDungeonLevel;
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("GridLevelRuntimeActor: CurrentDungeonLevelId %s is not valid in DungeonAsset %s; keeping configured LevelAsset."),
					*CurrentDungeonLevelId.ToString(), *DungeonAsset->GetPathName());
			}
		}

		if (CurrentDungeonLevelId.IsNone())
		{
			for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
			{
				if (Entry.bEnabled && Entry.LevelAsset == LevelAsset)
				{
					CurrentDungeonLevelId = Entry.LevelId;
					break;
				}
			}

			if (CurrentDungeonLevelId.IsNone())
			{
				if (DungeonAsset->IsValidLevelId(DungeonAsset->DefaultLevelId))
				{
					CurrentDungeonLevelId = DungeonAsset->DefaultLevelId;
					LevelAsset = DungeonAsset->GetLevelAssetById(DungeonAsset->DefaultLevelId);
					UE_LOG(LogTemp, Log, TEXT("GridLevelRuntimeActor: using DungeonAsset DefaultLevelId %s at BeginPlay."), *CurrentDungeonLevelId.ToString());
				}
				else
				{
					UE_LOG(LogTemp, Warning,
						TEXT("GridLevelRuntimeActor: could not resolve CurrentDungeonLevelId from DungeonAsset %s; keeping configured LevelAsset %s."),
						*DungeonAsset->GetPathName(), LevelAsset ? *LevelAsset->GetPathName() : TEXT("None"));
				}
			}
		}
	}

	if (ActivationComponent)
	{
		ActivationComponent->Initialize(this);
		ActivationComponent->ResetRuntimeState();
	}
	if (DoorSystemComponent)
	{
		DoorSystemComponent->Initialize(this);
		DoorSystemComponent->ResetRuntimeState();
	}
	if (EditorPreviewComponent)
	{
		EditorPreviewComponent->Initialize(this);
	}
	RebuildLevel();
	ApplyCurrentLevelRuntimeState();
	if (ActivationComponent)
	{
		ActivationComponent->RefreshAllPressurePlates();
	}
}

FGridLevelRuntimeState* AGridLevelRuntimeActor::GetOrCreateRuntimeStateForCurrentLevel()
{
	const FName RuntimeLevelId = ResolveRuntimeStateLevelId(DungeonAsset, CurrentDungeonLevelId);
	FGridLevelRuntimeState& State = DungeonRuntimeState.LevelStates.FindOrAdd(RuntimeLevelId);
	State.LevelId = RuntimeLevelId;
	return &State;
}

const FGridLevelRuntimeState* AGridLevelRuntimeActor::FindRuntimeStateForCurrentLevel() const
{
	const FName RuntimeLevelId = ResolveRuntimeStateLevelId(DungeonAsset, CurrentDungeonLevelId);
	return DungeonRuntimeState.LevelStates.Find(RuntimeLevelId);
}

void AGridLevelRuntimeActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideCombatFeedback();
	Super::EndPlay(EndPlayReason);
}

void AGridLevelRuntimeActor::ClearVisuals(EGridRuntimeRebuildMode RebuildMode)
{
	if (FloorISM)
		FloorISM->ClearInstances();
	if (WallISM)
		WallISM->ClearInstances();
	if (CeilingISM)
	{
		CeilingISM->ClearInstances();
		CeilingISM->EmptyOverrideMaterials();
	}

	const bool bClearObjects = RebuildMode != EGridRuntimeRebuildMode::GeometryOnly;
	if (bClearObjects)
	{
		RuntimeMonsterSpawnFailureCount = 0;
		ClearRuntimeObjectActors();
		if (EditorPreviewComponent)
			EditorPreviewComponent->ClearPreviewObjects();
		if (DoorSystemComponent)
			DoorSystemComponent->ResetRuntimeState();
		if (ActivationComponent)
			ActivationComponent->ResetRuntimeState();
	}
}

bool AGridLevelRuntimeActor::IsValidCell(int32 X, int32 Y) const
{
	return LevelAsset && LevelAsset->IsValidCoord(X, Y);
}

const FGridLevelCellData& AGridLevelRuntimeActor::GetCell(int32 X, int32 Y) const
{
	check(LevelAsset);
	return LevelAsset->GetCell(X, Y);
}

FVector AGridLevelRuntimeActor::CellToWorld(int32 X, int32 Y, float ZOffset) const
{
	const float Size = LevelAsset ? LevelAsset->CellSize : 200.f;
	return GridOrigin + FVector(X * Size, Y * Size, ZOffset);
}

FVector AGridLevelRuntimeActor::GetCellCenterWorld(int32 X, int32 Y, float ZOffset) const
{
	const float CellSize = LevelAsset ? LevelAsset->CellSize : 200.f;
	return GetActorLocation() + GridOrigin + FVector((X * CellSize) + (CellSize * 0.5f), (Y * CellSize) + (CellSize * 0.5f), ZOffset);
}

void AGridLevelRuntimeActor::AddFloor(int32 X, int32 Y, float CellSize)
{
	const FVector Base = CellToWorld(X, Y, 0.f);
	const FVector CenterOffset(CellSize * 0.5f, CellSize * 0.5f, 0.f);
	const FTransform T(FRotator::ZeroRotator, Base + CenterOffset, FVector::OneVector);
	if (!IsSafeRuntimeRenderTransform(T))
	{
		LogUnsafeInstanceTransform(TEXT("AddFloor"), FloorISM, X, Y, EGridEdge::None, T);
		return;
	}
	FloorISM->AddInstance(T);
}

void AGridLevelRuntimeActor::AddCeiling(int32 X, int32 Y, float CellSize)
{
	const FVector Base = CellToWorld(X, Y, 200.f);
	const FVector CenterOffset(CellSize * 0.5f, CellSize * 0.5f, 0.f);
	const FTransform T(FRotator::ZeroRotator, Base + CenterOffset, FVector(1.f, 1.f, 1.f));
	if (!IsSafeRuntimeRenderTransform(T))
	{
		LogUnsafeInstanceTransform(TEXT("AddCeiling"), CeilingISM, X, Y, EGridEdge::None, T);
		return;
	}
	CeilingISM->AddInstance(T);
}

bool AGridLevelRuntimeActor::ShouldSuppressStandardWallForEdge(int32 X, int32 Y, EGridEdge Edge) const
{
	if (!LevelAsset || Edge == EGridEdge::None)
	{
		return false;
	}

	for (const FGridWorldObjectInstance& ObjectData : LevelAsset->WorldObjectInstances)
	{
		if (ObjectData.CellX == X && ObjectData.CellY == Y && ObjectData.WallSide == Edge)
		{
			const UGridWorldObjectDefinitionAsset* Definition = FindWorldObjectDefinition(ObjectData.WorldObjectDefinitionId);
			if (Definition && Definition->bReplacesStandardWall)
			{
				return true;
			}
		}
	}

	return false;
}

bool AGridLevelRuntimeActor::ShouldHideCellFloor(int32 CellX, int32 CellY) const
{
	if (!LevelAsset)
	{
		return false;
	}

	for (const FGridWorldObjectInstance& ObjectData : LevelAsset->WorldObjectInstances)
	{
		if (ObjectData.CellX != CellX || ObjectData.CellY != CellY)
		{
			continue;
		}

		const UGridWorldObjectDefinitionAsset* Definition = FindWorldObjectDefinition(ObjectData.WorldObjectDefinitionId);
		if (Definition && Definition->bHideCellFloor)
		{
			return true;
		}
	}

	return false;
}

void AGridLevelRuntimeActor::AddEdgeInstance(UInstancedStaticMeshComponent* TargetISM, int32 X, int32 Y, EGridEdge Edge, float CellSize)
{
	if (!TargetISM)
	{
		return;
	}
	const FVector Base = CellToWorld(X, Y, 0.f);
	FVector Pos = Base;
	FRotator Rot = FRotator::ZeroRotator;
	switch (Edge)
	{
		case EGridEdge::North:
		{
			Pos = Base + FVector(CellSize * 0.5f, CellSize, 0.f);
			Rot = FRotator(0.f, 0.f, 0.f);
			break;
		}
		case EGridEdge::South:
		{
			Pos = Base + FVector(CellSize * 0.5f, 0.f, 0.f);
			Rot = FRotator(0.f, 180.f, 0.f);
			break;
		}
		case EGridEdge::East:
		{
			Pos = Base + FVector(CellSize, CellSize * 0.5f, 0.f);
			Rot = FRotator(0.f, -90.f, 0.f);
			break;
		}
		case EGridEdge::West:
		{
			Pos = Base + FVector(0.f, CellSize * 0.5f, 0.f);
			Rot = FRotator(0.f, 90.f, 0.f);
			break;
		}
		default:
		{
			return;
		}
	}
	const FTransform T(Rot, Pos, FVector::OneVector);
	if (!IsSafeRuntimeRenderTransform(T))
	{
		LogUnsafeInstanceTransform(TEXT("AddEdgeInstance"), TargetISM, X, Y, Edge, T);
		return;
	}
	TargetISM->AddInstance(T);
}

void AGridLevelRuntimeActor::RebuildLevel(EGridRuntimeRebuildMode RebuildMode)
{
	ClearVisuals(RebuildMode);

	if (!LevelAsset || !FloorISM || !WallISM || !CeilingISM)
	{
		return;
	}

	LevelAsset->EnsureCellCount();
#if WITH_EDITOR
	for (const UGridWorldObjectDefinitionAsset* Definition : WorldObjectDefinitions)
	{
		if (!Definition)
		{
			continue;
		}

		TArray<FGridWorldObjectDefinitionValidationMessage> ValidationMessages;
		Definition->ValidateDefinition(ValidationMessages);

		bool bHasWarningOrError = false;
		for (const FGridWorldObjectDefinitionValidationMessage& Message : ValidationMessages)
		{
			if (Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Warning || Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Error)
			{
				bHasWarningOrError = true;
				break;
			}
		}

		if (bHasWarningOrError)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Definition->GetValidationSummary());
		}
	}
#endif
	if (ActivationComponent)
	{
		ActivationComponent->Initialize(this);
		ActivationComponent->RebuildIndexes();
	}
	if (MonsterEncounterComponent)
	{
		MonsterEncounterComponent->Initialize(this);
	}

	if (DoorSystemComponent)
	{
		DoorSystemComponent->Initialize(this);
		DoorSystemComponent->RebuildIndexes();
	}
	if (EditorPreviewComponent)
	{
		EditorPreviewComponent->Initialize(this);
	}
	FloorISM->SetStaticMesh(FloorMesh);
	WallISM->SetStaticMesh(WallMesh);
	CeilingISM->SetStaticMesh(CeilingMesh);

	const bool bIsGameWorld = GetWorld() && GetWorld()->IsGameWorld();

	if (!bIsGameWorld && CeilingEditorMaterial && CeilingMesh)
	{
		const int32 MaterialCount = CeilingMesh->GetStaticMaterials().Num();

		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			CeilingISM->SetMaterial(MaterialIndex, CeilingEditorMaterial);
		}
	}
	const float CellSize = LevelAsset->CellSize;
	for (int32 Y = 0; Y < LevelAsset->Height; ++Y)
	{
		for (int32 X = 0; X < LevelAsset->Width; ++X)
		{
			const FGridLevelCellData& Cell = LevelAsset->GetCell(X, Y);
			if (Cell.CellType == EGridCellType::Empty)
			{
				continue;
			}
			if (!ShouldHideCellFloor(X, Y))
			{
				AddFloor(X, Y, CellSize);
			}

			if (Cell.bHasCeiling)
			{
				AddCeiling(X, Y, CellSize);
			}
			auto DrawEdgeIfNeeded = [&](EGridEdge Edge, EGridWallType WallType, bool bShouldDraw)
			{
				if (!bShouldDraw || ShouldSuppressStandardWallForEdge(X, Y, Edge))
				{
					return;
				}
				switch (WallType)
				{
					case EGridWallType::Solid:
						AddEdgeInstance(WallISM, X, Y, Edge, CellSize);
						break;

					default:
						break;
				}
			};
			// Each stored cell edge is rendered independently. Opposite neighbor edges are not merged.
			DrawEdgeIfNeeded(EGridEdge::North, Cell.NorthWall, Cell.NorthWall != EGridWallType::None);
			DrawEdgeIfNeeded(EGridEdge::East, Cell.EastWall, Cell.EastWall != EGridWallType::None);
			DrawEdgeIfNeeded(EGridEdge::South, Cell.SouthWall, Cell.SouthWall != EGridWallType::None);
			DrawEdgeIfNeeded(EGridEdge::West, Cell.WestWall, Cell.WestWall != EGridWallType::None);
		}
	}
	if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
	{
		if (!GetWorld() || !GetWorld()->IsGameWorld())
		{
			if (EditorPreviewComponent)
			{
				EditorPreviewComponent->RebuildPreviewObjects();
			}
		}
	}
	if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
	{
		if (bIsGameWorld)
		{
			++RuntimeObjectRebuildGeneration;
			RebuildRuntimeObjects();
		}
	}
	if (bEnableRuntimeDebugLog)
	{
		LogRuntimeDebugSummary();
	}

	if (bEnableRuntimeDebugScreen)
	{
		ShowRuntimeDebugSummary();
	}
}

bool AGridLevelRuntimeActor::IsWalkableCell(int32 X, int32 Y) const
{
	if (!LevelAsset || !LevelAsset->IsValidCoord(X, Y))
	{
		return false;
	}
	const FGridLevelCellData& Cell = LevelAsset->GetCell(X, Y);
	if (Cell.CellType == EGridCellType::Empty)
	{
		return false;
	}
	if (Cell.bBlocksOccupancy)
	{
		return false;
	}
	return true;
}

bool AGridLevelRuntimeActor::TryGetNeighborCell(int32 X, int32 Y, EGridEdge Direction, int32& OutX, int32& OutY) const
{
	OutX = X;
	OutY = Y;

	if (!LevelAsset)
	{
		return false;
	}

	switch (Direction)
	{
		case EGridEdge::North:
			OutY = Y + 1;
			break;

		case EGridEdge::East:
			OutX = X + 1;
			break;

		case EGridEdge::South:
			OutY = Y - 1;
			break;

		case EGridEdge::West:
			OutX = X - 1;
			break;

		default:
			return false;
	}

	return LevelAsset->IsValidCoord(OutX, OutY);
}

EGridWallType AGridLevelRuntimeActor::GetWallOnEdge(int32 X, int32 Y, EGridEdge Edge) const
{
	if (!LevelAsset || !LevelAsset->IsValidCoord(X, Y))
	{
		return EGridWallType::Solid;
	}

	const FGridLevelCellData& Cell = LevelAsset->GetCell(X, Y);

	// Shared edges are intentionally directional: only the requested cell edge is authoritative.
	// The editor does not mirror this value to the neighboring cell.
	switch (Edge)
	{
		case EGridEdge::North:
			return Cell.NorthWall;

		case EGridEdge::East:
			return Cell.EastWall;

		case EGridEdge::South:
			return Cell.SouthWall;

		case EGridEdge::West:
			return Cell.WestWall;

		default:
			return EGridWallType::Solid;
	}
}

bool AGridLevelRuntimeActor::CanMove(int32 FromX, int32 FromY, EGridEdge Direction) const
{
	if (!IsWalkableCell(FromX, FromY))
	{
		return false;
	}

	int32 ToX = INDEX_NONE;
	int32 ToY = INDEX_NONE;

	if (!TryGetNeighborCell(FromX, FromY, Direction, ToX, ToY))
	{
		return false;
	}

	if (!IsWalkableCell(ToX, ToY))
	{
		return false;
	}
	if (DoorSystemComponent && DoorSystemComponent->IsDoorPassageBlocked(FromX, FromY, Direction))
	{
		return false;
	}
	// Movement follows the same directional wall convention as painting and rendering.
	const EGridWallType Wall = GetWallOnEdge(FromX, FromY, Direction);

	switch (Wall)
	{
		case EGridWallType::None:
			return true;

		case EGridWallType::Solid:
		default:
			return false;
	}
}

bool AGridLevelRuntimeActor::CanSoundTraverse(int32 FromX, int32 FromY, EGridEdge Direction) const
{
	if (!IsWalkableCell(FromX, FromY))
	{
		return false;
	}

	int32 ToX = INDEX_NONE;
	int32 ToY = INDEX_NONE;
	if (!TryGetNeighborCell(FromX, FromY, Direction, ToX, ToY) || !IsWalkableCell(ToX, ToY))
	{
		return false;
	}

	int32 DoorX = INDEX_NONE;
	int32 DoorY = INDEX_NONE;
	EGridEdge DoorEdge = EGridEdge::None;
	bool bResolvedOpposite = false;
	if (DoorSystemComponent && TryResolveDoorEdge(FromX, FromY, Direction, DoorX, DoorY, DoorEdge, bResolvedOpposite))
	{
		if (!DoorSystemComponent->IsSecretDoorOnEdge(DoorX, DoorY, DoorEdge))
		{
			return true;
		}
		return DoorSystemComponent->IsDoorFullyOpenOnEdge(DoorX, DoorY, DoorEdge);
	}

	if (GetWallOnEdge(FromX, FromY, Direction) == EGridWallType::Solid)
	{
		return false;
	}

	const EGridEdge OppositeEdge = GridDirectionUtils::GetBackward(Direction);
	return OppositeEdge != EGridEdge::None && GetWallOnEdge(ToX, ToY, OppositeEdge) != EGridWallType::Solid;
}

void AGridLevelRuntimeActor::GetEdgeTransform(int32 X, int32 Y, EGridEdge Edge, float CellSize, FVector& OutWorldLocation, FRotator& OutWorldRotation) const
{
	const FVector Base = GetActorLocation() + CellToWorld(X, Y, 0.f);

	switch (Edge)
	{
		case EGridEdge::North:
			OutWorldLocation = Base + FVector(CellSize * 0.5f, CellSize, 0.f);
			OutWorldRotation = FRotator(0.f, 0.f, 0.f);
			break;

		case EGridEdge::East:
			OutWorldLocation = Base + FVector(CellSize, CellSize * 0.5f, 0.f);
			OutWorldRotation = FRotator(0.f, -90.f, 0.f);
			break;

		case EGridEdge::South:
			OutWorldLocation = Base + FVector(CellSize * 0.5f, 0.f, 0.f);
			OutWorldRotation = FRotator(0.f, 180.f, 0.f);
			break;

		case EGridEdge::West:
			OutWorldLocation = Base + FVector(0.f, CellSize * 0.5f, 0.f);
			OutWorldRotation = FRotator(0.f, 90.f, 0.f);
			break;

		default:
			OutWorldLocation = Base;
			OutWorldRotation = FRotator::ZeroRotator;
			break;
	}
}

bool AGridLevelRuntimeActor::HasDoorOnEdge(int32 X, int32 Y, EGridEdge Edge) const
{
	int32 ResolvedX = INDEX_NONE;
	int32 ResolvedY = INDEX_NONE;
	EGridEdge ResolvedEdge = EGridEdge::None;
	bool bResolvedOpposite = false;
	return TryResolveDoorEdge(X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite);
}

bool AGridLevelRuntimeActor::TryGetOppositeEdge(int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge) const
{
	OutX = X;
	OutY = Y;
	OutEdge = EGridEdge::None;

	if (Edge == EGridEdge::None)
	{
		return false;
	}
	if (!TryGetNeighborCell(X, Y, Edge, OutX, OutY))
	{
		return false;
	}
	OutEdge = GridDirectionUtils::GetBackward(Edge);
	return OutEdge != EGridEdge::None;
}

bool AGridLevelRuntimeActor::TryResolveDoorEdge(
	int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge, bool& bOutResolvedOpposite) const
{
	OutX = X;
	OutY = Y;
	OutEdge = Edge;
	bOutResolvedOpposite = false;

	if (!DoorSystemComponent)
	{
		return false;
	}
	if (DoorSystemComponent->HasDoorOnEdge(X, Y, Edge))
	{
		return true;
	}
	if (!TryGetOppositeEdge(X, Y, Edge, OutX, OutY, OutEdge))
	{
		return false;
	}
	bOutResolvedOpposite = DoorSystemComponent->HasDoorOnEdge(OutX, OutY, OutEdge);
	return bOutResolvedOpposite;
}

bool AGridLevelRuntimeActor::IsDoorOpenOnEdge(int32 X, int32 Y, EGridEdge Edge) const
{
	int32 ResolvedX = INDEX_NONE;
	int32 ResolvedY = INDEX_NONE;
	EGridEdge ResolvedEdge = EGridEdge::None;
	bool bResolvedOpposite = false;
	return TryResolveDoorEdge(X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
		DoorSystemComponent->IsDoorOpenOnEdge(ResolvedX, ResolvedY, ResolvedEdge);
}

bool AGridLevelRuntimeActor::ToggleDoorOnEdge(int32 X, int32 Y, EGridEdge Edge)
{
	int32 ResolvedX = INDEX_NONE;
	int32 ResolvedY = INDEX_NONE;
	EGridEdge ResolvedEdge = EGridEdge::None;
	bool bResolvedOpposite = false;
	if (TryResolveDoorEdge(X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
		DoorSystemComponent->ToggleDoorOnEdge(ResolvedX, ResolvedY, ResolvedEdge))
	{
		if (bResolvedOpposite)
		{
			UE_LOG(LogTemp, Log, TEXT("Grid Use: toggled door on opposite edge (%d,%d,%d)."), ResolvedX, ResolvedY, static_cast<int32>(ResolvedEdge));
		}
		return true;
	}
	return false;
}

bool AGridLevelRuntimeActor::OpenDoorOnEdge(int32 X, int32 Y, EGridEdge Edge)
{
	int32 ResolvedX = INDEX_NONE;
	int32 ResolvedY = INDEX_NONE;
	EGridEdge ResolvedEdge = EGridEdge::None;
	bool bResolvedOpposite = false;
	if (TryResolveDoorEdge(X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
		DoorSystemComponent->OpenDoorOnEdge(ResolvedX, ResolvedY, ResolvedEdge))
	{
		if (bResolvedOpposite)
		{
			UE_LOG(LogTemp, Log, TEXT("Grid Use: opened door on opposite edge (%d,%d,%d)."), ResolvedX, ResolvedY, static_cast<int32>(ResolvedEdge));
		}
		return true;
	}
	return false;
}

bool AGridLevelRuntimeActor::CloseDoorOnEdge(int32 X, int32 Y, EGridEdge Edge)
{
	int32 ResolvedX = INDEX_NONE;
	int32 ResolvedY = INDEX_NONE;
	EGridEdge ResolvedEdge = EGridEdge::None;
	bool bResolvedOpposite = false;
	if (TryResolveDoorEdge(X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
		DoorSystemComponent->CloseDoorOnEdge(ResolvedX, ResolvedY, ResolvedEdge))
	{
		if (bResolvedOpposite)
		{
			UE_LOG(LogTemp, Log, TEXT("Grid Use: closed door on opposite edge (%d,%d,%d)."), ResolvedX, ResolvedY, static_cast<int32>(ResolvedEdge));
		}
		return true;
	}
	return false;
}

bool AGridLevelRuntimeActor::TryInteractAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn)
{
	if (!ActivationComponent || !CanPartyInteractWithEdgeObject(FromCellX, FromCellY, Edge, PartyPawn))
	{
		return false;
	}
	if (ActivationComponent->TryInteractAtEdge(FromCellX, FromCellY, Edge, PartyPawn))
	{
		return true;
	}
	int32 OppositeX = INDEX_NONE;
	int32 OppositeY = INDEX_NONE;
	EGridEdge OppositeEdge = EGridEdge::None;
	if (TryGetOppositeEdge(FromCellX, FromCellY, Edge, OppositeX, OppositeY, OppositeEdge) &&
		ActivationComponent->TryInteractAtEdge(OppositeX, OppositeY, OppositeEdge, PartyPawn))
	{
		UE_LOG(LogTemp, Log, TEXT("Grid Use: interacted with object on opposite edge (%d,%d,%d)."), OppositeX, OppositeY, static_cast<int32>(OppositeEdge));
		return true;
	}
	return false;
}

bool AGridLevelRuntimeActor::CanPartyInteractWithEdgeObject(
	int32 ObjectCellX, int32 ObjectCellY, EGridEdge ObjectEdge, const AGrimrockPartyPawn* PartyPawn) const
{
	if (!PartyPawn || PartyPawn->LevelRuntimeActor != this || ObjectEdge == EGridEdge::None || PartyPawn->Facing == EGridEdge::None)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("Grid edge interaction refused Reason=EdgeNotFacingParty PartyCell=(%d,%d) PartyFacing=%s ObjectCell=(%d,%d) ObjectEdge=%s"),
			PartyPawn ? PartyPawn->CurrentCellX : INDEX_NONE, PartyPawn ? PartyPawn->CurrentCellY : INDEX_NONE,
			*GetRuntimeEdgeText(PartyPawn ? PartyPawn->Facing : EGridEdge::None), ObjectCellX, ObjectCellY, *GetRuntimeEdgeText(ObjectEdge));
		return false;
	}

	const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
	if (FIntPoint(ObjectCellX, ObjectCellY) == PartyCell && ObjectEdge == PartyPawn->Facing)
	{
		return true;
	}

	int32 FrontCellX = PartyCell.X;
	int32 FrontCellY = PartyCell.Y;
	const bool bIsFrontOppositeEdge = TryGetNeighborCell(PartyCell.X, PartyCell.Y, PartyPawn->Facing, FrontCellX, FrontCellY) && ObjectCellX == FrontCellX &&
		ObjectCellY == FrontCellY && ObjectEdge == GridDirectionUtils::GetOpposite(PartyPawn->Facing);
	if (bIsFrontOppositeEdge)
	{
		return true;
	}

	UE_LOG(LogTemp, Verbose, TEXT("Grid edge interaction refused Reason=EdgeNotFacingParty PartyCell=(%d,%d) PartyFacing=%s ObjectCell=(%d,%d) ObjectEdge=%s"),
		PartyCell.X, PartyCell.Y, *GetRuntimeEdgeText(PartyPawn->Facing), ObjectCellX, ObjectCellY, *GetRuntimeEdgeText(ObjectEdge));
	return false;
}

AGridReceptacleActor* AGridLevelRuntimeActor::FindReceptacleAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge) const
{
	if (!ActivationComponent)
	{
		return nullptr;
	}

	if (AGridReceptacleActor* ReceptacleActor = ActivationComponent->FindReceptacleAtEdge(FromCellX, FromCellY, Edge))
	{
		return ReceptacleActor;
	}

	int32 OppositeX = INDEX_NONE;
	int32 OppositeY = INDEX_NONE;
	EGridEdge OppositeEdge = EGridEdge::None;
	if (TryGetOppositeEdge(FromCellX, FromCellY, Edge, OppositeX, OppositeY, OppositeEdge))
	{
		return ActivationComponent->FindReceptacleAtEdge(OppositeX, OppositeY, OppositeEdge);
	}

	return nullptr;
}

AGridWallLockActor* AGridLevelRuntimeActor::FindWallLockAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge) const
{
	if (!LevelAsset || Edge == EGridEdge::None)
	{
		return nullptr;
	}

	const auto FindAtExactEdge = [this](int32 CellX, int32 CellY, EGridEdge CandidateEdge) -> AGridWallLockActor*
	{
		for (const FGridWorldObjectInstance& ObjectData : LevelAsset->WorldObjectInstances)
		{
			if (ObjectData.Type == EGridLevelObjectType::Receptacle && ObjectData.CellX == CellX && ObjectData.CellY == CellY &&
				ObjectData.WallSide == CandidateEdge)
			{
				if (AGridWallLockActor* WallLockActor = FindRuntimeObjectActor<AGridWallLockActor>(ObjectData.InstanceId))
				{
					return WallLockActor;
				}
			}
		}
		return nullptr;
	};

	if (AGridWallLockActor* WallLockActor = FindAtExactEdge(FromCellX, FromCellY, Edge))
	{
		return WallLockActor;
	}

	int32 OppositeX = INDEX_NONE;
	int32 OppositeY = INDEX_NONE;
	EGridEdge OppositeEdge = EGridEdge::None;
	return TryGetOppositeEdge(FromCellX, FromCellY, Edge, OppositeX, OppositeY, OppositeEdge) ? FindAtExactEdge(OppositeX, OppositeY, OppositeEdge) : nullptr;
}

bool AGridLevelRuntimeActor::ExecuteLinksFromRuntimeObject(FGuid SourceObjectId, EGridObjectEvent SourceEvent)
{
	return ActivationComponent ? ActivationComponent->ExecuteLinksFromObjectForEvent(SourceObjectId, SourceEvent) : false;
}

void AGridLevelRuntimeActor::HandlePartyCellChanged(int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY)
{
	if (ActivationComponent)
	{
		ActivationComponent->HandlePartyCellChanged(OldCellX, OldCellY, NewCellX, NewCellY);
	}
}

void AGridLevelRuntimeActor::NotifyPawnEnteredCell(int32 CellX, int32 CellY)
{
	if (ActivationComponent)
	{
		ActivationComponent->NotifyPawnEnteredCell(CellX, CellY);
	}
}

void AGridLevelRuntimeActor::NotifyPawnExitedCell(int32 CellX, int32 CellY)
{
	if (ActivationComponent)
	{
		ActivationComponent->NotifyPawnExitedCell(CellX, CellY);
	}
}

bool AGridLevelRuntimeActor::FindTransitionAtCell(int32 CellX, int32 CellY, bool bTriggeredByUseAction, FGridObjectTransitionParams& OutTransition) const
{
	if (!LevelAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Dungeon transition lookup failed: LevelAsset is null."));
		return false;
	}

	int32 TransitionCountAtCell = 0;
	bool bFoundUsableTransition = false;

	for (const FGridWorldObjectInstance& Obj : LevelAsset->WorldObjectInstances)
	{
		const FGridObjectTransitionParams& Transition = Obj.InstanceConfig.Transition;
		if (IsEffectivePitObject(Obj) || Obj.CellX != CellX || Obj.CellY != CellY || !Transition.bIsTransition)
		{
			continue;
		}

		++TransitionCountAtCell;

		if (!bTriggeredByUseAction && Transition.bRequireUseAction)
		{
			UE_LOG(LogTemp, Log, TEXT("Dungeon transition ignored at Cell=(%d,%d): object %s requires Use action."), CellX, CellY, *Obj.InstanceId.ToString());
			continue;
		}

		if (!bFoundUsableTransition)
		{
			OutTransition = Transition;
			bFoundUsableTransition = true;
			UE_LOG(LogTemp, Log, TEXT("Dungeon transition found at Cell=(%d,%d): TargetLevelId=%s TargetCell=(%d,%d) Facing=%s."), CellX, CellY,
				*Transition.TargetLevelId.ToString(), Transition.TargetCellX, Transition.TargetCellY, *GetRuntimeEdgeText(Transition.TargetFacing));
		}
	}

	if (TransitionCountAtCell > 1)
	{
		UE_LOG(
			LogTemp, Warning, TEXT("Dungeon transition: multiple transition objects found at Cell=(%d,%d); using the first valid transition."), CellX, CellY);
	}

	return bFoundUsableTransition;
}

bool AGridLevelRuntimeActor::IsEffectivePitObject(const FGridWorldObjectInstance& ObjectData) const
{
	if (ObjectData.Type == EGridLevelObjectType::Pit)
	{
		return true;
	}

	const UGridWorldObjectDefinitionAsset* Definition = FindWorldObjectDefinition(ObjectData.WorldObjectDefinitionId);
	return Definition && Definition->SupportedType == EGridLevelObjectType::Pit;
}

bool AGridLevelRuntimeActor::ResolvePitLandingCell(
	FName TargetLevelId, int32 PreferredCellX, int32 PreferredCellY, int32& OutCellX, int32& OutCellY) const
{
	OutCellX = INDEX_NONE;
	OutCellY = INDEX_NONE;

	if (!DungeonAsset || TargetLevelId.IsNone())
	{
		return false;
	}

	const FGridDungeonLevelEntry* TargetEntry = DungeonAsset->FindLevelEntry(TargetLevelId);
	const UGridLevelAsset* TargetLevelAsset = TargetEntry && TargetEntry->bEnabled ? TargetEntry->LevelAsset.Get() : nullptr;
	if (!TargetLevelAsset)
	{
		return false;
	}

	const auto ContainsOpenPit = [this, TargetLevelId, TargetLevelAsset](int32 X, int32 Y)
	{
		return TargetLevelAsset->WorldObjectInstances.ContainsByPredicate(
			[this, TargetLevelId, X, Y](const FGridWorldObjectInstance& Candidate)
			{
				return Candidate.CellX == X && Candidate.CellY == Y && IsEffectivePitObject(Candidate) && IsPitOpenForLevel(TargetLevelId, Candidate);
			});
	};

	const auto IsWalkable = [TargetLevelAsset](int32 X, int32 Y)
	{
		if (!TargetLevelAsset->IsValidCoord(X, Y))
		{
			return false;
		}
		const FGridLevelCellData& Cell = TargetLevelAsset->GetCell(X, Y);
		return Cell.CellType != EGridCellType::Empty && !Cell.bBlocksOccupancy;
	};

	if (IsWalkable(PreferredCellX, PreferredCellY))
	{
		if (ContainsOpenPit(PreferredCellX, PreferredCellY))
		{
			return false;
		}
		OutCellX = PreferredCellX;
		OutCellY = PreferredCellY;
		return true;
	}

	int32 BestDistance = MAX_int32;
	for (int32 Y = 0; Y < TargetLevelAsset->Height; ++Y)
	{
		for (int32 X = 0; X < TargetLevelAsset->Width; ++X)
		{
			if (!IsWalkable(X, Y) || ContainsOpenPit(X, Y))
			{
				continue;
			}

			const int32 Distance = FMath::Abs(X - PreferredCellX) + FMath::Abs(Y - PreferredCellY);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				OutCellX = X;
				OutCellY = Y;
			}
		}
	}

	return OutCellX != INDEX_NONE && OutCellY != INDEX_NONE;
}

bool AGridLevelRuntimeActor::IsPitOpenForLevel(FName LevelId, const FGridWorldObjectInstance& PitObject) const
{
	if (!IsEffectivePitObject(PitObject))
	{
		return false;
	}

	if (const UGridWorldObjectDefinitionAsset* Definition = FindWorldObjectDefinition(PitObject.WorldObjectDefinitionId))
	{
		if (!Definition->HasCompletePitTrapdoorCover())
		{
			return true;
		}
	}

	if (PitObject.InstanceId.IsValid())
	{
		const FName RuntimeLevelId = DungeonAsset && !LevelId.IsNone() ? LevelId : SingleLevelRuntimeStateId;
		if (const FGridLevelRuntimeState* State = DungeonRuntimeState.LevelStates.Find(RuntimeLevelId))
		{
			if (const FGridRuntimePitState* PitState = State->Pits.Find(PitObject.InstanceId))
			{
				return PitState->bIsOpen;
			}
		}
	}

	return PitObject.InstanceConfig.Pit.bInitiallyOpen;
}

bool AGridLevelRuntimeActor::IsPitOpen(FGuid PitObjectId) const
{
	if (!LevelAsset || !PitObjectId.IsValid())
	{
		return false;
	}

	if (const AGridPitTrapdoorActor* PitActor = FindRuntimeObjectActor<AGridPitTrapdoorActor>(PitObjectId))
	{
		if (PitActor->IsAnimating())
		{
			return true;
		}
		return PitActor->IsPitOpenVisualState();
	}

	const FGridWorldObjectInstance* PitObject = LevelAsset->FindWorldObjectInstanceById(PitObjectId);
	if (PitObject && !IsEffectivePitObject(*PitObject))
	{
		return false;
	}
	return PitObject && IsPitOpenForLevel(CurrentDungeonLevelId, *PitObject);
}

bool AGridLevelRuntimeActor::SetPitOpen(FGuid PitObjectId, bool bOpen, bool bEmitEvent)
{
	if (!LevelAsset || !PitObjectId.IsValid())
	{
		return false;
	}

	const FGridWorldObjectInstance* PitObject = LevelAsset->FindWorldObjectInstanceById(PitObjectId);
	if (PitObject && !IsEffectivePitObject(*PitObject))
	{
		return false;
	}
	if (!PitObject || !PitObject->bInitiallyEnabled)
	{
		return false;
	}

	const UGridWorldObjectDefinitionAsset* PitDefinition = FindWorldObjectDefinition(PitObject->WorldObjectDefinitionId);
	if (!bOpen && PitDefinition && !PitDefinition->HasCompletePitTrapdoorCover())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("GridPit Close ignored ObjectId=%s Cell=(%d,%d): definition %s does not define both trapdoor leaves; static Pit remains Open."),
			*PitObjectId.ToString(), PitObject->CellX, PitObject->CellY, *PitObject->WorldObjectDefinitionId.ToString());
		return false;
	}

	const bool bWasGameplayOpen = IsPitOpen(PitObjectId);

	FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
	if (!State)
	{
		return false;
	}

	FGridRuntimePitState& PitState = State->Pits.FindOrAdd(PitObjectId);
	PitState.ObjectId = PitObjectId;
	PitState.bIsOpen = bOpen;

	if (AGridPitTrapdoorActor* PitActor = FindRuntimeObjectActor<AGridPitTrapdoorActor>(PitObjectId))
	{
		if (!PitActor->IsAnimating() && PitActor->IsPitOpenVisualState() == bOpen && PitActor->IsTargetOpen() == bOpen)
		{
			PendingPitEmitEvents.Remove(PitObjectId);
			return true;
		}

		if (bOpen)
		{
			PendingPitEmitEvents.Remove(PitObjectId);
			PitActor->SetPitOpenVisualState(true, true);
			FinalizePitGameplayStateChange(PitObjectId, bWasGameplayOpen, true, bEmitEvent);
			return true;
		}

		const bool bSameTarget = PitActor->IsTargetOpen() == false;
		if (bSameTarget)
		{
			bool& bPendingEmit = PendingPitEmitEvents.FindOrAdd(PitObjectId);
			bPendingEmit = bPendingEmit || bEmitEvent;
		}
		else
		{
			PendingPitEmitEvents.Add(PitObjectId, bEmitEvent);
		}

		PitActor->SetPitOpenVisualState(false, true);
		return true;
	}

	FinalizePitGameplayStateChange(PitObjectId, bWasGameplayOpen, bOpen, bEmitEvent);
	return true;
}

bool AGridLevelRuntimeActor::TogglePit(FGuid PitObjectId, bool bEmitEvent)
{
	if (const AGridPitTrapdoorActor* PitActor = FindRuntimeObjectActor<AGridPitTrapdoorActor>(PitObjectId))
	{
		return SetPitOpen(PitObjectId, !PitActor->IsTargetOpen(), bEmitEvent);
	}

	return SetPitOpen(PitObjectId, !IsPitOpen(PitObjectId), bEmitEvent);
}

void AGridLevelRuntimeActor::HandlePitTrapdoorAnimationFinished(FGuid PitObjectId, bool bWasOpen, bool bIsOpen)
{
	const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel();
	const FGridRuntimePitState* PitState = State ? State->Pits.Find(PitObjectId) : nullptr;
	if (!PitState || PitState->bIsOpen != bIsOpen)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("GridPit animation completion ignored ObjectId=%s Settled=%s Reason=target changed"),
			*PitObjectId.ToString(), bIsOpen ? TEXT("Open") : TEXT("Closed"));
		return;
	}

	if (bIsOpen)
	{
		PendingPitEmitEvents.Remove(PitObjectId);
		UE_LOG(LogTemp, Verbose, TEXT("GridPit opening visual endpoint reached ObjectId=%s; gameplay was already Open."),
			*PitObjectId.ToString());
		return;
	}

	const bool bEmitEvent = PendingPitEmitEvents.FindRef(PitObjectId);
	PendingPitEmitEvents.Remove(PitObjectId);
	FinalizePitGameplayStateChange(PitObjectId, true, false, bEmitEvent);
}

void AGridLevelRuntimeActor::FinalizePitGameplayStateChange(FGuid PitObjectId, bool bWasOpen, bool bIsOpen, bool bEmitEvent)
{
	if (bWasOpen == bIsOpen || !LevelAsset)
	{
		return;
	}

	const FGridWorldObjectInstance* PitObject = LevelAsset->FindWorldObjectInstanceById(PitObjectId);
	if (PitObject && !IsEffectivePitObject(*PitObject))
	{
		return;
	}
	if (!PitObject)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("GridPit gameplay state settled ObjectId=%s Cell=(%d,%d) Previous=%s New=%s"), *PitObjectId.ToString(),
		PitObject->CellX, PitObject->CellY, bWasOpen ? TEXT("Open") : TEXT("Closed"), bIsOpen ? TEXT("Open") : TEXT("Closed"));

	if (bIsOpen)
	{
		DropWorldItemsThroughOpenPitAtCell(PitObject->CellX, PitObject->CellY);

		if (AGrimrockPartyPawn* PartyPawn = GetWorld() ? Cast<AGrimrockPartyPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) : nullptr)
		{
			if (PartyPawn->CurrentCellX == PitObject->CellX && PartyPawn->CurrentCellY == PitObject->CellY && !PartyPawn->IsPitFalling())
			{
				TryBeginPitFallAtCell(PitObject->CellX, PitObject->CellY, PartyPawn);
			}
		}
	}

	if (bEmitEvent && ActivationComponent)
	{
		ExecuteLinksFromRuntimeObject(PitObjectId, bIsOpen ? EGridObjectEvent::Opened : EGridObjectEvent::Closed);
	}
}

bool AGridLevelRuntimeActor::FindOpenPitAtCell(int32 CellX, int32 CellY, FGridObjectTransitionParams& OutTransition) const
{
	if (!LevelAsset)
	{
		return false;
	}

	for (const FGridWorldObjectInstance& Obj : LevelAsset->WorldObjectInstances)
	{
		if (!IsEffectivePitObject(Obj) || Obj.CellX != CellX || Obj.CellY != CellY)
		{
			continue;
		}

		const bool bPitOpen = Obj.InstanceId.IsValid() ? IsPitOpen(Obj.InstanceId) : IsPitOpenForLevel(CurrentDungeonLevelId, Obj);
		if (!bPitOpen)
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("GridPit cell detected but closed Cell=(%d,%d) ObjectId=%s WorldObjectDefinitionId=%s StoredType=%d."),
				CellX, CellY, *Obj.InstanceId.ToString(), *Obj.WorldObjectDefinitionId.ToString(), static_cast<int32>(Obj.Type));
			continue;
		}

		UE_LOG(LogTemp, Log,
			TEXT("GridPit OPEN cell entered Cell=(%d,%d) ObjectId=%s WorldObjectDefinitionId=%s StoredType=%d CurrentLevel=%s."),
			CellX, CellY, *Obj.InstanceId.ToString(), *Obj.WorldObjectDefinitionId.ToString(), static_cast<int32>(Obj.Type), *CurrentDungeonLevelId.ToString());

		OutTransition = Obj.InstanceConfig.Transition;
		const bool bExplicitTargetValid = DungeonAsset && !OutTransition.TargetLevelId.IsNone() && DungeonAsset->IsValidLevelId(OutTransition.TargetLevelId);
		if (!bExplicitTargetValid && DungeonAsset)
		{
			if (const FGridDungeonLevelEntry* LowerLevel = DungeonAsset->FindLevelBelow(CurrentDungeonLevelId))
			{
				if (!OutTransition.TargetLevelId.IsNone())
				{
					UE_LOG(LogTemp, Warning,
						TEXT("Pit at Cell=(%d,%d) explicit TargetLevelId=%s is unavailable; falling to automatic lower level %s."),
						CellX, CellY, *OutTransition.TargetLevelId.ToString(), *LowerLevel->LevelId.ToString());
				}
				OutTransition.TargetLevelId = LowerLevel->LevelId;
			}
		}

		if (Obj.InstanceConfig.Pit.bUseSameCellCoordinates)
		{
			OutTransition.TargetCellX = CellX;
			OutTransition.TargetCellY = CellY;
		}
		return true;
	}

	return false;
}

bool AGridLevelRuntimeActor::TryBeginPitFallAtCell(int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn)
{
	if (!PartyPawn)
	{
		return false;
	}

	FGridObjectTransitionParams Transition;
	if (!FindOpenPitAtCell(CellX, CellY, Transition))
	{
		const bool bAnyPitAtCell = LevelAsset && LevelAsset->WorldObjectInstances.ContainsByPredicate(
			[this, CellX, CellY](const FGridWorldObjectInstance& Candidate)
			{
				return Candidate.CellX == CellX && Candidate.CellY == CellY && IsEffectivePitObject(Candidate);
			});
		if (bAnyPitAtCell)
		{
			UE_LOG(LogTemp, Warning, TEXT("GridPit fall not started at Cell=(%d,%d): Pit exists but is disabled or Closed."), CellX, CellY);
		}
		return false;
	}

	if (!DungeonAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Pit fall failed at Cell=(%d,%d): DungeonAsset is null."), CellX, CellY);
		return false;
	}

	if (Transition.TargetLevelId.IsNone())
	{
		UE_LOG(LogTemp, Error,
			TEXT("Pit fall failed at Cell=(%d,%d) on level %s: no enabled lower dungeon level could be resolved."),
			CellX, CellY, *CurrentDungeonLevelId.ToString());
		return false;
	}

	if (Transition.TargetFacing == EGridEdge::None)
	{
		Transition.TargetFacing = PartyPawn->Facing;
	}

	const FGridDungeonLevelEntry* TargetEntry = DungeonAsset->FindLevelEntry(Transition.TargetLevelId);
	if (!TargetEntry || !TargetEntry->bEnabled || !TargetEntry->LevelAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Pit fall rejected at Cell=(%d,%d): target level %s is unavailable."), CellX, CellY,
			*Transition.TargetLevelId.ToString());
		return false;
	}

	UGridLevelAsset* TargetLevelAsset = TargetEntry->LevelAsset.Get();
	const int32 PreferredTargetX = Transition.TargetCellX;
	const int32 PreferredTargetY = Transition.TargetCellY;

	const bool bPreferredWalkable = TargetLevelAsset->IsValidCoord(PreferredTargetX, PreferredTargetY) &&
		TargetLevelAsset->GetCell(PreferredTargetX, PreferredTargetY).CellType != EGridCellType::Empty &&
		!TargetLevelAsset->GetCell(PreferredTargetX, PreferredTargetY).bBlocksOccupancy;
	const bool bPreferredContainsOpenPit = bPreferredWalkable && TargetLevelAsset->WorldObjectInstances.ContainsByPredicate(
		[this, &Transition, PreferredTargetX, PreferredTargetY](const FGridWorldObjectInstance& Candidate)
		{
			return Candidate.CellX == PreferredTargetX && Candidate.CellY == PreferredTargetY && IsEffectivePitObject(Candidate) &&
				IsPitOpenForLevel(Transition.TargetLevelId, Candidate);
		});
	if (bPreferredContainsOpenPit)
	{
		UE_LOG(LogTemp, Error, TEXT("Pit fall rejected: destination (%d,%d) on level %s contains another open pit."),
			PreferredTargetX, PreferredTargetY, *Transition.TargetLevelId.ToString());
		return false;
	}

	int32 LandingCellX = INDEX_NONE;
	int32 LandingCellY = INDEX_NONE;
	if (!ResolvePitLandingCell(Transition.TargetLevelId, PreferredTargetX, PreferredTargetY, LandingCellX, LandingCellY))
	{
		UE_LOG(LogTemp, Error,
			TEXT("Pit fall failed at Cell=(%d,%d): target level %s contains no usable landing cell near requested (%d,%d)."),
			CellX, CellY, *Transition.TargetLevelId.ToString(), PreferredTargetX, PreferredTargetY);
		return false;
	}

	if (LandingCellX != PreferredTargetX || LandingCellY != PreferredTargetY)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("GridPit landing fallback Source=(%d,%d) TargetLevel=%s Requested=(%d,%d) Resolved=(%d,%d)."),
			CellX, CellY, *Transition.TargetLevelId.ToString(), PreferredTargetX, PreferredTargetY, LandingCellX, LandingCellY);
		Transition.TargetCellX = LandingCellX;
		Transition.TargetCellY = LandingCellY;
	}

	if (!PartyPawn->BeginPitFall(Transition))
	{
		return false;
	}

	AbortActiveCombatAndMonsterActions();
	return true;
}

bool AGridLevelRuntimeActor::TryExecuteTransitionAtCell(int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn, bool bTriggeredByUseAction)
{
	FGridObjectTransitionParams Transition;
	if (!FindTransitionAtCell(CellX, CellY, bTriggeredByUseAction, Transition))
	{
		return false;
	}

	return TravelToDungeonLevel(Transition.TargetLevelId, Transition.TargetCellX, Transition.TargetCellY, Transition.TargetFacing, PartyPawn);
}

bool AGridLevelRuntimeActor::TravelToDungeonLevel(
	FName TargetLevelId, int32 TargetCellX, int32 TargetCellY, EGridEdge TargetFacing, AGrimrockPartyPawn* PartyPawn)
{
	if (bIsExecutingDungeonTransition)
	{
		UE_LOG(LogTemp, Warning, TEXT("Dungeon transition ignored: another transition is already executing."));
		return false;
	}

	struct FScopedDungeonTransitionGuard
	{
		bool& bGuard;

		explicit FScopedDungeonTransitionGuard(bool& InGuard)
			: bGuard(InGuard)
		{
			bGuard = true;
		}

		~FScopedDungeonTransitionGuard()
		{
			bGuard = false;
		}
	};

	FScopedDungeonTransitionGuard TransitionGuard(bIsExecutingDungeonTransition);

	if (!DungeonAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Dungeon transition failed: DungeonAsset is null."));
		return false;
	}

	if (TargetLevelId.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("Dungeon transition failed: TargetLevelId is None."));
		return false;
	}

	const FGridDungeonLevelEntry* TargetEntry = DungeonAsset->FindLevelEntry(TargetLevelId);
	if (!TargetEntry)
	{
		UE_LOG(LogTemp, Error, TEXT("Dungeon transition failed: TargetLevelId %s was not found in DungeonAsset %s."), *TargetLevelId.ToString(),
			*DungeonAsset->GetPathName());
		return false;
	}

	if (!TargetEntry->bEnabled)
	{
		UE_LOG(LogTemp, Error, TEXT("Dungeon transition failed: TargetLevelId %s is disabled."), *TargetLevelId.ToString());
		return false;
	}

	UGridLevelAsset* TargetLevelAsset = TargetEntry->LevelAsset.Get();
	if (!TargetLevelAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Dungeon transition failed: TargetLevelId %s has no LevelAsset."), *TargetLevelId.ToString());
		return false;
	}

	if (!TargetLevelAsset->IsValidCoord(TargetCellX, TargetCellY))
	{
		UE_LOG(LogTemp, Error, TEXT("Dungeon transition failed: Target cell (%d,%d) is outside LevelAsset %s."), TargetCellX, TargetCellY,
			*TargetLevelAsset->GetPathName());
		return false;
	}

	const FGridLevelCellData& TargetCell = TargetLevelAsset->GetCell(TargetCellX, TargetCellY);
	if (TargetCell.CellType == EGridCellType::Empty || TargetCell.bBlocksOccupancy)
	{
		UE_LOG(LogTemp, Error, TEXT("Dungeon transition failed: Target cell (%d,%d) is not walkable in LevelAsset %s. CellType=%d BlocksOccupancy=%s."),
			TargetCellX, TargetCellY, *TargetLevelAsset->GetPathName(), static_cast<int32>(TargetCell.CellType), *GetRuntimeBoolText(TargetCell.bBlocksOccupancy));
		return false;
	}

	if (TargetFacing == EGridEdge::None)
	{
		UE_LOG(LogTemp, Error, TEXT("Dungeon transition failed: TargetFacing is None."));
		return false;
	}

	if (!PartyPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("Dungeon transition failed: PartyPawn is null."));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Dungeon transition: %s -> %s, Cell=(%d,%d), Facing=%s."), *CurrentDungeonLevelId.ToString(), *TargetLevelId.ToString(),
		TargetCellX, TargetCellY, *GetRuntimeEdgeText(TargetFacing));

	AbortActiveCombatAndMonsterActions();

	const FName OldLevelId = ResolveRuntimeStateLevelId(DungeonAsset, CurrentDungeonLevelId);
	CaptureCurrentLevelRuntimeState();
	if (const FGridLevelRuntimeState* StoredState = DungeonRuntimeState.LevelStates.Find(OldLevelId))
	{
		UE_LOG(LogTemp, Log, TEXT("GridRuntimeState Stored Level=%s Receptacles=%d Items=%d Doors=%d Monsters=%d"), *OldLevelId.ToString(),
			StoredState->Receptacles.Num(), StoredState->Items.Num(), StoredState->Doors.Num(), StoredState->Monsters.Num());
	}

	TArray<AGridMonsterActor*> Monsters;
	GetWorldMonsters(GetWorld(), Monsters);
	for (AGridMonsterActor* Monster : Monsters)
	{
		if (Monster->ResolveRuntimeDungeonLevelId(OldLevelId) == OldLevelId)
		{
			SetMonsterRuntimeLevelActive(Monster, false);
		}
	}

	CurrentDungeonLevelId = TargetLevelId;
	LevelAsset = TargetLevelAsset;

	RebuildLevel();
	ApplyCurrentLevelRuntimeState();
	PartyPawn->SetGridStart(this, TargetCellX, TargetCellY, TargetFacing);
	if (ActivationComponent)
	{
		ActivationComponent->RefreshAllPressurePlates();
	}

	UE_LOG(LogTemp, Log, TEXT("Dungeon transition complete: CurrentDungeonLevelId=%s LevelAsset=%s PartyCell=(%d,%d) Facing=%s."),
		*CurrentDungeonLevelId.ToString(), LevelAsset ? *LevelAsset->GetPathName() : TEXT("None"), TargetCellX, TargetCellY, *GetRuntimeEdgeText(TargetFacing));
	return true;
}

void AGridLevelRuntimeActor::SetEditorHoveredObject(FGuid ObjectId)
{
	if (EditorPreviewComponent)
	{
		EditorPreviewComponent->SetHoveredObject(ObjectId);
	}
}

void AGridLevelRuntimeActor::SetEditorSelectedObject(FGuid ObjectId)
{
	if (EditorPreviewComponent)
	{
		EditorPreviewComponent->SetSelectedObject(ObjectId);
	}
}

void AGridLevelRuntimeActor::CleanupOrphanEditorPreviewObjects()
{
	if (EditorPreviewComponent)
	{
		EditorPreviewComponent->CleanupOrphanPreviewObjects();
	}
}

const UGridWorldObjectDefinitionAsset* AGridLevelRuntimeActor::FindWorldObjectDefinition(FName WorldObjectDefinitionId) const
{
	if (WorldObjectDefinitionId.IsNone())
	{
		return nullptr;
	}
	for (const UGridWorldObjectDefinitionAsset* Definition : WorldObjectDefinitions)
	{
		if (!Definition)
		{
			continue;
		}
		if (Definition->DefinitionId == WorldObjectDefinitionId)
		{
			return Definition;
		}
	}
	return nullptr;
}

UGridItemDefinitionAsset* AGridLevelRuntimeActor::ResolveRuntimeItemDefinition(FName ItemDefinitionId) const
{
	if (ItemDefinitionId.IsNone())
	{
		return nullptr;
	}

	if (LevelAsset)
	{
		for (const FGridLooseItemInstance& Item : LevelAsset->LooseItemInstances)
		{
			if (Item.ItemDefinition && Item.ItemDefinition->ItemDefinitionId == ItemDefinitionId)
			{
				return Item.ItemDefinition;
			}
		}
		for (const FGridItemSpawnInstance& Spawn : LevelAsset->ItemSpawns)
		{
			if (Spawn.ItemDefinition && Spawn.ItemDefinition->ItemDefinitionId == ItemDefinitionId)
			{
				return Spawn.ItemDefinition;
			}
		}
		for (const FGridWorldObjectInstance& ObjectData : LevelAsset->WorldObjectInstances)
		{
			for (const FGridReceptacleInitialItemConfig& InitialItem : ObjectData.InstanceConfig.ReceptacleInitialContent)
			{
				if (InitialItem.ItemDefinition && InitialItem.ItemDefinition->ItemDefinitionId == ItemDefinitionId)
				{
					return InitialItem.ItemDefinition;
				}
			}
		}
	}

	for (const UGridWorldObjectDefinitionAsset* Definition : WorldObjectDefinitions)
	{
		if (!Definition)
		{
			continue;
		}

		const FGridReceptacleBehaviorParams& ReceptacleParams = Definition->DefaultBehavior.Receptacle;
		for (const FGridReceptacleInitialItemConfig& InitialItem : ReceptacleParams.InitialContent)
		{
			if (InitialItem.ItemDefinition && InitialItem.ItemDefinition->ItemDefinitionId == ItemDefinitionId)
			{
				return InitialItem.ItemDefinition;
			}
		}
	}

	return nullptr;
}

AGridItemActor* AGridLevelRuntimeActor::SpawnItemActorForDefinition(UGridItemDefinitionAsset* ItemDefinition, FName ItemDefinitionId, AActor* OwnerActor,
	USceneComponent* AttachParent, TSubclassOf<AGridItemActor> PreferredItemActorClass) const
{
	ItemDefinitionId = ItemDefinition && !ItemDefinition->ItemDefinitionId.IsNone() ? ItemDefinition->ItemDefinitionId : ItemDefinitionId;
	if (!ItemDefinition && ItemDefinitionId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("Grid item spawn failed: missing ItemDefinition and ItemDefinitionId."));
		return nullptr;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	TSubclassOf<AGridItemActor> ItemClass = PreferredItemActorClass;
	if (!ItemClass)
	{
		ItemClass = AGridItemActor::StaticClass();
	}
	const FTransform SpawnTransform(AttachParent ? AttachParent->GetComponentRotation() : FRotator::ZeroRotator,
		AttachParent ? AttachParent->GetComponentLocation() : GetActorLocation(), FVector::OneVector);
	if (!IsSafeRuntimeRenderTransform(SpawnTransform))
	{
		UE_LOG(LogTemp, Warning, TEXT("Grid item spawn failed: unsafe transform Item=%s Owner=%s AttachParent=%s."), *ItemDefinitionId.ToString(),
			OwnerActor ? *OwnerActor->GetName() : TEXT("None"), AttachParent ? *AttachParent->GetName() : TEXT("None"));
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.Owner = OwnerActor ? OwnerActor : const_cast<AGridLevelRuntimeActor*>(this);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGridItemActor* ItemActor = World->SpawnActor<AGridItemActor>(ItemClass, SpawnTransform.GetLocation(), SpawnTransform.GetRotation().Rotator(), Params);

	if (!ItemActor)
	{
		return nullptr;
	}
	if (ItemDefinition)
	{
		ItemActor->InitializeFromItemDefinition(ItemDefinition, FGuid());
	}
	else
	{
		ItemActor->InitializeFromItemDefinitionId(ItemDefinitionId, FGuid());
	}
	if (AttachParent)
	{
		ItemActor->ConfigureAsAttachedItem();
		ItemActor->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		ItemActor->SetActorRelativeTransform(FTransform::Identity);
	}
	return ItemActor;
}


void AGridLevelRuntimeActor::RegisterRuntimeObjectActor(const FGuid& ObjectId, AGridRuntimeObjectActor* Actor)
{
	if (!ObjectId.IsValid() || !IsValid(Actor))
	{
		return;
	}
	SpawnedRuntimeObjectActors.Add(ObjectId, Actor);
}

void AGridLevelRuntimeActor::ClearRuntimeObjectActors()
{
	ClearSpawnedMonsterActors();
	for (AGridItemActor* ItemActor : SpawnedItemActors)
	{
		if (IsValid(ItemActor))
		{
			ItemActor->OnRemovedFromWorld();
			ItemActor->Destroy();
		}
	}
	SpawnedItemActors.Reset();
	SpawnedItemEntries.Reset();

	for (TPair<FGuid, TObjectPtr<AGridRuntimeObjectActor>>& Pair : SpawnedRuntimeObjectActors)
	{
		if (IsValid(Pair.Value))
		{
			if (AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor>(Pair.Value.Get()))
			{
				ReceptacleActor->ForceClearRuntimeContents(false);
			}
			Pair.Value->Destroy();
		}
	}
	SpawnedRuntimeObjectActors.Empty();
	PendingPitEmitEvents.Reset();
}

bool AGridLevelRuntimeActor::IsPartyOnCell(int32 CellX, int32 CellY) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	for (TActorIterator<AGrimrockPartyPawn> It(World); It; ++It)
	{
		const AGrimrockPartyPawn* PartyPawn = *It;
		if (PartyPawn && PartyPawn->LevelRuntimeActor == this && PartyPawn->CurrentCellX == CellX && PartyPawn->CurrentCellY == CellY)
		{
			return true;
		}
	}
	return false;
}

TSubclassOf<AGridRuntimeObjectActor> AGridLevelRuntimeActor::GetObjectRuntimeActorClass(const FGridWorldObjectInstance& ObjectData) const
{
	const UGridWorldObjectDefinitionAsset* Definition = FindWorldObjectDefinition(ObjectData.WorldObjectDefinitionId);
	return Definition ? Definition->RuntimeActorClass : nullptr;
}

bool AGridLevelRuntimeActor::IsRuntimeSpawnableObject(const FGridWorldObjectInstance& Instance) const
{
	if (!LevelAsset || !Instance.bInitiallyEnabled || !LevelAsset->IsValidCoord(Instance.CellX, Instance.CellY))
	{
		return false;
	}
	const UGridWorldObjectDefinitionAsset* Definition = FindWorldObjectDefinition(Instance.WorldObjectDefinitionId);
	return Definition && Definition->RuntimeActorClass && (Definition->PlacementSurface != EGridObjectPlacementKind::Wall || Instance.WallSide != EGridEdge::None);
}

void AGridLevelRuntimeActor::AddPlacedItemActor(const FGridLooseItemInstance& ObjectData)
{
	FTransform Transform;
	if (!GridPlacementTransformResolver::ResolveLooseItem(*this, ObjectData, Transform))
	{
		UE_LOG(LogTemp, Warning, TEXT("Placed item skipped: could not compute placement transform for object %s."), *ObjectData.InstanceId.ToString());
		return;
	}
	if (!IsSafeRuntimeRenderTransform(Transform))
	{
		LogUnsafeItemTransform(TEXT("AddPlacedItemActor"), ObjectData.ItemDefinition ? ObjectData.ItemDefinition->ItemDefinitionId : NAME_None, this, nullptr, nullptr, Transform);
		return;
	}

	UGridItemDefinitionAsset* ItemDefinition = ObjectData.ItemDefinition.Get();
	if (!IsValid(ItemDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("Placed item skipped: definition missing for instance %s."), *ObjectData.InstanceId.ToString());
		return;
	}
	const FName ItemDefinitionId = ItemDefinition->ItemDefinitionId;
	AGridItemActor* ItemActor = SpawnItemActorForDefinition(ItemDefinition, ItemDefinitionId, this, nullptr);
	if (!ItemActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Placed item skipped: failed to spawn item definition %s."), *ItemDefinitionId.ToString());
		return;
	}

	ItemActor->SetActorTransform(Transform);
	ItemActor->SetRuntimeObjectId(ObjectData.InstanceId);
	ItemActor->InitializeFromItemDefinition(ItemDefinition, ObjectData.InstanceId);
	UGridReadableContentAsset* ReadableContentAsset = ObjectData.ReadableContentAsset;
	FName ReadableContentId = ObjectData.ReadableContentId;
	FText ReadTitleOverride = ObjectData.ReadTitleOverride;
	FText ReadTextOverride = ObjectData.ReadTextOverride;

	ItemActor->InitializeReadableContent(ReadableContentAsset, ReadableContentId, ReadTitleOverride, ReadTextOverride);
	ItemActor->SetRuntimeCell(ObjectData.CellX, ObjectData.CellY);
	ItemActor->ApplyWorldPhysicsInitialNudge();
	ItemActor->ConfigureAsWorldPickup();
	ItemActor->OnRemovedFromWorld();
	SpawnedItemActors.Add(ItemActor);

	FGridSpawnedItemRuntimeEntry Entry;
	Entry.Cell = FIntPoint(ObjectData.CellX, ObjectData.CellY);
	Entry.Edge = ObjectData.SurfaceSide;
	Entry.ItemActor = ItemActor;
	Entry.ObjectId = ObjectData.InstanceId;
	Entry.ItemDefinitionAsset = ItemDefinition;
	Entry.ItemDefinitionId = ItemDefinitionId;
	Entry.Quantity = FMath::Max(1, ObjectData.Quantity);
	SpawnedItemEntries.Add(Entry);
	UE_LOG(LogTemp, Log, TEXT("Placed item spawned: %s at object %s. Runtime=%s RebuildGeneration=%d ActiveItemCount=%d"), *ItemDefinitionId.ToString(),
		*ObjectData.InstanceId.ToString(), *GetName(), RuntimeObjectRebuildGeneration, SpawnedItemEntries.Num());
}

void AGridLevelRuntimeActor::AddRuntimeObjectActor(const FGridWorldObjectInstance& ObjectData)
{
	UStaticMesh* Mesh = nullptr;
	FTransform Transform;
	const TSubclassOf<AGridRuntimeObjectActor> RuntimeActorClass = GetObjectRuntimeActorClass(ObjectData);
	AGridRuntimeObjectActor* Actor = SpawnRuntimeObjectActor<AGridRuntimeObjectActor>(ObjectData, Mesh, Transform);
	UE_LOG(LogTemp, VeryVerbose,
		TEXT("GridRuntime Diagnostic AddRuntimeObjectActor ObjectId=%s WorldObjectDefinitionId=%s ObjectData.Type=%s RuntimeActorClass=%s ActorClass=%s Mesh=%s Transform=%s"),
		*ObjectData.InstanceId.ToString(), *ObjectData.WorldObjectDefinitionId.ToString(), *UEnum::GetValueAsString(ObjectData.Type),
		RuntimeActorClass ? *RuntimeActorClass->GetPathName() : TEXT("None"), Actor ? *Actor->GetClass()->GetPathName() : TEXT("None"),
		Mesh ? *Mesh->GetPathName() : TEXT("None"), *Transform.ToHumanReadableString());
	if (!Actor)
	{
		return;
	}
	const FGridRuntimeWorldObjectData RuntimeObjectData(ObjectData);
	const UGridWorldObjectDefinitionAsset* Definition = FindWorldObjectDefinition(ObjectData.WorldObjectDefinitionId);
	if (AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor>(Actor))
	{
		ReceptacleActor->ContainedItemActorClass = Definition ? Definition->ItemActorClass : nullptr;
	}
	if (AGridMechanismActor* MechanismActor = Cast<AGridMechanismActor>(Actor))
	{
		MechanismActor->InitializeRuntimeMechanismVisuals(RuntimeObjectData, Definition, Transform);
		Actor->InitializeRuntimeWorldObject(RuntimeObjectData, Mesh, Transform);
	}
	else if (AGridGenericObjectActor* GenericActor = Cast<AGridGenericObjectActor>(Actor))
	{
		GenericActor->InitializeRuntimeGenericObject(RuntimeObjectData, Definition, Mesh, Transform);
	}
	else
	{
		Actor->InitializeRuntimeWorldObject(RuntimeObjectData, Mesh, Transform);
	}
	Actor->ConfigureObjectAudio(Definition);

	if (AGridPitTrapdoorActor* PitActor = Cast<AGridPitTrapdoorActor>(Actor))
	{
		PitActor->OnPitAnimationFinished.AddUObject(this, &AGridLevelRuntimeActor::HandlePitTrapdoorAnimationFinished);
		PitActor->SnapPitOpenState(IsPitOpenForLevel(CurrentDungeonLevelId, ObjectData));
	}
	if (ActivationComponent)
	{
		ActivationComponent->RegisterInitialObjectState(ObjectData);
	}
	if (ObjectData.Type == EGridLevelObjectType::Door && DoorSystemComponent)
	{
		DoorSystemComponent->RegisterDoorObject(RuntimeObjectData, Actor);
	}
}

void AGridLevelRuntimeActor::RebuildRuntimeObjects()
{
	if (!LevelAsset)
	{
		return;
	}

	LevelAsset->EnsureCellCount();
	RuntimeMonsterSpawnFailureCount = 0;
	const FGridLevelRuntimeState* SavedLevelState = FindRuntimeStateForCurrentLevel();

	for (const FGridWorldObjectInstance& Instance : LevelAsset->WorldObjectInstances)
	{
		if (!IsRuntimeSpawnableObject(Instance))
		{
			if (Instance.bInitiallyEnabled)
			{
				const UGridWorldObjectDefinitionAsset* Definition = FindWorldObjectDefinition(Instance.WorldObjectDefinitionId);
				if (Definition && !Definition->RuntimeActorClass)
				{
					UE_LOG(LogTemp, Warning, TEXT("Runtime object skipped: definition %s has no RuntimeActorClass."), *Instance.WorldObjectDefinitionId.ToString());
				}
			}
			continue;
		}
		AddRuntimeObjectActor(Instance);
	}

	for (const FGridLooseItemInstance& Instance : LevelAsset->LooseItemInstances)
	{
		if (!Instance.bInitiallyEnabled)
		{
			continue;
		}

		AddPlacedItemActor(Instance);
	}

	for (const FGridMonsterSpawnInstance& Spawn : LevelAsset->MonsterSpawns)
	{
		const FGridRuntimeMonsterPlacementState* PlacementState = SavedLevelState ? SavedLevelState->MonsterPlacements.Find(Spawn.SpawnId) : nullptr;
		const bool bShouldSpawn = PlacementState ? PlacementState->bIsSpawned : Spawn.bInitiallyEnabled;
		const FGridRuntimeMonsterState* RestoreState = PlacementState && PlacementState->bHasMonsterState ? &PlacementState->MonsterState : nullptr;
		if (bShouldSpawn && !AddMonsterSpawnActor(Spawn, RestoreState))
		{
			++RuntimeMonsterSpawnFailureCount;
		}
	}
}
