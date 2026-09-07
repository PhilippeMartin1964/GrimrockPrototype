#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG09SparseMarkerPurgeTest,
	"Grimrock.WorldObjects.MIG09.SparseMarkerPurge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG09SparseMarkerPurgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* LevelAssetClass = UGridLevelAsset::StaticClass();
	if (!TestNotNull(TEXT("Grid level asset class exists"), LevelAssetClass))
	{
		return false;
	}

	TestNull(
		TEXT("MIG09-D1 removes the serialized SparseBehaviorOverrideObjectIds migration marker"),
		LevelAssetClass->FindPropertyByName(TEXT("SparseBehaviorOverrideObjectIds")));

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();
	FGridLevelObjectData WorldObject;
	WorldObject.ObjectId = FGuid::NewGuid();
	WorldObject.Type = EGridLevelObjectType::Button;
	Level->Objects.Add(WorldObject);
	TestTrue(TEXT("Sparse behavior is structural for a reusable world object"), Level->UsesSparseBehaviorOverrides(WorldObject.ObjectId));

	FGridLevelObjectData LooseItem;
	LooseItem.ObjectId = FGuid::NewGuid();
	LooseItem.Type = EGridLevelObjectType::Item;
	Level->Objects.Add(LooseItem);
	TestFalse(TEXT("Loose items are not reusable world-object sparse behavior instances"), Level->UsesSparseBehaviorOverrides(LooseItem.ObjectId));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
