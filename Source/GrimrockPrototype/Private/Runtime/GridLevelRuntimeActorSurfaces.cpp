#include "Runtime/GridLevelRuntimeActor.h"

#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatSurfaceResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridSurfaceRuntime, Log, All);

namespace
{
	constexpr float OilSplashUpdateIntervalSeconds = 1.f / 30.f;
	constexpr int32 OilSplashDropletCount = 8;
	constexpr float CatalystVisualTickInterval = 1.f / 30.f;
	constexpr int32 MaxFireCells = 48;
	constexpr int32 MaxFireLights = 8;
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
		World->GetTimerManager().ClearTimer(CatalystVisualTimer);
	if (IsValid(ActiveCatalystPulse)) ActiveCatalystPulse->DestroyComponent();
	ActiveCatalystPulse = nullptr;
	ActiveCatalystPulseMaterial = nullptr;
	if (IsValid(ActiveCatalystLight)) ActiveCatalystLight->DestroyComponent();
	ActiveCatalystLight = nullptr;
	for (UStaticMeshComponent* Spark : ActiveCatalystSparks)
		if (IsValid(Spark)) Spark->DestroyComponent();
	ActiveCatalystSparks.Reset();
	CatalystVisualElapsedSeconds = 0.f;
}

void AGridLevelRuntimeActor::PlaySurfaceCatalystReactionAtCell(
	int32 CellX, int32 CellY, EGridCombatSurfaceType InputType,
	const FGridCombatSurfaceReactionResult& Reaction)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !LevelAsset ||
		!IsValidCell(CellX,CellY) || ShouldHideCellFloor(CellX,CellY)) return;
	ClearCatalystReactionVfx();
	CatalystVisualKind = ResolveCatalystVfxKind(InputType,Reaction);
	const bool bPoison = CatalystVisualKind == EGridSurfaceCatalystVfxKind::PoisonBurst;
	const bool bIce = CatalystVisualKind == EGridSurfaceCatalystVfxKind::IceMelt;
	const bool bFizzle = CatalystVisualKind == EGridSurfaceCatalystVfxKind::Fizzle;
	const FLinearColor Color = CatalystTint(CatalystVisualKind);
	CatalystVisualCellSize = LevelAsset->CellSize;
	CatalystVisualCenter = CellToWorld(CellX,CellY,0.f) +
		FVector(CatalystVisualCellSize*.5f,CatalystVisualCellSize*.5f,0.f);
	UMaterialInterface* Ring = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_RunePulse.M_SurfaceCatalyst_RunePulse"));
	if (IsValid(Ring))
	{
		UDecalComponent* Pulse = NewObject<UDecalComponent>(this);
		Pulse->SetupAttachment(SceneRoot);
		Pulse->SetMobility(EComponentMobility::Movable);
		Pulse->DecalSize = FVector(48.f,CatalystVisualCellSize,CatalystVisualCellSize);
		Pulse->SetRelativeLocation(CatalystVisualCenter);
		Pulse->SetRelativeRotation(FQuat::FindBetweenNormals(-FVector::ForwardVector,-FVector::UpVector));
		Pulse->SetDecalMaterial(Ring);
		Pulse->SetSortOrder(5);
		Pulse->SetFadeScreenSize(0.f);
		Pulse->RegisterComponent();
		ActiveCatalystPulse = Pulse;
		ActiveCatalystPulseMaterial = Pulse->CreateDynamicMaterialInstance();
		if (IsValid(ActiveCatalystPulseMaterial))
		{
			ActiveCatalystPulseMaterial->SetVectorParameterValue(TEXT("CatalystGlowColor"),Color);
			ActiveCatalystPulseMaterial->SetScalarParameterValue(TEXT("CatalystOpacity"),bFizzle?.35f:1.f);
		}
		Pulse->SetRelativeScale3D(FVector(1.f,.12f,.12f));
	}
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(SceneRoot);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetRelativeLocation(CatalystVisualCenter+FVector(0.f,0.f,38.f));
	Light->SetLightColor(Color);
	Light->SetIntensity(bFizzle?420.f:(bPoison?5200.f:2900.f));
	Light->SetAttenuationRadius(CatalystVisualCellSize*(bPoison?2.2f:1.5f));
	Light->SetCastShadows(false);
	Light->RegisterComponent();
	ActiveCatalystLight = Light;
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_Ember.M_SurfaceCatalyst_Ember"));
	if (IsValid(Sphere) && IsValid(Glow))
	{
		const int32 Count = bPoison?14:(bIce?10:(bFizzle?2:9));
		for (int32 I=0;I<Count;++I)
		{
			UStaticMeshComponent* Spark = NewObject<UStaticMeshComponent>(this);
			Spark->SetupAttachment(SceneRoot);
			Spark->SetMobility(EComponentMobility::Movable);
			Spark->SetStaticMesh(Sphere);
			Spark->SetMaterial(0,Glow);
			Spark->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Spark->SetGenerateOverlapEvents(false);
			Spark->SetCastShadow(false);
			Spark->SetRelativeLocation(CatalystVisualCenter+FVector(0.f,0.f,16.f));
			Spark->SetRelativeScale3D(FVector(.045f));
			Spark->RegisterComponent();
			if (UMaterialInstanceDynamic* Dyn = Spark->CreateDynamicMaterialInstance(0))
			{
				Dyn->SetVectorParameterValue(TEXT("CatalystGlowColor"),
					bPoison && I%3==0 ? FLinearColor(1.f,.23f,.018f) : Color);
				Dyn->SetScalarParameterValue(TEXT("CatalystGlowStrength"),bFizzle?.8f:3.8f);
			}
			ActiveCatalystSparks.Add(Spark);
		}
	}
	CatalystVisualElapsedSeconds = 0.f;
	++CatalystPresentationCount;
	World->GetTimerManager().SetTimer(CatalystVisualTimer,this,
		&AGridLevelRuntimeActor::UpdateCatalystReactionVfx,CatalystVisualTickInterval,true);
}

