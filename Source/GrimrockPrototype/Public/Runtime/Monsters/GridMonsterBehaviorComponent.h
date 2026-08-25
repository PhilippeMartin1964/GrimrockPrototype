#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"
#include "GridMonsterBehaviorComponent.generated.h"

class AGrimrockPartyPawn;
class AGridLevelRuntimeActor;
class AGridMonsterActor;
class UGridMonsterOccupancySubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogGridMonsterAI, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGridMonsterPerceptionChangedSignature, bool, bCanSeeParty, bool, bCanHearParty);

/**
 * MON4 logical perception and pursuit-path planning.
 * The component never moves the monster and never reasons on Tick. MON5 will
 * ask it for actions at the beginning of a monster turn.
 */
UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridMonsterBehaviorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridMonsterBehaviorComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Behavior")
	bool bAutoInitialize = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Behavior")
	bool bRefreshPerceptionOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Behavior|Debug")
	bool bDrawPathAfterQuery = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Behavior")
	TObjectPtr<AGridLevelRuntimeActor> RuntimeActor = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Behavior")
	TObjectPtr<AGrimrockPartyPawn> PartyPawn = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Perception")
	bool bCanSeeParty = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Perception")
	bool bCanHearParty = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Perception")
	bool bHasLastKnownPartyCell = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Perception")
	FIntPoint LastKnownPartyCell = FIntPoint::ZeroValue;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Pathfinding")
	bool bLastPathFound = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Pathfinding")
	TArray<FIntPoint> LastPath;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Pathfinding")
	FIntPoint LastPathGoal = FIntPoint::ZeroValue;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Pathfinding")
	int32 LastVisitedCellCount = 0;

	UPROPERTY(BlueprintAssignable, Category = "Monster|Perception")
	FGridMonsterPerceptionChangedSignature OnPerceptionChanged;

	UFUNCTION(BlueprintCallable, Category = "Monster|Behavior")
	bool InitializeBehavior(AGridLevelRuntimeActor* InRuntimeActor = nullptr, AGrimrockPartyPawn* InPartyPawn = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Monster|Perception")
	bool RefreshPerception();

	UFUNCTION(BlueprintPure, Category = "Monster|Perception")
	bool HasPartyPerception() const
	{
		return bCanSeeParty || bCanHearParty;
	}

	UFUNCTION(BlueprintCallable, Category = "Monster|Perception")
	void ClearLastKnownPartyCell();

	/** Finds the shortest route to an accessible cell adjacent to the party. */
	UFUNCTION(BlueprintCallable, Category = "Monster|Pathfinding")
	bool FindPursuitPath();

	/** Uses the remembered cell after perception is lost. */
	UFUNCTION(BlueprintCallable, Category = "Monster|Pathfinding")
	bool FindPathToLastKnownPartyCell();

	UFUNCTION(BlueprintCallable, Category = "Monster|Pathfinding")
	bool FindPathToCell(FIntPoint TargetCell, bool bAllowBlockedGoal = false);

	UFUNCTION(BlueprintPure, Category = "Monster|Pathfinding")
	bool GetNextPathCell(FIntPoint& OutCell) const;

	UFUNCTION(BlueprintPure, Category = "Monster|Pathfinding")
	bool GetNextPathDirection(EGridEdge& OutDirection) const;

	UFUNCTION(BlueprintCallable, Category = "Monster|Behavior|Debug")
	void DrawDebugPath(float Duration = 5.0f) const;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Monster|Behavior|Debug")
	void LogDebugState() const;

	UFUNCTION(BlueprintPure, Category = "Monster|Behavior")
	bool IsInitialized() const
	{
		return bInitialized;
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<UGridMonsterOccupancySubsystem> OccupancySubsystem = nullptr;

	bool bInitialized = false;

	AGridMonsterActor* GetMonsterOwner() const;
	AGridLevelRuntimeActor* FindRuntimeActor() const;
	AGrimrockPartyPawn* FindPartyPawn() const;
	FIntPoint GetPartyCell() const;
	bool BuildAttackGoals(const FIntPoint& TargetPartyCell, TArray<FIntPoint>& OutGoals) const;
	bool ExecutePathQuery(const FGridMonsterPathQuery& Query);
	bool CanTraverseCells(const FIntPoint& From, const FIntPoint& To) const;
	bool IsCellBlockedForOwner(const FIntPoint& Cell) const;
	void StorePathResult(const FGridMonsterPathResult& Result);
	void ClearPathResult();
	void UpdateOwnerStateFromPerception();
};
