#include "Runtime/GrimrockPartyPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Core/GridDirectionUtils.h"
#include "InputCoreTypes.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"

namespace
{
    bool IsHandEquipmentSlot (EGridEquipmentSlot Slot)
    {
        return Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand;
    }

    const TCHAR* GetPawnEquipmentSlotName (EGridEquipmentSlot Slot)
    {
        switch (Slot)
        {
        case EGridEquipmentSlot::None:
            return TEXT ("None");
        case EGridEquipmentSlot::MainHand:
            return TEXT ("MainHand");
        case EGridEquipmentSlot::OffHand:
            return TEXT ("OffHand");
        case EGridEquipmentSlot::Head:
            return TEXT ("Head");
        case EGridEquipmentSlot::Chest:
            return TEXT ("Chest");
        case EGridEquipmentSlot::Legs:
            return TEXT ("Legs");
        case EGridEquipmentSlot::Feet:
            return TEXT ("Feet");
        case EGridEquipmentSlot::Amulet:
            return TEXT ("Amulet");
        case EGridEquipmentSlot::Ring1:
            return TEXT ("Ring1");
        case EGridEquipmentSlot::Ring2:
            return TEXT ("Ring2");
        case EGridEquipmentSlot::Shoulders:
            return TEXT ("Shoulders");
        case EGridEquipmentSlot::Gloves:
            return TEXT ("Gloves");
        case EGridEquipmentSlot::Belt:
            return TEXT ("Belt");
        case EGridEquipmentSlot::Cloak:
            return TEXT ("Cloak");
        case EGridEquipmentSlot::Talisman:
            return TEXT ("Talisman");
        case EGridEquipmentSlot::QuickSlot1:
            return TEXT ("QuickSlot1");
        case EGridEquipmentSlot::QuickSlot2:
            return TEXT ("QuickSlot2");
        default:
            return TEXT ("Unsupported");
        }
    }
}

AGrimrockPartyPawn::AGrimrockPartyPawn ()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    SpringArm = CreateDefaultSubobject<USpringArmComponent> (TEXT ("SpringArm"));
    SpringArm->SetupAttachment (SceneRoot);
    SpringArm->TargetArmLength = 0.f;
    SpringArm->bDoCollisionTest = false;
    SpringArm->bUsePawnControlRotation = false;
    SpringArm->bInheritPitch = false;
    SpringArm->bInheritYaw = true;
    SpringArm->bInheritRoll = false;
    SpringArm->SetRelativeLocation (FVector::ZeroVector);
    SpringArm->SetRelativeRotation (FRotator::ZeroRotator);
    SpringArmBaseRelativeLocation = SpringArm->GetRelativeLocation ();

    Camera = CreateDefaultSubobject<UCameraComponent> (TEXT ("Camera"));
    Camera->SetupAttachment (SpringArm);
    Camera->bUsePawnControlRotation = false;

    HeldItemRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("HeldItemRoot"));
    HeldItemRoot->SetupAttachment (Camera ? Cast<USceneComponent> (Camera) : SceneRoot);

    PartyInventoryComponent = CreateDefaultSubobject<UGridPartyInventoryComponent> (TEXT ("PartyInventoryComponent"));

    AutoPossessPlayer = EAutoReceiveInput::Player0;
    Camera->SetRelativeLocation (CameraLocalOffset);
}

void AGrimrockPartyPawn::BeginPlay ()
{
    Super::BeginPlay ();

    if (PartyInventoryComponent)
    {
        PartyInventoryComponent->InitializeDefaultPartyIfNeeded ();
    }

    if (!LevelRuntimeActor)
    {
        LevelRuntimeActor = Cast<AGridLevelRuntimeActor> (
            UGameplayStatics::GetActorOfClass (GetWorld (), AGridLevelRuntimeActor::StaticClass ())
        );
    }

    if (!LevelRuntimeActor)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GrimrockPartyPawn: no AGridLevelRuntimeActor found."));
    }
    else if (LevelRuntimeActor->bApplyLevelStartOnBeginPlay && LevelRuntimeActor->LevelAsset)
    {
        if (LevelRuntimeActor->LevelAsset->IsStartCellValid ())
        {
            CurrentCellX = LevelRuntimeActor->LevelAsset->StartCellX;
            CurrentCellY = LevelRuntimeActor->LevelAsset->StartCellY;
            Facing = LevelRuntimeActor->LevelAsset->StartFacing == EGridEdge::None
                ? EGridEdge::North
                : LevelRuntimeActor->LevelAsset->StartFacing;
        }
        else
        {
            UE_LOG (
                LogTemp,
                Warning,
                TEXT ("GrimrockPartyPawn: LevelAsset start cell is invalid, keeping configured pawn cell (%d,%d)."),
                CurrentCellX,
                CurrentCellY);
        }
    }
    else if (!LevelRuntimeActor->bApplyLevelStartOnBeginPlay)
    {
        UE_LOG (LogTemp, Log, TEXT ("GrimrockPartyPawn: LevelAsset start application is disabled, keeping configured pawn start."));
    }
    else
    {
        UE_LOG (LogTemp, Warning, TEXT ("GrimrockPartyPawn: LevelRuntimeActor has no LevelAsset, keeping configured pawn start."));
    }

    SnapToCurrentCell ();

    if (LevelRuntimeActor)
    {
        LevelRuntimeActor->HandlePartyCellChanged (CurrentCellX, CurrentCellY, CurrentCellX, CurrentCellY);
    }

    SyncHeldVisualFromSelectedCharacterEquipment ();

    if (APlayerController* PC = Cast<APlayerController> (GetController ()))
    {
        if (ULocalPlayer* LP = PC->GetLocalPlayer ())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem> ())
            {
                if (DefaultMappingContext)
                {
                    Subsystem->AddMappingContext (DefaultMappingContext, 0);
                }
            }
        }
    }
    ApplyCameraLocalViewOffset ();
}

