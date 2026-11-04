#include "Runtime/GridLevelRuntimeActor.h"

#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatSurfaceResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridSurfaceRuntime, Log, All);

namespace
{
	constexpr float OilSplashDurationSeconds = 0.42f;
	constexpr float OilSplashUpdateIntervalSeconds = 1.f / 30.f;
	const FName OilSplashOpacityParameter(TEXT("OilOpacity"));
	constexpr float OilExplorationTimerInterval = 0.25f;
}

void AGridLevelRuntimeActor::ClearExplorationOilTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OilExplorationTimer);
	}
	LastOilExplorationUpdateTime = 0.0;
}

void AGridLevelRuntimeActor::RefreshExplorationOilTimer()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	bool bHasOil = false;
	if (LevelAsset)
	{
		if (const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel())
		{
			for (const TPair<FIntPoint, FGridCombatSurfaceState>& Pair : State->Surfaces)
			{
				if (Pair.Value.IsValid() && Pair.Value.SurfaceType == EGridCombatSurfaceType::Oil)
				{
					bHasOil = true;
					break;
				}
			}
		}
	}
	if (!bHasOil)
	{
		ClearExplorationOilTimer();
		return;
	}
	if (!World->GetTimerManager().IsTimerActive(OilExplorationTimer))
	{
		LastOilExplorationUpdateTime = World->GetTimeSeconds();
		World->GetTimerManager().SetTimer(OilExplorationTimer, this,
			&AGridLevelRuntimeActor::UpdateExplorationOilTimer,
			OilExplorationTimerInterval, true);
	}
}

void AGridLevelRuntimeActor::UpdateExplorationOilTimer()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		ClearExplorationOilTimer();
		return;
	}
	const double Now = World->GetTimeSeconds();
	const float DeltaSeconds = static_cast<float>(FMath::Max(0.0, Now - LastOilExplorationUpdateTime));
	LastOilExplorationUpdateTime = Now;
	// Un combat actif utilise exclusivement le décompte de fin de round.
	if (const UGridTurnManagerComponent* Turns = FindComponentByClass<UGridTurnManagerComponent>())
	{
		if (Turns->bCombatActive)
		{
			return;
		}
	}
	AdvanceExplorationOilTime(DeltaSeconds);
}

int32 AGridLevelRuntimeActor::AdvanceExplorationOilTime(
	float ElapsedSeconds, TArray<FIntPoint>* OutExpiredCells)
{
	if (OutExpiredCells)
	{
		OutExpiredCells->Reset();
	}
	if (!FMath::IsFinite(ElapsedSeconds) || ElapsedSeconds <= 0.f)
	{
		return 0;
	}
	if (const UGridTurnManagerComponent* Turns = FindComponentByClass<UGridTurnManagerComponent>())
	{
		if (Turns->bCombatActive)
		{
			return 0;
		}
	}
	FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
	if (!LevelAsset || !State)
	{
		return 0;
	}
	const float SecondsPerRound = FMath::Max(0.1f, OilExplorationSecondsPerRound);
	TArray<FIntPoint> Cells;
	State->Surfaces.GetKeys(Cells);
	Cells.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return A.Y == B.Y ? A.X < B.X : A.Y < B.Y;
	});
	int32 Expired = 0;
	for (const FIntPoint& Cell : Cells)
	{
		FGridCombatSurfaceState* Surface = State->Surfaces.Find(Cell);
		if (!Surface || !Surface->IsValid() || Surface->SurfaceType != EGridCombatSurfaceType::Oil)
		{
			continue;
		}
		const float Before = Surface->RemainingOilSeconds > 0.f
			? Surface->RemainingOilSeconds
			: Surface->RemainingRounds * SecondsPerRound;
		const float Remaining = FMath::Max(0.f, Before - ElapsedSeconds);
		if (Remaining <= KINDA_SMALL_NUMBER)
		{
			State->Surfaces.Remove(Cell);
			if (OutExpiredCells)
			{
				OutExpiredCells->Add(Cell);
			}
			++Expired;
			continue;
		}
		Surface->RemainingOilSeconds = Remaining;
		Surface->RemainingRounds = FMath::Clamp(FMath::CeilToInt(Remaining / SecondsPerRound), 1, 6);
	}
	if (Expired > 0)
	{
		RefreshCombatOilSurfaceVisuals();
	}
	return Expired;
}

