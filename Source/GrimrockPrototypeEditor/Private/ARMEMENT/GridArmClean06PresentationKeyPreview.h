#pragma once

#include "CoreMinimal.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

/**
 * Read-only, Editor-only *proposal* to reuse an existing authored ActionId
 * as the presentation key when no PresentationProfileId was ever authored.
 * Never mutates the source DataAsset or the candidate root.
 */
namespace GridArmClean06PresentationKeyPreview
{
	struct FProposal
	{
		FName ArchetypeId = NAME_None;
		FName ExistingActionId = NAME_None;
		FGridCanonicalActionPresentation Presentation;
	};

	struct FSummary
	{
		int32 PassiveItems = 0;
		int32 ExistingBoundActions = 0;
		int32 ProposedFromExistingActionId = 0;
		int32 MissingLocalPresentation = 0;
		int32 UnconvertedPrimaryAction = 0;
		int32 ExplicitPresentationIdStillUnresolved = 0;
		int32 ConflictingPresentationId = 0;
		int32 PresentationProjectionRejected = 0;
		int32 OtherLocalActionsPreserved = 0;
		TArray<FProposal> Proposals;

		bool AllAttacksClassified() const
		{
			return PassiveItems == 3 &&
				ExistingBoundActions + ProposedFromExistingActionId +
					MissingLocalPresentation + UnconvertedPrimaryAction +
					ExplicitPresentationIdStillUnresolved + ConflictingPresentationId +
					PresentationProjectionRejected == 19 &&
				ProposedFromExistingActionId == Proposals.Num();
		}
	};

	bool Build(const TArray<FGridWeaponPipelineRow>& Manifest,
		const UGridItemSystemAsset& Root,
		FSummary& OutSummary);
}
