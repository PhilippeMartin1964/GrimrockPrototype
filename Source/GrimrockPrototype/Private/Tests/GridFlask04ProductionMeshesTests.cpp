#include "Misc/AutomationTest.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Engine/StaticMesh.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFlask04ProductionMeshes,
	"Grimrock.RPG.FLASK04.VersionedProjectileMeshes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFlask04ProductionMeshes::RunTest(const FString&)
{
	const AGridLevelRuntimeActor* Defaults = GetDefault<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime C++ defaults"), Defaults)) return false;
	TestFalse(TEXT("Oil fallback mesh reference is configured"), Defaults->OilFlaskFallbackMesh.IsNull());
	TestFalse(TEXT("Catalyst fallback mesh reference is configured"), Defaults->CatalystFlaskFallbackMesh.IsNull());
	TestNotNull(TEXT("Versioned oil bottle fallback loads"),
		Defaults->OilFlaskFallbackMesh.LoadSynchronous());
	TestNotNull(TEXT("Versioned catalyst bottle fallback loads"),
		Defaults->CatalystFlaskFallbackMesh.LoadSynchronous());
	const UGridItemDefinitionAsset* OilItem = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.DA_Item_Flask_Oil"));
	const UGridItemDefinitionAsset* CatalystItem = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_SurfaceCatalyst_01.DA_Item_SurfaceCatalyst_01"));
	TestNotNull(TEXT("Oil quick item is versioned"), OilItem);
	TestNotNull(TEXT("Catalyst quick item is versioned"), CatalystItem);

	// The level uses the Blueprint class, not the native CDO used above.
	// Assert the *actual production* Blueprint inherited the game-owned fallbacks.
	UClass* RuntimeBlueprint = LoadClass<AGridLevelRuntimeActor>(nullptr,
		TEXT("/Game/GrimrockPrototype/Blueprints/Runtime/BP_GridLevelRuntimeActor.BP_GridLevelRuntimeActor_C"));
	if (!TestNotNull(TEXT("Production runtime Blueprint resolves"), RuntimeBlueprint)) return false;
	const AGridLevelRuntimeActor* BpDefaults =
		RuntimeBlueprint->GetDefaultObject<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Production runtime Blueprint defaults"), BpDefaults)) return false;
	TestNotNull(TEXT("Oil bottle fallback inherited by the actual Blueprint"),
		BpDefaults->OilFlaskFallbackMesh.LoadSynchronous());
	TestNotNull(TEXT("Catalyst bottle fallback inherited by the actual Blueprint"),
		BpDefaults->CatalystFlaskFallbackMesh.LoadSynchronous());
	TestTrue(TEXT("FireBurst is configured in the production Blueprint (soft reference only)"),
		!BpDefaults->FireIgnitionBurstVfx.IsNull());
	return true;
}
#endif
