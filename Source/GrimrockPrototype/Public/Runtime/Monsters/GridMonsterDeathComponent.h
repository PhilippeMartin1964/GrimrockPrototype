#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridMonsterDeathComponent.generated.h"

class AGridLevelRuntimeActor;
class AGridMonsterActor;

UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridMonsterDeathComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridMonsterDeathComponent ();

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
    TObjectPtr<AGridMonsterActor> OwnerMonster = nullptr;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
    TObjectPtr<AGridLevelRuntimeActor> RuntimeActor = nullptr;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
    FIntPoint DeathCell = FIntPoint::ZeroValue;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
    bool bDeathCommitted = false;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
    bool bLootGenerated = false;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
    bool bDeathPresentationActive = false;

    /** Successfully placed runtime item instances. */
    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
    TArray<FGridItemInstance> GeneratedLoot;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
    int32 PlacedLootCount = 0;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
    int32 FailedLootCount = 0;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death|Debug")
    int32 LogicalDeathEventCount = 0;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death|Debug")
    int32 LinkExecutionAttemptCount = 0;

    UFUNCTION (BlueprintCallable, Category = "Monster|Death")
    bool InitializeDeathComponent (AGridLevelRuntimeActor* InRuntimeActor = nullptr);

    UFUNCTION (BlueprintCallable, Category = "Monster|Death")
    bool CommitDeath ();

    UFUNCTION (BlueprintCallable, Category = "Monster|Death")
    void StartDeathPresentation ();

    UFUNCTION (BlueprintCallable, Category = "Monster|Death|Animation Notify")
    void NotifyDeathPresentationComplete ();

    UFUNCTION (BlueprintPure, Category = "Monster|Death")
    bool IsDeathCommitted () const { return bDeathCommitted; }

    UFUNCTION (BlueprintCallable, CallInEditor, Category = "Monster|Death|Debug")
    void DebugKillMonster ();

private:
    FTimerHandle DeathPresentationTimerHandle;

    void GenerateAndPlaceLoot ();
    AGridLevelRuntimeActor* FindRuntimeActor () const;
};
