#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMCLEAN03LosslessCatalogueTest,
	"Grimrock.RPG.ARMEMENT.CLEAN03.LosslessCompactCatalogue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMCLEAN03LosslessCatalogueTest::RunTest(const FString&)
{
	const UGridWeaponCatalogueAsset* Source = LoadObject<UGridWeaponCatalogueAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	if (!TestNotNull(TEXT("Canonical full catalogue"), Source)) return false;
	if (!TestTrue(TEXT("Canonical catalogue valid"), Source->IsValidCatalogue())) return false;

	TArray<FGridWeaponArchetypeProgression> Candidate;
	int32 ExceptionCount = 0;
	if (!TestTrue(TEXT("Build lossless compact candidate"), Source->BuildCompactCandidate(Candidate, ExceptionCount)))
	{
		return false;
	}
	TestEqual(TEXT("82 compact archetypes"), Candidate.Num(), 82);
	TestTrue(TEXT("Exceptions nonnegative"), ExceptionCount >= 0);

	UGridWeaponCatalogueAsset* Compressed = NewObject<UGridWeaponCatalogueAsset>();
	Compressed->ArchetypeProgressions = Candidate;
	if (!TestTrue(TEXT("Compressed catalogue meets canonical rules"), Compressed->IsValidCatalogue()))
	{
		return false;
	}
	int32 Compared = 0;
	for (const FGridWeaponLevelProfile& Original : Source->Profiles)
	{
		const FGridWeaponLevelProfile* Restored =
			Compressed->FindProfile(Original.ArchetypeId, Original.ItemLevel);
		if (!Restored ||
			Restored->ArchetypeId != Original.ArchetypeId ||
			Restored->ItemLevel != Original.ItemLevel ||
			!Restored->DisplayName.EqualTo(Original.DisplayName) ||
			Restored->Family != Original.Family ||
			Restored->HandUsage != Original.HandUsage ||
			Restored->DamageType != Original.DamageType ||
			Restored->PhysicalSubtype != Original.PhysicalSubtype ||
			Restored->ScalingAttribute != Original.ScalingAttribute ||
			Restored->AccuracyBonus != Original.AccuracyBonus ||
			Restored->DamageDice.bEnabled != Original.DamageDice.bEnabled ||
			Restored->DamageDice.DiceCount != Original.DamageDice.DiceCount ||
			Restored->DamageDice.DiceSides != Original.DamageDice.DiceSides ||
			Restored->DamageDice.FlatBonus != Original.DamageDice.FlatBonus ||
			Restored->PhysicalArmor != Original.PhysicalArmor ||
			Restored->MagicalArmor != Original.MagicalArmor ||
			Restored->ActionPointCost != Original.ActionPointCost ||
			Restored->RangeCells != Original.RangeCells ||
			!FMath::IsNearlyEqual(Restored->WeightKg, Original.WeightKg, 0.0001f))
		{
			AddError(FString::Printf(TEXT("Compact mismatch: %s level %d"),
				*Original.ArchetypeId.ToString(), Original.ItemLevel));
			return false;
		}
		++Compared;
	}
	TestEqual(TEXT("Every legacy profile restored exactly"), Compared, 1640);

	// An invalid rule must be rejected before a corrupt asset can be used.
	FGridWeaponGrowthException Duplicate;
	Duplicate.ItemLevel = 2;
	Compressed->ArchetypeProgressions[0].DiceCountGrowth.Exceptions = { Duplicate, Duplicate };
	Compressed->InvalidateProfileCache();
	TestFalse(TEXT("Duplicate exception level rejected"), Compressed->IsValidCatalogue());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