float AGridLevelRuntimeActor::GetOilSplashSpreadScale(float NormalizedProgress)
{
	const float Alpha = FMath::Clamp(NormalizedProgress, 0.f, 1.f);
	const float EaseOut = 1.f - FMath::Pow(1.f - Alpha, 3.f);
	return FMath::Lerp(0.18f, 1.f, EaseOut);
}

void AGridLevelRuntimeActor::ClearCombatOilSplash()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OilSplashTimer);
	}
	if (IsValid(ActiveOilSplashDecal))
	{
		ActiveOilSplashDecal->DestroyComponent();
	}
	ActiveOilSplashDecal = nullptr;
	ActiveOilSplashMaterial = nullptr;
	OilSplashElapsedSeconds = 0.f;
}

void AGridLevelRuntimeActor::UpdateCombatOilSplash()
{
	if (!IsValid(ActiveOilSplashDecal) || !IsValid(ActiveOilSplashMaterial))
	{
		ClearCombatOilSplash();
		return;
	}
	OilSplashElapsedSeconds += OilSplashUpdateIntervalSeconds;
	const float Alpha = FMath::Clamp(OilSplashElapsedSeconds / OilSplashDurationSeconds, 0.f, 1.f);
	if (Alpha >= 1.f)
	{
		ClearCombatOilSplash();
		return;
	}
	// Expansion brève sur les pierres, avec atténuation progressive ; aucune
	// géométrie superposée et aucune modification du décalque persistant.
	const float Spread = GetOilSplashSpreadScale(Alpha);
	ActiveOilSplashDecal->SetRelativeScale3D(FVector(1.f, Spread, Spread));
	ActiveOilSplashMaterial->SetScalarParameterValue(
		OilSplashOpacityParameter, 0.52f * FMath::Square(1.f - Alpha));
}

void AGridLevelRuntimeActor::PlayCombatOilSplashAtCell(int32 CellX, int32 CellY)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !LevelAsset ||
		!IsValidCell(CellX, CellY) || ShouldHideCellFloor(CellX, CellY))
	{
		return;
	}
	UMaterialInterface* Material = OilSurfaceMaterial.LoadSynchronous();
	if (!IsValid(Material))
	{
		return; // Le VFX n'a jamais le droit d'invalider l'action de combat.
	}
	ClearCombatOilSplash();
	UDecalComponent* Splash = NewObject<UDecalComponent>(this);
	if (!IsValid(Splash))
	{
		return;
	}
	Splash->SetupAttachment(SceneRoot);
	Splash->SetMobility(EComponentMobility::Movable);
	const float CellSize = LevelAsset->CellSize;
	Splash->DecalSize = FVector(48.f, CellSize * 1.55f, CellSize * 1.55f);
	Splash->SetRelativeLocation(CellToWorld(CellX, CellY, 0.f) +
		FVector(CellSize * .5f, CellSize * .5f, 0.f));
	Splash->SetRelativeRotation(FQuat::FindBetweenNormals(
		-FVector::ForwardVector, -FVector::UpVector));
	Splash->SetDecalMaterial(Material);
	Splash->SetFadeScreenSize(0.f);
	Splash->SetSortOrder(2);
	Splash->RegisterComponent();

	UMaterialInstanceDynamic* Dynamic = Splash->CreateDynamicMaterialInstance();
	if (!IsValid(Dynamic))
	{
		Splash->DestroyComponent();
		return;
	}
	ActiveOilSplashDecal = Splash;
	ActiveOilSplashMaterial = Dynamic;
	OilSplashElapsedSeconds = 0.f;
	Splash->SetRelativeScale3D(FVector(1.f, GetOilSplashSpreadScale(0.f), GetOilSplashSpreadScale(0.f)));
	Dynamic->SetScalarParameterValue(OilSplashOpacityParameter, 0.52f);
	++OilSplashPresentationCount;
	World->GetTimerManager().SetTimer(
		OilSplashTimer, this, &AGridLevelRuntimeActor::UpdateCombatOilSplash,
		OilSplashUpdateIntervalSeconds, true);
}


