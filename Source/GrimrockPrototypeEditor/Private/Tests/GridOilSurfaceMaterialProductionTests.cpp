#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialEditingLibrary.h"
#include "Engine/StaticMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"

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
	TestTrue(TEXT("Domaine Surface"), Master->MaterialDomain == MD_Surface);
	TestEqual(TEXT("Mélange translucide"), Master->BlendMode, TEnumAsByte<EBlendMode>(BLEND_Translucent));
	TestTrue(TEXT("Éclairage direct par pixel et reflets locaux"),
		Master->TranslucencyLightingMode == TLM_SurfacePerPixelLighting);
	TestTrue(TEXT("L'instance appartient au matériau canonique"), Instance->GetMaterial() == Master);

	// Deux assets valides peuvent encore porter un graphe de matériau vide.
	// Un tel shader donne l'impression du matériau générique Unreal en PIE.
	TestTrue(TEXT("Graphe : couleur reliée à BaseColor"),
		UMaterialEditingLibrary::GetMaterialPropertyInputNode(Master, MP_BaseColor) != nullptr);
	TestTrue(TEXT("Graphe : transparence reliée à Opacity"),
		UMaterialEditingLibrary::GetMaterialPropertyInputNode(Master, MP_Opacity) != nullptr);
	TestTrue(TEXT("Graphe : rugosité reliée à Roughness"),
		UMaterialEditingLibrary::GetMaterialPropertyInputNode(Master, MP_Roughness) != nullptr);

	// La présence du matériau seul ne suffit pas : il doit être réellement
	// affecté au slot 0 d'un composant possédant un mesh dans le CDO natif.
	const AGridLevelRuntimeActor* NativeDefaults = GetDefault<AGridLevelRuntimeActor>();
	const UInstancedStaticMeshComponent* NativeOil = NativeDefaults
		? NativeDefaults->GetCombatOilSurfaceVisualComponent() : nullptr;
	if (!TestNotNull(TEXT("Composant de flaque natif"), NativeOil))
	{
		return false;
	}
	TestTrue(TEXT("Plan superposé lié au composant natif"), NativeOil->GetStaticMesh() != nullptr);
	TestTrue(TEXT("MI_Surface_Oil affecté au matériau du mesh, slot 0"),
		NativeOil->GetMaterial(0) == Instance);

	// Contrôler aussi le Blueprint livré avec le projet, celui que le
	// joueur est susceptible de voir et d'utiliser dans le donjon.
	UClass* RuntimeBlueprintClass = LoadClass<AGridLevelRuntimeActor>(nullptr,
		TEXT("/Game/GrimrockPrototype/Blueprints/Runtime/BP_GridLevelRuntimeActor.BP_GridLevelRuntimeActor_C"));
	if (!TestNotNull(TEXT("Blueprint runtime de production"), RuntimeBlueprintClass))
	{
		return false;
	}
	const AGridLevelRuntimeActor* BlueprintDefaults = RuntimeBlueprintClass->GetDefaultObject<AGridLevelRuntimeActor>();
	const UInstancedStaticMeshComponent* BlueprintOil = BlueprintDefaults
		? BlueprintDefaults->GetCombatOilSurfaceVisualComponent() : nullptr;
	if (!TestNotNull(TEXT("Composant de flaque hérité dans le Blueprint"), BlueprintOil))
	{
		return false;
	}
	TestTrue(TEXT("Plan superposé présent dans le Blueprint"), BlueprintOil->GetStaticMesh() != nullptr);
	TestTrue(TEXT("MI_Surface_Oil lié au slot 0 du Blueprint"), BlueprintOil->GetMaterial(0) == Instance);
	TestTrue(TEXT("Huile sans collision"), BlueprintOil->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestEqual(TEXT("Aucune flaque permanente au repos"), BlueprintOil->GetInstanceCount(), 0);

	// Contrôle de diagnostic : le matériau de la pierre est normalement
	// porté par le StaticMesh ; il ne doit pas être celui de secours UE.
	if (UStaticMesh* ShippedFloor = BlueprintDefaults->FloorMesh)
	{
		const UMaterialInterface* FloorMaterial = ShippedFloor->GetMaterial(0);
		TestTrue(TEXT("Le plancher de production n'utilise pas la grille UE"),
			FloorMaterial != nullptr &&
			FloorMaterial != UMaterial::GetDefaultMaterial(MD_Surface) &&
			!FloorMaterial->GetPathName().Contains(TEXT("WorldGridMaterial")) &&
			!FloorMaterial->GetPathName().Contains(TEXT("DefaultMaterial")));
	}
	else
	{
		AddInfo(TEXT("Le mesh du sol n'est pas défini sur le CDO Blueprint : il peut être configuré sur l'acteur dans la carte."));
	}
	return true;
}


// Vérification en lecture seule du binaire présent dans Git LFS.
// Ne jamais appeler CheckMaterialUsage ici : il modifierait l'asset.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilInstancedMaterialUsage,
	"Grimrock.RPG.ARMEMENT01.17.1.4.InstancedMaterialUsage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilInstancedMaterialUsage::RunTest(const FString&)
{
	const UMaterial* Master = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Surfaces/M_Surface_Oil.M_Surface_Oil"));
	if (!TestNotNull(TEXT("Matériau natif d'huile"), Master))
	{
		return false;
	}
	TestTrue(TEXT("Permutation compilable pour les Instanced Static Meshes"),
		Master->bUsedWithInstancedStaticMeshes != 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
