#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponProgressionTypes.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "UObject/StrongObjectPtr.h"
#include "Runtime/Combat/GridCombatResolver.h"

namespace ARMEMENT014
{
	FGridWeaponLevelProfile MakeProfile(FName Id, int32 Level, bool bPassive = false)
	{
		FGridWeaponLevelProfile P;
		P.ArchetypeId = Id;
		P.ItemLevel = Level;
		P.DisplayName = FText::FromName(Id);
		P.Family = bPassive ? TEXT("Boucliers") : TEXT("Légères");
		P.HandUsage = bPassive ? TEXT("Secondaire") : TEXT("1 main");
		P.ScalingAttribute = EGridAttackScalingAttribute::Dexterity;
		P.DamageType = EGridDamageType::Physical;
		P.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
		P.AccuracyBonus = 1;
		P.PhysicalArmor = bPassive ? 2 : 0;
		P.ActionPointCost = bPassive ? 0 : 2;
		P.RangeCells = bPassive ? 0 : 1;
		P.WeightKg = bPassive ? 1.5f : 0.4f;
		P.DamageDice.bEnabled = !bPassive;
		P.DamageDice.DiceCount = bPassive ? 0 : 2;
		P.DamageDice.DiceSides = bPassive ? 0 : 8;
		P.DamageDice.FlatBonus = bPassive ? 0 : Level;
		return P;
	}

