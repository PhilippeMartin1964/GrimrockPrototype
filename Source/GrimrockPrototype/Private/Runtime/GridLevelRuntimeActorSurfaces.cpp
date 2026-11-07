#include "Runtime/GridLevelRuntimeActor.h"

#include "Core/GridSurfaceVfxProfileAsset.h"
#include "Core/GridFlaskBallistics.h"
#include "Core/GridCatalystVfxProfileAsset.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatSurfaceResolver.h"
#include "Runtime/Combat/GridCatalystAreaResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridSurfaceRuntime, Log, All);

namespace
{
	constexpr float OilSplashUpdateIntervalSeconds = 1.f / 30.f;
	constexpr int32 OilSplashDropletCount = 8;
	constexpr float CatalystVisualTickInterval = 1.f / 30.f;
	FVector CatalystDirection(int32 Index, int32 Count)
	{
		const float Angle = (2.f * PI * Index) / FMath::Max(1, Count);
		return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	}
	FLinearColor CatalystTint(EGridSurfaceCatalystVfxKind Kind)
	{
		switch (Kind)
		{
		case EGridSurfaceCatalystVfxKind::PoisonBurst: return FLinearColor(.48f,1.f,.09f);
		case EGridSurfaceCatalystVfxKind::IceMelt: return FLinearColor(.35f,.82f,1.f);
		case EGridSurfaceCatalystVfxKind::Fizzle: return FLinearColor(.36f,.23f,.15f);
		default: return FLinearColor(1.f,.34f,.025f);
		}
	}
	const FName OilSplashOpacityParameter(TEXT("OilOpacity"));
	const FName OilSplashTintParameter(TEXT("OilTint"));

	FVector OilSplashDirection(int32 Index)
	{
		// Fixed, non-random pattern: stable gameplay capture and reproducible tests.
		const float Angle = 2.f * PI * (static_cast<float>(Index) + 0.12f * (Index % 3)) /
			static_cast<float>(OilSplashDropletCount);
		return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	}
	constexpr float OilExplorationTimerInterval = 0.25f;
}


FGridFireVfxSettings AGridLevelRuntimeActor::ResolveFireVfxSettings() const
{
	const UGridSurfaceVfxProfileAsset* Profile = FireVfxProfile.LoadSynchronous();
	return IsValid(Profile) ? Profile->GetSettings() : FGridFireVfxSettings().Sanitized();
}

FGridCatalystVfxSettings AGridLevelRuntimeActor::ResolveCatalystVfxSettings() const
{
	const UGridCatalystVfxProfileAsset* Profile = CatalystVfxProfile.LoadSynchronous();
	return IsValid(Profile) ? Profile->GetSettings() : FGridCatalystVfxSettings().Sanitized();
}

void AGridLevelRuntimeActor::ClearExplorationOilTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OilExplorationTimer);
	}
	LastOilExplorationUpdateTime = 0.0;
	if (UGridTurnManagerComponent* Turns = FindComponentByClass<UGridTurnManagerComponent>())
	{
		Turns->ResetExplorationSurfaceDamageClock();
	}
}

void AGridLevelRuntimeActor::RefreshExplorationOilTimer()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	bool bHasSurfaces = false;
	if (LevelAsset)
	{
		if (const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel())
		{
			for (const TPair<FIntPoint, FGridCombatSurfaceState>& Pair : State->Surfaces)
			{
				if (Pair.Value.IsValid())
				{
					bHasSurfaces = true;
					break;
				}
			}
		}
	}
	if (!bHasSurfaces)
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
	if (UGridTurnManagerComponent* Turns = FindComponentByClass<UGridTurnManagerComponent>())
	{
		if (Turns->bCombatActive)
		{
			Turns->ResetExplorationSurfaceDamageClock();
			return;
		}
	}
	// Apply periodic occupant damage while the surfaces are still present.
	// Combat rounds have their own independent tick and duration authority.
	if (UGridTurnManagerComponent* Turns = FindComponentByClass<UGridTurnManagerComponent>())
	{
		Turns->ResolveExplorationSurfaceDamage(DeltaSeconds);
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
		if (!Surface || !Surface->IsValid())
		{
			continue;
		}
		const float Before = Surface->RemainingSurfaceSeconds > 0.f
			? Surface->RemainingSurfaceSeconds
			: (Surface->SurfaceType == EGridCombatSurfaceType::Oil && Surface->RemainingOilSeconds > 0.f
				? Surface->RemainingOilSeconds : Surface->RemainingRounds * SecondsPerRound);
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
		Surface->RemainingSurfaceSeconds = Remaining;
		if (Surface->SurfaceType == EGridCombatSurfaceType::Oil)
		{
			Surface->RemainingOilSeconds = Remaining;
		}
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

	for (UStaticMeshComponent* Drop : ActiveOilSplashDroplets)
	{
		if (IsValid(Drop))
		{
			Drop->DestroyComponent();
		}
	}
	ActiveOilSplashDroplets.Reset();

	for (UDecalComponent* Spatter : ActiveOilSplashSpatters)
	{
		if (IsValid(Spatter))
		{
			Spatter->DestroyComponent();
		}
	}
	ActiveOilSplashSpatters.Reset();
	ActiveOilSplashSpatterMaterials.Reset();

	OilSplashElapsedSeconds = 0.f;
	OilSplashImpactCenter = FVector::ZeroVector;
}

