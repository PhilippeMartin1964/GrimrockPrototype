#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
    bool IsCardinalFacing (EGridEdge Facing)
    {
        return Facing == EGridEdge::North ||
            Facing == EGridEdge::East ||
            Facing == EGridEdge::South ||
            Facing == EGridEdge::West;
    }

    FString GetPlayerAttackRejectReasonText (
        EGridPlayerAttackRejectReason RejectReason)
    {
        const UEnum* RejectReasonEnum =
            StaticEnum<EGridPlayerAttackRejectReason> ();
        return RejectReasonEnum
            ? RejectReasonEnum->GetNameStringByValue (
                static_cast<int64> (RejectReason))
            : TEXT ("Unknown");
    }
}

bool UGridTurnManagerComponent::RequestSelectedCharacterAttack (
    FGridPlayerAttackRequest& OutRequest,
    EGridPlayerAttackRejectReason& OutRejectReason)
{
    const int32 SelectedCharacterIndex =
        IsValid (PartyPawn) &&
        IsValid (PartyPawn->PartyInventoryComponent)
            ? PartyPawn->PartyInventoryComponent->GetSelectedCharacterIndex ()
            : INDEX_NONE;
    return RequestCharacterAttack (
        SelectedCharacterIndex,
        OutRequest,
        OutRejectReason);
}

bool UGridTurnManagerComponent::RequestCharacterAttack (
    int32 AttackerCharacterIndex,
    FGridPlayerAttackRequest& OutRequest,
    EGridPlayerAttackRejectReason& OutRejectReason)
{
    OutRequest = FGridPlayerAttackRequest ();
    OutRejectReason = EGridPlayerAttackRejectReason::None;

    if (!bInitialized)
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TurnManagerNotInitialized,
            OutRejectReason);
    }
    if (!IsValid (RuntimeActor) ||
        !IsValid (PartyPawn) ||
        !IsValid (PartyPawn->PartyInventoryComponent))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::PartyUnavailable,
            OutRejectReason);
    }
    if (!bCombatActive)
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::CombatInactive,
            OutRejectReason);
    }
    if (CurrentPhase != EGridCombatPhase::PlayerPhase)
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::NotPlayerPhase,
            OutRejectReason);
    }
    if (!IsPartyAtRest ())
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::PartyBusy,
            OutRejectReason);
    }

    const TArray<FGridCharacterInventoryState>& Characters =
        PartyPawn->PartyInventoryComponent->PartyInventoryState
            .ActiveCharacters;
    if (!Characters.IsValidIndex (AttackerCharacterIndex))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::InvalidAttacker,
            OutRejectReason);
    }

    const FGridCharacterInventoryState& Attacker =
        Characters[AttackerCharacterIndex];
    if (!Attacker.CharacterId.IsValid ())
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::InvalidAttacker,
            OutRejectReason);
    }
    if (Attacker.DerivedStats.CurrentHealth <= 0)
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::AttackerDefeated,
            OutRejectReason);
    }
    if (PlayerAttackCommittedCharacterIds.Contains (Attacker.CharacterId))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::AttackerAlreadyActed,
            OutRejectReason);
    }
    if (!IsCardinalFacing (PartyPawn->Facing))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::InvalidFacing,
            OutRejectReason);
    }

    const FIntPoint PartyCell (
        PartyPawn->CurrentCellX,
        PartyPawn->CurrentCellY);
    int32 TargetCellX = INDEX_NONE;
    int32 TargetCellY = INDEX_NONE;
    if (!RuntimeActor->TryGetNeighborCell (
        PartyCell.X,
        PartyCell.Y,
        PartyPawn->Facing,
        TargetCellX,
        TargetCellY))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetCellUnavailable,
            OutRejectReason);
    }
    if (!RuntimeActor->CanMove (
        PartyCell.X,
        PartyCell.Y,
        PartyPawn->Facing))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::PassageBlocked,
            OutRejectReason);
    }

    UWorld* World = GetWorld ();
    UGridMonsterOccupancySubsystem* Occupancy =
        World ? World->GetSubsystem<UGridMonsterOccupancySubsystem> () : nullptr;
    if (!IsValid (Occupancy))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetCellUnavailable,
            OutRejectReason);
    }

    const FIntPoint TargetCell (TargetCellX, TargetCellY);
    AGridMonsterActor* TargetMonster =
        Occupancy->GetOccupantAtCell (TargetCell);
    if (!IsValid (TargetMonster))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::NoMonsterInFront,
            OutRejectReason);
    }
    if (!IsCombatMonster (TargetMonster))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetNotInEncounter,
            OutRejectReason);
    }
    if (!TargetMonster->bMonsterEnabled)
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetInactive,
            OutRejectReason);
    }
    if (!TargetMonster->IsRuntimeLevelActive ())
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetInactive,
            OutRejectReason);
    }
    if (TargetMonster->IsDead () || TargetMonster->CurrentHealth <= 0)
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetDefeated,
            OutRejectReason);
    }

    const FGuid TargetMonsterId = TargetMonster->ResolvePersistenceId ();
    if (!TargetMonsterId.IsValid ())
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetInactive,
            OutRejectReason);
    }

    const int32 GridDistance =
        FMath::Abs (TargetCell.X - PartyCell.X) +
        FMath::Abs (TargetCell.Y - PartyCell.Y);
    if (GridDistance != 1)
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetOutOfRange,
            OutRejectReason);
    }

    FGridPlayerAttackRequest Request;
    Request.RequestId = FGuid::NewGuid ();
    Request.RoundNumber = RoundNumber;
    Request.AttackerCharacterIndex = AttackerCharacterIndex;
    Request.AttackerCharacterId = Attacker.CharacterId;
    Request.TargetMonsterId = TargetMonsterId;
    Request.PartyCell = PartyCell;
    Request.TargetCell = TargetCell;
    Request.PartyFacing = PartyPawn->Facing;
    Request.RangeCells = 1;
    Request.AttackId = NAME_None;
    if (!Request.IsValid ())
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetCellUnavailable,
            OutRejectReason);
    }

    OutRequest = Request;
    OutRejectReason = EGridPlayerAttackRejectReason::None;
    LastPlayerAttackRequest = Request;
    LastPlayerAttackRejectReason = EGridPlayerAttackRejectReason::None;
    PlayerAttackCommittedCharacterIds.Add (Attacker.CharacterId);

    UE_LOG (LogGridTurnManager, Log,
        TEXT ("[GridPlayerAttack] Accepted=true Request=%s Round=%d Attacker=%d Character=%s Target=%s PartyCell=(%d,%d) TargetCell=(%d,%d) Facing=%s Range=%d"),
        *Request.RequestId.ToString (EGuidFormats::Digits),
        Request.RoundNumber,
        Request.AttackerCharacterIndex,
        *Request.AttackerCharacterId.ToString (EGuidFormats::Digits),
        *Request.TargetMonsterId.ToString (EGuidFormats::Digits),
        Request.PartyCell.X,
        Request.PartyCell.Y,
        Request.TargetCell.X,
        Request.TargetCell.Y,
        *UEnum::GetValueAsString (Request.PartyFacing),
        Request.RangeCells);
    OnPlayerAttackRequested.Broadcast (Request);
    return true;
}

