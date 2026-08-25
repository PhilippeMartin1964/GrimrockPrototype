#include "Runtime/GridPressurePlateActor.h"

AGridPressurePlateActor::AGridPressurePlateActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);
}

void AGridPressurePlateActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsAnimating)
	{
		UpdateAnimation(DeltaSeconds);
	}
}

void AGridPressurePlateActor::InitializePlate(
	const FGridLevelObjectData& ObjectData, UStaticMesh* InPlateMesh, UMaterialInterface* InMaterial, const FVector& InWorldLocation, bool bStartPressed)
{
	AGridRuntimeObjectActor::InitializeGridObject(ObjectData, nullptr, nullptr, FTransform(FRotator::ZeroRotator, InWorldLocation));

	ReleasedHeightAboveFloor = ObjectData.Behavior.PressurePlateAnimation.ReleasedHeightAboveFloor;
	PressedHeightAboveFloor = ObjectData.Behavior.PressurePlateAnimation.PressedHeightAboveFloor;
	MoveDuration = ObjectData.Behavior.PressurePlateAnimation.MoveDuration;
	SetWeightState(0.0f, ObjectData.Behavior.PressurePlateWeight.RequiredItemWeight, ObjectData.Behavior.PressurePlateWeight.bUseItemWeight,
		ObjectData.Behavior.PressurePlateWeight.bActivateWhenPartyPresent);

	ReleasedLocation = FVector(0.f, 0.f, ReleasedHeightAboveFloor);
	PressedLocation = FVector(0.f, 0.f, PressedHeightAboveFloor);

	bIsPressed = bStartPressed;
	bIsAnimating = false;
	AnimElapsed = 0.f;

	SetMovingRelativeLocation(bIsPressed ? PressedLocation : ReleasedLocation);
}

void AGridPressurePlateActor::SetPressed(bool bNewPressed)
{
	if (bIsPressed == bNewPressed && !bIsAnimating)
	{
		return;
	}
	bIsPressed = bNewPressed;
	AnimElapsed = 0.f;
	bIsAnimating = true;

	AnimStartLocation = GetMovingRelativeLocation();
	AnimTargetLocation = bIsPressed ? PressedLocation : ReleasedLocation;
	SetActorTickEnabled(true);
}

void AGridPressurePlateActor::SetWeightState(float InCurrentItemWeight, float InRequiredItemWeight, bool bInUseItemWeight, bool bInActivateWhenPartyPresent)
{
	CurrentItemWeight = FMath::Max(0.0f, InCurrentItemWeight);
	RequiredItemWeight = FMath::Max(0.0f, InRequiredItemWeight);
	bUseItemWeight = bInUseItemWeight;
	bActivateWhenPartyPresent = bInActivateWhenPartyPresent;
}

void AGridPressurePlateActor::UpdateAnimation(float DeltaSeconds)
{
	const float SafeDuration = FMath::Max(0.01f, MoveDuration);

	AnimElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(AnimElapsed / SafeDuration, 0.f, 1.f);

	SetMovingRelativeLocation(FMath::Lerp(AnimStartLocation, AnimTargetLocation, Alpha));

	if (Alpha >= 1.f)
	{
		SetMovingRelativeLocation(AnimTargetLocation);
		bIsAnimating = false;
		AnimElapsed = 0.f;
		SetActorTickEnabled(false);
	}
}

void AGridPressurePlateActor::InitializeGridObject(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform)
{
	InitializePlate(ObjectData, Mesh, Material, WorldTransform.GetLocation(), ObjectData.bInitiallyActive);
}
