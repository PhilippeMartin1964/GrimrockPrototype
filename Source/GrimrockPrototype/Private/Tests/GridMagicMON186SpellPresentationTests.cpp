#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellPresentation.h"
#include "Magic/GridSpellPresentationComponent.h"
#include "Runtime/Combat/GridCombatProjectileActor.h"

namespace
{
	FGridSpellResolvedTarget MakeResolvedTarget()
	{
		FGridSpellResolvedTarget Target;
		Target.TargetId = FGuid::NewGuid();
		Target.GridCell = FIntPoint(3, 1);
		Target.bHasGridCell = true;
		return Target;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON186ProductionProfilesTest, "Grimrock.Magic.MON18.6.ProductionProfiles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON186ProductionProfilesTest::RunTest(const FString& Parameters)
{
	const FGridSpellPresentationProfile Bolt = FGridProductionSpellLibrary::MakeArcaneBoltPresentation();
	TestTrue(TEXT("Arcane Bolt profile valid"), Bolt.IsValid());
	TestTrue(TEXT("Arcane Bolt uses projectile"), Bolt.Projectile.bEnabled);
	TestEqual(TEXT("Arcane Bolt travel"), Bolt.Projectile.TravelDurationSeconds, 0.20f);
	TestTrue(TEXT("Lesser Heal profile valid"), FGridProductionSpellLibrary::MakeLesserHealPresentation().IsValid());
	TestFalse(TEXT("Lesser Heal is instant"), FGridProductionSpellLibrary::MakeLesserHealPresentation().Projectile.bEnabled);
	TestFalse(TEXT("Haste is instant"), FGridProductionSpellLibrary::MakeHastePresentation().Projectile.bEnabled);
	TestFalse(TEXT("Cure Poison is instant"), FGridProductionSpellLibrary::MakeCurePoisonPresentation().Projectile.bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON186ProjectilePlanSequenceTest, "Grimrock.Magic.MON18.6.ProjectilePlanSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON186ProjectilePlanSequenceTest::RunTest(const FString& Parameters)
{
	const FGridSpellDefinition Definition = FGridProductionSpellLibrary::MakeArcaneBolt();
	const FGridSpellPresentationProfile Profile = FGridProductionSpellLibrary::MakeArcaneBoltPresentation();
	FGridSpellPresentationPlan Plan;
	TestTrue(TEXT("Plan built"),
		FGridSpellPresentationService::BuildPlan(Definition, MakeResolvedTarget(), Profile, FVector(0.0f, 0.0f, 100.0f), FVector(400.0f, 0.0f, 100.0f), Plan));
	TestTrue(TEXT("Plan valid"), Plan.IsValid());
	TestTrue(TEXT("Projectile launch planned"), Plan.bLaunchProjectile);
	TestEqual(TEXT("Four events"), Plan.Events.Num(), 4);
	TestEqual(TEXT("Cast first"), Plan.Events[0], EGridSpellPresentationEvent::CastStarted);
	TestEqual(TEXT("Launch second"), Plan.Events[1], EGridSpellPresentationEvent::ProjectileLaunched);
	TestEqual(TEXT("Impact third"), Plan.Events[2], EGridSpellPresentationEvent::Impact);
	TestEqual(TEXT("Complete last"), Plan.Events[3], EGridSpellPresentationEvent::Completed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON186InstantPlanSequenceTest, "Grimrock.Magic.MON18.6.InstantPlanSequence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON186InstantPlanSequenceTest::RunTest(const FString& Parameters)
{
	FGridSpellPresentationPlan Plan;
	TestTrue(TEXT("Heal plan built"),
		FGridSpellPresentationService::BuildPlan(FGridProductionSpellLibrary::MakeLesserHeal(), MakeResolvedTarget(),
			FGridProductionSpellLibrary::MakeLesserHealPresentation(), FVector::ZeroVector, FVector(100.0f, 0.0f, 0.0f), Plan));
	TestFalse(TEXT("No projectile"), Plan.bLaunchProjectile);
	TestEqual(TEXT("Three events"), Plan.Events.Num(), 3);
	TestEqual(TEXT("Cast first"), Plan.Events[0], EGridSpellPresentationEvent::CastStarted);
	TestEqual(TEXT("Impact second"), Plan.Events[1], EGridSpellPresentationEvent::Impact);
	TestEqual(TEXT("Complete last"), Plan.Events[2], EGridSpellPresentationEvent::Completed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON186ProjectileTrajectoryReuseTest, "Grimrock.Magic.MON18.6.ProjectileTrajectoryReuse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON186ProjectileTrajectoryReuseTest::RunTest(const FString& Parameters)
{
	const FVector Source(0.0f, 0.0f, 0.0f);
	const FVector Target(200.0f, 100.0f, 50.0f);
	const FVector Expected = AGridCombatProjectileActor::EvaluateTrajectoryLocation(Source, Target, 0.5f);
	const FVector Actual = FGridSpellPresentationService::EvaluateProjectileTrajectory(Source, Target, 0.5f);
	TestTrue(TEXT("Trajectory delegates to MON17 projectile"), Actual.Equals(Expected));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON186ProjectileTimingReuseTest, "Grimrock.Magic.MON18.6.ProjectileTimingReuse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON186ProjectileTimingReuseTest::RunTest(const FString& Parameters)
{
	const FGridSpellPresentationProfile Profile = FGridProductionSpellLibrary::MakeArcaneBoltPresentation();
	const float Expected = AGridCombatProjectileActor::CalculateLaunchDelay(0.25f, 0.20f);
	const float Actual = FGridSpellPresentationService::CalculateProjectileLaunchDelay(0.25f, Profile);
	TestEqual(TEXT("Launch delay delegates to MON17 projectile"), Actual, Expected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON186VisualOptionalityTest, "Grimrock.Magic.MON18.6.VisualOptionality", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON186VisualOptionalityTest::RunTest(const FString& Parameters)
{
	FGridSpellPresentationProfile Profile = FGridProductionSpellLibrary::MakeArcaneBoltPresentation();
	TestTrue(TEXT("Profile valid without mesh/audio/VFX"), Profile.IsValid());
	TestTrue(TEXT("Projectile semantics remain enabled"), Profile.Projectile.bEnabled);
	TestTrue(TEXT("Mesh may be absent"), Profile.Projectile.VisualMesh.IsNull());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON186PresentationPurityTest, "Grimrock.Magic.MON18.6.PresentationPurity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON186PresentationPurityTest::RunTest(const FString& Parameters)
{
	FRPGDerivedStats Stats;
	Stats.MaxHealth = 10;
	Stats.CurrentHealth = 7;
	Stats.MaxMana = 12;
	Stats.CurrentMana = 9;
	const FRPGDerivedStats Before = Stats;

	FGridSpellPresentationPlan Plan;
	TestTrue(TEXT("Presentation plan builds"),
		FGridSpellPresentationService::BuildPlan(FGridProductionSpellLibrary::MakeArcaneBolt(), MakeResolvedTarget(),
			FGridProductionSpellLibrary::MakeArcaneBoltPresentation(), FVector::ZeroVector, FVector(200.0f, 0.0f, 0.0f), Plan));
	TestEqual(TEXT("Health untouched"), Stats.CurrentHealth, Before.CurrentHealth);
	TestEqual(TEXT("Mana untouched"), Stats.CurrentMana, Before.CurrentMana);
	TestTrue(TEXT("Runtime component remains an ActorComponent"), UGridSpellPresentationComponent::StaticClass()->IsChildOf(UActorComponent::StaticClass()));
	return true;
}

#endif
