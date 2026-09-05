#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridWorldObjectVisual.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG03VisualCompositionTypesTest,
	"Grimrock.WorldObjects.MIG03.VisualCompositionTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG03VisualCompositionTypesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* StaticPartStruct = FGridWorldObjectStaticPart::StaticStruct();
	UScriptStruct* MotionStruct = FGridWorldObjectMotion::StaticStruct();
	UScriptStruct* MovingPartStruct = FGridWorldObjectMovingPart::StaticStruct();

	TestNotNull(TEXT("StaticPart struct exists"), StaticPartStruct);
	TestNotNull(TEXT("Motion struct exists"), MotionStruct);
	TestNotNull(TEXT("MovingPart struct exists"), MovingPartStruct);

	if (StaticPartStruct)
	{
		TestNotNull(TEXT("StaticPart exposes Mesh"), StaticPartStruct->FindPropertyByName(TEXT("Mesh")));
		TestNotNull(TEXT("StaticPart exposes LocalTransform"), StaticPartStruct->FindPropertyByName(TEXT("LocalTransform")));
	}

	if (MotionStruct)
	{
		TestNotNull(TEXT("Motion exposes Type"), MotionStruct->FindPropertyByName(TEXT("Type")));
		TestNotNull(TEXT("Motion exposes Axis"), MotionStruct->FindPropertyByName(TEXT("Axis")));
		TestNotNull(TEXT("Motion exposes Pivot"), MotionStruct->FindPropertyByName(TEXT("Pivot")));
		TestNotNull(TEXT("Motion exposes Amount"), MotionStruct->FindPropertyByName(TEXT("Amount")));
		TestNotNull(TEXT("Motion exposes Duration"), MotionStruct->FindPropertyByName(TEXT("Duration")));
	}

	if (MovingPartStruct)
	{
		TestNotNull(TEXT("MovingPart exposes Mesh"), MovingPartStruct->FindPropertyByName(TEXT("Mesh")));
		TestNotNull(TEXT("MovingPart exposes LocalTransform"), MovingPartStruct->FindPropertyByName(TEXT("LocalTransform")));
		TestNotNull(TEXT("MovingPart exposes Motion"), MovingPartStruct->FindPropertyByName(TEXT("Motion")));
	}

	const FGridWorldObjectStaticPart StaticPart;
	TestFalse(TEXT("StaticPart is optional when Mesh is null"), StaticPart.IsDefined());
	TestTrue(TEXT("StaticPart LocalTransform defaults to identity"), StaticPart.LocalTransform.Equals(FTransform::Identity));

	const FGridWorldObjectMovingPart MovingPart;
	TestFalse(TEXT("MovingPart is absent when Mesh is null"), MovingPart.IsDefined());
	TestTrue(TEXT("MovingPart LocalTransform defaults to identity"), MovingPart.LocalTransform.Equals(FTransform::Identity));
	TestEqual(TEXT("Default motion axis is Z"), MovingPart.Motion.Axis, EGridWorldObjectMotionAxis::Z);
	TestEqual(TEXT("Default motion amount is zero"), MovingPart.Motion.Amount, 0.0f);
	TestEqual(TEXT("Default motion duration is zero"), MovingPart.Motion.Duration, 0.0f);

	const UEnum* MotionTypeEnum = StaticEnum<EGridWorldObjectMotionType>();
	const UEnum* MotionAxisEnum = StaticEnum<EGridWorldObjectMotionAxis>();
	TestNotNull(TEXT("Motion type enum exists"), MotionTypeEnum);
	TestNotNull(TEXT("Motion axis enum exists"), MotionAxisEnum);

	if (MotionTypeEnum)
	{
		TestTrue(TEXT("Rotation motion exists"), MotionTypeEnum->IsValidEnumValue(static_cast<int64>(EGridWorldObjectMotionType::Rotation)));
		TestTrue(TEXT("Translation motion exists"), MotionTypeEnum->IsValidEnumValue(static_cast<int64>(EGridWorldObjectMotionType::Translation)));
	}
	if (MotionAxisEnum)
	{
		TestTrue(TEXT("X axis exists"), MotionAxisEnum->IsValidEnumValue(static_cast<int64>(EGridWorldObjectMotionAxis::X)));
		TestTrue(TEXT("Y axis exists"), MotionAxisEnum->IsValidEnumValue(static_cast<int64>(EGridWorldObjectMotionAxis::Y)));
		TestTrue(TEXT("Z axis exists"), MotionAxisEnum->IsValidEnumValue(static_cast<int64>(EGridWorldObjectMotionAxis::Z)));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
