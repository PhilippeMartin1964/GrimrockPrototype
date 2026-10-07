#include "UI/GridSkillsPageService.h"

#include "Engine/AssetManager.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillService.h"
#include "RPG/RPGTalentRuntimeService.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace
{
	const FPrimaryAssetType RPGSkillPrimaryAssetType(TEXT("RPGSkill"));
	using FChoiceArray = TArray<const FRPGClassProgressionChoiceDefinition*>;

	bool ValidateAndSortDefinitions(const TArray<const URPGSkillAsset*>& SkillDefinitions, TArray<const URPGSkillAsset*>& OutSortedDefinitions)
	{
		OutSortedDefinitions.Reset(SkillDefinitions.Num());
		TSet<FName> SeenSkillIds;
		for (const URPGSkillAsset* Definition : SkillDefinitions)
		{
			if (!IsValid(Definition) || !Definition->IsValidDefinition() || SeenSkillIds.Contains(Definition->SkillId))
			{
				OutSortedDefinitions.Reset();
				return false;
			}
			SeenSkillIds.Add(Definition->SkillId);
			OutSortedDefinitions.Add(Definition);
		}
		OutSortedDefinitions.Sort(
			[](const URPGSkillAsset& Left, const URPGSkillAsset& Right)
			{
				const FString LeftLabel = Left.DisplayName.IsEmpty() ? Left.SkillId.ToString() : Left.DisplayName.ToString();
				const FString RightLabel = Right.DisplayName.IsEmpty() ? Right.SkillId.ToString() : Right.DisplayName.ToString();
				const int32 LabelComparison = LeftLabel.Compare(RightLabel, ESearchCase::IgnoreCase);
				if (LabelComparison != 0)
				{
					return LabelComparison < 0;
				}
				return Left.SkillId.ToString().Compare(Right.SkillId.ToString(), ESearchCase::CaseSensitive) < 0;
			});
		return true;
	}

	URPGClassAsset* ResolveClassDefinition(const FGridCharacterInventoryState& Character)
	{
		URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
		if (!FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(Character.ClassId, ClassDefinition))
		{
			ClassDefinition = FRPGAuthoringIdentityResolver::ResolveClassById(Character.ClassId);
		}
		return IsValid(ClassDefinition) && ClassDefinition->IsValidDefinition() ? ClassDefinition : nullptr;
	}

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

	void BuildSatisfiedIdsForChoice(const FRPGClassProgressionChoiceDefinition& Choice, TSet<FName>& OutIds)
	{
		OutIds.Reset();
		OutIds.Add(Choice.ChoiceId);
		for (const FName RequirementId : Choice.GrantedRequirementIds) OutIds.Add(RequirementId);
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
			Variant.Description = Choice->Description;
			Variant.bSelected = bSelected;
			if (!TryMapChoiceState(bSelected, Availability, Variant.State)) return false;
			Variant.bAvailable = Variant.State == EGridTalentNodeState::Available;
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
					Node.PreviousNodeId = Previous.TalentNodeId;
				}
				if (Node.State == EGridTalentNodeState::Acquired) ++Branch.AcquiredNodeCount;
			}
			OutTree.Branches.Add(MoveTemp(Branch));
		}
		return true;
	}
}

