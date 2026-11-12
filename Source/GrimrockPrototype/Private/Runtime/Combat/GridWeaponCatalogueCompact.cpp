#include "Runtime/Combat/GridWeaponCatalogueAsset.h"

namespace
{
	int32 CalculateGrowthValue(const int32 Base, const int32 Level,
		const int32 Every, const int32 Amount)
	{
		const int64 Delta = static_cast<int64>((Level - 1) / Every) * Amount;
		return static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(Base) + Delta,
			static_cast<int64>(MIN_int32), static_cast<int64>(MAX_int32)));
	}

	FGridWeaponIntegerGrowthRule FitExactGrowth(const TArray<int32>& Values)
	{
		// The candidate set includes every slope that explains at least one
		// observed level, plus zero; 1..20 intervals cover arbitrary tiering.
		FGridWeaponIntegerGrowthRule Best;
		int32 BestExceptions = MAX_int32;
		bool bFound = false;
		for (int32 Every = 1; Every <= 20; ++Every)
		{
			TSet<int32> Amounts;
			Amounts.Add(0);
			for (int32 Index = 1; Index < 20; ++Index)
			{
				const int32 Steps = Index / Every;
				if (Steps <= 0) continue;
				const int64 Difference = static_cast<int64>(Values[Index]) - Values[0];
				if (Difference % Steps == 0 &&
					Difference / Steps >= MIN_int32 && Difference / Steps <= MAX_int32)
				{
					Amounts.Add(static_cast<int32>(Difference / Steps));
				}
			}

			for (const int32 Amount : Amounts)
			{
				int32 MismatchCount = 0;
				for (int32 Level = 2; Level <= 20; ++Level)
				{
					MismatchCount += CalculateGrowthValue(Values[0], Level, Every, Amount) != Values[Level - 1] ? 1 : 0;
				}
				const bool bBetter = !bFound || MismatchCount < BestExceptions ||
					(MismatchCount == BestExceptions &&
					 (FMath::Abs(static_cast<int64>(Amount)) < FMath::Abs(static_cast<int64>(Best.StepAmount)) ||
					  (FMath::Abs(static_cast<int64>(Amount)) == FMath::Abs(static_cast<int64>(Best.StepAmount)) &&
					   (Every < Best.StepEveryLevels ||
						(Every == Best.StepEveryLevels && Amount < Best.StepAmount)))));
				if (!bBetter) continue;
				bFound = true;
				BestExceptions = MismatchCount;
				Best.StepEveryLevels = Every;
				Best.StepAmount = Amount;
			}
		}
		for (int32 Level = 2; Level <= 20; ++Level)
		{
			if (CalculateGrowthValue(Values[0], Level, Best.StepEveryLevels, Best.StepAmount) != Values[Level - 1])
			{
				FGridWeaponGrowthException& Exception = Best.Exceptions.AddDefaulted_GetRef();
				Exception.ItemLevel = Level;
				Exception.Value = Values[Level - 1];
			}
		}
		return Best;
	}

	enum class ECompressedStat : uint8
	{
		DiceCount, DiceSides, FlatBonus, PhysicalArmor, MagicalArmor
	};

	int32 ReadStat(const FGridWeaponLevelProfile& Profile, ECompressedStat Stat)
	{
		switch (Stat)
		{
			case ECompressedStat::DiceCount: return Profile.DamageDice.DiceCount;
			case ECompressedStat::DiceSides: return Profile.DamageDice.DiceSides;
			case ECompressedStat::FlatBonus: return Profile.DamageDice.FlatBonus;
			case ECompressedStat::PhysicalArmor: return Profile.PhysicalArmor;
			default: return Profile.MagicalArmor;
		}
	}
}

bool FGridWeaponIntegerGrowthRule::IsValid() const
{
	if (StepEveryLevels < 1 || StepEveryLevels > 20) return false;
	TSet<int32> SeenLevels;
	for (const FGridWeaponGrowthException& Exception : Exceptions)
	{
		if (Exception.ItemLevel < 2 || Exception.ItemLevel > 20 ||
			SeenLevels.Contains(Exception.ItemLevel))
		{
			return false;
		}
		SeenLevels.Add(Exception.ItemLevel);
	}
	return true;
}

int32 FGridWeaponIntegerGrowthRule::Resolve(int32 ItemLevel, int32 N1Value) const
{
	int32 Value = CalculateGrowthValue(N1Value, ItemLevel, FMath::Max(1, StepEveryLevels), StepAmount);
	for (const FGridWeaponGrowthException& Exception : Exceptions)
	{
		if (Exception.ItemLevel == ItemLevel)
		{
			Value = Exception.Value;
			break;
		}
	}
	return Value;
}

