#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridSurfaceVfxProfileAsset.h"
#include "Core/GridCatalystVfxProfileAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "UObject/UnrealType.h"

namespace GridVfxArchitectureTest
{
	struct FWorldScope
	{
		UWorld* World = nullptr;

		FWorldScope()
		{
			const UWorld::InitializationValues Init = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
				.ShouldSimulatePhysics(false).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("VfxArchitecture_%s"),
					*FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Init);
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
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridVfxArchitectureDataOwnership,
	"Grimrock.RPG.ARMEMENT01.17.4D.DataOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridVfxArchitectureDataOwnership::RunTest(const FString&)
{
	const UClass* Level = UGridLevelAsset::StaticClass();
	TestNull(TEXT("No per-level SurfaceVfxSettings"),
		Level->FindPropertyByName(TEXT("SurfaceVfxSettings")));
	TestNull(TEXT("No per-level SurfaceVfxProfile"),
		Level->FindPropertyByName(TEXT("SurfaceVfxProfile")));
	TestNull(TEXT("No VFX legacy type in level DataAsset"),
		Level->FindPropertyByName(TEXT("bOverrideRuntimeVisuals")));

	const UClass* Actor = AGridLevelRuntimeActor::StaticClass();
	TestNotNull(TEXT("One reusable fire profile"), Actor->FindPropertyByName(TEXT("FireVfxProfile")));
	TestNotNull(TEXT("Finite ignition burst lifetime"), Actor->FindPropertyByName(TEXT("FireIgnitionBurstMaxLifetimeSeconds")));
	TestNotNull(TEXT("Finite glass-break lifetime"), Actor->FindPropertyByName(TEXT("FlaskGlassBreakMaxLifetimeSeconds")));
	TestNotNull(TEXT("One independent catalyst profile"), Actor->FindPropertyByName(TEXT("CatalystVfxProfile")));
	for (const TCHAR* DeadField : {
			TEXT("DefaultSurfaceVfxProfile"),
			TEXT("FireSurfaceNiagaraSystem"),
			TEXT("FireSurfaceNiagaraScale"),
			TEXT("FireSurfaceLightIntensity"),
			TEXT("CatalystFlightDurationSeconds"),
			TEXT("CatalystPulseDurationSeconds"),
			TEXT("CatalystWaveDelayPerCellSeconds"),
			TEXT("CatalystProjectileMesh"),
			TEXT("CatalystRunePulseMaterial"),
			TEXT("CatalystEmberMaterial"),
			TEXT("ActiveFireSurfaceFlames")})
	{
		TestNull(*FString::Printf(TEXT("Deleted duplicate actor property %s"), DeadField),
			Actor->FindPropertyByName(DeadField));
	}
	TestNotNull(TEXT("Fire profile exposes fire settings"),
		UGridSurfaceVfxProfileAsset::StaticClass()->FindPropertyByName(TEXT("Settings")));
	TestNotNull(TEXT("Catalyst profile exposes separate settings"),
		UGridCatalystVfxProfileAsset::StaticClass()->FindPropertyByName(TEXT("Settings")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridVfxArchitectureProfileSanitization,
	"Grimrock.RPG.ARMEMENT01.17.4D.ProfileBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridVfxArchitectureProfileSanitization::RunTest(const FString&)
{
	UGridSurfaceVfxProfileAsset* Fire = NewObject<UGridSurfaceVfxProfileAsset>();
	UGridCatalystVfxProfileAsset* Catalyst = NewObject<UGridCatalystVfxProfileAsset>();
	if (!TestNotNull(TEXT("Fire"), Fire) ||
		!TestNotNull(TEXT("Catalyst"), Catalyst))
	{
		return false;
	}
	TestTrue(TEXT("No FireFX dependency by default"), Fire->Settings.GroundFireSystem.IsNull());
	TestEqual(TEXT("Fire cell default budget"), Fire->GetSettings().MaxFireCells, 48);
	TestEqual(TEXT("Fire light default budget"), Fire->GetSettings().MaxFireLights, 8);
	TestEqual(TEXT("One fire audio voice per region, budget four by default"),
		Fire->GetSettings().MaxFireCrackleSources, 4);
	TestTrue(TEXT("No sound dependency by default"), Fire->GetSettings().FireCrackleLoopSound.IsNull());
	TestTrue(TEXT("Default coverage 90 percent"),
		FMath::IsNearlyEqual(Fire->GetSettings().GroundFireCoverage, .9f));

	Fire->Settings.GroundFireScale = -5.f;
	Fire->Settings.GroundFireCoverage = 8.f;
	Fire->Settings.MaxFireCells = 500;
	Fire->Settings.MaxFireLights = -10;
	Fire->Settings.FireLightIntensity = -1.f;
	Fire->Settings.SmokeDensity = 50.f;
	Fire->Settings.MaxFireCrackleSources = 100;
	Fire->Settings.FireCrackleVolume = -1.f;
	const FGridFireVfxSettings SafeFire = Fire->GetSettings();
	TestTrue(TEXT("Positive Niagara scale"), SafeFire.GroundFireScale >= .01f);
	TestEqual(TEXT("No coverage outside cell"), SafeFire.GroundFireCoverage, 1.f);
	TestEqual(TEXT("Upper bound for Niagara count"), SafeFire.MaxFireCells, 128);
	TestEqual(TEXT("Negative light count disabled"), SafeFire.MaxFireLights, 0);
	TestEqual(TEXT("Negative light intensity disabled"), SafeFire.FireLightIntensity, 0.f);
	TestEqual(TEXT("Smoke density constrained"), SafeFire.SmokeDensity, 4.f);
	TestEqual(TEXT("Crackle audio budget capped"), SafeFire.MaxFireCrackleSources, 8);
	TestEqual(TEXT("Negative audio volume silenced"), SafeFire.FireCrackleVolume, 0.f);
	TestEqual(TEXT("Authoring data not silently mutated"), Fire->Settings.MaxFireCells, 500);

	Catalyst->Settings.FlightDurationSeconds = 0.f;
	Catalyst->Settings.PulseDurationSeconds = 50.f;
	Catalyst->Settings.WaveDelayPerCellSeconds = -1.f;
	Catalyst->Settings.FlightArcHeightCm = 99999.f;
	const FGridCatalystVfxSettings SafeCatalyst = Catalyst->GetSettings();
	TestTrue(TEXT("Flight bounded"), FMath::IsNearlyEqual(SafeCatalyst.FlightDurationSeconds, .1f));
	TestTrue(TEXT("Pulse bounded"), FMath::IsNearlyEqual(SafeCatalyst.PulseDurationSeconds, 2.f));
	TestTrue(TEXT("Wave bounded"), FMath::IsNearlyEqual(SafeCatalyst.WaveDelayPerCellSeconds, 0.f));
	TestTrue(TEXT("Arc bounded"), FMath::IsNearlyEqual(SafeCatalyst.FlightArcHeightCm, 300.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridVfxArchitectureLevelIndependence,
	"Grimrock.RPG.ARMEMENT01.17.4D.IndependentOfLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridVfxArchitectureLevelIndependence::RunTest(const FString&)
{
	GridVfxArchitectureTest::FWorldScope Scope;
	if (!TestNotNull(TEXT("World"), Scope.World))
	{
		return false;
	}
	AGridLevelRuntimeActor* Actor = Scope.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime actor"), Actor))
	{
		return false;
	}

	UGridSurfaceVfxProfileAsset* Fire = NewObject<UGridSurfaceVfxProfileAsset>(Actor);
	UGridCatalystVfxProfileAsset* Catalyst = NewObject<UGridCatalystVfxProfileAsset>(Actor);
	Fire->Settings.FireLightIntensity = 1700.f;
	Catalyst->Settings.FlightDurationSeconds = .6f;
	Actor->FireVfxProfile = Fire;
	Actor->CatalystVfxProfile = Catalyst;

	UGridLevelAsset* FirstLevel = NewObject<UGridLevelAsset>(Actor);
	UGridLevelAsset* SecondLevel = NewObject<UGridLevelAsset>(Actor);
	Actor->LevelAsset = FirstLevel;
	TestEqual(TEXT("Shared fire independent from level A"),
		Actor->ResolveFireVfxSettings().FireLightIntensity, 1700.f);
	TestTrue(TEXT("Shared projectile independent from level A"),
		FMath::IsNearlyEqual(Actor->ResolveCatalystVfxSettings().FlightDurationSeconds, .6f));
	Actor->LevelAsset = SecondLevel;
	TestEqual(TEXT("Same fire in level B"),
		Actor->ResolveFireVfxSettings().FireLightIntensity, 1700.f);
	TestTrue(TEXT("Same catalyst in level B"),
		FMath::IsNearlyEqual(Actor->ResolveCatalystVfxSettings().FlightDurationSeconds, .6f));

	Actor->FireVfxProfile.Reset();
	Actor->CatalystVfxProfile.Reset();
	TestTrue(TEXT("No hardcoded commercial Niagara fallback"),
		Actor->ResolveFireVfxSettings().GroundFireSystem.IsNull());
	TestTrue(TEXT("No hardcoded projectile mesh fallback"),
		Actor->ResolveCatalystVfxSettings().ProjectileMesh.IsNull());
	return true;
}

#endif
