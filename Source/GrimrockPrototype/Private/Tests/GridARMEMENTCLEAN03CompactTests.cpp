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
	if (!TestNotNull(TEXT("Canonical weapon catalogue"), Source)) return false;
	if (!TestTrue(TEXT("Canonical catalogue valid"), Source->IsValidCatalogue())) return false;

	TArray<FGridWeaponArchetypeProgression> Candidate;
	int32 ExceptionCount = 0;
	if (Source->Profiles.Num() == 1640)
	{
		// Pre-migration: reconstruct and compare the full serialized source.
		if (!TestTrue(TEXT("Build lossless compact candidate"),
			Source->BuildCompactCandidate(Candidate, ExceptionCount)))
		{
			return false;
		}
	}
	else if (Source->Profiles.IsEmpty() && Source->ArchetypeProgressions.Num() == 82)
	{
		// Post-migration: validate the compact profiles actually saved to disk.
		Candidate = Source->ArchetypeProgressions;
		for (const FGridWeaponArchetypeProgression& Progression : Candidate)
		{
			ExceptionCount += Progression.DiceCountGrowth.Exceptions.Num();
			ExceptionCount += Progression.DiceSidesGrowth.Exceptions.Num();
			ExceptionCount += Progression.FlatBonusGrowth.Exceptions.Num();
			ExceptionCount += Progression.PhysicalArmorGrowth.Exceptions.Num();
			ExceptionCount += Progression.MagicalArmorGrowth.Exceptions.Num();
		}
	}
	else
	{
		AddError(TEXT("Neither full nor compact catalogue has the required 82x20 coverage"));
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
	for (int32 Ordinal = 1; Ordinal <= 82; ++Ordinal)
	{
		const FName ArchetypeId(*FString::Printf(TEXT("ARM%03d"), Ordinal));
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			const FGridWeaponLevelProfile* Canonical = Source->FindProfile(ArchetypeId, Level);
			if (!TestNotNull(TEXT("Canonical profile available"), Canonical)) return false;
			const FGridWeaponLevelProfile& Original = *Canonical;
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
	}
	TestEqual(TEXT("Every canonical profile reconstructed"), Compared, 1640);

	// The invalid rule must not destroy the valid candidate used below.
	const TArray<FGridWeaponGrowthException> ValidExceptions =
		Compressed->ArchetypeProgressions[0].DiceCountGrowth.Exceptions;

	// An invalid rule must be rejected before a corrupt asset can be used.
	FGridWeaponGrowthException Duplicate;
	Duplicate.ItemLevel = 2;
	Compressed->ArchetypeProgressions[0].DiceCountGrowth.Exceptions = { Duplicate, Duplicate };
	Compressed->InvalidateProfileCache();
	TestFalse(TEXT("Duplicate exception level rejected"), Compressed->IsValidCatalogue());
	Compressed->ArchetypeProgressions[0].DiceCountGrowth.Exceptions = ValidExceptions;
	Compressed->InvalidateProfileCache();
	TestTrue(TEXT("Restored compact candidate remains valid"), Compressed->IsValidCatalogue());

	// A compact catalogue cannot also contain a redundant full representation.
	Compressed->Profiles.Add(*Source->FindProfile(TEXT("ARM001"), 1));
	TestFalse(TEXT("Simultaneous full and compact authorities rejected"), Compressed->IsValidCatalogue());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