void AGrimrockPartyPawn::Tick (float DeltaSeconds)
{
    Super::Tick (DeltaSeconds);

    if (bIsMoving)
    {
        UpdateMove (DeltaSeconds);
    }

    if (bIsTurning)
    {
        UpdateTurn (DeltaSeconds);
    }

    UpdateHeadBob (DeltaSeconds);
	UpdateFreeLook (DeltaSeconds);

    if (BufferedCommandType != EBufferedCommandType::None)
    {
        BufferedCommandAge += DeltaSeconds;

        if (BufferedCommandAge > InputBufferMaxAge)
        {
            ClearBufferedCommand ();
        }
    }

    if (!IsBusy ())
    {
        TryConsumeBufferedCommand ();
    }
}

void AGrimrockPartyPawn::SetupPlayerInputComponent (UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent (PlayerInputComponent);

    if (!PlayerInputComponent)
    {
        return;
    }

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent> (PlayerInputComponent))
    {
        if (MoveForwardAction)
        {
            EIC->BindAction (MoveForwardAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleMoveForward);
        }

        if (MoveBackwardAction)
        {
            EIC->BindAction (MoveBackwardAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleMoveBackward);
        }

        if (TurnLeftAction)
        {
            EIC->BindAction (TurnLeftAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleTurnLeft);
        }

        if (TurnRightAction)
        {
            EIC->BindAction (TurnRightAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleTurnRight);
        }
        if (StrafeLeftAction)
        {
            EIC->BindAction (StrafeLeftAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleStrafeLeft);
        }

        if (StrafeRightAction)
        {
            EIC->BindAction (StrafeRightAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleStrafeRight);
        }
        
        if (bEnableLegacyKeyboardUseAction && UseAction)
        {
            EIC->BindAction (UseAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleUse);
        }
    }
    PlayerInputComponent->BindKey (EKeys::RightMouseButton, IE_Pressed, this, &AGrimrockPartyPawn::BeginFreeLook);
    PlayerInputComponent->BindKey (EKeys::RightMouseButton, IE_Released, this, &AGrimrockPartyPawn::EndFreeLook);
}

void AGrimrockPartyPawn::ApplyCameraLocalViewOffset ()
{
    if (!Camera)
    {
        return;
    }
    Camera->SetRelativeLocation (CameraLocalOffset);
    Camera->SetRelativeRotation (CameraLocalRotationOffset);
}

void AGrimrockPartyPawn::SetGridStart (
    AGridLevelRuntimeActor* InLevelRuntimeActor,
    int32 StartX,
    int32 StartY,
    EGridEdge StartFacing)
{
    LevelRuntimeActor = InLevelRuntimeActor;
    CurrentCellX = StartX;
    CurrentCellY = StartY;
    Facing = StartFacing;

    SnapToCurrentCell ();
}

void AGrimrockPartyPawn::SnapToCurrentCell ()
{
    if (!HasLevelRuntimeActor ())
    {
        return;
    }

    const FVector WorldPos = GetCellCenterOnLevel (CurrentCellX, CurrentCellY, EyeHeight);
    SetActorLocation (WorldPos);

    FRotator Rot = GetActorRotation ();
    Rot.Yaw = GridDirectionUtils::ToYaw (Facing);
    SetActorRotation (Rot);
}

void AGrimrockPartyPawn::HandleMoveForward (const FInputActionValue& Value)
{
    (void)Value;

    const EGridEdge Direction = GridDirectionUtils::GetForward (Facing);

    if (IsBusy ())
    {
        BufferMoveCommand (Direction);
        return;
    }

    TryStartMove (Direction);
}

void AGrimrockPartyPawn::HandleMoveBackward (const FInputActionValue& Value)
{
    (void)Value;

    const EGridEdge Direction = GridDirectionUtils::GetBackward (Facing);

    if (IsBusy ())
    {
        BufferMoveCommand (Direction);
        return;
    }

    TryStartMove (Direction);
}

void AGrimrockPartyPawn::HandleTurnLeft (const FInputActionValue& Value)
{
    (void)Value;

    if (IsBusy ())
    {
        BufferTurnCommand (false);
        return;
    }

    TryStartTurn (false);
}

void AGrimrockPartyPawn::HandleTurnRight (const FInputActionValue& Value)
{
    (void)Value;

    if (IsBusy ())
    {
        BufferTurnCommand (true);
        return;
    }

    TryStartTurn (true);
}

void AGrimrockPartyPawn::HandleStrafeLeft (const FInputActionValue& Value)
{
    (void)Value;

    const EGridEdge Direction = GridDirectionUtils::GetLeft (Facing);

    if (IsBusy ())
    {
        BufferMoveCommand (Direction);
        return;
    }

    TryStartMove (Direction);
}

