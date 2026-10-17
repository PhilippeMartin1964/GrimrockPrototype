#include "UI/GridSkillsPageService.h"

#include "UI/RPGTalentPresentationAsset.h"
#include "UI/RPGSkillPresentationAsset.h"
#include "Runtime/Monsters/GridMonsterCategoryAsset.h"

#include "Engine/AssetManager.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGRecipeCatalogAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "RPG/RPGSkillPointService.h"
#include "RPG/RPGSkillRequirementProjectionService.h"
#include "RPG/RPGSkillService.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectPersistence.h"
#include "Runtime/GridPartyInventoryComponent.h"

#include "GridSkillsPageServiceInternal.h"

namespace GridSkillsPageServicePrivate
{
	using FChoiceArray = TArray<const FRPGClassProgressionChoiceDefinition*>;

	int32 ResolveTalentTier(int32 MinimumLevel)
	{
		switch (MinimumLevel)
		{
			case 2: return 1;
			case 6: return 2;
			case 10: return 3;
			case 14: return 4;
			case 18: return 5;
			default: return 0;
		}
	}

	bool BuildSelectedChoiceSet(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, TSet<FName>& OutSelectedChoiceIds)
	{
		OutSelectedChoiceIds.Reset();
		TArray<FName> SelectedChoiceIds;
		if (!FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(PartyInventoryComponent, CharacterIndex, SelectedChoiceIds))
		{
			return false;
		}
		for (const FName ChoiceId : SelectedChoiceIds)
		{
			if (ChoiceId.IsNone() || OutSelectedChoiceIds.Contains(ChoiceId))
			{
				OutSelectedChoiceIds.Reset();
				return false;
			}
			OutSelectedChoiceIds.Add(ChoiceId);
		}
		return true;
	}

	bool TryMapChoiceState(bool bSelected, ERPGClassProgressionChoiceAvailabilityReason Availability, EGridTalentNodeState& OutState)
	{
		if (bSelected)
		{
			OutState = EGridTalentNodeState::Acquired;
			return Availability == ERPGClassProgressionChoiceAvailabilityReason::AlreadySelected ||
				Availability == ERPGClassProgressionChoiceAvailabilityReason::None;
		}
		switch (Availability)
		{
			case ERPGClassProgressionChoiceAvailabilityReason::None:
				OutState = EGridTalentNodeState::Available; return true;
			case ERPGClassProgressionChoiceAvailabilityReason::LevelTooLow:
				OutState = EGridTalentNodeState::LockedLevel; return true;
			case ERPGClassProgressionChoiceAvailabilityReason::MissingPrerequisite:
				OutState = EGridTalentNodeState::LockedPrerequisite; return true;
			case ERPGClassProgressionChoiceAvailabilityReason::InsufficientChoicePoints:
				OutState = EGridTalentNodeState::LockedPoints; return true;
			case ERPGClassProgressionChoiceAvailabilityReason::MutuallyExclusiveChoice:
				OutState = EGridTalentNodeState::LockedExclusive; return true;
			default:
				return false;
		}
	}

	bool AreNameArraysEquivalent(const TArray<FName>& Left, const TArray<FName>& Right)
	{
		if (Left.Num() != Right.Num()) return false;
		TSet<FName> RightSet;
		for (const FName Id : Right)
		{
			if (Id.IsNone() || RightSet.Contains(Id)) return false;
			RightSet.Add(Id);
		}
		TSet<FName> LeftSet;
		for (const FName Id : Left)
		{
			if (Id.IsNone() || LeftSet.Contains(Id) || !RightSet.Contains(Id)) return false;
			LeftSet.Add(Id);
		}
		return LeftSet.Num() == RightSet.Num();
	}

