#include "Magic/GridSpellEffectResolver.h"

#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

bool FGridSpellEffectResolver::ResolveEffects(const FGridSpellDefinition& Definition, const FGuid& SourceId, int32 MaxHealth, int32& InOutCurrentHealth,
	FGridStatusEffectCollection& InOutStatusEffects, TFunctionRef<const UGridStatusEffectDefinitionAsset*(FName)> StatusDefinitionResolver,
	FGridSpellEffectResolutionResult& OutResult, EGridSpellEffectResolutionRejectReason& OutRejectReason, FString& OutError)
{
	OutResult.Reset();
	OutRejectReason = EGridSpellEffectResolutionRejectReason::None;
	OutError.Reset();

	if (FGridSpellContract::ValidateDefinition(Definition) != EGridSpellValidationError::None)
	{
		OutRejectReason = EGridSpellEffectResolutionRejectReason::InvalidSpellDefinition;
		OutError = TEXT("Spell definition is structurally invalid.");
		return false;
	}

	if (!SourceId.IsValid() || MaxHealth < 1 || InOutCurrentHealth < 0 || InOutCurrentHealth > MaxHealth)
	{
		OutRejectReason = EGridSpellEffectResolutionRejectReason::InvalidTargetState;
		OutError = TEXT("Spell target health/source state is invalid.");
		return false;
	}

	TMap<FName, const UGridStatusEffectDefinitionAsset*> ResolvedStatusDefinitions;
	for (const FGridSpellEffectDefinition& Effect : Definition.Effects)
	{
		if (Effect.Type != EGridSpellEffectType::ApplyStatusEffect)
		{
			continue;
		}

		const UGridStatusEffectDefinitionAsset* StatusDefinition = StatusDefinitionResolver(Effect.StatusEffectId);
		if (!IsValid(StatusDefinition))
		{
			OutRejectReason = EGridSpellEffectResolutionRejectReason::MissingStatusEffectDefinition;
			OutError = FString::Printf(TEXT("Missing status effect definition '%s'."), *Effect.StatusEffectId.ToString());
			return false;
		}
		if (!StatusDefinition->IsValidDefinition() || StatusDefinition->EffectId != Effect.StatusEffectId)
		{
			OutRejectReason = EGridSpellEffectResolutionRejectReason::InvalidStatusEffectDefinition;
			OutError = FString::Printf(TEXT("Status effect definition '%s' is invalid or mismatched."), *Effect.StatusEffectId.ToString());
			return false;
		}
		ResolvedStatusDefinitions.Add(Effect.StatusEffectId, StatusDefinition);
	}

	int32 WorkingHealth = InOutCurrentHealth;
	FGridStatusEffectCollection WorkingStatuses = InOutStatusEffects;
	FGridSpellEffectResolutionResult WorkingResult;
	WorkingResult.SpellId = Definition.SpellId;

	for (const FGridSpellEffectDefinition& Effect : Definition.Effects)
	{
		FGridResolvedSpellEffect Resolved;
		Resolved.Type = Effect.Type;
		Resolved.StatusEffectId = Effect.StatusEffectId;

		switch (Effect.Type)
		{
			case EGridSpellEffectType::Damage:
			{
				const int32 NewHealth = FMath::Max(0, WorkingHealth - Effect.Magnitude);
				Resolved.MagnitudeApplied = WorkingHealth - NewHealth;
				Resolved.bMutatedTarget = Resolved.MagnitudeApplied > 0;
				WorkingHealth = NewHealth;
				WorkingResult.TotalDamage += Resolved.MagnitudeApplied;
				break;
			}

			case EGridSpellEffectType::Heal:
			{
				const int32 NewHealth = FMath::Min(MaxHealth, WorkingHealth + Effect.Magnitude);
				Resolved.MagnitudeApplied = NewHealth - WorkingHealth;
				Resolved.bMutatedTarget = Resolved.MagnitudeApplied > 0;
				WorkingHealth = NewHealth;
				WorkingResult.TotalHealing += Resolved.MagnitudeApplied;
				break;
			}

			case EGridSpellEffectType::ApplyStatusEffect:
			{
				const UGridStatusEffectDefinitionAsset* const* FoundDefinition = ResolvedStatusDefinitions.Find(Effect.StatusEffectId);
				FGridStatusEffectApplyResult ApplyResult;
				FString ApplyError;
				if (!FoundDefinition || !IsValid(*FoundDefinition) || !WorkingStatuses.TryApply(**FoundDefinition, SourceId, ApplyResult, ApplyError))
				{
					OutRejectReason = EGridSpellEffectResolutionRejectReason::StatusEffectApplyFailed;
					OutError =
						ApplyError.IsEmpty() ? FString::Printf(TEXT("Failed to apply status effect '%s'."), *Effect.StatusEffectId.ToString()) : ApplyError;
					return false;
				}
				Resolved.StatusApplyOutcome = ApplyResult.Outcome;
				Resolved.bMutatedTarget = ApplyResult.DidMutate();
				break;
			}

			case EGridSpellEffectType::RemoveStatusEffect:
			{
				const int32 RemovedCount = WorkingStatuses.ActiveEffects.RemoveAll(
					[&Effect](const FGridStatusEffectRuntimeState& RuntimeState)
					{
						return RuntimeState.EffectId == Effect.StatusEffectId;
					});
				Resolved.bMutatedTarget = RemovedCount > 0;
				break;
			}

			default:
				OutRejectReason = EGridSpellEffectResolutionRejectReason::InvalidSpellDefinition;
				OutError = TEXT("Unsupported spell effect type.");
				return false;
		}

		WorkingResult.Effects.Add(Resolved);
	}

	InOutCurrentHealth = WorkingHealth;
	InOutStatusEffects = MoveTemp(WorkingStatuses);
	OutResult = MoveTemp(WorkingResult);
	return true;
}

bool FGridSpellEffectResolver::ResolveCharacterEffects(const FGridSpellDefinition& Definition, const FGuid& SourceId, const FRPGDerivedStats& DerivedStats,
	FRPGCharacterResources& InOutResources, FGridStatusEffectCollection& InOutStatusEffects,
	TFunctionRef<const UGridStatusEffectDefinitionAsset*(FName)> StatusDefinitionResolver, FGridSpellEffectResolutionResult& OutResult,
	EGridSpellEffectResolutionRejectReason& OutRejectReason, FString& OutError)
{
	return ResolveEffects(Definition, SourceId, DerivedStats.MaxHealth, InOutResources.CurrentHealth, InOutStatusEffects, StatusDefinitionResolver, OutResult,
		OutRejectReason, OutError);
}
