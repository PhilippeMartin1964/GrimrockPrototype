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
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPlayerController.h"
#include "Runtime/GridReceptacleActor.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridInventoryWidget.h"
#include "UI/GrimrockMenuWidget.h"
#include "UI/RPGCharacterCreationWidget.h"

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

    bool bLoadedSavedGame = false;
    if (PartyInventoryComponent)
    {
        if (PartyStartupMode == EGrimrockPartyStartupMode::NewGame)
        {
            if (HasCurrentSave ())
            {
                UGameplayStatics::DeleteGameInSlot (PartySaveSlotName, PartySaveUserIndex);
            }
            PartyInventoryComponent->ResetPartyForNewGame ();
        }
        else if (HasCurrentSave ())
        {
            FText LoadError;
            bLoadedSavedGame = LoadCurrentGameData (LoadError, true);
            if (!bLoadedSavedGame)
            {
                UE_LOG (
                    LogTemp,
                    Warning,
                    TEXT ("PartySave Load Failed Slot=%s Reason=%s"),
                    *PartySaveSlotName,
                    *LoadError.ToString ());
                PartyInventoryComponent->ResetPartyForNewGame ();
            }
        }
        else
        {
            PartyInventoryComponent->ResetPartyForNewGame ();
        }
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

    if (bLoadedSavedGame)
    {
        UE_LOG (
            LogTemp,
            Log,
            TEXT ("PartySave Continued Slot=%s CharacterCount=%d"),
            *PartySaveSlotName,
            PartyInventoryComponent ? PartyInventoryComponent->GetActiveCharacterCount () : 0);
    }

    if (PartyInventoryComponent && !PartyInventoryComponent->HasCompletedInitialCharacterCreation ())
    {
        ShowInitialCharacterCreationWidget ();
    }
}

void AGrimrockPartyPawn::EndPlay (const EEndPlayReason::Type EndPlayReason)
{
    if (PartyInventoryComponent && PartyInventoryComponent->HasCompletedInitialCharacterCreation ())
    {
        FText SaveError;
        if (!SaveCurrentGame (SaveError))
        {
            UE_LOG (
                LogTemp,
                Warning,
                TEXT ("PartySave EndPlay Failed Slot=%s Reason=%s"),
                *PartySaveSlotName,
                *SaveError.ToString ());
        }
    }

    Super::EndPlay (EndPlayReason);
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
    PlayerInputComponent->BindKey (EKeys::I, IE_Pressed, this, &AGrimrockPartyPawn::ToggleInventoryWidget);
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

    if (bCharacterCreationModalActive)
    {
        return;
    }
    DismissReadableMessageIfVisible ();

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

    if (bCharacterCreationModalActive)
    {
        return;
    }
    DismissReadableMessageIfVisible ();

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

    if (bCharacterCreationModalActive)
    {
        return;
    }
    DismissReadableMessageIfVisible ();

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

    if (bCharacterCreationModalActive)
    {
        return;
    }
    DismissReadableMessageIfVisible ();

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

    if (bCharacterCreationModalActive)
    {
        return;
    }
    DismissReadableMessageIfVisible ();

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

    if (bCharacterCreationModalActive)
    {
        return;
    }
    DismissReadableMessageIfVisible ();

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

    if (bCharacterCreationModalActive)
    {
        return;
    }

    if (IsBusy ())
    {
        BufferUseCommand ();
        return;
    }

    TryUseFrontInteraction ();
}

