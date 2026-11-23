#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Same read-only throw gate for menu, controller and actual projectile launch. */
struct FGridPhysicalThrowStrengthReadView
{
	bool bRegisteredIdentity = false;
	bool bResolved = false;
	bool bOneHanded = false;
	bool bCanThrow = false;
	float UnitWeightKg = 0.0f;
	float MaxWeightKg = 0.0f;
	float SpeedScale = 0.0f;
};

struct FGridPhysicalThrowStrengthReadModel
{
	static bool IsCanonicalOneHanded(const FGridResolvedItem& Item)
	{
		if (!Item.bIsArmament)
		{
			return Item.NonArmHandPolicy == EGridNonArmamentHandPolicy::OneHanded;
		}
		const auto& Projectile = Item.Weapon.Projectile;
		return Projectile.bGenericThrowAuthored ? Projectile.bGenericThrowOneHanded :
			Item.Weapon.HandPolicy != EGridHandPolicy::TwoHanded &&
			Item.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand);
	}

	static FGridPhysicalThrowStrengthReadView Resolve(
		const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Instance, int32 Strength)
	{
		FGridPhysicalThrowStrengthReadView Out;
		// Membership must be recognized BEFORE IsValid(): an invalid
		// registered N21 is not an unknown legacy identity.
		Out.bRegisteredIdentity = Registry && Registry->ContainsItem(Instance.ItemDefinitionId);
		if (!Instance.IsValid())
		{
			return Out;
		}
		if (Out.bRegisteredIdentity)
		{
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical) ||
				!Canonical.IsValid() || Canonical.ItemId != Instance.ItemDefinitionId ||
				Canonical.ItemLevel != Instance.ItemLevel || Instance.Quantity > FMath::Max(1, Canonical.MaxStack) ||
				(!Canonical.bIsArmament && Instance.ItemLevel != 0))
			{
				return Out; // A known invalid N0 does not fall back to stale copies.
			}
			// Same global rule as GetMaxThrowableWeightForStrength: 0.25 kg
			// for each point of effective Strength, independent of old DA.
			Out.MaxWeightKg = FMath::Max(0, Strength) * 0.25f;
			Out.bResolved = true;
			Out.bOneHanded = IsCanonicalOneHanded(Canonical);
			Out.UnitWeightKg = Canonical.WeightKg;
			const float Speed = !Canonical.bIsArmament ? Canonical.GenericThrow.InitialSpeed :
				(Canonical.Weapon.Projectile.bEnabled || Canonical.Weapon.Projectile.bGenericThrowAuthored)
				? Canonical.Weapon.Projectile.InitialSpeed
				: 0.0f;
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
		// Identities absent from ItemSystem keep legacy physics during 06.3.
		// Only active, registry-less Automation fixtures may use old physics.
#if !WITH_DEV_AUTOMATION_TESTS
		return Out;
#else
		if (!GIsAutomationTesting || Registry) return Out;
		if (!IsValid(LegacyDefinition) ||
			LegacyDefinition->ItemDefinitionId != Instance.ItemDefinitionId)
		{
			return Out;
		}
		Out.MaxWeightKg = LegacyDefinition->GetMaxThrowableWeightForStrength(Strength);
		Out.bResolved = true;
		Out.bOneHanded = LegacyDefinition->GetEffectiveHandUsage() == EGridItemHandUsage::OneHanded;
		Out.UnitWeightKg = LegacyDefinition->Weight;
		Out.SpeedScale = LegacyDefinition->GetThrowSpeedScaleForStrength(Strength);
		Out.bCanThrow = Out.SpeedScale > 0.0f;
		return Out;
#endif
	}
};
