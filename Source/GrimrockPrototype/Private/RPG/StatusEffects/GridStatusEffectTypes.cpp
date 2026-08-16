#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

const FGridStatusEffectRuntimeState* FGridStatusEffectCollection::FindByEffectId (
    FName EffectId) const
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
    if (!Definition.BuildRuntimeState (
            SourceId,
            InitialStackCount,
            DurationOverride,
            Candidate,
            OutError))
    {
        return false;
    }

    if (Contains (Candidate.EffectId))
    {
        OutError = FString::Printf (
            TEXT ("EffectId '%s' is already active. Stacking resolution begins in MON16.2."),
            *Candidate.EffectId.ToString ());
        return false;
    }

    ActiveEffects.Add (Candidate);
    SortDeterministically ();
    OutError.Reset ();
    return true;
}

void FGridStatusEffectCollection::SortDeterministically ()
{
    ActiveEffects.Sort (
        [] (
            const FGridStatusEffectRuntimeState& Left,
            const FGridStatusEffectRuntimeState& Right)
        {
            return Left.EffectId.ToString ().Compare (
                Right.EffectId.ToString (),
                ESearchCase::CaseSensitive) < 0;
        });
}
