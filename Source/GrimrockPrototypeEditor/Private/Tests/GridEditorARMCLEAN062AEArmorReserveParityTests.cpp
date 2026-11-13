#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062AEProductionArmorReserves,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionInstanceArmorReserveParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062AEProductionArmorReserves::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("Read official 22 real ARM production rows"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("Exactly 22 source items"), Manifest.Num(), 22))
	{
		return false;
	}
	const UGridWeaponCatalogueAsset* Source =
		LoadObject<UGridWeaponCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	if (!TestNotNull(TEXT("Actual compact N1-N20 source catalogue"), Source) ||
		!TestTrue(TEXT("Existing catalogue structurally valid"), Source->IsValidCatalogue()))
	{
		return false;
	}
	const bool bWasDirty = Source->GetOutermost()->IsDirty();
	int32 Profiles = 0;
	int32 PassiveItems = 0;
	int32 DepletedReserves = 0;
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const FName ArmId(*Row.ArchetypeId);
		const UGridItemDefinitionAsset* Item =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		if (!IsValid(Item) ||
			Item->ItemDefinitionId != FName(*Row.ItemDefinitionId) ||
			Item->WeaponArchetypeId != ArmId)
		{
			AddError(TEXT("Manifest ARM identity does not match real definition"));
			return false;
		}
		if (Row.IsPassive()) { ++PassiveItems; }
		const bool bItemWasDirty = Item->GetOutermost()->IsDirty();
		for (const int32 Level : { 1, 20 })
		{
			const FGridWeaponLevelProfile* Legacy = Source->FindProfile(ArmId, Level);
			if (!Legacy)
			{
				AddError(TEXT("Missing actual ARM fixed-level profile"));
				return false;
			}
			FGridCanonicalResolvedWeapon Resolved;
			Resolved.PhysicalArmor = Legacy->PhysicalArmor;
			Resolved.MagicalArmor = Legacy->MagicalArmor;
			const int32 SpentPhysical = FMath::Min(3, Resolved.PhysicalArmor);
			const int32 SpentMagical = FMath::Min(2, Resolved.MagicalArmor);
			FGridCanonicalArmorReserveView View;
			if (!TestTrue(TEXT("Nominal profile + exact spent values resolve"),
				FGridCanonicalArmorReserveResolver::TryCompute(
					Resolved, SpentPhysical, SpentMagical, View)))
			{
				return false;
			}
			TestEqual(TEXT("Nominal physical armor unchanged"),
				View.NominalPhysicalArmor, Legacy->PhysicalArmor);
			TestEqual(TEXT("Nominal magical armor unchanged"),
				View.NominalMagicalArmor, Legacy->MagicalArmor);
			TestEqual(TEXT("Original physical spent amount retained"),
				View.SpentPhysicalArmor, SpentPhysical);
			TestEqual(TEXT("Original magical spent amount retained"),
				View.SpentMagicalArmor, SpentMagical);
			TestEqual(TEXT("Physical reserve is not refilled during resolve"),
				View.RemainingPhysicalArmor,
				FMath::Max(0, Legacy->PhysicalArmor - SpentPhysical));
			TestEqual(TEXT("Magical reserve is not refilled during resolve"),
				View.RemainingMagicalArmor,
				FMath::Max(0, Legacy->MagicalArmor - SpentMagical));
			if (SpentPhysical > 0 || SpentMagical > 0) { ++DepletedReserves; }
			++Profiles;
		}
		if (Item->GetOutermost()->IsDirty() != bItemWasDirty)
		{
			AddError(TEXT("Armor reserve preview dirtied a real weapon package"));
			return false;
		}
	}
	TestEqual(TEXT("All 44 real N1 and N20 armor profiles compared"), Profiles, 44);
	TestEqual(TEXT("Three passive ARM definitions included"), PassiveItems, 3);
	TestTrue(TEXT("Real armor depletion cases were exercised"), DepletedReserves > 0);
	TestTrue(TEXT("Global numeric catalogue is unchanged"),
		Source->GetOutermost()->IsDirty() == bWasDirty);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2AE: %d actual fixed levels, %d passive item types, "
		"%d depleted cases; nominal and remaining armor are separate, "
		"no SaveGame, shields or assets mutated."),
		Profiles, PassiveItems, DepletedReserves));
	return Profiles == 44 && PassiveItems == 3 &&
		Source->GetOutermost()->IsDirty() == bWasDirty;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062AENeverRefillShield,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.NoFreeArmorRefillDuringCanonicalResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062AENeverRefillShield::RunTest(const FString&)
{
	FGridCanonicalResolvedWeapon Shield;
	Shield.Kind = EGridCanonicalArmamentKind::Shield;
	Shield.PhysicalArmor = 5;
	Shield.MagicalArmor = 2;
	FGridCanonicalArmorReserveView View;
	if (!TestTrue(TEXT("A 5-point physical shield can be three points spent"),
		FGridCanonicalArmorReserveResolver::TryCompute(Shield, 3, 1, View)))
	{
		return false;
	}
	TestEqual(TEXT("Only two physical armor points remain"), View.RemainingPhysicalArmor, 2);
	TestEqual(TEXT("One magical armor point remains"), View.RemainingMagicalArmor, 1);
	TestEqual(TEXT("Three physical points remain consumed"), View.SpentPhysicalArmor, 3);

	TestTrue(TEXT("Exhausted physical reserve remains exhausted"),
		FGridCanonicalArmorReserveResolver::TryCompute(Shield, 5, 2, View));
	TestEqual(TEXT("No free physical refill"), View.RemainingPhysicalArmor, 0);
	TestEqual(TEXT("No free magical refill"), View.RemainingMagicalArmor, 0);

	TestTrue(TEXT("Outdated higher spent amount is retained and remaining is clamped"),
		FGridCanonicalArmorReserveResolver::TryCompute(Shield, 9, 3, View));
	TestEqual(TEXT("Spent amount is never silently clamped"),
		View.SpentPhysicalArmor, 9);
	TestEqual(TEXT("Overdepletion still leaves no shield armor"),
		View.RemainingPhysicalArmor, 0);
	TestEqual(TEXT("Excess spent magic also retained"), View.SpentMagicalArmor, 3);
	TestEqual(TEXT("No negative remaining magical armor"), View.RemainingMagicalArmor, 0);

	TestFalse(TEXT("Negative persisted physical debt is invalid"),
		FGridCanonicalArmorReserveResolver::TryCompute(Shield, -1, 0, View));
	TestEqual(TEXT("Failure clears the previous successful physical result"),
		View.SpentPhysicalArmor, 0);
	TestEqual(TEXT("Failure clears previous magical result"),
		View.SpentMagicalArmor, 0);
	Shield.PhysicalArmor = -1;
	TestFalse(TEXT("Negative nominal armor is invalid"),
		FGridCanonicalArmorReserveResolver::TryCompute(Shield, 0, 0, View));
	return true;
}

#endif