void AGridLevelRuntimeActor::UpdateCombatOilSplash()
{
	if (!IsValid(ActiveOilSplashDecal) || !IsValid(ActiveOilSplashMaterial))
	{
		ClearCombatOilSplash();
		return;
	}

	OilSplashElapsedSeconds += OilSplashUpdateIntervalSeconds;
	const float Alpha = FMath::Clamp(OilSplashElapsedSeconds /
		FMath::Max(0.4f, OilSplashDurationSeconds), 0.f, 1.f);
	if (Alpha >= 1.f)
	{
		ClearCombatOilSplash();
		return;
	}

	// Main expanding footprint, but not the same static appearance as the
	// underlying persistent oil: warm specular tint and stronger initial opacity.
	const float Spread = GetOilSplashSpreadScale(Alpha);
	ActiveOilSplashDecal->SetRelativeScale3D(FVector(1.f, Spread * 1.15f, Spread * 1.15f));
	ActiveOilSplashMaterial->SetScalarParameterValue(
		OilSplashOpacityParameter, FMath::Lerp(1.f, 0.06f, Alpha));

	// Satellite stains visibly travel away from the point of impact.
	const float Flow = FMath::Clamp((Alpha - 0.04f) / 0.76f, 0.f, 1.f);
	const float Ease = 1.f - FMath::Pow(1.f - Flow, 3.f);
	for (int32 Index = 0; Index < ActiveOilSplashSpatters.Num(); ++Index)
	{
		UDecalComponent* Spatter = ActiveOilSplashSpatters[Index];
		if (!IsValid(Spatter))
		{
			continue;
		}
		const FVector Direction = OilSplashDirection(Index);
		const float Travel = OilSplashImpactCellSize * (0.25f + 0.055f * (Index % 3)) * Ease;
		Spatter->SetRelativeLocation(OilSplashImpactCenter + Direction * Travel);
		const float Size = FMath::Lerp(0.12f, 0.72f, Ease);
		Spatter->SetRelativeScale3D(FVector(1.f, Size, Size));
		if (ActiveOilSplashSpatterMaterials.IsValidIndex(Index) &&
			IsValid(ActiveOilSplashSpatterMaterials[Index]))
		{
			ActiveOilSplashSpatterMaterials[Index]->SetScalarParameterValue(
				OilSplashOpacityParameter, 0.97f * FMath::Square(1.f - Flow));
		}
	}

	// Actual airborne drops make the action perceptible in first-person,
	// independent of the ground decal and the camera angle.
	for (int32 Index = 0; Index < ActiveOilSplashDroplets.Num(); ++Index)
	{
		UStaticMeshComponent* Drop = ActiveOilSplashDroplets[Index];
		if (!IsValid(Drop))
		{
			continue;
		}
		const float Delay = 0.012f * Index;
		const float TravelAlpha = FMath::Clamp((Alpha - Delay) / (0.82f - Delay), 0.f, 1.f);
		const float DropEase = 1.f - FMath::Pow(1.f - TravelAlpha, 3.f);
		const float Travel = OilSplashImpactCellSize * (0.37f + 0.09f * (Index % 3));
		const float Arc = 4.f * TravelAlpha * (1.f - TravelAlpha);
		const FVector Offset = OilSplashDirection(Index) * Travel * DropEase;
		Drop->SetRelativeLocation(OilSplashImpactCenter + Offset + FVector(0.f, 0.f,
			15.f + (42.f + 15.f * (Index % 3)) * Arc));
		const float Shrink = TravelAlpha > 0.67f
			? FMath::Clamp((1.f - TravelAlpha) / 0.33f, 0.f, 1.f) : 1.f;
		const float Size = (0.085f + 0.017f * (Index % 3)) * Shrink;
		Drop->SetRelativeScale3D(FVector(Size));
	}
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
		return; // Visual effects must never invalidate the already resolved action.
	}

	ClearCombatOilSplash();
	const float CellSize = LevelAsset->CellSize;
	OilSplashImpactCellSize = CellSize;
	OilSplashImpactCenter = CellToWorld(CellX, CellY, 0.f) +
		FVector(CellSize * .5f, CellSize * .5f, 0.f);
	const FQuat ProjectionRotation = FQuat::FindBetweenNormals(
		-FVector::ForwardVector, -FVector::UpVector);

	UDecalComponent* Splash = NewObject<UDecalComponent>(this);
	if (!IsValid(Splash))
	{
		return;
	}
	Splash->SetupAttachment(SceneRoot);
	Splash->SetMobility(EComponentMobility::Movable);
	Splash->DecalSize = FVector(48.f, CellSize * 1.40f, CellSize * 1.40f);
	Splash->SetRelativeLocation(OilSplashImpactCenter);
	Splash->SetRelativeRotation(ProjectionRotation);
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
	Splash->SetRelativeScale3D(FVector(1.f, GetOilSplashSpreadScale(0.f), GetOilSplashSpreadScale(0.f)));
	Dynamic->SetScalarParameterValue(OilSplashOpacityParameter, 1.f);
	Dynamic->SetVectorParameterValue(OilSplashTintParameter, FLinearColor(0.11f, 0.075f, 0.035f, 1.f));

	// Eight short-lived satellite stains projected onto the real floor.
	for (int32 Index = 0; Index < OilSplashDropletCount; ++Index)
	{
		UDecalComponent* Spatter = NewObject<UDecalComponent>(this);
		if (!IsValid(Spatter))
		{
			continue;
		}
		Spatter->SetupAttachment(SceneRoot);
		Spatter->SetMobility(EComponentMobility::Movable);
		Spatter->DecalSize = FVector(48.f, CellSize * 0.45f, CellSize * 0.45f);
		Spatter->SetRelativeLocation(OilSplashImpactCenter);
		Spatter->SetRelativeRotation(ProjectionRotation);
		Spatter->SetDecalMaterial(Material);
		Spatter->SetFadeScreenSize(0.f);
		Spatter->SetSortOrder(3);
		Spatter->RegisterComponent();
		UMaterialInstanceDynamic* SpatterMaterial = Spatter->CreateDynamicMaterialInstance();
		if (!IsValid(SpatterMaterial))
		{
			Spatter->DestroyComponent();
			continue;
		}
		Spatter->SetRelativeScale3D(FVector(1.f, 0.12f, 0.12f));
		SpatterMaterial->SetScalarParameterValue(OilSplashOpacityParameter, 0.97f);
		SpatterMaterial->SetVectorParameterValue(OilSplashTintParameter, FLinearColor(0.10f, 0.066f, 0.032f, 1.f));
		ActiveOilSplashSpatters.Add(Spatter);
		ActiveOilSplashSpatterMaterials.Add(SpatterMaterial);
	}

	// Use the shipped surface material on real geometry, not the decal material.
	// UMaterialInterface does not expose GetMaterialDomain() in UE 5.5.4.
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr,
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* DropMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Item_Materials/M_Flask_Oil.M_Flask_Oil"));
	if (IsValid(SphereMesh) && IsValid(DropMaterial))
	{
		for (int32 Index = 0; Index < OilSplashDropletCount; ++Index)
		{
			UStaticMeshComponent* Drop = NewObject<UStaticMeshComponent>(this);
			if (!IsValid(Drop))
			{
				continue;
			}
			Drop->SetupAttachment(SceneRoot);
			Drop->SetMobility(EComponentMobility::Movable);
			Drop->SetStaticMesh(SphereMesh);
			Drop->SetMaterial(0, DropMaterial);
			Drop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Drop->SetGenerateOverlapEvents(false);
			Drop->SetCastShadow(false);
			Drop->SetRelativeLocation(OilSplashImpactCenter + FVector(0.f, 0.f, 15.f));
			Drop->SetRelativeScale3D(FVector(0.085f));
			Drop->RegisterComponent();
			ActiveOilSplashDroplets.Add(Drop);
		}
	}
	else
	{
		UE_LOG(LogGridSurfaceRuntime, Warning,
			TEXT("[ARMEMENT01.17.1.5C] Missing 3D droplet mesh/material; ground spatter remains active."));
	}

	OilSplashElapsedSeconds = 0.f;
	++OilSplashPresentationCount;
	UE_LOG(LogGridSurfaceRuntime, Verbose,
		TEXT("[ARMEMENT01.17.1.5C] OilSplash Cell=(%d,%d) Drops=%d Spatters=%d Duration=%.2f"),
		CellX, CellY, ActiveOilSplashDroplets.Num(), ActiveOilSplashSpatters.Num(),
		FMath::Max(0.4f, OilSplashDurationSeconds));
	World->GetTimerManager().SetTimer(
		OilSplashTimer, this, &AGridLevelRuntimeActor::UpdateCombatOilSplash,
		OilSplashUpdateIntervalSeconds, true);
}


