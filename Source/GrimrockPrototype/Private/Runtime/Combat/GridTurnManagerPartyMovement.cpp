#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Core/GridDirectionUtils.h"
#include "Engine/World.h"
#include "RPG/StatusEffects/GridStatusEffectControlResolver.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
    bool IsPartyMovementCardinalDirection (EGridEdge Direction)
    {
        return Direction == EGridEdge::North ||
            Direction == EGridEdge::East ||
            Direction == EGridEdge::South ||
            Direction == EGridEdge::West;
    }

    FString GetPartyMovementRejectReasonText (
        EGridPartyMovementRejectReason RejectReason)
    {
        const UEnum* RejectReasonEnum =
            StaticEnum<EGridPartyMovementRejectReason> ();
        return RejectReasonEnum
            ? RejectReasonEnum->GetNameStringByValue (
                static_cast<int64> (RejectReason))
            : TEXT ("Unknown");
    }
}

bool UGridTurnManagerComponent::RequestPartyTranslation (
    EGridEdge MoveDirection,
    FIntPoint& OutTargetCell,
    EGridPartyMovementRejectReason& OutRejectReason)
{
    OutTargetCell = FIntPoint::ZeroValue;
    OutRejectReason = EGridPartyMovementRejectReason::None;

    const int32 CharacterIndex = ResolveActivePartyCharacterIndex ();
    if (!bInitialized)
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::TurnManagerNotInitialized,
            OutRejectReason);
    }
    if (!bCombatActive)
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::CombatInactive,
            OutRejectReason);
    }
    if (!IsValid (RuntimeActor) ||
        !IsValid (PartyPawn) ||
        !IsValid (PartyPawn->PartyInventoryComponent))
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::PartyUnavailable,
            OutRejectReason);
    }
    if (CurrentPhase != EGridCombatPhase::PlayerPhase)
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::NotPlayerTurn,
            OutRejectReason);
    }
    if (CharacterIndex == INDEX_NONE ||
        (!InitiativeOrder.IsEmpty () &&
            !IsActivePlayerCharacter (CharacterIndex)))
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::NotActiveCombatant,
            OutRejectReason);
    }
    if (bPartyInputLocked ||
        bPlayerAttackResolutionInProgress ||
        PendingPartyMotionType != EGridPendingPartyMotionType::None ||
        !IsPartyAtRest ())
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::PartyBusy,
            OutRejectReason);
    }

    const TArray<FGridCharacterInventoryState>& Characters =
        PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
    if (Characters.IsValidIndex (CharacterIndex) &&
        FGridStatusEffectControlResolver::Resolve (
            Characters[CharacterIndex].StatusEffects).bBlockTranslation)
    {
        UE_LOG (
            LogGridTurnManager,
            Log,
            TEXT ("[MON16.5] TranslationBlocked Target=Party Character=%d Round=%d"),
            CharacterIndex,
            RoundNumber);

        // MON16.6 will surface a dedicated presentation reason. Keep the
        // existing public movement reject contract unchanged in MON16.5.
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::PartyBusy,
            OutRejectReason);
    }

    if (!IsPartyMovementCardinalDirection (MoveDirection))
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::InvalidDirection,
            OutRejectReason);
    }

    const FIntPoint FromCell (
        PartyPawn->CurrentCellX,
        PartyPawn->CurrentCellY);
    int32 TargetX = INDEX_NONE;
    int32 TargetY = INDEX_NONE;
    if (!RuntimeActor->TryGetNeighborCell (
        FromCell.X,
        FromCell.Y,
        MoveDirection,
        TargetX,
        TargetY))
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::TargetCellUnavailable,
            OutRejectReason);
    }
    if (!RuntimeActor->CanMove (
        FromCell.X,
        FromCell.Y,
        MoveDirection))
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::PassageBlocked,
            OutRejectReason);
    }

    const FIntPoint TargetCell (TargetX, TargetY);
    UGridMonsterOccupancySubsystem* Occupancy = GetWorld ()
        ? GetWorld ()->GetSubsystem<UGridMonsterOccupancySubsystem> ()
        : nullptr;
    if (IsValid (Occupancy) &&
        IsValid (Occupancy->GetOccupantAtCell (TargetCell)))
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::TargetCellOccupied,
            OutRejectReason);
    }

    const int32 PersonalActionPointCost = FMath::Clamp (
        PartyTranslationActionPointCost,
        1,
        6);
    const int32 MobilityActionPointCost = FMath::Clamp (
        PartyTranslationMobilityActionPointCost,
        1,
        4);
    if (!CanCharacterSpendActionPoints (
        CharacterIndex,
        PersonalActionPointCost))
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::InsufficientActionPoints,
            OutRejectReason);
    }
    if (!PartyMobilityState.CanSpend (MobilityActionPointCost))
    {
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::
                InsufficientMobilityActionPoints,
            OutRejectReason);
    }

    PendingPartyMotionType = EGridPendingPartyMotionType::Translation;
    PendingPartyMotionCharacterIndex = CharacterIndex;
    PendingPartyTranslationFromCell = FromCell;
    PendingPartyTranslationTargetCell = TargetCell;
    if (!SpendPlayerCharacterActionPoints (
        CharacterIndex,
        PersonalActionPointCost))
    {
        ClearPendingPartyMotion ();
        return RejectPartyMovement (
            CharacterIndex,
            MoveDirection,
            EGridPartyMovementRejectReason::InsufficientActionPoints,
            OutRejectReason);
    }

    PartyMobilityState.RemainingMobilityActionPoints = FMath::Max (
        0,
        PartyMobilityState.RemainingMobilityActionPoints -
            MobilityActionPointCost);
    LastPartyMovementRejectReason =
        EGridPartyMovementRejectReason::None;
    OutTargetCell = TargetCell;
    OnPartyMobilityStateChanged.Broadcast (PartyMobilityState);

    FGridPlayerCharacterTurnState TurnState;
    GetPlayerCharacterTurnState (CharacterIndex, TurnState);
    UE_LOG (
        LogGridTurnManager,
        Log,
        TEXT ("[GridPartyMovement] Accepted=true Type=Translation Round=%d Character=%d From=(%d,%d) To=(%d,%d) Direction=%s AP=%d/%d PAM=%d/%d"),
        RoundNumber,
        CharacterIndex,
        FromCell.X,
        FromCell.Y,
        TargetCell.X,
        TargetCell.Y,
        *UEnum::GetValueAsString (MoveDirection),
        TurnState.RemainingActionPoints,
        TurnState.MaximumActionPoints,
        PartyMobilityState.RemainingMobilityActionPoints,
        PartyMobilityState.MaximumMobilityActionPoints);
    return true;
}