void AGrimrockPartyPawn::HandleStrafeRight (const FInputActionValue& Value)
{
    (void)Value;

    const EGridEdge Direction = GridDirectionUtils::GetRight (Facing);

    if (IsBusy ())
    {
        BufferMoveCommand (Direction);
        return;
    }

    TryStartMove (Direction);
}

void AGrimrockPartyPawn::HandleUse (const FInputActionValue& Value)
{
    (void)Value;

    if (IsBusy ())
    {
        BufferUseCommand ();
        return;
    }

    TryUseFrontInteraction ();
}

bool AGrimrockPartyPawn::TryUseFrontInteraction ()
{
    if (bIsMoving || bIsTurning || !HasLevelRuntimeActor ())
    {
        return false;
    }

    if (LevelRuntimeActor->TryPickupItemAtCell (CurrentCellX, CurrentCellY, this))
    {
        return true;
    }

    const EGridEdge FrontEdge = GridDirectionUtils::GetForward (Facing);

    if (TryInteractOnLevel (CurrentCellX, CurrentCellY, FrontEdge))
    {
        return true;
    }

    return TryToggleDoorOnLevel (CurrentCellX, CurrentCellY, FrontEdge);
}

bool AGrimrockPartyPawn::TryStartMove (EGridEdge MoveDirection)
{
    if (bIsMoving || bIsTurning || !HasLevelRuntimeActor ())
    {
        return false;
    }

    if (!CanMoveOnLevel (CurrentCellX, CurrentCellY, MoveDirection))
    {
        return false;
    }

    int32 NextX = CurrentCellX;
    int32 NextY = CurrentCellY;

    if (!TryGetNeighborOnLevel (CurrentCellX, CurrentCellY, MoveDirection, NextX, NextY))
    {
        return false;
    }

    MoveStartLocation = GetActorLocation ();
    MoveTargetLocation = GetCellCenterOnLevel (NextX, NextY, EyeHeight);
    MoveElapsed = 0.f;
    bIsMoving = true;

    MoveStartCellX = CurrentCellX;
    MoveStartCellY = CurrentCellY;

    CurrentCellX = NextX;
    CurrentCellY = NextY;
    ActiveMoveDirection = MoveDirection;

    return true;
}

bool AGrimrockPartyPawn::TryStartTurn (bool bTurnRight)
{
    if (bIsMoving || bIsTurning)
    {
        return false;
    }

    TurnStartYaw = GetActorRotation ().Yaw;

    Facing = bTurnRight ? GridDirectionUtils::RotateRight (Facing) : GridDirectionUtils::RotateLeft (Facing);
    TurnTargetYaw = GridDirectionUtils::ToYaw (Facing);

    TurnDeltaYaw = FMath::FindDeltaAngleDegrees (TurnStartYaw, TurnTargetYaw);

    TurnElapsed = 0.f;
    bIsTurning = true;
    return true;
}

void AGrimrockPartyPawn::UpdateMove (float DeltaSeconds)
{
    const float SafeDuration = FMath::Max (0.01f, MoveDuration);

    MoveElapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp (MoveElapsed / SafeDuration, 0.f, 1.f);

    const FVector NewLocation = FMath::Lerp (MoveStartLocation, MoveTargetLocation, Alpha);
    SetActorLocation (NewLocation);

    if (Alpha >= 1.f)
    {
        SetActorLocation (MoveTargetLocation);
        bIsMoving = false;
        MoveElapsed = 0.f;
        ActiveMoveDirection = EGridEdge::None;
        if (LevelRuntimeActor)
        {
            LevelRuntimeActor->HandlePartyCellChanged (
                MoveStartCellX,
                MoveStartCellY,
                CurrentCellX,
                CurrentCellY);
            LevelRuntimeActor->TryExecuteTransitionAtCell (CurrentCellX, CurrentCellY, this, false);
        }
    }
}

void AGrimrockPartyPawn::UpdateTurn (float DeltaSeconds)
{
    const float SafeDuration = FMath::Max (0.01f, TurnDuration);

    TurnElapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp (TurnElapsed / SafeDuration, 0.f, 1.f);

    const float NewYaw = TurnStartYaw + (TurnDeltaYaw * Alpha);

    FRotator Rot = GetActorRotation ();
    Rot.Yaw = NewYaw;
    SetActorRotation (Rot);

    if (Alpha >= 1.f)
    {
        Rot.Yaw = TurnTargetYaw;
        SetActorRotation (Rot);

        bIsTurning = false;
        TurnElapsed = 0.f;
        TurnDeltaYaw = 0.f;
    }
}

bool AGrimrockPartyPawn::HasLevelRuntimeActor () const
{
    return LevelRuntimeActor != nullptr;
}

bool AGrimrockPartyPawn::CanMoveOnLevel (int32 FromX, int32 FromY, EGridEdge Direction) const
{
    return LevelRuntimeActor && LevelRuntimeActor->CanMove (FromX, FromY, Direction);
}

