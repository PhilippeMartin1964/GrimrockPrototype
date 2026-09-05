#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridWorldObjectVisual.h"
#include "Engine/StaticMesh.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridMechanismActor.h"
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
	UScriptStruct* MovingPartsStruct = FGridWorldObjectMovingParts::StaticStruct();

	TestNotNull(TEXT("StaticPart struct exists"), StaticPartStruct);
	TestNotNull(TEXT("Motion struct exists"), MotionStruct);
	TestNotNull(TEXT("MovingPart struct exists"), MovingPartStruct);
	TestNotNull(TEXT("MovingParts container exists"), MovingPartsStruct);

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

	if (MovingPartsStruct)
	{
		TestNotNull(TEXT("MovingParts exposes Part0"), MovingPartsStruct->FindPropertyByName(TEXT("Part0")));
		TestNotNull(TEXT("MovingParts exposes Part1"), MovingPartsStruct->FindPropertyByName(TEXT("Part1")));
		TestNull(TEXT("MovingParts cannot expose a third part"), MovingPartsStruct->FindPropertyByName(TEXT("Part2")));
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

	FGridWorldObjectMotion TranslationMotion;
	TranslationMotion.Type = EGridWorldObjectMotionType::Translation;
	TranslationMotion.Axis = EGridWorldObjectMotionAxis::Z;
	TranslationMotion.Amount = 20.0f;
	const FTransform TranslationBase(FRotator::ZeroRotator, FVector(1.0f, 2.0f, 3.0f));
	const FTransform TranslationHalf = TranslationMotion.Evaluate(TranslationBase, 0.5f);
	TestTrue(TEXT("Translation motion evaluates from the authored local transform"),
		TranslationHalf.GetLocation().Equals(FVector(1.0f, 2.0f, 13.0f), KINDA_SMALL_NUMBER));

	FGridWorldObjectMotion RotationMotion;
	RotationMotion.Type = EGridWorldObjectMotionType::Rotation;
	RotationMotion.Axis = EGridWorldObjectMotionAxis::Z;
	RotationMotion.Pivot = FVector::ZeroVector;
	RotationMotion.Amount = 90.0f;
	const FTransform RotationBase(FRotator::ZeroRotator, FVector(10.0f, 0.0f, 0.0f));
	const FTransform RotationOpen = RotationMotion.Evaluate(RotationBase, 1.0f);
	TestTrue(TEXT("Rotation motion orbits around the local pivot"),
		RotationOpen.GetLocation().Equals(FVector(0.0f, 10.0f, 0.0f), 0.01f));
	TestTrue(TEXT("Rotation motion rotates the visual part"),
		RotationOpen.GetRotation().Rotator().Equals(FRotator(0.0f, 90.0f, 0.0f), 0.01f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG03ArchetypeVisualContractTest,
	"Grimrock.WorldObjects.MIG03.ArchetypeVisualContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG03ArchetypeVisualContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ArchetypeClass = UGridObjectArchetypeAsset::StaticClass();
	if (!TestNotNull(TEXT("World object archetype class exists"), ArchetypeClass))
	{
		return false;
	}

	FProperty* StaticPartProperty = ArchetypeClass->FindPropertyByName(TEXT("StaticPart"));
	FProperty* MovingPartsProperty = ArchetypeClass->FindPropertyByName(TEXT("MovingParts"));
	TestNotNull(TEXT("Archetype exposes StaticPart"), StaticPartProperty);
	TestNotNull(TEXT("Archetype exposes MovingParts"), MovingPartsProperty);

	if (StaticPartProperty)
	{
		TestEqual(TEXT("StaticPart belongs to the target visual composition category"), StaticPartProperty->GetMetaData(TEXT("Category")),
			FString(TEXT("Visual|Composition")));
		TestTrue(TEXT("StaticPart is editable authoring data"), StaticPartProperty->HasAnyPropertyFlags(CPF_Edit));
	}
	if (MovingPartsProperty)
	{
		TestEqual(TEXT("MovingParts belongs to the target visual composition category"), MovingPartsProperty->GetMetaData(TEXT("Category")),
			FString(TEXT("Visual|Composition")));
		TestTrue(TEXT("MovingParts is editable authoring data"), MovingPartsProperty->HasAnyPropertyFlags(CPF_Edit));
	}

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>(GetTransientPackage());
	if (!TestNotNull(TEXT("Transient archetype exists"), Archetype))
	{
		return false;
	}

	TestEqual(TEXT("New archetype has zero moving parts"), Archetype->GetDefinedMovingPartCount(), 0);

	UStaticMesh* Mesh0 = NewObject<UStaticMesh>(Archetype);
	UStaticMesh* Mesh1 = NewObject<UStaticMesh>(Archetype);
	Archetype->MovingParts.Part0.Mesh = Mesh0;
	TestEqual(TEXT("One defined slot means one moving part"), Archetype->GetDefinedMovingPartCount(), 1);
	Archetype->MovingParts.Part1.Mesh = Mesh1;
	TestEqual(TEXT("Two defined slots means two moving parts"), Archetype->GetDefinedMovingPartCount(), 2);

	TestNull(TEXT("No third moving-part authoring slot exists"), FGridWorldObjectMovingParts::StaticStruct()->FindPropertyByName(TEXT("Part2")));
	TestNull(TEXT("No MaxMovingParts parameter exists"), ArchetypeClass->FindPropertyByName(TEXT("MaxMovingParts")));

	const FName LegacyVisualNames[] = {TEXT("PreviewMesh"), TEXT("FixedMesh"), TEXT("MovingMesh"), TEXT("PitLeftLeafMesh"), TEXT("PitRightLeafMesh")};
	for (const FName LegacyName : LegacyVisualNames)
	{
		FProperty* LegacyProperty = ArchetypeClass->FindPropertyByName(LegacyName);
		TestNotNull(*FString::Printf(TEXT("%s remains only for MIG03 runtime rewiring"), *LegacyName.ToString()), LegacyProperty);
		if (LegacyProperty)
		{
			TestTrue(*FString::Printf(TEXT("%s is explicitly categorized as a temporary legacy runtime bridge"), *LegacyName.ToString()),
				LegacyProperty->GetMetaData(TEXT("Category")).StartsWith(TEXT("Visual|Legacy Runtime Bridge")));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG03RuntimeConsumerContractTest,
	"Grimrock.WorldObjects.MIG03.RuntimeConsumerContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG03RuntimeConsumerContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* MechanismClass = AGridMechanismActor::StaticClass();
	UClass* PreviewClass = AGridEditorPreviewObjectActor::StaticClass();
	TestNotNull(TEXT("Mechanism runtime class exists"), MechanismClass);
	TestNotNull(TEXT("Editor preview object class exists"), PreviewClass);

	if (MechanismClass)
	{
		TestNotNull(TEXT("Mechanism exposes primary moving visual component"), MechanismClass->FindPropertyByName(TEXT("MovingMeshComponent")));
		TestNotNull(TEXT("Mechanism exposes the second target moving visual component"),
			MechanismClass->FindPropertyByName(TEXT("SecondaryMovingMeshComponent")));
	}

	if (PreviewClass)
	{
		TestNotNull(TEXT("Preview exposes target MovingPart0 component"), PreviewClass->FindPropertyByName(TEXT("MovingPart0MeshComponent")));
		TestNotNull(TEXT("Preview exposes target MovingPart1 component"), PreviewClass->FindPropertyByName(TEXT("MovingPart1MeshComponent")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