bool UGridTurnManagerComponent::RequestPartyRotation (
    EGridEdge TargetFacing,
    EGridPartyMovementRejectReason& OutRejectReason)
{
    OutRejectReason = EGridPartyMovementRejectReason::None;
    const int32 CharacterIndex = ResolveActivePartyCharacterIndex ();

    if (!bInitialized)
    {
        return RejectPartyMovement (
            CharacterIndex,
            TargetFacing,
            EGridPartyMovementRejectReason::TurnManagerNotInitialized,
            OutRejectReason);
    }
    if (!bCombatActive)
    {
        return RejectPartyMovement (
            CharacterIndex,
            TargetFacing,
            EGridPartyMovementRejectReason::CombatInactive,
            OutRejectReason);
    }
    if (!IsValid (PartyPawn) ||
        !IsValid (PartyPawn->PartyInventoryComponent))
    {
        return RejectPartyMovement (
            CharacterIndex,
            TargetFacing,
            EGridPartyMovementRejectReason::PartyUnavailable,
            OutRejectReason);
    }
    if (CurrentPhase != EGridCombatPhase::PlayerPhase)
    {
        return RejectPartyMovement (
            CharacterIndex,
            TargetFacing,
            EGridPartyMovementRejectReason::NotPlayerTurn,
            OutRejectReason);
    }
    if (CharacterIndex == INDEX_NONE ||
        (!InitiativeOrder.IsEmpty () &&
            !IsActivePlayerCharacter (CharacterIndex)))
    {
        return RejectPartyMovement (
            CharacterIndex,
            TargetFacing,
            EGridPartyMovementRejectReason::NotActiveCombatant,
            OutRejectReason);
    }
    if (bPartyInputLocked ||
        bPlayerAttackResolutionInProgress ||
        PendingPartyMotionType != EGridPendingPartyMotionType::None ||
        !IsPartyAtRest ())
    {
        return RejectPartyMovement (
            CharacterIndex,
            TargetFacing,
            EGridPartyMovementRejectReason::PartyBusy,
            OutRejectReason);
    }
    if (!IsPartyMovementCardinalDirection (TargetFacing) ||
        (TargetFacing != GridDirectionUtils::RotateLeft (PartyPawn->Facing) &&
            TargetFacing != GridDirectionUtils::RotateRight (
                PartyPawn->Facing)))
    {
        return RejectPartyMovement (
            CharacterIndex,
            TargetFacing,
            EGridPartyMovementRejectReason::InvalidDirection,
            OutRejectReason);
    }
    if (!CanCharacterSpendActionPoints (CharacterIndex, 0))
    {
        return RejectPartyMovement (
            CharacterIndex,
            TargetFacing,
            EGridPartyMovementRejectReason::InsufficientActionPoints,
            OutRejectReason);
    }

    PendingPartyMotionType = EGridPendingPartyMotionType::Rotation;
    PendingPartyMotionCharacterIndex = CharacterIndex;
    LastPartyMovementRejectReason =
        EGridPartyMovementRejectReason::None;
    if (const FGridPlayerCharacterTurnState* TurnState =
        FindPlayerCharacterTurnState (
            ResolvePlayerCombatantId (CharacterIndex)))
    {
        BroadcastPlayerCharacterTurnState (*TurnState);
    }

    UE_LOG (
        LogGridTurnManager,
        Log,
        TEXT ("[GridPartyMovement] Accepted=true Type=Rotation Round=%d Character=%d From=%s To=%s CostAP=0 CostPAM=0"),
        RoundNumber,
        CharacterIndex,
        *UEnum::GetValueAsString (PartyPawn->Facing),
        *UEnum::GetValueAsString (TargetFacing));
    return true;
}

