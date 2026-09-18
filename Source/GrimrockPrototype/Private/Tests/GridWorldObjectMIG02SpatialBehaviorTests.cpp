#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"

#include "Core/GridBoundary.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG02SpatialBehaviorSchemaTest,
	"Grimrock.WorldObjects.MIG02.SpatialBehaviorSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG02SpatialBehaviorSchemaTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* DefinitionClass = UGridWorldObjectDefinitionAsset::StaticClass();
	if (!TestNotNull(TEXT("World object definition class exists"), DefinitionClass))
	{
		return false;
	}

	FProperty* BlocksMovement = DefinitionClass->FindPropertyByName(TEXT("bBlocksMovement"));
	FProperty* OccupiesBoundary = DefinitionClass->FindPropertyByName(TEXT("bOccupiesBoundary"));
	FProperty* ReplacesWall = DefinitionClass->FindPropertyByName(TEXT("bReplacesStandardWall"));
	TestNotNull(TEXT("Blocks Cell Movement authoring property exists"), BlocksMovement);
	TestNotNull(TEXT("Occupies Boundary authoring property exists"), OccupiesBoundary);
	TestNotNull(TEXT("Suppress Base Wall authoring property exists"), ReplacesWall);

	if (BlocksMovement)
	{
		TestEqual(TEXT("Blocks Movement is exposed as Blocks Cell Movement"), BlocksMovement->GetMetaData(TEXT("DisplayName")), FString(TEXT("Blocks Cell Movement")));
		TestEqual(TEXT("Blocks Cell Movement belongs to Spatial Behavior"), BlocksMovement->GetMetaData(TEXT("Category")), FString(TEXT("Spatial Behavior")));
		TestTrue(TEXT("Blocks Cell Movement is editable"), BlocksMovement->HasAnyPropertyFlags(CPF_Edit));
	}
	if (OccupiesBoundary)
	{
		TestEqual(TEXT("Occupies Boundary display name"), OccupiesBoundary->GetMetaData(TEXT("DisplayName")), FString(TEXT("Occupies Boundary")));
		TestTrue(TEXT("Occupies Boundary is editable"), OccupiesBoundary->HasAnyPropertyFlags(CPF_Edit));
	}
	if (ReplacesWall)
	{
		TestEqual(TEXT("Replaces Standard Wall is exposed as Suppress Base Wall"), ReplacesWall->GetMetaData(TEXT("DisplayName")), FString(TEXT("Suppress Base Wall")));
		TestTrue(TEXT("Suppress Base Wall is editable"), ReplacesWall->HasAnyPropertyFlags(CPF_Edit));
	}

	int32 EditableSpatialPropertyCount = 0;
	for (TFieldIterator<FProperty> It(DefinitionClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const FProperty* Property = *It;
		if (Property && Property->HasAnyPropertyFlags(CPF_Edit) && Property->GetMetaData(TEXT("Category")).StartsWith(TEXT("Spatial Behavior")))
		{
			++EditableSpatialPropertyCount;
		}
	}
	TestEqual(TEXT("Exactly three spatial behavior authoring parameters remain"), EditableSpatialPropertyCount, 3);

	const FName LegacyCellSharingName(*(FString(TEXT("bCan")) + TEXT("ShareCell")));
	const FName LegacyAnchorSharingName(*(FString(TEXT("bCan")) + TEXT("ShareAnchor")));
	TestNull(TEXT("Legacy cell-sharing property is removed"), DefinitionClass->FindPropertyByName(LegacyCellSharingName));
	TestNull(TEXT("Legacy anchor-sharing property is removed"), DefinitionClass->FindPropertyByName(LegacyAnchorSharingName));

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>();
	TestFalse(TEXT("Default does not block cell movement"), Definition->BlocksCellMovement());
	TestFalse(TEXT("Default does not occupy a boundary"), Definition->OccupiesBoundary());
	TestFalse(TEXT("Default does not suppress a base wall"), Definition->SuppressesBaseWall());

	Definition->bBlocksMovement = true;
	Definition->bOccupiesBoundary = true;
	Definition->bReplacesStandardWall = true;
	TestTrue(TEXT("BlocksCellMovement semantic accessor reflects authoring data"), Definition->BlocksCellMovement());
	TestTrue(TEXT("OccupiesBoundary semantic accessor reflects authoring data"), Definition->OccupiesBoundary());
	TestTrue(TEXT("SuppressesBaseWall semantic accessor reflects authoring data"), Definition->SuppressesBaseWall());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG02BoundaryKeyTest,
	"Grimrock.WorldObjects.MIG02.BoundaryKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG02BoundaryKeyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridBoundaryKey North = FGridBoundaryKey::MakeCanonical(3, 4, EGridEdge::North);
	const FGridBoundaryKey SameFromSouth = FGridBoundaryKey::MakeCanonical(3, 5, EGridEdge::South);
	TestTrue(TEXT("North of a cell equals South of its northern neighbour"), North == SameFromSouth);
	TestTrue(TEXT("North canonical boundary is valid"), North.IsValid());

	const FGridBoundaryKey East = FGridBoundaryKey::MakeCanonical(3, 4, EGridEdge::East);
	const FGridBoundaryKey SameFromWest = FGridBoundaryKey::MakeCanonical(4, 4, EGridEdge::West);
	TestTrue(TEXT("East of a cell equals West of its eastern neighbour"), East == SameFromWest);
	TestTrue(TEXT("East canonical boundary is valid"), East.IsValid());

	TestFalse(TEXT("Different physical boundaries remain different"), North == East);

	const FGridBoundaryKey None = FGridBoundaryKey::MakeCanonical(3, 4, EGridEdge::None);
	TestFalse(TEXT("None does not form a valid boundary"), None.IsValid());

	TSet<FGridBoundaryKey> Boundaries;
	Boundaries.Add(North);
	Boundaries.Add(SameFromSouth);
	Boundaries.Add(East);
	Boundaries.Add(SameFromWest);
	TestEqual(TEXT("Canonical hashing deduplicates opposite descriptions"), Boundaries.Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG021DefinitionValidationTest,
	"Grimrock.WorldObjects.MIG02.1.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG021DefinitionValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>();
	Definition->DefinitionId = TEXT("MIG02_1_Test");
	Definition->SupportedType = EGridLevelObjectType::Decoration;
	Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
	Definition->bOccupiesBoundary = false;
	Definition->bReplacesStandardWall = true;

	TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
	TestFalse(TEXT("Suppress Base Wall without Occupies Boundary is invalid"), Definition->ValidateDefinition(Messages));
	TestTrue(TEXT("Modern validation reports missing boundary ownership"), Messages.ContainsByPredicate([](const FGridWorldObjectDefinitionValidationMessage& Message)
	{
		return Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Error && Message.Message.Contains(TEXT("Suppress Base Wall requires Occupies Boundary"));
	}));

	Definition->bOccupiesBoundary = true;
	Messages.Reset();
	TestTrue(TEXT("Wall replacement with boundary ownership has no validation errors"), Definition->ValidateDefinition(Messages));

	const FString LegacyAnchorPhrase = FString(TEXT("Can Share")) + TEXT(" Anchor");
	TestFalse(TEXT("Validation no longer emits legacy anchor-sharing messages"), Messages.ContainsByPredicate([&LegacyAnchorPhrase](const FGridWorldObjectDefinitionValidationMessage& Message)
	{
		return Message.Message.Contains(LegacyAnchorPhrase);
	}));

	Definition->SupportedType = EGridLevelObjectType::Door;
	Definition->bOccupiesBoundary = false;
	Messages.Reset();
	Definition->ValidateDefinition(Messages);
	TestTrue(TEXT("Door requires canonical boundary ownership"), Messages.ContainsByPredicate([](const FGridWorldObjectDefinitionValidationMessage& Message)
	{
		return Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Error && Message.Message.Contains(TEXT("Door must occupy its wall boundary"));
	}));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
