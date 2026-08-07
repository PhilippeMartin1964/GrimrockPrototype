#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
    const FName UnarmedAttackId = TEXT ("Attack_Unarmed");

    FGridOffensiveEquipmentProfile MakeUnarmedOffensiveProfile ()
    {
        FGridOffensiveEquipmentProfile Profile;
        Profile.AttackId = UnarmedAttackId;
        Profile.AttackDefinition.DamageType = EGridDamageType::Physical;
        Profile.AttackDefinition.PhysicalSubtype =
            EGridPhysicalDamageSubtype::Bludgeoning;
        Profile.AttackDefinition.MinDamage = 1;
        Profile.AttackDefinition.MaxDamage = 3;
        Profile.AttackDefinition.AccuracyBonus = 0;
        Profile.FlatDamageBonus = 0;
        Profile.DamageScalingAttribute =
            EGridAttackScalingAttribute::Strength;
        Profile.RangeCells = 1;
        return Profile;
    }

    bool ResolveMON12ItemAttackProfile (
        const UGridItemDefinitionAsset* Definition,
        FGridOffensiveEquipmentProfile& OutProfile)
    {
        OutProfile = FGridOffensiveEquipmentProfile ();
        if (!IsValid (Definition))
        {
            return false;
        }
        if (!Definition->CombatActions.IsEmpty ())
        {
            for (const FGridCombatActionDefinition& Action :
                Definition->CombatActions)
            {
                if (Action.IsValid () &&
                    Action.SourcePolicy ==
                        EGridCombatActionSourcePolicy::Equipment &&
                    Action.ResolutionProfile ==
                        EGridCombatActionResolutionProfile::Attack)
                {
                    OutProfile = Action.OffensiveProfile;
                    return true;
                }
            }
            return false;
        }
        if (!Definition->HasValidOffensiveProfile ())
        {
            return false;
        }
        OutProfile = Definition->OffensiveProfile;
        return true;
    }

    bool DoesMON12ItemDeclareAttack (
        const UGridItemDefinitionAsset* Definition)
    {
        return IsValid (Definition) &&
            (Definition->bProvidesAttack ||
                Definition->CombatActions.ContainsByPredicate (
                    [] (const FGridCombatActionDefinition& Action)
                    {
                        return Action.ResolutionProfile ==
                            EGridCombatActionResolutionProfile::Attack;
                    }));
    }

    int32 GetScalingAttributeValue (
        const FRPGAttributes& Attributes,
        EGridAttackScalingAttribute ScalingAttribute)
    {
        switch (ScalingAttribute)
        {
        case EGridAttackScalingAttribute::Strength:
            return Attributes.Strength;
        case EGridAttackScalingAttribute::Dexterity:
            return Attributes.Dexterity;
        case EGridAttackScalingAttribute::Constitution:
            return Attributes.Constitution;
        case EGridAttackScalingAttribute::Intelligence:
            return Attributes.Intelligence;
        case EGridAttackScalingAttribute::Wisdom:
            return Attributes.Wisdom;
        case EGridAttackScalingAttribute::Charisma:
            return Attributes.Charisma;
        case EGridAttackScalingAttribute::None:
        default:
            return 0;
        }
    }

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
    FGridAttackResult& OutResult,
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
        OutResult,
        OutRejectReason);
}

