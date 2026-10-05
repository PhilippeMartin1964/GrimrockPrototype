#include "RPG/StatusEffects/GridCombatStatusApplicationResolver.h"

#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectPersistence.h"
#include "UObject/UObjectIterator.h"

int32 FGridCombatStatusApplicationResolver::GetPostPhysicalArmor(
	const FGridAttackTargetStats& TargetBefore, const FGridAttackResult* AttackResult)
{
	return FMath::Max(0, TargetBefore.PhysicalArmor - (AttackResult ? FMath::Max(0, AttackResult->PhysicalArmorDamage) : 0));
}

int32 FGridCombatStatusApplicationResolver::GetPostMagicalArmor(
	const FGridAttackTargetStats& TargetBefore, const FGridAttackResult* AttackResult)
{
	return FMath::Max(0, TargetBefore.MagicalArmor - (AttackResult ? FMath::Max(0, AttackResult->MagicalArmorDamage) : 0));
}

int32 FGridCombatStatusApplicationResolver::GetPostHealth(
	const FGridAttackTargetStats& TargetBefore, const FGridAttackResult* AttackResult)
{
	return AttackResult ? FMath::Max(0, AttackResult->TargetHealthAfter) : FMath::Max(0, TargetBefore.CurrentHealth);
}

bool FGridCombatStatusApplicationResolver::IsEligible(const FGridCombatStatusApplicationProfile& Profile,
	const FGridAttackTargetStats& TargetBefore, const FGridAttackResult* AttackResult)
{
	if (!Profile.IsValid() || GetPostHealth(TargetBefore, AttackResult) <= 0)
	{
		return false;
	}

	if (Profile.Trigger == EGridCombatStatusApplicationTrigger::AfterSuccessfulHit && (!AttackResult || !AttackResult->bHit))
	{
		return false;
	}

	switch (Profile.ArmorGate)
	{
		case EGridCombatStatusArmorGate::None:
			return true;
		case EGridCombatStatusArmorGate::PhysicalArmorDepleted:
			return GetPostPhysicalArmor(TargetBefore, AttackResult) <= 0;
		case EGridCombatStatusArmorGate::MagicalArmorDepleted:
			return GetPostMagicalArmor(TargetBefore, AttackResult) <= 0;
		default:
			return false;
	}
}

UGridStatusEffectDefinitionAsset* FGridCombatStatusApplicationResolver::ResolveDefinition(FName StatusEffectId)
{
	if (StatusEffectId.IsNone())
	{
		return nullptr;
	}

	if (UGridStatusEffectDefinitionAsset* Definition = FGridStatusEffectPersistence::ResolveDefinitionByEffectId(StatusEffectId))
	{
		return Definition;
	}

	for (TObjectIterator<UGridStatusEffectDefinitionAsset> It; It; ++It)
	{
		UGridStatusEffectDefinitionAsset* Definition = *It;
		if (IsValid(Definition) && Definition->EffectId == StatusEffectId && Definition->IsValidDefinition())
		{
			return Definition;
		}
	}
	return nullptr;
}

bool FGridCombatStatusApplicationResolver::WouldAnyMutate(const TArray<FGridCombatStatusApplicationProfile>& Profiles, const FGuid& SourceId,
	const FGridAttackTargetStats& TargetBefore, const FGridAttackResult* AttackResult, const FGridStatusEffectCollection& CurrentEffects)
{
	for (const FGridCombatStatusApplicationProfile& Profile : Profiles)
	{
		if (!IsEligible(Profile, TargetBefore, AttackResult))
		{
			continue;
		}

		UGridStatusEffectDefinitionAsset* Definition = ResolveDefinition(Profile.StatusEffectId);
		if (!IsValid(Definition))
		{
			continue;
		}

		FGridStatusEffectCollection Candidate = CurrentEffects;
		FGridStatusEffectApplyResult Result;
		FString Error;
		if (Candidate.TryApply(*Definition, SourceId, Profile.InitialStackCount, Profile.DurationOverride, Profile.PotencyOverride, Result, Error) &&
			Result.DidMutate())
		{
			return true;
		}
	}
	return false;
}
