#include "ARMEMENT/GridArmClean06ProductionNumericsCandidate.h"

#include "ARMEMENT/GridArmClean06ClassAffinityProjection.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
	bool SameNumbers(const FGridWeaponLevelProfile& Source,
		const FGridCanonicalResolvedWeapon& Candidate)
	{
		return Source.DamageType == Candidate.DamageType &&
			Source.PhysicalSubtype == Candidate.PhysicalSubtype &&
			Source.ScalingAttribute == Candidate.ScalingAttribute &&
			Source.AccuracyBonus == Candidate.AccuracyBonus &&
			Source.ActionPointCost == Candidate.ActionPointCost &&
			Source.RangeCells == Candidate.RangeCells &&
			Source.PhysicalArmor == Candidate.PhysicalArmor &&
			Source.MagicalArmor == Candidate.MagicalArmor &&
			Source.DamageDice.bEnabled == Candidate.DamageDice.bEnabled &&
			Source.DamageDice.DiceCount == Candidate.DamageDice.DiceCount &&
			Source.DamageDice.DiceSides == Candidate.DamageDice.DiceSides &&
			Source.DamageDice.FlatBonus == Candidate.DamageDice.FlatBonus;
	}

	FGridCanonicalWeaponArchetype ProjectNumericBase(
		const FGridWeaponArchetypeProgression& Row)
	{
		const FGridWeaponLevelProfile& Base = Row.BaseProfile;
		FGridCanonicalWeaponArchetype Out;
		Out.ArchetypeId = Base.ArchetypeId;
		// These are structural numeric placeholders ONLY while
		// bHandAndKindProven=false. No runtime reader accepts that state.
		Out.Kind = Base.DamageDice.bEnabled ?
			EGridCanonicalArmamentKind::Weapon : EGridCanonicalArmamentKind::Shield;
		Out.HandPolicy = EGridCanonicalHandPolicy::MainHand;
		Out.bHandAndKindProven = false;
		Out.DamageType = Base.DamageType;
		Out.PhysicalSubtype = Base.PhysicalSubtype;
		Out.ScalingAttribute = Base.ScalingAttribute;
		Out.AccuracyBonus = Base.AccuracyBonus;
		Out.ActionPointCost = Base.ActionPointCost;
		Out.RangeCells = Base.RangeCells;
		Out.DamageDiceN1 = Base.DamageDice;
		Out.PhysicalArmorN1 = Base.PhysicalArmor;
		Out.MagicalArmorN1 = Base.MagicalArmor;
		Out.DiceCountGrowth = Row.DiceCountGrowth;
		Out.DiceSidesGrowth = Row.DiceSidesGrowth;
		Out.FlatBonusGrowth = Row.FlatBonusGrowth;
		Out.PhysicalArmorGrowth = Row.PhysicalArmorGrowth;
		Out.MagicalArmorGrowth = Row.MagicalArmorGrowth;
		return Out;
	}

	bool ApplyRealHandAndKind(const FGridWeaponPipelineRow& Row,
		FGridCanonicalWeaponArchetype& Candidate, bool& bAttackMain, bool& bAttackOff)
	{
		const UGridItemDefinitionAsset* Item =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		if (!IsValid(Item) || !Item->bUseWeaponCatalogue ||
			Item->ItemDefinitionId != FName(*Row.ItemDefinitionId) ||
			Item->WeaponArchetypeId != Candidate.ArchetypeId)
		{
			return false;
		}
		const bool Main = Item->CanEquipToSlot(EGridEquipmentSlot::MainHand);
		const bool Off = Item->CanEquipToSlot(EGridEquipmentSlot::OffHand);
		if (Main && Off)
		{
			Candidate.HandPolicy = Item->GetEffectiveHandUsage() ==
				EGridItemHandUsage::TwoHanded ?
				EGridCanonicalHandPolicy::TwoHanded : EGridCanonicalHandPolicy::EitherHand;
		}
		else if (Main) { Candidate.HandPolicy = EGridCanonicalHandPolicy::MainHand; }
		else if (Off) { Candidate.HandPolicy = EGridCanonicalHandPolicy::OffHand; }
		else { return false; }

		if (Row.IsFocus()) { Candidate.Kind = EGridCanonicalArmamentKind::Focus; }
		else if (Row.IsShield()) { Candidate.Kind = EGridCanonicalArmamentKind::Shield; }
		else { Candidate.Kind = EGridCanonicalArmamentKind::Weapon; }
		if ((Candidate.Kind == EGridCanonicalArmamentKind::Weapon) !=
			Candidate.DamageDiceN1.bEnabled)
		{
			return false;
		}
		// Capture the proved attack-source permissions in this dry-run report.
		// Do not write AttackSourceSlots before real ActionId+PresentationId
		// bindings are available: the canonical validator correctly rejects
		// an authorable attack slot without a bound action.
		bAttackMain = Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand);
		bAttackOff = Item->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand);
		if (Candidate.Kind != EGridCanonicalArmamentKind::Weapon &&
			(bAttackMain || bAttackOff))
		{
			return false;
		}
		Candidate.bHandAndKindProven = true;
		return true;
	}
}

