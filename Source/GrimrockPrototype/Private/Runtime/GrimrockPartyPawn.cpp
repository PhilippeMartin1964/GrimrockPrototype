#include "Runtime/GrimrockPartyPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridLevelRuntimeActor.h"

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

    Camera = CreateDefaultSubobject<UCameraComponent> (TEXT ("Camera"));
    Camera->SetupAttachment (SpringArm);
    Camera->bUsePawnControlRotation = false;

    AutoPossessPlayer = EAutoReceiveInput::Player0;
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

    SnapToCurrentCell ();

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
        
        if (UseAction)
        {
            EIC->BindAction (UseAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleUse);
        }
    }
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

    const EGridEdge FrontEdge = GridDirectionUtils::GetForward (Facing);
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

    CurrentCellX = NextX;
    CurrentCellY = NextY;

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