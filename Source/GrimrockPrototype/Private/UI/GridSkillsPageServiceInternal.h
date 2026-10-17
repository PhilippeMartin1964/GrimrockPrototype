#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsUiTypes.h"

namespace GridSkillsPageServicePrivate
{
	void BuildSatisfiedIdsForChoice(
		const FRPGClassProgressionChoiceDefinition& Choice,
		TSet<FName>& OutIds);

	void BuildAcquisitionView(
		const URPGClassAsset& ClassDefinition,
		const FRPGClassProgressionChoiceDefinition& Choice,
		bool bVariantNode,
		FGridTalentAcquisitionView& Out);

	void BuildStructuredUsage(
		const URPGClassAsset& ClassDefinition,
		const FRPGClassProgressionChoiceDefinition& Choice,
		TArray<FGridTalentDetailLineView>& Out);

	FText ResolveStatusReferences(const FText& Source);

	void BuildStructuredEffects(
		const URPGClassAsset& ClassDefinition,
		const FRPGClassProgressionChoiceDefinition& Choice,
		TArray<FGridTalentDetailLineView>& Out);

	bool ResolveConceptPresentation(
		FName ClassId,
		FName BranchId,
		FName NodeId,
		FText& OutName,
		FText& OutPrinciple);

	bool BuildTalentTree(
		UGridPartyInventoryComponent* PartyInventoryComponent,
		int32 CharacterIndex,
		const FGridCharacterInventoryState& Character,
		URPGClassAsset& ClassDefinition,
		FGridTalentTreeView& OutTree);
}