EGridSurfaceCatalystVfxKind AGridLevelRuntimeActor::ResolveCatalystVfxKind(
	EGridCombatSurfaceType InputType, const FGridCombatSurfaceReactionResult& Reaction)
{
	if (!Reaction.bReacted) return EGridSurfaceCatalystVfxKind::Fizzle;
	if (InputType == EGridCombatSurfaceType::Ice &&
		Reaction.OutputSurfaceType == EGridCombatSurfaceType::Water)
		return EGridSurfaceCatalystVfxKind::IceMelt;
	if ((InputType == EGridCombatSurfaceType::Poison ||
		InputType == EGridCombatSurfaceType::PoisonCloud) &&
		Reaction.OutputSurfaceType == EGridCombatSurfaceType::Fire && Reaction.bExplosive)
		return EGridSurfaceCatalystVfxKind::PoisonBurst;
	if (InputType == EGridCombatSurfaceType::Oil &&
		Reaction.OutputSurfaceType == EGridCombatSurfaceType::Fire)
		return EGridSurfaceCatalystVfxKind::OilIgnition;
	return EGridSurfaceCatalystVfxKind::Fizzle;
}

void AGridLevelRuntimeActor::ClearCatalystReactionVfx()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CatalystVisualTimer);
	}
	if (IsValid(ActiveCatalystProjectile))
	{
		ActiveCatalystProjectile->DestroyComponent();
	}
	ActiveCatalystProjectile = nullptr;
	for (UStaticMeshComponent* Trail : ActiveCatalystTrailSparks)
	{
		if (IsValid(Trail)) Trail->DestroyComponent();
	}
	ActiveCatalystTrailSparks.Reset();
	for (UDecalComponent* Pulse : ActiveCatalystCellPulses)
	{
		if (IsValid(Pulse)) Pulse->DestroyComponent();
	}
	for (UPointLightComponent* Light : ActiveCatalystCellLights)
	{
		if (IsValid(Light)) Light->DestroyComponent();
	}
	for (UStaticMeshComponent* Spark : ActiveCatalystSparks)
	{
		if (IsValid(Spark)) Spark->DestroyComponent();
	}
	ActiveCatalystCellPulses.Reset();
	ActiveCatalystCellPulseMaterials.Reset();
	ActiveCatalystCellLights.Reset();
	ActiveCatalystSparks.Reset();
	ActiveCatalystVisualCells.Reset();
	ActiveCatalystPulse = nullptr;
	ActiveCatalystPulseMaterial = nullptr;
	ActiveCatalystLight = nullptr;
	CatalystVisualElapsedSeconds = 0.f;
	CatalystImpactElapsedStart = 0.f;
	CatalystBallisticFlightSeconds = 0.f;
	CatalystFlightInitialVelocity = FVector::ZeroVector;
	bCatalystImpactStarted = false;
	if (bCatalystVisualsDeferred)
	{
		// Explicit cancel/unload: never leave permanent surfaces invisible.
		bCatalystVisualsDeferred = false;
		RefreshCombatOilSurfaceVisuals();
	}
}

void AGridLevelRuntimeActor::BeginCatalystSurfaceVisualBatch()
{
	// A later throw supersedes only transient graphics, never existing RPG03.6.
	ClearCatalystReactionVfx();
	bCatalystVisualsDeferred = true;
}

void AGridLevelRuntimeActor::CancelCatalystSurfaceVisualBatch()
{
	ClearCatalystReactionVfx();
}

void AGridLevelRuntimeActor::PlaySurfaceCatalystReactionAtCell(
	int32 CellX, int32 CellY, EGridCombatSurfaceType InputType,
	const FGridCombatSurfaceReactionResult& Reaction)
{
	// Backward-compatible one-cell presentation used by older C6 tests/callers.
	FGridCatalystCellOutcome Cell;
	Cell.Cell = FIntPoint(CellX, CellY);
	Cell.InputType = InputType;
	Cell.Reaction = Reaction;
	TArray<FGridCatalystCellOutcome> Area;
	Area.Add(Cell);
	PlaySurfaceCatalystArea(Cell.Cell, Cell.Cell, Area);
}

