#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridSurfaceVfxProfileAsset.h"
#include "Core/GridCatalystVfxProfileAsset.h"
#include "Core/GridFireNiagaraGeometry.h"
#include "Core/GridFireNiagaraBindings.h"
#include "Core/GridNiagaraUserOverrideApplicator.h"
#include "Core/GridFireIgnitionWave.h"
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
	const UFunction* Diagnostic = Actor->FindFunctionByName(TEXT("LogGroundFireNiagaraBindings"));
	TestNotNull(TEXT("Editor Niagara parameter audit available"), Diagnostic);
	if (Diagnostic)
	{
		// CallInEditor is an editor UFUNCTION specifier, not an EFunctionFlags value.
		// Ensure the diagnostic remains invokable through generated reflection.
		TestTrue(TEXT("Diagnostic is Blueprint-callable"),
			Diagnostic->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	}
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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFireNiagaraGeometryControlsTest,
	"Grimrock.RPG.FIREFX10.GeometryAndProfileRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFireNiagaraGeometryControlsTest::RunTest(const FString&)
{
	FGridFireVfxSettings Settings;
	const FVector Baseline = FGridFireNiagaraGeometry::ComponentScale(Settings, true);
	TestTrue(TEXT("SpawnBox preserves native XY component scale"),
		Baseline.Equals(FVector(1.f), .001f));
	const FVector Unbound = FGridFireNiagaraGeometry::ComponentScale(Settings, false);
	TestTrue(TEXT("Unbound XY scales only proportionally, not as a 200cm guarantee"),
		FMath::IsNearlyEqual(Unbound.X, .9f));
	Settings.FlameHeightCm = 150.f;
	const FVector Tall = FGridFireNiagaraGeometry::ComponentScale(Settings, false);
	TestTrue(TEXT("Height 150 makes Z twice as tall"), FMath::IsNearlyEqual(Tall.Z, 2.f));
	TestTrue(TEXT("Changing height never changes width"),
		FMath::IsNearlyEqual(Tall.X, Unbound.X));
	Settings.GroundFireCoverage = .45f;
	const FVector Narrow = FGridFireNiagaraGeometry::ComponentScale(Settings, false);
	TestTrue(TEXT("Half coverage changes XY only"), FMath::IsNearlyEqual(Narrow.X, .45f));
	TestTrue(TEXT("Coverage preserves Z"), FMath::IsNearlyEqual(Narrow.Z, Tall.Z));
	TestTrue(TEXT("Typed SpawnBox controls XY and avoids a second transform"),
		FMath::IsNearlyEqual(
			FGridFireNiagaraGeometry::ComponentScale(Settings, true).X, 1.f));
	Settings.GroundFireScale = 1.5f;
	const FVector Bigger = FGridFireNiagaraGeometry::ComponentScale(Settings, false);
	TestTrue(TEXT("General size multiplies every axis"),
		Bigger.Equals(Narrow * 1.5f, .001f));
	Settings.NiagaraReferenceHeightCm = 0.f;
	TestTrue(TEXT("Missing calibration cannot divide by zero"),
		FMath::IsFinite(FGridFireNiagaraGeometry::ComponentScale(Settings, false).Z));
	TestEqual(TEXT("User namespace is removed consistently"),
		FGridFireNiagaraBindings::ShortName(FName(TEXT("User.SpawnBox"))),
		FString(TEXT("SpawnBox")));
	TestEqual(TEXT("Qualified user name generated"),
		FGridFireNiagaraBindings::QualifiedName(FName(TEXT("SpawnBox"))),
		FName(TEXT("User.SpawnBox")));
	TestTrue(TEXT("Native flame rate preserved at density one"),
		FMath::IsNearlyEqual(FGridFireNiagaraBindings::ScaledRate(84.f, 1.f), 84.f));
	TestTrue(TEXT("Flame rate scales without clobbering the native base"),
		FMath::IsNearlyEqual(FGridFireNiagaraBindings::ScaledRate(84.f, .5f), 42.f));
	TestEqual(TEXT("Smoke disabled means no rate"), FGridFireNiagaraBindings::ScaledRate(84.f, 0.f), 0.f);
	TestTrue(TEXT("Typed override accepts actual Float"), FGridNiagaraUserOverrideApplicator::MatchesType(
		EGridNiagaraOverrideValueType::Float, FNiagaraTypeDefinition::GetFloatDef()));
	TestFalse(TEXT("Typed override never writes Float into Vector"),
		FGridNiagaraUserOverrideApplicator::MatchesType(
			EGridNiagaraOverrideValueType::Float, FNiagaraTypeDefinition::GetVec3Def()));
	TestTrue(TEXT("Typed override accepts Vector3"),
		FGridNiagaraUserOverrideApplicator::MatchesType(
			EGridNiagaraOverrideValueType::Vector3, FNiagaraTypeDefinition::GetVec3Def()));
	TestEqual(TEXT("Overrides always use canonical User namespace"),
		FGridNiagaraUserOverrideApplicator::ShortName(FName(TEXT("User.FlameSizeMax"))),
		FString(TEXT("FlameSizeMax")));

	// Reflection comparison drives FIREFX10's no-op path during PIE.
	FGridFireVfxSettings Same = Settings;
	const UScriptStruct* FireStruct = FGridFireVfxSettings::StaticStruct();
	TestTrue(TEXT("Unchanged data profile never needs a Niagara rebuild"),
		FireStruct->CompareScriptStruct(&Settings, &Same, PPF_None));
	Same.FlameDensity = 0.25f;
	TestFalse(TEXT("Changing one visual setting triggers a new presentation"),
		FireStruct->CompareScriptStruct(&Settings, &Same, PPF_None));
	Same = Settings;
	FGridNiagaraUserOverride Override;
	Override.ParameterName = FName(TEXT("FlameSizeMax"));
	Override.FloatValue = 25.f;
	Same.UserParameterOverrides.Add(Override);
	TestFalse(TEXT("Changing a typed user override triggers a new presentation"),
		FireStruct->CompareScriptStruct(&Settings, &Same, PPF_None));

	const UScriptStruct* FireSettingsType = FGridFireVfxSettings::StaticStruct();
	for (const TCHAR* Retired : { TEXT("SpawnAreaParameter"), TEXT("FlameHeightParameter"),
		TEXT("FlameDensityParameter"), TEXT("SmokeDensityParameter"), TEXT("FireTintParameter"),
		TEXT("NiagaraReferenceWidthCm"), TEXT("bAutoBindKnownFireControls"),
		TEXT("bEnableSmoke") })
	{
		TestNull(*FString::Printf(TEXT("FIREFX10 removes obsolete authoring property %s"), Retired),
			FireSettingsType->FindPropertyByName(FName(Retired)));
	}
	return true;
}



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFX11SettingsTest,
	"Grimrock.RPG.FIREFX11.Settings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFX11SettingsTest::RunTest(const FString&)
{
	FGridFireVfxSettings Settings;
	TestTrue(TEXT("Default propagation 150ms"), FMath::IsNearlyEqual(Settings.PropagationDelayPerCell,.15f));
	TestTrue(TEXT("Default cell growth 400ms"), FMath::IsNearlyEqual(Settings.IgnitionGrowthDuration,.4f));
	TestTrue(TEXT("Default ignition coverage 0.1"), FMath::IsNearlyEqual(Settings.InitialCoverage,.1f));
	Settings.PropagationDelayPerCell = 500.f;
	Settings.IgnitionGrowthDuration = -100.f;
	Settings.InitialCoverage = -1.f;
	const FGridFireVfxSettings Safe = Settings.Sanitized();
	TestTrue(TEXT("Propagation bounded"), Safe.PropagationDelayPerCell <= 1.f);
	TestTrue(TEXT("Growth bounded"), Safe.IgnitionGrowthDuration >= .05f);
	TestTrue(TEXT("Coverage bounded"), Safe.InitialCoverage >= .01f);
	const UScriptStruct* Reflected = FGridFireVfxSettings::StaticStruct();
	for (const TCHAR* Property : {TEXT("PropagationDelayPerCell"),
		TEXT("IgnitionGrowthDuration"), TEXT("InitialCoverage")})
	{
		TestNotNull(*FString::Printf(TEXT("FIREFX11 field %s"), Property),
			Reflected->FindPropertyByName(FName(Property)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFX11CrossTest,
	"Grimrock.RPG.FIREFX11.Cross",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFX11CrossTest::RunTest(const FString&)
{
	const FIntPoint Origin(5,5);
	const TArray<FIntPoint> Cross = {
		Origin,FIntPoint(4,5),FIntPoint(6,5),FIntPoint(5,4),FIntPoint(5,6)
	};
	TMap<FIntPoint,float> Delays;
	FGridFireIgnitionWave::BuildDelays(Origin,Cross,.15f,Delays);
	TestEqual(TEXT("Cross five cells"),Delays.Num(),5);
	TestTrue(TEXT("Impact starts at zero"),FMath::IsNearlyZero(Delays.FindRef(Origin)));
	for (const FIntPoint& Neighbor : {
		FIntPoint(4,5),FIntPoint(6,5),FIntPoint(5,4),FIntPoint(5,6)})
	{
		TestTrue(TEXT("Cross neighbor follows at 150ms"),
			FMath::IsNearlyEqual(Delays.FindRef(Neighbor),.15f));
	}
	return true;
}

namespace FIREFX11Test
{
	static TMap<FIntPoint, float> MakeTurnDelays()
	{
		const TArray<FIntPoint> Turn = {
			FIntPoint(0,0),FIntPoint(1,0),FIntPoint(1,1),FIntPoint(2,1)
		};
		TMap<FIntPoint, float> Delays;
		FGridFireIgnitionWave::BuildDelays(FIntPoint(0,0),Turn,.15f,Delays);
		return Delays;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFX11TurnCountTest,
	"Grimrock.RPG.FIREFX11.TurnCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFX11TurnCountTest::RunTest(const FString&)
{
	const auto Delays = FIREFX11Test::MakeTurnDelays();
	TestEqual(TEXT("Four contiguous cells"), Delays.Num(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFX11TurnOneTest,
	"Grimrock.RPG.FIREFX11.TurnStep1",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFX11TurnOneTest::RunTest(const FString&)
{
	const auto Delays = FIREFX11Test::MakeTurnDelays();
	TestTrue(TEXT("First cell 150ms"), FMath::IsNearlyEqual(Delays.FindRef(FIntPoint(1,0)), .15f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFX11TurnTwoTest,
	"Grimrock.RPG.FIREFX11.TurnStep2",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFX11TurnTwoTest::RunTest(const FString&)
{
	const auto Delays = FIREFX11Test::MakeTurnDelays();
	TestTrue(TEXT("Second cell 300ms"), FMath::IsNearlyEqual(Delays.FindRef(FIntPoint(1,1)), .30f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFX11TurnThreeTest,
	"Grimrock.RPG.FIREFX11.TurnStep3",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFX11TurnThreeTest::RunTest(const FString&)
{
	const auto Delays = FIREFX11Test::MakeTurnDelays();
	// 0.15f * 3 and 0.45f have distinct IEEE float representations;
	// compare durations with a physical-time tolerance (0.1 ms).
	TestTrue(TEXT("Third cell 450ms"), FMath::IsNearlyEqual(Delays.FindRef(FIntPoint(2,1)), .45f, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFX11IslandsTest,
	"Grimrock.RPG.FIREFX11.DisconnectedIslands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFX11IslandsTest::RunTest(const FString&)
{
	const TArray<FIntPoint> Islands = {FIntPoint(0,0),FIntPoint(4,0)};
	TMap<FIntPoint,float> Delays;
	FGridFireIgnitionWave::BuildDelays(FIntPoint(0,0),Islands,.15f,Delays);
	TestEqual(TEXT("Two islands only"),Delays.Num(),2);
	TestTrue(TEXT("Distant island starts after 600ms"),
		FMath::IsNearlyEqual(Delays.FindRef(FIntPoint(4,0)),.6f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFX11GrowthTest,
	"Grimrock.RPG.FIREFX11.CellGrowth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFX11GrowthTest::RunTest(const FString&)
{
	TestTrue(TEXT("Start 0.1/0.9"),
		FMath::IsNearlyEqual(FGridFireIgnitionWave::CoverageFraction(.1f,.9f,0.f),1.f/9.f));
	TestTrue(TEXT("Finish full coverage"),
		FMath::IsNearlyEqual(FGridFireIgnitionWave::CoverageFraction(.1f,.9f,1.f),1.f));
	TestTrue(TEXT("Smooth monotonic growth"),
		FGridFireIgnitionWave::CoverageFraction(.1f,.9f,.25f) <
		FGridFireIgnitionWave::CoverageFraction(.1f,.9f,.75f));
	return true;
}

#endif
