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

namespace
{
	const FPrimaryAssetType RPGSkillPrimaryAssetType(TEXT("RPGSkill"));

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

	FRPGSkillPointBalance SkillPointBalance;
	if (!FRPGSkillPointService::TryGetBalance(Character, SkillPointBalance)) return false;
	Candidate.GrantedSkillPoints = SkillPointBalance.GrantedPoints;
	Candidate.SpentSkillPoints = SkillPointBalance.SpentPoints;
	Candidate.RemainingSkillPoints = SkillPointBalance.RemainingPoints;
	Candidate.SkillRankCap = SkillPointBalance.RankCap;

	Candidate.Skills.Reserve(SortedDefinitions.Num());
	for (const URPGSkillAsset* Definition : SortedDefinitions)
	{
		const int32 Rank = FRPGSkillService::GetSkillRank(Character, Definition->SkillId);
		if (Rank < 0 || Rank > Definition->MaxRank) return false;
		FGridSkillEntryView SkillView;
		SkillView.SkillId = Definition->SkillId;
		SkillView.DisplayName = Definition->DisplayName;
		SkillView.GoverningAttribute = Definition->GoverningAttribute;
		SkillView.Rank = Rank;
		SkillView.MaxRank = Definition->MaxRank;
		SkillView.bAllowUntrainedChecks = Definition->bAllowUntrainedChecks;
		SkillView.bTrained = Rank > 0;
		SkillView.CurrentRankCap = FMath::Min(Definition->MaxRank, SkillPointBalance.RankCap);
		SkillView.bCanIncreaseRank =
			FRPGSkillPointService::GetNextRankPurchaseAvailability(Character, Definition) ==
			ERPGSkillPointMutationRejectReason::None;
		if (!SkillView.IsValid()) return false;
		Candidate.Skills.Add(MoveTemp(SkillView));
	}

	if (!FRPGClassProgressionTransactionService::TryGetChoicePointBalance(
			PartyInventoryComponent,
			CharacterIndex,
			Candidate.GrantedTalentPoints,
			Candidate.SpentTalentPoints,
			Candidate.RemainingTalentPoints))
	{
		return false;
	}

