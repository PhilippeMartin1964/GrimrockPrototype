#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06VisualProjection.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062JVisualProjectionTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionVisualProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062JVisualProjectionTest::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Rows;
	FString Error;
	if (!TestTrue(TEXT("22-production manifest read"),
		FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("Production ARM count"), Rows.Num(), 22))
	{
		return false;
	}
	const UGridWeaponVisualCatalogueAsset* Source =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue"));
	if (!TestNotNull(TEXT("Legacy weapon visual catalogue"), Source) ||
		!TestTrue(TEXT("Legacy visual catalogue structurally valid"), Source->IsValidCatalogue()))
	{
		return false;
	}

	const bool bSourceWasDirty = Source->GetOutermost()->IsDirty();
	TStrongObjectPtr<UGridCanonicalWeaponVisualCatalogueAsset> Candidate(
		NewObject<UGridCanonicalWeaponVisualCatalogueAsset>(GetTransientPackage()));
	TSet<FName> UsedVisualIds;
	TSet<FName> UsedArchetypes;
	int32 LegacyDirectIconDeltas = 0;
	int32 LegacyDirectMeshDeltas = 0;
	int32 LegacyEquippedFallbacks = 0;
	for (const FGridWeaponPipelineRow& Row : Rows)
	{
		const FName ArmId(*Row.ArchetypeId);
		const FName VisualId(*Row.VisualId);
		if (Row.ProductionState != TEXT("Active") || UsedArchetypes.Contains(ArmId))
		{
			AddError(TEXT("Duplicate or inactive ARM production record"));
			return false;
		}
		UsedArchetypes.Add(ArmId);
		const FGridWeaponVisualProfile* SourceVisual = Source->FindProfileForArchetype(ArmId);
		const UGridItemDefinitionAsset* Item =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		if (!SourceVisual || SourceVisual->VisualId != VisualId ||
			!IsValid(Item) || Item->WeaponArchetypeId != ArmId ||
			Item->ItemDefinitionId != FName(*Row.ItemDefinitionId) ||
			!Item->bUseWeaponVisualCatalogue)
		{
			AddError(TEXT("Production visual identity is not aligned"));
			return false;
		}
		const bool bItemWasDirty = Item->GetOutermost()->IsDirty();
		FGridCanonicalWeaponVisualBinding Binding;
		Binding.ArchetypeId = ArmId;
		Binding.VisualId = VisualId;
		Candidate->ArchetypeBindings.Add(Binding);
		if (!UsedVisualIds.Contains(VisualId))
		{
			UsedVisualIds.Add(VisualId);
			FGridCanonicalWeaponVisual Canonical;
			if (!GridArmClean06VisualProjection::TryProject(*SourceVisual, Canonical))
			{
				AddError(TEXT("A real ARM visual cannot project without loss"));
				return false;
			}
			Candidate->VisualProfiles.Add(Canonical);
		}
		const FGridCanonicalWeaponVisual* Check = Candidate->FindForArchetype(ArmId);
		if (!Check || !GridArmClean06VisualProjection::EqualsSource(*SourceVisual, *Check))
		{
			AddError(TEXT("ARM binding loses an authored visual field"));
			return false;
		}
		if (Item->Icon.ToSoftObjectPath() != SourceVisual->BaseIcon.ToSoftObjectPath())
		{
			++LegacyDirectIconDeltas;
		}
		if (Item->WorldMesh.ToSoftObjectPath() != SourceVisual->WorldMesh.ToSoftObjectPath())
		{
			++LegacyDirectMeshDeltas;
		}
		if (SourceVisual->EquippedMesh.IsNull()) { ++LegacyEquippedFallbacks; }
		if (Item->GetOutermost()->IsDirty() != bItemWasDirty)
		{
			AddError(TEXT("Visual projection changed an original item package"));
			return false;
		}
	}
	TArray<FString> Errors;
	TestTrue(TEXT("Canonical production visuals satisfy schema"), Candidate->Validate(Errors));
	TestEqual(TEXT("Exactly 22 visual bindings"), Candidate->ArchetypeBindings.Num(), 22);
	TestTrue(TEXT("No art package dirtied"), Source->GetOutermost()->IsDirty() == bSourceWasDirty);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2J: 22 bindings; %d distinct visual profiles; "
		"%d icon overrides; %d world-mesh overrides; %d equipped-mesh fallbacks. "
		"All authored profile fields projected without saving."),
		UsedVisualIds.Num(), LegacyDirectIconDeltas, LegacyDirectMeshDeltas,
		LegacyEquippedFallbacks));
	return Errors.IsEmpty() && Candidate->ArchetypeBindings.Num() == 22 &&
		Source->GetOutermost()->IsDirty() == bSourceWasDirty;
}
#endif
