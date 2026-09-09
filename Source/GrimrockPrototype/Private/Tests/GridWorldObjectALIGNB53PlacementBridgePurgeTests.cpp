#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridWorldObjectDefinitionAsset.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridWorldObjectALIGNB53PlacementBridgePurgeTest,
	"Grimrock.WorldObjects.ALIGN_B5_3.PlacementBridgePurge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectALIGNB53PlacementBridgePurgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UClass* DefinitionClass = UGridWorldObjectDefinitionAsset::StaticClass();
	for (const FName PropertyName : {FName(TEXT("PlacementSurface")), FName(TEXT("DefaultLocalPosition"))})
	{
		const FProperty* Property = DefinitionClass->FindPropertyByName(PropertyName);
		if (TestNotNull(*FString::Printf(TEXT("%s remains the placement authority"), *PropertyName.ToString()), Property))
		{
			TestTrue(TEXT("Placement authority remains editable and persistent"),
				Property->HasAnyPropertyFlags(CPF_Edit) && !Property->HasAnyPropertyFlags(CPF_Transient));
		}
	}

	const TArray<FName> RemovedPlacementProperties = {
		TEXT("PlacementKind"), TEXT("PlacementZOffset"), TEXT("WallInset"), TEXT("LocalOffsetAlongWall"), TEXT("LocalOffsetVertical")
	};
	for (const FName PropertyName : RemovedPlacementProperties)
	{
		TestNull(*FString::Printf(TEXT("%s no longer exists in the reflected schema"), *PropertyName.ToString()),
			DefinitionClass->FindPropertyByName(PropertyName));
	}

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>();
	for (const EGridObjectPlacementKind Surface : {EGridObjectPlacementKind::Floor, EGridObjectPlacementKind::Wall, EGridObjectPlacementKind::Ceiling})
	{
		Definition->PlacementSurface = Surface;
		TestTrue(TEXT("Floor, Wall and Ceiling remain valid authoring surfaces"), Definition->HasValidPlacementSurface());
	}
	for (const EGridObjectPlacementKind Surface : {EGridObjectPlacementKind::Center, EGridObjectPlacementKind::Edge})
	{
		Definition->PlacementSurface = Surface;
		TestFalse(TEXT("Legacy Center and Edge enum values remain invalid authoring surfaces"), Definition->HasValidPlacementSurface());
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
