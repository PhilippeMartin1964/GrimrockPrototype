#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06ProductionAmmoCandidate.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062VRealAmmo,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionAmmoIdentityAndBowLinks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062VRealAmmo::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("Production manifest of exactly 22 ARM"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
	{
		AddError(Error);
		return false;
	}
	const UGridWeaponCatalogueAsset* N1ToN20 =
		LoadObject<UGridWeaponCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	const UGridWeaponVisualCatalogueAsset* Visuals =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue"));
	const UGridEquippedWeaponActionIconPalette* Icons =
		LoadObject<UGridEquippedWeaponActionIconPalette>(nullptr,
			TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.DA_EquippedWeaponActionIcons"));
	const UGridItemDefinitionAsset* SourceArrows =
		LoadObject<UGridItemDefinitionAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.DA_Item_Arrows_01"));
	if (!TestNotNull(TEXT("Source numbers"), N1ToN20) ||
		!TestNotNull(TEXT("Source visual catalogue"), Visuals) ||
		!TestNotNull(TEXT("Source action icons"), Icons) ||
		!TestNotNull(TEXT("Real Arrow definition"), SourceArrows))
	{
		return false;
	}
	const bool bArrowsDirty = SourceArrows->GetOutermost()->IsDirty();
	TStrongObjectPtr<UGridItemSystemAsset> Root(NewObject<UGridItemSystemAsset>(GetTransientPackage()));
	GridArmClean06FullProductionCandidate::FSummary Full;
	if (!TestTrue(TEXT("Canonical 22 ARM complete transient preflight"),
		GridArmClean06FullProductionCandidate::Build(
			Manifest, *N1ToN20, *Visuals, *Icons, *Root, Full)))
	{
		return false;
	}
	GridArmClean06ProductionAmmoCandidate::FSummary Ammo;
	if (!TestTrue(TEXT("Real arrow item and bow ammo joins projected"),
		GridArmClean06ProductionAmmoCandidate::AddExistingArrows(
			Manifest, *Root, Full, Ammo)))
	{
		return false;
	}
	TestEqual(TEXT("One real canonical ammo definition"), Ammo.CanonicalAmmoDefinitions, 1);
	TestTrue(TEXT("At least one real bow references source arrows"),
		Ammo.WeaponAmmoReferences > 0);
	TestEqual(TEXT("23 items: 22 weapons plus one real ammunition"), Root->ItemDefinitions.Num(), 23);
	TestEqual(TEXT("Existing item ID preserved as canonical ammo kind"),
		Ammo.ResultAmmoKindId, FName(TEXT("Item_Arrows_01")));
	TestEqual(TEXT("Original missing ammo joins fully resolved"),
		Full.UnmappedAmmunition, 0);
	int32 Count = 0;
	for (const FGridCanonicalWeaponArchetype& Arm : Root->WeaponCatalogue->Archetypes)
	{
		if (Arm.RequiredAmmoKindId == Ammo.ResultAmmoKindId) { ++Count; }
	}
	TestEqual(TEXT("Only the original weapons now consume this ammo kind"),
		Count, Ammo.WeaponAmmoReferences);
	GridArmClean06ProductionAmmoCandidate::FSummary Second;
	TestFalse(TEXT("No second pass silently duplicates ammunition"),
		GridArmClean06ProductionAmmoCandidate::AddExistingArrows(
			Manifest, *Root, Full, Second));
	TestFalse(TEXT("Ammo links do not authorize an unfinished cutover"),
		Full.ReadyForBinaryCutover());
	TestEqual(TEXT("Real arrows unchanged"),
		SourceArrows->GetOutermost()->IsDirty(), bArrowsDirty);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2V: one real arrow definition, %d exact weapon links, "
		"zero unresolved arrows, no binary writes, cutover still prohibited."),
		Ammo.WeaponAmmoReferences));
	return Ammo.CanonicalAmmoDefinitions == 1 && Count == Ammo.WeaponAmmoReferences &&
		SourceArrows->GetOutermost()->IsDirty() == bArrowsDirty;
}
#endif
