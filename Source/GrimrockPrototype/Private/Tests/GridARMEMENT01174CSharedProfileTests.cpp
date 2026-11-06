#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridSurfaceVfxProfileAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace ARMEMENT01174C
{
	struct FWorldScope
	{
		UWorld* World = nullptr;
		FWorldScope()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
				.ShouldSimulatePhysics(false).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ARMEMENT174C_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}
		~FWorldScope()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) GEngine->DestroyWorldContext(World);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01174CSharedVfxProfile,
	"Grimrock.RPG.ARMEMENT01.17.4C.SharedVfxProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01174CSharedVfxProfile::RunTest(const FString&)
{
	ARMEMENT01174C::FWorldScope Fixture;
	if (!TestNotNull(TEXT("Test world"), Fixture.World)) return false;
	AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime actor"), Runtime)) return false;

	UGridLevelAsset* LevelA = NewObject<UGridLevelAsset>(Runtime);
	UGridLevelAsset* LevelB = NewObject<UGridLevelAsset>(Runtime);
	UGridSurfaceVfxProfileAsset* Shared = NewObject<UGridSurfaceVfxProfileAsset>(Runtime);
	UGridSurfaceVfxProfileAsset* Local = NewObject<UGridSurfaceVfxProfileAsset>(Runtime);
	Shared->Settings.CatalystWaveDelayPerCellSeconds = .07f;
	Shared->Settings.FireLightIntensity = 1100.f;
	Local->Settings.CatalystWaveDelayPerCellSeconds = .03f;
	Local->Settings.FireLightIntensity = 400.f;

	Runtime->LevelAsset = LevelA;
	Runtime->DefaultSurfaceVfxProfile = Shared;
	const FGridLevelSurfaceVfxSettings A = Runtime->ResolveSurfaceVfxSettings();
	TestTrue(TEXT("Shared profile is an explicit visual override"), A.bOverrideRuntimeVisuals);
	TestEqual(TEXT("Shared profile intensity"), A.FireLightIntensity, 1100.f);
	TestTrue(TEXT("Shared profile wave"), FMath::IsNearlyEqual(A.CatalystWaveDelayPerCellSeconds, .07f));

	LevelA->SurfaceVfxProfile = Local;
	const FGridLevelSurfaceVfxSettings B = Runtime->ResolveSurfaceVfxSettings();
	TestEqual(TEXT("Level profile overrides only that level"), B.FireLightIntensity, 400.f);
	TestTrue(TEXT("Level wave override"), FMath::IsNearlyEqual(B.CatalystWaveDelayPerCellSeconds, .03f));

	Runtime->LevelAsset = LevelB;
	TestEqual(TEXT("Second level inherits shared profile, not previous level"), Runtime->ResolveSurfaceVfxSettings().FireLightIntensity, 1100.f);

	LevelB->SurfaceVfxSettings.bOverrideRuntimeVisuals = true;
	LevelB->SurfaceVfxSettings.FireLightIntensity = 250.f;
	TestEqual(TEXT("17.4B legacy values are not discarded"), Runtime->ResolveSurfaceVfxSettings().FireLightIntensity, 250.f);

	LevelB->SurfaceVfxProfile = Local;
	TestEqual(TEXT("New explicit profile wins legacy inline values"), Runtime->ResolveSurfaceVfxSettings().FireLightIntensity, 400.f);
	TestTrue(TEXT("Shared source asset remains unchanged"), FMath::IsNearlyEqual(Shared->Settings.CatalystWaveDelayPerCellSeconds, .07f));

	Runtime->LevelAsset = nullptr;
	Runtime->DefaultSurfaceVfxProfile.Reset();
	Runtime->FireSurfaceLightIntensity = 321.f;
	TestEqual(TEXT("Legacy runtime defaults remain in absence of profile"), Runtime->ResolveSurfaceVfxSettings().FireLightIntensity, 321.f);
	return true;
}
#endif
