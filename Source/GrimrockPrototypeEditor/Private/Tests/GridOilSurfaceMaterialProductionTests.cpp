#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialDomain.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Components/DecalComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilDecalProduction,
	"Grimrock.RPG.ARMEMENT01.17.1.4.ProductionMaterial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilDecalProduction::RunTest(const FString&)
{
	UMaterial* Master = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Surfaces/M_Surface_Oil.M_Surface_Oil"));
	UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Surfaces/MI_Surface_Oil.MI_Surface_Oil"));
	if (!TestNotNull(TEXT("Matériau d'huile natif"), Master) ||
		!TestNotNull(TEXT("Instance d'huile native"), Instance))
	{
		return false;
	}
	TestTrue(TEXT("Deferred Decal : aucune géométrie au-dessus du sol"),
		Master->MaterialDomain == MD_DeferredDecal);
	TestTrue(TEXT("Translucence du décalque"),
		Master->BlendMode == BLEND_Translucent);
	TestTrue(TEXT("Instance liée au bon maître"), Instance->GetMaterial() == Master);
	TestTrue(TEXT("Couleur projetée"), UMaterialEditingLibrary::GetMaterialPropertyInputNode(Master, MP_BaseColor) != nullptr);
	TestTrue(TEXT("Rugosité d'huile projetée"), UMaterialEditingLibrary::GetMaterialPropertyInputNode(Master, MP_Roughness) != nullptr);
	TestTrue(TEXT("Contour irrégulier et transparent"),
		UMaterialEditingLibrary::GetMaterialPropertyInputNode(Master, MP_Opacity) != nullptr);
	const AGridLevelRuntimeActor* Defaults = GetDefault<AGridLevelRuntimeActor>();
	TestTrue(TEXT("Aucun plan instancié au repos"),
		Defaults != nullptr && Defaults->GetActiveOilDecalCount() == 0);
	UClass* BPClass = LoadClass<AGridLevelRuntimeActor>(nullptr,
		TEXT("/Game/GrimrockPrototype/Blueprints/Runtime/BP_GridLevelRuntimeActor.BP_GridLevelRuntimeActor_C"));
	if (!TestNotNull(TEXT("Blueprint de production chargé"), BPClass)) return false;
	const AGridLevelRuntimeActor* BPDefaults = BPClass->GetDefaultObject<AGridLevelRuntimeActor>();
	TestTrue(TEXT("L'instance de décalque est référencée par le Blueprint"),
		BPDefaults && BPDefaults->OilSurfaceMaterial.LoadSynchronous() == Instance);
	return true;
}
#endif