bool AGrimrockPartyPawn::TryGetNeighborOnLevel (
    int32 X,
    int32 Y,
    EGridEdge Direction,
    int32& OutX,
    int32& OutY) const
{
    if (!LevelRuntimeActor)
    {
        OutX = X;
        OutY = Y;
        return false;
    }

    return LevelRuntimeActor->TryGetNeighborCell (X, Y, Direction, OutX, OutY);
}

FVector AGrimrockPartyPawn::GetCellCenterOnLevel (int32 X, int32 Y, float ZOffset) const
{
    if (!LevelRuntimeActor)
    {
        return GetActorLocation ();
    }

    return LevelRuntimeActor->GetCellCenterWorld (X, Y, ZOffset);
}

bool AGrimrockPartyPawn::TryToggleDoorOnLevel (int32 X, int32 Y, EGridEdge Edge)
{
    return LevelRuntimeActor && LevelRuntimeActor->ToggleDoorOnEdge (X, Y, Edge);
}

void AGrimrockPartyPawn::BufferMoveCommand (EGridEdge MoveDirection)
{
    if (!bEnableInputBuffer)
    {
        return;
    }

    BufferedCommandType = EBufferedCommandType::Move;
    BufferedMoveDirection = MoveDirection;
    bBufferedTurnRight = false;
    BufferedCommandAge = 0.f;
}

void AGrimrockPartyPawn::BufferTurnCommand (bool bTurnRight)
{
    if (!bEnableInputBuffer)
    {
        return;
    }

    BufferedCommandType = EBufferedCommandType::Turn;
    BufferedMoveDirection = EGridEdge::None;
    bBufferedTurnRight = bTurnRight;
    BufferedCommandAge = 0.f;
}

void AGrimrockPartyPawn::BufferUseCommand ()
{
    if (!bEnableInputBuffer)
    {
        return;
    }

    BufferedCommandType = EBufferedCommandType::Use;
    BufferedMoveDirection = EGridEdge::None;
    bBufferedTurnRight = false;
    BufferedCommandAge = 0.f;
}

void AGrimrockPartyPawn::ClearBufferedCommand ()
{
    BufferedCommandType = EBufferedCommandType::None;
    BufferedMoveDirection = EGridEdge::None;
    bBufferedTurnRight = false;
    BufferedCommandAge = 0.f;
}

bool AGrimrockPartyPawn::TryConsumeBufferedCommand ()
{
    if (BufferedCommandType == EBufferedCommandType::None)
    {
        return false;
    }

    const EBufferedCommandType CommandType = BufferedCommandType;
    const EGridEdge MoveDirection = BufferedMoveDirection;
    const bool bTurnRight = bBufferedTurnRight;

    ClearBufferedCommand ();

    switch (CommandType)
    {
        case EBufferedCommandType::Move:
            return TryStartMove (MoveDirection);

        case EBufferedCommandType::Turn:
            return TryStartTurn (bTurnRight);

        case EBufferedCommandType::Use:
            return TryUseFrontInteraction ();

        default:
            return false;
    }
}

bool AGrimrockPartyPawn::IsBusy () const
{
    return bIsMoving || bIsTurning;
}
void AGrimrockPartyPawn::UpdateHeadBob (float DeltaSeconds)
{
    if (!bEnableHeadBob)
    {
        TargetHeadBobOffset = FVector::ZeroVector;
        CurrentHeadBobOffset = FMath::VInterpTo (
            CurrentHeadBobOffset,
            FVector::ZeroVector,
            DeltaSeconds,
            HeadBobReturnSpeed
        );

        ApplyCameraOffsets ();
        return;
    }

    if (bIsMoving)
    {
        const float SafeDuration = FMath::Max (0.01f, MoveDuration);
        HeadBobAlpha = FMath::Clamp (MoveElapsed / SafeDuration, 0.f, 1.f);

        // Courbe simple type Grimrock : un seul "pas" par déplacement de case.
        const float VerticalCurve = FMath::Sin (HeadBobAlpha * PI);
        const float VerticalOffset = -VerticalCurve * HeadBobVerticalAmplitude;

        float HorizontalOffset = 0.f;

        if (bHeadBobStrafeSway)
        {
            const EGridEdge LeftDir = GridDirectionUtils::GetLeft (Facing);
            const EGridEdge RightDir = GridDirectionUtils::GetRight (Facing);

            if (ActiveMoveDirection == RightDir)
            {
                HorizontalOffset = VerticalCurve * HeadBobHorizontalAmplitude;
            } else if (ActiveMoveDirection == LeftDir)
            {
                HorizontalOffset = -VerticalCurve * HeadBobHorizontalAmplitude;
            }
        }

        TargetHeadBobOffset = FVector (0.f, HorizontalOffset, VerticalOffset);
    } else
    {
        HeadBobAlpha = 0.f;
        TargetHeadBobOffset = FVector::ZeroVector;
    }

    CurrentHeadBobOffset = FMath::VInterpTo (
        CurrentHeadBobOffset,
        TargetHeadBobOffset,
        DeltaSeconds,
        bIsMoving ? 18.f : HeadBobReturnSpeed
    );

    ApplyCameraOffsets ();
}

void AGrimrockPartyPawn::ApplyCameraOffsets ()
{
    if (!SpringArm)
    {
        return;
    }

    SpringArm->SetRelativeLocation (SpringArmBaseRelativeLocation + CurrentHeadBobOffset);
}