void AGridLevelRuntimeActor::PlaySurfaceCatalystArea(
	FIntPoint LaunchCell, FIntPoint ImpactCell,
	const TArray<FGridCatalystCellOutcome>& Outcomes)
{
	// When called from a gameplay transaction BeginCatalystSurfaceVisualBatch
	// has already cleared the previous effect. Do not flush that batch early.
	if (!bCatalystVisualsDeferred)
	{
		ClearCatalystReactionVfx();
	}
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !LevelAsset ||
		!IsWalkableCell(ImpactCell.X, ImpactCell.Y) || Outcomes.IsEmpty())
	{
		CancelCatalystSurfaceVisualBatch();
		return;
	}
	CatalystVisualElapsedSeconds = 0.f;
	CatalystVisualCellSize = LevelAsset->CellSize;
	CatalystVisualCenter = CellToWorld(ImpactCell.X, ImpactCell.Y, 0.f) +
		FVector(CatalystVisualCellSize * .5f, CatalystVisualCellSize * .5f, 0.f);
	CatalystFlightStart = CellToWorld(LaunchCell.X, LaunchCell.Y, 0.f) +
		FVector(CatalystVisualCellSize * .5f, CatalystVisualCellSize * .5f, 110.f);
	CatalystFlightEnd = CatalystVisualCenter + FVector(0.f, 0.f, 14.f);
	FGridFlaskBallistics Flight;
	const bool bBallisticReachable = FGridFlaskBallistics::Solve(CatalystFlightStart,
		CatalystFlightEnd, ResolveCatalystVfxSettings().LaunchSpeedCmPerSec,
		FMath::Abs(World->GetGravityZ()), Flight);
	CatalystFlightInitialVelocity = bBallisticReachable ? Flight.Velocity : FVector::ZeroVector;
	CatalystBallisticGravityCmPerSec2 = bBallisticReachable ? Flight.GravityCmPerSec2 : 980.f;
	CatalystBallisticFlightSeconds = bBallisticReachable ? Flight.DurationSeconds : 0.f;
	CatalystVisualKind = EGridSurfaceCatalystVfxKind::Fizzle;
	ActiveCatalystVisualCells.Reserve(FMath::Min(Outcomes.Num(), 9));
	TSet<FIntPoint> Seen;
	for (const FGridCatalystCellOutcome& Result : Outcomes)
	{
		if (ActiveCatalystVisualCells.Num() >= 9) break;
		if (!IsWalkableCell(Result.Cell.X, Result.Cell.Y) ||
			ShouldHideCellFloor(Result.Cell.X, Result.Cell.Y) ||
			Seen.Contains(Result.Cell))
		{
			continue;
		}
		Seen.Add(Result.Cell);
		FGridCatalystVisualCell& Visual = ActiveCatalystVisualCells.AddDefaulted_GetRef();
		Visual.Center = CellToWorld(Result.Cell.X, Result.Cell.Y, 0.f) +
			FVector(CatalystVisualCellSize * .5f, CatalystVisualCellSize * .5f, 0.f);
		Visual.Kind = ResolveCatalystVfxKind(Result.InputType, Result.Reaction);
		const int32 Distance = FMath::Max(
			FMath::Abs(Result.Cell.X - ImpactCell.X),
			FMath::Abs(Result.Cell.Y - ImpactCell.Y));
		Visual.WaveDelay = Distance * ResolveCatalystVfxSettings().WaveDelayPerCellSeconds;
		// Prefer the strongest variant when preserving the legacy single-kind accessor.
		if (Visual.Kind == EGridSurfaceCatalystVfxKind::PoisonBurst ||
			(CatalystVisualKind == EGridSurfaceCatalystVfxKind::Fizzle &&
			 Visual.Kind != EGridSurfaceCatalystVfxKind::Fizzle))
		{
			CatalystVisualKind = Visual.Kind;
		}
	}
	if (ActiveCatalystVisualCells.IsEmpty())
	{
		CancelCatalystSurfaceVisualBatch();
		return;
	}
	++CatalystPresentationCount;

	// A real artist-authored mesh is the projectile; its embedded material
	// remains MI_Catalyst. No temporary procedural duplicate is generated.
	UStaticMesh* BottleMesh = ResolveCatalystVfxSettings().ProjectileMesh.LoadSynchronous();
	if (LaunchCell != ImpactCell && bBallisticReachable && IsValid(BottleMesh))
	{
		UStaticMeshComponent* Bottle = NewObject<UStaticMeshComponent>(this);
		Bottle->SetupAttachment(SceneRoot);
		Bottle->SetMobility(EComponentMobility::Movable);
		Bottle->SetStaticMesh(BottleMesh);
		Bottle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Bottle->SetGenerateOverlapEvents(false);
		Bottle->SetCastShadow(true);
		Bottle->SetRelativeLocation(CatalystFlightStart);
		const float MaxExtent = BottleMesh->GetBounds().BoxExtent.GetMax();
		const float Scale = FMath::Clamp(18.f / FMath::Max(1.f, MaxExtent), .005f, 3.f);
		Bottle->SetRelativeScale3D(FVector(Scale));
		Bottle->RegisterComponent();
		ActiveCatalystProjectile = Bottle;
		const FGridCatalystVfxSettings Vfx = ResolveCatalystVfxSettings();
		UStaticMesh* TrailSphere = Vfx.SparkMesh.LoadSynchronous();
		UMaterialInterface* TrailMaterial = Vfx.EmberMaterial.LoadSynchronous();
		if (IsValid(TrailSphere) && IsValid(TrailMaterial))
		{
			for (int32 Index = 0; Index < 6; ++Index)
			{
				UStaticMeshComponent* Trail = NewObject<UStaticMeshComponent>(this);
				Trail->SetupAttachment(SceneRoot);
				Trail->SetMobility(EComponentMobility::Movable);
				Trail->SetStaticMesh(TrailSphere);
				Trail->SetMaterial(0, TrailMaterial);
				Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Trail->SetGenerateOverlapEvents(false);
				Trail->SetCastShadow(false);
				Trail->SetRelativeLocation(CatalystFlightStart);
				Trail->SetRelativeScale3D(FVector::ZeroVector);
				Trail->RegisterComponent();
				if (UMaterialInstanceDynamic* Dyn = Trail->CreateDynamicMaterialInstance(0))
				{
					Dyn->SetVectorParameterValue(TEXT("CatalystGlowColor"),
						FLinearColor(1.f, .33f, .024f));
					Dyn->SetScalarParameterValue(TEXT("CatalystGlowStrength"), 3.5f);
				}
				ActiveCatalystTrailSparks.Add(Trail);
			}
		}
	}
	// If the visual mesh was removed, never leave the surface refresh locked.
	if (!IsValid(ActiveCatalystProjectile))
	{
		BeginCatalystImpactVfx();
	}
	World->GetTimerManager().SetTimer(CatalystVisualTimer, this,
		&AGridLevelRuntimeActor::UpdateCatalystReactionVfx,
		CatalystVisualTickInterval, true);
}

