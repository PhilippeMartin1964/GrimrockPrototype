#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06FullProductionCandidate.h"
#include "ARMEMENT/GridArmClean06ProjectilePolicyProjection.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062WRealCombatThrows,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionCombatThrowPhysics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062WRealCombatThrows::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("22 canonical ARM production rows"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
	{
		AddError(Error);
		return false;
	}
	const UGridWeaponCatalogueAsset* Numeric =
		LoadObject<UGridWeaponCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	const UGridWeaponVisualCatalogueAsset* Visual =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue"));
	const UGridEquippedWeaponActionIconPalette* Icons =
		LoadObject<UGridEquippedWeaponActionIconPalette>(nullptr,
			TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.DA_EquippedWeaponActionIcons"));
	if (!TestNotNull(TEXT("Real source numbers"), Numeric) ||
		!TestNotNull(TEXT("Real source visuals"), Visual) ||
		!TestNotNull(TEXT("Real action icons"), Icons))
	{
		return false;
	}
	TStrongObjectPtr<UGridItemSystemAsset> Root(
		NewObject<UGridItemSystemAsset>(GetTransientPackage()));
	GridArmClean06FullProductionCandidate::FSummary Full;
	if (!TestTrue(TEXT("Build 22-ARM transient production root"),
		GridArmClean06FullProductionCandidate::Build(
			Manifest, *Numeric, *Visual, *Icons, *Root, Full)))
	{
		return false;
	}
	int32 Throws = 0;
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const UGridItemDefinitionAsset* Source =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		FGridCanonicalWeaponArchetype* Arm =
			Root->WeaponCatalogue->Archetypes.FindByPredicate(
				[&Row](const FGridCanonicalWeaponArchetype& Entry)
				{
					return Entry.ArchetypeId == FName(*Row.ArchetypeId);
				});
		if (!IsValid(Source) || !Arm)
		{
			AddError(TEXT("Production source ARM or canonical destination missing"));
			return false;
		}
		const bool bWasDirty = Source->GetOutermost()->IsDirty();
		if (!TestTrue(TEXT("Exact source throw policy projects"),
			GridArmClean06ProjectilePolicyProjection::TryProject(Row, *Source, *Arm)))
		{
			return false;
		}
		if (!TestTrue(TEXT("All four physical throw parameters preserved"),
			GridArmClean06ProjectilePolicyProjection::Matches(*Source, Arm->Projectile)))
		{
			return false;
		}
		FGridCanonicalResolvedWeapon Resolved;
		if (!TestTrue(TEXT("Level 20 retains the source projectile physics"),
			Arm->TryResolve(20, Resolved)))
		{
			return false;
		}
		TestEqual(TEXT("Resolved throw enabled matches source"),
			Resolved.Projectile.bEnabled, Source->bCombatThrowWeapon);
		TestEqual(TEXT("Resolved landing offset preserved"),
			Resolved.Projectile.ImpactDropOffset,
			Source->bCombatThrowWeapon ? Source->ThrowImpactDropOffset : 0.0f);
		if (Arm->Projectile.bEnabled) { ++Throws; }
		if (Source->GetOutermost()->IsDirty() != bWasDirty)
		{
			AddError(TEXT("Projection dirtied an existing source package"));
			return false;
		}
	}
	TestEqual(TEXT("Every production combat throw explicitly projected"),
		Throws, Full.UnconvertedThrowPolicies);
	TestTrue(TEXT("Production ARM includes recoverable thrown weapons"), Throws > 0);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2W: %d recoverable projectiles projected from manifest. "
		"Visual spin/orientation and consumption remain separate; zero assets written."), Throws));
	return Throws > 0 && Throws == Full.UnconvertedThrowPolicies;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062WRejectInvalidThrow,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.RejectInvalidThrowPhysics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062WRejectInvalidThrow::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemDefinitionAsset> Source(
		NewObject<UGridItemDefinitionAsset>(GetTransientPackage()));
	Source->ItemDefinitionId = TEXT("SyntheticThrow");
	Source->WeaponArchetypeId = TEXT("ARM054");
	Source->bCombatThrowWeapon = true;
	Source->ThrowSpeed = -1.0f;
	FGridWeaponPipelineRow Row;
	Row.ArchetypeId = TEXT("ARM054");
	Row.ItemDefinitionId = TEXT("SyntheticThrow");
	Row.ProductionState = TEXT("Active");
	Row.BuildKind = TEXT("ThrowingOneHand");
	FGridCanonicalWeaponArchetype Destination;
	Destination.ArchetypeId = TEXT("ARM054");
	Destination.Kind = EGridCanonicalArmamentKind::Weapon;
	Destination.bHandAndKindProven = true;
	TestFalse(TEXT("Negative nominal speed rejected"),
		GridArmClean06ProjectilePolicyProjection::TryProject(
			Row, *Source, Destination));
	TestFalse(TEXT("Rejected source cannot create a projectile"),
		Destination.Projectile.bEnabled);
	Source->ThrowSpeed = 1800.0f;
	Source->ThrowLifeSeconds = -1.0f;
	TestFalse(TEXT("Negative lifetime rejected"),
		GridArmClean06ProjectilePolicyProjection::TryProject(
			Row, *Source, Destination));
	TestFalse(TEXT("Rejected second source does not mutate projectile"),
		Destination.Projectile.bEnabled);
	return true;
}
#endif