void AGrimrockPartyPawn::BeginFreeLook ()
{
    bIsFreeLooking = true;
}

void AGrimrockPartyPawn::EndFreeLook ()
{
    bIsFreeLooking = false;
}

void AGrimrockPartyPawn::UpdateFreeLook (float DeltaSeconds)
{
    if (!SpringArm)
    {
        return;
    }

    if (APlayerController* PC = Cast<APlayerController> (GetController ()))
    {
        if (bIsFreeLooking)
        {
            float MouseDeltaX = 0.f;
            float MouseDeltaY = 0.f;
            PC->GetInputMouseDelta (MouseDeltaX, MouseDeltaY);

            FreeLookYaw += MouseDeltaX * FreeLookSensitivityYaw;
            FreeLookPitch = FMath::Clamp (
                FreeLookPitch + (MouseDeltaY * FreeLookSensitivityPitch),
                -FreeLookPitchDownLimit,
                FreeLookPitchUpLimit
            );

            FreeLookYaw = FMath::Clamp (
                FreeLookYaw,
                -FreeLookYawLimit,
                FreeLookYawLimit
            );
        } else if (bEnableFreeLookRecentering)
        {
            FreeLookYaw = FMath::FInterpTo (FreeLookYaw, 0.f, DeltaSeconds, FreeLookRecenteringSpeed);
            FreeLookPitch = FMath::FInterpTo (FreeLookPitch, 0.f, DeltaSeconds, FreeLookRecenteringSpeed);

            if (FMath::Abs (FreeLookYaw) < 0.01f)
            {
                FreeLookYaw = 0.f;
            }

            if (FMath::Abs (FreeLookPitch) < 0.01f)
            {
                FreeLookPitch = 0.f;
            }
        }
    }

    ApplyFreeLookRotation ();
}

void AGrimrockPartyPawn::ApplyFreeLookRotation ()
{
    if (!SpringArm)
    {
        return;
    }

    SpringArm->SetRelativeRotation (FRotator (FreeLookPitch, FreeLookYaw, 0.f));
}

bool AGrimrockPartyPawn::TryInteractOnLevel (int32 X, int32 Y, EGridEdge Edge)
{
    return LevelRuntimeActor && LevelRuntimeActor->TryInteractAtEdge (X, Y, Edge, this);
}

bool AGrimrockPartyPawn::HasInventoryItem (FName ItemDefinitionId) const
{
    return PartyInventoryComponent &&
        PartyInventoryComponent->HasItemDefinitionInSelectedCharacterInventory (ItemDefinitionId);
}

bool AGrimrockPartyPawn::CanAddItemInstanceToSelectedCharacterInventory (const FGridItemInstance& ItemInstance) const
{
    if (!PartyInventoryComponent)
    {
        return false;
    }

    FGridItemInstance InventoryItem = ItemInstance;
    PartyInventoryComponent->ApplyItemDefinitionToInstance (InventoryItem);
    return PartyInventoryComponent->CanAddItemToSelectedCharacterInventory (InventoryItem);
}

bool AGrimrockPartyPawn::AddItemInstanceToSelectedCharacterInventory (const FGridItemInstance& ItemInstance)
{
    if (!PartyInventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Pickup Failed NoPartyInventoryComponent Item=%s RuntimeId=%s"),
            *ItemInstance.ItemDefinitionId.ToString (),
            *ItemInstance.RuntimeObjectId.ToString ());
        return false;
    }

    PartyInventoryComponent->InitializeDefaultPartyIfNeeded ();

    FGridItemInstance InventoryItem = ItemInstance;
    PartyInventoryComponent->ApplyItemDefinitionToInstance (InventoryItem);
    InventoryItem.OwnerType = EGridItemOwnerType::CharacterInventory;
    InventoryItem.OwnerCharacterIndex = PartyInventoryComponent->GetSelectedCharacterIndex ();
    InventoryItem.EquipmentSlot = EGridEquipmentSlot::None;

    const bool bAdded = PartyInventoryComponent->AddItemToSelectedCharacterInventory (InventoryItem);
    UE_LOG (LogTemp, Log, TEXT ("GridInventory Pickup AddedToSelectedCharacter Item=%s RuntimeId=%s CharacterIndex=%d Result=%s"),
        *InventoryItem.ItemDefinitionId.ToString (),
        *InventoryItem.RuntimeObjectId.ToString (),
        PartyInventoryComponent->GetSelectedCharacterIndex (),
        bAdded ? TEXT ("true") : TEXT ("false"));

    if (!bAdded)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Pickup Failed InventoryFull Item=%s RuntimeId=%s"),
            *InventoryItem.ItemDefinitionId.ToString (),
            *InventoryItem.RuntimeObjectId.ToString ());
        return false;
    }

    return true;
}

bool AGrimrockPartyPawn::AddRuntimeItemToSelectedCharacterInventory (
    const FGuid& RuntimeObjectId,
    FName ItemDefinitionId,
    float Weight,
    int32 Quantity,
    bool bLightsEnabled)
{
    FGridItemInstance ItemInstance;
    ItemInstance.RuntimeObjectId = RuntimeObjectId;
    ItemInstance.ItemDefinitionId = ItemDefinitionId;
    ItemInstance.Quantity = Quantity;
    ItemInstance.Weight = Weight;
    ItemInstance.bLightsEnabled = bLightsEnabled;
    return AddItemInstanceToSelectedCharacterInventory (ItemInstance);
}

