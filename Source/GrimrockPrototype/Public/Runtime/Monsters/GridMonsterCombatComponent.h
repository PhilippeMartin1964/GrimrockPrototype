#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "GridMonsterCombatComponent.generated.h"

class AGridMonsterActor;
class AGrimrockPartyPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE (FGridMonsterCombatNotifySignature);

/**
 * Runtime combat bridge owned by a grid monster.
 *
 * The component resolves party targets and applies one attack impact. It does
 * not decide when the monster may act; that remains the TurnManager's job.
 */
UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridMonsterCombatComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridMonsterCombatComponent ();

    virtual void BeginPlay () override;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Monster|Combat")
    bool bAutoInitialize = true;

    /** Number of party slots treated as the first line. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Monster|Combat", meta = (ClampMin = "0"))
    int32 FrontLineSlotCount = 3;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
    TObjectPtr<AGridMonsterActor> OwnerMonster = nullptr;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
    TObjectPtr<AGrimrockPartyPawn> PartyPawn = nullptr;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
    FName LastAttackId = NAME_None;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
    int32 LastTargetCharacterIndex = INDEX_NONE;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
    FGridAttackResult LastAttackResult;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Combat")
    bool bAttackPresentationActive = false;

    /** Broadcast by an Anim Notify or by the TurnManager timer fallback. */
    UPROPERTY (BlueprintAssignable, Category = "Monster|Combat")
    FGridMonsterCombatNotifySignature OnAttackImpactNotify;

    /** Broadcast when the presentation is complete. */
    UPROPERTY (BlueprintAssignable, Category = "Monster|Combat")
    FGridMonsterCombatNotifySignature OnActionCompleteNotify;

    UFUNCTION (BlueprintCallable, Category = "Monster|Combat")
    bool InitializeCombat (AGrimrockPartyPawn* InPartyPawn = nullptr);

    UFUNCTION (BlueprintPure, Category = "Monster|Combat")
    bool IsInitialized () const { return bInitialized; }

    UFUNCTION (BlueprintCallable, Category = "Monster|Combat")
    bool GetPreferredMeleeAttack (FGridMonsterAttackDefinition& OutAttack) const;

    int32 SelectPartyTarget (FRandomStream& RandomStream) const;

    bool ResolveAndApplyPartyAttack (
        int32 TargetCharacterIndex,
        const FGridMonsterAttackDefinition& Attack,
        FRandomStream& RandomStream,
        FGridAttackResult& OutResult);

    /** Starts the optional montage and marks the owner as Attacking. */
    bool StartAttackPresentation (
        const FGridCombatAction& Action,
        const FGridMonsterAttackDefinition& Attack);

    UFUNCTION (BlueprintCallable, Category = "Monster|Combat|Animation Notify")
    void NotifyAttackImpact ();

    UFUNCTION (BlueprintCallable, Category = "Monster|Combat|Animation Notify")
    void NotifyActionComplete ();

    UFUNCTION (BlueprintCallable, Category = "Monster|Combat")
    void CancelAttackPresentation ();

    UFUNCTION (BlueprintCallable, CallInEditor, Category = "Monster|Combat|Debug")
    void LogCombatState () const;

private:
    bool bInitialized = false;

    AGrimrockPartyPawn* FindPartyPawn () const;
    static int32 GetResistancePercent (
        const FGridDamageResistanceSet& Resistances,
        EGridDamageType DamageType);
};
