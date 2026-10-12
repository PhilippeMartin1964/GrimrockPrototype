#include "RPG/RPGClassProgressionAuthoring.h"

#include "RPG/RPGClassAsset.h"

namespace
{
	struct FAuthoredClassRecordText
	{
		FText DisplayName;
		FText Description;
	};
}

namespace RPGClassProgressionAuthoring
{
	constexpr int32 TalentGrantLevels[] = { 2, 4, 6, 8, 10, 12, 14, 16, 18, 20 };
}

void FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants(URPGClassAsset& ClassAsset)
{
	ClassAsset.ProgressionLevelGrants.Reset();
	for (const int32 Level : RPGClassProgressionAuthoring::TalentGrantLevels)
	{
		FRPGClassProgressionLevelGrant Grant;
		Grant.Level = Level;
		Grant.ChoicePointsGranted = 1;
		ClassAsset.ProgressionLevelGrants.Add(Grant);
	}
}

bool FRPGClassProgressionAuthoring::HasCanonicalTalentGrants(const URPGClassAsset& ClassAsset)
{
	if (ClassAsset.ProgressionLevelGrants.Num() != UE_ARRAY_COUNT(RPGClassProgressionAuthoring::TalentGrantLevels))
	{
		return false;
	}

	for (int32 Index = 0; Index < ClassAsset.ProgressionLevelGrants.Num(); ++Index)
	{
		const FRPGClassProgressionLevelGrant& Grant = ClassAsset.ProgressionLevelGrants[Index];
		if (Grant.Level != RPGClassProgressionAuthoring::TalentGrantLevels[Index] ||
			Grant.ChoicePointsGranted != 1 || !Grant.GrantedRequirementIds.IsEmpty())
		{
			return false;
		}
	}
	return true;
}

void FRPGClassProgressionAuthoring::RematerializePreservingChoiceAndActionText(
	URPGClassAsset& ClassAsset,
	TFunctionRef<void()> Rematerialize)
{
	TMap<FName, FAuthoredClassRecordText> ChoiceTextById;
	ChoiceTextById.Reserve(ClassAsset.ProgressionChoices.Num());
	for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset.ProgressionChoices)
	{
		if (!Choice.ChoiceId.IsNone())
		{
			ChoiceTextById.Add(
				Choice.ChoiceId,
				FAuthoredClassRecordText{ Choice.DisplayName, Choice.Description });
		}
	}

	TMap<FName, FAuthoredClassRecordText> ActionTextById;
	ActionTextById.Reserve(ClassAsset.CombatActions.Num());
	for (const FGridCombatActionDefinition& Action : ClassAsset.CombatActions)
	{
		if (!Action.ActionId.IsNone())
		{
			ActionTextById.Add(
				Action.ActionId,
				FAuthoredClassRecordText{ Action.DisplayName, Action.Description });
		}
	}

	Rematerialize();

	for (FRPGClassProgressionChoiceDefinition& Choice : ClassAsset.ProgressionChoices)
	{
		if (const FAuthoredClassRecordText* ExistingText = ChoiceTextById.Find(Choice.ChoiceId))
		{
			Choice.DisplayName = ExistingText->DisplayName;
			Choice.Description = ExistingText->Description;
		}
	}

	for (FGridCombatActionDefinition& Action : ClassAsset.CombatActions)
	{
		if (const FAuthoredClassRecordText* ExistingText = ActionTextById.Find(Action.ActionId))
		{
			Action.DisplayName = ExistingText->DisplayName;
			Action.Description = ExistingText->Description;
		}
	}
}