void AGrimrockPartyPawn::LogPartyInventoryDiagnostics () const
{
    if (PartyInventoryComponent)
    {
        PartyInventoryComponent->LogPartyInventoryDiagnostics ();
        return;
    }

    UE_LOG (LogTemp, Warning, TEXT ("PartyInventoryComponent is null on %s"), *GetName ());
}

void AGrimrockPartyPawn::LogItemDefinitionDiagnostics () const
{
    if (PartyInventoryComponent)
    {
        PartyInventoryComponent->LogItemDefinitionDiagnostics ();
        return;
    }

    UE_LOG (LogTemp, Warning, TEXT ("PartyInventoryComponent is null on %s"), *GetName ());
}

bool AGrimrockPartyPawn::EquipSelectedCharacterItemFromInventorySlot (
    int32 InventorySlotIndex,
    EGridEquipmentSlot TargetSlot)
{
    if (!PartyInventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Equip Failed Pawn=%s Reason=NoPartyInventoryComponent"), *GetName ());
        return false;
    }

    const bool bEquipped = PartyInventoryComponent->EquipItemFromInventorySlot (
        PartyInventoryComponent->GetSelectedCharacterIndex (),
        InventorySlotIndex,
        TargetSlot);

    if (bEquipped && IsHandEquipmentSlot (TargetSlot))
    {
        SyncHeldVisualFromSelectedCharacterEquipment ();
    }
    return bEquipped;
}

bool AGrimrockPartyPawn::UnequipSelectedCharacterItemToInventory (EGridEquipmentSlot SourceSlot)
{
    if (!PartyInventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Unequip Failed Pawn=%s Reason=NoPartyInventoryComponent"), *GetName ());
        return false;
    }

    const int32 CharacterIndex = PartyInventoryComponent->GetSelectedCharacterIndex ();
    FGridItemInstance PreviouslyEquippedItem;
    const bool bHadHandItem = IsHandEquipmentSlot (SourceSlot) &&
        PartyInventoryComponent->GetEquippedItem (CharacterIndex, SourceSlot, PreviouslyEquippedItem);

    const bool bUnequipped = PartyInventoryComponent->UnequipItemToInventory (CharacterIndex, SourceSlot);
    if (bUnequipped && IsHandEquipmentSlot (SourceSlot))
    {
        if (bHadHandItem && PreviouslyEquippedItem.ItemDefinitionId == GetHeldItemDefinitionId ())
        {
            ClearHeldItem ();
            UE_LOG (LogTemp, Log, TEXT ("GridInventory HeldVisual Clear Unequipped Character=%d Slot=%s Item=%s"),
                CharacterIndex,
                GetPawnEquipmentSlotName (SourceSlot),
                *PreviouslyEquippedItem.ItemDefinitionId.ToString ());
        }

        SyncHeldVisualFromSelectedCharacterEquipment ();
    }

    return bUnequipped;
}

bool AGrimrockPartyPawn::TryTakeSelectedCharacterEquipmentSlotToCursor (EGridEquipmentSlot SourceSlot)
{
    if (!PartyInventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Take Equipment Relay Failed Pawn=%s Slot=%s Reason=NoPartyInventoryComponent"),
            *GetName (),
            GetPawnEquipmentSlotName (SourceSlot));
        return false;
    }

    const bool bTaken = PartyInventoryComponent->TryTakeSelectedCharacterEquipmentSlotToCursor (SourceSlot);
    if (bTaken)
    {
        SyncHeldVisualFromSelectedCharacterEquipment ();
    }

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Take Equipment Relay Pawn=%s Slot=%s Result=%s"),
        *GetName (),
        GetPawnEquipmentSlotName (SourceSlot),
        bTaken ? TEXT ("true") : TEXT ("false"));
    return bTaken;
}

bool AGrimrockPartyPawn::TryTakeSelectedCharacterMainHandToCursor ()
{
    return TryTakeSelectedCharacterEquipmentSlotToCursor (EGridEquipmentSlot::MainHand);
}

bool AGrimrockPartyPawn::TryTakeSelectedCharacterOffHandToCursor ()
{
    return TryTakeSelectedCharacterEquipmentSlotToCursor (EGridEquipmentSlot::OffHand);
}

bool AGrimrockPartyPawn::TryEquipCursorItemToSelectedCharacterSlot (EGridEquipmentSlot TargetSlot)
{
    if (!PartyInventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Equip Failed Pawn=%s Slot=%s Reason=NoPartyInventoryComponent"),
            *GetName (),
            GetPawnEquipmentSlotName (TargetSlot));
        return false;
    }

    const bool bEquipped = PartyInventoryComponent->TryEquipCursorItemToSelectedCharacterSlot (TargetSlot);
    if (bEquipped && IsHandEquipmentSlot (TargetSlot))
    {
        SyncHeldVisualFromSelectedCharacterEquipment ();
    }

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Equip Relay Pawn=%s Slot=%s Result=%s"),
        *GetName (),
        GetPawnEquipmentSlotName (TargetSlot),
        bEquipped ? TEXT ("true") : TEXT ("false"));
    return bEquipped;
}

