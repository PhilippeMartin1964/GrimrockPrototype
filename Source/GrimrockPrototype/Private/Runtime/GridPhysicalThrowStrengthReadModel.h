#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Same read-only throw gate for menu, controller and actual projectile launch. */
struct FGridPhysicalThrowStrengthReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	bool bOneHanded = false;
	bool bCanThrow = false;
	float UnitWeightKg = 0.0f;
	float MaxWeightKg = 0.0f;
	float SpeedScale = 0.0f;
};

struct FGridPhysicalThrowStrengthReadModel
{
	static FGridPhysicalThrowStrengthReadView Resolve(
		const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Instance, int32 Strength)
	{
		FGridPhysicalThrowStrengthReadView Out;
		if (!IsValid(LegacyDefinition) || !Instance.IsValid() ||
			Instance.ItemDefinitionId != LegacyDefinition->ItemDefinitionId)
		{
			return Out;
		}
		Out.MaxWeightKg = LegacyDefinition->GetMaxThrowableWeightForStrength(Strength);
		if (LegacyDefinition->bUseWeaponCatalogue && Registry &&
			Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			Out.bCanonicalIdentity = true;
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical) ||
				!Canonical.bIsArmament)
			{
				return Out; // A known invalid N0 does not fall back to stale copies.
			}
			Out.bResolved = true;
			Out.bOneHanded = Canonical.Weapon.HandPolicy != EGridCanonicalHandPolicy::TwoHanded &&
				Canonical.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand);
			Out.UnitWeightKg = Canonical.WeightKg;
			const float Speed = Canonical.Weapon.Projectile.bEnabled
				? Canonical.Weapon.Projectile.InitialSpeed : LegacyDefinition->ThrowSpeed;
			if (Strength <= 0 || !Out.bOneHanded || !FMath::IsFinite(Speed) ||
				Speed <= KINDA_SMALL_NUMBER || !FMath::IsFinite(Out.UnitWeightKg) ||
				Out.UnitWeightKg < 0.0f || Out.UnitWeightKg > Out.MaxWeightKg + KINDA_SMALL_NUMBER)
			{
				return Out;
			}
			// Numeric formula mirrors the pre-migration throw speed curve exactly;
			// only the unit mass switches to ItemSystem's authoritative WeightKg.
			const float MaxWeight = FMath::Max(KINDA_SMALL_NUMBER, Out.MaxWeightKg);
			const float WeightRatio = FMath::Clamp(Out.UnitWeightKg / MaxWeight, 0.0f, 1.0f);
			const float StrengthFactor = FMath::Clamp(
				FMath::Sqrt(FMath::Max(1.0f, static_cast<float>(Strength)) / 10.0f), 0.65f, 1.5f);
			const float WeightFactor = FMath::Lerp(1.0f, 0.65f, WeightRatio);
			Out.SpeedScale = FMath::Clamp(StrengthFactor * WeightFactor, 0.5f, 1.5f);
			Out.bCanThrow = Out.SpeedScale > 0.0f;
			return Out;
		}
		// Nonregistered and non-ARM identities retain all legacy throw semantics.
		Out.bResolved = true;
		Out.bOneHanded = LegacyDefinition->GetEffectiveHandUsage() == EGridItemHandUsage::OneHanded;
		Out.UnitWeightKg = LegacyDefinition->Weight;
		Out.SpeedScale = LegacyDefinition->GetThrowSpeedScaleForStrength(Strength);
		Out.bCanThrow = Out.SpeedScale > 0.0f;
		return Out;
	}
};
