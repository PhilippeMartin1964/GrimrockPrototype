#include "Runtime/Combat/GridCombatSurfaceResolver.h"

namespace
{
	int32 ScalePositive(int32 Value, int32 PercentModifier)
	{
		const float Multiplier = FMath::Max(0.0f, 1.0f + static_cast<float>(PercentModifier) / 100.0f);
		return FMath::Max(0, FMath::RoundToInt(static_cast<float>(FMath::Max(0, Value)) * Multiplier));
	}

	void SetReaction(FGridCombatSurfaceReactionResult& OutResult, EGridCombatSurfaceType Output, bool bExplosive = false)
	{
		OutResult.bReacted = true;
		OutResult.OutputSurfaceType = Output;
		OutResult.bRemoveSurface = Output == EGridCombatSurfaceType::None;
		OutResult.bExplosive = bExplosive;
		OutResult.ExplosionDamageType = EGridDamageType::Fire;
	}
}

bool FGridCombatSurfaceResolver::BuildState(const FGridCombatSurfaceEffectProfile& Profile, const FGuid& SourceCombatantId, FName SourceActionId,
	const FGridResolvedCombatModifiers& SourceModifiers, FGridCombatSurfaceState& OutState)
{
	OutState = FGridCombatSurfaceState();
	if (!Profile.IsValid())
	{
		return false;
	}

	OutState.SurfaceType = Profile.SurfaceType;
	OutState.RemainingRounds = FMath::Clamp(Profile.DurationRounds + SourceModifiers.SurfaceDurationRoundsModifier, 1, 6);
	OutState.SourceCombatantId = SourceCombatantId;
	OutState.SourceActionId = SourceActionId;
	OutState.PeriodicDamageType = Profile.PeriodicDamageType;
	OutState.PeriodicDamagePerRound = ScalePositive(Profile.PeriodicDamagePerRound, SourceModifiers.SurfacePeriodicDamagePercentModifier);
	OutState.TraversalCostModifier = Profile.TraversalCostModifier;
	OutState.PeriodicStatusApplications = Profile.PeriodicStatusApplications;
	return OutState.IsValid();
}

bool FGridCombatSurfaceResolver::ResolveReaction(const FGridCombatSurfaceState& ExistingSurface, EGridCombatSurfaceInteraction Interaction,
	const FGridResolvedCombatModifiers& SourceModifiers, FGridCombatSurfaceReactionResult& OutResult)
{
	OutResult = FGridCombatSurfaceReactionResult();
	if (!ExistingSurface.IsValid() || Interaction == EGridCombatSurfaceInteraction::None)
	{
		return false;
	}
	if (Interaction == EGridCombatSurfaceInteraction::AnyCanonical)
	{
		for (const EGridCombatSurfaceInteraction Candidate : {
			EGridCombatSurfaceInteraction::Fire,
			EGridCombatSurfaceInteraction::Ice,
			EGridCombatSurfaceInteraction::Lightning,
			EGridCombatSurfaceInteraction::Wind })
		{
			FGridCombatSurfaceReactionResult CandidateResult;
			if (ResolveReaction(ExistingSurface, Candidate, SourceModifiers, CandidateResult))
			{
				OutResult = CandidateResult;
				return true;
			}
		}
		return false;
	}

	switch (ExistingSurface.SurfaceType)
	{
		case EGridCombatSurfaceType::Oil:
			if (Interaction == EGridCombatSurfaceInteraction::Fire)
			{
				SetReaction(OutResult, EGridCombatSurfaceType::Fire);
			}
			break;
		case EGridCombatSurfaceType::Poison:
			if (Interaction == EGridCombatSurfaceInteraction::Fire)
			{
				SetReaction(OutResult, EGridCombatSurfaceType::Fire, true);
			}
			break;
		case EGridCombatSurfaceType::Water:
			if (Interaction == EGridCombatSurfaceInteraction::Ice)
			{
				SetReaction(OutResult, EGridCombatSurfaceType::Ice);
			}
			else if (Interaction == EGridCombatSurfaceInteraction::Lightning)
			{
				SetReaction(OutResult, EGridCombatSurfaceType::ElectrifiedWater);
			}
			break;
		case EGridCombatSurfaceType::Ice:
			if (Interaction == EGridCombatSurfaceInteraction::Fire)
			{
				SetReaction(OutResult, EGridCombatSurfaceType::Water);
			}
			break;
		case EGridCombatSurfaceType::PoisonCloud:
			if (Interaction == EGridCombatSurfaceInteraction::Fire)
			{
				SetReaction(OutResult, EGridCombatSurfaceType::Fire, true);
			}
			break;
		case EGridCombatSurfaceType::Smoke:
			if (Interaction == EGridCombatSurfaceInteraction::Wind)
			{
				SetReaction(OutResult, EGridCombatSurfaceType::None);
			}
			break;
		default:
			break;
	}

	if (!OutResult.bReacted)
	{
		return false;
	}
	OutResult.ResolvedInteraction = Interaction;

	if (OutResult.bExplosive)
	{
		OutResult.ExplosionDamagePercentModifier = SourceModifiers.SurfaceReactionDamagePercentModifier;
		OutResult.ExplosionAreaRadiusModifier = FMath::Clamp(SourceModifiers.SurfaceReactionAreaRadiusModifier, -8, 8);
	}
	return true;
}