bool FGridSkillsPageService::TryBuildCharacterView(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex,
	const TArray<const URPGSkillAsset*>& SkillDefinitions, FGridSkillsPageView& OutView)
{
	OutView = FGridSkillsPageView();
	if (!IsValid(PartyInventoryComponent) || !PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex)) return false;

	const FGridCharacterInventoryState& Character = PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	URPGClassAsset* ClassDefinition = ResolveClassDefinition(Character);
	if (!Character.CharacterId.IsValid() || !ClassDefinition || !FRPGSkillService::ValidateSkillState(Character)) return false;

	TArray<const URPGSkillAsset*> SortedDefinitions;
	if (!ValidateAndSortDefinitions(SkillDefinitions, SortedDefinitions)) return false;

	TSet<FName> DefinitionSkillIds;
	for (const URPGSkillAsset* Definition : SortedDefinitions) DefinitionSkillIds.Add(Definition->SkillId);
	for (const FRPGSkillRank& SkillRank : Character.SkillRanks)
	{
		if (!DefinitionSkillIds.Contains(SkillRank.SkillId)) return false;
	}

	FGridSkillsPageView Candidate;
	Candidate.CharacterIndex = CharacterIndex;
	Candidate.CharacterId = Character.CharacterId;
	Candidate.CharacterName = Character.DisplayName;
	Candidate.CharacterLevel = Character.Level;
	Candidate.ClassId = ClassDefinition->ClassId;
	Candidate.ClassDisplayName = ClassDefinition->DisplayName;

	Candidate.Skills.Reserve(SortedDefinitions.Num());
	for (const URPGSkillAsset* Definition : SortedDefinitions)
	{
		const int32 Rank = FRPGSkillService::GetSkillRank(Character, Definition->SkillId);
		if (Rank < 0 || Rank > Definition->MaxRank) return false;
		FGridSkillEntryView SkillView;
		SkillView.SkillId = Definition->SkillId;
		SkillView.DisplayName = Definition->DisplayName;
		SkillView.Description = Definition->Description;
		SkillView.GoverningAttribute = Definition->GoverningAttribute;
		SkillView.Rank = Rank;
		SkillView.MaxRank = Definition->MaxRank;
		SkillView.bAllowUntrainedChecks = Definition->bAllowUntrainedChecks;
		SkillView.bTrained = Rank > 0;
		Candidate.Skills.Add(MoveTemp(SkillView));
	}

	TArray<FRPGTalentRuntimeView> TalentViews;
	if (!FRPGTalentRuntimeService::TryGetSelectedTalents(PartyInventoryComponent, CharacterIndex, TalentViews)) return false;
	TalentViews.Sort([](const FRPGTalentRuntimeView& Left, const FRPGTalentRuntimeView& Right)
	{
		return Left.ChoiceId.ToString().Compare(Right.ChoiceId.ToString(), ESearchCase::CaseSensitive) < 0;
	});
	Candidate.Talents.Reserve(TalentViews.Num());
	for (const FRPGTalentRuntimeView& Talent : TalentViews)
	{
		FGridTalentEntryView TalentView;
		TalentView.ChoiceId = Talent.ChoiceId;
		TalentView.DisplayName = Talent.DisplayName;
		TalentView.Description = Talent.Description;
		TalentView.MinimumLevel = Talent.MinimumLevel;
		TalentView.PointCost = Talent.PointCost;
		TalentView.bSelected = Talent.bSelected;
		Candidate.Talents.Add(MoveTemp(TalentView));
	}

	FRPGTalentPointBalance Balance;
	if (!FRPGTalentRuntimeService::TryGetTalentPointBalance(PartyInventoryComponent, CharacterIndex, Balance)) return false;
	Candidate.GrantedTalentPoints = Balance.GrantedPoints;
	Candidate.SpentTalentPoints = Balance.SpentPoints;
	Candidate.RemainingTalentPoints = Balance.RemainingPoints;

	if (!BuildTalentTree(PartyInventoryComponent, CharacterIndex, Character, *ClassDefinition, Candidate.TalentTree)) return false;
	OutView = MoveTemp(Candidate);
	return true;
}

bool FGridSkillsPageService::TryBuildSelectedCharacterView(
	UGridPartyInventoryComponent* PartyInventoryComponent, const TArray<const URPGSkillAsset*>& SkillDefinitions, FGridSkillsPageView& OutView)
{
	OutView = FGridSkillsPageView();
	if (!IsValid(PartyInventoryComponent)) return false;
	return TryBuildCharacterView(PartyInventoryComponent, PartyInventoryComponent->GetSelectedCharacterIndex(), SkillDefinitions, OutView);
}

void FGridSkillsPageService::ResolveCanonicalSkillDefinitions(TArray<const URPGSkillAsset*>& OutDefinitions)
{
	OutDefinitions.Reset();
	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FString> SearchPaths;
	SearchPaths.Add(TEXT("/Game"));
	AssetManager.ScanPathsForPrimaryAssets(RPGSkillPrimaryAssetType, SearchPaths, URPGSkillAsset::StaticClass(), false, false, true);

	TArray<FPrimaryAssetId> AssetIds;
	AssetManager.GetPrimaryAssetIdList(RPGSkillPrimaryAssetType, AssetIds);
	AssetIds.Sort([](const FPrimaryAssetId& Left, const FPrimaryAssetId& Right)
	{
		return Left.PrimaryAssetName.ToString().Compare(Right.PrimaryAssetName.ToString(), ESearchCase::CaseSensitive) < 0;
	});

	TSet<FName> SeenSkillIds;
	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		URPGSkillAsset* Definition = AssetManager.GetPrimaryAssetObject<URPGSkillAsset>(AssetId);
		if (!IsValid(Definition))
		{
			const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);
			if (AssetPath.IsValid()) Definition = Cast<URPGSkillAsset>(AssetPath.TryLoad());
		}
		if (!IsValid(Definition) || !Definition->IsValidDefinition() || Definition->GetPrimaryAssetId() != AssetId || SeenSkillIds.Contains(Definition->SkillId)) continue;
		SeenSkillIds.Add(Definition->SkillId);
		OutDefinitions.Add(Definition);
	}
}
