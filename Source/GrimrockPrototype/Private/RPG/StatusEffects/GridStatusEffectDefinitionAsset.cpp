#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

FPrimaryAssetId UGridStatusEffectDefinitionAsset::GetPrimaryAssetId () const
{
    if (EffectId.IsNone ())
    {
        return Super::GetPrimaryAssetId ();
    }
    return FPrimaryAssetId (FPrimaryAssetType (TEXT ("GridStatusEffect")), EffectId);
}

bool UGridStatusEffectDefinitionAsset::IsValidDefinition () const
{
    FString Error;
    return ValidateDefinition (Error);
}

bool UGridStatusEffectDefinitionAsset::ValidateDefinition (FString& OutError) const
{
    TArray<FString> Errors;
    if (EffectId.IsNone ())
    {
        Errors.Add (TEXT ("EffectId must not be None."));
    }
    if (DisplayName.IsEmpty ())
    {
        Errors.Add (TEXT ("DisplayName must not be empty."));
    }
    if (DurationUnit == EGridStatusEffectDurationUnit::Permanent)
    {
        if (DefaultDuration != 0)
        {
            Errors.Add (TEXT ("Permanent effects must use DefaultDuration = 0."));
        }
    }
    else if (DefaultDuration <= 0)
    {
        Errors.Add (TEXT ("Timed effects require DefaultDuration greater than zero."));
    }
    if (DefaultPotency < 0)
    {
        Errors.Add (TEXT ("DefaultPotency must not be negative."));
    }
    if (MaxStacks < 1)
    {
        Errors.Add (TEXT ("MaxStacks must be at least one."));
    }
    if (StackPolicy == EGridStatusEffectStackPolicy::AddStacks)
    {
        if (MaxStacks < 2)
        {
            Errors.Add (TEXT ("AddStacks requires MaxStacks of at least two."));
        }
    }
    else if (MaxStacks != 1)
    {
        Errors.Add (TEXT ("Only AddStacks may declare MaxStacks greater than one."));
    }
    if (!PeriodicDamage.IsValid ())
    {
        Errors.Add (TEXT ("PeriodicDamage.DamagePerStack must not be negative."));
    }
    if (PeriodicDamage.IsEnabled () &&
        DurationUnit == EGridStatusEffectDurationUnit::Permanent)
    {
        Errors.Add (TEXT ("Periodic damage requires a Turns or Rounds duration in MON16.3."));
    }
    if (Control.bSkipActivation &&
        DurationUnit == EGridStatusEffectDurationUnit::Permanent)
    {
        Errors.Add (TEXT ("SkipActivation requires a Turns or Rounds duration in MON16.5."));
    }

    OutError = FString::Join (Errors, TEXT ("\n"));
    return Errors.IsEmpty ();
}

bool UGridStatusEffectDefinitionAsset::BuildRuntimeState (
    const FGuid& SourceId,
    int32 InitialStackCount,
    int32 DurationOverride,
    int32 PotencyOverride,
    FGridStatusEffectRuntimeState& OutState,
    FString& OutError) const
{
    FString DefinitionError;
    if (!ValidateDefinition (DefinitionError))
    {
        OutError = FString::Printf (TEXT ("Status effect definition is invalid: %s"), *DefinitionError);
        return false;
    }
    if (InitialStackCount < 1 || InitialStackCount > MaxStacks)
    {
        OutError = FString::Printf (
            TEXT ("InitialStackCount %d is outside the valid range [1, %d]."),
            InitialStackCount,
            MaxStacks);
        return false;
    }

    const int32 ResolvedDuration = DurationOverride == INDEX_NONE ? DefaultDuration : DurationOverride;
    const int32 ResolvedPotency = PotencyOverride == INDEX_NONE ? DefaultPotency : PotencyOverride;
    if (DurationUnit == EGridStatusEffectDurationUnit::Permanent)
    {
        if (ResolvedDuration != 0)
        {
            OutError = TEXT ("Permanent runtime states require RemainingDuration = 0.");
            return false;
        }
    }
    else if (ResolvedDuration <= 0)
    {
        OutError = TEXT ("Timed runtime states require RemainingDuration greater than zero.");
        return false;
    }
    if (ResolvedPotency < 0)
    {
        OutError = TEXT ("Runtime status effect potency must not be negative.");
        return false;
    }

    FGridStatusEffectRuntimeState Candidate;
    Candidate.EffectId = EffectId;
    Candidate.SourceId = SourceId;
    Candidate.StackCount = InitialStackCount;
    Candidate.DurationUnit = DurationUnit;
    Candidate.RemainingDuration = ResolvedDuration;
    Candidate.Potency = ResolvedPotency;
    Candidate.DefinitionAsset =
        const_cast<UGridStatusEffectDefinitionAsset*> (this);
    if (!Candidate.IsValid ())
    {
        OutError = TEXT ("The generated runtime status effect state is invalid.");
        return false;
    }

    OutState = Candidate;
    OutError.Reset ();
    return true;
}