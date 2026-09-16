#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLightEmitterComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemLIGHT03PhysicsMeshEmitterAttachmentTest,
	"Grimrock.Items.LIGHT03.PhysicsMeshEmitterAttachment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemLIGHT03PhysicsMeshEmitterAttachmentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const AGridItemActor* ItemCDO = GetDefault<AGridItemActor>();
	TestNotNull(TEXT("Grid item CDO exists"), ItemCDO);
	if (!ItemCDO)
	{
		return false;
	}

	TestNotNull(TEXT("Physics mesh component exists"), ItemCDO->MeshComponent);
	TestNotNull(TEXT("Light emitter component exists"), ItemCDO->LightEmitterComponent);
	if (!ItemCDO->MeshComponent || !ItemCDO->LightEmitterComponent)
	{
		return false;
	}

	TestTrue(TEXT("Light emitter is attached directly to the physics mesh so world-item motion carries the flame and point light"),
		ItemCDO->LightEmitterComponent->GetAttachParent() == ItemCDO->MeshComponent);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
