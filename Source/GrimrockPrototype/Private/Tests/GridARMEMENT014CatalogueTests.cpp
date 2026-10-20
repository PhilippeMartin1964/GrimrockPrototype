#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
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

	UGridWeaponCatalogueAsset* MakeFullCatalogue()
	{
		UGridWeaponCatalogueAsset* Catalogue = NewObject<UGridWeaponCatalogueAsset>();
		for (int32 Id = 1; Id <= 82; ++Id)
		{
			const FName Archetype(*FString::Printf(TEXT("ARM%03d"), Id));
			for (int32 Level = 1; Level <= 20; ++Level)
			{
				Catalogue->Profiles.Add(MakeProfile(Archetype, Level, Id >= 70));
			}
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
	UGridWeaponCatalogueAsset* Catalogue = ARMEMENT014::MakeFullCatalogue();
	TestEqual(TEXT("1640 entries"), Catalogue->Profiles.Num(), 1640);
	TestTrue(TEXT("82 archetypes x 20 levels pass strict validation"), Catalogue->IsValidCatalogue());
	const FGridWeaponLevelProfile* First = Catalogue->FindProfile(TEXT("ARM001"), 1);
	const FGridWeaponLevelProfile* Last = Catalogue->FindProfile(TEXT("ARM082"), 20);
	TestNotNull(TEXT("First profile"), First);
	TestNotNull(TEXT("Last profile"), Last);
	TestTrue(TEXT("No invalid level"), Catalogue->FindProfile(TEXT("ARM001"), 21) == nullptr);
	TestTrue(TEXT("No unknown item"), Catalogue->FindProfile(TEXT("ARM099"), 1) == nullptr);

	FGridAttackDefinition Attack;
	TestTrue(TEXT("A level-4 dagger generates attack dice"), Catalogue->TryBuildAttack(TEXT("ARM001"), 4, Attack));
	TestEqual(TEXT("2d8+4 min"), Attack.MinDamage, 6);
	TestEqual(TEXT("2d8+4 max"), Attack.MaxDamage, 20);
	TestFalse(TEXT("Shield cannot generate damage"), Catalogue->TryBuildAttack(TEXT("ARM082"), 20, Attack));
	TestFalse(TEXT("Failed passive query clears previous attack"), Attack.DamageDice.bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT014InvalidCatalogueTest,
	"Grimrock.RPG.ARMEMENT01.4.DuplicateAndMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT014InvalidCatalogueTest::RunTest(const FString&)
{
	UGridWeaponCatalogueAsset* Catalogue = ARMEMENT014::MakeFullCatalogue();
	Catalogue->Profiles[1].ItemLevel = 1;
	TestFalse(TEXT("Duplicate level rejected"), Catalogue->IsValidCatalogue());
	Catalogue = ARMEMENT014::MakeFullCatalogue();
	Catalogue->Profiles.RemoveAt(0);
	TestFalse(TEXT("Missing level rejected"), Catalogue->IsValidCatalogue());
	Catalogue = ARMEMENT014::MakeFullCatalogue();
	Catalogue->Profiles[0].DamageDice.FlatBonus = -1;
	TestFalse(TEXT("Invalid dice rejected in full catalogue"), Catalogue->IsValidCatalogue());
	Catalogue = ARMEMENT014::MakeFullCatalogue();
	Catalogue->Profiles[19].AccuracyBonus++;
	TestFalse(TEXT("Archetype accuracy must remain constant across levels"), Catalogue->IsValidCatalogue());
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
