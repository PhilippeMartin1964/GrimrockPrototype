#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridSpellTypes.generated.h"

/** Broad spell school. Production content may extend this vocabulary later. */
UENUM(BlueprintType)
enum class EGridSpellSchool : uint8
{
	None UMETA(DisplayName = "None"),
	Arcane UMETA(DisplayName = "Arcane"),
	Fire UMETA(DisplayName = "Fire"),
	Frost UMETA(DisplayName = "Frost"),
	Air UMETA(DisplayName = "Air"),
	Earth UMETA(DisplayName = "Earth"),
	Life UMETA(DisplayName = "Life"),
	Death UMETA(DisplayName = "Death"),
	Protection UMETA(DisplayName = "Protection")
};

/** Declarative effect kinds. Runtime execution is deliberately outside MON18.1. */
UENUM(BlueprintType)
enum class EGridSpellEffectType : uint8
{
	None UMETA(DisplayName = "None"),
	Damage UMETA(DisplayName = "Damage"),
	Heal UMETA(DisplayName = "Heal"),
	ApplyStatusEffect UMETA(DisplayName = "Apply Status Effect"),
	RemoveStatusEffect UMETA(DisplayName = "Remove Status Effect")
};

/** Stable validation failures suitable for tests, logs and later UI feedback. */
UENUM(BlueprintType)
enum class EGridSpellValidationError : uint8
{
	None UMETA(DisplayName = "None"),
	MissingSpellId UMETA(DisplayName = "Missing Spell Id"),
	MissingDisplayName UMETA(DisplayName = "Missing Display Name"),
	InvalidManaCost UMETA(DisplayName = "Invalid Mana Cost"),
	InvalidActionCost UMETA(DisplayName = "Invalid Action Point Cost"),
	InvalidRange UMETA(DisplayName = "Invalid Range"),
	MissingTargetPolicy UMETA(DisplayName = "Missing Target Policy"),
	MissingEffects UMETA(DisplayName = "Missing Effects"),
	InvalidEffect UMETA(DisplayName = "Invalid Effect"),
	MissingCaster UMETA(DisplayName = "Missing Caster"),
	MissingTarget UMETA(DisplayName = "Missing Target")
};

/**
 * One declarative effect produced by a spell.
 * StatusEffectId intentionally references MON16 by stable identity instead of
 * embedding or duplicating the MON16 runtime state.
 */
USTRUCT(BlueprintType)
struct FGridSpellEffectDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Effect")
	EGridSpellEffectType Type = EGridSpellEffectType::None;

	/** Generic magnitude for damage/healing and future scalable effects. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Effect", meta = (ClampMin = "0"))
	int32 Magnitude = 0;

	/** Stable MON16 effect id for ApplyStatusEffect/RemoveStatusEffect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Effect")
	FName StatusEffectId = NAME_None;

	bool IsStructurallyValid() const
	{
		switch (Type)
		{
			case EGridSpellEffectType::Damage:
			case EGridSpellEffectType::Heal:
				return Magnitude > 0;

			case EGridSpellEffectType::ApplyStatusEffect:
			case EGridSpellEffectType::RemoveStatusEffect:
				return !StatusEffectId.IsNone();

			default:
				return false;
		}
	}
};

/** Pure data contract for one spell. No runtime resource mutation occurs here. */
USTRUCT(BlueprintType)
struct FGridSpellDefinition
{
	GENERATED_BODY()

	/** Stable primary identity used by spellbooks, hotbar and persistence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell")
	FName SpellId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell")
	EGridSpellSchool School = EGridSpellSchool::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Cost", meta = (ClampMin = "0"))
	int32 ManaCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Cost", meta = (ClampMin = "0"))
	int32 ActionPointCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Targeting", meta = (ClampMin = "0"))
	int32 MinRangeCells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Targeting", meta = (ClampMin = "0"))
	int32 MaxRangeCells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Targeting")
	EGridCombatTargetingPolicy TargetingPolicy = EGridCombatTargetingPolicy::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Targeting")
	bool bRequiresLineOfSight = false;

	/** Number of complete combat rounds before the spell may be used again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Cost", meta = (ClampMin = "0"))
	int32 CooldownRounds = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Effects")
	TArray<FGridSpellEffectDefinition> Effects;
};

/**
 * Serializable spell target descriptor. Actor pointers are deliberately absent.
 * Party/monster targets use stable ids; cell targeting uses grid coordinates.
 */
USTRUCT(BlueprintType)
struct FGridSpellTarget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Target")
	FGuid TargetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Target")
	FIntPoint GridCell = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Target")
	bool bHasGridCell = false;
};

/** Immutable request contract. Runtime validation/execution is added in MON18.3. */
USTRUCT(BlueprintType)
struct FGridSpellCastRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Cast")
	FGuid CasterCharacterId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Cast")
	FName SpellId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Cast")
	FGridSpellTarget Target;
};

/** Pure MON18.1 structural validation. It never spends PA/mana or applies effects. */
struct GRIMROCKPROTOTYPE_API FGridSpellContract{ static EGridSpellValidationError ValidateDefinition(const FGridSpellDefinition& Definition){
	if (Definition.SpellId.IsNone()){ return EGridSpellValidationError::MissingSpellId;
}
if (Definition.DisplayName.IsEmpty())
{
	return EGridSpellValidationError::MissingDisplayName;
}
if (Definition.ManaCost < 0)
{
	return EGridSpellValidationError::InvalidManaCost;
}
if (Definition.ActionPointCost < 0)
{
	return EGridSpellValidationError::InvalidActionCost;
}
if (Definition.MinRangeCells < 0 || Definition.MaxRangeCells < Definition.MinRangeCells)
{
	return EGridSpellValidationError::InvalidRange;
}
if (Definition.TargetingPolicy == EGridCombatTargetingPolicy::None)
{
	return EGridSpellValidationError::MissingTargetPolicy;
}
if (Definition.Effects.IsEmpty())
{
	return EGridSpellValidationError::MissingEffects;
}
for (const FGridSpellEffectDefinition& Effect : Definition.Effects)
{
	if (!Effect.IsStructurallyValid())
	{
		return EGridSpellValidationError::InvalidEffect;
	}
}
return EGridSpellValidationError::None;
}

static EGridSpellValidationError ValidateRequest(const FGridSpellDefinition& Definition, const FGridSpellCastRequest& Request)
{
	const EGridSpellValidationError DefinitionError = ValidateDefinition(Definition);
	if (DefinitionError != EGridSpellValidationError::None)
	{
		return DefinitionError;
	}
	if (!Request.CasterCharacterId.IsValid() || Request.SpellId != Definition.SpellId)
	{
		return EGridSpellValidationError::MissingCaster;
	}

	switch (Definition.TargetingPolicy)
	{
		case EGridCombatTargetingPolicy::Self:
			return EGridSpellValidationError::None;

		case EGridCombatTargetingPolicy::Ally:
		case EGridCombatTargetingPolicy::FirstAxialTarget:
			return Request.Target.TargetId.IsValid() ? EGridSpellValidationError::None : EGridSpellValidationError::MissingTarget;

		case EGridCombatTargetingPolicy::Cell:
		case EGridCombatTargetingPolicy::Area:
			return Request.Target.bHasGridCell ? EGridSpellValidationError::None : EGridSpellValidationError::MissingTarget;

		default:
			return EGridSpellValidationError::MissingTargetPolicy;
	}
}
}
;