	TStrongObjectPtr<UGridCanonicalWeaponCatalogueAsset> MakeCatalogue()
	{
		TStrongObjectPtr<UGridCanonicalWeaponCatalogueAsset> Catalogue(NewObject<UGridCanonicalWeaponCatalogueAsset>());
		for (int32 Ordinal = 1; Ordinal <= 82; ++Ordinal)
		{
			FGridWeaponArchetype& Weapon = Catalogue->Archetypes.AddDefaulted_GetRef();
			Weapon.ArchetypeId = FName(*FString::Printf(TEXT("ARM%03d"), Ordinal));
			Weapon.bHandAndKindProven = true;
			if (Ordinal >= 70)
			{
				Weapon.Kind = EGridArmamentKind::Shield;
				Weapon.PhysicalArmorN1 = 2;
				Weapon.DamageDiceN1.DiceCount = 0;
				Weapon.DamageDiceN1.DiceSides = 0;
				continue;
			}
			Weapon.Kind = EGridArmamentKind::Weapon;
			Weapon.ActionPointCost = 2;
			Weapon.RangeCells = 1;
			Weapon.AccuracyBonus = 1;
			Weapon.DamageDiceN1.bEnabled = true;
			Weapon.DamageDiceN1.DiceCount = 2;
			Weapon.DamageDiceN1.DiceSides = 8;
			Weapon.DamageDiceN1.FlatBonus = 1;
			Weapon.FlatBonusGrowth.StepAmount = 1;
			FGridWeaponGrowthException& Exception = Weapon.FlatBonusGrowth.Exceptions.AddDefaulted_GetRef();
			Exception.ItemLevel = 8;
			Exception.Value = 11;
		}
		return Catalogue;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT014ProfileTest,
	"Grimrock.RPG.ARMEMENT01.4.ProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT014ProfileTest::RunTest(const FString&)
{
	FGridWeaponLevelProfile P = ARMEMENT014::MakeProfile(TEXT("ARM001"), 4);
	TestTrue(TEXT("Offensive 2d8+4 profile valid"), P.IsValidProfile());
	FGridAttackDefinition Attack;
	TestTrue(TEXT("Build real dice attack"), P.TryBuildAttackDefinition(Attack));
	TestTrue(TEXT("Dice are enabled"), Attack.DamageDice.bEnabled);
	TestEqual(TEXT("Minimum"), Attack.MinDamage, 6);
	TestEqual(TEXT("Maximum"), Attack.MaxDamage, 20);
	TestEqual(TEXT("Accuracy comes from catalogue"), Attack.AccuracyBonus, 1);

	P.DamageDice.DiceSides = 5;
	TestFalse(TEXT("Noncanonical d5 rejected"), P.IsValidProfile());
	P = ARMEMENT014::MakeProfile(TEXT("ARM001"), 4);
	P.ItemLevel = 21;
	TestFalse(TEXT("Level above 20 rejected"), P.IsValidProfile());
	P = ARMEMENT014::MakeProfile(TEXT("ARM001"), 4);
	P.WeightKg = -1.0f;
	TestFalse(TEXT("Negative weight rejected"), P.IsValidProfile());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT014PassiveTest,
	"Grimrock.RPG.ARMEMENT01.4.PassiveEquipment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT014PassiveTest::RunTest(const FString&)
{
	FGridWeaponLevelProfile Shield = ARMEMENT014::MakeProfile(TEXT("ARM072"), 7, true);
	TestTrue(TEXT("Shield profile valid without dice"), Shield.IsValidProfile());
	FGridAttackDefinition Attack;
	TestFalse(TEXT("Passive shield never authors a fake attack"), Shield.TryBuildAttackDefinition(Attack));
	Shield.DamageDice.DiceSides = 4;
	TestFalse(TEXT("0d4 fake passive is invalid"), Shield.IsValidProfile());
	Shield = ARMEMENT014::MakeProfile(TEXT("ARM072"), 7, true);
	Shield.ActionPointCost = 2;
	TestFalse(TEXT("Passive with action cost is invalid"), Shield.IsValidProfile());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT014CatalogueTest,
	"Grimrock.RPG.ARMEMENT01.4.CatalogueCompleteness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT014CatalogueTest::RunTest(const FString&)
{
	const auto Catalogue = ARMEMENT014::MakeCatalogue();
	TArray<FString> Errors;
	if (!TestTrue(TEXT("82 archetypes define all physical levels"), Catalogue->Validate(Errors)))
	{
		for (const FString& Error : Errors)
			AddError(Error);
		return false;
	}

	int32 NumProfiles = 0;
	for (int32 Ordinal = 1; Ordinal <= 82; ++Ordinal)
	{
		const FName Id(*FString::Printf(TEXT("ARM%03d"), Ordinal));
		const FGridWeaponArchetype* Archetype = Catalogue->Find(Id);
		if (!TestNotNull(TEXT("Every expected archetype exists"), Archetype))
			return false;
		const bool bAttack = Ordinal < 70;
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			FGridResolvedWeapon Profile;
			if (!TestTrue(TEXT("Every physical level resolves"), Archetype->TryResolve(Level, Profile)))
				return false;
			++NumProfiles;
			TestEqual(TEXT("Attack role"), Profile.DamageDice.bEnabled, bAttack);
			TestEqual(TEXT("Physical protection"), Profile.PhysicalArmor, bAttack ? 0 : 2);
			TestEqual(TEXT("Number of dice"), Profile.DamageDice.DiceCount, bAttack ? 2 : 0);
			TestEqual(TEXT("Die size"), Profile.DamageDice.DiceSides, bAttack ? 8 : 0);
			const int32 ExpectedBonus = bAttack ? (Level == 8 ? 11 : Level) : 0;
			TestEqual(TEXT("Flat bonus, including absolute level-eight exception"), Profile.DamageDice.FlatBonus, ExpectedBonus);
			TestEqual(TEXT("Minimum damage"), Profile.DamageDice.GetMinimumDamage(), bAttack ? 2 + ExpectedBonus : 0);
			TestEqual(TEXT("Maximum damage"), Profile.DamageDice.GetMaximumDamage(), bAttack ? 16 + ExpectedBonus : 0);
			TestEqual(TEXT("Action point cost"), Profile.ActionPointCost, bAttack ? 2 : 0);
			TestEqual(TEXT("Accuracy"), Profile.AccuracyBonus, bAttack ? 1 : 0);
		}
		FGridResolvedWeapon Invalid;
		TestFalse(TEXT("No N0 weapon profile"), Archetype->TryResolve(0, Invalid));
		TestFalse(TEXT("No N21 weapon profile"), Archetype->TryResolve(21, Invalid));
	}
	TestEqual(TEXT("1640 resolved profiles"), NumProfiles, 1640);
	TestNull(TEXT("Unknown identity is refused"), Catalogue->Find(TEXT("ARM099")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT014InvalidCatalogueTest,
	"Grimrock.RPG.ARMEMENT01.4.DuplicateAndMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT014InvalidCatalogueTest::RunTest(const FString&)
{
	const auto CheckInvalid = [this](const TCHAR* Label, const UGridCanonicalWeaponCatalogueAsset& Catalogue)
	{
		TArray<FString> Errors;
		TestFalse(Label, Catalogue.Validate(Errors));
		TestFalse(TEXT("Invalid input has a diagnostic"), Errors.IsEmpty());
	};
	auto Catalogue = ARMEMENT014::MakeCatalogue();
	Catalogue->Archetypes[1].ArchetypeId = TEXT("ARM001");
	CheckInvalid(TEXT("Duplicate archetype rejected"), *Catalogue);
	Catalogue = ARMEMENT014::MakeCatalogue();
	Catalogue->Archetypes.RemoveAt(0);
	CheckInvalid(TEXT("Missing archetype rejected"), *Catalogue);
	Catalogue = ARMEMENT014::MakeCatalogue();
	Catalogue->Archetypes[0].DamageDiceN1.DiceSides = 5;
	CheckInvalid(TEXT("Invalid die rejected"), *Catalogue);
	Catalogue = ARMEMENT014::MakeCatalogue();
	auto& Exceptions = Catalogue->Archetypes[0].FlatBonusGrowth.Exceptions;
	const FGridWeaponGrowthException Duplicate = Exceptions[0];
	Exceptions.Add(Duplicate);
	CheckInvalid(TEXT("Duplicate level exception rejected"), *Catalogue);
	Catalogue = ARMEMENT014::MakeCatalogue();
	Catalogue->Archetypes[0].FlatBonusGrowth.Exceptions[0].ItemLevel = 21;
	CheckInvalid(TEXT("Out-of-range level exception rejected"), *Catalogue);
	Catalogue = ARMEMENT014::MakeCatalogue();
	Catalogue->Archetypes[0].FlatBonusGrowth.StepEveryLevels = 0;
	CheckInvalid(TEXT("Invalid level-growth interval rejected"), *Catalogue);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT014CombatCompatibilityTest,
	"Grimrock.RPG.ARMEMENT01.4.AttackResolverCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT014CombatCompatibilityTest::RunTest(const FString&)
{
	FGridWeaponLevelProfile P = ARMEMENT014::MakeProfile(TEXT("ARM001"), 4);
	FGridAttackDefinition Attack;
	if (!TestTrue(TEXT("Catalogue attack builds"), P.TryBuildAttackDefinition(Attack)))
	{
		return false;
	}
	FGridAttackSourceStats Source;
	FGridAttackTargetStats Target;
	Target.CurrentHealth = 200;
	const FGridAttackResult Result = FGridCombatResolver::ResolveAttackFromDiceRolls(
		Source, Target, Attack, 15, { 3, 7 });
	TestTrue(TEXT("Catalogue-produced attack hits"), Result.bHit);
	TestEqual(TEXT("Real 2d8+4 total"), Result.Trace.Damage.Roll.FinalValue, 14);
	TestEqual(TEXT("Two dice recorded"), Result.Trace.Damage.Roll.IndividualRolls.Num(), 2);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