	if (!GridSkillsPageServicePrivate::BuildTalentTree(PartyInventoryComponent, CharacterIndex, Character, *ClassDefinition, Candidate.TalentTree)) return false;
	if (!Candidate.IsValid()) return false;
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

FName FGridSkillsPageService::ResolveSkillSelection(const FGridSkillsPageView& PageView, FName PreferredSkillId)
{
	if (!PreferredSkillId.IsNone())
	{
		const bool bPreferredExists = PageView.Skills.ContainsByPredicate(
			[PreferredSkillId](const FGridSkillEntryView& Entry)
			{
				return Entry.SkillId == PreferredSkillId;
			});
		if (bPreferredExists)
		{
			return PreferredSkillId;
		}
	}

	return PageView.Skills.IsEmpty() ? NAME_None : PageView.Skills[0].SkillId;
}

bool FGridSkillsPageService::TryBuildSkillDetailView(
	UGridPartyInventoryComponent* PartyInventoryComponent,
	const FGridSkillsPageView& PageView,
	FName SkillId,
	const TArray<const URPGSkillAsset*>& SkillDefinitions,
	const URPGSkillPresentationAsset* PresentationCatalog,
	FGridSkillDetailView& OutView)
{
	OutView = FGridSkillDetailView();
	if (!IsValid(PartyInventoryComponent) || !PageView.IsValid() || SkillId.IsNone() ||
		!PartyInventoryComponent->IsValidCharacterIndex(PageView.CharacterIndex))
	{
		return false;
	}

	const FGridCharacterInventoryState& Character =
		PartyInventoryComponent->PartyInventoryState.ActiveCharacters[PageView.CharacterIndex];
	if (Character.CharacterId != PageView.CharacterId ||
		Character.Level != PageView.CharacterLevel ||
		Character.ClassId != PageView.ClassId ||
		!FRPGSkillService::ValidateSkillState(Character))
	{
		return false;
	}

	TArray<const URPGSkillAsset*> SortedDefinitions;
	if (!ValidateAndSortDefinitions(SkillDefinitions, SortedDefinitions))
	{
		return false;
	}

	const URPGSkillAsset* SkillDefinition = nullptr;
	for (const URPGSkillAsset* Definition : SortedDefinitions)
	{
		if (Definition && Definition->SkillId == SkillId)
		{
			SkillDefinition = Definition;
			break;
		}
	}
	if (!SkillDefinition)
	{
		return false;
	}

	const FGridSkillEntryView* SkillEntry = PageView.Skills.FindByPredicate(
		[SkillId](const FGridSkillEntryView& Entry)
		{
			return Entry.SkillId == SkillId;
		});
	if (!SkillEntry)
	{
		return false;
	}

	FRPGSkillPointBalance Balance;
	if (!FRPGSkillPointService::TryGetBalance(Character, Balance))
	{
		return false;
	}

	const int32 Rank = FRPGSkillService::GetSkillRank(Character, SkillId);
	const int32 CurrentRankCap = FMath::Min(SkillDefinition->MaxRank, Balance.RankCap);
	const bool bCanIncrease =
		FRPGSkillPointService::GetNextRankPurchaseAvailability(Character, SkillDefinition) ==
		ERPGSkillPointMutationRejectReason::None;

	if (PageView.RemainingSkillPoints != Balance.RemainingPoints ||
		PageView.SkillRankCap != Balance.RankCap ||
		!SkillEntry->DisplayName.EqualTo(SkillDefinition->DisplayName) ||
		SkillEntry->GoverningAttribute != SkillDefinition->GoverningAttribute ||
		SkillEntry->Rank != Rank ||
		SkillEntry->MaxRank != SkillDefinition->MaxRank ||
		SkillEntry->bAllowUntrainedChecks != SkillDefinition->bAllowUntrainedChecks ||
		SkillEntry->bTrained != (Rank > 0) ||
		SkillEntry->CurrentRankCap != CurrentRankCap ||
		SkillEntry->bCanIncreaseRank != bCanIncrease)
	{
		return false;
	}

	FGridSkillDetailView Candidate;
	Candidate.SkillId = SkillId;
	Candidate.DisplayName = SkillEntry->DisplayName;
	Candidate.Description = SkillDefinition->Description;
	Candidate.GoverningAttribute = SkillDefinition->GoverningAttribute;
	Candidate.GoverningAttributeValue =
		FRPGSkillCheckService::GetGoverningAttributeValue(Character.Attributes, SkillDefinition->GoverningAttribute);
	Candidate.GoverningAttributeModifier =
		SkillDefinition->GoverningAttribute == ERPGSkillGoverningAttribute::None
			? 0
			: URPGCharacterRulesLibrary::GetAttributeModifier(Candidate.GoverningAttributeValue);
	Candidate.BaseCheckBonus = Rank + Candidate.GoverningAttributeModifier;
	Candidate.Rank = Rank;
	Candidate.MaxRank = SkillDefinition->MaxRank;
	Candidate.CurrentRankCap = CurrentRankCap;
	Candidate.RemainingSkillPoints = Balance.RemainingPoints;
	Candidate.NextRank = Rank < SkillDefinition->MaxRank ? Rank + 1 : 0;
	Candidate.NextRankCost = Candidate.NextRank > 0 ? FRPGSkillPointService::GetRankPointCost() : 0;
	Candidate.bAllowUntrainedChecks = SkillDefinition->bAllowUntrainedChecks;
	Candidate.TrainingState = Rank > 0
		? EGridSkillTrainingState::Trained
		: (SkillDefinition->bAllowUntrainedChecks
			? EGridSkillTrainingState::UntrainedAllowed
			: EGridSkillTrainingState::TrainingRequired);
	Candidate.bCanIncreaseRank = bCanIncrease;
	Candidate.bCanDecreaseRank = SkillEntry->bCanDecreaseRank;
	Candidate.bAtCurrentRankCap = Rank >= CurrentRankCap;
	Candidate.bAtSkillMaxRank = Rank >= SkillDefinition->MaxRank;

	if (PresentationCatalog)
	{
		if (!IsValid(PresentationCatalog) || !PresentationCatalog->IsValidDefinition())
		{
			return false;
		}

		if (const FRPGSkillPresentationDefinition* Presentation = PresentationCatalog->FindSkill(SkillId))
		{
			Candidate.Icon = Presentation->Icon;
			Candidate.UsageDescription = Presentation->UsageDescription;
			Candidate.BuildAdvice = Presentation->BuildAdvice;

			auto ResolveClassAffinities =
				[](const TArray<FName>& ClassIds, TArray<FGridSkillClassAffinityView>& OutAffinities) -> bool
				{
					OutAffinities.Reset();
					for (const FName ClassId : ClassIds)
					{
						const URPGClassAsset* ClassDefinition = FRPGAuthoringIdentityResolver::ResolveClassById(ClassId);
						if (!IsValid(ClassDefinition) || ClassDefinition->DisplayName.IsEmpty())
						{
							OutAffinities.Reset();
							return false;
						}

						FGridSkillClassAffinityView Affinity;
						Affinity.ClassId = ClassId;
						Affinity.DisplayName = ClassDefinition->DisplayName;
						OutAffinities.Add(MoveTemp(Affinity));
					}
					return true;
				};

			if (!ResolveClassAffinities(Presentation->RecommendedClassIds, Candidate.RecommendedClasses) ||
				!ResolveClassAffinities(Presentation->UsefulClassIds, Candidate.UsefulClasses) ||
				!ResolveClassAffinities(Presentation->DiscouragedClassIds, Candidate.DiscouragedClasses))
			{
				return false;
			}

			for (const FName SynergySkillId : Presentation->SynergySkillIds)
			{
				const URPGSkillAsset* SynergyDefinition = nullptr;
				for (const URPGSkillAsset* Definition : SortedDefinitions)
				{
					if (Definition && Definition->SkillId == SynergySkillId)
					{
						SynergyDefinition = Definition;
						break;
					}
				}
				if (!IsValid(SynergyDefinition) || SynergyDefinition->DisplayName.IsEmpty())
				{
					return false;
				}

				FGridSkillSynergyView Synergy;
				Synergy.SkillId = SynergySkillId;
				Synergy.DisplayName = SynergyDefinition->DisplayName;
				Candidate.Synergies.Add(MoveTemp(Synergy));
			}
		}
	}

	if (!Candidate.IsValid())
	{
		return false;
	}

	OutView = MoveTemp(Candidate);
	return true;
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
