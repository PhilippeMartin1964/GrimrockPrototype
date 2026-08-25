#include "Magic/GridSpellPresentation.h"

#include "Runtime/Combat/GridCombatProjectileActor.h"

bool FGridSpellPresentationService::BuildPlan(const FGridSpellDefinition& Definition, const FGridSpellResolvedTarget& ResolvedTarget,
	const FGridSpellPresentationProfile& Profile, const FVector& SourceWorldLocation, const FVector& TargetWorldLocation, FGridSpellPresentationPlan& OutPlan)
{
	OutPlan = FGridSpellPresentationPlan();
	if (FGridSpellContract::ValidateDefinition(Definition) != EGridSpellValidationError::None || !Profile.IsValid() || SourceWorldLocation.ContainsNaN() ||
		TargetWorldLocation.ContainsNaN())
	{
		return false;
	}

	OutPlan.SpellId = Definition.SpellId;
	OutPlan.ResolvedTarget = ResolvedTarget;
	OutPlan.SourceWorldLocation = SourceWorldLocation;
	OutPlan.TargetWorldLocation = TargetWorldLocation;
	OutPlan.bLaunchProjectile = Profile.Projectile.bEnabled;
	OutPlan.ProjectileTravelDurationSeconds = Profile.Projectile.bEnabled ? Profile.Projectile.TravelDurationSeconds : 0.0f;

	OutPlan.Events.Add(EGridSpellPresentationEvent::CastStarted);
	if (OutPlan.bLaunchProjectile)
	{
		OutPlan.Events.Add(EGridSpellPresentationEvent::ProjectileLaunched);
	}
	OutPlan.Events.Add(EGridSpellPresentationEvent::Impact);
	OutPlan.Events.Add(EGridSpellPresentationEvent::Completed);
	return OutPlan.IsValid();
}

float FGridSpellPresentationService::CalculateProjectileLaunchDelay(float ImpactTimeSeconds, const FGridSpellPresentationProfile& Profile)
{
	return Profile.Projectile.bEnabled ? AGridCombatProjectileActor::CalculateLaunchDelay(ImpactTimeSeconds, Profile.Projectile.TravelDurationSeconds) : 0.0f;
}

FVector FGridSpellPresentationService::EvaluateProjectileTrajectory(const FVector& Source, const FVector& Target, float Alpha)
{
	return AGridCombatProjectileActor::EvaluateTrajectoryLocation(Source, Target, Alpha);
}