	bool BuildCommonSatisfiedIds(const FChoiceArray& Choices, TSet<FName>& OutCommonIds)
	{
		OutCommonIds.Reset();
		if (Choices.IsEmpty() || !Choices[0]) return false;
		BuildSatisfiedIdsForChoice(*Choices[0], OutCommonIds);
		for (int32 Index = 1; Index < Choices.Num(); ++Index)
		{
			if (!Choices[Index]) return false;
			TSet<FName> CurrentIds;
			BuildSatisfiedIdsForChoice(*Choices[Index], CurrentIds);
			TArray<FName> ToRemove;
			for (const FName ExistingId : OutCommonIds)
			{
				if (!CurrentIds.Contains(ExistingId)) ToRemove.Add(ExistingId);
			}
			for (const FName Id : ToRemove) OutCommonIds.Remove(Id);
		}
		return !OutCommonIds.IsEmpty();
	}

	bool NodeDependsOnPrevious(const FChoiceArray& CurrentChoices, const FChoiceArray& PreviousChoices)
	{
		TSet<FName> CommonPreviousIds;
		if (!BuildCommonSatisfiedIds(PreviousChoices, CommonPreviousIds)) return false;
		for (const FRPGClassProgressionChoiceDefinition* Choice : CurrentChoices)
		{
			if (!Choice) return false;
			bool bDependsOnPrevious = false;
			for (const FName PrerequisiteId : Choice->PrerequisiteChoiceIds)
			{
				if (CommonPreviousIds.Contains(PrerequisiteId)) { bDependsOnPrevious = true; break; }
			}
			if (!bDependsOnPrevious)
			{
				for (const FName RequirementId : Choice->PrerequisiteRequirementIds)
				{
					if (CommonPreviousIds.Contains(RequirementId)) { bDependsOnPrevious = true; break; }
				}
			}
			if (!bDependsOnPrevious) return false;
		}
		return true;
	}