void AGridLevelRuntimeActor::UpdateCatalystReactionVfx()
{
	CatalystVisualElapsedSeconds += CatalystVisualTickInterval;
	const float T = FMath::Clamp(CatalystVisualElapsedSeconds/FMath::Max(.3f,CatalystPulseDurationSeconds),0.f,1.f);
	if (T >= 1.f) { ClearCatalystReactionVfx(); return; }
	const float Ease = 1.f-FMath::Pow(1.f-T,3.f);
	const bool bPoison = CatalystVisualKind == EGridSurfaceCatalystVfxKind::PoisonBurst;
	const bool bIce = CatalystVisualKind == EGridSurfaceCatalystVfxKind::IceMelt;
	const bool bFizzle = CatalystVisualKind == EGridSurfaceCatalystVfxKind::Fizzle;
	if (IsValid(ActiveCatalystPulse))
	{
		const float S=FMath::Lerp(.12f,bPoison?1.5f:.95f,Ease);
		ActiveCatalystPulse->SetRelativeScale3D(FVector(1.f,S,S));
	}
	if (IsValid(ActiveCatalystPulseMaterial))
		ActiveCatalystPulseMaterial->SetScalarParameterValue(TEXT("CatalystOpacity"),
			(bFizzle?.35f:1.f)*FMath::Square(1.f-T));
	if (IsValid(ActiveCatalystLight))
		ActiveCatalystLight->SetIntensity((bFizzle?420.f:(bPoison?5200.f:2900.f))*FMath::Square(1.f-T));
	const int32 Count=ActiveCatalystSparks.Num();
	for (int32 I=0;I<Count;++I)
	{
		UStaticMeshComponent* Spark=ActiveCatalystSparks[I];
		if (!IsValid(Spark)) continue;
		const float Delay=.018f*(I%4);
		const float Progress=FMath::Clamp((T-Delay)/(1.f-Delay),0.f,1.f);
		const float Radius=CatalystVisualCellSize*(bPoison?.65f:(bIce?.26f:.38f));
		const float Arc=4.f*Progress*(1.f-Progress);
		Spark->SetRelativeLocation(CatalystVisualCenter+
			CatalystDirection(I,Count)*Radius*Progress+
			FVector(0.f,0.f,16.f+(bIce?36.f:(bPoison?115.f:75.f))*Arc));
		const float Shrink=Progress>.7f?FMath::Clamp((1.f-Progress)/.3f,0.f,1.f):1.f;
		const float S=(bPoison?.055f:.042f)*Shrink;
		Spark->SetRelativeScale3D(FVector(S,S,S*(bIce?.65f:1.4f)));
	}
}