void AGridLevelRuntimeActor::ClearCombatOilSurfaceVisuals()
{
	for (UDecalComponent* Decal : ActiveOilDecals)
	{
		if (IsValid(Decal))
		{
			Decal->DestroyComponent();
		}
	}
	ActiveOilDecals.Reset();
}

void AGridLevelRuntimeActor::RefreshCombatOilSurfaceVisuals()
{
	// Le vieillissement dépend de l'état, non du matériau de décalque.
	RefreshExplorationOilTimer();
	// Recréer uniquement la présentation depuis RPG03.6, sans modifier l'état de combat.
	ClearCombatOilSurfaceVisuals();
	if (!LevelAsset || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}

	const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel();
	if (!State)
	{
		return;
	}
	TSet<FIntPoint> Pending;
	for (const TPair<FIntPoint, FGridCombatSurfaceState>& Entry : State->Surfaces)
	{
		const FIntPoint Cell = Entry.Key;
		if (Entry.Value.IsValid() && Entry.Value.SurfaceType == EGridCombatSurfaceType::Oil &&
			IsWalkableCell(Cell.X, Cell.Y) && !ShouldHideCellFloor(Cell.X, Cell.Y))
		{
			Pending.Add(Cell);
		}
	}
	if (Pending.IsEmpty())
	{
		return;
	}

	UMaterialInterface* Material = OilSurfaceMaterial.LoadSynchronous();
	if (!IsValid(Material))
	{
		UE_LOG(LogGridSurfaceRuntime, Warning,
			TEXT("[ARMEMENT01.17.1.4] Matériau de décalque d'huile introuvable."));
		return;
	}

	const float Size = LevelAsset->CellSize;
	// La flaque est une projection dans un volume traversant Z=0, pas un
	// plan à Z+2 ou au sommet supposé de la bounding box du mesh.
	const FQuat ProjectionRotation = FQuat::FindBetweenNormals(
		-FVector::ForwardVector, -FVector::UpVector);
	while (!Pending.IsEmpty())
	{
		// Partition déterministe en rectangles ne contenant QUE des cases d'huile.
		FIntPoint First = *Pending.CreateConstIterator();
		for (const FIntPoint& Cell : Pending)
		{
			if (Cell.Y < First.Y || (Cell.Y == First.Y && Cell.X < First.X))
			{
				First = Cell;
			}
		}
		int32 Width = 1;
		while (Pending.Contains(FIntPoint(First.X + Width, First.Y)))
		{
			++Width;
		}
		int32 Height = 1;
		bool bCanExtend = true;
		while (bCanExtend)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				if (!Pending.Contains(FIntPoint(First.X + X, First.Y + Height)))
				{
					bCanExtend = false;
					break;
				}
			}
			if (bCanExtend)
			{
				++Height;
			}
		}
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				Pending.Remove(FIntPoint(First.X + X, First.Y + Y));
			}
		}

		UDecalComponent* Decal = NewObject<UDecalComponent>(this);
		if (!IsValid(Decal))
		{
			continue;
		}
		Decal->SetupAttachment(SceneRoot);
		Decal->SetMobility(EComponentMobility::Movable);
		// Axe local X = profondeur. Rotation vers le bas ; XY suit le carrelage.
		Decal->DecalSize = FVector(48.f, Height * Size * 1.08f, Width * Size * 1.08f);
		Decal->SetRelativeLocation(
			CellToWorld(First.X, First.Y, 0.f) +
			FVector(Width * Size * 0.5f, Height * Size * 0.5f, 0.f));
		Decal->SetRelativeRotation(ProjectionRotation);
		Decal->SetDecalMaterial(Material);
		Decal->SetFadeScreenSize(0.f);
		Decal->SetSortOrder(1);
		Decal->RegisterComponent();
		ActiveOilDecals.Add(Decal);
	}
}