bool UGridTurnManagerComponent::RequestCharacterAttack (
    int32 AttackerCharacterIndex,
    FGridPlayerAttackRequest& OutRequest,
    FGridAttackResult& OutResult,
    EGridPlayerAttackRejectReason& OutRejectReason)
{
    TArray<FGridAvailableCombatAction> AvailableActions;
    GetAvailableCombatActions (
        AttackerCharacterIndex,
        AvailableActions);
    const FGridAvailableCombatAction* CatalogAttack =
        AvailableActions.FindByPredicate (
            [] (const FGridAvailableCombatAction& Action)
            {
                return Action.Definition.SourcePolicy ==
                        EGridCombatActionSourcePolicy::Equipment &&
                    Action.Definition.ResolutionProfile ==
                        EGridCombatActionResolutionProfile::Attack;
            });
    if (!CatalogAttack)
    {
        CatalogAttack = AvailableActions.FindByPredicate (
            [] (const FGridAvailableCombatAction& Action)
            {
                return Action.Definition.ActionId ==
                        UnarmedAttackId &&
                    Action.Definition.SourcePolicy ==
                        EGridCombatActionSourcePolicy::Universal &&
                    Action.Definition.ResolutionProfile ==
                        EGridCombatActionResolutionProfile::Attack;
            });
    }
    if (CatalogAttack)
    {
        return RequestCharacterAttackInternal (
            AttackerCharacterIndex,
            CatalogAttack->SourceEquipmentSlot,
            CatalogAttack->Definition.SourcePolicy ==
                EGridCombatActionSourcePolicy::Equipment,
            CatalogAttack,
            OutRequest,
            OutResult,
            OutRejectReason);
    }

    return RequestCharacterAttackInternal (
        AttackerCharacterIndex,
        EGridEquipmentSlot::None,
        false,
        nullptr,
        OutRequest,
        OutResult,
        OutRejectReason);
}

bool UGridTurnManagerComponent::RequestCharacterAttackFromSlot (
    int32 AttackerCharacterIndex,
    EGridEquipmentSlot RequestedEquipmentSlot,
    FGridPlayerAttackRequest& OutRequest,
    FGridAttackResult& OutResult,
    EGridPlayerAttackRejectReason& OutRejectReason)
{
    TArray<FGridAvailableCombatAction> AvailableActions;
    GetAvailableCombatActions (
        AttackerCharacterIndex,
        AvailableActions);
    const FGridAvailableCombatAction* FirstAttackFromSlot =
        AvailableActions.FindByPredicate (
            [RequestedEquipmentSlot]
            (const FGridAvailableCombatAction& Action)
            {
                return Action.SourceEquipmentSlot ==
                        RequestedEquipmentSlot &&
                    Action.Definition.SourcePolicy ==
                        EGridCombatActionSourcePolicy::Equipment &&
                    Action.Definition.ResolutionProfile ==
                        EGridCombatActionResolutionProfile::Attack;
            });
    if (FirstAttackFromSlot)
    {
        return RequestCharacterAttackInternal (
            AttackerCharacterIndex,
            RequestedEquipmentSlot,
            true,
            FirstAttackFromSlot,
            OutRequest,
            OutResult,
            OutRejectReason);
    }

    return RequestCharacterAttackInternal (
        AttackerCharacterIndex,
        RequestedEquipmentSlot,
        true,
        nullptr,
        OutRequest,
        OutResult,
        OutRejectReason);
}

