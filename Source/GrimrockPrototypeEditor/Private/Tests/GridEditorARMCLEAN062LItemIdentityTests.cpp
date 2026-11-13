#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06ItemIdentityProjection.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062LProductionItems,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionItemIdentityProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062LProductionItems::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Rows;
	FString Error;
	if (!TestTrue(TEXT("Production manifest is valid"),
		FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("22 active item rows"), Rows.Num(), 22))
	{
		return false;
	}
	TSet<FName> SeenItemIds;
	TSet<FName> SeenArmIds;
	TStrongObjectPtr<UObject> Owner(NewObject<UObject>(GetTransientPackage()));
	int32 CustomDefaultLevels = 0;
	int32 TagsTotal = 0;
	int32 NonemptyDescriptions = 0;
	for (const FGridWeaponPipelineRow& Row : Rows)
	{
		const FName ArmId(*Row.ArchetypeId);
		const FName ItemId(*Row.ItemDefinitionId);
		if (Row.ProductionState != TEXT("Active") ||
			SeenItemIds.Contains(ItemId) || SeenArmIds.Contains(ArmId))
		{
			AddError(TEXT("Manifest contains an inactive or duplicate production ARM item"));
			return false;
		}
		SeenItemIds.Add(ItemId);
		SeenArmIds.Add(ArmId);
		const UGridItemDefinitionAsset* Source =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		if (!IsValid(Source) || Source->ItemDefinitionId != ItemId ||
			Source->WeaponArchetypeId != ArmId)
		{
			AddError(TEXT("Missing ARM production concrete definition or identity"));
			return false;
		}
		const bool bWasDirty = Source->GetOutermost()->IsDirty();
		UGridArmamentDefinitionAsset* Candidate =
			NewObject<UGridArmamentDefinitionAsset>(Owner.Get());
		if (!GridArmClean06ItemIdentityProjection::TryProject(*Source, *Candidate) ||
			!GridArmClean06ItemIdentityProjection::Matches(*Source, *Candidate))
		{
			AddError(TEXT("Authored concrete identity/weight/text/tags projection lost data"));
			return false;
		}
		TagsTotal += Candidate->Tags.Num();
		if (!Source->Description.IsEmpty()) { ++NonemptyDescriptions; }
		if (Source->DefaultWeaponItemLevel != 1) { ++CustomDefaultLevels; }
		if (Source->GetOutermost()->IsDirty() != bWasDirty)
		{
			AddError(TEXT("Source ARM identity package was marked dirty"));
			return false;
		}
	}
	TestEqual(TEXT("22 distinct ItemIds"), SeenItemIds.Num(), 22);
	TestEqual(TEXT("22 distinct ArchetypeIds"), SeenArmIds.Num(), 22);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2L: 22 concrete identities/weights projected; %d tags; "
		"%d nonempty descriptions; %d custom source default levels "
		"(migration must preserve default-level semantics separately). No assets saved."),
		TagsTotal, NonemptyDescriptions, CustomDefaultLevels));
	return SeenItemIds.Num() == 22 && SeenArmIds.Num() == 22;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062LIdentitySafety,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ItemIdentityProjectionSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062LIdentitySafety::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemDefinitionAsset> Source(
		NewObject<UGridItemDefinitionAsset>(GetTransientPackage()));
	TStrongObjectPtr<UGridArmamentDefinitionAsset> Candidate(
		NewObject<UGridArmamentDefinitionAsset>(GetTransientPackage()));
	Source->ItemDefinitionId = TEXT("TestWeapon");
	Source->WeaponArchetypeId = TEXT("ARM014");
	Source->DisplayName = FText::FromString(TEXT("Test Sword"));
	Source->Weight = 2.5f;
	Source->bUseWeaponCatalogue = true;
	Source->ItemTags.Add(TEXT("Blade"));
	TestTrue(TEXT("Fresh canonical definition accepts explicit source fields"),
		GridArmClean06ItemIdentityProjection::TryProject(*Source, *Candidate));
	TestTrue(TEXT("All six fields are preserved"),
		GridArmClean06ItemIdentityProjection::Matches(*Source, *Candidate));
	TestEqual(TEXT("One source tag retained"), Candidate->Tags.Num(), 1);
	TestFalse(TEXT("Repeated projection refuses to overwrite an existing definition"),
		GridArmClean06ItemIdentityProjection::TryProject(*Source, *Candidate));
	Source->Weight = -1.0f;
	TStrongObjectPtr<UGridArmamentDefinitionAsset> Rejected(
		NewObject<UGridArmamentDefinitionAsset>(GetTransientPackage()));
	TestFalse(TEXT("Invalid mass refused, not silently clamped"),
		GridArmClean06ItemIdentityProjection::TryProject(*Source, *Rejected));
	TestTrue(TEXT("Rejected candidate has no ItemId"), Rejected->ItemId.IsNone());
	return true;
}
#endif