bool AGridLevelRuntimeActor::ApplyCombatSurfaceAtCell(int32 CellX, int32 CellY, const FGridCombatSurfaceEffectProfile& Profile,
	const FGuid& SourceCombatantId, FName SourceActionId, const FGridResolvedCombatModifiers& SourceModifiers)
{
	if (!IsValidCell(CellX, CellY) || !IsWalkableCell(CellX, CellY))
	{
		return false;
	}

	FGridCombatSurfaceState State;
	if (!FGridCombatSurfaceResolver::BuildState(Profile, SourceCombatantId, SourceActionId, SourceModifiers, State))
	{
		return false;
	}

	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		return false;
	}
	if (State.SurfaceType == EGridCombatSurfaceType::Oil)
	{
		State.RemainingOilSeconds = State.RemainingRounds * FMath::Max(0.1f, OilExplorationSecondsPerRound);
	}
	RuntimeState->Surfaces.Add(FIntPoint(CellX, CellY), State);
	UE_LOG(LogGridSurfaceRuntime, Log,
		TEXT("[RPG03.6] SurfaceApplied Cell=(%d,%d) Type=%s Rounds=%d Source=%s Action=%s Periodic=%d %s"),
		CellX, CellY, *UEnum::GetValueAsString(State.SurfaceType), State.RemainingRounds,
		*State.SourceCombatantId.ToString(EGuidFormats::Digits), *State.SourceActionId.ToString(), State.PeriodicDamagePerRound,
		*UEnum::GetValueAsString(State.PeriodicDamageType));
	RefreshCombatOilSurfaceVisuals();
	return true;
}

bool AGridLevelRuntimeActor::InteractCombatSurfaceAtCell(int32 CellX, int32 CellY, EGridCombatSurfaceInteraction Interaction,
	const FGridResolvedCombatModifiers& SourceModifiers, FGridCombatSurfaceReactionResult& OutReaction)
{
	OutReaction = FGridCombatSurfaceReactionResult();
	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		return false;
	}

	const FIntPoint Cell(CellX, CellY);
	FGridCombatSurfaceState* Existing = RuntimeState->Surfaces.Find(Cell);
	if (!Existing || !Existing->IsValid() || !FGridCombatSurfaceResolver::ResolveReaction(*Existing, Interaction, SourceModifiers, OutReaction))
	{
		return false;
	}

	FGridCombatSurfaceResolver::ApplyReactionToState(OutReaction, *Existing);
	if (!Existing->IsValid())
	{
		RuntimeState->Surfaces.Remove(Cell);
	}

	UE_LOG(LogGridSurfaceRuntime, Log,
		TEXT("[RPG03.6] SurfaceReaction Cell=(%d,%d) Interaction=%s Output=%s Explosive=%s DamageMod=%d AreaMod=%d"),
		CellX, CellY, *UEnum::GetValueAsString(Interaction), *UEnum::GetValueAsString(OutReaction.OutputSurfaceType),
		OutReaction.bExplosive ? TEXT("true") : TEXT("false"), OutReaction.ExplosionDamagePercentModifier,
		OutReaction.ExplosionAreaRadiusModifier);
	RefreshCombatOilSurfaceVisuals();
	return true;
}

bool AGridLevelRuntimeActor::ConvertCombatSurfaceAtCell(int32 CellX, int32 CellY, const FGridCombatSurfaceConversionProfile& Profile,
	const FGuid& SourceCombatantId, FName SourceActionId, const FGridResolvedCombatModifiers& SourceModifiers)
{
	if (!IsValidCell(CellX, CellY) || !IsWalkableCell(CellX, CellY))
	{
		return false;
	}
	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		return false;
	}

	const FIntPoint Cell(CellX, CellY);
	const FGridCombatSurfaceState* Existing = RuntimeState->Surfaces.Find(Cell);
	FGridCombatSurfaceState Converted;
	if (!FGridCombatSurfaceResolver::ResolveConversion(
			Profile, Existing, SourceCombatantId, SourceActionId, SourceModifiers, Converted))
	{
		return false;
	}

	if (Converted.SurfaceType == EGridCombatSurfaceType::Oil)
	{
		Converted.RemainingOilSeconds = Converted.RemainingRounds * FMath::Max(0.1f, OilExplorationSecondsPerRound);
	}
	RuntimeState->Surfaces.Add(Cell, Converted);
	UE_LOG(LogGridSurfaceRuntime, Log,
		TEXT("[RPG03.9.4F1] SurfaceConverted Cell=(%d,%d) Output=%s Rounds=%d Source=%s Action=%s"),
		CellX, CellY, *UEnum::GetValueAsString(Converted.SurfaceType), Converted.RemainingRounds,
		*Converted.SourceCombatantId.ToString(EGuidFormats::Digits), *Converted.SourceActionId.ToString());
	RefreshCombatOilSurfaceVisuals();
	return true;
}