bool UGridTurnManagerComponent::RequestCharacterAttackInternal (
    int32 AttackerCharacterIndex,
    EGridEquipmentSlot RequestedEquipmentSlot,
    bool bRequireRequestedEquipmentSlot,
    const FGridAvailableCombatAction* CombatActionOverride,
    FGridPlayerAttackRequest& OutRequest,
    FGridAttackResult& OutResult,
    EGridPlayerAttackRejectReason& OutRejectReason)
{
    OutRequest = FGridPlayerAttackRequest ();
    OutResult = FGridAttackResult ();
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
    if (!IsPartyAtRest () || bPlayerAttackResolutionInProgress)
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
    if (!InitiativeOrder.IsEmpty () &&
        !IsActivePlayerCharacter (AttackerCharacterIndex))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::NotActiveCombatant,
            OutRejectReason);
    }
    const int32 SafeAttackActionPointCost = FMath::Clamp (
        CombatActionOverride
            ? CombatActionOverride->CurrentActionPointCost
            : PlayerAttackActionPointCost,
        1,
        6);
    if (!CanCharacterSpendActionPoints (
        AttackerCharacterIndex,
        SafeAttackActionPointCost))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::InsufficientActionPoints,
            OutRejectReason);
    }
    if (!IsCardinalFacing (PartyPawn->Facing))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::InvalidFacing,
            OutRejectReason);
    }

    FGridOffensiveEquipmentProfile OffensiveProfile;
    FName OffensiveItemDefinitionId = NAME_None;
    EGridEquipmentSlot OffensiveEquipmentSlot =
        EGridEquipmentSlot::None;
    EGridPlayerAttackRejectReason OffensiveProfileRejectReason =
        EGridPlayerAttackRejectReason::None;
    if (CombatActionOverride)
    {
        const bool bValidOverride =
            CombatActionOverride->IsValid () &&
            CombatActionOverride->bEnabled &&
            CombatActionOverride->CharacterIndex ==
                AttackerCharacterIndex &&
            CombatActionOverride->CharacterId ==
                Attacker.CharacterId &&
            CombatActionOverride->Definition.ResolutionProfile ==
                EGridCombatActionResolutionProfile::Attack &&
            CombatActionOverride->Definition.OffensiveProfile.IsValid ();
        if (!bValidOverride)
        {
            return RejectPlayerAttack (
                AttackerCharacterIndex,
                EGridPlayerAttackRejectReason::
                    InvalidOffensiveEquipment,
                OutRejectReason);
        }
        OffensiveProfile =
            CombatActionOverride->Definition.OffensiveProfile;
        if (CombatActionOverride->Definition.SourcePolicy ==
                EGridCombatActionSourcePolicy::Equipment ||
            CombatActionOverride->Definition.SourcePolicy ==
                EGridCombatActionSourcePolicy::QuickItem)
        {
            OffensiveItemDefinitionId =
                CombatActionOverride->SourceDefinitionId;
        }
        if (CombatActionOverride->Definition.SourcePolicy ==
            EGridCombatActionSourcePolicy::Equipment)
        {
            OffensiveEquipmentSlot =
                CombatActionOverride->SourceEquipmentSlot;
        }
    }
    else if (!ResolvePlayerOffensiveProfile (
        PartyPawn->PartyInventoryComponent,
        AttackerCharacterIndex,
        RequestedEquipmentSlot,
        bRequireRequestedEquipmentSlot,
        OffensiveProfile,
        OffensiveItemDefinitionId,
        OffensiveEquipmentSlot,
        OffensiveProfileRejectReason))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            OffensiveProfileRejectReason,
            OutRejectReason);
    }

    const FIntPoint PartyCell (
        PartyPawn->CurrentCellX,
        PartyPawn->CurrentCellY);

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

    FIntPoint SearchCell = PartyCell;
    FIntPoint TargetCell = PartyCell;
    AGridMonsterActor* TargetMonster = nullptr;
    int32 TargetDistance = INDEX_NONE;
    const int32 SearchDistance = OffensiveProfile.RangeCells + 1;
    for (int32 Distance = 1; Distance <= SearchDistance; ++Distance)
    {
        int32 NextCellX = INDEX_NONE;
        int32 NextCellY = INDEX_NONE;
        if (!RuntimeActor->TryGetNeighborCell (
            SearchCell.X,
            SearchCell.Y,
            PartyPawn->Facing,
            NextCellX,
            NextCellY))
        {
            if (Distance == 1)
            {
                return RejectPlayerAttack (
                    AttackerCharacterIndex,
                    EGridPlayerAttackRejectReason::TargetCellUnavailable,
                    OutRejectReason);
            }
            break;
        }
        if (!RuntimeActor->CanMove (
            SearchCell.X,
            SearchCell.Y,
            PartyPawn->Facing))
        {
            return RejectPlayerAttack (
                AttackerCharacterIndex,
                EGridPlayerAttackRejectReason::PassageBlocked,
                OutRejectReason);
        }

        SearchCell = FIntPoint (NextCellX, NextCellY);
        if (AGridMonsterActor* Occupant =
            Occupancy->GetOccupantAtCell (SearchCell))
        {
            TargetMonster = Occupant;
            TargetCell = SearchCell;
            TargetDistance = Distance;
            break;
        }
    }

    if (!IsValid (TargetMonster))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::NoMonsterInFront,
            OutRejectReason);
    }

    if (TargetDistance > OffensiveProfile.RangeCells)
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetOutOfRange,
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
    if (TargetMonster->IsDead ())
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
    if (GridDistance < 1 ||
        GridDistance > OffensiveProfile.RangeCells ||
        GridDistance != TargetDistance)
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetOutOfRange,
            OutRejectReason);
    }

    FGridInventoryCharacterSummary CharacterSummary;
    if (!PartyPawn->PartyInventoryComponent->GetCharacterSummary (
        AttackerCharacterIndex,
        CharacterSummary))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::InvalidAttacker,
            OutRejectReason);
    }
    if (!IsValid (TargetMonster->MonsterDefinition))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetInactive,
            OutRejectReason);
    }

    FGridAttackSourceStats Source;
    FGridAttackTargetStats Target;
    FGridAttackDefinition AttackDefinition;
    if (!BuildPlayerAttackResolutionInputs (
        CharacterSummary,
        TargetMonster,
        OffensiveProfile,
        Source,
        Target,
        AttackDefinition))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetInactive,
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
    Request.RangeCells = OffensiveProfile.RangeCells;
    Request.AttackId = OffensiveProfile.AttackId;
    Request.OffensiveItemDefinitionId =
        OffensiveItemDefinitionId;
    Request.OffensiveEquipmentSlot =
        OffensiveEquipmentSlot;
    Request.ActionPointCost = SafeAttackActionPointCost;
    if (!Request.IsValid ())
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::TargetCellUnavailable,
            OutRejectReason);
    }

    if (!SpendPlayerCharacterActionPoints (
        AttackerCharacterIndex,
        Request.ActionPointCost))
    {
        return RejectPlayerAttack (
            AttackerCharacterIndex,
            EGridPlayerAttackRejectReason::InsufficientActionPoints,
            OutRejectReason);
    }

    const FGridAttackResult Result =
        FGridCombatResolver::ResolveAttack (
            Source,
            Target,
            AttackDefinition,
            CombatRandomStream);

    OutRequest = Request;
    OutResult = Result;
    OutRejectReason = EGridPlayerAttackRejectReason::None;
    LastPlayerAttackRequest = Request;
    LastPlayerAttackResult = Result;
    LastPlayerAttackRejectReason = EGridPlayerAttackRejectReason::None;

    UE_LOG (LogGridTurnManager, Log,
        TEXT ("[GridPlayerAttack] Accepted=true Request=%s Round=%d Attacker=%d Character=%s Target=%s PartyCell=(%d,%d) TargetCell=(%d,%d) Facing=%s Range=%d Attack=%s Item=%s Slot=%s APCost=%d"),
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
        Request.RangeCells,
        *Request.AttackId.ToString (),
        *Request.OffensiveItemDefinitionId.ToString (),
        *UEnum::GetValueAsString (Request.OffensiveEquipmentSlot),
        Request.ActionPointCost);
    ++PlayerAttackRequestedBroadcastCount;
    OnPlayerAttackRequested.Broadcast (Request);

    FGridCombatLogEntry AttackEntry;
    AttackEntry.RoundNumber = RoundNumber;
    AttackEntry.Phase = CurrentPhase;
    AttackEntry.Type = Result.bHit
        ? EGridCombatLogEntryType::AttackHit
        : EGridCombatLogEntryType::AttackMiss;
    AttackEntry.SourceId = FName (*Attacker.CharacterId.ToString (
        EGuidFormats::Digits));
    AttackEntry.SourceDisplayName = CharacterSummary.DisplayName;
    AttackEntry.TargetId = FName (*TargetMonsterId.ToString (
        EGuidFormats::Digits));
    AttackEntry.TargetDisplayName =
        ResolveMonsterDisplayName (TargetMonster);
    AttackEntry.TargetCharacterIndex = INDEX_NONE;
    AttackEntry.AttackId = Request.AttackId;
    AttackEntry.OffensiveItemDefinitionId =
        Request.OffensiveItemDefinitionId;
    AttackEntry.OffensiveEquipmentSlot =
        Request.OffensiveEquipmentSlot;
    AttackEntry.AttackResult = Result;
    AttackEntry.bTargetDefeated =
        Result.TargetHealthBefore > 0 &&
        Result.TargetHealthAfter <= 0;
    AttackEntry.Message = FGridCombatLogFormatter::FormatPlayerAttack (
        AttackEntry.SourceDisplayName,
        AttackEntry.TargetDisplayName,
        AttackEntry.AttackId,
        Result);
    AppendCombatLogEntry (AttackEntry);

    bPlayerAttackResolutionInProgress = true;
    TargetMonster->ApplyAttackResult (Result);
    if (FGridCombatantInitiativeEntry* TargetEntry =
        FindInitiativeEntry (
            EGridCombatantSide::Monster,
            TargetMonsterId))
    {
        const int32 PreviousHealth = TargetEntry->CurrentHealth;
        RefreshInitiativeEntryVitals (*TargetEntry);
        if (TargetEntry->CurrentHealth != PreviousHealth &&
            TargetEntry->State != EGridCombatantTurnState::Defeated)
        {
            OnCombatantStateChanged.Broadcast (*TargetEntry);
        }
    }
    ++PlayerAttackResolvedBroadcastCount;
    bPlayerAttackResolutionInProgress = false;
    OnPlayerAttackResolved.Broadcast (Request, TargetMonster, Result);

    if (bCollectRuntimeMetrics)
    {
        ++RuntimeMetrics.AttacksResolved;
    }

    if (bPendingVictoryAfterPlayerAttack)
    {
        bPendingVictoryAfterPlayerAttack = false;
        FinishCombat (EGridCombatPhase::Victory);
    }
    else if (!InitiativeOrder.IsEmpty ())
    {
        FGridPlayerCharacterTurnState TurnState;
        if (GetPlayerCharacterTurnState (
                AttackerCharacterIndex,
                TurnState) &&
            TurnState.RemainingActionPoints <= 0 &&
            IsActivePlayerCharacter (AttackerCharacterIndex))
        {
            FinishActivePlayerTurn ();
        }
    }
    return true;
}

