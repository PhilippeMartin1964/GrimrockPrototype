#include "RPG/RPGPartyProgressionResolver.h"

#include "RPG/RPGClassAsset.h"
#include "Runtime/GridInventoryTypes.h"

namespace
{
	void CollectLivingSelectedChoices(const FGridPartyInventoryState& PartyState,
		TFunctionRef<void(const FRPGClassProgressionChoiceDefinition&)> Visitor)
	{
		for (const FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
		{
			if (Character.Resources.CurrentHealth <= 0)
			{
				continue;
			}
			const URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
			if (!IsValid(ClassDefinition) || !ClassDefinition->IsValidDefinition())
			{
				continue;
			}
			for (const FName ChoiceId : Character.SelectedClassProgressionChoiceIds)
			{
				if (const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition->FindProgressionChoice(ChoiceId))
				{
					Visitor(*Choice);
				}
			}
		}
	}

	int32 ResolveGroupedPartyModifier(const FGridPartyInventoryState& PartyState,
		TFunctionRef<bool(const FRPGPartyProgressionModifier&)> Filter,
		TFunctionRef<int32(const FRPGPartyProgressionModifier&)> Value)
	{
		int32 Ungrouped = 0;
		TMap<FName, int32> Grouped;
		CollectLivingSelectedChoices(PartyState,
			[&](const FRPGClassProgressionChoiceDefinition& Choice)
			{
				for (const FRPGPartyProgressionModifier& Modifier : Choice.PartyModifiers)
				{
					if (!Modifier.IsValid() || !Filter(Modifier))
					{
						continue;
					}
					const int32 Candidate = Value(Modifier);
					if (Modifier.StackingGroupId.IsNone())
					{
						Ungrouped += Candidate;
					}
					else
					{
						int32& Existing = Grouped.FindOrAdd(Modifier.StackingGroupId);
						Existing = Candidate >= 0 ? FMath::Max(Existing, Candidate) : FMath::Min(Existing, Candidate);
					}
				}
			});

		int32 Result = Ungrouped;
		for (const TPair<FName, int32>& Pair : Grouped)
		{
			Result += Pair.Value;
		}
		return Result;
	}
}

int32 FRPGPartyProgressionResolver::ResolveGroupSkillCheckModifier(const FGridPartyInventoryState& PartyState, FName SkillId)
{
	if (SkillId.IsNone())
	{
		return 0;
	}
	return ResolveGroupedPartyModifier(
		PartyState,
		[SkillId](const FRPGPartyProgressionModifier& Modifier)
		{
			return Modifier.GroupSkillCheckModifier != 0 && Modifier.GroupSkillIds.Contains(SkillId);
		},
		[](const FRPGPartyProgressionModifier& Modifier)
		{
			return Modifier.GroupSkillCheckModifier;
		});
}

int32 FRPGPartyProgressionResolver::ResolveMaximumMobilityActionPointsModifier(const FGridPartyInventoryState& PartyState)
{
	return ResolveGroupedPartyModifier(
		PartyState,
		[](const FRPGPartyProgressionModifier& Modifier)
		{
			return Modifier.MaximumMobilityActionPointsModifier != 0;
		},
		[](const FRPGPartyProgressionModifier& Modifier)
		{
			return Modifier.MaximumMobilityActionPointsModifier;
		});
}

int32 FRPGPartyProgressionResolver::ResolveFirstRoundInitiativeModifier(const FGridCharacterInventoryState& CharacterState)
{
	if (CharacterState.Resources.CurrentHealth <= 0)
	{
		return 0;
	}
	const URPGClassAsset* ClassDefinition = CharacterState.ClassDefinition.Get();
	if (!IsValid(ClassDefinition) || !ClassDefinition->IsValidDefinition())
	{
		return 0;
	}
	int32 Result = 0;
	for (const FName ChoiceId : CharacterState.SelectedClassProgressionChoiceIds)
	{
		if (const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition->FindProgressionChoice(ChoiceId))
		{
			Result += Choice->FirstRoundInitiativeModifier;
		}
	}
	return Result;
}