bool AGrimrockPartyPawn::TryEquipCursorItemToSelectedCharacterMainHand ()
{
    return TryEquipCursorItemToSelectedCharacterSlot (EGridEquipmentSlot::MainHand);
}

bool AGrimrockPartyPawn::TryEquipCursorItemToSelectedCharacterOffHand ()
{
    return TryEquipCursorItemToSelectedCharacterSlot (EGridEquipmentSlot::OffHand);
}

bool AGrimrockPartyPawn::DebugTakeInventorySlotToCursor (int32 CharacterIndex, int32 InventorySlotIndex)
{
    if (!PartyInventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Take Relay Failed Pawn=%s Reason=NoPartyInventoryComponent"), *GetName ());
        return false;
    }

    const bool bTaken = PartyInventoryComponent->TryTakeInventorySlotToCursor (CharacterIndex, InventorySlotIndex);
    UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Take Relay Pawn=%s Character=%d Slot=%d Result=%s"),
        *GetName (),
        CharacterIndex,
        InventorySlotIndex,
        bTaken ? TEXT ("true") : TEXT ("false"));
    return bTaken;
}

bool AGrimrockPartyPawn::DebugPlaceCursorItemInSelectedInventory ()
{
    if (!PartyInventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place Relay Failed Pawn=%s Reason=NoPartyInventoryComponent"), *GetName ());
        return false;
    }

    const bool bPlaced = PartyInventoryComponent->TryPlaceCursorItemInSelectedCharacterInventory ();
    UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Place Relay Pawn=%s Result=%s"),
        *GetName (),
        bPlaced ? TEXT ("true") : TEXT ("false"));
    return bPlaced;
}

bool AGrimrockPartyPawn::TryPlaceCursorItemInReceptacle (AGridReceptacleActor* ReceptacleActor)
{
    if (!PartyInventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place ToReceptacle Failed Item=None Receptacle=%s Reason=NoPartyInventoryComponent"),
            ReceptacleActor ? *ReceptacleActor->GetName () : TEXT ("None"));
        return false;
    }

    if (!ReceptacleActor)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place ToReceptacle Failed Item=%s Receptacle=None Reason=NoReceptacle"),
            PartyInventoryComponent->HasCursorItem () ? *PartyInventoryComponent->GetCursorItem ().ItemDefinitionId.ToString () : TEXT ("None"));
        return false;
    }

    if (!PartyInventoryComponent->HasCursorItem ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place ToReceptacle Failed Item=None Receptacle=%s Reason=NoCursorItem"),
            *ReceptacleActor->GetName ());
        return false;
    }

    const FGridItemInstance CursorItem = PartyInventoryComponent->GetCursorItem ();
    FGridItemInstance AcceptedItem;
    if (!ReceptacleActor->TryInsertItemInstanceFromCursor (CursorItem, AcceptedItem))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place ToReceptacle Failed Item=%s RuntimeId=%s Receptacle=%s Reason=ReceptacleRejected"),
            *CursorItem.ItemDefinitionId.ToString (),
            *CursorItem.RuntimeObjectId.ToString (),
            *ReceptacleActor->GetName ());
        return false;
    }

    PartyInventoryComponent->ClearCursorItem ();
    UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Cleared AfterReceptacle Item=%s RuntimeId=%s Receptacle=%s"),
        *AcceptedItem.ItemDefinitionId.ToString (),
        *AcceptedItem.RuntimeObjectId.ToString (),
        *ReceptacleActor->GetName ());
    PartyInventoryComponent->LogInventoryOwnershipDiagnostics ();

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Place ToReceptacle Item=%s RuntimeId=%s Receptacle=%s Result=true"),
        *AcceptedItem.ItemDefinitionId.ToString (),
        *AcceptedItem.RuntimeObjectId.ToString (),
        *ReceptacleActor->GetName ());
    return true;
}

bool AGrimrockPartyPawn::DebugPlaceCursorItemInFrontReceptacle ()
{
    if (!PartyInventoryComponent)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridInventory Cursor DebugPlaceInFront Failed Cell=(%d,%d) Edge=%d Item=None Receptacle=None Reason=NoPartyInventoryComponent"),
            CurrentCellX,
            CurrentCellY,
            static_cast<int32> (Facing));
        return false;
    }

    if (!LevelRuntimeActor)
    {
        LevelRuntimeActor = Cast<AGridLevelRuntimeActor> (
            UGameplayStatics::GetActorOfClass (GetWorld (), AGridLevelRuntimeActor::StaticClass ())
        );
    }

    const FString CursorItemText = PartyInventoryComponent->HasCursorItem ()
        ? PartyInventoryComponent->GetCursorItem ().ItemDefinitionId.ToString ()
        : FString (TEXT ("None"));

    if (!LevelRuntimeActor)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridInventory Cursor DebugPlaceInFront Failed Cell=(%d,%d) Edge=%d Item=%s Receptacle=None Reason=NoLevelRuntimeActor"),
            CurrentCellX,
            CurrentCellY,
            static_cast<int32> (Facing),
            *CursorItemText);
        return false;
    }

    AGridReceptacleActor* ReceptacleActor = LevelRuntimeActor->FindReceptacleAtEdge (CurrentCellX, CurrentCellY, Facing);
    UE_LOG (LogTemp, Log,
        TEXT ("GridInventory Cursor DebugPlaceInFront Cell=(%d,%d) Edge=%d Item=%s Receptacle=%s"),
        CurrentCellX,
        CurrentCellY,
        static_cast<int32> (Facing),
        *CursorItemText,
        *GetNameSafe (ReceptacleActor));

    if (!ReceptacleActor)
    {
        return false;
    }

    return TryPlaceCursorItemInReceptacle (ReceptacleActor);
}

