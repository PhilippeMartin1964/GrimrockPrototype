#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridObjectArchetypeAsset.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMaterialOwnershipAuditTest,
	"Grimrock.Architecture.MaterialOwnership.Audit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMaterialOwnershipAuditTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	static const FName RetiredMaterialProperties[] = {
		TEXT("PreviewMaterial"),
		TEXT("FixedMaterial"),
		TEXT("MovingMaterial"),
		TEXT("PitLeftLeafMaterial"),
		TEXT("PitRightLeafMaterial")
	};

	bool bSuccess = true;
	for (const FName PropertyName : RetiredMaterialProperties)
	{
		if (FindFProperty<FProperty>(UGridObjectArchetypeAsset::StaticClass(), PropertyName) != nullptr)
		{
			AddError(FString::Printf(TEXT("%s must not be a reflected UGridObjectArchetypeAsset property. Materials belong to Static Mesh Material Slots."),
				*PropertyName.ToString()));
			bSuccess = false;
		}
	}

	AddInfo(TEXT("Material ownership contract: GridObjectArchetypeAsset exposes no mesh-material override properties; Static Mesh Material Slots are authoritative."));
	return bSuccess;
}

#endif // WITH_DEV_AUTOMATION_TESTS
