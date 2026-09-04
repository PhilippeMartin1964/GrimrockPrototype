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

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>();
	if (!Archetype)
	{
		AddError(TEXT("Unable to create a transient UGridObjectArchetypeAsset for material ownership validation."));
		return false;
	}

	const bool bCompatibilityShimsAreNull =
		Archetype->PreviewMaterial.Get() == nullptr && Archetype->FixedMaterial.Get() == nullptr && Archetype->MovingMaterial.Get() == nullptr &&
		Archetype->PitLeftLeafMaterial.Get() == nullptr && Archetype->PitRightLeafMaterial.Get() == nullptr;
	if (!bCompatibilityShimsAreNull)
	{
		AddError(TEXT("Retired material compatibility shims must always resolve to nullptr."));
		bSuccess = false;
	}

	AddInfo(TEXT("Material ownership contract: GridObjectArchetypeAsset exposes no mesh-material override properties; Static Mesh Material Slots are authoritative."));
	return bSuccess;
}

#endif // WITH_DEV_AUTOMATION_TESTS
