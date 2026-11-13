#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06CompleteProductionPreview.h"
#include "ARMEMENT/GridArmClean06PresentationKeyPreview.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ZPresentationKeyPreview,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.AuthoredActionPresentationKeyProposals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062ZPresentationKeyPreview::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Rows;
	FString Error;
	if (!TestTrue(TEXT("Read exact 22 production ARM entries"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
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
	if (!TestNotNull(TEXT("Production numerics"), Numerics) ||
		!TestNotNull(TEXT("Production visuals"), Visuals) ||
		!TestNotNull(TEXT("Production action icons"), Icons))
	{
		return false;
	}
	TStrongObjectPtr<UGridItemSystemAsset> Root(
		NewObject<UGridItemSystemAsset>(GetTransientPackage()));
	GridArmClean06CompleteProductionPreview::FSummary Full;
	if (!TestTrue(TEXT("Construct real 23-item canonical dry-run candidate"),
		GridArmClean06CompleteProductionPreview::Build(
			Rows, *Numerics, *Visuals, *Icons, *Root, Full)))
	{
		return false;
	}
	const int32 InitialActionCount = Root->ActionCatalogue->Actions.Num();
	const int32 InitialPresentationCount =
		Root->ActionPresentationCatalogue->Presentations.Num();
	GridArmClean06PresentationKeyPreview::FSummary Keys;
	if (!TestTrue(TEXT("Classify all 19 attack keys without mutating source"),
		GridArmClean06PresentationKeyPreview::Build(Rows, *Root, Keys)))
	{
		return false;
	}
	TestTrue(TEXT("Exactly 19 actual attacks classified"), Keys.AllAttacksClassified());
	TestEqual(TEXT("No invented attack in the three passive ARM"), Keys.PassiveItems, 3);
	TestEqual(TEXT("Existing authored bindings are not proposed again"),
		Keys.ExistingBoundActions, Full.Core.Actions.BoundActions);
	TestEqual(TEXT("Other local custom actions remain unconverted"),
		Keys.OtherLocalActionsPreserved,
		Full.Core.Actions.OtherLocalActionsUnconverted);
	TestTrue(TEXT("No more proposals than real unbound attacks"),
		Keys.ProposedFromExistingActionId <= Full.Core.UnboundOffensiveArmaments);
	TSet<FName> UniqueKeys;
	for (const GridArmClean06PresentationKeyPreview::FProposal& Proposal : Keys.Proposals)
	{
		if (UniqueKeys.Contains(Proposal.Presentation.PresentationId) ||
			Proposal.ExistingActionId != Proposal.Presentation.PresentationId ||
			!Proposal.Presentation.IsValid() ||
			!Root->ActionCatalogue->Find(Proposal.ExistingActionId) ||
			Root->ActionPresentationCatalogue->Find(Proposal.Presentation.PresentationId))
		{
			AddError(TEXT("A key is synthetic, duplicated, invalid or already authored"));
			return false;
		}
		UniqueKeys.Add(Proposal.Presentation.PresentationId);
	}
	TestEqual(TEXT("Each proposed key reuses a distinct real ActionId"),
		UniqueKeys.Num(), Keys.ProposedFromExistingActionId);
	TestEqual(TEXT("Action catalogue not edited by proposal"), 
		Root->ActionCatalogue->Actions.Num(), InitialActionCount);
	TestEqual(TEXT("Presentation catalogue not edited by proposal"),
		Root->ActionPresentationCatalogue->Presentations.Num(), InitialPresentationCount);
	TestFalse(TEXT("Proposed migration cannot authorize a binary cutover"),
		Full.ReadyForBinaryCutover());

	GridArmClean06PresentationKeyPreview::FSummary Replay;
	TestTrue(TEXT("Read-only proposed keys are deterministic"),
		GridArmClean06PresentationKeyPreview::Build(Rows, *Root, Replay));
	TestEqual(TEXT("Same number of proposed real keys"),
		Replay.ProposedFromExistingActionId, Keys.ProposedFromExistingActionId);
	TestEqual(TEXT("Same number of unconverted source actions"),
		Replay.UnconvertedPrimaryAction, Keys.UnconvertedPrimaryAction);
	AddInfo(FString::Printf(TEXT(
		"ARM-CLEAN06.2Z: 19 primary actions; %d existing authoritative bindings; "
		"%d existing ActionIds eligible as provisional presentation keys; "
		"%d no valid local presentations; %d unconverted attacks; "
		"%d explicit IDs not resolved; %d conflicts; %d invalid presentations; "
		"%d other local actions retained. Zero assets modified and cutover refused."),
		Keys.ExistingBoundActions,
		Keys.ProposedFromExistingActionId,
		Keys.MissingLocalPresentation,
		Keys.UnconvertedPrimaryAction,
		Keys.ExplicitPresentationIdStillUnresolved,
		Keys.ConflictingPresentationId,
		Keys.PresentationProjectionRejected,
		Keys.OtherLocalActionsPreserved));
	return Keys.AllAttacksClassified() &&
		Root->ActionPresentationCatalogue->Presentations.Num() ==
			InitialPresentationCount;
}

#endif