bool UGridTurnManagerComponent::BuildPlayerAttackResolutionInputs (
    const FGridInventoryCharacterSummary& CharacterSummary,
    const AGridMonsterActor* TargetMonster,
    const FGridOffensiveEquipmentProfile& OffensiveProfile,
    FGridAttackSourceStats& OutSource,
    FGridAttackTargetStats& OutTarget,
    FGridAttackDefinition& OutAttackDefinition) const
{
    OutSource = FGridAttackSourceStats ();
    OutTarget = FGridAttackTargetStats ();
    OutAttackDefinition = FGridAttackDefinition ();
    if (!IsValid (TargetMonster) ||
        !IsValid (TargetMonster->MonsterDefinition) ||
        !OffensiveProfile.IsValid ())
    {
        return false;
    }

    OutSource.Accuracy = CharacterSummary.DerivedStats.Accuracy;
    OutSource.DamageBonus = OffensiveProfile.FlatDamageBonus;
    if (OffensiveProfile.DamageScalingAttribute !=
        EGridAttackScalingAttribute::None)
    {
        OutSource.DamageBonus +=
            URPGCharacterRulesLibrary::GetAttributeModifier (
                GetScalingAttributeValue (
                    CharacterSummary.Attributes,
                    OffensiveProfile.DamageScalingAttribute));
    }

    OutAttackDefinition = OffensiveProfile.AttackDefinition;

    OutTarget.Evasion = TargetMonster->MonsterDefinition->Evasion;
    OutTarget.CurrentHealth = TargetMonster->CurrentHealth;
    OutTarget.PhysicalArmor =
        TargetMonster->CurrentPhysicalArmor;
    OutTarget.MagicalArmor =
        TargetMonster->CurrentMagicalArmor;
    OutTarget.ResistancePercent = 0;
    OutTarget.DamageMultiplier =
        TargetMonster->MonsterDefinition->GetDamageMultiplier (
            OutAttackDefinition.DamageType,
            OutAttackDefinition.PhysicalSubtype);
    return OutAttackDefinition.IsValid ();
}

