#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "GrimrockStartupModeComponent.generated.h"

class AGridLevelRuntimeActor;
class AGrimrockPartyPawn;
class UGridDungeonBuildProgressWidget;

UCLASS(ClassGroup = (Grimrock), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGrimrockStartupModeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGrimrockStartupModeComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void DeferNewGameRuntimeActivation(AGrimrockPartyPawn* PartyPawn);
    void TryActivateDeferredNewGameRuntime();
    void TryCompleteLoadedGameProgress();
    void SetWaitingTickEnabled();
    void ShowBuildProgress(const FText& Title, const FText& StatusText, float Progress);
    void UpdateBuildProgress(const FText& StatusText, float Progress);
    void CompleteBuildProgress(const FText& StatusText);
    void HideBuildProgress();

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime|Build Progress", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UGridDungeonBuildProgressWidget> BuildProgressWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime|Build Progress", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
    float BuildProgressMinimumVisibleSeconds = 0.35f;

    UPROPERTY(Transient)
    TObjectPtr<UGridDungeonBuildProgressWidget> BuildProgressWidgetInstance;

    UPROPERTY(Transient)
    TObjectPtr<AGrimrockPartyPawn> CachedPartyPawn;

    UPROPERTY(Transient)
    TObjectPtr<AGridLevelRuntimeActor> DeferredRuntimeActor;

    UPROPERTY(Transient)
    bool bWaitingForInitialCharacterCreation = false;

    UPROPERTY(Transient)
    bool bWaitingForLoadedGameRuntime = false;

    FTimerHandle HideBuildProgressTimerHandle;
};