void AGridLevelRuntimeActor::BeginCatalystImpactVfx()
{
	if (bCatalystImpactStarted) return;
	bCatalystImpactStarted = true;
	CatalystImpactElapsedStart = CatalystVisualElapsedSeconds;
	++CatalystImpactCount;
	if (IsValid(ActiveCatalystProjectile))
	{
		ActiveCatalystProjectile->DestroyComponent();
	}
	ActiveCatalystProjectile = nullptr;
	for (UStaticMeshComponent* Trail : ActiveCatalystTrailSparks)
	{
		if (IsValid(Trail)) Trail->DestroyComponent();
	}
	ActiveCatalystTrailSparks.Reset();
	if (bCatalystVisualsDeferred)
	{
		bCatalystVisualsDeferred = false;
		// One rebuild of the complete final state, at the moment of impact.
		RefreshCombatOilSurfaceVisuals();
	}

	const FGridCatalystVfxSettings Vfx = ResolveCatalystVfxSettings();
	UMaterialInterface* Ring = Vfx.RunePulseMaterial.LoadSynchronous();
	UMaterialInterface* Glow = Vfx.EmberMaterial.LoadSynchronous();
	UStaticMesh* Sphere = Vfx.SparkMesh.LoadSynchronous();
	const FQuat Projection = FQuat::FindBetweenNormals(
		-FVector::ForwardVector, -FVector::UpVector);
	for (int32 Index = 0; Index < ActiveCatalystVisualCells.Num(); ++Index)
	{
		FGridCatalystVisualCell& Cell = ActiveCatalystVisualCells[Index];
		const bool bPoison = Cell.Kind == EGridSurfaceCatalystVfxKind::PoisonBurst;
		const bool bIce = Cell.Kind == EGridSurfaceCatalystVfxKind::IceMelt;
		const bool bFizzle = Cell.Kind == EGridSurfaceCatalystVfxKind::Fizzle;
		const FLinearColor Color = CatalystTint(Cell.Kind);
		UDecalComponent* Pulse = nullptr;
		UMaterialInstanceDynamic* PulseMID = nullptr;
		if (IsValid(Ring))
		{
			Pulse = NewObject<UDecalComponent>(this);
			Pulse->SetupAttachment(SceneRoot);
			Pulse->SetMobility(EComponentMobility::Movable);
			Pulse->DecalSize = FVector(48.f, CatalystVisualCellSize, CatalystVisualCellSize);
			Pulse->SetRelativeLocation(Cell.Center);
			Pulse->SetRelativeRotation(Projection);
			Pulse->SetDecalMaterial(Ring);
			Pulse->SetSortOrder(5);
			Pulse->SetFadeScreenSize(0.f);
			Pulse->RegisterComponent();
			PulseMID = Pulse->CreateDynamicMaterialInstance();
			if (IsValid(PulseMID))
			{
				PulseMID->SetVectorParameterValue(TEXT("CatalystGlowColor"), Color);
				PulseMID->SetScalarParameterValue(TEXT("CatalystOpacity"), 0.f);
			}
			Pulse->SetRelativeScale3D(FVector(1.f, .05f, .05f));
		}
		ActiveCatalystCellPulses.Add(Pulse);
		ActiveCatalystCellPulseMaterials.Add(PulseMID);
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(SceneRoot);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetRelativeLocation(Cell.Center + FVector(0.f, 0.f, 38.f));
		Light->SetLightColor(Color);
		Light->SetIntensity(0.f);
		Light->SetAttenuationRadius(CatalystVisualCellSize * (bPoison ? 2.2f : 1.35f));
		Light->SetCastShadows(false);
		Light->RegisterComponent();
		ActiveCatalystCellLights.Add(Light);

		Cell.FirstSpark = ActiveCatalystSparks.Num();
		const int32 Count = bPoison ? 14 : (bIce ? 8 : (bFizzle ? 2 : 7));
		if (IsValid(Sphere) && IsValid(Glow))
		{
			for (int32 SparkIndex = 0; SparkIndex < Count; ++SparkIndex)
			{
				UStaticMeshComponent* Spark = NewObject<UStaticMeshComponent>(this);
				Spark->SetupAttachment(SceneRoot);
				Spark->SetMobility(EComponentMobility::Movable);
				Spark->SetStaticMesh(Sphere);
				Spark->SetMaterial(0, Glow);
				Spark->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Spark->SetGenerateOverlapEvents(false);
				Spark->SetCastShadow(false);
				Spark->SetRelativeLocation(Cell.Center + FVector(0.f, 0.f, 16.f));
				Spark->SetRelativeScale3D(FVector::ZeroVector);
				Spark->RegisterComponent();
				if (UMaterialInstanceDynamic* Dyn = Spark->CreateDynamicMaterialInstance(0))
				{
					Dyn->SetVectorParameterValue(TEXT("CatalystGlowColor"),
						bPoison && SparkIndex % 3 == 0 ? FLinearColor(1.f, .24f, .016f) : Color);
					Dyn->SetScalarParameterValue(TEXT("CatalystGlowStrength"), bFizzle ? .8f : 3.8f);
				}
				ActiveCatalystSparks.Add(Spark);
			}
		}
		Cell.SparkCount = ActiveCatalystSparks.Num() - Cell.FirstSpark;
	}
	ActiveCatalystPulse = ActiveCatalystCellPulses.IsValidIndex(0) ?
		ActiveCatalystCellPulses[0] : nullptr;
	ActiveCatalystPulseMaterial = ActiveCatalystCellPulseMaterials.IsValidIndex(0) ?
		ActiveCatalystCellPulseMaterials[0] : nullptr;
	ActiveCatalystLight = ActiveCatalystCellLights.IsValidIndex(0) ?
		ActiveCatalystCellLights[0] : nullptr;
	UE_LOG(LogGridSurfaceRuntime, Verbose,
		TEXT("[ARMEMENT01.17.3C] CatalystImpact Cells=%d Sparks=%d"),
		ActiveCatalystVisualCells.Num(), ActiveCatalystSparks.Num());
}

