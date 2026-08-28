#pragma once

#include "CoreMinimal.h"
#include "Magic/GridSpellTypes.h"
#include "RPG/RPGCharacterTypes.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "GridSpellEffectResolver.generated.h"

class UGridStatusEffectDefinitionAsset;

UENUM(BlueprintType)
enum class EGridSpellEffectResolutionRejectReason : uint8
{
	None UMETA(DisplayName = "None"),
	InvalidSpellDefinition UMETA(DisplayName = "Invalid Spell Definition"),
	InvalidTargetState UMETA(DisplayName = "Invalid Target State"),
	MissingStatusEffectDefinition UMETA(DisplayName = "Missing Status Effect Definition"),
	InvalidStatusEffectDefinition UMETA(DisplayName = "Invalid Status Effect Definition"),
	StatusEffectApplyFailed UMETA(DisplayName = "Status Effect Apply Failed"),
	NoEffectWouldApply UMETA(DisplayName = "No Effect Would Apply")
};

USTRUCT(BlueprintType)
struct FGridResolvedSpellEffect
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Effect")
	EGridSpellEffectType Type = EGridSpellEffectType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Effect")
	int32 MagnitudeApplied = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Effect")
	FName StatusEffectId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Effect")
	EGridStatusEffectApplyOutcome StatusApplyOutcome = EGridStatusEffectApplyOutcome::None;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Effect")
	bool bMutatedTarget = false;
};

USTRUCT(BlueprintType)
struct FGridSpellEffectResolutionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Effect")
	FName SpellId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Effect")
	int32 TotalDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Effect")
	int32 TotalHealing = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Effect")
	TArray<FGridResolvedSpellEffect> Effects;

	bool DidMutateTarget() const
	{
		return Effects.ContainsByPredicate(
			[](const FGridResolvedSpellEffect& Effect)
			{
				return Effect.bMutatedTarget;
			});
	}

	void Reset()
	{
		*this = FGridSpellEffectResolutionResult();
	}
};

/**
 * MON18.5 deterministic spell-effect resolver.
 *
 * The service mutates the already-authoritative health/status fields supplied
 * by the caller. It performs all work on copies first and only commits the
 * complete effect batch after every effect has resolved successfully.
 */
struct GRIMROCKPROTOTYPE_API FGridSpellEffectResolver
{
	static bool ResolveEffects(const FGridSpellDefinition& Definition, const FGuid& SourceId, int32 MaxHealth, int32& InOutCurrentHealth,
		FGridStatusEffectCollection& InOutStatusEffects, TFunctionRef<const UGridStatusEffectDefinitionAsset*(FName)> StatusDefinitionResolver,
		FGridSpellEffectResolutionResult& OutResult, EGridSpellEffectResolutionRejectReason& OutRejectReason, FString& OutError);

	static bool ResolveCharacterEffects(const FGridSpellDefinition& Definition, const FGuid& SourceId, const FRPGDerivedStats& DerivedStats,
		FRPGCharacterResources& InOutResources, FGridStatusEffectCollection& InOutStatusEffects,
		TFunctionRef<const UGridStatusEffectDefinitionAsset*(FName)> StatusDefinitionResolver, FGridSpellEffectResolutionResult& OutResult,
		EGridSpellEffectResolutionRejectReason& OutRejectReason, FString& OutError);
};