bool FGridWeaponArchetypeProgression::TryBuildProfile(int32 ItemLevel, FGridWeaponLevelProfile& OutProfile) const
{
	OutProfile = FGridWeaponLevelProfile();
	if (ItemLevel < 1 || ItemLevel > 20 || BaseProfile.ItemLevel != 1 ||
		!DiceCountGrowth.IsValid() || !DiceSidesGrowth.IsValid() ||
		!FlatBonusGrowth.IsValid() || !PhysicalArmorGrowth.IsValid() ||
		!MagicalArmorGrowth.IsValid())
	{
		return false;
	}
	OutProfile = BaseProfile;
	OutProfile.ItemLevel = ItemLevel;
	OutProfile.DamageDice.DiceCount = DiceCountGrowth.Resolve(ItemLevel, BaseProfile.DamageDice.DiceCount);
	OutProfile.DamageDice.DiceSides = DiceSidesGrowth.Resolve(ItemLevel, BaseProfile.DamageDice.DiceSides);
	OutProfile.DamageDice.FlatBonus = FlatBonusGrowth.Resolve(ItemLevel, BaseProfile.DamageDice.FlatBonus);
	OutProfile.PhysicalArmor = PhysicalArmorGrowth.Resolve(ItemLevel, BaseProfile.PhysicalArmor);
	OutProfile.MagicalArmor = MagicalArmorGrowth.Resolve(ItemLevel, BaseProfile.MagicalArmor);
	return OutProfile.IsValidProfile();
}

const TArray<FGridWeaponLevelProfile>* UGridWeaponCatalogueAsset::GetRuntimeProfiles() const
{
	if (!Profiles.IsEmpty())
	{
		return &Profiles;
	}
	if (ArchetypeProgressions.Num() != 82)
	{
		return nullptr;
	}
	if (ExpandedProfileCache.Num() == 82 * 20)
	{
		return &ExpandedProfileCache;
	}
	ExpandedProfileCache.Reset();
	ExpandedProfileCache.Reserve(82 * 20);
	for (const FGridWeaponArchetypeProgression& Progression : ArchetypeProgressions)
	{
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			FGridWeaponLevelProfile Profile;
			if (!Progression.TryBuildProfile(Level, Profile))
			{
				ExpandedProfileCache.Reset();
				return nullptr;
			}
			ExpandedProfileCache.Add(MoveTemp(Profile));
		}
	}
	return &ExpandedProfileCache;
}

bool UGridWeaponCatalogueAsset::BuildCompactCandidate(
	TArray<FGridWeaponArchetypeProgression>& OutProgressions, int32& OutExceptionCount) const
{
	OutProgressions.Reset();
	OutExceptionCount = 0;
	// Only the full, valid 1640-profile source may be compacted. Do not
	// derive new balancing data from an already compacted or partial asset.
	if (Profiles.Num() != 82 * 20 || !ArchetypeProgressions.IsEmpty() || !IsValidCatalogue())
	{
		return false;
	}

	for (int32 Ordinal = 1; Ordinal <= 82; ++Ordinal)
	{
		const FName Id(*FString::Printf(TEXT("ARM%03d"), Ordinal));
		TArray<const FGridWeaponLevelProfile*> Source;
		Source.Reserve(20);
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			const FGridWeaponLevelProfile* Profile = FindProfile(Id, Level);
			if (!Profile)
			{
				OutProgressions.Reset();
				return false;
			}
			Source.Add(Profile);
		}
		FGridWeaponArchetypeProgression Progression;
		Progression.BaseProfile = *Source[0];

		const ECompressedStat Stats[] =
		{
			ECompressedStat::DiceCount, ECompressedStat::DiceSides,
			ECompressedStat::FlatBonus, ECompressedStat::PhysicalArmor,
			ECompressedStat::MagicalArmor
		};
		FGridWeaponIntegerGrowthRule* Rules[] =
		{
			&Progression.DiceCountGrowth, &Progression.DiceSidesGrowth,
			&Progression.FlatBonusGrowth, &Progression.PhysicalArmorGrowth,
			&Progression.MagicalArmorGrowth
		};
		for (int32 StatIndex = 0; StatIndex < 5; ++StatIndex)
		{
			TArray<int32> Values;
			Values.Reserve(20);
			for (const FGridWeaponLevelProfile* Profile : Source)
			{
				Values.Add(ReadStat(*Profile, Stats[StatIndex]));
			}
			*Rules[StatIndex] = FitExactGrowth(Values);
			OutExceptionCount += Rules[StatIndex]->Exceptions.Num();
		}

		// Exact parity gate for every level and every mutable scalar.
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			FGridWeaponLevelProfile Restored;
			if (!Progression.TryBuildProfile(Level, Restored) ||
				Restored.DamageDice.DiceCount != Source[Level-1]->DamageDice.DiceCount ||
				Restored.DamageDice.DiceSides != Source[Level-1]->DamageDice.DiceSides ||
				Restored.DamageDice.FlatBonus != Source[Level-1]->DamageDice.FlatBonus ||
				Restored.PhysicalArmor != Source[Level-1]->PhysicalArmor ||
				Restored.MagicalArmor != Source[Level-1]->MagicalArmor)
			{
				OutProgressions.Reset();
				OutExceptionCount = 0;
				return false;
			}
		}
		OutProgressions.Add(MoveTemp(Progression));
	}
	return OutProgressions.Num() == 82;
}

#if WITH_EDITOR
void UGridWeaponCatalogueAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	InvalidateProfileCache();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
