#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridBoundary.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG02SpatialBehaviorSchemaTest,
	"Grimrock.WorldObjects.MIG02.SpatialBehaviorSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG02SpatialBehaviorSchemaTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ArchetypeClass = UGridObjectArchetypeAsset::StaticClass();
	if (!TestNotNull(TEXT("World object archetype class exists"), ArchetypeClass))
	{
		return false;
	}

	FProperty* BlocksMovement = ArchetypeClass->FindPropertyByName(TEXT("bBlocksMovement"));
	FProperty* OccupiesBoundary = ArchetypeClass->FindPropertyByName(TEXT("bOccupiesBoundary"));
	FProperty* ReplacesWall = ArchetypeClass->FindPropertyByName(TEXT("bReplacesStandardWall"));
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
	for (TFieldIterator<FProperty> It(ArchetypeClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const FProperty* Property = *It;
		if (Property && Property->HasAnyPropertyFlags(CPF_Edit) && Property->GetMetaData(TEXT("Category")).StartsWith(TEXT("Spatial Behavior")))
		{
			++EditableSpatialPropertyCount;
		}
	}
	TestEqual(TEXT("Exactly three spatial behavior authoring parameters remain"), EditableSpatialPropertyCount, 3);

	const FName LegacySharingNames[] = {TEXT("bCanShareCell"), TEXT("bCanShareAnchor")};
	for (const FName LegacySharingName : LegacySharingNames)
	{
		FProperty* LegacyProperty = ArchetypeClass->FindPropertyByName(LegacySharingName);
		TestNotNull(*FString::Printf(TEXT("%s remains only as an internal compile bridge"), *LegacySharingName.ToString()), LegacyProperty);
		if (LegacyProperty)
		{
			TestTrue(*FString::Printf(TEXT("%s is transient"), *LegacySharingName.ToString()), LegacyProperty->HasAnyPropertyFlags(CPF_Transient));
			TestFalse(*FString::Printf(TEXT("%s is not an authoring parameter"), *LegacySharingName.ToString()), LegacyProperty->HasAnyPropertyFlags(CPF_Edit));
		}
	}

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>();
	TestFalse(TEXT("Default does not block cell movement"), Archetype->BlocksCellMovement());
	TestFalse(TEXT("Default does not occupy a boundary"), Archetype->OccupiesBoundary());
	TestFalse(TEXT("Default does not suppress a base wall"), Archetype->SuppressesBaseWall());

	Archetype->bBlocksMovement = true;
	Archetype->bOccupiesBoundary = true;
	Archetype->bReplacesStandardWall = true;
	TestTrue(TEXT("BlocksCellMovement semantic accessor reflects authoring data"), Archetype->BlocksCellMovement());
	TestTrue(TEXT("OccupiesBoundary semantic accessor reflects authoring data"), Archetype->OccupiesBoundary());
	TestTrue(TEXT("SuppressesBaseWall semantic accessor reflects authoring data"), Archetype->SuppressesBaseWall());

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

#endif // WITH_DEV_AUTOMATION_TESTS
