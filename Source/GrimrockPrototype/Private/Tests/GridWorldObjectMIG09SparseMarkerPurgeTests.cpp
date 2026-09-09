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

	FGridWorldObjectInstance& WorldObject = Level->WorldObjectInstances.AddDefaulted_GetRef();
	WorldObject.InstanceId = FGuid::NewGuid();
	WorldObject.WorldObjectDefinitionId = TEXT("MIG09_SparseButton");
	WorldObject.Type = EGridLevelObjectType::Button;
	TestTrue(TEXT("Sparse behavior is structural for a reusable world object"), Level->UsesSparseBehaviorOverrides(WorldObject.InstanceId));

	FGridLooseItemInstance& LooseItem = Level->LooseItemInstances.AddDefaulted_GetRef();
	LooseItem.InstanceId = FGuid::NewGuid();
	TestFalse(TEXT("Loose items are not reusable world-object sparse behavior instances"), Level->UsesSparseBehaviorOverrides(LooseItem.InstanceId));

	TestEqual(TEXT("Sparse classification preserves the native placement count"), Level->GetTypedPlacementCount(), 2);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
