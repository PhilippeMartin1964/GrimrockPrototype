#include "RPG/StatusEffects/GridStatusEffectPeriodicDamageResolver.h"

#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatResolver.h"

bool FGridStatusEffectPeriodicDamageResolver::Resolve(const FGridStatusEffectRuntimeState& State, const FGridAttackTargetStats& Target,
	FGridStatusEffectPeriodicDamageResolution& OutResolution, FString& OutError)
{
	OutResolution = FGridStatusEffectPeriodicDamageResolution();

	if (!State.IsValid())
	{
		OutError = TEXT("Periodic damage requires a valid runtime status state.");
		return false;
	}
	if (!IsValid(State.DefinitionAsset))
	{
		OutError = FString::Printf(TEXT("EffectId '%s' has no runtime definition asset."), *State.EffectId.ToString());
		return false;
	}

	const UGridStatusEffectDefinitionAsset& Definition = *State.DefinitionAsset;
	if (Definition.EffectId != State.EffectId)
	{
		OutError = FString::Printf(
			TEXT("Runtime EffectId '%s' does not match definition EffectId '%s'."), *State.EffectId.ToString(), *Definition.EffectId.ToString());
		return false;
	}
	if (!Definition.PeriodicDamage.IsEnabled())
	{
		OutError = FString::Printf(TEXT("EffectId '%s' does not declare periodic damage."), *State.EffectId.ToString());
		return false;
	}
	if (State.DurationUnit == EGridStatusEffectDurationUnit::Permanent)
	{
		OutError = TEXT("Permanent status effects cannot resolve periodic damage in MON16.3.");
		return false;
	}

	const int64 ScaledDamage = static_cast<int64>(Definition.PeriodicDamage.DamagePerStack) * static_cast<int64>(State.StackCount);
	const int32 RawDamage = static_cast<int32>(FMath::Clamp<int64>(ScaledDamage, 0, MAX_int32));
	if (RawDamage <= 0)
	{
		OutError = TEXT("Periodic damage resolved to zero raw damage.");
		return false;
	}

	OutResolution.EffectId = State.EffectId;
	OutResolution.SourceId = State.SourceId;
	OutResolution.DamageType = Definition.PeriodicDamage.DamageType;
	OutResolution.StackCount = State.StackCount;
	OutResolution.RawDamage = RawDamage;
	OutResolution.DamageResult = FGridCombatResolver::ResolveDirectDamage(Target, Definition.PeriodicDamage.DamageType, RawDamage);
	OutError.Reset();
	return true;
}
