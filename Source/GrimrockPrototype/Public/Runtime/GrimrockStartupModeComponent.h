#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "GrimrockStartupModeComponent.generated.h"

class AGridLevelRuntimeActor;
class AGrimrockPartyPawn;
class UGridDungeonBuildProgressWidget;
class UGridPartyInventoryComponent;
struct FGridPartyInventoryState;

UCLASS(ClassGroup = (Grimrock), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGrimrockStartupModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrimrockStartupModeComponent();

	/** STARTUPFLOW/ARMEMENT-LOT02.18: complete the frontend -> dungeon
	 * handoff, including the non-serialized item-definition registry. */
	static bool RestorePendingNewGameParty(UGridPartyInventoryComponent* Inventory,
		const FGridPartyInventoryState& PendingPartyState, AGridLevelRuntimeActor* RuntimeActor,
		FText& OutError);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void TryAdvanceNewGameRuntimeBuild();
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
	bool bWaitingForLoadedGameRuntime = false;

	UPROPERTY(Transient)
	bool bWaitingForNewGameRuntimeBuild = false;

	UPROPERTY(Transient)
	bool bNewGameBuildPreparationPresented = false;

	UPROPERTY(Transient)
	bool bNewGameBuildRuntimeReady = false;

	FTimerHandle HideBuildProgressTimerHandle;
};
