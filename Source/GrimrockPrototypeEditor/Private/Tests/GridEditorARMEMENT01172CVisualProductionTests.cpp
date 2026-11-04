#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "RPG/RPGSurfaceCatalystVisualAuthoring.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Materials/Material.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01172CProductionVisuals,
	"Grimrock.RPG.ARMEMENT01.17.2C.ProductionVisualAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01172CProductionVisuals::RunTest(const FString&)
{
	FString Problem;
	return TestTrue(TEXT("Native catalyst icon, mesh, 3 working materials and item references"),
		FRPGSurfaceCatalystVisualAuthoring::MatchesProductionAssets(&Problem));
}
#endif
