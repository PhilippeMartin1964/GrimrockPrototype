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


void AGridPressurePlateActor::InitializeRuntimePlate(
	const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* InPlateMesh, const FVector& InWorldLocation, bool bStartPressed)
{
	(void)InPlateMesh;
	AGridRuntimeObjectActor::InitializeRuntimeWorldObject(ObjectData, nullptr, FTransform(FRotator::ZeroRotator, InWorldLocation));

	// RECOVERY01-C2: MoveDuration remains the forward cache; reverse timing is resolved on demand.
	MoveDuration = GetTargetMotionDuration(false);
	const FGridObjectBehaviorParams EffectiveBehavior = ResolveEffectiveBehavior(ObjectData);
	const FGridPressurePlateWeightParams& WeightParams = EffectiveBehavior.PressurePlateWeight;
	SetWeightState(0.0f, WeightParams.RequiredItemWeight, WeightParams.bUseItemWeight, WeightParams.bActivateWhenPartyPresent);

	bIsPressed = bStartPressed;
	bIsAnimating = false;
	AnimElapsed = 0.f;
	CurrentMoveDuration = 0.0f;
	CurrentMotionAlpha = bIsPressed ? 1.0f : 0.0f;
	AnimStartMotionAlpha = CurrentMotionAlpha;
	AnimTargetMotionAlpha = CurrentMotionAlpha;
	ApplyMovingPartMotionAlpha(0, CurrentMotionAlpha);
}

void AGridPressurePlateActor::SetPressed(bool bNewPressed)
{
	if (bIsPressed == bNewPressed && !bIsAnimating)
	{
		return;
	}
	const bool bReverseMotion = !bNewPressed;
	bIsPressed = bNewPressed;
	AnimElapsed = 0.f;
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

	const float DirectionDuration = GetTargetMotionDuration(bReverseMotion);
	CurrentMoveDuration = FMath::Max(0.01f, DirectionDuration * Travel);
	bIsAnimating = true;
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
}

void AGridPressurePlateActor::InitializeRuntimeWorldObject(
	const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	InitializeRuntimePlate(ObjectData, Mesh, WorldTransform.GetLocation(), ObjectData.bInitiallyActive);
}
