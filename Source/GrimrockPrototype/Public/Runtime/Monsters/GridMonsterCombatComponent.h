#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "GridMonsterCombatComponent.generated.h"

class AGridMonsterActor;
class AGrimrockPartyPawn;
class UGridTurnManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGridMonsterCombatNotifySignature);

/**
 * Runtime-only per-monster attack cooldown state.
 *
 * CooldownTurns counts subsequent activations. For CooldownTurns=2, an
 * attack used on turn N is unavailable on N+1 and N+2, then available on N+3.
 */
class GRIMROCKPROTOTYPE_API FGridMonsterAttackCooldownState
{
public:
	void Reset()
	{
		CurrentTurnSerial = 0;
		UnavailableThroughTurnSerial.Reset();
	}

	void BeginTurn()
	{
		++CurrentTurnSerial;
		for (auto It = UnavailableThroughTurnSerial.CreateIterator(); It; ++It)
		{
			if (It.Value() < CurrentTurnSerial)
			{
				It.RemoveCurrent();
			}
		}
	}

	bool IsOnCooldown(FName AttackId) const
	{
		if (AttackId.IsNone() || CurrentTurnSerial <= 0)
		{
			return false;
		}

		const int32* UnavailableThrough = UnavailableThroughTurnSerial.Find(AttackId);
		return UnavailableThrough && CurrentTurnSerial <= *UnavailableThrough;
	}

	bool IsAttackAvailable(const FGridMonsterAttackDefinition& Attack) const
	{
		return !IsOnCooldown(Attack.AttackId);
	}

	bool CommitAttack(const FGridMonsterAttackDefinition& Attack)
	{
		if (Attack.AttackId.IsNone() || CurrentTurnSerial <= 0)
		{
			return false;
		}

		const int32 CooldownTurns = FMath::Max(0, Attack.CooldownTurns);
		if (CooldownTurns <= 0)
		{
			UnavailableThroughTurnSerial.Remove(Attack.AttackId);
			return false;
		}

		UnavailableThroughTurnSerial.Add(Attack.AttackId, CurrentTurnSerial + CooldownTurns);
		return true;
	}

	int32 GetCurrentTurnSerial() const
	{
		return CurrentTurnSerial;
	}

private:
	int32 CurrentTurnSerial = 0;
	TMap<FName, int32> UnavailableThroughTurnSerial;
};

/**
 * Runtime combat bridge owned by a grid monster.
 *
 * The component resolves party targets and applies one attack impact. It does
 * not decide when the monster may act; that remains the TurnManager's job.
 */
UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridMonsterCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridMonsterCombatComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Combat")
	bool bAutoInitialize = true;

	/** Number of party slots treated as the first line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Combat", meta = (ClampMin = "0"))
	int32 FrontLineSlotCount = 3;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
	TObjectPtr<AGridMonsterActor> OwnerMonster = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
	TObjectPtr<AGrimrockPartyPawn> PartyPawn = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
	FName LastAttackId = NAME_None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
	int32 LastTargetCharacterIndex = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
	FGridAttackResult LastAttackResult;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
	bool bAttackPresentationActive = false;

	/** Broadcast by an Anim Notify or by the TurnManager timer fallback. */
	UPROPERTY(BlueprintAssignable, Category = "Monster|Combat")
	FGridMonsterCombatNotifySignature OnAttackImpactNotify;

	/** Broadcast when the presentation is complete. */
	UPROPERTY(BlueprintAssignable, Category = "Monster|Combat")
	FGridMonsterCombatNotifySignature OnActionCompleteNotify;

	UFUNCTION(BlueprintCallable, Category = "Monster|Combat")
	bool InitializeCombat(AGrimrockPartyPawn* InPartyPawn = nullptr);

	UFUNCTION(BlueprintPure, Category = "Monster|Combat")
	bool IsInitialized() const
	{
		return bInitialized;
	}

	/** Returns the highest-priority valid, off-cooldown attack for DistanceCells. */
	UFUNCTION(BlueprintCallable, Category = "Monster|Combat")
	bool GetPreferredAttackForRange(int32 DistanceCells, FGridMonsterAttackDefinition& OutAttack) const;

	/** Compatibility wrapper for existing contact planners. No attack id is hard-coded. */
	UFUNCTION(BlueprintCallable, Category = "Monster|Combat")
	bool GetPreferredMeleeAttack(FGridMonsterAttackDefinition& OutAttack) const;

	UFUNCTION(BlueprintPure, Category = "Monster|Combat|Cooldown")
	bool IsAttackOnCooldown(FName AttackId) const
	{
		return AttackCooldownState.IsOnCooldown(AttackId);
	}

	int32 SelectPartyTarget(FRandomStream& RandomStream) const;

	bool ResolveAndApplyPartyAttack(
		int32 TargetCharacterIndex, const FGridMonsterAttackDefinition& Attack, FRandomStream& RandomStream, FGridAttackResult& OutResult);

	/** Starts the optional montage and marks the owner as Attacking. */
	bool StartAttackPresentation(const FGridCombatAction& Action, const FGridMonsterAttackDefinition& Attack);

	UFUNCTION(BlueprintCallable, Category = "Monster|Combat|Animation Notify")
	void NotifyAttackImpact();

	UFUNCTION(BlueprintCallable, Category = "Monster|Combat|Animation Notify")
	void NotifyActionComplete();

	UFUNCTION(BlueprintCallable, Category = "Monster|Combat")
	void CancelAttackPresentation();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Monster|Combat|Debug")
	void LogCombatState() const;

private:
	bool bInitialized = false;
	FGridMonsterAttackCooldownState AttackCooldownState;
	int32 LastObservedCombatRound = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<UGridTurnManagerComponent> BoundTurnManager = nullptr;

	void ResetAttackCooldowns();
	void RefreshTurnManagerBinding();
	void UnbindTurnManagerEvents();
	void EnsureCurrentCombatTurnObserved();
	void ObserveCombatTurn(int32 RoundNumber);

	UFUNCTION()
	void HandleMonsterTurnStarted(AGridMonsterActor* Monster);

	UFUNCTION()
	void HandleCombatPhaseChanged(EGridCombatPhase NewPhase);

	AGrimrockPartyPawn* FindPartyPawn() const;
	static int32 GetResistancePercent(const FGridDamageResistanceSet& Resistances, EGridDamageType DamageType);
};