bool UGridTurnManagerComponent::HasCharacterCommittedAttackThisPhase (
    int32 CharacterIndex) const
{
    if (!IsValid (PartyPawn) ||
        !IsValid (PartyPawn->PartyInventoryComponent))
    {
        return false;
    }

    const TArray<FGridCharacterInventoryState>& Characters =
        PartyPawn->PartyInventoryComponent->PartyInventoryState
            .ActiveCharacters;
    return Characters.IsValidIndex (CharacterIndex) &&
        Characters[CharacterIndex].CharacterId.IsValid () &&
        PlayerAttackCommittedCharacterIds.Contains (
            Characters[CharacterIndex].CharacterId);
}

bool UGridTurnManagerComponent::RejectPlayerAttack (
    int32 AttackerCharacterIndex,
    EGridPlayerAttackRejectReason RejectReason,
    EGridPlayerAttackRejectReason& OutRejectReason)
{
    OutRejectReason = RejectReason;
    LastPlayerAttackRejectReason = RejectReason;
    UE_LOG (LogGridTurnManager, Log,
        TEXT ("[GridPlayerAttack] Accepted=false Reason=%s Round=%d Phase=%s Attacker=%d"),
        *GetPlayerAttackRejectReasonText (RejectReason),
        RoundNumber,
        *UEnum::GetValueAsString (CurrentPhase),
        AttackerCharacterIndex);
    return false;
}

bool UGridTurnManagerComponent::IsCombatMonster (
    const AGridMonsterActor* Monster) const
{
    return IsValid (Monster) &&
        CombatMonsters.ContainsByPredicate (
            [Monster] (const TObjectPtr<AGridMonsterActor>& Candidate)
            {
                return Candidate.Get () == Monster;
            });
}

void UGridTurnManagerComponent::ResetPlayerAttackPhaseState ()
{
    PlayerAttackCommittedCharacterIds.Reset ();
}
