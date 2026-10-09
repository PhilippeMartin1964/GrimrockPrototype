#include "RPG/RPGClassProgressionService.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"

namespace
{
	bool IsMON154LevelValid(int32 CharacterLevel)
	{
		return CharacterLevel >= URPGCharacterRulesLibrary::GetMinimumLevel() && CharacterLevel <= URPGCharacterRulesLibrary::GetMaximumLevel();
	}

	bool IsMON154ClassValid(const URPGClassAsset* ClassDefinition)
	{
		return IsValid(ClassDefinition) && ClassDefinition->IsValidDefinition();
	}

	void AddAutomaticRequirements(const URPGClassAsset& ClassDefinition, int32 CharacterLevel, TSet<FName>& InOutRequirements)
	{
		InOutRequirements.Add(ClassDefinition.ClassId);
		for (const FRPGClassProgressionLevelGrant& Grant : ClassDefinition.ProgressionLevelGrants)
		{
			if (Grant.Level <= CharacterLevel)
			{
				for (const FName RequirementId : Grant.GrantedRequirementIds)
				{
					InOutRequirements.Add(RequirementId);
				}
			}
		}
	}

	bool HasExclusiveGroupConflict(const URPGClassAsset& ClassDefinition, const TSet<FName>& SelectedChoiceIds,
		const FRPGClassProgressionChoiceDefinition* Candidate = nullptr)
	{
		TSet<FName> SelectedGroups;
		for (const FName ChoiceId : SelectedChoiceIds)
		{
			const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition.FindProgressionChoice(ChoiceId);
			if (!Choice || Choice->ExclusiveChoiceGroupId.IsNone())
			{
				continue;
			}
			if (SelectedGroups.Contains(Choice->ExclusiveChoiceGroupId))
			{
				return true;
			}
			SelectedGroups.Add(Choice->ExclusiveChoiceGroupId);
		}
		return Candidate && !Candidate->ExclusiveChoiceGroupId.IsNone() && SelectedGroups.Contains(Candidate->ExclusiveChoiceGroupId);
	}

	bool ResolveSelectionRequirements(const URPGClassAsset& ClassDefinition, int32 CharacterLevel,
		const TSet<FName>& SelectedChoiceIds, TSet<FName>& OutRequirements)
	{
		OutRequirements.Reset();
		AddAutomaticRequirements(ClassDefinition, CharacterLevel, OutRequirements);

		TSet<FName> Pending = SelectedChoiceIds;
		bool bProgress = true;
		while (!Pending.IsEmpty() && bProgress)
		{
			bProgress = false;
			TArray<FName> ResolvedThisPass;
			for (const FName ChoiceId : Pending)
			{
				const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition.FindProgressionChoice(ChoiceId);
				if (!Choice)
				{
					return false;
				}
				bool bRequirementsSatisfied = true;
				for (const FName RequirementId : Choice->PrerequisiteRequirementIds)
				{
					if (!OutRequirements.Contains(RequirementId))
					{
						bRequirementsSatisfied = false;
						break;
					}
				}
				if (!bRequirementsSatisfied)
				{
					continue;
				}

				OutRequirements.Add(Choice->ChoiceId);
				for (const FName RequirementId : Choice->GrantedRequirementIds)
				{
					OutRequirements.Add(RequirementId);
				}
				ResolvedThisPass.Add(ChoiceId);
				bProgress = true;
			}
			for (const FName ChoiceId : ResolvedThisPass)
			{
				Pending.Remove(ChoiceId);
			}
		}
		return Pending.IsEmpty();
	}
}

int32 FRPGClassProgressionService::GetTotalChoicePointsGranted(const URPGClassAsset* ClassDefinition, int32 CharacterLevel)
{
	if (!IsMON154ClassValid(ClassDefinition) || !IsMON154LevelValid(CharacterLevel))
	{
		return 0;
	}

	int32 TotalPoints = 0;
	for (const FRPGClassProgressionLevelGrant& Grant : ClassDefinition->ProgressionLevelGrants)
	{
		if (Grant.Level <= CharacterLevel)
		{
			TotalPoints += Grant.ChoicePointsGranted;
		}
	}
	return FMath::Max(0, TotalPoints);
}