bool UGridTurnManagerComponent::NotifyPartyTranslationCompleted ()
{
    return CompletePendingPartyMotion (
        EGridPendingPartyMotionType::Translation);
}

bool UGridTurnManagerComponent::NotifyPartyRotationCompleted ()
{
    return CompletePendingPartyMotion (
        EGridPendingPartyMotionType::Rotation);
}

int32 UGridTurnManagerComponent::ResolveActivePartyCharacterIndex () const
{
    if (!InitiativeOrder.IsEmpty ())
    {
        FGridCombatantInitiativeEntry ActiveCombatant;
        return GetActiveCombatant (ActiveCombatant) &&
            ActiveCombatant.Side == EGridCombatantSide::Party
                ? ActiveCombatant.CharacterIndex
                : INDEX_NONE;
    }

    return CurrentPhase == EGridCombatPhase::PlayerPhase &&
        IsValid (PartyPawn) &&
        IsValid (PartyPawn->PartyInventoryComponent)
            ? PartyPawn->PartyInventoryComponent
                ->GetSelectedCharacterIndex ()
            : INDEX_NONE;
}

bool UGridTurnManagerComponent::RejectPartyMovement (
    int32 CharacterIndex,
    EGridEdge Direction,
    EGridPartyMovementRejectReason RejectReason,
    EGridPartyMovementRejectReason& OutRejectReason)
{
    OutRejectReason = RejectReason;
    LastPartyMovementRejectReason = RejectReason;
    FGridPlayerCharacterTurnState TurnState;
    const int32 RemainingActionPoints =
        GetPlayerCharacterTurnState (CharacterIndex, TurnState)
            ? TurnState.RemainingActionPoints
            : 0;
    UE_LOG (
        LogGridTurnManager,
        Log,
        TEXT ("[GridPartyMovement] Accepted=false Round=%d Character=%d Direction=%s Reason=%s AP=%d PAM=%d/%d"),
        RoundNumber,
        CharacterIndex,
        *UEnum::GetValueAsString (Direction),
        *GetPartyMovementRejectReasonText (RejectReason),
        RemainingActionPoints,
        PartyMobilityState.RemainingMobilityActionPoints,
        PartyMobilityState.MaximumMobilityActionPoints);
    OnPartyMovementRejected.Broadcast (
        CharacterIndex,
        Direction,
        RejectReason);
    return false;
}

