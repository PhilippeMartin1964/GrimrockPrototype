#include "Runtime/Combat/GridCombatTargetingResolver.h"

#include "RPG/StatusEffects/GridStatusEffectControlResolver.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

bool FGridCombatTargetingResolver::IsDirectHostileTargetable(const FGridStatusEffectCollection& StatusEffects)
{
	return !FGridStatusEffectControlResolver::Resolve(StatusEffects).bBlockDirectHostileTargeting;
}

bool FGridCombatTargetingResolver::MatchesTargetFilter(const FGridCombatTargetFilterProfile& Filter, FName MonsterCategoryId,
	const FGridStatusEffectCollection& StatusEffects, const FGuid& ActingSourceId,
	const FGridAttackTargetStats* TargetStats, int32 MaximumHealth)
{
	if (!Filter.IsValid())
	{
		return false;
	}
	if (!Filter.AllowedMonsterCategoryIds.IsEmpty() && !Filter.AllowedMonsterCategoryIds.Contains(MonsterCategoryId))
	{
		return false;
	}

	for (const FName RequiredId : Filter.RequiredStatusEffectIds)
	{
		const bool bFound = StatusEffects.ActiveEffects.ContainsByPredicate(
			[RequiredId, &Filter, &ActingSourceId](const FGridStatusEffectRuntimeState& State)
			{
				return State.EffectId == RequiredId &&
					(!Filter.bRequiredStatusesFromSource || State.SourceId == ActingSourceId);
			});
		if (!bFound)
		{
			return false;
		}
	}

	for (const FName RequiredTag : Filter.RequiredStatusTags)
	{
		const bool bFound = StatusEffects.ActiveEffects.ContainsByPredicate(
			[RequiredTag](const FGridStatusEffectRuntimeState& State)
			{
				return State.IsValid() && IsValid(State.DefinitionAsset) && State.DefinitionAsset->StatusTags.Contains(RequiredTag);
			});
		if (!bFound)
		{
			return false;
		}
	}

	const bool bNeedsVitals =
		Filter.bRequirePhysicalArmorDepleted || Filter.bRequireMagicalArmorDepleted || Filter.MaximumHealthPercent > 0;
	if (!bNeedsVitals)
	{
		return true;
	}
	if (!TargetStats)
	{
		return false;
	}
	if (Filter.bRequirePhysicalArmorDepleted && TargetStats->PhysicalArmor > 0)
	{
		return false;
	}
	if (Filter.bRequireMagicalArmorDepleted && TargetStats->MagicalArmor > 0)
	{
		return false;
	}
	if (Filter.MaximumHealthPercent > 0)
	{
		if (MaximumHealth <= 0 ||
			static_cast<int64>(FMath::Max(0, TargetStats->CurrentHealth)) * 100 >
				static_cast<int64>(MaximumHealth) * Filter.MaximumHealthPercent)
		{
			return false;
		}
	}
	return true;
}

bool FGridCombatTargetingResolver::MatchesStatusRemoval(
	const FGridStatusEffectRuntimeState& State, const FGridCombatStatusRemovalProfile& Profile)
{
	if (!State.IsValid() || !IsValid(State.DefinitionAsset) || !Profile.IsValid())
	{
		return false;
	}
	if (!Profile.AllowedDispositions.IsEmpty() && !Profile.AllowedDispositions.Contains(State.DefinitionAsset->Disposition))
	{
		return false;
	}

	const bool bHasIdentityFilter = !Profile.EffectIds.IsEmpty();
	const bool bHasTagFilter = !Profile.AnyStatusTags.IsEmpty();
	if (!bHasIdentityFilter && !bHasTagFilter)
	{
		return true;
	}

	if (bHasIdentityFilter && Profile.EffectIds.Contains(State.EffectId))
	{
		return true;
	}
	if (bHasTagFilter)
	{
		for (const FName Tag : Profile.AnyStatusTags)
		{
			if (State.DefinitionAsset->StatusTags.Contains(Tag))
			{
				return true;
			}
		}
	}
	return false;
}

void FGridCombatTargetingResolver::CollectStatusRemovalIds(const FGridStatusEffectCollection& StatusEffects,
	const TArray<FGridCombatStatusRemovalProfile>& Profiles, TArray<FName>& OutEffectIds)
{
	OutEffectIds.Reset();
	for (const FGridCombatStatusRemovalProfile& Profile : Profiles)
	{
		if (!Profile.IsValid())
		{
			continue;
		}

		TArray<FName> MatchingIds;
		for (const FGridStatusEffectRuntimeState& State : StatusEffects.ActiveEffects)
		{
			if (!OutEffectIds.Contains(State.EffectId) && MatchesStatusRemoval(State, Profile))
			{
				MatchingIds.AddUnique(State.EffectId);
			}
		}
		MatchingIds.Sort(
			[](const FName Left, const FName Right)
			{
				return Left.ToString().Compare(Right.ToString(), ESearchCase::CaseSensitive) < 0;
			});

		const int32 Count = FMath::Min(Profile.MaximumRemovals, MatchingIds.Num());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			OutEffectIds.Add(MatchingIds[Index]);
		}
	}
}

