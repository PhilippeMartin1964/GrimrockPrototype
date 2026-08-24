#include "RPG/RPGSkillService.h"

#include "RPG/RPGSkillAsset.h"
#include "Runtime/GridInventoryTypes.h"

bool FRPGSkillService::ValidateSkillState (
    const FGridCharacterInventoryState& CharacterState)
{
    TSet<FName> SeenSkillIds;

    for (const FRPGSkillRank& SkillRank : CharacterState.SkillRanks)
    {
        if (!SkillRank.IsValid () || SeenSkillIds.Contains (SkillRank.SkillId))
        {
            return false;
        }

        SeenSkillIds.Add (SkillRank.SkillId);
    }

    return true;
}

int32 FRPGSkillService::GetSkillRank (
    const FGridCharacterInventoryState& CharacterState,
    FName SkillId)
{
    if (SkillId.IsNone () || !ValidateSkillState (CharacterState))
    {
        return 0;
    }

    const FRPGSkillRank* SkillRank = CharacterState.SkillRanks.FindByPredicate (
        [SkillId] (const FRPGSkillRank& Entry)
        {
            return Entry.SkillId == SkillId;
        });

    return SkillRank ? SkillRank->Rank : 0;
}

bool FRPGSkillService::TrySetSkillRank (
    FGridCharacterInventoryState& CharacterState,
    const URPGSkillAsset* SkillDefinition,
    int32 NewRank,
    FRPGSkillMutationResult& OutResult)
{
    OutResult = FRPGSkillMutationResult ();

    if (!IsValid (SkillDefinition) || !SkillDefinition->IsValidDefinition ())
    {
        OutResult.RejectReason = ERPGSkillMutationRejectReason::InvalidDefinition;
        return false;
    }

    if (!ValidateSkillState (CharacterState))
    {
        OutResult.RejectReason = ERPGSkillMutationRejectReason::InvalidCurrentState;
        return false;
    }

    if (NewRank < 0 || NewRank > SkillDefinition->MaxRank)
    {
        OutResult.RejectReason = ERPGSkillMutationRejectReason::RankOutOfRange;
        return false;
    }

    const FName SkillId = SkillDefinition->SkillId;
    const int32 ExistingIndex = CharacterState.SkillRanks.IndexOfByPredicate (
        [SkillId] (const FRPGSkillRank& Entry)
        {
            return Entry.SkillId == SkillId;
        });

    const int32 PreviousRank = ExistingIndex != INDEX_NONE
        ? CharacterState.SkillRanks[ExistingIndex].Rank
        : 0;

    OutResult.PreviousRank = PreviousRank;
    OutResult.NewRank = NewRank;

    if (PreviousRank == NewRank)
    {
        return true;
    }

    if (NewRank == 0)
    {
        if (ExistingIndex != INDEX_NONE)
        {
            CharacterState.SkillRanks.RemoveAt (ExistingIndex);
            OutResult.bChanged = true;
        }
        return true;
    }

    if (ExistingIndex != INDEX_NONE)
    {
        CharacterState.SkillRanks[ExistingIndex].Rank = NewRank;
    }
    else
    {
        FRPGSkillRank NewEntry;
        NewEntry.SkillId = SkillId;
        NewEntry.Rank = NewRank;
        CharacterState.SkillRanks.Add (NewEntry);
    }

    OutResult.bChanged = true;
    return true;
}

bool FRPGSkillService::TryIncreaseSkillRank (
    FGridCharacterInventoryState& CharacterState,
    const URPGSkillAsset* SkillDefinition,
    int32 Delta,
    FRPGSkillMutationResult& OutResult)
{
    OutResult = FRPGSkillMutationResult ();

    if (!IsValid (SkillDefinition) || !SkillDefinition->IsValidDefinition ())
    {
        OutResult.RejectReason = ERPGSkillMutationRejectReason::InvalidDefinition;
        return false;
    }

    if (!ValidateSkillState (CharacterState))
    {
        OutResult.RejectReason = ERPGSkillMutationRejectReason::InvalidCurrentState;
        return false;
    }

    if (Delta <= 0)
    {
        OutResult.RejectReason = ERPGSkillMutationRejectReason::InvalidDelta;
        return false;
    }

    const int32 CurrentRank = GetSkillRank (
        CharacterState,
        SkillDefinition->SkillId);

    if (Delta > SkillDefinition->MaxRank - CurrentRank)
    {
        OutResult.PreviousRank = CurrentRank;
        OutResult.NewRank = CurrentRank;
        OutResult.RejectReason = ERPGSkillMutationRejectReason::RankOutOfRange;
        return false;
    }

    return TrySetSkillRank (
        CharacterState,
        SkillDefinition,
        CurrentRank + Delta,
        OutResult);
}
