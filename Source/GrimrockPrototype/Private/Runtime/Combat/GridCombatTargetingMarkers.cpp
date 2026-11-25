#include "Runtime/Combat/GridCombatTargetingMarkers.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "UObject/ConstructorHelpers.h"

AGridCombatTargetingMarkers::AGridCombatTargetingMarkers()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("GridOrigin"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
		TEXT("/Game/GrimrockPrototype/Art/Materials/Combat/M_CombatTargetingCell.M_CombatTargetingCell"));
	const FName Names[] = { TEXT("Center"), TEXT("Area"), TEXT("Invalid") };
	for (int32 Index = 0; Index < MarkerCount; ++Index)
	{
		UInstancedStaticMeshComponent* Marker = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Names[Index]);
		Marker->SetupAttachment(RootComponent);
		Marker->SetStaticMesh(Plane.Object);
		Marker->SetMaterial(0, Material.Object);
		Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Marker->SetGenerateOverlapEvents(false);
		Marker->SetCanEverAffectNavigation(false);
		Marker->SetCastShadow(false);
		Markers[Index] = Marker;
	}
}

void AGridCombatTargetingMarkers::Show(const AGridLevelRuntimeActor& Level, const FGridCombatActionTargetingPreview& Preview)
{
	if (!IsValid(Level.LevelAsset))
	{
		Clear();
		return;
	}

	TArrayView<const FIntPoint> Cells = Preview.AffectedCells;
	if (Cells.IsEmpty() && !Preview.bValid && Level.IsValidCell(Preview.TargetCell.X, Preview.TargetCell.Y))
	{
		Cells = MakeArrayView(&Preview.TargetCell, 1);
	}
	if (Cells.IsEmpty())
	{
		Clear();
		return;
	}

	constexpr double FloorOffset = 8.0;
	constexpr double PlaneSize = 100.0;
	const double CellSize = Level.LevelAsset->CellSize;
	const double Scale = CellSize * 0.92 / PlaneSize;
	const FVector Origin = Level.GetCellCenterWorld(0, 0, FloorOffset);
	if (Origin != GetActorLocation())
	{
		SetActorLocation(Origin);
	}
	if (DisplayedCells == Cells && DisplayedTarget == Preview.TargetCell && bDisplayedValid == Preview.bValid && DisplayedScale == Scale)
	{
		return;
	}

	Clear();
	DisplayedCells = Cells;
	DisplayedTarget = Preview.TargetCell;
	bDisplayedValid = Preview.bValid;
	DisplayedScale = Scale;

	TArray<FTransform> Instances[MarkerCount];
	for (const FIntPoint& Cell : Cells)
	{
		const EMarker Marker = Preview.bValid ? (Cell == Preview.TargetCell ? Center : Area) : Invalid;
		Instances[Marker].Emplace(FQuat::Identity, FVector(Cell.X * CellSize, Cell.Y * CellSize, 0.0), FVector(Scale, Scale, 1.0));
	}

	const FVector Colors[] = { FVector(1.0, 1.0, 0.0), FVector(0.03, 0.72, 0.12), FVector(0.72, 0.03, 0.03) };
	for (int32 Index = 0; Index < MarkerCount; ++Index)
	{
		if (!Instances[Index].IsEmpty())
		{
			Markers[Index]->SetVectorParameterValueOnMaterials(TEXT("MarkerColor"), Colors[Index]);
			Markers[Index]->AddInstances(Instances[Index], false, false, false);
		}
	}
}

void AGridCombatTargetingMarkers::Clear()
{
	if (!DisplayedCells.IsEmpty())
	{
		for (UInstancedStaticMeshComponent* Marker : Markers)
		{
			Marker->ClearInstances();
		}
		DisplayedCells.Reset();
	}
}
