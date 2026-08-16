#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

const FGridStatusEffectRuntimeState* FGridStatusEffectCollection::FindByEffectId (FName EffectId) const
{
    if (EffectId.IsNone ())
    {
        return nullptr;
    }

    return ActiveEffects.FindByPredicate (
        [EffectId] (const FGridStatusEffectRuntimeState& State)
        {
            return State.EffectId == EffectId;
        });
}

bool FGridStatusEffectCollection::TryAdd (
    const UGridStatusEffectDefinitionAsset& Definition,
    const FGuid& SourceId,
    int32 InitialStackCount,
    int32 DurationOverride,
    FString& OutError)
{
    FGridStatusEffectRuntimeState Candidate;
    if (!Definition.BuildRuntimeState (SourceId, InitialStackCount, DurationOverride, Candidate, OutError))
    {
        return false;
    }

    if (Contains (Candidate.EffectId))
    {
        OutError = FString::Printf (
            TEXT ("EffectId '%s' is already active. Use TryApply for MON16.2 reapplication rules."),
            *Candidate.EffectId.ToString ());
        return false;
    }

    ActiveEffects.Add (Candidate);
    SortDeterministically ();
    OutError.Reset ();
    return true;
}

bool FGridStatusEffectCollection::TryApply (
    const UGridStatusEffectDefinitionAsset& Definition,
    const FGuid& SourceId,
    int32 InitialStackCount,
    int32 DurationOverride,
    int32 PotencyOverride,
    FGridStatusEffectApplyResult& OutResult,
    FString& OutError)
{
    OutResult.Reset ();

    FGridStatusEffectRuntimeState Candidate;
    if (!Definition.BuildRuntimeState (
            SourceId,
            InitialStackCount,
            DurationOverride,
            PotencyOverride,
            Candidate,
            OutError))
    {
        return false;
    }

    const int32 ExistingIndex = ActiveEffects.IndexOfByPredicate (
        [&Candidate] (const FGridStatusEffectRuntimeState& State)
        {
            return State.EffectId == Candidate.EffectId;
        });

    OutResult.EffectId = Candidate.EffectId;
    if (ExistingIndex == INDEX_NONE)
    {
        ActiveEffects.Add (Candidate);
        SortDeterministically ();
        OutResult.Outcome = EGridStatusEffectApplyOutcome::Added;
        OutResult.CurrentStackCount = Candidate.StackCount;
        OutResult.CurrentRemainingDuration = Candidate.RemainingDuration;
        OutResult.CurrentPotency = Candidate.Potency;
        OutError.Reset ();
        return true;
    }

    const FGridStatusEffectRuntimeState ExistingSnapshot = ActiveEffects[ExistingIndex];
    OutResult.PreviousStackCount = ExistingSnapshot.StackCount;
    OutResult.CurrentStackCount = ExistingSnapshot.StackCount;
    OutResult.PreviousRemainingDuration = ExistingSnapshot.RemainingDuration;
    OutResult.CurrentRemainingDuration = ExistingSnapshot.RemainingDuration;
    OutResult.PreviousPotency = ExistingSnapshot.Potency;
    OutResult.CurrentPotency = ExistingSnapshot.Potency;

    if (ExistingSnapshot.DurationUnit != Candidate.DurationUnit)
    {
        OutError = FString::Printf (
            TEXT ("EffectId '%s' cannot be reapplied with a different DurationUnit."),
            *Candidate.EffectId.ToString ());
        return false;
    }

    FGridStatusEffectRuntimeState Updated = ExistingSnapshot;
    switch (Definition.StackPolicy)
    {
    case EGridStatusEffectStackPolicy::NoStack:
        OutResult.Outcome = EGridStatusEffectApplyOutcome::RejectedNoStack;
        OutError = FString::Printf (
            TEXT ("EffectId '%s' rejects reapplication because StackPolicy is NoStack."),
            *Candidate.EffectId.ToString ());
        return false;

    case EGridStatusEffectStackPolicy::RefreshDuration:
        Updated = Candidate;
        OutResult.Outcome = EGridStatusEffectApplyOutcome::RefreshedDuration;
        break;

    case EGridStatusEffectStackPolicy::AddStacks:
        Updated.SourceId = Candidate.SourceId;
        Updated.StackCount = FMath::Min (
            Definition.MaxStacks,
            ExistingSnapshot.StackCount + Candidate.StackCount);
        Updated.RemainingDuration = Candidate.RemainingDuration;
        Updated.Potency = FMath::Max (ExistingSnapshot.Potency, Candidate.Potency);
        OutResult.Outcome = EGridStatusEffectApplyOutcome::AddedStacks;
        break;

    case EGridStatusEffectStackPolicy::ReplaceIfStronger:
        if (Candidate.Potency <= ExistingSnapshot.Potency)
        {
            OutResult.Outcome = EGridStatusEffectApplyOutcome::RejectedNotStronger;
            OutError = FString::Printf (
                TEXT ("EffectId '%s' potency %d is not stronger than active potency %d."),
                *Candidate.EffectId.ToString (),
                Candidate.Potency,
                ExistingSnapshot.Potency);
            return false;
        }
        Updated = Candidate;
        OutResult.Outcome = EGridStatusEffectApplyOutcome::ReplacedStronger;
        break;

    default:
        OutError = TEXT ("Unsupported status effect stack policy.");
        return false;
    }

    if (!Updated.IsValid ())
    {
        OutError = TEXT ("Status effect reapplication produced an invalid runtime state.");
        return false;
    }

    ActiveEffects[ExistingIndex] = Updated;
    SortDeterministically ();
    OutResult.CurrentStackCount = Updated.StackCount;
    OutResult.CurrentRemainingDuration = Updated.RemainingDuration;
    OutResult.CurrentPotency = Updated.Potency;
    OutError.Reset ();
    return true;
}

void FGridStatusEffectCollection::AdvanceDuration (
    EGridStatusEffectDurationUnit DurationUnit,
    FGridStatusEffectAdvanceResult& OutResult)
{
    OutResult.Reset (DurationUnit);
    if (DurationUnit == EGridStatusEffectDurationUnit::Permanent || ActiveEffects.IsEmpty ())
    {
        return;
    }

    TArray<FGridStatusEffectRuntimeState> Survivors;
    Survivors.Reserve (ActiveEffects.Num ());
    for (const FGridStatusEffectRuntimeState& Existing : ActiveEffects)
    {
        if (Existing.DurationUnit != DurationUnit)
        {
            Survivors.Add (Existing);
            continue;
        }

        FGridStatusEffectRuntimeState Advanced = Existing;
        --Advanced.RemainingDuration;
        OutResult.AdvancedEffectIds.Add (Advanced.EffectId);
        if (Advanced.RemainingDuration <= 0)
        {
            OutResult.ExpiredEffectIds.Add (Advanced.EffectId);
            continue;
        }
        Survivors.Add (Advanced);
    }

    ActiveEffects = MoveTemp (Survivors);
}

void FGridStatusEffectCollection::SortDeterministically ()
{
    ActiveEffects.Sort (
        [] (const FGridStatusEffectRuntimeState& Left, const FGridStatusEffectRuntimeState& Right)
        {
            return Left.EffectId.ToString ().Compare (
                Right.EffectId.ToString (),
                ESearchCase::CaseSensitive) < 0;
        });
}
