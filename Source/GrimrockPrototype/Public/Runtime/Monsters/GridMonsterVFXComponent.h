#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "Runtime/Monsters/GridMonsterVFXTypes.h"
#include "GridMonsterVFXComponent.generated.h"

class AGridMonsterActor;
class UNiagaraComponent;

DECLARE_LOG_CATEGORY_EXTERN (LogGridMonsterVFX, Log, All);

UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridMonsterVFXComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridMonsterVFXComponent ();

    virtual void BeginPlay () override;
    virtual void EndPlay (
        const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Monster|VFX")
    bool bVFXEnabled = true;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Transient,
        Category = "Monster|VFX|Debug")
    bool bNativeSpawnEnabled = true;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Monster|VFX")
    TObjectPtr<AGridMonsterActor> OwnerMonster = nullptr;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Monster|VFX")
    FGridMonsterVFXSpawnRequest LastSpawnRequest;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Monster|VFX|Debug")
    int32 SpawnRequestCount = 0;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Monster|VFX|Debug")
    int32 VFXSpawnBroadcastCount = 0;

    UPROPERTY (BlueprintAssignable, Category = "Monster|VFX")
    FGridMonsterVFXSpawnRequestedSignature OnVFXSpawnRequested;

    UFUNCTION (BlueprintCallable, Category = "Monster|VFX")
    bool InitializeMonsterVFX ();

    UFUNCTION (BlueprintPure, Category = "Monster|VFX")
    bool IsInitialized () const { return bInitialized; }

    UFUNCTION (BlueprintCallable, Category = "Monster|VFX")
    bool PlayAlertVFX ();

    UFUNCTION (BlueprintCallable, Category = "Monster|VFX")
    bool PlayAttackVFX (
        const FGridMonsterAttackDefinition& Attack);

    UFUNCTION (BlueprintCallable, Category = "Monster|VFX")
    bool PlayAttackImpactVFX (
        const FGridMonsterAttackDefinition& Attack,
        const FGridAttackResult& Result,
        FVector ImpactWorldLocation,
        int32 TargetCharacterIndex);

    UFUNCTION (BlueprintCallable, Category = "Monster|VFX")
    bool PlayHurtVFX (const FGridAttackResult& Result);

    UFUNCTION (BlueprintCallable, Category = "Monster|VFX")
    bool PlayDeathVFX ();

    UFUNCTION (BlueprintCallable, Category = "Monster|VFX")
    void StopAllMonsterVFX ();

    UFUNCTION (BlueprintCallable, Category = "Monster|VFX")
    void ResetTransientVFXState ();

    UFUNCTION (BlueprintPure, Category = "Monster|VFX")
    int32 GetSpawnRequestCountForEvent (
        EGridMonsterVFXEvent Event) const;

    UFUNCTION (BlueprintPure, Category = "Monster|VFX|Debug")
    int32 GetActiveNiagaraComponentCount () const;

    UFUNCTION (
        BlueprintCallable,
        CallInEditor,
        Category = "Monster|VFX|Debug")
    void LogMonsterVFXState () const;

private:
    static constexpr int32 VFXEventCount = 6;

    bool bInitialized = false;
    int32 NextSequenceNumber = 1;
    TArray<int32> EventOccurrenceCounts;
    TArray<int32> EventSpawnRequestCounts;
    TArray<double> LastAcceptedEventTimes;
    TArray<TWeakObjectPtr<UNiagaraComponent>> ActiveNiagaraComponents;

    bool PlayDefinition (
        EGridMonsterVFXEvent Event,
        const FGridMonsterVFXEventDefinition& Definition,
        FName AttackId,
        const FGridAttackResult* Result,
        FVector ImpactWorldLocation,
        int32 TargetCharacterIndex);

    const FGridMonsterVFXEventDefinition* GetMonsterDefinition (
        EGridMonsterVFXEvent Event) const;
    bool CanRequestVFX (EGridMonsterVFXEvent Event) const;
    int32 GetEventIndex (EGridMonsterVFXEvent Event) const;
    void CompactActiveNiagaraComponents ();
};