bool AGrimrockPartyPawn::TryUseFrontInteraction ()
{
    if (bCharacterCreationModalActive || bIsMoving || bIsTurning || !HasLevelRuntimeActor ())
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
    if (bCharacterCreationModalActive || bIsMoving || bIsTurning || !HasLevelRuntimeActor ())
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
    if (bCharacterCreationModalActive || bIsMoving || bIsTurning)
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

bool AGrimrockPartyPawn::DismissReadableMessageIfVisible ()
{
    return LevelRuntimeActor &&
        LevelRuntimeActor->DismissReadableMessage ();
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

    if (LevelRuntimeActor && !ItemInstance.ItemDefinitionId.IsNone ())
    {
        if (UGridItemDefinitionAsset* RuntimeDefinition =
            LevelRuntimeActor->ResolveRuntimeItemDefinition (ItemInstance.ItemDefinitionId))
        {
            PartyInventoryComponent->RegisterItemDefinition (RuntimeDefinition);
        }
    }

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

void AGrimrockPartyPawn::ToggleInventoryWidget ()
{
    if (bCharacterCreationModalActive)
    {
        return;
    }

    if (bInventoryWidgetVisible)
    {
        HideInventoryWidget ();
    }
    else
    {
        ShowInventoryWidget ();
    }
}

void AGrimrockPartyPawn::ShowInventoryWidget ()
{
    if (bCharacterCreationModalActive)
    {
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController> (GetController ());
    if (!PlayerController)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI Show Failed Pawn=%s Reason=NoPlayerController"), *GetName ());
        return;
    }

    if (!MenuWidgetClass)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GrimrockMenu UI Show Failed Pawn=%s Reason=NoMenuWidgetClass"), *GetName ());
        return;
    }

    if (!MenuWidgetInstance)
    {
        MenuWidgetInstance = CreateWidget<UGrimrockMenuWidget> (PlayerController, MenuWidgetClass);
        if (MenuWidgetInstance)
        {
            MenuWidgetInstance->InitializeMenuWidget (this);
        }
    }

    if (!MenuWidgetInstance)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GrimrockMenu UI Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName ());
        return;
    }

    if (!MenuWidgetInstance->IsInViewport ())
    {
        MenuWidgetInstance->AddToViewport (100);
    }
    MenuWidgetInstance->SetVisibility (ESlateVisibility::Visible);
    MenuWidgetInstance->RefreshInventory ();
    bInventoryWidgetVisible = true;

    PlayerController->bEnableClickEvents = true;
    PlayerController->bEnableMouseOverEvents = true;
    PlayerController->bShowMouseCursor = true;
    PlayerController->DefaultMouseCursor = EMouseCursor::Default;
    PlayerController->CurrentMouseCursor = EMouseCursor::Default;
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus (MenuWidgetInstance->TakeWidget ());
    InputMode.SetLockMouseToViewportBehavior (EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture (false);
    PlayerController->SetInputMode (InputMode);
    PlayerController->bShowMouseCursor = true;
    PlayerController->DefaultMouseCursor = EMouseCursor::Default;
    PlayerController->CurrentMouseCursor = EMouseCursor::Default;

    if (AGrimrockPlayerController* GrimrockPlayerController = Cast<AGrimrockPlayerController> (PlayerController))
    {
        GrimrockPlayerController->SetInventoryUiOpen (true);
    }

    UE_LOG (LogTemp, Log, TEXT ("GrimrockMenu UI Shown Pawn=%s"), *GetName ());
}

void AGrimrockPartyPawn::HideInventoryWidget ()
{
    if (MenuWidgetInstance)
    {
        MenuWidgetInstance->SetVisibility (ESlateVisibility::Collapsed);
    }
    bInventoryWidgetVisible = false;

    if (APlayerController* PlayerController = Cast<APlayerController> (GetController ()))
    {
        if (AGrimrockPlayerController* GrimrockPlayerController = Cast<AGrimrockPlayerController> (PlayerController))
        {
            GrimrockPlayerController->SetInventoryUiOpen (false);
        }
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture (false);
        PlayerController->SetInputMode (InputMode);
        PlayerController->bShowMouseCursor = true;
        PlayerController->DefaultMouseCursor = EMouseCursor::Default;
        PlayerController->CurrentMouseCursor = EMouseCursor::Default;
    }

    if (bAutoSaveOnInventoryClose &&
        PartyInventoryComponent &&
        PartyInventoryComponent->HasCompletedInitialCharacterCreation ())
    {
        FText SaveError;
        if (!SaveCurrentGame (SaveError))
        {
            UE_LOG (
                LogTemp,
                Warning,
                TEXT ("PartySave InventoryClose Failed Slot=%s Reason=%s"),
                *PartySaveSlotName,
                *SaveError.ToString ());
        }
    }

    UE_LOG (LogTemp, Log, TEXT ("GrimrockMenu UI Hidden Pawn=%s"), *GetName ());
}

