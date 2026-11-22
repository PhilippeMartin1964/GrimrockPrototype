#include "Misc/AutomationTest.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Items/GridItemCatalogues.h"
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
	// Both flasks are now authored in the canonical stage. Never use the
	// retired DA_Item_* as a production-mesh acceptance dependency.
	UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem"));
	if (!TestNotNull(TEXT("Canonical production item registry loads"), Root)) return false;
	const auto CheckFlask = [this, Root](FName ItemId, const TCHAR* Description)
	{
		const auto* Entry = Root->ItemDefinitions.FindByPredicate(
			[ItemId](const TObjectPtr<UGridCanonicalItemDefinitionAsset>& Candidate)
			{
				return IsValid(Candidate.Get()) && Candidate->ItemId == ItemId;
			});
		return TestTrue(Description, Entry && IsValid(Entry->Get()) &&
			(*Entry)->HasValidIdentity() && (*Entry)->IsA<UGridNonArmamentDefinitionAsset>());
	};
	CheckFlask(TEXT("Item_Flask_Oil"), TEXT("Oil flask resolves as canonical consumable"));
	CheckFlask(TEXT("Item_SurfaceCatalyst_01"), TEXT("Catalyst resolves as canonical consumable"));

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
	// FireBurst is an optional one-shot effect. The author may configure
	// it on the placed L_Dungeon actor rather than the Blueprint class CDO;
	// absence from the class does not invalidate the bottle's mesh.
	if (BpDefaults->FireIgnitionBurstVfx.IsNull())
	{
		AddInfo(TEXT("FireBurst not set on Blueprint Class Defaults; a placed runtime actor may override it."));
	}
	return true;
}
#endif
