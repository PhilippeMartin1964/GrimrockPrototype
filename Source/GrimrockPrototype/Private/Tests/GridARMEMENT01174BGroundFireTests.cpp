#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace ARMEMENT01174B
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
				FName(*FString::Printf(TEXT("ARMEMENT174B_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01174BProfileSanitization,
	"Grimrock.RPG.ARMEMENT01.17.4B.LevelSurfaceVfxProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01174BProfileSanitization::RunTest(const FString&)
{
	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();
	if (!TestNotNull(TEXT("Only the level DataAsset owns this profile"), Level)) return false;
	const FGridLevelSurfaceVfxSettings& Defaults = Level->SurfaceVfxSettings;
	TestFalse(TEXT("Legacy visual defaults retained until level opts in"), Defaults.bOverrideRuntimeVisuals);
	TestTrue(TEXT("Default 90 percent ground coverage"), FMath::IsNearlyEqual(Defaults.GroundFireCoverage, .9f));
	TestEqual(TEXT("Default cap for cells"), Defaults.MaxFireCells, 48);
	TestEqual(TEXT("Default cap for lights"), Defaults.MaxFireLights, 8);
	TestTrue(TEXT("Default catalyst wave preserved"), FMath::IsNearlyEqual(Defaults.CatalystWaveDelayPerCellSeconds, .085f));
	TestTrue(TEXT("No proprietary asset assumed installed"), Defaults.GroundFireSystem.IsNull());

	Level->SurfaceVfxSettings.GroundFireCoverage = 25.f;
	Level->SurfaceVfxSettings.FlameHeightCm = -1.f;
	Level->SurfaceVfxSettings.MaxFireCells = 500;
	Level->SurfaceVfxSettings.MaxFireLights = -100;
	Level->SurfaceVfxSettings.GroundFireScale = 0.f;
	Level->SurfaceVfxSettings.FireLightIntensity = -4.f;
	Level->SurfaceVfxSettings.SmokeDensity = 100.f;
	Level->SurfaceVfxSettings.CatalystWaveDelayPerCellSeconds = 4.f;
	const FGridLevelSurfaceVfxSettings Safe = Level->SurfaceVfxSettings.Sanitized();
	TestEqual(TEXT("Cell footprint never exceeds cell"), Safe.GroundFireCoverage, 1.f);
	TestEqual(TEXT("Minimum allowed fire height"), Safe.FlameHeightCm, 5.f);
	TestEqual(TEXT("Max cells capped"), Safe.MaxFireCells, 128);
	TestEqual(TEXT("Negative lights disabled"), Safe.MaxFireLights, 0);
	TestTrue(TEXT("Positive Niagara scale"), Safe.GroundFireScale >= .01f);
	TestEqual(TEXT("Negative light intensity clamped"), Safe.FireLightIntensity, 0.f);
	TestEqual(TEXT("Smoke bounded"), Safe.SmokeDensity, 4.f);
	TestTrue(TEXT("Wave interval bounded"), FMath::IsNearlyEqual(Safe.CatalystWaveDelayPerCellSeconds, .3f));
	TestEqual(TEXT("Serialized original is not mutated by reading"), Level->SurfaceVfxSettings.MaxFireCells, 500);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01174BPerLevelRuntime,
	"Grimrock.RPG.ARMEMENT01.17.4B.RuntimeLevelSwitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01174BPerLevelRuntime::RunTest(const FString&)
{
	ARMEMENT01174B::FWorldScope Fixture;
	if (!TestNotNull(TEXT("Test world exists"), Fixture.World)) return false;
	AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime)) return false;
	Runtime->FireSurfaceNiagaraScale = .24f;
	Runtime->FireSurfaceLightIntensity = 420.f;
	Runtime->CatalystWaveDelayPerCellSeconds = .12f;

	UGridLevelAsset* LevelA = NewObject<UGridLevelAsset>(Runtime);
	UGridLevelAsset* LevelB = NewObject<UGridLevelAsset>(Runtime);
	Runtime->LevelAsset = LevelA;
	FGridLevelSurfaceVfxSettings Legacy = Runtime->ResolveSurfaceVfxSettings();
	TestFalse(TEXT("Legacy mode when not opted in"), Legacy.bOverrideRuntimeVisuals);
	TestTrue(TEXT("Legacy torch scale preserved"), FMath::IsNearlyEqual(Legacy.GroundFireScale, .24f));
	TestTrue(TEXT("Legacy fire intensity preserved"), FMath::IsNearlyEqual(Legacy.FireLightIntensity, 420.f));
	TestTrue(TEXT("Legacy wave delay preserved"), FMath::IsNearlyEqual(Legacy.CatalystWaveDelayPerCellSeconds, .12f));

	LevelA->SurfaceVfxSettings.bOverrideRuntimeVisuals = true;
	LevelA->SurfaceVfxSettings.GroundFireCoverage = .88f;
	LevelA->SurfaceVfxSettings.GroundFireScale = 1.15f;
	LevelA->SurfaceVfxSettings.FireLightIntensity = 1200.f;
	LevelA->SurfaceVfxSettings.MaxFireCells = 6;
	LevelA->SurfaceVfxSettings.CatalystWaveDelayPerCellSeconds = .04f;
	LevelA->SurfaceVfxSettings.FlameDensityParameter = FName(TEXT("User.FlamesPerSecond"));
	LevelA->SurfaceVfxSettings.bEnableSmoke = false;
	const FGridLevelSurfaceVfxSettings First = Runtime->ResolveSurfaceVfxSettings();
	TestTrue(TEXT("Level A overrides actor"), First.bOverrideRuntimeVisuals);
	TestEqual(TEXT("Level A fire cap"), First.MaxFireCells, 6);
	TestTrue(TEXT("Level A wave"), FMath::IsNearlyEqual(First.CatalystWaveDelayPerCellSeconds, .04f));
	TestTrue(TEXT("Level A Niagara mapping retained"),
		First.FlameDensityParameter == FName(TEXT("User.FlamesPerSecond")));
	TestFalse(TEXT("Level A smoke toggle retained"), First.bEnableSmoke);

	LevelB->SurfaceVfxSettings.bOverrideRuntimeVisuals = true;
	LevelB->SurfaceVfxSettings.GroundFireCoverage = .70f;
	LevelB->SurfaceVfxSettings.MaxFireCells = 3;
	LevelB->SurfaceVfxSettings.CatalystWaveDelayPerCellSeconds = .09f;
	Runtime->LevelAsset = LevelB;
	const FGridLevelSurfaceVfxSettings Second = Runtime->ResolveSurfaceVfxSettings();
	TestEqual(TEXT("New level has independent visual cap"), Second.MaxFireCells, 3);
	TestTrue(TEXT("New level wave replaces previous"), FMath::IsNearlyEqual(Second.CatalystWaveDelayPerCellSeconds, .09f));
	TestTrue(TEXT("Previous profile remains unchanged"),
		FMath::IsNearlyEqual(LevelA->SurfaceVfxSettings.CatalystWaveDelayPerCellSeconds, .04f));
	TestTrue(TEXT("New level mapping not inherited"), Second.FlameDensityParameter.IsNone());
	return true;
}
#endif
