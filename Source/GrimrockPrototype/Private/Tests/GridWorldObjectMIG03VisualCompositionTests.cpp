#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridWorldObjectVisual.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridMechanismActor.h"
#include "Runtime/GridPitTrapdoorActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FGridWorldObjectMIG03TestWorld
	{
		UWorld* World = nullptr;

		FGridWorldObjectMIG03TestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("WorldObjectMIG03_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridWorldObjectMIG03TestWorld()
		{
			if (!World)
			{
				return;
			}
			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};

	UStaticMeshComponent* FindStaticMeshComponentByName(AActor* Actor, FName ComponentName)
	{
		if (!Actor)
		{
			return nullptr;
		}

		TArray<UStaticMeshComponent*> Components;
		Actor->GetComponents<UStaticMeshComponent>(Components);
		for (UStaticMeshComponent* Component : Components)
		{
			if (Component && Component->GetFName() == ComponentName)
			{
				return Component;
			}
		}
		return nullptr;
	}
}

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
		TestNull(*FString::Printf(TEXT("%s legacy visual field is physically removed"), *LegacyName.ToString()),
			ArchetypeClass->FindPropertyByName(LegacyName));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG03TargetMotionStateMachinesTest,
	"Grimrock.WorldObjects.MIG03.TargetMotionStateMachines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG03TargetMotionStateMachinesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridWorldObjectMIG03TestWorld TestWorld;
	if (!TestNotNull(TEXT("MIG03 transient world exists"), TestWorld.World))
	{
		return false;
	}

	// Door: two opposite moving leaves must be driven by the same normalized open state.
	UGridObjectArchetypeAsset* DoorArchetype = NewObject<UGridObjectArchetypeAsset>(TestWorld.World);
	DoorArchetype->StaticPart.Mesh = NewObject<UStaticMesh>(DoorArchetype);
	DoorArchetype->MovingParts.Part0.Mesh = NewObject<UStaticMesh>(DoorArchetype);
	DoorArchetype->MovingParts.Part0.LocalTransform = FTransform(FRotator::ZeroRotator, FVector(10.0f, 0.0f, 0.0f));
	DoorArchetype->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Translation;
	DoorArchetype->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::X;
	DoorArchetype->MovingParts.Part0.Motion.Amount = 20.0f;
	DoorArchetype->MovingParts.Part0.Motion.Duration = 0.20f;
	DoorArchetype->MovingParts.Part1.Mesh = NewObject<UStaticMesh>(DoorArchetype);
	DoorArchetype->MovingParts.Part1.LocalTransform = FTransform(FRotator::ZeroRotator, FVector(-10.0f, 0.0f, 0.0f));
	DoorArchetype->MovingParts.Part1.Motion.Type = EGridWorldObjectMotionType::Translation;
	DoorArchetype->MovingParts.Part1.Motion.Axis = EGridWorldObjectMotionAxis::X;
	DoorArchetype->MovingParts.Part1.Motion.Amount = -20.0f;
	DoorArchetype->MovingParts.Part1.Motion.Duration = 0.20f;

	FGridLevelObjectData DoorData;
	DoorData.ObjectId = FGuid::NewGuid();
	DoorData.Type = EGridLevelObjectType::Door;
	DoorData.CellX = 1;
	DoorData.CellY = 1;
	DoorData.Edge = EGridEdge::North;
	DoorData.bInitiallyActive = false;
	DoorData.Behavior.DoorAnimation.MoveDuration = 4.0f;

	AGridDoorActor* Door = TestWorld.World->SpawnActor<AGridDoorActor>();
	if (!TestNotNull(TEXT("Target-composition door exists"), Door))
	{
		return false;
	}
	Door->InitializeMechanismVisuals(DoorData, DoorArchetype, FTransform::Identity);
	Door->InitializeGridObject(DoorData, nullptr, FTransform::Identity);
	TestEqual(TEXT("Door consumes authored Motion.Duration"), Door->MoveDuration, 0.20f);
	Door->OpenDoor();
	TestTrue(TEXT("Target door starts opening"), Door->IsAnimating());
	Door->Tick(0.20f);
	TestTrue(TEXT("Target door reaches fully open state"), Door->IsFullyOpen());

	UStaticMeshComponent* DoorPart0 = FindStaticMeshComponentByName(Door, TEXT("MovingMesh"));
	UStaticMeshComponent* DoorPart1 = FindStaticMeshComponentByName(Door, TEXT("MovingMesh1"));
	TestNotNull(TEXT("Door MovingPart[0] component exists"), DoorPart0);
	TestNotNull(TEXT("Door MovingPart[1] component exists"), DoorPart1);
	if (DoorPart0)
	{
		TestTrue(TEXT("Door MovingPart[0] reaches authored open transform"),
			DoorPart0->GetRelativeTransform().Equals(
				DoorArchetype->MovingParts.Part0.Motion.Evaluate(DoorArchetype->MovingParts.Part0.LocalTransform, 1.0f), 0.01f));
	}
	if (DoorPart1)
	{
		TestTrue(TEXT("Door MovingPart[1] reaches authored open transform"),
			DoorPart1->GetRelativeTransform().Equals(
				DoorArchetype->MovingParts.Part1.Motion.Evaluate(DoorArchetype->MovingParts.Part1.LocalTransform, 1.0f), 0.01f));
	}

	// Button: the target Motion replaces the old hard-coded press offset while hold behavior remains gameplay data.
	UGridObjectArchetypeAsset* ButtonArchetype = NewObject<UGridObjectArchetypeAsset>(TestWorld.World);
	ButtonArchetype->MovingParts.Part0.Mesh = NewObject<UStaticMesh>(ButtonArchetype);
	ButtonArchetype->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Translation;
	ButtonArchetype->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::X;
	ButtonArchetype->MovingParts.Part0.Motion.Amount = 7.0f;
	ButtonArchetype->MovingParts.Part0.Motion.Duration = 0.05f;

	FGridLevelObjectData ButtonData;
	ButtonData.ObjectId = FGuid::NewGuid();
	ButtonData.Type = EGridLevelObjectType::Button;
	ButtonData.CellX = 1;
	ButtonData.CellY = 2;
	ButtonData.Edge = EGridEdge::North;
	ButtonData.Behavior.ButtonAnimation.ButtonPressDuration = 1.0f;
	ButtonData.Behavior.ButtonAnimation.ButtonReleaseDuration = 1.0f;

	AGridButtonActor* Button = TestWorld.World->SpawnActor<AGridButtonActor>();
	TestNotNull(TEXT("Target-composition button exists"), Button);
	if (Button)
	{
		Button->InitializeMechanismVisuals(ButtonData, ButtonArchetype, FTransform::Identity);
		Button->InitializeGridObject(ButtonData, nullptr, FTransform::Identity);
		TestEqual(TEXT("Button press duration comes from Motion"), Button->PressDuration, 0.05f);
		TestEqual(TEXT("Button release duration comes from Motion"), Button->ReleaseDuration, 0.05f);
		Button->TriggerPress();
		Button->Tick(0.05f);
		if (UStaticMeshComponent* ButtonPart = FindStaticMeshComponentByName(Button, TEXT("MovingMesh")))
		{
			TestTrue(TEXT("Button reaches authored pressed transform"),
				ButtonPart->GetRelativeTransform().Equals(
					ButtonArchetype->MovingParts.Part0.Motion.Evaluate(ButtonArchetype->MovingParts.Part0.LocalTransform, 1.0f), 0.01f));
		}
		else
		{
			AddError(TEXT("Button MovingPart[0] component missing"));
		}
	}

	// Lever: target rotation and pivot own the visual state instead of LeverOffPitch/LeverOnPitch.
	UGridObjectArchetypeAsset* LeverArchetype = NewObject<UGridObjectArchetypeAsset>(TestWorld.World);
	LeverArchetype->MovingParts.Part0.Mesh = NewObject<UStaticMesh>(LeverArchetype);
	LeverArchetype->MovingParts.Part0.LocalTransform = FTransform(FRotator::ZeroRotator, FVector(12.0f, 0.0f, 0.0f));
	LeverArchetype->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Rotation;
	LeverArchetype->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::Z;
	LeverArchetype->MovingParts.Part0.Motion.Pivot = FVector::ZeroVector;
	LeverArchetype->MovingParts.Part0.Motion.Amount = 90.0f;
	LeverArchetype->MovingParts.Part0.Motion.Duration = 0.10f;

	FGridLevelObjectData LeverData;
	LeverData.ObjectId = FGuid::NewGuid();
	LeverData.Type = EGridLevelObjectType::Lever;
	LeverData.CellX = 2;
	LeverData.CellY = 1;
	LeverData.Edge = EGridEdge::East;
	LeverData.bInitiallyActive = false;
	LeverData.Behavior.LeverAnimation.ToggleDuration = 1.0f;

	AGridLeverActor* Lever = TestWorld.World->SpawnActor<AGridLeverActor>();
	TestNotNull(TEXT("Target-composition lever exists"), Lever);
	if (Lever)
	{
		Lever->InitializeMechanismVisuals(LeverData, LeverArchetype, FTransform::Identity);
		Lever->InitializeGridObject(LeverData, nullptr, FTransform::Identity);
		TestEqual(TEXT("Lever duration comes from Motion"), Lever->ToggleDuration, 0.10f);
		Lever->SetLeverState(true);
		Lever->Tick(0.10f);
		if (UStaticMeshComponent* LeverPart = FindStaticMeshComponentByName(Lever, TEXT("MovingMesh")))
		{
			TestTrue(TEXT("Lever reaches authored on transform"),
				LeverPart->GetRelativeTransform().Equals(
					LeverArchetype->MovingParts.Part0.Motion.Evaluate(LeverArchetype->MovingParts.Part0.LocalTransform, 1.0f), 0.01f));
		}
		else
		{
			AddError(TEXT("Lever MovingPart[0] component missing"));
		}
	}

	// Pressure plate: target translation owns the released/pressed visual positions.
	UGridObjectArchetypeAsset* PlateArchetype = NewObject<UGridObjectArchetypeAsset>(TestWorld.World);
	PlateArchetype->MovingParts.Part0.Mesh = NewObject<UStaticMesh>(PlateArchetype);
	PlateArchetype->MovingParts.Part0.LocalTransform = FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 4.0f));
	PlateArchetype->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Translation;
	PlateArchetype->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::Z;
	PlateArchetype->MovingParts.Part0.Motion.Amount = -3.0f;
	PlateArchetype->MovingParts.Part0.Motion.Duration = 0.07f;

	FGridLevelObjectData PlateData;
	PlateData.ObjectId = FGuid::NewGuid();
	PlateData.Type = EGridLevelObjectType::PressurePlate;
	PlateData.CellX = 2;
	PlateData.CellY = 2;
	PlateData.bInitiallyActive = false;
	PlateData.Behavior.PressurePlateAnimation.MoveDuration = 1.0f;

	AGridPressurePlateActor* Plate = TestWorld.World->SpawnActor<AGridPressurePlateActor>();
	TestNotNull(TEXT("Target-composition pressure plate exists"), Plate);
	if (Plate)
	{
		Plate->InitializeMechanismVisuals(PlateData, PlateArchetype, FTransform::Identity);
		Plate->InitializeGridObject(PlateData, nullptr, FTransform::Identity);
		TestEqual(TEXT("Pressure plate duration comes from Motion"), Plate->MoveDuration, 0.07f);
		Plate->SetPressed(true);
		Plate->Tick(0.07f);
		if (UStaticMeshComponent* PlatePart = FindStaticMeshComponentByName(Plate, TEXT("MovingMesh")))
		{
			TestTrue(TEXT("Pressure plate reaches authored pressed transform"),
				PlatePart->GetRelativeTransform().Equals(
					PlateArchetype->MovingParts.Part0.Motion.Evaluate(PlateArchetype->MovingParts.Part0.LocalTransform, 1.0f), 0.01f));
		}
		else
		{
			AddError(TEXT("Pressure plate MovingPart[0] component missing"));
		}
	}

	// Pit trapdoor: target MovingPart[0]/[1] own the two leaves and their hinge motions.
	UGridObjectArchetypeAsset* PitArchetype = NewObject<UGridObjectArchetypeAsset>(TestWorld.World);
	PitArchetype->StaticPart.Mesh = NewObject<UStaticMesh>(PitArchetype);
	PitArchetype->MovingParts.Part0.Mesh = NewObject<UStaticMesh>(PitArchetype);
	PitArchetype->MovingParts.Part0.LocalTransform = FTransform(FRotator::ZeroRotator, FVector(-40.0f, 0.0f, 0.0f));
	PitArchetype->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Rotation;
	PitArchetype->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::Y;
	PitArchetype->MovingParts.Part0.Motion.Pivot = FVector(-85.0f, 0.0f, -5.0f);
	PitArchetype->MovingParts.Part0.Motion.Amount = -80.0f;
	PitArchetype->MovingParts.Part0.Motion.Duration = 0.30f;
	PitArchetype->MovingParts.Part1.Mesh = NewObject<UStaticMesh>(PitArchetype);
	PitArchetype->MovingParts.Part1.LocalTransform = FTransform(FRotator::ZeroRotator, FVector(40.0f, 0.0f, 0.0f));
	PitArchetype->MovingParts.Part1.Motion.Type = EGridWorldObjectMotionType::Rotation;
	PitArchetype->MovingParts.Part1.Motion.Axis = EGridWorldObjectMotionAxis::Y;
	PitArchetype->MovingParts.Part1.Motion.Pivot = FVector(85.0f, 0.0f, -5.0f);
	PitArchetype->MovingParts.Part1.Motion.Amount = 80.0f;
	PitArchetype->MovingParts.Part1.Motion.Duration = 0.30f;

	FGridLevelObjectData PitData;
	PitData.ObjectId = FGuid::NewGuid();
	PitData.Type = EGridLevelObjectType::Pit;
	PitData.CellX = 3;
	PitData.CellY = 3;
	PitData.Behavior.Pit.bInitiallyOpen = false;
	PitData.Behavior.PitAnimation.MoveDuration = 1.50f;

	AGridPitTrapdoorActor* Pit = TestWorld.World->SpawnActor<AGridPitTrapdoorActor>();
	TestNotNull(TEXT("Target-composition pit trapdoor exists"), Pit);
	if (Pit)
	{
		Pit->InitializeMechanismVisuals(PitData, PitArchetype, FTransform::Identity);
		Pit->InitializeGridObject(PitData, nullptr, FTransform::Identity);
		TestTrue(TEXT("Target pit recognizes its two-part cover"), Pit->HasCompleteTrapdoorCover());
		TestEqual(TEXT("Pit duration comes from MovingParts Motion"), Pit->MoveDuration, 0.30f);
		TestTrue(TEXT("Pit left pivot comes from MovingPart[0] Motion"), Pit->GetLeftHingeLocation().Equals(PitArchetype->MovingParts.Part0.Motion.Pivot));
		TestTrue(TEXT("Pit right pivot comes from MovingPart[1] Motion"), Pit->GetRightHingeLocation().Equals(PitArchetype->MovingParts.Part1.Motion.Pivot));
		Pit->SetPitOpenVisualState(true, false);
		TestTrue(TEXT("Target pit starts opening"), Pit->IsAnimating());
		Pit->Tick(0.30f);
		TestTrue(TEXT("Target pit reaches open state"), Pit->IsPitOpenVisualState());

		UStaticMeshComponent* PitPart0 = FindStaticMeshComponentByName(Pit, TEXT("MovingMesh"));
		UStaticMeshComponent* PitPart1 = FindStaticMeshComponentByName(Pit, TEXT("MovingMesh1"));
		TestNotNull(TEXT("Pit MovingPart[0] component exists"), PitPart0);
		TestNotNull(TEXT("Pit MovingPart[1] component exists"), PitPart1);
		if (PitPart0)
		{
			TestTrue(TEXT("Pit MovingPart[0] reaches authored open transform"),
				PitPart0->GetRelativeTransform().Equals(
					PitArchetype->MovingParts.Part0.Motion.Evaluate(PitArchetype->MovingParts.Part0.LocalTransform, 1.0f), 0.01f));
		}
		if (PitPart1)
		{
			TestTrue(TEXT("Pit MovingPart[1] reaches authored open transform"),
				PitPart1->GetRelativeTransform().Equals(
					PitArchetype->MovingParts.Part1.Motion.Evaluate(PitArchetype->MovingParts.Part1.LocalTransform, 1.0f), 0.01f));
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
