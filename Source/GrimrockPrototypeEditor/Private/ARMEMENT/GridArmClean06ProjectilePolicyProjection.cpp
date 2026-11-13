#include "ARMEMENT/GridArmClean06ProjectilePolicyProjection.h"

bool GridArmClean06ProjectilePolicyProjection::Matches(
	const UGridItemDefinitionAsset& Source,
	const FGridCanonicalProjectilePolicy& Destination)
{
	return Source.bCombatThrowWeapon == Destination.bEnabled &&
		(!Source.bCombatThrowWeapon ||
			(Source.ThrowSpeed == Destination.InitialSpeed &&
			 Source.ThrowArc == Destination.Arc &&
			 Source.ThrowLifeSeconds == Destination.FlightLifetime &&
			 Source.ThrowImpactDropOffset == Destination.ImpactDropOffset));
}

bool GridArmClean06ProjectilePolicyProjection::TryProject(
	const FGridWeaponPipelineRow& Row,
	const UGridItemDefinitionAsset& Source,
	FGridCanonicalWeaponArchetype& Destination)
{
	if (Row.ProductionState != TEXT("Active") ||
		Destination.ArchetypeId != FName(*Row.ArchetypeId) ||
		Source.WeaponArchetypeId != Destination.ArchetypeId ||
		Source.ItemDefinitionId != FName(*Row.ItemDefinitionId) ||
		!Destination.bHandAndKindProven ||
		Row.IsThrowable() != Source.bCombatThrowWeapon ||
		Destination.Projectile.bEnabled)
	{
		return false;
	}
	if (!Source.bCombatThrowWeapon)
	{
		return Matches(Source, Destination);
	}
	if (Destination.Kind != EGridCanonicalArmamentKind::Weapon ||
		!FMath::IsFinite(Source.ThrowSpeed) || Source.ThrowSpeed <= 0.0f ||
		!FMath::IsFinite(Source.ThrowArc) || Source.ThrowArc < 0.0f ||
		!FMath::IsFinite(Source.ThrowLifeSeconds) || Source.ThrowLifeSeconds <= 0.0f ||
		!FMath::IsFinite(Source.ThrowImpactDropOffset) ||
		Source.ThrowImpactDropOffset < 0.0f)
	{
		return false;
	}
	Destination.Projectile.bEnabled = true;
	Destination.Projectile.InitialSpeed = Source.ThrowSpeed;
	Destination.Projectile.Arc = Source.ThrowArc;
	Destination.Projectile.FlightLifetime = Source.ThrowLifeSeconds;
	Destination.Projectile.ImpactDropOffset = Source.ThrowImpactDropOffset;
	return Matches(Source, Destination);
}
