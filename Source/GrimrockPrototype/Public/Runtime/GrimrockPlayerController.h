#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/GridInteractableInterface.h"
#include "Runtime/GridInventoryTypes.h"
#include "GrimrockPlayerController.generated.h"

class UPrimitiveComponent;
class UUserWidget;
class AGridReceptacleActor;
class AGridWallLockActor;
class AGrimrockPartyPawn;
class UGridTurnManagerComponent;

UCLASS()
class GRIMROCKPROTOTYPE_API AGrimrockPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGrimrockPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bInventoryUiOpen = false;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetInventoryUiOpen(bool bOpen);

	/** Starts the exploration/puzzle MainHand throw targeting mode. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Throw")
	bool BeginPhysicalThrowAiming();


	UFUNCTION(BlueprintCallable, Category = "Inventory|Throw")
	bool BeginPhysicalInventoryThrowAiming(FName ItemDefinitionId);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Throw")
	void CancelPhysicalThrowAiming();

	UFUNCTION(BlueprintPure, Category = "Inventory|Throw")
	bool IsPhysicalThrowAimingActive() const
	{
		return bPhysicalThrowAimingActive;
	}

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Interaction", meta = (ClampMin = "0.0"))
	float MaxInteractionDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Throw", meta = (ClampMin = "0.0"))
	float ThrowDistanceThreshold = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Throw", meta = (ClampMin = "0.0"))
	float MaxThrowTargetDistance = 2000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Throw")
	bool bPhysicalThrowAimingActive = false;

	UPROPERTY(Transient)
	int32 PhysicalThrowCharacterIndex = INDEX_NONE;

	UPROPERTY(Transient)
	FGuid PhysicalThrowSourceRuntimeId;


	UPROPERTY(Transient)
	FName PhysicalThrowInventoryDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Interaction|Debug")
	bool bDebugMouseInteraction = false;

	//UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid Interaction|Cursor")
	//bool bUseCustomMouseCursor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Interaction|Cursor")
	TSubclassOf<UUserWidget> CustomCursorWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Interaction|Cursor")
	TObjectPtr<UUserWidget> CustomCursorWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Interaction|Cursor")
	EGridInteractionCursor CurrentGridInteractionCursor = EGridInteractionCursor::Default;

	enum class EGridMouseInteractionIntent : uint8
	{
		DismissReadableMessage,
		IgnoreInventoryUiWithoutCursorItem,
		IgnoreModalUi,
		CursorItemNoWorldHit,
		CursorItemWallLock,
		CursorItemReceptacle,
		CursorItemWorldDrop,
		CursorItemThrow,
		WorldInteractable,
		WorldInteractableOutOfRange,
		WorldInteractableInvalidPawnOrComponent,
		WorldInteractableCanInteractRejected,
		FallbackNoInteractable
	};

	struct FGridMouseInteractionResolution
	{
		EGridMouseInteractionIntent Intent = EGridMouseInteractionIntent::FallbackNoInteractable;
		FName DiagnosticReason = NAME_None;
		AGrimrockPartyPawn* PartyPawn = nullptr;
		bool bHasCursorItem = false;
		bool bItemActionMenuOpen = false;
		FGridItemInstance CursorItem;
		FHitResult HitResult;
		bool bHasWorldHit = false;
		bool bWithinInteractionDistance = false;
		AActor* InteractableActor = nullptr;
		UPrimitiveComponent* HitComponent = nullptr;
		AGridReceptacleActor* ReceptacleActor = nullptr;
		AGridWallLockActor* WallLockActor = nullptr;
		bool bReceptacleAccessible = false;
		int32 DropCellX = INDEX_NONE;
		int32 DropCellY = INDEX_NONE;
		FVector DropLocalOffset = FVector::ZeroVector;
	};

	void HandleLeftMousePressed();
	void HandleCancelCombatTargeting();
	bool UpdatePhysicalThrowAiming();
	bool HandlePhysicalThrowAimingClick();
	void UpdateHoveredInteractable();
	bool UpdateCombatTargeting();
	bool HandleCombatTargetingClick();
	bool TryResolveCombatTargetCellUnderCursor(FIntPoint& OutTargetCell) const;
	void DrawCombatTargetingPreview(const AGrimrockPartyPawn& PartyPawn) const;
	void InitializeCustomCursor();
	void SetGridInteractionCursor(EGridInteractionCursor NewCursor, const TCHAR* Reason = TEXT("Unspecified"));
	FGridMouseInteractionResolution ResolveLeftMouseInteraction();
	bool ResolveCursorItemHoverCursor(const FGridMouseInteractionResolution& MouseResolution, EGridInteractionCursor& OutCursor, const TCHAR*& OutReason) const;
	bool TryGetInteractableUnderCursor(FHitResult& OutHitResult, AActor*& OutInteractableActor) const;
	bool TryGetReceptacleUnderCursor(FHitResult& OutHitResult, AGridReceptacleActor*& OutReceptacleActor) const;
	bool TryGetWorldHitUnderCursor(FHitResult& OutHitResult) const;
	bool TryResolveWorldDropFromHit(
		const FHitResult& HitResult, const AGrimrockPartyPawn* PartyPawn, int32& OutCellX, int32& OutCellY, FVector& OutLocalOffset) const;
	bool IsHitWithinInteractionDistance(const FHitResult& HitResult) const;
	void ShowInteractionFeedback(const FText& MessageText) const;

#if !UE_BUILD_SHIPPING
	UGridTurnManagerComponent* ResolveMON5TurnManager() const;
	void LogMON5CommandResult(const TCHAR* CommandName, bool bSucceeded) const;

	void HandleMON5StartCombatFromPerception();
	void HandleMON5EndPlayerPhase();
	void HandleMON5AbortCombat();
	void HandleMON5LogTurnState();
	void HandleMON5StartCombatWithAllMonsters();
	void HandleMON5ForceVictory();
	void HandleMON11RequestSelectedCharacterAttack();
#endif
};