void AGridLevelRuntimeActor::ClearFireSurfaceVisuals()
{
	for (UStaticMeshComponent* Flame : ActiveFireSurfaceFlames)
		if (IsValid(Flame)) Flame->DestroyComponent();
	ActiveFireSurfaceFlames.Reset();
	for (UPointLightComponent* Light : ActiveFireSurfaceLights)
		if (IsValid(Light)) Light->DestroyComponent();
	ActiveFireSurfaceLights.Reset();
}

void AGridLevelRuntimeActor::RefreshFireSurfaceVisuals()
{
	ClearFireSurfaceVisuals();
	UWorld* World=GetWorld();
	const FGridLevelRuntimeState* State=FindRuntimeStateForCurrentLevel();
	if (!World || !World->IsGameWorld() || !State || !LevelAsset) return;
	UStaticMesh* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* Glow=LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_Ember.M_SurfaceCatalyst_Ember"));
	if (!IsValid(Sphere) || !IsValid(Glow)) return;
	TArray<FIntPoint> Cells;
	for (const TPair<FIntPoint,FGridCombatSurfaceState>& Entry : State->Surfaces)
	{
		if (Entry.Value.IsValid() && Entry.Value.SurfaceType==EGridCombatSurfaceType::Fire &&
			IsWalkableCell(Entry.Key.X,Entry.Key.Y) && !ShouldHideCellFloor(Entry.Key.X,Entry.Key.Y))
			Cells.Add(Entry.Key);
	}
	Cells.Sort([](const FIntPoint& A,const FIntPoint& B){
		return A.Y==B.Y ? A.X<B.X : A.Y<B.Y;
	});
	const float Size=LevelAsset->CellSize;
	for(int32 I=0;I<FMath::Min(Cells.Num(),MaxFireCells);++I)
	{
		const FIntPoint Cell=Cells[I];
		const FVector Center=CellToWorld(Cell.X,Cell.Y,0.f)+FVector(Size*.5f,Size*.5f,0.f);
		for(int32 J=0;J<3;++J)
		{
			UStaticMeshComponent* Flame=NewObject<UStaticMeshComponent>(this);
			Flame->SetupAttachment(SceneRoot);
			Flame->SetMobility(EComponentMobility::Movable);
			Flame->SetStaticMesh(Sphere);
			Flame->SetMaterial(0,Glow);
			Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Flame->SetGenerateOverlapEvents(false);
			Flame->SetCastShadow(false);
			Flame->SetRelativeLocation(Center+CatalystDirection(J,3)*Size*.19f+
				FVector(0.f,0.f,22.f+J*5.f));
			Flame->SetRelativeScale3D(FVector(.15f,.10f,.42f+J*.09f));
			Flame->RegisterComponent();
			if (UMaterialInstanceDynamic* Dyn=Flame->CreateDynamicMaterialInstance(0))
			{
				Dyn->SetVectorParameterValue(TEXT("CatalystGlowColor"),
					J==0?FLinearColor(1.f,.25f,.015f):FLinearColor(1.f,.48f,.07f));
				Dyn->SetScalarParameterValue(TEXT("CatalystGlowStrength"),2.8f);
			}
			ActiveFireSurfaceFlames.Add(Flame);
		}
		if (I<MaxFireLights)
		{
			UPointLightComponent* Light=NewObject<UPointLightComponent>(this);
			Light->SetupAttachment(SceneRoot);
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetRelativeLocation(Center+FVector(0.f,0.f,45.f));
			Light->SetLightColor(FLinearColor(1.f,.3f,.06f));
			Light->SetIntensity(650.f);
			Light->SetAttenuationRadius(Size*1.2f);
			Light->SetCastShadows(false);
			Light->RegisterComponent();
			ActiveFireSurfaceLights.Add(Light);
		}
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
	// Oil decals and permanent Fire tongues are presentation derived from RPG03.6.
	RefreshFireSurfaceVisuals();
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
