#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06CompleteProductionPreview.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062YCompleteProductionPreview,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.CompleteProductionPreview23",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062YCompleteProductionPreview::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("Official manifest includes exactly 22 active ARM"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
	{
		AddError(Error);
		return false;
	}
	const UGridWeaponCatalogueAsset* Numerics =
		LoadObject<UGridWeaponCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	const UGridWeaponVisualCatalogueAsset* Visuals =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue"));
	const UGridEquippedWeaponActionIconPalette* Icons =
		LoadObject<UGridEquippedWeaponActionIconPalette>(nullptr,
			TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.DA_EquippedWeaponActionIcons"));
	const UGridItemDefinitionAsset* Arrows =
		LoadObject<UGridItemDefinitionAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.DA_Item_Arrows_01"));
	if (!TestNotNull(TEXT("Real numbers"), Numerics) ||
		!TestNotNull(TEXT("Real visual source"), Visuals) ||
		!TestNotNull(TEXT("Real action icons"), Icons) ||
		!TestNotNull(TEXT("Real arrows"), Arrows))
	{
		return false;
	}
	const bool bNumberDirty = Numerics->GetOutermost()->IsDirty();
	const bool bVisualDirty = Visuals->GetOutermost()->IsDirty();
	const bool bIconDirty = Icons->GetOutermost()->IsDirty();
	const bool bArrowDirty = Arrows->GetOutermost()->IsDirty();
	TStrongObjectPtr<UGridItemSystemAsset> Candidate(
		NewObject<UGridItemSystemAsset>(GetTransientPackage()));
	GridArmClean06CompleteProductionPreview::FSummary Stats;
	if (!TestTrue(TEXT("Build single transient four-catalogue+ammo+throw preflight"),
		GridArmClean06CompleteProductionPreview::Build(
			Manifest, *Numerics, *Visuals, *Icons, *Candidate, Stats)))
	{
		return false;
	}
	TestEqual(TEXT("22 weapon definitions plus real arrows"), Candidate->ItemDefinitions.Num(), 23);
	TestEqual(TEXT("All 82 source archetypes"), Stats.Core.Numerics.Archetypes, 82);
	TestEqual(TEXT("Exact N1-N20 comparisons"), Stats.Core.Numerics.ComparedProfiles, 1640);
	TestEqual(TEXT("All passive levels"), Stats.Core.Numerics.PassiveProfiles, 260);
	TestEqual(TEXT("All 22 active hand/type policies proven"), Stats.Core.Numerics.ProvenConcreteArchetypes, 22);
	TestEqual(TEXT("19 real main-hand attack sources"), Stats.Core.AttacksMainHand, 19);
	TestEqual(TEXT("2 real off-hand attack sources"), Stats.Core.AttacksOffHand, 2);
	TestEqual(TEXT("Exactly one authored ammunition type"), Stats.Ammo.CanonicalAmmoDefinitions, 1);
	TestEqual(TEXT("All real ammo joins accounted for"), Stats.Core.UnmappedAmmunition, 0);
	TestEqual(TEXT("Exactly three real throwable physics profiles"), Stats.PhysicalThrowProfiles, 3);
	TestEqual(TEXT("Exactly three matching projectile presentations"), Stats.VisualThrowProfiles, 3);
	TestEqual(TEXT("All originally unconverted throw physics now accounted for"),
		Stats.Core.UnconvertedThrowPolicies, 0);
	TestTrue(TEXT("Final visual catalogue validates"), Stats.bVisualCatalogueValid);
	TestTrue(TEXT("Final numeric catalogue validates"), Stats.bWeaponCatalogueValid);
	TestEqual(TEXT("Unbound offensive candidates explicitly counted"),
		Stats.Core.UnboundOffensiveArmaments, 19 - Stats.Core.Actions.BoundActions);
	TestFalse(TEXT("A preview never grants binary cutover without full action parity"),
		Stats.ReadyForBinaryCutover());

	const FGridCanonicalWeaponArchetype* Knife =
		Candidate->WeaponCatalogue->Find(FName(TEXT("ARM054")));
	const FGridCanonicalWeaponVisual* KnifeVisual =
		Candidate->WeaponVisualCatalogue->FindForArchetype(FName(TEXT("ARM054")));
	if (!TestNotNull(TEXT("Actual canonical throwing knife numeric view"), Knife) ||
		!TestNotNull(TEXT("Actual canonical throwing knife visual view"), KnifeVisual))
	{
		return false;
	}
	TestTrue(TEXT("Throwing knife has separate physical flight policy"), Knife->Projectile.bEnabled);
	TestTrue(TEXT("Throwing knife has separate visual flight policy"),
		KnifeVisual->ProjectileAppearance.bEnabled);

	GridArmClean06CompleteProductionPreview::FSummary Retry;
	TestFalse(TEXT("A second pass cannot overwrite an existing canonical registry"),
		GridArmClean06CompleteProductionPreview::Build(
			Manifest, *Numerics, *Visuals, *Icons, *Candidate, Retry));
	TestTrue(TEXT("Numeric source unchanged"), Numerics->GetOutermost()->IsDirty() == bNumberDirty);
	TestTrue(TEXT("Visual source unchanged"), Visuals->GetOutermost()->IsDirty() == bVisualDirty);
	TestTrue(TEXT("Action icon source unchanged"), Icons->GetOutermost()->IsDirty() == bIconDirty);
	TestTrue(TEXT("Real arrow source unchanged"), Arrows->GetOutermost()->IsDirty() == bArrowDirty);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2Y: 22 weapons + 1 real ammo, 82/1640/260 numeric proof; "
		"%d real action bindings, %d unbound attacks, %d arrow links, "
		"3 complete throw physics and 3 visual profiles; "
		"binary cutover=%s. No assets written."),
		Stats.Core.Actions.BoundActions,
		Stats.Core.UnboundOffensiveArmaments,
		Stats.Ammo.WeaponAmmoReferences,
		Stats.ReadyForBinaryCutover() ? TEXT("allowed") : TEXT("refused")));
	return !Stats.ReadyForBinaryCutover() &&
		Stats.Core.UnmappedAmmunition == 0 &&
		Stats.Core.UnconvertedThrowPolicies == 0;
}

#endif