void AGridLevelRuntimeActor::UpdateCatalystReactionVfx()
{
	CatalystVisualElapsedSeconds += CatalystVisualTickInterval;
	const FGridCatalystVfxSettings Vfx = ResolveCatalystVfxSettings();
	const float Flight = CatalystBallisticFlightSeconds;
	if (!bCatalystImpactStarted)
	{
		if (IsValid(ActiveCatalystProjectile) && Flight > 0.f && CatalystVisualElapsedSeconds < Flight)
		{
			const float Alpha = FMath::Clamp(CatalystVisualElapsedSeconds / Flight, 0.f, 1.f);
			const float Time = CatalystVisualElapsedSeconds;
			const FVector Position = CatalystFlightStart + CatalystFlightInitialVelocity * Time +
				FVector(0.f, 0.f, .5f * -CatalystBallisticGravityCmPerSec2 * Time * Time);
			ActiveCatalystProjectile->SetRelativeLocation(Position);
			ActiveCatalystProjectile->SetRelativeRotation(
				FRotator(Alpha * 380.f, Alpha * 470.f, Alpha * 250.f));
			for (int32 Index = 0; Index < ActiveCatalystTrailSparks.Num(); ++Index)
			{
				UStaticMeshComponent* Trail = ActiveCatalystTrailSparks[Index];
				if (!IsValid(Trail)) continue;
				const float TrailAlpha = FMath::Max(0.f, Alpha - .045f * (Index + 1));
				const float TrailTime = TrailAlpha * Flight;
				const FVector TrailPos = CatalystFlightStart + CatalystFlightInitialVelocity * TrailTime +
					FVector(0.f, 0.f, .5f * -CatalystBallisticGravityCmPerSec2 * TrailTime * TrailTime);
				Trail->SetRelativeLocation(TrailPos);
				const float Scale = .038f * (1.f - static_cast<float>(Index) / 7.f) *
					FMath::Clamp(Alpha * 5.f, 0.f, 1.f);
				Trail->SetRelativeScale3D(FVector(Scale));
			}
			return;
		}
		BeginCatalystImpactVfx();
	}
	const float ImpactElapsed = FMath::Max(0.f,
		CatalystVisualElapsedSeconds - CatalystImpactElapsedStart);
	const float Duration = Vfx.PulseDurationSeconds;
	const float MaxWave = Vfx.WaveDelayPerCellSeconds;
	if (ImpactElapsed >= Duration + MaxWave)
	{
		ClearCatalystReactionVfx();
		return;
	}
	for (int32 Index = 0; Index < ActiveCatalystVisualCells.Num(); ++Index)
	{
		const FGridCatalystVisualCell& Cell = ActiveCatalystVisualCells[Index];
		const float Progress = FMath::Clamp((ImpactElapsed - Cell.WaveDelay) / Duration, 0.f, 1.f);
		const float Ease = 1.f - FMath::Pow(1.f - Progress, 3.f);
		const bool bPoison = Cell.Kind == EGridSurfaceCatalystVfxKind::PoisonBurst;
		const bool bIce = Cell.Kind == EGridSurfaceCatalystVfxKind::IceMelt;
		const bool bFizzle = Cell.Kind == EGridSurfaceCatalystVfxKind::Fizzle;
		if (ActiveCatalystCellPulses.IsValidIndex(Index) &&
			IsValid(ActiveCatalystCellPulses[Index]))
		{
			const float Scale = FMath::Lerp(.05f, bPoison ? 1.38f : .92f, Ease);
			ActiveCatalystCellPulses[Index]->SetRelativeScale3D(FVector(1.f, Scale, Scale));
		}
		if (ActiveCatalystCellPulseMaterials.IsValidIndex(Index) &&
			IsValid(ActiveCatalystCellPulseMaterials[Index]))
		{
			ActiveCatalystCellPulseMaterials[Index]->SetScalarParameterValue(
				TEXT("CatalystOpacity"), (bFizzle ? .26f : 1.f) * FMath::Square(1.f - Progress));
		}
		if (ActiveCatalystCellLights.IsValidIndex(Index) &&
			IsValid(ActiveCatalystCellLights[Index]))
		{
			ActiveCatalystCellLights[Index]->SetIntensity(
				(bFizzle ? 350.f : (bPoison ? 5800.f : 2900.f)) *
				FMath::Square(1.f - Progress));
		}
		for (int32 SparkIndex = 0; SparkIndex < Cell.SparkCount; ++SparkIndex)
		{
			UStaticMeshComponent* Spark = ActiveCatalystSparks[Cell.FirstSpark + SparkIndex];
			if (!IsValid(Spark)) continue;
			const float Delay = .018f * (SparkIndex % 4);
			const float T = FMath::Clamp((Progress - Delay) / (1.f - Delay), 0.f, 1.f);
			const float Radius = CatalystVisualCellSize * (bPoison ? .66f : (bIce ? .25f : .39f));
			const float Arc = 4.f * T * (1.f - T);
			Spark->SetRelativeLocation(Cell.Center +
				CatalystDirection(SparkIndex, Cell.SparkCount) * Radius * T +
				FVector(0.f, 0.f, 15.f + (bIce ? 31.f : (bPoison ? 120.f : 75.f)) * Arc));
			const float Shrink = T > .7f ? FMath::Clamp((1.f - T) / .3f, 0.f, 1.f) : 1.f;
			const float Scale = (bPoison ? .06f : .042f) * Shrink;
			Spark->SetRelativeScale3D(FVector(Scale, Scale, Scale * (bIce ? .7f : 1.4f)));
		}
	}
}

void AGridLevelRuntimeActor::ClearFireSurfaceVisuals()
{
	for (UNiagaraComponent* FlameFx : ActiveFireNiagaraComponents)
	{
		if (IsValid(FlameFx))
		{
			FlameFx->DeactivateImmediate();
			FlameFx->DestroyComponent();
		}
	}
	ActiveFireNiagaraComponents.Reset();
	for (UPointLightComponent* Light : ActiveFireSurfaceLights)
	{
		if (IsValid(Light))
		{
			Light->DestroyComponent();
		}
	}
	ActiveFireSurfaceLights.Reset();
}

