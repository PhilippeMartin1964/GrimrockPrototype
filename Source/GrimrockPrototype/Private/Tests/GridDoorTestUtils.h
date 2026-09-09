#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/StaticMesh.h"
#include "Runtime/GridDoorActor.h"

namespace GridDoorTestUtils
{
	inline UGridObjectArchetypeAsset* InitializeDoorFromMotion(
		AGridDoorActor* Door,
		const FGridWorldObjectInstance& ObjectData,
		UObject* Outer,
		float MotionDuration,
		float MotionAmount = 180.0f,
		UGridObjectArchetypeAsset* Archetype = nullptr)
	{
		if (!Door || !Outer)
		{
			return nullptr;
		}

		if (!Archetype)
		{
			Archetype = NewObject<UGridObjectArchetypeAsset>(Outer);
		}
		Archetype->SupportedType = EGridLevelObjectType::Door;

		FGridWorldObjectMovingPart& MovingPart = Archetype->MovingParts.Part0;
		if (!MovingPart.Mesh)
		{
			MovingPart.Mesh = NewObject<UStaticMesh>(Archetype);
		}
		MovingPart.Motion.Type = EGridWorldObjectMotionType::Translation;
		MovingPart.Motion.Axis = EGridWorldObjectMotionAxis::Z;
		MovingPart.Motion.Amount = MotionAmount;
		MovingPart.Motion.Duration = FMath::Max(0.0f, MotionDuration);

		const FGridRuntimeWorldObjectData RuntimeData(ObjectData);
		Door->InitializeRuntimeMechanismVisuals(RuntimeData, Archetype, FTransform::Identity);
		Door->InitializeRuntimeWorldObject(RuntimeData, nullptr, FTransform::Identity);
		return Archetype;
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
