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
	const FGridLevelObjectData& ObjectData, UStaticMesh* InPlateMesh, const FVector& InWorldLocation, bool bStartPressed)
{
	AGridRuntimeObjectActor::InitializeGridObject(ObjectData, nullptr, FTransform(FRotator::ZeroRotator, InWorldLocation));

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
	CurrentMoveDuration = 0.0f;

	if (UsesTargetVisualComposition())
	{
		const float TargetDuration = GetTargetMotionDuration();
		if (TargetDuration > KINDA_SMALL_NUMBER)
		{
			MoveDuration = TargetDuration;
		}
		CurrentMotionAlpha = bIsPressed ? 1.0f : 0.0f;
		AnimStartMotionAlpha = CurrentMotionAlpha;
		AnimTargetMotionAlpha = CurrentMotionAlpha;
		ApplyMovingPartMotionAlpha(0, CurrentMotionAlpha);
	}
	else
	{
		CurrentMotionAlpha = bIsPressed ? 1.0f : 0.0f;
		AnimStartMotionAlpha = CurrentMotionAlpha;
		AnimTargetMotionAlpha = CurrentMotionAlpha;
		SetMovingRelativeLocation(bIsPressed ? PressedLocation : ReleasedLocation);
	}
}

void AGridPressurePlateActor::SetPressed(bool bNewPressed)
{
	if (bIsPressed == bNewPressed && !bIsAnimating)
	{
		return;
	}
	bIsPressed = bNewPressed;
	AnimElapsed = 0.f;

	if (UsesTargetVisualComposition())
	{
		AnimStartMotionAlpha = CurrentMotionAlpha;
		AnimTargetMotionAlpha = bIsPressed ? 1.0f : 0.0f;
		const float Travel = FMath::Abs(AnimTargetMotionAlpha - AnimStartMotionAlpha);
		if (Travel <= KINDA_SMALL_NUMBER)
		{
			CurrentMotionAlpha = AnimTargetMotionAlpha;
			ApplyMovingPartMotionAlpha(0, CurrentMotionAlpha);
			bIsAnimating = false;
			CurrentMoveDuration = 0.0f;
			SetActorTickEnabled(false);
			return;
		}

		CurrentMoveDuration = FMath::Max(0.01f, MoveDuration * Travel);
		bIsAnimating = true;
		SetActorTickEnabled(true);
		return;
	}

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
	if (UsesTargetVisualComposition())
	{
		const float SafeDuration = FMath::Max(0.01f, CurrentMoveDuration);
		AnimElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(AnimElapsed / SafeDuration, 0.f, 1.f);
		CurrentMotionAlpha = FMath::Lerp(AnimStartMotionAlpha, AnimTargetMotionAlpha, Alpha);
		ApplyMovingPartMotionAlpha(0, CurrentMotionAlpha);

		if (Alpha >= 1.f)
		{
			CurrentMotionAlpha = AnimTargetMotionAlpha;
			ApplyMovingPartMotionAlpha(0, CurrentMotionAlpha);
			bIsAnimating = false;
			AnimElapsed = 0.f;
			CurrentMoveDuration = 0.0f;
			SetActorTickEnabled(false);
		}
		return;
	}

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
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	InitializePlate(ObjectData, Mesh, WorldTransform.GetLocation(), ObjectData.bInitiallyActive);
}