void AGridLevelRuntimeActor::RefreshFireSurfaceVisuals()
{
	ClearFireSurfaceVisuals();
	UWorld* World = GetWorld();
	const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel();
	if (!World || !World->IsGameWorld() || !State || !LevelAsset)
	{
		return;
	}

	const FGridFireVfxSettings Vfx = ResolveFireVfxSettings();
	if (Vfx.MaxFireCells <= 0)
	{
		return;
	}
	// No torch fallback, procedural spheres or vendor-specific hardcoded paths.
	// A selected, reusable visual profile owns the Niagara asset.
	UNiagaraSystem* FlameSystem = Vfx.GroundFireSystem.LoadSynchronous();
	if (!IsValid(FlameSystem))
	{
		return;
	}

	TArray<FIntPoint> Cells;
	for (const TPair<FIntPoint, FGridCombatSurfaceState>& Entry : State->Surfaces)
	{
		if (Entry.Value.IsValid() && Entry.Value.SurfaceType == EGridCombatSurfaceType::Fire &&
			IsWalkableCell(Entry.Key.X, Entry.Key.Y) && !ShouldHideCellFloor(Entry.Key.X, Entry.Key.Y))
		{
			Cells.Add(Entry.Key);
		}
	}
	Cells.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return A.Y == B.Y ? A.X < B.X : A.Y < B.Y;
	});

	const float CellSize = FMath::Max(1.f, LevelAsset->CellSize);
	const float Scale = Vfx.GroundFireScale;
	const int32 VisibleCount = FMath::Min(Cells.Num(), Vfx.MaxFireCells);

	for (int32 Index = 0; Index < VisibleCount; ++Index)
	{
		const FIntPoint Cell = Cells[Index];
		const FVector Center = CellToWorld(Cell.X, Cell.Y, 0.f) +
			FVector(CellSize * .5f, CellSize * .5f, 0.f);
		UNiagaraComponent* FlameFx = NewObject<UNiagaraComponent>(this);
		FlameFx->SetAutoActivate(false);
		FlameFx->SetupAttachment(SceneRoot);
		FlameFx->SetAsset(FlameSystem);
		FlameFx->SetRelativeLocation(Center + FVector(0.f, 0.f, 4.f));
		FlameFx->SetRelativeScale3D(FVector(Scale));
		FlameFx->RegisterComponent();

		// Names and types must match actual User Parameters on the selected Niagara.
		// Empty names preserve the asset's own authored values.
		if (!Vfx.SpawnAreaParameter.IsNone())
		{
			const float Width = CellSize * Vfx.GroundFireCoverage / Scale;
			FlameFx->SetVariableVec3(Vfx.SpawnAreaParameter,
				FVector(Width, Width, Vfx.FlameHeightCm / Scale));
		}
		if (!Vfx.FlameHeightParameter.IsNone())
		{
			FlameFx->SetVariableFloat(Vfx.FlameHeightParameter, Vfx.FlameHeightCm / Scale);
		}
		if (!Vfx.FlameDensityParameter.IsNone())
		{
			FlameFx->SetVariableFloat(Vfx.FlameDensityParameter, Vfx.FlameDensity);
		}
		if (!Vfx.SmokeDensityParameter.IsNone())
		{
			FlameFx->SetVariableFloat(Vfx.SmokeDensityParameter,
				Vfx.bEnableSmoke ? Vfx.SmokeDensity : 0.f);
		}
		if (!Vfx.FireTintParameter.IsNone())
		{
			FlameFx->SetVariableLinearColor(Vfx.FireTintParameter, Vfx.FireTint);
		}
		FlameFx->Activate(true);
		ActiveFireNiagaraComponents.Add(FlameFx);

		if (Index < Vfx.MaxFireLights && Vfx.FireLightIntensity > 0.f)
		{
			UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
			Light->SetupAttachment(SceneRoot);
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetRelativeLocation(Center + FVector(0.f, 0.f, 45.f));
			Light->SetLightColor(Vfx.FireLightColor);
			Light->SetIntensity(Vfx.FireLightIntensity);
			Light->SetAttenuationRadius(CellSize * Vfx.FireLightRadiusCells);
			Light->SetCastShadows(false);
			Light->RegisterComponent();
			ActiveFireSurfaceLights.Add(Light);
		}
	}
}

void AGridLevelRuntimeActor::ClearWaterSurfaceVisuals()
{
	for (UDecalComponent* Decal : ActiveWaterSurfaceDecals)
		if (IsValid(Decal)) Decal->DestroyComponent();
	ActiveWaterSurfaceDecals.Reset();
	ActiveWaterSurfaceMaterials.Reset();
}

