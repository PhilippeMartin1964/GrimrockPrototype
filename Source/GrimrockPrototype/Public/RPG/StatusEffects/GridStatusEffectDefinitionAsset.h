#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridStatusEffectDefinitionAsset.generated.h"

class UTexture2D;

/** Optional deterministic periodic damage payload for MON16.3. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectPeriodicDamageProfile
{
	GENERATED_BODY()

	/** Canonical combat damage type; no parallel resistance vocabulary. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Periodic Damage")
	EGridDamageType DamageType = EGridDamageType::Physical;

	/** Raw damage dealt by each active stack on the effect's duration boundary. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Periodic Damage", meta = (ClampMin = "0"))
	int32 DamagePerStack = 0;

	bool IsEnabled() const
	{
		return DamagePerStack > 0;
	}

	bool IsValid() const
	{
		return DamagePerStack >= 0;
	}
};

/** Optional deterministic periodic Health restoration on the matching duration boundary. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectPeriodicHealingProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Periodic Healing", meta = (ClampMin = "0"))
	int32 HealingPerStack = 0;

	/** Semantic source policy used by outgoing healing modifiers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Periodic Healing")
	EGridCombatActionSourcePolicy SourcePolicy = EGridCombatActionSourcePolicy::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Periodic Healing")
	EGridAttackScalingAttribute ScalingAttribute = EGridAttackScalingAttribute::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Periodic Healing", meta = (ClampMin = "0", ClampMax = "10"))
	int32 AttributeModifierScale = 0;

	bool IsEnabled() const
	{
		return HealingPerStack > 0;
	}

	bool IsValid() const
	{
		return HealingPerStack >= 0 && AttributeModifierScale >= 0 && AttributeModifierScale <= 10 &&
			(IsEnabled() ? SourcePolicy != EGridCombatActionSourcePolicy::None : SourcePolicy == EGridCombatActionSourcePolicy::None) &&
			((ScalingAttribute == EGridAttackScalingAttribute::None) == (AttributeModifierScale == 0));
	}
};

/**
 * Generic MON16.5 combat restrictions. Names such as Stun, Silence and
 * Immobilize remain data-only conventions; production code consumes these
 * boolean capabilities and never compares EffectId.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectControlProfile
{
	GENERATED_BODY()

	/** Consume the target's next matching initiative activation without acting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Control")
	bool bSkipActivation = false;

	/** Block combat actions whose canonical SourcePolicy is Spell. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Control")
	bool bBlockSpellActions = false;

	/** Block cell translation while still allowing turning and attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Control")
	bool bBlockTranslation = false;

	/** C8: direct hostile attacks/spells cannot select this combatant; AoE/DoT/surfaces still apply. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Control")
	bool bBlockDirectHostileTargeting = false;

	bool HasAnyRestriction() const
	{
		return bSkipActivation || bBlockSpellActions || bBlockTranslation || bBlockDirectHostileTargeting;
	}

	void Merge(const FGridStatusEffectControlProfile& Other)
	{
		bSkipActivation = bSkipActivation || Other.bSkipActivation;
		bBlockSpellActions = bBlockSpellActions || Other.bBlockSpellActions;
		bBlockTranslation = bBlockTranslation || Other.bBlockTranslation;
		bBlockDirectHostileTargeting = bBlockDirectHostileTargeting || Other.bBlockDirectHostileTargeting;
	}
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridStatusEffectDefinitionAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Identity")
	FName EffectId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Identity", meta = (MultiLine = "true"))
	FText Description;

	/** C8 semantic tags used by generic removal/target filters (for example Toxin or Purifiable). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Identity")
	TArray<FName> StatusTags;

	/** Optional MON16.6 HUD icon. Runtime rules never depend on it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Rules")
	EGridStatusEffectDisposition Disposition = EGridStatusEffectDisposition::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Rules")
	EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Rounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Rules", meta = (ClampMin = "0"))
	int32 DefaultDuration = 1;

	/** Removes this effect when its owner next becomes the active combatant. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Rules")
	bool bExpireAtOwnerNextActivation = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Rules", meta = (ClampMin = "0"))
	int32 DefaultPotency = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Stacking")
	EGridStatusEffectStackPolicy StackPolicy = EGridStatusEffectStackPolicy::NoStack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Stacking", meta = (ClampMin = "1"))
	int32 MaxStacks = 1;

	/** Allows one runtime instance per SourceId instead of one instance per EffectId. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Stacking")
	bool bDistinctPerSource = false;

	/** When applied to a monster, removes this same EffectId+SourceId from every other combat monster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Stacking", meta = (EditCondition = "bDistinctPerSource"))
	bool bUniquePerSourceAcrossMonsters = false;

	/**
     * Periodic damage executes immediately before the matching Turns/Rounds
     * duration decrement. A zero DamagePerStack means no periodic damage.
     */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Periodic Damage")
	FGridStatusEffectPeriodicDamageProfile PeriodicDamage;

	/** Optional positive tick resolved before the matching duration decrement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Periodic Healing")
	FGridStatusEffectPeriodicHealingProfile PeriodicHealing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Combat")
	int32 InitiativeModifier = 0;

	/** Optional action/movement restrictions aggregated by MON16.5. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Control")
	FGridStatusEffectControlProfile Control;

	/** RPG03.2: C2 modifiers projected while this status is active. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Combat")
	TArray<FGridCombatModifierProfile> CombatModifiers;

	/** RPG03.4: reactions projected while this status is active. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Combat")
	TArray<FGridCombatReactionProfile> CombatReactions;

	UFUNCTION(BlueprintPure, Category = "RPG|Status Effects|Validation")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "RPG|Status Effects|Validation")
	bool ValidateDefinition(UPARAM(ref) FString& OutError) const;

	bool BuildRuntimeState(
		const FGuid& SourceId, int32 InitialStackCount, int32 DurationOverride, FGridStatusEffectRuntimeState& OutState, FString& OutError) const
	{
		return BuildRuntimeState(SourceId, InitialStackCount, DurationOverride, INDEX_NONE, OutState, OutError);
	}

	bool BuildRuntimeState(const FGuid& SourceId, int32 InitialStackCount, int32 DurationOverride, int32 PotencyOverride,
		FGridStatusEffectRuntimeState& OutState, FString& OutError) const;
};
