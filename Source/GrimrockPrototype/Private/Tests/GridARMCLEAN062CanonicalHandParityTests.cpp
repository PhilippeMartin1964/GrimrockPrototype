#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Serialization/Csv/CsvParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

/**
 * ARM-CLEAN06.2F: ONLY the 22 explicitly authored production ARM rows.
 * Does not enumerate unrelated packages, resolve redirectors or save assets.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ProductionARMHandsTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionHandActionParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062ProductionARMHandsTest::RunTest(const FString&)
{
	const UGridWeaponCatalogueAsset* Source = LoadObject<UGridWeaponCatalogueAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	if (!TestNotNull(TEXT("ARM source catalogue"), Source) ||
		!TestTrue(TEXT("ARM 82-source catalogue valid"), Source->IsValidCatalogue()))
	{
		return false;
	}
	FString Csv;
	if (!TestTrue(TEXT("Production manifest readable"), FFileHelper::LoadFileToString(
		Csv, *FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/Armement/WeaponProductionManifest.csv")))))
	{
		return false;
	}
	FCsvParser Parser(Csv);
	const TArray<TArray<const TCHAR*>>& Rows = Parser.GetRows();
	if (!TestEqual(TEXT("22 production rows plus header"), Rows.Num(), 23))
	{
		return false;
	}
	TSet<FName> SeenIds;
	int32 WidenedEquipWithoutAttack = 0;
	int32 NotHandHeldWithExplicitSlot = 0;
	int32 PassiveItems = 0;
	int32 FocusItems = 0;
	int32 AttacksMain = 0;
	int32 AttacksOff = 0;
	bool bAllEquivalent = true;
	for (int32 Index = 1; Index < Rows.Num(); ++Index)
	{
		const TArray<const TCHAR*>& Row = Rows[Index];
		if (Row.Num() < 5)
		{
			AddError(TEXT("Production ARM row has fewer than five CSV fields"));
			return false;
		}
		const FName ArchetypeId(Row[0]);
		const FString State(Row[1]);
		const FString BuildKind(Row[2]);
		const FName ExpectedItemId(Row[3]);
		const FString ItemPath(Row[4]);
		if (State != TEXT("Active") || SeenIds.Contains(ArchetypeId))
		{
			AddError(TEXT("Production ARM manifest has inactive/duplicate source row"));
			return false;
		}
		SeenIds.Add(ArchetypeId);
		const UGridItemDefinitionAsset* Item = LoadObject<UGridItemDefinitionAsset>(nullptr, *ItemPath);
		const FGridWeaponArchetypeProgression* Progression =
			Source->ArchetypeProgressions.FindByPredicate([ArchetypeId](const FGridWeaponArchetypeProgression& Entry)
			{
				return Entry.BaseProfile.ArchetypeId == ArchetypeId;
			});
		if (!IsValid(Item) || !Progression ||
			!Item->bUseWeaponCatalogue || Item->WeaponArchetypeId != ArchetypeId ||
			Item->ItemDefinitionId != ExpectedItemId)
		{
			AddError(FString::Printf(TEXT("%s: production item or ARM identity mismatch"), *ArchetypeId.ToString()));
			return false;
		}
		FGridCanonicalWeaponArchetype Candidate;
		const FGridWeaponLevelProfile& Base = Progression->BaseProfile;
		Candidate.ArchetypeId = ArchetypeId;
		Candidate.DamageType = Base.DamageType;
		Candidate.PhysicalSubtype = Base.PhysicalSubtype;
		Candidate.ScalingAttribute = Base.ScalingAttribute;
		Candidate.AccuracyBonus = Base.AccuracyBonus;
		Candidate.ActionPointCost = Base.ActionPointCost;
		Candidate.RangeCells = Base.RangeCells;
		Candidate.DamageDiceN1 = Base.DamageDice;
		Candidate.PhysicalArmorN1 = Base.PhysicalArmor;
		Candidate.MagicalArmorN1 = Base.MagicalArmor;
		Candidate.DiceCountGrowth = Progression->DiceCountGrowth;
		Candidate.DiceSidesGrowth = Progression->DiceSidesGrowth;
		Candidate.FlatBonusGrowth = Progression->FlatBonusGrowth;
		Candidate.PhysicalArmorGrowth = Progression->PhysicalArmorGrowth;
		Candidate.MagicalArmorGrowth = Progression->MagicalArmorGrowth;
		// Focus and Shield are EXPLICIT authored manifest kinds, never inferred
		// from absence of damage. An attacking Focus/Shield is a parity failure.
		if (BuildKind == TEXT("FocusPassive"))
		{
			Candidate.Kind = EGridCanonicalArmamentKind::Focus;
			++FocusItems;
		}
		else if (BuildKind == TEXT("ShieldPassive"))
		{
			Candidate.Kind = EGridCanonicalArmamentKind::Shield;
		}
		else
		{
			Candidate.Kind = EGridCanonicalArmamentKind::Weapon;
		}
		if (Candidate.Kind != EGridCanonicalArmamentKind::Weapon) { ++PassiveItems; }
		const bool bMain = Item->CanEquipToSlot(EGridEquipmentSlot::MainHand);
		const bool bOff = Item->CanEquipToSlot(EGridEquipmentSlot::OffHand);
		const bool bAttackMain = Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand);
		const bool bAttackOff = Item->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand);
		if (Item->GetEffectiveHandUsage() == EGridItemHandUsage::NotHandHeld &&
			(bMain || bOff))
		{
			++NotHandHeldWithExplicitSlot;
		}
		if (bMain && bOff)
		{
			Candidate.HandPolicy = Item->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded ?
				EGridCanonicalHandPolicy::TwoHanded : EGridCanonicalHandPolicy::EitherHand;
		}
		else if (bMain) { Candidate.HandPolicy = EGridCanonicalHandPolicy::MainHand; }
		else if (bOff) { Candidate.HandPolicy = EGridCanonicalHandPolicy::OffHand; }
		else
		{
			AddError(FString::Printf(TEXT("%s: no representable equipment hand"), *ArchetypeId.ToString()));
			return false;
		}
		if (bAttackMain) { Candidate.AttackSourceSlots.Add(EGridEquipmentSlot::MainHand); ++AttacksMain; }
		if (bAttackOff) { Candidate.AttackSourceSlots.Add(EGridEquipmentSlot::OffHand); ++AttacksOff; }
		if ((bMain && !Item->CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::MainHand) && !bAttackMain) ||
			(bOff && !Item->CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::OffHand) && !bAttackOff))
		{
			++WidenedEquipWithoutAttack;
		}
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			FGridCanonicalResolvedWeapon Resolved;
			const FGridWeaponLevelProfile* LegacyLevel = Source->FindProfile(ArchetypeId, Level);
			if (!LegacyLevel || !Candidate.TryResolve(Level, Resolved) ||
				Resolved.CanEquipInHand(EGridEquipmentSlot::MainHand) != bMain ||
				Resolved.CanEquipInHand(EGridEquipmentSlot::OffHand) != bOff ||
				Resolved.HasAttackSourceInHand(EGridEquipmentSlot::MainHand) != bAttackMain ||
				Resolved.HasAttackSourceInHand(EGridEquipmentSlot::OffHand) != bAttackOff ||
				Resolved.DamageDice.bEnabled != LegacyLevel->DamageDice.bEnabled ||
				Resolved.DamageDice.DiceCount != LegacyLevel->DamageDice.DiceCount ||
				Resolved.DamageDice.DiceSides != LegacyLevel->DamageDice.DiceSides ||
				Resolved.DamageDice.FlatBonus != LegacyLevel->DamageDice.FlatBonus ||
				Resolved.PhysicalArmor != LegacyLevel->PhysicalArmor ||
				Resolved.MagicalArmor != LegacyLevel->MagicalArmor)
			{
				AddError(FString::Printf(TEXT("%s N%d: equip/attack/numeric projection mismatch"),
					*ArchetypeId.ToString(), Level));
				bAllEquivalent = false;
				break;
			}
		}
	}
	TestEqual(TEXT("22 manifest archetypes checked"), SeenIds.Num(), 22);
	TestEqual(TEXT("Runtime equip expansion without extra attack"), WidenedEquipWithoutAttack, 17);
	TestEqual(TEXT("NotHandHeld but equippable in declared slot"), NotHandHeldWithExplicitSlot, 3);
	TestEqual(TEXT("Three passive ARM items"), PassiveItems, 3);
	TestEqual(TEXT("One explicit FocusPassive"), FocusItems, 1);
	TestEqual(TEXT("Legacy MainHand attack grants"), AttacksMain, 19);
	TestEqual(TEXT("Legacy OffHand attack grants"), AttacksOff, 2);
	// Negative fixtures: a passive item cannot source an attack; a MainHand
	// policy never grants an OffHand attack despite an authored source slot.
	FGridCanonicalResolvedWeapon Guard;
	Guard.Kind = EGridCanonicalArmamentKind::Focus;
	Guard.HandPolicy = EGridCanonicalHandPolicy::OffHand;
	Guard.AttackSourceSlots.Add(EGridEquipmentSlot::OffHand);
	TestFalse(TEXT("Passive focus never grants an attack"), Guard.HasAttackSourceInHand(EGridEquipmentSlot::OffHand));
	Guard.Kind = EGridCanonicalArmamentKind::Weapon;
	Guard.HandPolicy = EGridCanonicalHandPolicy::MainHand;
	TestFalse(TEXT("MainHand-only cannot attack offhand"), Guard.HasAttackSourceInHand(EGridEquipmentSlot::OffHand));

	TestTrue(TEXT("22 ARM hand/attack + 440 level projections preserve legacy"), bAllEquivalent);
	return bAllEquivalent && SeenIds.Num() == 22 &&
		WidenedEquipWithoutAttack == 17 && NotHandHeldWithExplicitSlot == 3 &&
		PassiveItems == 3 && FocusItems == 1 && AttacksMain == 19 && AttacksOff == 2;
}
#endif