UGridInventoryWidget* AGrimrockPartyPawn::GetInventoryWidget () const
{
    return MenuWidgetInstance ? MenuWidgetInstance->GetInventoryWidget () : nullptr;
}

void AGrimrockPartyPawn::CloseCharacterCreationWidget ()
{
    if (CharacterCreationWidgetInstance)
    {
        CharacterCreationWidgetInstance->RemoveFromParent ();
        CharacterCreationWidgetInstance = nullptr;
    }
}

void AGrimrockPartyPawn::ApplyCharacterCreationInputMode (bool bIsActive)
{
    APlayerController* PlayerController = Cast<APlayerController> (GetController ());
    if (!PlayerController)
    {
        return;
    }

    if (AGrimrockPlayerController* GrimrockPlayerController = Cast<AGrimrockPlayerController> (PlayerController))
    {
        GrimrockPlayerController->SetInventoryUiOpen (bIsActive);
    }

    PlayerController->bEnableClickEvents = true;
    PlayerController->bEnableMouseOverEvents = true;
    PlayerController->bShowMouseCursor = true;
    PlayerController->DefaultMouseCursor = EMouseCursor::Default;
    PlayerController->CurrentMouseCursor = EMouseCursor::Default;

    if (bIsActive)
    {
        FInputModeUIOnly InputMode;
        InputMode.SetLockMouseToViewportBehavior (EMouseLockMode::DoNotLock);
        PlayerController->SetInputMode (InputMode);

        if (CharacterCreationWidgetInstance)
        {
            CharacterCreationWidgetInstance->FocusNameInput ();
        }
        return;
    }

    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior (EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture (false);
    PlayerController->SetInputMode (InputMode);
}

void AGrimrockPartyPawn::ShowInitialCharacterCreationWidget ()
{
    if (!PartyInventoryComponent || PartyInventoryComponent->HasCompletedInitialCharacterCreation ())
    {
        bCharacterCreationModalActive = false;
        return;
    }

    bCharacterCreationModalActive = true;
    ClearBufferedCommand ();

    if (bInventoryWidgetVisible)
    {
        HideInventoryWidget ();
    }

    APlayerController* PlayerController = Cast<APlayerController> (GetController ());
    if (!PlayerController)
    {
        UE_LOG (LogTemp, Error, TEXT ("CharacterCreation UI Show Failed Pawn=%s Reason=NoPlayerController"), *GetName ());
        return;
    }

    ApplyCharacterCreationInputMode (true);

    if (!CharacterCreationWidgetClass)
    {
        UE_LOG (LogTemp, Error, TEXT ("CharacterCreation UI Show Failed Pawn=%s Reason=NoWidgetClass"), *GetName ());
        return;
    }

    if (!CharacterCreationWidgetInstance)
    {
        CharacterCreationWidgetInstance = CreateWidget<URPGCharacterCreationWidget> (
            PlayerController,
            CharacterCreationWidgetClass);
    }

    if (!CharacterCreationWidgetInstance)
    {
        UE_LOG (LogTemp, Error, TEXT ("CharacterCreation UI Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName ());
        return;
    }

    CharacterCreationWidgetInstance->InitializeCharacterCreationWidget (this);
    if (!CharacterCreationWidgetInstance->IsInViewport ())
    {
        CharacterCreationWidgetInstance->AddToViewport (1000);
    }
    CharacterCreationWidgetInstance->SetVisibility (ESlateVisibility::Visible);
    CharacterCreationWidgetInstance->FocusNameInput ();

    UE_LOG (LogTemp, Log, TEXT ("CharacterCreation UI Shown Pawn=%s"), *GetName ());
}

void AGrimrockPartyPawn::HandleInitialCharacterCreated ()
{
    if (!PartyInventoryComponent || !PartyInventoryComponent->HasCompletedInitialCharacterCreation ())
    {
        return;
    }

    CloseCharacterCreationWidget ();

    bCharacterCreationModalActive = false;
    ClearBufferedCommand ();
    SyncHeldVisualFromSelectedCharacterEquipment ();

    ApplyCharacterCreationInputMode (false);

    FText SaveError;
    if (!SaveCurrentGame (SaveError))
    {
        UE_LOG (
            LogTemp,
            Warning,
            TEXT ("PartySave InitialCharacter Failed Slot=%s Reason=%s"),
            *PartySaveSlotName,
            *SaveError.ToString ());
    }

    UE_LOG (LogTemp, Log, TEXT ("CharacterCreation Completed Pawn=%s"), *GetName ());
}

bool AGrimrockPartyPawn::IsCharacterCreationModalActive () const
{
    return bCharacterCreationModalActive;
}

bool AGrimrockPartyPawn::HasCurrentSave () const
{
    return !PartySaveSlotName.IsEmpty () &&
        UGameplayStatics::DoesSaveGameExist (PartySaveSlotName, PartySaveUserIndex);
}

bool AGrimrockPartyPawn::SaveCurrentGame (FText& OutError)
{
    OutError = FText::GetEmpty ();

    if (PartySaveSlotName.IsEmpty ())
    {
        OutError = FText::FromString (TEXT ("Le nom du slot de sauvegarde est vide."));
        return false;
    }

    if (!PartyInventoryComponent ||
        !PartyInventoryComponent->HasCompletedInitialCharacterCreation ())
    {
        OutError = FText::FromString (TEXT ("Aucun personnage finalisé ne peut être sauvegardé."));
        return false;
    }

    FString OwnershipError;
    if (!PartyInventoryComponent->ValidateInventoryOwnership (OwnershipError))
    {
        OutError = FText::FromString (
            FString::Printf (TEXT ("L'ownership du groupe est invalide : %s"), *OwnershipError));
        return false;
    }

    if (!IsValid (LevelRuntimeActor) || !LevelRuntimeActor->CaptureCurrentLevelRuntimeState ())
    {
        OutError = FText::FromString (TEXT ("L'état runtime du niveau ne peut pas être capturé."));
        return false;
    }

    UGrimrockPartySaveGame* SaveGame = Cast<UGrimrockPartySaveGame> (
        UGameplayStatics::CreateSaveGameObject (UGrimrockPartySaveGame::StaticClass ()));
    if (!SaveGame)
    {
        OutError = FText::FromString (TEXT ("L'objet de sauvegarde ne peut pas être créé."));
        return false;
    }

    SaveGame->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion;
    SaveGame->PartyInventoryState = PartyInventoryComponent->PartyInventoryState;
    SaveGame->DungeonRuntimeState = LevelRuntimeActor->DungeonRuntimeState;
    SaveGame->CurrentDungeonLevelId = LevelRuntimeActor->CurrentDungeonLevelId;
    SaveGame->PartyCellX = CurrentCellX;
    SaveGame->PartyCellY = CurrentCellY;
    SaveGame->PartyFacing = Facing;

    if (!UGameplayStatics::SaveGameToSlot (SaveGame, PartySaveSlotName, PartySaveUserIndex))
    {
        OutError = FText::FromString (TEXT ("L'écriture du fichier de sauvegarde a échoué."));
        return false;
    }

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("PartySave Saved Slot=%s Version=%d Characters=%d Cell=(%d,%d) Facing=%d"),
        *PartySaveSlotName,
        SaveGame->SaveVersion,
        SaveGame->PartyInventoryState.ActiveCharacters.Num (),
        CurrentCellX,
        CurrentCellY,
        static_cast<int32> (Facing));
    return true;
}

bool AGrimrockPartyPawn::LoadCurrentGameData (FText& OutError, bool bApplyDungeonState)
{
    OutError = FText::GetEmpty ();

    if (!HasCurrentSave ())
    {
        OutError = FText::FromString (TEXT ("Aucune sauvegarde n'est disponible."));
        return false;
    }

    UGrimrockPartySaveGame* SaveGame = Cast<UGrimrockPartySaveGame> (
        UGameplayStatics::LoadGameFromSlot (PartySaveSlotName, PartySaveUserIndex));
    if (!SaveGame)
    {
        OutError = FText::FromString (TEXT ("Le fichier ne contient pas une sauvegarde de groupe valide."));
        return false;
    }

    if (!SaveGame->IsCompatible ())
    {
        OutError = FText::FromString (
            FString::Printf (
                TEXT ("Version de sauvegarde incompatible : %d, version attendue : %d."),
                SaveGame->SaveVersion,
                UGrimrockPartySaveGame::CurrentSaveVersion));
        return false;
    }

    if (!PartyInventoryComponent || !IsValid (LevelRuntimeActor))
    {
        OutError = FText::FromString (TEXT ("Le groupe ou le niveau runtime est indisponible."));
        return false;
    }

    const FGridPartyInventoryState PreviousPartyState = PartyInventoryComponent->PartyInventoryState;
    const FGridDungeonRuntimeState PreviousDungeonState = LevelRuntimeActor->DungeonRuntimeState;
    const FName PreviousDungeonLevelId = LevelRuntimeActor->CurrentDungeonLevelId;
    const int32 PreviousCellX = CurrentCellX;
    const int32 PreviousCellY = CurrentCellY;
    const EGridEdge PreviousFacing = Facing;

    if (!PartyInventoryComponent->RestorePartyInventoryState (
        SaveGame->PartyInventoryState,
        OutError))
    {
        return false;
    }

    LevelRuntimeActor->DungeonRuntimeState = SaveGame->DungeonRuntimeState;
    LevelRuntimeActor->CurrentDungeonLevelId = SaveGame->CurrentDungeonLevelId;
    CurrentCellX = SaveGame->PartyCellX;
    CurrentCellY = SaveGame->PartyCellY;
    Facing = SaveGame->PartyFacing == EGridEdge::None ? EGridEdge::North : SaveGame->PartyFacing;

    if (bApplyDungeonState && !LevelRuntimeActor->ApplyCurrentLevelRuntimeState ())
    {
        PartyInventoryComponent->PartyInventoryState = PreviousPartyState;
        LevelRuntimeActor->DungeonRuntimeState = PreviousDungeonState;
        LevelRuntimeActor->CurrentDungeonLevelId = PreviousDungeonLevelId;
        CurrentCellX = PreviousCellX;
        CurrentCellY = PreviousCellY;
        Facing = PreviousFacing;
        OutError = FText::FromString (TEXT ("L'état runtime sauvegardé ne peut pas être appliqué au niveau."));
        return false;
    }

    return true;
}

bool AGrimrockPartyPawn::LoadCurrentGame (FText& OutError)
{
    if (!LoadCurrentGameData (OutError, true))
    {
        return false;
    }

    CloseCharacterCreationWidget ();
    bCharacterCreationModalActive = false;
    ClearBufferedCommand ();
    SnapToCurrentCell ();

    if (LevelRuntimeActor)
    {
        LevelRuntimeActor->HandlePartyCellChanged (
            CurrentCellX,
            CurrentCellY,
            CurrentCellX,
            CurrentCellY);
    }

    ApplyCharacterCreationInputMode (false);
    SyncHeldVisualFromSelectedCharacterEquipment ();

    if (MenuWidgetInstance)
    {
        MenuWidgetInstance->RefreshInventory ();
    }

    UE_LOG (LogTemp, Log, TEXT ("PartySave Loaded Slot=%s"), *PartySaveSlotName);
    return true;
}

bool AGrimrockPartyPawn::StartNewGame (FText& OutError)
{
    OutError = FText::GetEmpty ();

    if (HasCurrentSave () &&
        !UGameplayStatics::DeleteGameInSlot (PartySaveSlotName, PartySaveUserIndex))
    {
        OutError = FText::FromString (TEXT ("La sauvegarde existante ne peut pas être supprimée."));
        return false;
    }

    if (!PartyInventoryComponent)
    {
        OutError = FText::FromString (TEXT ("Le composant de groupe est indisponible."));
        return false;
    }

    PartyInventoryComponent->ResetPartyForNewGame ();

    if (bInventoryWidgetVisible)
    {
        HideInventoryWidget ();
    }

    CloseCharacterCreationWidget ();
    bCharacterCreationModalActive = false;
    ClearBufferedCommand ();
    ClearHeldItem ();

    if (LevelRuntimeActor)
    {
        LevelRuntimeActor->DungeonRuntimeState = FGridDungeonRuntimeState ();
        LevelRuntimeActor->RebuildRuntimeObjects ();

        if (LevelRuntimeActor->LevelAsset &&
            LevelRuntimeActor->LevelAsset->IsStartCellValid ())
        {
            CurrentCellX = LevelRuntimeActor->LevelAsset->StartCellX;
            CurrentCellY = LevelRuntimeActor->LevelAsset->StartCellY;
            Facing = LevelRuntimeActor->LevelAsset->StartFacing == EGridEdge::None
                ? EGridEdge::North
                : LevelRuntimeActor->LevelAsset->StartFacing;
            SnapToCurrentCell ();
        }
    }

    ShowInitialCharacterCreationWidget ();
    UE_LOG (LogTemp, Log, TEXT ("PartySave NewGame Slot=%s"), *PartySaveSlotName);
    return true;
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

bool AGrimrockPartyPawn::HasCursorItem () const
{
    return PartyInventoryComponent && PartyInventoryComponent->HasCursorItem ();
}

bool AGrimrockPartyPawn::GetCursorItem (FGridItemInstance& OutItem) const
{
    OutItem = FGridItemInstance ();
    if (!PartyInventoryComponent || !PartyInventoryComponent->HasCursorItem ())
    {
        return false;
    }

    OutItem = PartyInventoryComponent->GetCursorItem ();
    return true;
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

    if (!LevelRuntimeActor || !LevelRuntimeActor->CanPartyInteractWithEdgeObject (
        ReceptacleActor->CellX,
        ReceptacleActor->CellY,
        ReceptacleActor->Edge,
        this))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridInventory Cursor Place ToReceptacle Failed Item=%s Receptacle=%s Reason=EdgeNotFacingParty"),
            *PartyInventoryComponent->GetCursorItem ().ItemDefinitionId.ToString (),
            *ReceptacleActor->GetName ());
        return false;
    }

    const FGridItemInstance CursorItem = PartyInventoryComponent->GetCursorItem ();
    FGridItemInstance SingleItem;
    if (!BuildSingleItemInstanceFromCursor (SingleItem))
    {
        return false;
    }

    FGridItemInstance AcceptedItem;
    if (!ReceptacleActor->TryInsertItemInstanceFromCursor (SingleItem, AcceptedItem))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place ToReceptacle Failed Item=%s RuntimeId=%s Receptacle=%s Reason=ReceptacleRejected"),
            *CursorItem.ItemDefinitionId.ToString (),
            *CursorItem.RuntimeObjectId.ToString (),
            *ReceptacleActor->GetName ());
        return false;
    }

    ConsumeOneCursorItemAfterSuccessfulAction ();
    UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor ConsumedOne AfterReceptacle Item=%s RuntimeId=%s Receptacle=%s"),
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

