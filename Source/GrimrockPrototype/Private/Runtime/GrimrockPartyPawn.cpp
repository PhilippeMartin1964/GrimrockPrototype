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
    Rot.Yaw = FacingToYaw (Facing);
    SetActorRotation (Rot);
}

void AGrimrockPartyPawn::HandleMoveForward (const FInputActionValue& Value)
{
    (void)Value;
    TryStartMove (GetRelativeDirectionForward ());
}

void AGrimrockPartyPawn::HandleMoveBackward (const FInputActionValue& Value)
{
    (void)Value;
    TryStartMove (GetRelativeDirectionBackward ());
}

void AGrimrockPartyPawn::HandleTurnLeft (const FInputActionValue& Value)
{
    (void)Value;
    TryStartTurn (true);
}

void AGrimrockPartyPawn::HandleTurnRight (const FInputActionValue& Value)
{
    (void)Value;
    TryStartTurn (false);
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

    Facing = bTurnRight ? RotateRight (Facing) : RotateLeft (Facing);
    TurnTargetYaw = FacingToYaw (Facing);

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

EGridEdge AGrimrockPartyPawn::GetRelativeDirectionForward () const
{
    return Facing;
}

EGridEdge AGrimrockPartyPawn::GetRelativeDirectionBackward () const
{
    return RotateRight (RotateRight (Facing));
}

EGridEdge AGrimrockPartyPawn::RotateLeft (EGridEdge Dir)
{
    switch (Dir)
    {
        case EGridEdge::North: return EGridEdge::West;
        case EGridEdge::West:  return EGridEdge::South;
        case EGridEdge::South: return EGridEdge::East;
        case EGridEdge::East:  return EGridEdge::North;
        default:               return EGridEdge::North;
    }
}

EGridEdge AGrimrockPartyPawn::RotateRight (EGridEdge Dir)
{
    switch (Dir)
    {
        case EGridEdge::North: return EGridEdge::East;
        case EGridEdge::East:  return EGridEdge::South;
        case EGridEdge::South: return EGridEdge::West;
        case EGridEdge::West:  return EGridEdge::North;
        default:               return EGridEdge::North;
    }
}

float AGrimrockPartyPawn::FacingToYaw (EGridEdge Dir)
{
    switch (Dir)
    {
        case EGridEdge::North: return 90.f;
        case EGridEdge::East:  return 0.f;
        case EGridEdge::South: return -90.f;
        case EGridEdge::West:  return 180.f;
        default:               return 0.f;
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