void FGridCombatTargetingResolver::CollectPartyTargets(const FGridPartyInventoryState& PartyState, EGridCombatTargetingPolicy Policy,
	int32 SourceCharacterIndex, int32 ExplicitTargetCharacterIndex, int32 FrontLineSlotCount, TArray<int32>& OutCharacterIndices)
{
	TArray<int32> ExplicitTargets;
	if (ExplicitTargetCharacterIndex != INDEX_NONE)
	{
		ExplicitTargets.Add(ExplicitTargetCharacterIndex);
	}
	CollectPartyTargets(PartyState, Policy, SourceCharacterIndex, ExplicitTargets, FrontLineSlotCount, OutCharacterIndices);
}

void FGridCombatTargetingResolver::CollectPartyTargets(const FGridPartyInventoryState& PartyState, EGridCombatTargetingPolicy Policy,
	int32 SourceCharacterIndex, const TArray<int32>& ExplicitTargetCharacterIndices, int32 FrontLineSlotCount, TArray<int32>& OutCharacterIndices)
{
	OutCharacterIndices.Reset();
	auto AddIfLiving = [&PartyState, &OutCharacterIndices](int32 CharacterIndex)
	{
		if (PartyState.ActiveCharacters.IsValidIndex(CharacterIndex) &&
			PartyState.ActiveCharacters[CharacterIndex].Resources.CurrentHealth > 0)
		{
			OutCharacterIndices.AddUnique(CharacterIndex);
		}
	};

	switch (Policy)
	{
		case EGridCombatTargetingPolicy::Self:
			AddIfLiving(SourceCharacterIndex);
			break;
		case EGridCombatTargetingPolicy::Ally:
			for (const int32 CharacterIndex : ExplicitTargetCharacterIndices)
			{
				AddIfLiving(CharacterIndex);
			}
			break;
		case EGridCombatTargetingPolicy::Party:
			for (int32 CharacterIndex = 0; CharacterIndex < PartyState.ActiveCharacters.Num(); ++CharacterIndex)
			{
				AddIfLiving(CharacterIndex);
			}
			break;
		case EGridCombatTargetingPolicy::FrontRowParty:
		{
			const int32 Limit = FMath::Min(FMath::Max(0, FrontLineSlotCount), PartyState.ActiveCharacters.Num());
			for (int32 CharacterIndex = 0; CharacterIndex < Limit; ++CharacterIndex)
			{
				AddIfLiving(CharacterIndex);
			}
			break;
		}
		default:
			break;
	}
}

bool FGridCombatTargetingResolver::ShouldApplyStatusApplication(
	const FGridCombatStatusApplicationProfile& Profile, int32 TargetOrdinal)
{
	return Profile.TargetScope == EGridCombatResolvedTargetScope::AllResolvedTargets || TargetOrdinal == 0;
}


void FGridCombatTargetingResolver::BuildDeterministicChain(const FGuid& PrimaryTargetId, const FIntPoint& PrimaryCell,
	const TArray<FGridCombatChainTargetCandidate>& Candidates, int32 JumpRangeCells, int32 MaximumTargets,
	TArray<FGridCombatChainTargetCandidate>& OutTargets)
{
	OutTargets.Reset();
	if (!PrimaryTargetId.IsValid() || JumpRangeCells <= 0 || MaximumTargets <= 0)
	{
		return;
	}

	const FGridCombatChainTargetCandidate* Primary = Candidates.FindByPredicate(
		[&PrimaryTargetId](const FGridCombatChainTargetCandidate& Candidate)
		{
			return Candidate.TargetId == PrimaryTargetId && Candidate.IsValid();
		});
	if (!Primary)
	{
		return;
	}

	FGridCombatChainTargetCandidate PrimaryCopy = *Primary;
	PrimaryCopy.Cell = PrimaryCell;
	OutTargets.Add(PrimaryCopy);

	TSet<FGuid> SelectedIds;
	SelectedIds.Add(PrimaryTargetId);
	FIntPoint PreviousCell = PrimaryCell;

	while (OutTargets.Num() < MaximumTargets)
	{
		const FGridCombatChainTargetCandidate* Best = nullptr;
		int32 BestDistance = MAX_int32;
		for (const FGridCombatChainTargetCandidate& Candidate : Candidates)
		{
			if (!Candidate.IsValid() || SelectedIds.Contains(Candidate.TargetId))
			{
				continue;
			}
			const int32 Distance =
				FMath::Abs(Candidate.Cell.X - PreviousCell.X) + FMath::Abs(Candidate.Cell.Y - PreviousCell.Y);
			if (Distance <= 0 || Distance > JumpRangeCells)
			{
				continue;
			}

			const bool bBetter = !Best || Distance < BestDistance ||
				(Distance == BestDistance && (Candidate.Cell.Y < Best->Cell.Y ||
					(Candidate.Cell.Y == Best->Cell.Y && (Candidate.Cell.X < Best->Cell.X ||
						(Candidate.Cell.X == Best->Cell.X &&
							Candidate.TargetId.ToString(EGuidFormats::Digits) < Best->TargetId.ToString(EGuidFormats::Digits))))));
			if (bBetter)
			{
				Best = &Candidate;
				BestDistance = Distance;
			}
		}

		if (!Best)
		{
			break;
		}
		OutTargets.Add(*Best);
		SelectedIds.Add(Best->TargetId);
		PreviousCell = Best->Cell;
	}
}