bool FGridCombatSurfaceResolver::ResolveAppliedSurfaceReaction(
	const FGridCombatSurfaceState& ExistingSurface,
	const FGridCombatSurfaceEffectProfile& IncomingSurface,
	const FGridResolvedCombatModifiers& SourceModifiers,
	FGridCombatSurfaceReactionResult& OutResult)
{
	OutResult = FGridCombatSurfaceReactionResult();
	if (!IncomingSurface.IsValid())
	{
		return false;
	}

	EGridCombatSurfaceInteraction Interaction = EGridCombatSurfaceInteraction::None;
	switch (IncomingSurface.SurfaceType)
	{
		case EGridCombatSurfaceType::Fire:
			Interaction = EGridCombatSurfaceInteraction::Fire;
			break;
		case EGridCombatSurfaceType::Ice:
			Interaction = EGridCombatSurfaceInteraction::Ice;
			break;
		default:
			// Water/Poison/Oil/etc. are persistent states, not canonical
			// interaction verbs. Lightning and Wind have no direct surface-effect
			// type and remain explicit interactions/conversions.
			return false;
	}

	return ResolveReaction(ExistingSurface, Interaction, SourceModifiers, OutResult);
}

void FGridCombatSurfaceResolver::ApplyReactionToState(
	const FGridCombatSurfaceReactionResult& Reaction, FGridCombatSurfaceState& InOutState)
{
	if (!Reaction.bReacted)
	{
		return;
	}
	if (Reaction.bRemoveSurface || Reaction.OutputSurfaceType == EGridCombatSurfaceType::None)
	{
		InOutState = FGridCombatSurfaceState();
		return;
	}

	InOutState.SurfaceType = Reaction.OutputSurfaceType;
	// Reaction outputs have no invented periodic magnitude. Authoring may
	// subsequently overlay an explicit output profile if the action defines one.
	InOutState.PeriodicDamagePerRound = 0;
	InOutState.TraversalCostModifier = 0;
	InOutState.PeriodicStatusApplications.Reset();
}


bool FGridCombatSurfaceResolver::ResolveConversion(const FGridCombatSurfaceConversionProfile& Profile,
	const FGridCombatSurfaceState* ExistingSurface, const FGuid& SourceCombatantId, FName SourceActionId,
	const FGridResolvedCombatModifiers& SourceModifiers, FGridCombatSurfaceState& OutState)
{
	OutState = FGridCombatSurfaceState();
	if (!Profile.IsValid() || !SourceCombatantId.IsValid() || SourceActionId.IsNone())
	{
		return false;
	}

	int32 BaseDuration = Profile.EmptyCellDurationRounds;
	if (ExistingSurface && ExistingSurface->IsValid())
	{
		if (!Profile.InputSurfaceTypes.Contains(ExistingSurface->SurfaceType))
		{
			return false;
		}
		BaseDuration = ExistingSurface->RemainingRounds;
	}
	else if (!Profile.bAllowEmptyCell)
	{
		return false;
	}

	OutState.SurfaceType = Profile.OutputSurfaceType;
	OutState.RemainingRounds = FMath::Clamp(BaseDuration + SourceModifiers.SurfaceDurationRoundsModifier, 1, 6);
	OutState.SourceCombatantId = SourceCombatantId;
	OutState.SourceActionId = SourceActionId;
	OutState.PeriodicDamageType = EGridDamageType::Physical;
	OutState.PeriodicDamagePerRound = 0;
	OutState.TraversalCostModifier = Profile.OutputTraversalCostModifier;
	OutState.PeriodicStatusApplications.Reset();
	return OutState.IsValid();
}