bool FRPGClassProgressionService::TryGetChoicePointBalance(const URPGClassAsset* ClassDefinition, int32 CharacterLevel, const TSet<FName>& SelectedChoiceIds,
	int32& OutGrantedPoints, int32& OutSpentPoints, int32& OutRemainingPoints)
{
	OutGrantedPoints = 0;
	OutSpentPoints = 0;
	OutRemainingPoints = 0;

	if (!IsMON154ClassValid(ClassDefinition) || !IsMON154LevelValid(CharacterLevel))
	{
		return false;
	}

	const int32 GrantedPoints = GetTotalChoicePointsGranted(ClassDefinition, CharacterLevel);
	int32 SpentPoints = 0;
	if (HasExclusiveGroupConflict(*ClassDefinition, SelectedChoiceIds))
	{
		return false;
	}

	for (const FName ChoiceId : SelectedChoiceIds)
	{
		const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition->FindProgressionChoice(ChoiceId);
		if (!Choice || Choice->MinimumLevel > CharacterLevel)
		{
			return false;
		}
		for (const FName PrerequisiteId : Choice->PrerequisiteChoiceIds)
		{
			if (!SelectedChoiceIds.Contains(PrerequisiteId))
			{
				return false;
			}
		}
		SpentPoints += Choice->PointCost;
		if (SpentPoints > GrantedPoints)
		{
			return false;
		}
	}

	TSet<FName> ResolvedRequirements;
	if (!ResolveSelectionRequirements(*ClassDefinition, CharacterLevel, SelectedChoiceIds, ResolvedRequirements))
	{
		return false;
	}

	OutGrantedPoints = GrantedPoints;
	OutSpentPoints = SpentPoints;
	OutRemainingPoints = GrantedPoints - SpentPoints;
	return true;
}

ERPGClassProgressionChoiceAvailabilityReason FRPGClassProgressionService::GetChoiceAvailability(
	const URPGClassAsset* ClassDefinition, int32 CharacterLevel, const TSet<FName>& SelectedChoiceIds, FName ChoiceId)
{
	if (!IsMON154ClassValid(ClassDefinition))
	{
		return ERPGClassProgressionChoiceAvailabilityReason::InvalidClassDefinition;
	}
	if (!IsMON154LevelValid(CharacterLevel))
	{
		return ERPGClassProgressionChoiceAvailabilityReason::InvalidLevel;
	}

	int32 GrantedPoints = 0;
	int32 SpentPoints = 0;
	int32 RemainingPoints = 0;
	if (!TryGetChoicePointBalance(ClassDefinition, CharacterLevel, SelectedChoiceIds, GrantedPoints, SpentPoints, RemainingPoints))
	{
		return ERPGClassProgressionChoiceAvailabilityReason::InvalidSelectionState;
	}

	const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition->FindProgressionChoice(ChoiceId);
	if (!Choice)
	{
		return ERPGClassProgressionChoiceAvailabilityReason::UnknownChoice;
	}
	if (SelectedChoiceIds.Contains(ChoiceId))
	{
		return ERPGClassProgressionChoiceAvailabilityReason::AlreadySelected;
	}
	if (HasExclusiveGroupConflict(*ClassDefinition, SelectedChoiceIds, Choice))
	{
		return ERPGClassProgressionChoiceAvailabilityReason::MutuallyExclusiveChoice;
	}
	if (CharacterLevel < Choice->MinimumLevel)
	{
		return ERPGClassProgressionChoiceAvailabilityReason::LevelTooLow;
	}
	for (const FName PrerequisiteId : Choice->PrerequisiteChoiceIds)
	{
		if (!SelectedChoiceIds.Contains(PrerequisiteId))
		{
			return ERPGClassProgressionChoiceAvailabilityReason::MissingPrerequisite;
		}
	}
	TSet<FName> ResolvedRequirements;
	if (!ResolveSelectionRequirements(*ClassDefinition, CharacterLevel, SelectedChoiceIds, ResolvedRequirements))
	{
		return ERPGClassProgressionChoiceAvailabilityReason::InvalidSelectionState;
	}
	for (const FName RequirementId : Choice->PrerequisiteRequirementIds)
	{
		if (!ResolvedRequirements.Contains(RequirementId))
		{
			return ERPGClassProgressionChoiceAvailabilityReason::MissingPrerequisite;
		}
	}
	if (RemainingPoints < Choice->PointCost)
	{
		return ERPGClassProgressionChoiceAvailabilityReason::InsufficientChoicePoints;
	}
	return ERPGClassProgressionChoiceAvailabilityReason::None;
}

bool FRPGClassProgressionService::CollectSatisfiedRequirements(
	const URPGClassAsset* ClassDefinition, int32 CharacterLevel, const TSet<FName>& SelectedChoiceIds, TSet<FName>& OutSatisfiedRequirements)
{
	OutSatisfiedRequirements.Reset();

	int32 GrantedPoints = 0;
	int32 SpentPoints = 0;
	int32 RemainingPoints = 0;
	if (!TryGetChoicePointBalance(ClassDefinition, CharacterLevel, SelectedChoiceIds, GrantedPoints, SpentPoints, RemainingPoints))
	{
		return false;
	}

	return ResolveSelectionRequirements(*ClassDefinition, CharacterLevel, SelectedChoiceIds, OutSatisfiedRequirements);
}