bool UGridTurnManagerComponent::ResolvePlayerOffensiveProfile (
    const UGridPartyInventoryComponent* PartyInventory,
    int32 AttackerCharacterIndex,
    EGridEquipmentSlot RequestedEquipmentSlot,
    bool bRequireRequestedEquipmentSlot,
    FGridOffensiveEquipmentProfile& OutProfile,
    FName& OutItemDefinitionId,
    EGridEquipmentSlot& OutEquipmentSlot,
    EGridPlayerAttackRejectReason& OutRejectReason) const
{
    OutProfile = FGridOffensiveEquipmentProfile ();
    OutItemDefinitionId = NAME_None;
    OutEquipmentSlot = EGridEquipmentSlot::None;
    OutRejectReason = EGridPlayerAttackRejectReason::None;
    if (!IsValid (PartyInventory))
    {
        OutRejectReason =
            EGridPlayerAttackRejectReason::PartyUnavailable;
        return false;
    }

    if (bRequireRequestedEquipmentSlot)
    {
        if (RequestedEquipmentSlot != EGridEquipmentSlot::MainHand &&
            RequestedEquipmentSlot != EGridEquipmentSlot::OffHand)
        {
            OutRejectReason =
                EGridPlayerAttackRejectReason::InvalidOffensiveEquipment;
            return false;
        }

        FGridItemInstance EquippedItem;
        if (!PartyInventory->GetEquippedItem (
            AttackerCharacterIndex,
            RequestedEquipmentSlot,
            EquippedItem))
        {
            OutProfile = MakeUnarmedOffensiveProfile ();
            return true;
        }

        const UGridItemDefinitionAsset* Definition =
            PartyInventory->FindItemDefinition (
                EquippedItem.ItemDefinitionId);
        if (!IsValid (Definition))
        {
            OutRejectReason =
                EGridPlayerAttackRejectReason::
                    EquippedItemDefinitionUnavailable;
            return false;
        }
        if (!Definition->IsValidDefinition ())
        {
            OutRejectReason =
                EGridPlayerAttackRejectReason::
                    InvalidOffensiveEquipment;
            return false;
        }
        if (!Definition->CanProvideAttackFromSlot (
            RequestedEquipmentSlot))
        {
            OutRejectReason =
                EGridPlayerAttackRejectReason::
                    InvalidOffensiveEquipment;
            return false;
        }

        if (!ResolveMON12ItemAttackProfile (
            Definition,
            OutProfile))
        {
            OutRejectReason =
                EGridPlayerAttackRejectReason::
                    InvalidOffensiveEquipment;
            return false;
        }
        OutItemDefinitionId = EquippedItem.ItemDefinitionId;
        OutEquipmentSlot = RequestedEquipmentSlot;
        return true;
    }

    const EGridEquipmentSlot HandSlots[] = {
        EGridEquipmentSlot::MainHand,
        EGridEquipmentSlot::OffHand
    };
    for (const EGridEquipmentSlot HandSlot : HandSlots)
    {
        FGridItemInstance EquippedItem;
        if (!PartyInventory->GetEquippedItem (
            AttackerCharacterIndex,
            HandSlot,
            EquippedItem))
        {
            continue;
        }

        const UGridItemDefinitionAsset* Definition =
            PartyInventory->FindItemDefinition (
                EquippedItem.ItemDefinitionId);
        if (!IsValid (Definition))
        {
            OutRejectReason =
                EGridPlayerAttackRejectReason::
                    EquippedItemDefinitionUnavailable;
            return false;
        }

        if (!DoesMON12ItemDeclareAttack (Definition))
        {
            continue;
        }
        if (!Definition->IsValidDefinition () ||
            !Definition->CanProvideAttackFromSlot (HandSlot))
        {
            OutRejectReason =
                EGridPlayerAttackRejectReason::
                    InvalidOffensiveEquipment;
            return false;
        }

        if (!ResolveMON12ItemAttackProfile (
            Definition,
            OutProfile))
        {
            OutRejectReason =
                EGridPlayerAttackRejectReason::
                    InvalidOffensiveEquipment;
            return false;
        }

        OutItemDefinitionId = EquippedItem.ItemDefinitionId;
        OutEquipmentSlot = HandSlot;
        return true;
    }

    OutProfile = MakeUnarmedOffensiveProfile ();
    return true;
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
    ++PlayerAttackRejectedBroadcastCount;
    OnPlayerAttackRejected.Broadcast (
        AttackerCharacterIndex,
        RejectReason);
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
