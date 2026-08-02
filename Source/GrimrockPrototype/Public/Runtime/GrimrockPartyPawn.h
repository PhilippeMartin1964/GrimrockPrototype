#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Core/GridTypes.h"
#include "Core/GridDirectionUtils.h"
#include "Runtime/GridInventoryTypes.h"
#include "GrimrockPartyPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AGridLevelRuntimeActor;
class AGridItemActor;
class AGridReceptacleActor;
class AGridThrownItemActor;
class UGridItemDefinitionAsset;
class UGridPartyInventoryComponent;
class UGridInventoryWidget;
class UGridCombatActionPanelWidget;
class UGridTurnManagerComponent;
class UGrimrockMenuWidget;
class URPGCharacterCreationWidget;

UENUM (BlueprintType)
enum class EGridItemThrowMode : uint8
{
    ShortToss,
    Throw
};

UENUM (BlueprintType)
enum class EGrimrockPartyStartupMode : uint8
{
    NewGame,
    Continue
};

UCLASS ()
class GRIMROCKPROTOTYPE_API AGrimrockPartyPawn : public APawn
{
    GENERATED_BODY ()

public:
    AGrimrockPartyPawn ();

    virtual void BeginPlay () override;
    virtual void EndPlay (const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick (float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent (UInputComponent* PlayerInputComponent) override;

public:
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USpringArmComponent* SpringArm;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UCameraComponent* Camera;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> HeldItemRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<UGridPartyInventoryComponent> PartyInventoryComponent;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    TObjectPtr<AGridLevelRuntimeActor> LevelRuntimeActor;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 CurrentCellX = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 CurrentCellY = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    EGridEdge Facing = EGridEdge::North;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float MoveDuration = 0.36f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float TurnDuration = 0.12f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float EyeHeight = 110.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Inventory|Throw", meta = (ClampMin = "0.0"))
    float ShortThrowSpeedScale = 0.45f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Inventory|Throw", meta = (ClampMin = "0.0"))
    float ShortThrowArcScale = 1.5f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|View")
    FVector CameraLocalOffset = FVector (-40.f, 0.f, 0.f);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|View")

    FRotator CameraLocalRotationOffset = FRotator (-4.f, 0.f, 0.f);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveForwardAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveBackwardAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> TurnLeftAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> TurnRightAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> StrafeLeftAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> StrafeRightAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> UseAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input|Legacy")
    bool bEnableLegacyKeyboardUseAction = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory|UI")
    TSubclassOf<UGrimrockMenuWidget> MenuWidgetClass;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|UI")
    TObjectPtr<UGrimrockMenuWidget> MenuWidgetInstance;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|UI")
    bool bInventoryWidgetVisible = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|UI")
    TSubclassOf<UGridCombatActionPanelWidget>
        CombatActionPanelWidgetClass;

    /**
     * INDEX_NONE follows the selected character. An explicit index associates
     * this first panel with a fixed party member and prepares MON12.7.
     */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|UI")
    int32 CombatActionPanelCharacterIndex = INDEX_NONE;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Combat|UI",
        meta = (ClampMin = "0"))
    int32 CombatActionPanelZOrder = 50;

    UPROPERTY (Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UGridCombatActionPanelWidget>
        CombatActionPanelWidgetInstance;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
    TSubclassOf<URPGCharacterCreationWidget> CharacterCreationWidgetClass;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation")
    TObjectPtr<URPGCharacterCreationWidget> CharacterCreationWidgetInstance;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation")
    bool bCharacterCreationModalActive = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Save")
    EGrimrockPartyStartupMode PartyStartupMode = EGrimrockPartyStartupMode::Continue;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Save")
    FString PartySaveSlotName = TEXT ("GrimrockParty");

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Save", meta = (ClampMin = "0"))
    int32 PartySaveUserIndex = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Save")
    bool bAutoSaveOnInventoryClose = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input Buffer")
    bool bEnableInputBuffer = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input Buffer", meta = (ClampMin = "0.0"))
    float InputBufferMaxAge = 0.25f;
    // Head Bob
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Head Bob")
    bool bEnableHeadBob = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Head Bob", meta = (ClampMin = "0.0"))
    float HeadBobVerticalAmplitude = 6.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Head Bob", meta = (ClampMin = "0.0"))
    float HeadBobHorizontalAmplitude = 1.5f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Head Bob", meta = (ClampMin = "0.0"))
    float HeadBobReturnSpeed = 10.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Head Bob")
    bool bHeadBobStrafeSway = true;
    // Free look
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    float FreeLookYawLimit = 60.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    float FreeLookPitchUpLimit = 35.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    float FreeLookPitchDownLimit = 45.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    float FreeLookSensitivityYaw = 0.20f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    float FreeLookSensitivityPitch = 0.20f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    bool bEnableFreeLookRecentering = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look", meta = (EditCondition = "bEnableFreeLookRecentering", ClampMin = "0.0"))
    float FreeLookRecenteringSpeed = 6.f;

    UPROPERTY (BlueprintReadOnly, Category = "Camera|Free Look")
    bool bIsFreeLooking = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    FName DefaultInteractionItemId = TEXT ("Item_Torch");

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Held Item")
    FName DefaultHeldItemDefinitionId = TEXT ("Item_Torch");

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Held Item")
    TSubclassOf<AGridItemActor> HeldTorchActorClass;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Held Item")
    FVector HeldItemRelativeLocation = FVector (45.f, 22.f, -24.f);

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Held Item")
    FRotator HeldItemRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Held Item")
    FVector HeldItemRelativeScale = FVector::OneVector;

    // Visual-only actor attached to the party view. Real item ownership stays in inventory, equipment, cursor, receptacle, or world state.
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Held Item")
    TObjectPtr<AGridItemActor> HeldItemActor;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Held Item")
    bool bHasTorchInHand = false;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Held Item")
    FName HeldItemDefinitionId = NAME_None;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool HasInventoryItem (FName ItemDefinitionId) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool CanAddItemInstanceToSelectedCharacterInventory (const FGridItemInstance& ItemInstance) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool AddItemInstanceToSelectedCharacterInventory (const FGridItemInstance& ItemInstance);

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool AddRuntimeItemToSelectedCharacterInventory (
        const FGuid& RuntimeObjectId,
        FName ItemDefinitionId,
        float Weight,
        int32 Quantity,
        bool bLightsEnabled);

    UFUNCTION (BlueprintCallable, CallInEditor, Category = "Inventory|Debug")
    void LogPartyInventoryDiagnostics () const;

    UFUNCTION (BlueprintCallable, CallInEditor, Category = "Inventory|Debug")
    void LogItemDefinitionDiagnostics () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|UI")
    void ToggleInventoryWidget ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|UI")
    void ShowInventoryWidget ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|UI")
    void HideInventoryWidget ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|UI")
    UGridInventoryWidget* GetInventoryWidget () const;

    UFUNCTION (BlueprintCallable, Category = "Combat|UI")
    bool ShowCombatActionPanelWidget ();

    UFUNCTION (BlueprintCallable, Category = "Combat|UI")
    void HideCombatActionPanelWidget ();

    UFUNCTION (BlueprintCallable, Category = "Combat|UI")
    void RefreshCombatActionPanelWidget ();

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation")
    void ShowInitialCharacterCreationWidget ();

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation")
    void HandleInitialCharacterCreated ();

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation")
    bool IsCharacterCreationModalActive () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Save")
    bool HasCurrentSave () const;

    UFUNCTION (BlueprintCallable, Category = "RPG|Save")
    bool SaveCurrentGame (UPARAM (ref) FText& OutError);

    UFUNCTION (BlueprintCallable, Category = "RPG|Save")
    bool LoadCurrentGame (UPARAM (ref) FText& OutError);

    UFUNCTION (BlueprintCallable, Category = "RPG|Save")
    bool StartNewGame (UPARAM (ref) FText& OutError);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool EquipSelectedCharacterItemFromInventorySlot (int32 InventorySlotIndex, EGridEquipmentSlot TargetSlot);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool UnequipSelectedCharacterItemToInventory (EGridEquipmentSlot SourceSlot);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool TryTakeSelectedCharacterEquipmentSlotToCursor (EGridEquipmentSlot SourceSlot);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool TryTakeSelectedCharacterMainHandToCursor ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool TryTakeSelectedCharacterOffHandToCursor ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool TryEquipCursorItemToSelectedCharacterSlot (EGridEquipmentSlot TargetSlot);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool TryEquipCursorItemToSelectedCharacterMainHand ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool TryEquipCursorItemToSelectedCharacterOffHand ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool HasCursorItem () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool GetCursorItem (FGridItemInstance& OutItem) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Debug")
    bool DebugTakeInventorySlotToCursor (int32 CharacterIndex, int32 InventorySlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Debug")
    bool DebugPlaceCursorItemInSelectedInventory ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Receptacle")
    bool TryPlaceCursorItemInReceptacle (AGridReceptacleActor* ReceptacleActor);

    UFUNCTION (BlueprintCallable, Category = "Inventory|World")
    bool TryDropCursorItemAtCell (int32 CellX, int32 CellY, EGridEdge Edge, const FVector& LocalOffset);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Throw")
    bool TryThrowOneCursorItem (const FVector& LaunchDirection, EGridItemThrowMode ThrowMode);

    /**
     * Transfers one unit from an equipped hand to a recoverable world
     * projectile. Combat damage remains entirely outside this function.
     */
    AGridThrownItemActor* TryLaunchEquippedItemForAttack (
        int32 CharacterIndex,
        EGridEquipmentSlot SourceSlot,
        FName ExpectedItemDefinitionId,
        const FVector& TargetWorldLocation,
        const FIntPoint& SourceCell);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Receptacle|Debug")
    bool DebugPlaceCursorItemInFrontReceptacle ();

    UFUNCTION (BlueprintCallable, Category = "Held Item")
    bool EquipHeldItem (FName ItemDefinitionId);

    UFUNCTION (BlueprintCallable, Category = "Held Item")
    void ClearHeldItem ();

    UFUNCTION (BlueprintCallable, Category = "Held Item")
    FName GetHeldItemDefinitionId () const;

    UFUNCTION (BlueprintCallable, Category = "Held Item")
    bool IsHoldingItem (FName ItemDefinitionId) const;

    UFUNCTION (BlueprintCallable, Category = "Held Item")
    void SyncHeldVisualFromSelectedCharacterEquipment ();

public:
    UFUNCTION (BlueprintCallable, Category = "Grid")
    void SnapToCurrentCell ();

    UFUNCTION (BlueprintCallable, Category = "Grid")
    void SetGridStart (AGridLevelRuntimeActor* InLevelRuntimeActor, int32 StartX, int32 StartY, EGridEdge StartFacing);

protected:
    void HandleMoveForward (const FInputActionValue& Value);
    void HandleMoveBackward (const FInputActionValue& Value);
    void HandleTurnLeft (const FInputActionValue& Value);
    void HandleTurnRight (const FInputActionValue& Value);
    void HandleStrafeLeft (const FInputActionValue& Value);
    void HandleStrafeRight (const FInputActionValue& Value);
    void HandleUse (const FInputActionValue& Value);
    bool TryUseFrontInteraction ();
    bool TryStartMove (EGridEdge MoveDirection);
    bool TryStartTurn (bool bTurnRight);

    void UpdateMove (float DeltaSeconds);
    void UpdateTurn (float DeltaSeconds);

    bool HasLevelRuntimeActor () const;
    bool CanMoveOnLevel (int32 FromX, int32 FromY, EGridEdge Direction) const;
    bool TryGetNeighborOnLevel (int32 X, int32 Y, EGridEdge Direction, int32& OutX, int32& OutY) const;
    FVector GetCellCenterOnLevel (int32 X, int32 Y, float ZOffset) const;
    bool TryToggleDoorOnLevel (int32 X, int32 Y, EGridEdge Edge);
    // Head Bob
    void UpdateHeadBob (float DeltaSeconds);
    void ApplyCameraOffsets ();
    // Free look
    void BeginFreeLook ();
    void EndFreeLook ();
    void UpdateFreeLook (float DeltaSeconds);
    void ApplyFreeLookRotation ();
    void ApplyCameraLocalViewOffset ();

    bool TryInteractOnLevel (int32 X, int32 Y, EGridEdge Edge);
    bool BuildSingleItemInstanceFromCursor (FGridItemInstance& OutSingleItem) const;
    void ConsumeOneCursorItemAfterSuccessfulAction ();

private:
    UGridItemDefinitionAsset* ResolveEquippedItemDefinition (const FGridItemInstance& Item) const;
    bool DoesEquippedItemEmitLight (const FGridItemInstance& Item) const;
    bool RecomputeEquippedLightState (
        const FGridItemInstance& MainHandItem,
        bool bHasMainHandItem,
        const FGridItemInstance& OffHandItem,
        bool bHasOffHandItem) const;

    enum class EBufferedCommandType : uint8
    {
        None,
        Move,
        Turn,
        Use
    };

    void BufferMoveCommand (EGridEdge MoveDirection);
    void BufferTurnCommand (bool bTurnRight);
    void BufferUseCommand ();
    void ClearBufferedCommand ();
    void ApplyCharacterCreationInputMode (bool bIsActive);
    bool LoadCurrentGameData (FText& OutError, bool bApplyDungeonState);
    bool RehydrateLoadedItemDefinitions (FText& OutError);
    void CloseCharacterCreationWidget ();
    bool TryConsumeBufferedCommand ();
    bool IsBusy () const;
    bool DismissReadableMessageIfVisible ();
    UGridTurnManagerComponent* FindTurnManager () const;

private:
    FVector MoveStartLocation = FVector::ZeroVector;
    FVector MoveTargetLocation = FVector::ZeroVector;
    float MoveElapsed = 0.f;
    bool bIsMoving = false;

    float TurnStartYaw = 0.f;
    float TurnTargetYaw = 0.f;
    float TurnElapsed = 0.f;
    float TurnDeltaYaw = 0.f;
    bool bIsTurning = false;

    EBufferedCommandType BufferedCommandType = EBufferedCommandType::None;
    EGridEdge BufferedMoveDirection = EGridEdge::None;
    bool bBufferedTurnRight = false;
    float BufferedCommandAge = 0.f;

    // Head Bob
    FVector SpringArmBaseRelativeLocation = FVector::ZeroVector;

    float HeadBobAlpha = 0.f;
    FVector CurrentHeadBobOffset = FVector::ZeroVector;
    FVector TargetHeadBobOffset = FVector::ZeroVector;
    EGridEdge ActiveMoveDirection = EGridEdge::None;
    //Free look
    float FreeLookYaw = 0.f;
    float FreeLookPitch = 0.f;

    int32 MoveStartCellX = 0;
    int32 MoveStartCellY = 0;

    friend class FGridMonsterMON12PartyMobilityLifecycleTest;
};