bool GridArmClean06ProductionNumericsCandidate::Build(
	const UGridWeaponCatalogueAsset& Source,
	const TArray<FGridWeaponPipelineRow>& Manifest,
	UGridCanonicalWeaponCatalogueAsset& Candidate, FSummary& OutSummary)
{
	OutSummary = FSummary();
	if (Manifest.Num() != 22 || Source.ArchetypeProgressions.Num() != 82 ||
		!Source.Profiles.IsEmpty() || !Source.IsValidCatalogue() ||
		!Candidate.Archetypes.IsEmpty() || !Candidate.ClassPreferences.IsEmpty())
	{
		return false;
	}
	TMap<FName, const FGridWeaponPipelineRow*> RealRows;
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const FName ArmId(*Row.ArchetypeId);
		if (Row.ProductionState != TEXT("Active") || ArmId.IsNone() ||
			RealRows.Contains(ArmId))
		{
			return false;
		}
		RealRows.Add(ArmId, &Row);
	}
	TArray<FGridCanonicalWeaponArchetype> Staged;
	Staged.Reserve(82);
	TSet<FName> Seen;
	for (const FGridWeaponArchetypeProgression& Row : Source.ArchetypeProgressions)
	{
		const FName ArmId = Row.BaseProfile.ArchetypeId;
		if (ArmId.IsNone() || Seen.Contains(ArmId)) { return false; }
		Seen.Add(ArmId);
		FGridCanonicalWeaponArchetype& Arm =
			Staged.Add_GetRef(ProjectNumericBase(Row));
		const FGridWeaponPipelineRow* const* Concrete = RealRows.Find(ArmId);
		if (Concrete)
		{
			bool bMainAttack = false;
			bool bOffAttack = false;
			if (!ApplyRealHandAndKind(**Concrete, Arm, bMainAttack, bOffAttack))
			{
				return false;
			}
			++OutSummary.ProvenConcreteArchetypes;
			OutSummary.MainHandAttacks += bMainAttack ? 1 : 0;
			OutSummary.OffHandAttacks += bOffAttack ? 1 : 0;
		}
		else
		{
			++OutSummary.UnprovenUninstantiatedArchetypes;
		}

		for (int32 Level = 1; Level <= 20; ++Level)
		{
			FGridCanonicalResolvedWeapon Numbers;
			const FGridWeaponLevelProfile* Historical = Source.FindProfile(ArmId, Level);
			if (!Historical || !Arm.TryResolve(Level, Numbers) ||
				!SameNumbers(*Historical, Numbers))
			{
				return false;
			}
			++OutSummary.ComparedProfiles;
			if (!Numbers.DamageDice.bEnabled) { ++OutSummary.PassiveProfiles; }
		}
	}
	if (OutSummary.ProvenConcreteArchetypes != 22 ||
		OutSummary.UnprovenUninstantiatedArchetypes != 60 ||
		OutSummary.ComparedProfiles != 1640 ||
		OutSummary.PassiveProfiles != 260 ||
		OutSummary.MainHandAttacks != 19 || OutSummary.OffHandAttacks != 2)
	{
		return false;
	}

	Candidate.Archetypes = MoveTemp(Staged);
	if (!GridArmClean06ClassAffinityProjection::TryProject(Source, Candidate))
	{
		Candidate.Archetypes.Reset();
		return false;
	}
	TArray<FString> Errors;
	if (!Candidate.Validate(Errors) ||
		!GridArmClean06ClassAffinityProjection::EqualsSource(Source, Candidate))
	{
		Candidate.ClassPreferences.Reset();
		Candidate.Archetypes.Reset();
		return false;
	}
	OutSummary.Archetypes = Candidate.Archetypes.Num();
	return true;
}