bool AGrimrockPartyPawn::EquipHeldItem (FName ItemDefinitionId)
{
    if (ItemDefinitionId.IsNone ())
    {
        return false;
    }

    const bool bUseHeldTorchClass = ItemDefinitionId == DefaultHeldItemDefinitionId && HeldTorchActorClass;

    if (!bUseHeldTorchClass && !LevelRuntimeActor)
    {
        LevelRuntimeActor = Cast<AGridLevelRuntimeActor> (
            UGameplayStatics::GetActorOfClass (GetWorld (), AGridLevelRuntimeActor::StaticClass ())
        );
    }

    if (!bUseHeldTorchClass && !LevelRuntimeActor)
    {
        UE_LOG (LogTemp, Warning, TEXT ("Held item equip failed: no AGridLevelRuntimeActor found for %s."), *ItemDefinitionId.ToString ());
        return false;
    }

    ClearHeldItem ();

    USceneComponent* AttachParent = HeldItemRoot ? HeldItemRoot.Get () : GetRootComponent ();
    if (bUseHeldTorchClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = GetInstigator ();

        HeldItemActor = GetWorld ()->SpawnActor<AGridItemActor> (HeldTorchActorClass, FTransform::Identity, SpawnParams);
        if (HeldItemActor)
        {
            HeldItemActor->ArchetypeId = ItemDefinitionId;
        }
    }
    else
    {
        HeldItemActor = LevelRuntimeActor->SpawnItemActorForDefinition (nullptr, ItemDefinitionId, this, AttachParent);
    }

    if (!HeldItemActor)
    {
        UE_LOG (LogTemp, Warning, TEXT ("Held item equip failed: could not spawn item definition %s."), *ItemDefinitionId.ToString ());
        return false;
    }

    HeldItemActor->AttachToComponent (AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    HeldItemActor->SetActorRelativeLocation (HeldItemRelativeLocation);
    HeldItemActor->SetActorRelativeRotation (HeldItemRelativeRotation);
    HeldItemActor->SetActorRelativeScale3D (HeldItemRelativeScale);
    HeldItemActor->OnPlacedInWorld ();
    HeldItemDefinitionId = ItemDefinitionId;
    bHasTorchInHand = ItemDefinitionId == DefaultHeldItemDefinitionId;

    UE_LOG (LogTemp, Log, TEXT ("Held item equipped: %s"), *ItemDefinitionId.ToString ());
    return true;
}

void AGrimrockPartyPawn::ClearHeldItem ()
{
    if (HeldItemActor)
    {
        HeldItemActor->OnRemovedFromWorld ();
        HeldItemActor->Destroy ();
        HeldItemActor = nullptr;
    }

    HeldItemDefinitionId = NAME_None;
    bHasTorchInHand = false;
}

FName AGrimrockPartyPawn::GetHeldItemDefinitionId () const
{
    return HeldItemActor ? HeldItemDefinitionId : NAME_None;
}

bool AGrimrockPartyPawn::IsHoldingItem (FName ItemDefinitionId) const
{
    return !ItemDefinitionId.IsNone () && GetHeldItemDefinitionId () == ItemDefinitionId;
}

void AGrimrockPartyPawn::SyncHeldVisualFromSelectedCharacterEquipment ()
{
    // TODO 5C: call this after any direct PartyInventoryComponent::SetSelectedCharacterIndex usage outside the pawn.
    if (!PartyInventoryComponent)
    {
        ClearHeldItem ();
        return;
    }

    const int32 CharacterIndex = PartyInventoryComponent->GetSelectedCharacterIndex ();
    FGridItemInstance EquippedItem;
    EGridEquipmentSlot EquippedSlot = EGridEquipmentSlot::None;

    if (PartyInventoryComponent->GetEquippedItem (CharacterIndex, EGridEquipmentSlot::MainHand, EquippedItem))
    {
        EquippedSlot = EGridEquipmentSlot::MainHand;
    }
    else if (PartyInventoryComponent->GetEquippedItem (CharacterIndex, EGridEquipmentSlot::OffHand, EquippedItem))
    {
        EquippedSlot = EGridEquipmentSlot::OffHand;
    }

    if (EquippedSlot == EGridEquipmentSlot::None)
    {
        ClearHeldItem ();
        return;
    }

    if (GetHeldItemDefinitionId () != EquippedItem.ItemDefinitionId &&
        !EquipHeldItem (EquippedItem.ItemDefinitionId))
    {
        return;
    }

    UE_LOG (LogTemp, Log, TEXT ("GridInventory HeldVisual Sync Equipped Character=%d Slot=%s Item=%s"),
        CharacterIndex,
        GetPawnEquipmentSlotName (EquippedSlot),
        *EquippedItem.ItemDefinitionId.ToString ());
}