	bool BuildTalentNode(const URPGClassAsset& ClassDefinition, int32 CharacterLevel, const TSet<FName>& SelectedChoiceIds,
		FName BranchId, FName NodeId, FChoiceArray Choices, FGridTalentNodeView& OutNode)
	{
		OutNode = FGridTalentNodeView();
		if (Choices.IsEmpty() || BranchId.IsNone() || NodeId.IsNone()) return false;
		Choices.Sort(
			[](const FRPGClassProgressionChoiceDefinition& Left, const FRPGClassProgressionChoiceDefinition& Right)
			{
				return Left.ChoiceId.ToString().Compare(Right.ChoiceId.ToString(), ESearchCase::CaseSensitive) < 0;
			});
		const FRPGClassProgressionChoiceDefinition* First = Choices[0];
		if (!First) return false;
		const int32 Tier = ResolveTalentTier(First->MinimumLevel);
		if (Tier == 0) return false;

		const bool bVariantNode = Choices.Num() > 1;
		const FName VariantGroup = First->ExclusiveChoiceGroupId;
		if (!bVariantNode && First->ChoiceId != NodeId) return false;
		if (bVariantNode && VariantGroup.IsNone()) return false;

		for (const FRPGClassProgressionChoiceDefinition* Choice : Choices)
		{
			if (!Choice || Choice->TalentBranchId != BranchId || Choice->TalentNodeId != NodeId ||
				Choice->MinimumLevel != First->MinimumLevel || Choice->PointCost != First->PointCost) return false;
			if (bVariantNode &&
				(Choice->ExclusiveChoiceGroupId != VariantGroup || !Choice->GrantedRequirementIds.Contains(NodeId) ||
					!AreNameArraysEquivalent(Choice->PrerequisiteChoiceIds, First->PrerequisiteChoiceIds) ||
					!AreNameArraysEquivalent(Choice->PrerequisiteRequirementIds, First->PrerequisiteRequirementIds))) return false;
		}

		OutNode.TalentNodeId = NodeId;
		OutNode.TalentBranchId = BranchId;
		OutNode.Tier = Tier;
		OutNode.MinimumLevel = First->MinimumLevel;
		OutNode.PointCost = First->PointCost;
		OutNode.bHasExclusiveVariants = bVariantNode;
		OutNode.Variants.Reserve(Choices.Num());

		int32 SelectedVariantCount = 0;
		bool bHaveUnselectedState = false;
		EGridTalentNodeState CommonUnselectedState = EGridTalentNodeState::LockedPrerequisite;
		for (const FRPGClassProgressionChoiceDefinition* Choice : Choices)
		{
			const bool bSelected = SelectedChoiceIds.Contains(Choice->ChoiceId);
			const ERPGClassProgressionChoiceAvailabilityReason Availability =
				FRPGClassProgressionService::GetChoiceAvailability(&ClassDefinition, CharacterLevel, SelectedChoiceIds, Choice->ChoiceId);
			FGridTalentVariantView Variant;
			Variant.ChoiceId = Choice->ChoiceId;
			Variant.DisplayName = Choice->DisplayName;
			Variant.Principle = ResolveStatusReferences(Choice->Description);
			Variant.Type = Choice->PresentationType;
			BuildStructuredEffects(ClassDefinition, *Choice, Variant.Effects);
			BuildStructuredUsage(ClassDefinition, *Choice, Variant.Usage);
			Variant.bAcquired = bSelected;
			if (!TryMapChoiceState(bSelected, Availability, Variant.State)) return false;
			Variant.bCanChoose =
				bVariantNode && Variant.State == EGridTalentNodeState::Available && !Variant.bAcquired;


			OutNode.Variants.Add(MoveTemp(Variant));

			if (bSelected)
			{
				++SelectedVariantCount;
				OutNode.SelectedChoiceId = Choice->ChoiceId;
			}
			else if (!bHaveUnselectedState)
			{
				CommonUnselectedState = OutNode.Variants.Last().State;
				bHaveUnselectedState = true;
			}
			else if (SelectedVariantCount == 0 && CommonUnselectedState != OutNode.Variants.Last().State)
			{
				return false;
			}
		}
		if (SelectedVariantCount > 1) return false;
		OutNode.State = SelectedVariantCount == 1 ? EGridTalentNodeState::Acquired : CommonUnselectedState;

		OutNode.Type = OutNode.Variants[0].Type;
		for (const FGridTalentVariantView& Variant : OutNode.Variants)
		{
			if (Variant.Type != OutNode.Type) return false;
		}
		BuildAcquisitionView(ClassDefinition, *First, bVariantNode, OutNode.Acquisition);

		if (bVariantNode)
		{
			FText OverrideName;
			FText OverridePrinciple;
			if (ResolveConceptPresentation(
				ClassDefinition.ClassId, BranchId, NodeId, OverrideName, OverridePrinciple) &&
				!OverrideName.IsEmpty())
			{
				OutNode.DisplayName = OverrideName;
				OutNode.Principle = ResolveStatusReferences(OverridePrinciple);
			}
			else
			{
				// Generic/synthetic class definitions may not belong to the production
				// presentation catalog. Keep their authored player text intact without
				// parsing labels or exposing a technical id. Production coverage is
				// enforced separately by the serialized presentation tests.
				if (First->DisplayName.IsEmpty())
				{
					return false;
				}
				OutNode.DisplayName = First->DisplayName;
				OutNode.Principle = ResolveStatusReferences(First->Description);
			}
		}
		else
		{
			if (First->DisplayName.IsEmpty())
			{
				return false;
			}
			OutNode.DisplayName = First->DisplayName;
			OutNode.Principle = ResolveStatusReferences(First->Description);
			OutNode.Effects = OutNode.Variants[0].Effects;
			OutNode.Usage = OutNode.Variants[0].Usage;
			OutNode.SimpleChoiceId = First->ChoiceId;
			OutNode.bCanAcquireSimple =
				OutNode.State == EGridTalentNodeState::Available && !SelectedChoiceIds.Contains(First->ChoiceId);
			// Simple Talents are not one-option variant nodes. Keep Variants reserved for
			// the four true exclusive-choice concepts.
			OutNode.Variants.Reset();
		}
		return SelectedVariantCount == 1 || bHaveUnselectedState;
	}