void UGridTurnManagerComponent::ResetPartyMobilityForRound ()
{
    ClearPendingPartyMotion ();
    PartyMobilityState.RoundNumber = RoundNumber;
    PartyMobilityState.MaximumMobilityActionPoints = FMath::Clamp (
        BasePartyMobilityActionPointsPerRound,
        0,
        4);
    PartyMobilityState.RemainingMobilityActionPoints =
        PartyMobilityState.MaximumMobilityActionPoints;
    LastPartyMovementRejectReason =
        EGridPartyMovementRejectReason::None;
    OnPartyMobilityStateChanged.Broadcast (PartyMobilityState);

    UE_LOG (
        LogGridTurnManager,
        Log,
        TEXT ("[GridPartyMovement] Round=%d PAM=%d/%d"),
        PartyMobilityState.RoundNumber,
        PartyMobilityState.RemainingMobilityActionPoints,
        PartyMobilityState.MaximumMobilityActionPoints);
}

void UGridTurnManagerComponent::ClearPartyMobilityState (bool bBroadcast)
{
    ClearPendingPartyMotion ();
    PartyMobilityState = FGridPartyMobilityState ();
    LastPartyMovementRejectReason =
        EGridPartyMovementRejectReason::None;
    if (bBroadcast)
    {
        OnPartyMobilityStateChanged.Broadcast (PartyMobilityState);
    }
}

void UGridTurnManagerComponent::ClearPendingPartyMotion ()
{
    PendingPartyMotionType = EGridPendingPartyMotionType::None;
    PendingPartyMotionCharacterIndex = INDEX_NONE;
    PendingPartyTranslationFromCell = FIntPoint::ZeroValue;
    PendingPartyTranslationTargetCell = FIntPoint::ZeroValue;
}

bool UGridTurnManagerComponent::CompletePendingPartyMotion (
    EGridPendingPartyMotionType ExpectedMotionType)
{
    if (PendingPartyMotionType != ExpectedMotionType)
    {
        return false;
    }

    const int32 CharacterIndex =
        PendingPartyMotionCharacterIndex;
    const FIntPoint FromCell = PendingPartyTranslationFromCell;
    const FIntPoint TargetCell = PendingPartyTranslationTargetCell;
    ClearPendingPartyMotion ();

    FGridPlayerCharacterTurnState TurnState;
    const bool bHasTurnState = GetPlayerCharacterTurnState (
        CharacterIndex,
        TurnState);
    UE_LOG (
        LogGridTurnManager,
        Log,
        TEXT ("[GridPartyMovement] Completed Type=%s Round=%d Character=%d From=(%d,%d) To=(%d,%d) AP=%d/%d PAM=%d/%d"),
        ExpectedMotionType == EGridPendingPartyMotionType::Translation
            ? TEXT ("Translation")
            : TEXT ("Rotation"),
        RoundNumber,
        CharacterIndex,
        FromCell.X,
        FromCell.Y,
        TargetCell.X,
        TargetCell.Y,
        bHasTurnState ? TurnState.RemainingActionPoints : 0,
        bHasTurnState ? TurnState.MaximumActionPoints : 0,
        PartyMobilityState.RemainingMobilityActionPoints,
        PartyMobilityState.MaximumMobilityActionPoints);

    if (!bCombatActive || !bHasTurnState ||
        TurnState.State != EGridCombatantTurnState::Active)
    {
        return true;
    }

    if (TurnState.RemainingActionPoints <= 0 &&
        !InitiativeOrder.IsEmpty () &&
        IsActivePlayerCharacter (CharacterIndex))
    {
        FinishActivePlayerTurn ();
    }
    else
    {
        BroadcastPlayerCharacterTurnState (TurnState);
    }
    return true;
}
