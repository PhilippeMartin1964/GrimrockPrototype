#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06ProductionCandidate.h"
#include "ARMEMENT/GridArmClean06ProjectileVisualProjection.h"
#include "Engine/StaticMesh.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062XRealProjectileVisuals,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionProjectileVisualParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062XRealProjectileVisuals::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("Exactly 22 production weapons in the official manifest"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("Exactly 22 production rows"), Manifest.Num(), 22))
	{
		return false;
	}
	const UGridWeaponVisualCatalogueAsset* Historical =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue"));
	const UGridEquippedWeaponActionIconPalette* Icons =
		LoadObject<UGridEquippedWeaponActionIconPalette>(nullptr,
			TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.DA_EquippedWeaponActionIcons"));
	if (!TestNotNull(TEXT("Existing visual catalogue"), Historical) ||
		!TestNotNull(TEXT("Existing action icon palette"), Icons))
	{
		return false;
	}
	const bool bCatalogueDirty = Historical->GetOutermost()->IsDirty();
	const bool bIconsDirty = Icons->GetOutermost()->IsDirty();
	TStrongObjectPtr<UGridItemSystemAsset> Root(
		NewObject<UGridItemSystemAsset>(GetTransientPackage()));
	GridArmClean06ProductionCandidate::FSummary Summary;
	if (!TestTrue(TEXT("Build only 22 real visual profiles and definitions transiently"),
		GridArmClean06ProductionCandidate::BuildDefinitionsAndVisuals(
			Manifest, *Historical, *Icons, *Root, Summary)))
	{
		return false;
	}
	TSet<FName> SeenVisualIds;
	int32 Thrown = 0;
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const FName VisualId(*Row.VisualId);
		if (SeenVisualIds.Contains(VisualId))
		{
			AddError(TEXT("A shared visual must be verified explicitly, not overwritten"));
			return false;
		}
		SeenVisualIds.Add(VisualId);
		const UGridItemDefinitionAsset* Item =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		FGridCanonicalWeaponVisual* Profile =
			Root->WeaponVisualCatalogue->VisualProfiles.FindByPredicate(
				[VisualId](const FGridCanonicalWeaponVisual& Candidate)
				{
					return Candidate.VisualId == VisualId;
				});
		if (!IsValid(Item) || !Profile)
		{
			AddError(TEXT("Concrete item or VisualId from manifest missing"));
			return false;
		}
		const bool bItemDirty = Item->GetOutermost()->IsDirty();
		if (!TestTrue(TEXT("Actual Stable/Tumble/Spin visual parameters project exactly"),
			GridArmClean06ProjectileVisualProjection::TryProject(
				Row, *Item, *Profile)) ||
			!TestTrue(TEXT("Every authored mesh and visual adjustment preserved"),
				GridArmClean06ProjectileVisualProjection::Matches(*Item, *Profile)))
		{
			return false;
		}
		if (Profile->ProjectileAppearance.bEnabled)
		{
			++Thrown;
			TestFalse(TEXT("Combat projectile has an actual source mesh"),
				Profile->ProjectileMesh.IsNull());
		}
		else
		{
			TestTrue(TEXT("Nonthrowing item never invents a projectile mesh"),
				Profile->ProjectileMesh.IsNull());
		}
		if (Item->GetOutermost()->IsDirty() != bItemDirty)
		{
			AddError(TEXT("Visual projection dirtied a real production DataAsset"));
			return false;
		}
	}
	TestEqual(TEXT("22 stable visual IDs have no collision"), SeenVisualIds.Num(), 22);
	TestEqual(TEXT("Exactly three authored combat throws in production"), Thrown, 3);
	TArray<FString> Errors;
	TestTrue(TEXT("Projected visual catalogue validates"),
		Root->WeaponVisualCatalogue->Validate(Errors));
	TestTrue(TEXT("Original visual catalogue stays clean"),
		Historical->GetOutermost()->IsDirty() == bCatalogueDirty);
	TestTrue(TEXT("Original palette stays clean"),
		Icons->GetOutermost()->IsDirty() == bIconsDirty);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2X: %d verified visual IDs; %d actual projectile appearances; "
		"rotation, scale, forward tip axis, Stable/Tumble/Spin and angular "
		"velocities preserved; no binary assets or gameplay written."),
		SeenVisualIds.Num(), Thrown));
	return Errors.IsEmpty() && SeenVisualIds.Num() == 22 && Thrown == 3;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062XProjectileVisualSafety,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProjectileVisualProjectionSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062XProjectileVisualSafety::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemDefinitionAsset> Source(
		NewObject<UGridItemDefinitionAsset>(GetTransientPackage()));
	Source->ItemDefinitionId = TEXT("TestDart");
	Source->WeaponArchetypeId = TEXT("ARM058");
	Source->bCombatThrowWeapon = true;
	Source->WorldMesh = TSoftObjectPtr<UStaticMesh>(
		FSoftObjectPath(TEXT("/Game/Tests/ARM062/TestDart.TestDart")));
	FGridWeaponPipelineRow Row;
	Row.ArchetypeId = TEXT("ARM058");
	Row.ItemDefinitionId = TEXT("TestDart");
	Row.VisualId = TEXT("VIS_TestDart");
	Row.ProductionState = TEXT("Active");
	Row.BuildKind = TEXT("MeleeOneHand");
	FGridCanonicalWeaponVisual Visual;
	Visual.VisualId = TEXT("VIS_TestDart");
	TestFalse(TEXT("Throw enabled on nonthrow kind cannot be projected"),
		GridArmClean06ProjectileVisualProjection::TryProject(
			Row, *Source, Visual));
	TestFalse(TEXT("Failed projection leaves visual disabled"),
		Visual.ProjectileAppearance.bEnabled);
	Row.BuildKind = TEXT("ThrowingOneHand");
	TestTrue(TEXT("Correct source visual category projects exactly"),
		GridArmClean06ProjectileVisualProjection::TryProject(
			Row, *Source, Visual));
	TestTrue(TEXT("Each source setting remains identical"),
		GridArmClean06ProjectileVisualProjection::Matches(*Source, Visual));
	TestFalse(TEXT("Second projection cannot overwrite an authored profile"),
		GridArmClean06ProjectileVisualProjection::TryProject(
			Row, *Source, Visual));
	TestTrue(TEXT("Rejected duplicate never overwrites first projection"),
		GridArmClean06ProjectileVisualProjection::Matches(*Source, Visual));
	return true;
}

#endif