bool AGrimrockPartyPawn::TryDropCursorItemAtCell (
    int32 CellX,
    int32 CellY,
    EGridEdge Edge,
    const FVector& LocalOffset)
{
    if (!PartyInventoryComponent || !PartyInventoryComponent->HasCursorItem () || !LevelRuntimeActor)
    {
        return false;
    }

    FGridItemInstance DroppedItem;
    if (!BuildSingleItemInstanceFromCursor (DroppedItem) ||
        !LevelRuntimeActor->TryDropItemInstanceAtCell (DroppedItem, CellX, CellY, Edge, LocalOffset))
    {
        return false;
    }

    ConsumeOneCursorItemAfterSuccessfulAction ();
    PartyInventoryComponent->LogInventoryOwnershipDiagnostics ();
    return true;
}

bool AGrimrockPartyPawn::TryThrowOneCursorItem (
    const FVector& LaunchDirection,
    EGridItemThrowMode ThrowMode)
{
    if (!PartyInventoryComponent ||
        !LevelRuntimeActor ||
        !PartyInventoryComponent->HasCursorItem ())
    {
        return false;
    }

    const FGridItemInstance CursorItem = PartyInventoryComponent->GetCursorItem ();
    UGridItemDefinitionAsset* ItemDefinition =
        LevelRuntimeActor->ResolveRuntimeItemDefinition (CursorItem.ItemDefinitionId);
    if (!ItemDefinition || !ItemDefinition->bThrowable)
    {
        return false;
    }

    FVector ThrowDirection = LaunchDirection.GetSafeNormal ();
    if (ThrowDirection.IsNearlyZero ())
    {
        ThrowDirection = Camera ? Camera->GetForwardVector () : GetActorForwardVector ();
    }
    const bool bShortToss = ThrowMode == EGridItemThrowMode::ShortToss;
    const float SpeedScale = bShortToss ? FMath::Max (0.0f, ShortThrowSpeedScale) : 1.0f;
    const float ArcScale = bShortToss ? FMath::Max (0.0f, ShortThrowArcScale) : 1.0f;
    ThrowDirection = (
        ThrowDirection +
        FVector::UpVector * FMath::Max (0.0f, ItemDefinition->ThrowArc) * ArcScale).GetSafeNormal ();

    FGridItemInstance ThrownItem;
    if (!BuildSingleItemInstanceFromCursor (ThrownItem))
    {
        return false;
    }

    ThrownItem.Weight = ItemDefinition->Weight;
    ThrownItem.OwnerType = EGridItemOwnerType::World;
    ThrownItem.OwnerGuid = FGuid ();
    ThrownItem.OwnerCharacterIndex = INDEX_NONE;
    ThrownItem.EquipmentSlot = EGridEquipmentSlot::None;

    const FVector StartLocation =
        (Camera ? Camera->GetComponentLocation () : GetActorLocation ()) +
        ThrowDirection * 60.0f;
    // TODO: Scale throw speed, accuracy and damage with the selected character's ranged/throwing skill.
    const FVector LaunchVelocity =
        ThrowDirection * FMath::Max (0.0f, ItemDefinition->ThrowSpeed) * SpeedScale;
    if (!LevelRuntimeActor->TrySpawnThrownItemProjectile (
        ThrownItem,
        StartLocation,
        LaunchVelocity,
        CurrentCellX,
        CurrentCellY))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridInventory Throw Failed Item=%s Quantity=%d Reason=ProjectileSpawnFailed"),
            *CursorItem.ItemDefinitionId.ToString (),
            CursorItem.Quantity);
        return false;
    }

    ConsumeOneCursorItemAfterSuccessfulAction ();

    UE_LOG (LogTemp, Log,
        TEXT ("GridInventory Throw Item=%s RuntimeId=%s Mode=%s CursorQuantityBefore=%d CursorQuantityAfter=%d Result=true"),
        *ThrownItem.ItemDefinitionId.ToString (),
        *ThrownItem.RuntimeObjectId.ToString (),
        bShortToss ? TEXT ("ShortToss") : TEXT ("Throw"),
        CursorItem.Quantity,
        FMath::Max (0, CursorItem.Quantity - 1));
    PartyInventoryComponent->LogInventoryOwnershipDiagnostics ();
    return true;
}

