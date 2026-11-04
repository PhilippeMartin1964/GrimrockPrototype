#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridOilMaterialProductionTest,
	"Grimrock.RPG.ARMEMENT01.17.1.4.ProductionMaterial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridOilMaterialProductionTest::RunTest(const FString&)
{
	UMaterial* Master = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Surfaces/M_Surface_Oil.M_Surface_Oil"));
	UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Surfaces/MI_Surface_Oil.MI_Surface_Oil"));

	TestNotNull(TEXT("Matériau huile natif de production"), Master);
	TestNotNull(TEXT("Instance huile native de production"), Instance);
	if (!Master || !Instance) return false;
	TestEqual(TEXT("Domaine Surface"), Master->MaterialDomain, TEnumAsByte<EMaterialDomain>(MD_Surface));
	TestEqual(TEXT("Mélange translucide"), Master->BlendMode, TEnumAsByte<EBlendMode>(BLEND_Translucent));
	TestTrue(TEXT("Éclairage direct par pixel et reflets locaux"),
		Master->TranslucencyLightingMode == TLM_SurfacePerPixelLighting);
	TestTrue(TEXT("L'instance appartient au matériau canonique"), Instance->GetMaterial() == Master);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
