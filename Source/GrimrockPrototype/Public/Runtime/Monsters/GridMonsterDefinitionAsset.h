#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/Monsters/GridMonsterBalanceTypes.h"
#include "Runtime/Monsters/GridMonsterIdleVariationTypes.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "GridMonsterDefinitionAsset.generated.h"

class UAnimInstance;
class UAnimMontage;
class USkeletalMesh;
class UTexture2D;
class AGridMonsterActor;

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridMonsterDefinitionAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGridMonsterDefinitionAsset();

	virtual void PostLoad() override;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Identity")
	FName MonsterId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Identity", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Identity")
	FName CategoryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Identity", meta = (ClampMin = "1"))
	int32 DangerLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Visual")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Visual")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Visual")
	TSubclassOf<UAnimInstance> AnimationClass;

	/** Actor implementation instantiated by the MON13 spawn pipeline. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Runtime")
	TSubclassOf<AGridMonsterActor> MonsterActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Visual")
	FVector VisualScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Visual")
	FVector VisualOffset = FVector::ZeroVector;

	/** Mesh-local orientation correction; logical grid Facing remains authoritative. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Visual")
	FRotator VisualRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stats", meta = (ClampMin = "1"))
	int32 MaxHealth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stats", meta = (ClampMin = "0"))
	int32 PhysicalArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stats", meta = (ClampMin = "0"))
	int32 MagicalArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
	int32 Initiative = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
	int32 Accuracy = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
	int32 Evasion = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stats", meta = (ClampMin = "1"))
	int32 ActionPointsPerTurn = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Movement")
	FIntPoint GridFootprint = FIntPoint(1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Movement", meta = (ClampMin = "0.0"))
	float MoveDuration = 0.36f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Movement", meta = (ClampMin = "0.0"))
	float TurnDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Movement")
	bool bBlocksMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Movement")
	bool bCanOpenDoors = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Movement")
	bool bCanUseTeleporters = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Perception", meta = (ClampMin = "0"))
	int32 SightRangeCells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Perception", meta = (ClampMin = "0"))
	int32 HearingRangeCells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Perception", meta = (ClampMin = "0"))
	int32 AggroPropagationRange = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Perception")
	bool bSharesAggroWithGroup = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	EGridMonsterAIProfile PrimaryAIProfile = EGridMonsterAIProfile::DirectMelee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	TArray<EGridMonsterAIProfile> AdditionalAIProfiles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI", meta = (ClampMin = "0"))
	int32 PreferredMinDistance = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI", meta = (ClampMin = "0"))
	int32 PreferredMaxDistance = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RetreatChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthThreshold = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Combat")
	TArray<FGridMonsterAttackDefinition> Attacks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Combat")
	TArray<FGridMonsterDamageModifier> DamageModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio")
	FGridMonsterAudioEventDefinition AlertAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio")
	FGridMonsterAudioEventDefinition HurtAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio")
	FGridMonsterAudioEventDefinition DeathAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio")
	FGridMonsterAudioEventDefinition IdleAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio", meta = (ClampMin = "0.1"))
	float IdleAudioMinDelay = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio", meta = (ClampMin = "0.1"))
	float IdleAudioMaxDelay = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio")
	bool bEnableIdleAudio = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|VFX")
	FGridMonsterVFXEventDefinition AlertVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|VFX")
	FGridMonsterVFXEventDefinition HurtVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|VFX")
	FGridMonsterVFXEventDefinition DeathVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Idle Variations")
	bool bEnableIdleVariations = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Idle Variations")
	TArray<FGridMonsterIdleVariationDefinition> IdleVariations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Idle Variations", meta = (ClampMin = "0.1"))
	float IdleVariationMinDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Idle Variations", meta = (ClampMin = "0.1"))
	float IdleVariationMaxDelay = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Idle Variations")
	bool bAvoidImmediateIdleVariationRepeat = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Idle Variations")
	FName IdleVariationSlotName = TEXT("DefaultSlot");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Idle Variations", meta = (ClampMin = "0.0"))
	float IdleVariationBlendInTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Idle Variations", meta = (ClampMin = "0.0"))
	float IdleVariationBlendOutTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Rewards", meta = (ClampMin = "0"))
	int32 ExperienceReward = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Rewards")
	TArray<FGridMonsterLootEntry> LootTable;

	/** Optional presentation only; gameplay never waits for this montage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation")
	TSoftObjectPtr<UAnimMontage> HurtMontage;

	/** Optional presentation only; logical death never waits for this montage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation")
	TSoftObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation", meta = (ClampMin = "0.01"))
	float DeathExpectedDuration = 1.0f;

	/** Optional visual-only corpse dissolve. Disabled by default for backward compatibility. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Death Dissolve")
	bool bEnableDeathDissolve = false;

	/** Time the final corpse pose remains fully visible after the death presentation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Death Dissolve", meta = (ClampMin = "0.0"))
	float DeathDissolveDelay = 2.0f;

	/** Duration of the visual dissolve from fully visible to fully dissolved. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Death Dissolve", meta = (ClampMin = "0.01"))
	float DeathDissolveDuration = 1.5f;

	/** Scalar material parameter driven from 0 (visible) to 1 (dissolved). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation|Death Dissolve")
	FName DeathDissolveParameterName = TEXT("DissolveAmount");

	UFUNCTION(BlueprintPure, Category = "Monster|Validation")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Monster|Validation")
	bool ValidateDefinition(UPARAM(ref) FString& OutError) const;

	UFUNCTION(BlueprintPure, Category = "Monster|Balance")
	bool BuildBalanceSnapshot(FGridMonsterBalanceSnapshot& OutSnapshot) const;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Monster|Balance|Debug")
	void LogBalanceSnapshot() const;

	UFUNCTION(BlueprintPure, Category = "Monster|AI")
	bool HasAIProfile(EGridMonsterAIProfile Profile) const;

	UFUNCTION(BlueprintPure, Category = "Monster|Combat")
	float GetDamageMultiplier(EGridDamageType DamageType, EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None) const;

	UFUNCTION(BlueprintCallable, Category = "Monster|Combat")
	bool GetAttackDefinition(FName AttackId, FGridMonsterAttackDefinition& OutAttack) const;

	const FGridMonsterAttackDefinition* FindAttackDefinition(FName AttackId) const;
};