	bool BuildTalentTree(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex,
		const FGridCharacterInventoryState& Character, URPGClassAsset& ClassDefinition, FGridTalentTreeView& OutTree)
	{
		OutTree = FGridTalentTreeView();
		OutTree.ClassId = ClassDefinition.ClassId;
		if (ClassDefinition.ProgressionChoices.IsEmpty()) return true;

		bool bAnyStructuralMetadata = false;
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassDefinition.ProgressionChoices)
		{
			const bool bHasBranch = !Choice.TalentBranchId.IsNone();
			const bool bHasNode = !Choice.TalentNodeId.IsNone();
			if (bHasBranch != bHasNode) return false;
			bAnyStructuralMetadata |= bHasBranch;
		}
		if (!bAnyStructuralMetadata) return true;

		TSet<FName> SelectedChoiceIds;
		if (!BuildSelectedChoiceSet(PartyInventoryComponent, CharacterIndex, SelectedChoiceIds)) return false;

		TMap<FName, TMap<FName, FChoiceArray>> ChoicesByBranch;
		TMap<FName, FName> BranchByNode;
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassDefinition.ProgressionChoices)
		{
			if (Choice.TalentBranchId.IsNone() || Choice.TalentNodeId.IsNone() || ResolveTalentTier(Choice.MinimumLevel) == 0) return false;
			if (const FName* ExistingBranch = BranchByNode.Find(Choice.TalentNodeId); ExistingBranch && *ExistingBranch != Choice.TalentBranchId) return false;
			BranchByNode.FindOrAdd(Choice.TalentNodeId) = Choice.TalentBranchId;
			ChoicesByBranch.FindOrAdd(Choice.TalentBranchId).FindOrAdd(Choice.TalentNodeId).Add(&Choice);
		}

		TArray<FName> BranchIds;
		ChoicesByBranch.GetKeys(BranchIds);
		BranchIds.Sort([](const FName Left, const FName Right)
		{
			return Left.ToString().Compare(Right.ToString(), ESearchCase::CaseSensitive) < 0;
		});

		for (const FName BranchId : BranchIds)
		{
			const TMap<FName, FChoiceArray>* NodeGroups = ChoicesByBranch.Find(BranchId);
			if (!NodeGroups) return false;
			FGridTalentBranchView Branch;
			Branch.TalentBranchId = BranchId;
			TArray<FName> NodeIds;
			NodeGroups->GetKeys(NodeIds);
			for (const FName NodeId : NodeIds)
			{
				const FChoiceArray* NodeChoices = NodeGroups->Find(NodeId);
				FGridTalentNodeView Node;
				if (!NodeChoices || !BuildTalentNode(ClassDefinition, Character.Level, SelectedChoiceIds, BranchId, NodeId, *NodeChoices, Node)) return false;
				Branch.Nodes.Add(MoveTemp(Node));
			}
			Branch.Nodes.Sort([](const FGridTalentNodeView& Left, const FGridTalentNodeView& Right)
			{
				if (Left.MinimumLevel != Right.MinimumLevel) return Left.MinimumLevel < Right.MinimumLevel;
				return Left.TalentNodeId.ToString().Compare(Right.TalentNodeId.ToString(), ESearchCase::CaseSensitive) < 0;
			});
			for (int32 NodeIndex = 0; NodeIndex < Branch.Nodes.Num(); ++NodeIndex)
			{
				FGridTalentNodeView& Node = Branch.Nodes[NodeIndex];
				if (NodeIndex > 0)
				{
					const FGridTalentNodeView& Previous = Branch.Nodes[NodeIndex - 1];
					if (Previous.MinimumLevel == Node.MinimumLevel) return false;
					const FChoiceArray* CurrentChoices = NodeGroups->Find(Node.TalentNodeId);
					const FChoiceArray* PreviousChoices = NodeGroups->Find(Previous.TalentNodeId);
					if (!CurrentChoices || !PreviousChoices || !NodeDependsOnPrevious(*CurrentChoices, *PreviousChoices)) return false;
					if (Previous.DisplayName.IsEmpty()) return false;
					Node.PreviousNodeId = Previous.TalentNodeId;
					Node.PreviousNodeDisplayName = Previous.DisplayName;
				}
				if (Node.State == EGridTalentNodeState::Acquired) ++Branch.AcquiredNodeCount;
			}
			OutTree.Branches.Add(MoveTemp(Branch));
		}
		return true;
	}
}