void AGridLevelRuntimeActor::RefreshWaterSurfaceVisuals()
{
	ClearWaterSurfaceVisuals();
	UWorld* World=GetWorld();
	const FGridLevelRuntimeState* State=FindRuntimeStateForCurrentLevel();
	if (!World || !World->IsGameWorld() || !State || !LevelAsset) return;
	UMaterialInterface* Wet=LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Surfaces/MI_Surface_Oil.MI_Surface_Oil"));
	if (!IsValid(Wet)) Wet=OilSurfaceMaterial.LoadSynchronous();
	if (!IsValid(Wet)) return;
	TArray<FIntPoint> Cells;
	for (const TPair<FIntPoint,FGridCombatSurfaceState>& E : State->Surfaces)
	{
		if (E.Value.IsValid() && E.Value.SurfaceType==EGridCombatSurfaceType::Water &&
			IsWalkableCell(E.Key.X,E.Key.Y) && !ShouldHideCellFloor(E.Key.X,E.Key.Y))
			Cells.Add(E.Key);
	}
	Cells.Sort([](const FIntPoint& A,const FIntPoint& B) {
		return A.Y==B.Y ? A.X<B.X : A.Y<B.Y;
	});
	const float Size=LevelAsset->CellSize;
	const FQuat Rot=FQuat::FindBetweenNormals(-FVector::ForwardVector,-FVector::UpVector);
	for(int32 I=0;I<FMath::Min(Cells.Num(),48);++I)
	{
		const FIntPoint Cell=Cells[I];
		UDecalComponent* Decal=NewObject<UDecalComponent>(this);
		Decal->SetupAttachment(SceneRoot);
		Decal->SetMobility(EComponentMobility::Movable);
		Decal->DecalSize=FVector(48.f,Size*.96f,Size*.96f);
		Decal->SetRelativeLocation(CellToWorld(Cell.X,Cell.Y,0.f)+FVector(Size*.5f,Size*.5f,0.f));
		Decal->SetRelativeRotation(Rot);
		Decal->SetDecalMaterial(Wet);
		Decal->SetFadeScreenSize(0.f);
		Decal->SetSortOrder(2);
		Decal->RegisterComponent();
		if (UMaterialInstanceDynamic* MID=Decal->CreateDynamicMaterialInstance())
		{
			MID->SetVectorParameterValue(TEXT("OilTint"),FLinearColor(.025f,.10f,.16f));
			MID->SetScalarParameterValue(TEXT("OilOpacity"),.45f);
			MID->SetScalarParameterValue(TEXT("OilRoughness"),.08f);
			ActiveWaterSurfaceMaterials.Add(MID);
		}
		ActiveWaterSurfaceDecals.Add(Decal);
	}
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
	// A catalyst throw commits its C6 data once, but the player sees surfaces
	// switch only at the bottle impact. Other gameplay readers see committed data.
	if (bCatalystVisualsDeferred)
	{
		return;
	}
	// Oil decals and permanent Fire tongues are presentation derived from RPG03.6.
	RefreshFireSurfaceVisuals();
	RefreshWaterSurfaceVisuals();
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


bool AGridLevelRuntimeActor::ConfigureCatalystFireAtCell(int32 CellX, int32 CellY,
	const FGuid& SourceCombatantId, FName SourceActionId)
{
	FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
	FGridCombatSurfaceState* Surface = State ? State->Surfaces.Find(FIntPoint(CellX, CellY)) : nullptr;
	if (!Surface || !Surface->IsValid() || Surface->SurfaceType != EGridCombatSurfaceType::Fire)
	{
		return false;
	}
	Surface->SourceCombatantId = SourceCombatantId;
	Surface->SourceActionId = SourceActionId;
	Surface->PeriodicDamageType = EGridDamageType::Fire;
	Surface->PeriodicDamagePerRound = FMath::Clamp(CatalystFireDamagePerRound, 0, 1000);
	return true;
}

bool AGridLevelRuntimeActor::CommitCatalystAreaPlan(
	const TArray<FGridCatalystCellSnapshot>& Snapshot,
	const TArray<FGridCatalystCellOutcome>& Outcomes,
	const FGuid& SourceCombatantId, FName SourceActionId)
{
	if (Snapshot.IsEmpty() || Snapshot.Num() != Outcomes.Num() ||
		!SourceCombatantId.IsValid() || SourceActionId.IsNone())
	{
		return false;
	}
	FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
	if (!LevelAsset || !State)
	{
		return false;
	}
	// Validate ALL original surface states and build replacements without writes.
	TMap<FIntPoint, FGridCombatSurfaceState> Updates;
	const float SecondsPerRound = FMath::Max(0.1f, OilExplorationSecondsPerRound);
	for (int32 Index = 0; Index < Snapshot.Num(); ++Index)
	{
		const FGridCatalystCellSnapshot& Initial = Snapshot[Index];
		const FGridCatalystCellOutcome& Planned = Outcomes[Index];
		if (Initial.Cell != Planned.Cell || !IsWalkableCell(Initial.Cell.X, Initial.Cell.Y) ||
			Updates.Contains(Initial.Cell))
		{
			return false;
		}
		const FGridCombatSurfaceState* Current = State->Surfaces.Find(Initial.Cell);
		if (Initial.bHasSurface != (Current && Current->IsValid()) ||
			(Initial.bHasSurface && (!Current ||
				Current->SurfaceType != Initial.Surface.SurfaceType ||
				Current->RemainingRounds != Initial.Surface.RemainingRounds)))
		{
			return false;
		}
		if (!Planned.Reaction.bReacted)
		{
			continue;
		}
		if (!Current || !Current->IsValid())
		{
			return false;
		}
		FGridCombatSurfaceState Updated = *Current;
		const float Duration = Updated.RemainingSurfaceSeconds > 0.f ? Updated.RemainingSurfaceSeconds
			: (Updated.SurfaceType == EGridCombatSurfaceType::Oil && Updated.RemainingOilSeconds > 0.f
				? Updated.RemainingOilSeconds : Updated.RemainingRounds * SecondsPerRound);
		FGridCombatSurfaceResolver::ApplyReactionToState(Planned.Reaction, Updated);
		if (Updated.IsValid())
		{
			Updated.SourceCombatantId = SourceCombatantId;
			Updated.SourceActionId = SourceActionId;
			Updated.RemainingSurfaceSeconds = Duration;
			Updated.RemainingOilSeconds = Updated.SurfaceType == EGridCombatSurfaceType::Oil ? Duration : 0.f;
			if (Updated.SurfaceType == EGridCombatSurfaceType::Fire)
			{
				Updated.PeriodicDamageType = EGridDamageType::Fire;
				Updated.PeriodicDamagePerRound = FMath::Clamp(CatalystFireDamagePerRound, 0, 1000);
			}
		}
		Updates.Add(Initial.Cell, Updated);
	}
	// No failure path after this point. A nonreactive plan has zero mutations.
	for (const TPair<FIntPoint, FGridCombatSurfaceState>& Entry : Updates)
	{
		if (Entry.Value.IsValid())
		{
			State->Surfaces.Add(Entry.Key, Entry.Value);
		}
		else
		{
			State->Surfaces.Remove(Entry.Key);
		}
	}
	if (!Updates.IsEmpty())
	{
		RefreshCombatOilSurfaceVisuals();
	}
	return true;
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
	State.RemainingSurfaceSeconds = State.RemainingRounds * FMath::Max(0.1f, OilExplorationSecondsPerRound);
	if (State.SurfaceType == EGridCombatSurfaceType::Oil)
	{
		State.RemainingOilSeconds = State.RemainingSurfaceSeconds;
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

	const float SecondsPerRound = FMath::Max(0.1f, OilExplorationSecondsPerRound);
	const float Remaining = Existing->RemainingSurfaceSeconds > 0.f
		? Existing->RemainingSurfaceSeconds
		: (Existing->SurfaceType == EGridCombatSurfaceType::Oil && Existing->RemainingOilSeconds > 0.f
			? Existing->RemainingOilSeconds : Existing->RemainingRounds * SecondsPerRound);
	FGridCombatSurfaceResolver::ApplyReactionToState(OutReaction, *Existing);
	if (Existing->IsValid())
	{
		Existing->RemainingSurfaceSeconds = Remaining;
		if (Existing->SurfaceType != EGridCombatSurfaceType::Oil)
		{
			Existing->RemainingOilSeconds = 0.f;
		}
	}
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

	Converted.RemainingSurfaceSeconds = Converted.RemainingRounds * FMath::Max(0.1f, OilExplorationSecondsPerRound);
	if (Converted.SurfaceType == EGridCombatSurfaceType::Oil)
	{
		Converted.RemainingOilSeconds = Converted.RemainingSurfaceSeconds;
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
		const float SecondsPerRound = FMath::Max(0.1f, OilExplorationSecondsPerRound);
		const float Before = Surface->RemainingSurfaceSeconds > 0.f ? Surface->RemainingSurfaceSeconds
			: (Surface->SurfaceType == EGridCombatSurfaceType::Oil && Surface->RemainingOilSeconds > 0.f
				? Surface->RemainingOilSeconds : Surface->RemainingRounds * SecondsPerRound);
		const float Remaining = FMath::Max(0.f, Before - SecondsPerRound);
		Surface->RemainingSurfaceSeconds = Remaining;
		if (Surface->SurfaceType == EGridCombatSurfaceType::Oil)
		{
			Surface->RemainingOilSeconds = Remaining;
		}
		Surface->RemainingRounds = Remaining <= KINDA_SMALL_NUMBER
			? 0 : FMath::Clamp(FMath::CeilToInt(Remaining / SecondsPerRound), 1, 6);
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