const FGridCombatSurfaceState* AGridLevelRuntimeActor::FindCombatSurfaceAtCell(int32 CellX, int32 CellY) const
{
	const FGridLevelRuntimeState* RuntimeState = FindRuntimeStateForCurrentLevel();
	return RuntimeState ? RuntimeState->Surfaces.Find(FIntPoint(CellX, CellY)) : nullptr;
}

int32 AGridLevelRuntimeActor::GetCombatSurfaceTraversalCostModifierAtCell(int32 CellX, int32 CellY) const
{
	const FGridCombatSurfaceState* Surface = FindCombatSurfaceAtCell(CellX, CellY);
	return Surface && Surface->IsValid() ? FMath::Clamp(Surface->TraversalCostModifier, 0, 4) : 0;
}

void AGridLevelRuntimeActor::GetCurrentCombatSurfaceSnapshot(TMap<FIntPoint, FGridCombatSurfaceState>& OutSurfaces) const
{
	OutSurfaces.Reset();
	if (const FGridLevelRuntimeState* RuntimeState = FindRuntimeStateForCurrentLevel())
	{
		OutSurfaces = RuntimeState->Surfaces;
	}
}

int32 AGridLevelRuntimeActor::AdvanceCombatSurfaceRound(TArray<FIntPoint>* OutExpiredCells)
{
	if (OutExpiredCells)
	{
		OutExpiredCells->Reset();
	}
	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		return 0;
	}

	TArray<FIntPoint> Cells;
	RuntimeState->Surfaces.GetKeys(Cells);
	Cells.Sort(
		[](const FIntPoint& A, const FIntPoint& B)
		{
			return A.Y == B.Y ? A.X < B.X : A.Y < B.Y;
		});

	int32 ExpiredCount = 0;
	for (const FIntPoint& Cell : Cells)
	{
		FGridCombatSurfaceState* Surface = RuntimeState->Surfaces.Find(Cell);
		if (!Surface)
		{
			continue;
		}
		if (Surface->SurfaceType == EGridCombatSurfaceType::Oil)
		{
			const float SecondsPerRound = FMath::Max(0.1f, OilExplorationSecondsPerRound);
			const float Before = Surface->RemainingOilSeconds > 0.f
				? Surface->RemainingOilSeconds
				: Surface->RemainingRounds * SecondsPerRound;
			Surface->RemainingOilSeconds = FMath::Max(0.f, Before - SecondsPerRound);
			Surface->RemainingRounds = Surface->RemainingOilSeconds <= KINDA_SMALL_NUMBER
				? 0 : FMath::Clamp(FMath::CeilToInt(Surface->RemainingOilSeconds / SecondsPerRound), 1, 6);
		}
		else
		{
			Surface->RemainingRounds = FMath::Max(0, Surface->RemainingRounds - 1);
		}
		if (Surface->RemainingRounds > 0)
		{
			continue;
		}
		if (OutExpiredCells)
		{
			OutExpiredCells->Add(Cell);
		}
		RuntimeState->Surfaces.Remove(Cell);
		++ExpiredCount;
	}
	if (ExpiredCount > 0)
	{
		RefreshCombatOilSurfaceVisuals();
	}
	return ExpiredCount;
}


bool AGridLevelRuntimeActor::IsCombatSmokeAtCell(int32 CellX, int32 CellY) const
{
	const FGridCombatSurfaceState* Surface = FindCombatSurfaceAtCell(CellX, CellY);
	return Surface && Surface->IsValid() && Surface->SurfaceType == EGridCombatSurfaceType::Smoke;
}

bool AGridLevelRuntimeActor::DoesCombatSmokeBlockLine(const FIntPoint& FromCell, const FIntPoint& ToCell) const
{
	const int32 DX = ToCell.X - FromCell.X;
	const int32 DY = ToCell.Y - FromCell.Y;
	if ((DX != 0 && DY != 0) || (DX == 0 && DY == 0))
	{
		return false;
	}

	const FIntPoint Step(FMath::Sign(DX), FMath::Sign(DY));
	FIntPoint Cell = FromCell + Step;
	while (Cell != ToCell)
	{
		if (IsCombatSmokeAtCell(Cell.X, Cell.Y))
		{
			return true;
		}
		Cell += Step;
	}
	return false;
}
