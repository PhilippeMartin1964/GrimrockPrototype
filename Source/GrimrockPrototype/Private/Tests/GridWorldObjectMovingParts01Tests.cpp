#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridPitTrapdoorActor.h"
#include "Runtime/GridRuntimeWorldObjectData.h"
#include "UObject/UnrealType.h"

namespace GridMovingParts01
{
	struct FTestWorld
	{
		UWorld* World = nullptr;
		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false).CreateNavigation(false)
				.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("MovingParts01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			}
		}
		~FTestWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) GEngine->DestroyWorldContext(World);
			}
		}
	};

	UStaticMeshComponent* FindPart(AActor* Actor, int32 Index)
	{
		if (!Actor) return nullptr;
		const FName Name = Index == 0 ? FName(TEXT("MovingMesh")) : FName(*FString::Printf(TEXT("MovingPart%d"), Index));
		TArray<UStaticMeshComponent*> Components;
		Actor->GetComponents(Components);
		for (UStaticMeshComponent* Component : Components)
		{
			if (Component && Component->GetFName() == Name) return Component;
		}
		return nullptr;
	}

	void ConfigureTranslation(FGridWorldObjectMovingPart& Part, UObject* Outer, float BaseX, float Amount, float Duration)
	{
		Part.Mesh = NewObject<UStaticMesh>(Outer);
		Part.LocalTransform = FTransform(FVector(BaseX, 0.0f, 0.0f));
		Part.Motion.Type = EGridWorldObjectMotionType::Translation;
		Part.Motion.Axis = EGridWorldObjectMotionAxis::X;
		Part.Motion.Amount = Amount;
		Part.Motion.Duration = Duration;
	}

	FGridWorldObjectInstance MakeInstance(EGridLevelObjectType Type)
	{
		FGridWorldObjectInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.Type = Type;
		return Instance;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridWorldObjectMovingParts01Test, "Grimrock.WorldObjects.MOVINGPARTS01",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMovingParts01Test::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMovingParts01;

	FProperty* MovingPartsProperty = UGridWorldObjectDefinitionAsset::StaticClass()->FindPropertyByName(TEXT("MovingParts"));
	FArrayProperty* MovingPartsArray = CastField<FArrayProperty>(MovingPartsProperty);
	TestNotNull(TEXT("MovingParts is an authorable FArrayProperty"), MovingPartsArray);
	if (MovingPartsProperty)
	{
		TestTrue(TEXT("MovingParts is editable"), MovingPartsProperty->HasAnyPropertyFlags(CPF_Edit));
		TestEqual(TEXT("MovingParts uses Visual|Composition"), MovingPartsProperty->GetMetaData(TEXT("Category")), FString(TEXT("Visual|Composition")));
	}
	TestNull(TEXT("Legacy Part0 property is absent"), UGridWorldObjectDefinitionAsset::StaticClass()->FindPropertyByName(TEXT("Part0")));
	TestNull(TEXT("Legacy Part1 property is absent"), UGridWorldObjectDefinitionAsset::StaticClass()->FindPropertyByName(TEXT("Part1")));
	TestNull(TEXT("Legacy FGridWorldObjectMovingParts reflected contract is absent"),
		FindObject<UScriptStruct>(nullptr, TEXT("/Script/GrimrockPrototype.GridWorldObjectMovingParts")));
	TestNull(TEXT("No MaxMovingParts property exists"), UGridWorldObjectDefinitionAsset::StaticClass()->FindPropertyByName(TEXT("MaxMovingParts")));

	UGridWorldObjectDefinitionAsset* Cardinality = NewObject<UGridWorldObjectDefinitionAsset>(GetTransientPackage());
	TestEqual(TEXT("Zero parts are supported"), Cardinality->MovingParts.Num(), 0);
	Cardinality->MovingParts.SetNum(1);
	ConfigureTranslation(Cardinality->MovingParts[0], Cardinality, 0.0f, 1.0f, 1.0f);
	TestEqual(TEXT("One part is supported"), Cardinality->MovingParts.Num(), 1);
	Cardinality->MovingParts.SetNum(2);
	ConfigureTranslation(Cardinality->MovingParts[1], Cardinality, 0.0f, 2.0f, 2.0f);
	TestEqual(TEXT("Two parts are supported"), Cardinality->MovingParts.Num(), 2);
	Cardinality->MovingParts.SetNum(3);
	ConfigureTranslation(Cardinality->MovingParts[2], Cardinality, 2.0f, 30.0f, 3.0f);
	TestEqual(TEXT("At least three parts are supported"), Cardinality->MovingParts.Num(), 3);
	TestNotNull(TEXT("Part 2 has a mesh"), Cardinality->MovingParts[2].Mesh.Get());
	TestTrue(TEXT("Part 2 keeps LocalTransform"), Cardinality->MovingParts[2].LocalTransform.GetLocation().Equals(FVector(2.0f, 0.0f, 0.0f)));
	TestEqual(TEXT("Part 2 keeps Motion"), Cardinality->MovingParts[2].Motion.Amount, 30.0f);

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Test world exists"), TestWorld.World)) return false;

	UGridWorldObjectDefinitionAsset* DoorDefinition = NewObject<UGridWorldObjectDefinitionAsset>(TestWorld.World);
	DoorDefinition->MovingParts = Cardinality->MovingParts;
	FGridWorldObjectInstance DoorInstance = MakeInstance(EGridLevelObjectType::Door);
	FGridWorldObjectMovingPartInstanceOverride Part2Override;
	Part2Override.PartIndex = 2;
	Part2Override.bOverrideMotionAmount = true;
	Part2Override.MotionAmount = 40.0f;
	Part2Override.bOverrideMotionDuration = true;
	Part2Override.MotionDuration = 4.0f;
	DoorInstance.InstanceConfig.MovingPartOverrides.Add(Part2Override);
	AGridDoorActor* Door = TestWorld.World->SpawnActor<AGridDoorActor>();
	const FGridRuntimeWorldObjectData DoorData(DoorInstance);
	Door->InitializeRuntimeMechanismVisuals(DoorData, DoorDefinition, FTransform::Identity);
	Door->InitializeRuntimeWorldObject(DoorData, nullptr, FTransform::Identity);
	UStaticMeshComponent* DoorPart2 = FindPart(Door, 2);
	TestNotNull(TEXT("Runtime creates component 2"), DoorPart2);
	TestEqual(TEXT("Duration considers part 2 override"), Door->MoveDuration, 4.0f);
	Door->OpenDoor();
	Door->Tick(4.0f);
	if (DoorPart2)
	{
		TestTrue(TEXT("ApplyAll animates part 2 with its sparse override"),
			DoorPart2->GetRelativeLocation().Equals(FVector(42.0f, 0.0f, 0.0f), 0.01f));
	}

	AGridEditorPreviewObjectActor* Preview = TestWorld.World->SpawnActor<AGridEditorPreviewObjectActor>();
	Preview->InitializePreviewObjectFromDefinition(DoorInstance.InstanceId, DoorInstance.Type, DoorDefinition, &DoorInstance.InstanceConfig);
	UStaticMeshComponent* PreviewPart2 = nullptr;
	TArray<UStaticMeshComponent*> PreviewComponents;
	Preview->GetComponents(PreviewComponents);
	for (UStaticMeshComponent* Component : PreviewComponents)
	{
		if (Component && Component->GetFName() == TEXT("MovingPart2")) PreviewPart2 = Component;
	}
	TestNotNull(TEXT("Preview creates component 2"), PreviewPart2);
	Preview->SetSelected(true);
	if (PreviewPart2)
	{
		TestTrue(TEXT("Preview selection reaches component 2"), PreviewPart2->bRenderCustomDepth);
		TestEqual(TEXT("Preview component 2 gets selected stencil"), PreviewPart2->CustomDepthStencilValue, 2);
	}

	for (EGridLevelObjectType Type : {EGridLevelObjectType::Button, EGridLevelObjectType::Lever})
	{
		UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(TestWorld.World);
		Definition->MovingParts.SetNum(3);
		ConfigureTranslation(Definition->MovingParts[0], Definition, 0.0f, 5.0f, 0.1f);
		ConfigureTranslation(Definition->MovingParts[1], Definition, 10.0f, 10.0f, 1.0f);
		ConfigureTranslation(Definition->MovingParts[2], Definition, 20.0f, 20.0f, 9.0f);
		const FGridWorldObjectInstance Instance = MakeInstance(Type);
		const FGridRuntimeWorldObjectData Data(Instance);
		if (Type == EGridLevelObjectType::Button)
		{
			AGridButtonActor* Button = TestWorld.World->SpawnActor<AGridButtonActor>();
			Button->InitializeRuntimeMechanismVisuals(Data, Definition, FTransform::Identity);
			Button->InitializeRuntimeWorldObject(Data, nullptr, FTransform::Identity);
			TestEqual(TEXT("Button duration remains index 0"), Button->PressDuration, 0.1f);
			Button->TriggerPress();
			Button->Tick(0.1f);
			TestTrue(TEXT("Button leaves part 2 at rest"), FindPart(Button, 2)->GetRelativeLocation().Equals(FVector(20.0f, 0.0f, 0.0f)));
		}
		else
		{
			AGridLeverActor* Lever = TestWorld.World->SpawnActor<AGridLeverActor>();
			Lever->InitializeRuntimeMechanismVisuals(Data, Definition, FTransform::Identity);
			Lever->InitializeRuntimeWorldObject(Data, nullptr, FTransform::Identity);
			TestEqual(TEXT("Lever duration remains index 0"), Lever->ToggleDuration, 0.1f);
			Lever->SetLeverState(true);
			Lever->Tick(0.1f);
			TestTrue(TEXT("Lever leaves part 2 at rest"), FindPart(Lever, 2)->GetRelativeLocation().Equals(FVector(20.0f, 0.0f, 0.0f)));
		}
	}

	UGridWorldObjectDefinitionAsset* PitDefinition = NewObject<UGridWorldObjectDefinitionAsset>(TestWorld.World);
	PitDefinition->MovingParts.SetNum(3);
	ConfigureTranslation(PitDefinition->MovingParts[0], PitDefinition, 0.0f, 1.0f, 0.0f);
	ConfigureTranslation(PitDefinition->MovingParts[1], PitDefinition, 10.0f, 1.0f, 0.0f);
	ConfigureTranslation(PitDefinition->MovingParts[2], PitDefinition, 20.0f, 50.0f, 9.0f);
	FGridWorldObjectInstance PitInstance = MakeInstance(EGridLevelObjectType::Pit);
	PitInstance.InstanceConfig.Pit.bInitiallyOpen = false;
	const FGridRuntimeWorldObjectData PitData(PitInstance);
	AGridPitTrapdoorActor* Pit = TestWorld.World->SpawnActor<AGridPitTrapdoorActor>();
	Pit->InitializeRuntimeMechanismVisuals(PitData, PitDefinition, FTransform::Identity);
	Pit->InitializeRuntimeWorldObject(PitData, nullptr, FTransform::Identity);
	TestTrue(TEXT("Pit cover is exactly indexes 0 and 1"), Pit->HasCompleteTrapdoorCover());
	TestEqual(TEXT("Pit duration ignores index 2"), Pit->MoveDuration, 0.0f);
	Pit->SetPitOpenVisualState(true, false);
	TestTrue(TEXT("Pit never animates index 2"), FindPart(Pit, 2)->GetRelativeLocation().Equals(FVector(20.0f, 0.0f, 0.0f)));

	return true;
}

#endif
