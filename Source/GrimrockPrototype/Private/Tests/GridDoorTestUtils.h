#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/StaticMesh.h"
#include "Runtime/GridDoorActor.h"

namespace GridDoorTestUtils
{
	inline UGridWorldObjectDefinitionAsset* InitializeDoorFromMotion(
		AGridDoorActor* Door,
		const FGridWorldObjectInstance& ObjectData,
		UObject* Outer,
		float MotionDuration,
		float MotionAmount = 180.0f,
		UGridWorldObjectDefinitionAsset* Definition = nullptr)
	{
		if (!Door || !Outer)
		{
			return nullptr;
		}

		if (!Definition)
		{
			Definition = NewObject<UGridWorldObjectDefinitionAsset>(Outer);
		}
		Definition->SupportedType = EGridLevelObjectType::Door;

		FGridWorldObjectMovingPart& MovingPart = Definition->MovingParts.Part0;
		if (!MovingPart.Mesh)
		{
			MovingPart.Mesh = NewObject<UStaticMesh>(Definition);
		}
		MovingPart.Motion.Type = EGridWorldObjectMotionType::Translation;
		MovingPart.Motion.Axis = EGridWorldObjectMotionAxis::Z;
		MovingPart.Motion.Amount = MotionAmount;
		MovingPart.Motion.Duration = FMath::Max(0.0f, MotionDuration);

		const FGridRuntimeWorldObjectData RuntimeData(ObjectData);
		Door->InitializeRuntimeMechanismVisuals(RuntimeData, Definition, FTransform::Identity);
		Door->InitializeRuntimeWorldObject(RuntimeData, nullptr, FTransform::Identity);
		return Definition;
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