bool AGrimrockPartyPawn::BuildSingleItemInstanceFromCursor (FGridItemInstance& OutSingleItem) const
{
    OutSingleItem = FGridItemInstance ();
    if (!PartyInventoryComponent || !PartyInventoryComponent->HasCursorItem ())
    {
        return false;
    }

    const FGridItemInstance& CursorItem = PartyInventoryComponent->GetCursorItem ();
    if (!CursorItem.IsValid ())
    {
        return false;
    }

    OutSingleItem = CursorItem;
    OutSingleItem.RuntimeObjectId = FGuid::NewGuid ();
    OutSingleItem.Quantity = 1;
    return true;
}

void AGrimrockPartyPawn::ConsumeOneCursorItemAfterSuccessfulAction ()
{
    if (!PartyInventoryComponent || !PartyInventoryComponent->HasCursorItem ())
    {
        return;
    }

    const FGridItemInstance CursorItem = PartyInventoryComponent->GetCursorItem ();
    if (CursorItem.Quantity > 1)
    {
        FGridItemInstance RemainingCursorItem = CursorItem;
        RemainingCursorItem.Quantity -= 1;
        PartyInventoryComponent->SetCursorItem (RemainingCursorItem);
        return;
    }

    PartyInventoryComponent->ClearCursorItem ();
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

UGridItemDefinitionAsset* AGrimrockPartyPawn::ResolveEquippedItemDefinition (
    const FGridItemInstance& Item) const
{
    if (!Item.IsValid ())
    {
        return nullptr;
    }

    if (PartyInventoryComponent)
    {
        if (UGridItemDefinitionAsset* ItemDefinition =
            PartyInventoryComponent->FindItemDefinition (Item.ItemDefinitionId))
        {
            return ItemDefinition;
        }
    }

    return LevelRuntimeActor
        ? LevelRuntimeActor->ResolveRuntimeItemDefinition (Item.ItemDefinitionId)
        : nullptr;
}

bool AGrimrockPartyPawn::DoesEquippedItemEmitLight (
    const FGridItemInstance& Item) const
{
    const UGridItemDefinitionAsset* ItemDefinition = ResolveEquippedItemDefinition (Item);
    return Item.IsValid () &&
        ((ItemDefinition && ItemDefinition->bCanEmitLight) || Item.bLightsEnabled);
}

bool AGrimrockPartyPawn::RecomputeEquippedLightState (
    const FGridItemInstance& MainHandItem,
    bool bHasMainHandItem,
    const FGridItemInstance& OffHandItem,
    bool bHasOffHandItem) const
{
    const bool bMainLight =
        bHasMainHandItem && DoesEquippedItemEmitLight (MainHandItem);
    const bool bOffLight =
        bHasOffHandItem && DoesEquippedItemEmitLight (OffHandItem);
    const bool bResult = bMainLight || bOffLight;

    UE_LOG (LogTemp, Log,
        TEXT ("GridEquipmentLight Recompute MainHand=%s MainLight=%s OffHand=%s OffLight=%s Result=%s"),
        bHasMainHandItem
            ? *MainHandItem.ItemDefinitionId.ToString ()
            : TEXT ("None"),
        bMainLight ? TEXT ("true") : TEXT ("false"),
        bHasOffHandItem
            ? *OffHandItem.ItemDefinitionId.ToString ()
            : TEXT ("None"),
        bOffLight ? TEXT ("true") : TEXT ("false"),
        bResult ? TEXT ("true") : TEXT ("false"));

    return bResult;
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
    FGridItemInstance MainHandItem;
    FGridItemInstance OffHandItem;
    const bool bHasMainHandItem = PartyInventoryComponent->GetEquippedItem (
        CharacterIndex,
        EGridEquipmentSlot::MainHand,
        MainHandItem);
    const bool bHasOffHandItem = PartyInventoryComponent->GetEquippedItem (
        CharacterIndex,
        EGridEquipmentSlot::OffHand,
        OffHandItem);

    const bool bMainLight = bHasMainHandItem && DoesEquippedItemEmitLight (MainHandItem);
    const bool bOffLight = bHasOffHandItem && DoesEquippedItemEmitLight (OffHandItem);
    const bool bAnyEquippedLight = RecomputeEquippedLightState (
        MainHandItem,
        bHasMainHandItem,
        OffHandItem,
        bHasOffHandItem);

    const FGridItemInstance* VisualItem = nullptr;
    EGridEquipmentSlot VisualSlot = EGridEquipmentSlot::None;
    if (bMainLight)
    {
        VisualItem = &MainHandItem;
        VisualSlot = EGridEquipmentSlot::MainHand;
    }
    else if (bOffLight)
    {
        VisualItem = &OffHandItem;
        VisualSlot = EGridEquipmentSlot::OffHand;
    }
    else if (bHasMainHandItem)
    {
        VisualItem = &MainHandItem;
        VisualSlot = EGridEquipmentSlot::MainHand;
    }
    else if (bHasOffHandItem)
    {
        VisualItem = &OffHandItem;
        VisualSlot = EGridEquipmentSlot::OffHand;
    }

    if (!VisualItem || VisualSlot == EGridEquipmentSlot::None)
    {
        ClearHeldItem ();
        return;
    }

    if (GetHeldItemDefinitionId () != VisualItem->ItemDefinitionId &&
        !EquipHeldItem (VisualItem->ItemDefinitionId))
    {
        return;
    }

    if (HeldItemActor)
    {
        HeldItemActor->SetItemLightsEnabled (bAnyEquippedLight);
    }
    bHasTorchInHand = bAnyEquippedLight;

    UE_LOG (LogTemp, Log, TEXT ("GridInventory HeldVisual Sync Equipped Character=%d Slot=%s Item=%s"),
        CharacterIndex,
        GetPawnEquipmentSlotName (VisualSlot),
        *VisualItem->ItemDefinitionId.ToString ());
}
