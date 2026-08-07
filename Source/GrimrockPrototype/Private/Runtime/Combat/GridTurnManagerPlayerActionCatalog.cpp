#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "RPG/RPGClassAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
    void AddMON126Contribution (
        const FGridCombatActionDefinition& Definition,
        FName SourceDefinitionId,
        const FGuid& SourceRuntimeId,
        EGridEquipmentSlot SourceEquipmentSlot,
        int32 AvailableSourceQuantity,
        TArray<FGridCombatActionContribution>& OutContributions)
    {
        FGridCombatActionContribution Contribution;
        Contribution.Definition = Definition;
        Contribution.SourceDefinitionId = SourceDefinitionId;
        Contribution.SourceRuntimeId = SourceRuntimeId;
        Contribution.SourceEquipmentSlot = SourceEquipmentSlot;
        Contribution.AvailableSourceQuantity =
            FMath::Max (0, AvailableSourceQuantity);
        if (Contribution.IsValid ())
        {
            OutContributions.Add (MoveTemp (Contribution));
        }
    }

    FString GetMON126AvailabilityText (
        const FGridAvailableCombatAction& Action)
    {
        return Action.bEnabled
            ? TEXT ("Enabled")
            : UEnum::GetValueAsString (Action.AvailabilityReason);
    }

    bool DoesMON126CatalogItemDeclareAttack (
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
}

void UGridTurnManagerComponent::BuildPlayerCombatActionContributions (
    int32 CharacterIndex,
    TArray<FGridCombatActionContribution>& OutContributions) const
{
    OutContributions.Reset ();
    const UGridPartyInventoryComponent* Inventory =
        IsValid (PartyPawn)
            ? PartyPawn->PartyInventoryComponent.Get ()
            : nullptr;
    if (!IsValid (Inventory) ||
        !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex (
            CharacterIndex))
    {
        return;
    }

    const FGridCharacterInventoryState& Character =
        Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
    if (!Character.ClassDefinition.IsNull ())
    {
        const URPGClassAsset* ClassDefinition =
            Character.ClassDefinition.LoadSynchronous ();
        if (IsValid (ClassDefinition) &&
            ClassDefinition->IsValidDefinition () &&
            ClassDefinition->ClassId == Character.ClassId)
        {
            for (const FGridCombatActionDefinition& Definition :
                ClassDefinition->CombatActions)
            {
                AddMON126Contribution (
                    Definition,
                    ClassDefinition->ClassId,
                    FGuid (),
                    EGridEquipmentSlot::None,
                    1,
                    OutContributions);
            }
        }
    }

    bool bHasEquipmentAttack = false;
    bool bHasInvalidEquipmentAttackSource = false;
    const EGridEquipmentSlot HandSlots[] = {
        EGridEquipmentSlot::MainHand,
        EGridEquipmentSlot::OffHand
    };
    for (const EGridEquipmentSlot HandSlot : HandSlots)
    {
        FGridItemInstance EquippedItem;
        if (!Inventory->GetEquippedItem (
            CharacterIndex,
            HandSlot,
            EquippedItem))
        {
            continue;
        }

        const UGridItemDefinitionAsset* ItemDefinition =
            Inventory->FindItemDefinition (
                EquippedItem.ItemDefinitionId);
        if (!IsValid (ItemDefinition))
        {
            bHasInvalidEquipmentAttackSource = true;
            continue;
        }
        const bool bDeclaresAttack =
            DoesMON126CatalogItemDeclareAttack (ItemDefinition);
        if (!ItemDefinition->IsValidDefinition () ||
            !ItemDefinition->CompatibleEquipmentSlots.Contains (HandSlot))
        {
            bHasInvalidEquipmentAttackSource =
                bHasInvalidEquipmentAttackSource || bDeclaresAttack;
            continue;
        }

        if (!ItemDefinition->CombatActions.IsEmpty ())
        {
            for (const FGridCombatActionDefinition& SourceAction :
                ItemDefinition->CombatActions)
            {
                FGridCombatActionDefinition Definition = SourceAction;
                if (Definition.DisplayName.IsEmpty ())
                {
                    Definition.DisplayName = ItemDefinition->DisplayName;
                }
                if (Definition.Description.IsEmpty ())
                {
                    Definition.Description = ItemDefinition->Description;
                }
                if (Definition.Icon.IsNull ())
                {
                    Definition.Icon = ItemDefinition->Icon;
                }
                if (Definition.IsValid () &&
                    Definition.SourcePolicy ==
                        EGridCombatActionSourcePolicy::Equipment &&
                    Definition.ResolutionProfile ==
                        EGridCombatActionResolutionProfile::Attack)
                {
                    bHasEquipmentAttack = true;
                }
                AddMON126Contribution (
                    Definition,
                    EquippedItem.ItemDefinitionId,
                    EquippedItem.RuntimeObjectId,
                    HandSlot,
                    EquippedItem.Quantity,
                    OutContributions);
            }
        }
        else if (ItemDefinition->CanProvideAttackFromSlot (HandSlot))
        {
            const FGridCombatActionDefinition LegacyDefinition =
                FGridCombatActionCatalog::
                    MakeLegacyEquipmentAttackDefinition (
                        *ItemDefinition,
                        PlayerAttackActionPointCost);
            bHasEquipmentAttack =
                bHasEquipmentAttack || LegacyDefinition.IsValid ();
            AddMON126Contribution (
                LegacyDefinition,
                EquippedItem.ItemDefinitionId,
                EquippedItem.RuntimeObjectId,
                HandSlot,
                EquippedItem.Quantity,
                OutContributions);
        }
    }

    if (!bHasEquipmentAttack &&
        !bHasInvalidEquipmentAttackSource)
    {
        FGridCombatActionDefinition UnarmedAction = FGridCombatActionCatalog::MakeUnarmedAttackDefinition (PlayerAttackActionPointCost);
        UnarmedAction.Icon = UnarmedAttackIcon;
        AddMON126Contribution (UnarmedAction, NAME_None, FGuid (), EGridEquipmentSlot::None, 0, OutContributions);
    }

    // A quick-item contribution is definition-based rather than stack-based.
    // Include configured hotbar sources even at quantity zero so a persistent
    // shortcut stays resolved, disabled, and ready for a replacement stack.
    TArray<FName> QuickItemDefinitionIds;
    for (const FGridInventorySlot& InventorySlot :
        Character.InventorySlots)
    {
        if (!InventorySlot.IsEmpty ())
        {
            QuickItemDefinitionIds.AddUnique (
                InventorySlot.Item.ItemDefinitionId);
        }
    }
    for (const FGridCombatHotbarBinding& Binding :
        Character.CombatHotbarSlots)
    {
        if (!Binding.IsEmpty () &&
            Binding.SourcePolicy ==
                EGridCombatActionSourcePolicy::QuickItem &&
            !Binding.SourceDefinitionId.IsNone ())
        {
            QuickItemDefinitionIds.AddUnique (
                Binding.SourceDefinitionId);
        }
    }
    QuickItemDefinitionIds.Sort (
        [] (const FName Left, const FName Right)
        {
            return Left.ToString () < Right.ToString ();
        });

    for (const FName ItemDefinitionId : QuickItemDefinitionIds)
    {
        const UGridItemDefinitionAsset* ItemDefinition =
            Inventory->FindItemDefinition (ItemDefinitionId);
        FGridCombatActionDefinition QuickItemAction;
        if (!IsValid (ItemDefinition) ||
            !ItemDefinition->BuildQuickItemCombatActionDefinition (
                QuickItemAction))
        {
            continue;
        }

        AddMON126Contribution (
            QuickItemAction,
            ItemDefinitionId,
            FGuid (),
            EGridEquipmentSlot::None,
            Inventory->CountItemDefinitionInCharacterInventory (
                CharacterIndex,
                ItemDefinitionId),
            OutContributions);
    }
}

void UGridTurnManagerComponent::GetAvailableCombatActions (
    int32 CharacterIndex,
    TArray<FGridAvailableCombatAction>& OutActions) const
{
    OutActions.Reset ();
    const UGridPartyInventoryComponent* Inventory =
        IsValid (PartyPawn)
            ? PartyPawn->PartyInventoryComponent.Get ()
            : nullptr;
    if (!IsValid (Inventory) ||
        !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex (
            CharacterIndex))
    {
        return;
    }

    const FGridCharacterInventoryState& Character =
        Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
    FGridPlayerCharacterTurnState TurnState;
    const bool bHasTurnState = GetPlayerCharacterTurnState (
        CharacterIndex,
        TurnState);

    FGridCombatActionCatalogContext Context;
    Context.CharacterIndex = CharacterIndex;
    Context.CharacterId = Character.CharacterId;
    Context.bCombatActive = bCombatActive;
    Context.bCharacterDefeated =
        Character.DerivedStats.CurrentHealth <= 0;
    Context.bActiveCombatant =
        bHasTurnState &&
        TurnState.State == EGridCombatantTurnState::Active &&
        CurrentPhase == EGridCombatPhase::PlayerPhase;
    Context.bPartyBusy =
        !IsPartyAtRest () ||
        bPlayerAttackResolutionInProgress ||
        IsPartyMotionInProgress ();
    Context.RemainingActionPoints = bHasTurnState
        ? FMath::Max (0, TurnState.RemainingActionPoints)
        : 0;
    FGridInventoryCharacterSummary CharacterSummary;
    if (Inventory->GetCharacterSummary (
            CharacterIndex,
            CharacterSummary))
    {
        Context.CurrentHealth = FMath::Max (
            0,
            CharacterSummary.DerivedStats.CurrentHealth);
        Context.MaximumHealth = FMath::Max (
            0,
            CharacterSummary.DerivedStats.MaxHealth);
        Context.CurrentMana = FMath::Max (
            0,
            CharacterSummary.DerivedStats.CurrentMana);
        Context.MaximumMana = FMath::Max (
            0,
            CharacterSummary.DerivedStats.MaxMana);
    }
    Context.bEnableQuickItemExecutors = true;
    if (!Character.ClassId.IsNone ())
    {
        Context.SatisfiedRequirements.Add (Character.ClassId);
    }

    if (Inventory->PartyInventoryState.ActiveEquipment.IsValidIndex (
        CharacterIndex))
    {
        const FGridCharacterEquipmentState& Equipment =
            Inventory->PartyInventoryState.ActiveEquipment[CharacterIndex];
        const EGridEquipmentSlot HandSlots[] = {
            EGridEquipmentSlot::MainHand,
            EGridEquipmentSlot::OffHand
        };
        for (const EGridEquipmentSlot HandSlot : HandSlots)
        {
            const FGridItemInstance* Item = Equipment.GetSlot (HandSlot);
            const UGridItemDefinitionAsset* Definition =
                Item && Item->IsValid ()
                ? Inventory->FindItemDefinition (
                    Item->ItemDefinitionId)
                : nullptr;
            if (!IsValid (Definition))
            {
                continue;
            }
            for (const FName ItemTag : Definition->ItemTags)
            {
                Context.SatisfiedRequirements.Add (ItemTag);
            }
        }
    }

    TArray<FGridCombatActionContribution> Contributions;
    BuildPlayerCombatActionContributions (
        CharacterIndex,
        Contributions);
    FGridCombatActionCatalog::Build (
        Context,
        Contributions,
        OutActions);
    for (FGridAvailableCombatAction& Action : OutActions)
    {
        ResolveSuggestedCombatActionTarget (Action);
    }
}

void UGridTurnManagerComponent::ResolveSuggestedCombatActionTarget (
    FGridAvailableCombatAction& Action) const
{
    Action.SuggestedTargetId = FGuid ();
    Action.SuggestedTargetCell = FIntPoint::ZeroValue;
    if (!IsValid (RuntimeActor) ||
        !IsValid (PartyPawn) ||
        Action.Definition.TargetingPolicy !=
            EGridCombatTargetingPolicy::FirstAxialTarget ||
        Action.Definition.RangeCells <= 0)
    {
        return;
    }

    UWorld* World = GetWorld ();
    UGridMonsterOccupancySubsystem* Occupancy = World
        ? World->GetSubsystem<UGridMonsterOccupancySubsystem> ()
        : nullptr;
    if (!IsValid (Occupancy))
    {
        return;
    }

    FIntPoint SearchCell (
        PartyPawn->CurrentCellX,
        PartyPawn->CurrentCellY);
    for (int32 Distance = 1;
        Distance <= Action.Definition.RangeCells;
        ++Distance)
    {
        int32 NextCellX = INDEX_NONE;
        int32 NextCellY = INDEX_NONE;
        if (!RuntimeActor->TryGetNeighborCell (
                SearchCell.X,
                SearchCell.Y,
                PartyPawn->Facing,
                NextCellX,
                NextCellY) ||
            !RuntimeActor->CanMove (
                SearchCell.X,
                SearchCell.Y,
                PartyPawn->Facing))
        {
            return;
        }
        SearchCell = FIntPoint (NextCellX, NextCellY);
        AGridMonsterActor* Monster =
            Occupancy->GetOccupantAtCell (SearchCell);
        if (!IsValid (Monster))
        {
            continue;
        }
        if (IsCombatMonster (Monster) &&
            Monster->bMonsterEnabled &&
            !Monster->IsDead ())
        {
            Action.SuggestedTargetId =
                Monster->ResolvePersistenceId ();
            Action.SuggestedTargetCell = SearchCell;
        }
        return;
    }
}

bool UGridTurnManagerComponent::CommitQuickItemResourcesAfterAttack (
    const FGridAvailableCombatAction& Action,
    FGridCombatQuickItemResult& OutResult)
{
    OutResult = FGridCombatQuickItemResult ();
    UGridPartyInventoryComponent* Inventory = IsValid (PartyPawn)
        ? PartyPawn->PartyInventoryComponent.Get ()
        : nullptr;
    if (!IsValid (Inventory) ||
        Action.Definition.SourcePolicy !=
            EGridCombatActionSourcePolicy::QuickItem ||
        Action.SourceDefinitionId.IsNone () ||
        !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex (
            Action.CharacterIndex))
    {
        return false;
    }

    FGridCharacterInventoryState& Character =
        Inventory->PartyInventoryState.ActiveCharacters[
            Action.CharacterIndex];
    OutResult.SourceQuantityBefore =
        Inventory->CountItemDefinitionInCharacterInventory (
            Action.CharacterIndex,
            Action.SourceDefinitionId);
    OutResult.HealthBefore = Character.DerivedStats.CurrentHealth;
    OutResult.HealthAfter = OutResult.HealthBefore;
    OutResult.ManaBefore = Character.DerivedStats.CurrentMana;
    const int32 QuantityCost =
        Action.CurrentSourceItemQuantityCost;
    const int32 ManaCost = Action.CurrentManaCost;
    if (QuantityCost <= 0 ||
        OutResult.SourceQuantityBefore < QuantityCost ||
        OutResult.ManaBefore < ManaCost)
    {
        return false;
    }

    Character.DerivedStats.CurrentMana = FMath::Max (
        0,
        OutResult.ManaBefore - ManaCost);
    if (!Inventory->RemoveItemDefinitionFromCharacterInventory (
            Action.CharacterIndex,
            Action.SourceDefinitionId,
            QuantityCost))
    {
        Character.DerivedStats.CurrentMana = OutResult.ManaBefore;
        return false;
    }

    OutResult.SourceQuantityAfter =
        Inventory->CountItemDefinitionInCharacterInventory (
            Action.CharacterIndex,
            Action.SourceDefinitionId);
    OutResult.ManaAfter = Character.DerivedStats.CurrentMana;
    return true;
}

bool UGridTurnManagerComponent::RequestCharacterQuickItemEffect (
    const FGridAvailableCombatAction& Action,
    FGridCombatQuickItemResult& OutResult)
{
    OutResult = FGridCombatQuickItemResult ();
    UGridPartyInventoryComponent* Inventory = IsValid (PartyPawn)
        ? PartyPawn->PartyInventoryComponent.Get ()
        : nullptr;
    if (!IsValid (Inventory) ||
        !Action.bEnabled ||
        Action.Definition.SourcePolicy !=
            EGridCombatActionSourcePolicy::QuickItem ||
        Action.Definition.ResolutionProfile !=
            EGridCombatActionResolutionProfile::Effect ||
        Action.Definition.TargetingPolicy !=
            EGridCombatTargetingPolicy::Self ||
        !Action.Definition.EffectProfile.IsValid () ||
        Action.SourceDefinitionId.IsNone () ||
        !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex (
            Action.CharacterIndex))
    {
        return false;
    }

    FGridCharacterInventoryState& Character =
        Inventory->PartyInventoryState.ActiveCharacters[
            Action.CharacterIndex];
    FGridInventoryCharacterSummary Summary;
    FGridPlayerCharacterTurnState TurnStateBefore;
    if (!Inventory->GetCharacterSummary (
            Action.CharacterIndex,
            Summary) ||
        !GetPlayerCharacterTurnState (
            Action.CharacterIndex,
            TurnStateBefore))
    {
        return false;
    }

    OutResult.SourceQuantityBefore =
        Inventory->CountItemDefinitionInCharacterInventory (
            Action.CharacterIndex,
            Action.SourceDefinitionId);
    OutResult.HealthBefore = Character.DerivedStats.CurrentHealth;
    OutResult.ManaBefore = Character.DerivedStats.CurrentMana;
    const int32 QuantityCost =
        Action.CurrentSourceItemQuantityCost;
    const int32 ManaCost = Action.CurrentManaCost;
    if (QuantityCost <= 0 ||
        OutResult.SourceQuantityBefore < QuantityCost ||
        OutResult.ManaBefore < ManaCost ||
        !CanCharacterSpendActionPoints (
            Action.CharacterIndex,
            Action.CurrentActionPointCost))
    {
        return false;
    }

    OutResult.HealthAfter = FMath::Clamp (
        OutResult.HealthBefore +
            Action.Definition.EffectProfile.RestoreHealth,
        0,
        FMath::Max (0, Summary.DerivedStats.MaxHealth));
    OutResult.ManaAfter = FMath::Clamp (
        OutResult.ManaBefore - ManaCost +
            Action.Definition.EffectProfile.RestoreMana,
        0,
        FMath::Max (0, Summary.DerivedStats.MaxMana));
    if (OutResult.HealthAfter <= OutResult.HealthBefore &&
        OutResult.ManaAfter <= OutResult.ManaBefore)
    {
        return false;
    }

    if (!SpendPlayerCharacterActionPoints (
            Action.CharacterIndex,
            Action.CurrentActionPointCost))
    {
        return false;
    }

    Character.DerivedStats.CurrentHealth = OutResult.HealthAfter;
    Character.DerivedStats.CurrentMana = OutResult.ManaAfter;
    if (!Inventory->RemoveItemDefinitionFromCharacterInventory (
            Action.CharacterIndex,
            Action.SourceDefinitionId,
            QuantityCost))
    {
        Character.DerivedStats.CurrentHealth = OutResult.HealthBefore;
        Character.DerivedStats.CurrentMana = OutResult.ManaBefore;
        if (FGridPlayerCharacterTurnState* RestoredTurnState =
            EnsurePlayerCharacterTurnState (Action.CharacterIndex))
        {
            *RestoredTurnState = TurnStateBefore;
            BroadcastPlayerCharacterTurnState (*RestoredTurnState);
        }
        return false;
    }

    OutResult.SourceQuantityAfter =
        Inventory->CountItemDefinitionInCharacterInventory (
            Action.CharacterIndex,
            Action.SourceDefinitionId);
    if (FGridCombatantInitiativeEntry* Entry = FindInitiativeEntry (
            EGridCombatantSide::Party,
            Character.CharacterId))
    {
        RefreshInitiativeEntryVitals (*Entry);
        OnCombatantStateChanged.Broadcast (*Entry);
    }

    FGridPlayerCharacterTurnState TurnStateAfter;
    if (!InitiativeOrder.IsEmpty () &&
        GetPlayerCharacterTurnState (
            Action.CharacterIndex,
            TurnStateAfter) &&
        TurnStateAfter.RemainingActionPoints <= 0 &&
        IsActivePlayerCharacter (Action.CharacterIndex))
    {
        FinishActivePlayerTurn ();
    }
    return true;
}

bool UGridTurnManagerComponent::RequestCharacterCombatAction (
    int32 CharacterIndex,
    FName ActionId,
    EGridCombatActionSourcePolicy SourcePolicy,
    FName SourceDefinitionId,
    EGridEquipmentSlot SourceEquipmentSlot,
    FGridCombatActionRequestResult& OutResult)
{
    OutResult = FGridCombatActionRequestResult ();
    if (!bInitialized)
    {
        OutResult.RejectReason =
            EGridCombatActionRequestRejectReason::
                TurnManagerNotInitialized;
        return false;
    }

    TArray<FGridAvailableCombatAction> AvailableActions;
    GetAvailableCombatActions (CharacterIndex, AvailableActions);
    const FGridAvailableCombatAction* Action =
        AvailableActions.FindByPredicate (
            [ActionId,
                SourcePolicy,
                SourceDefinitionId,
                SourceEquipmentSlot]
            (const FGridAvailableCombatAction& Candidate)
            {
                return Candidate.MatchesSource (
                    ActionId,
                    SourcePolicy,
                    SourceDefinitionId,
                    SourceEquipmentSlot);
            });
    if (!Action)
    {
        OutResult.RejectReason =
            EGridCombatActionRequestRejectReason::InvalidAction;
        UE_LOG (
            LogGridTurnManager,
            Log,
            TEXT ("[GridActionCatalog] Accepted=false Character=%d Action=%s Source=%s Slot=%s Reason=InvalidAction"),
            CharacterIndex,
            *ActionId.ToString (),
            *SourceDefinitionId.ToString (),
            *UEnum::GetValueAsString (SourceEquipmentSlot));
        return false;
    }

    OutResult.Action = *Action;
    if (!Action->bEnabled)
    {
        OutResult.RejectReason =
            EGridCombatActionRequestRejectReason::ActionUnavailable;
        UE_LOG (
            LogGridTurnManager,
            Log,
            TEXT ("[GridActionCatalog] Accepted=false Character=%d Action=%s Source=%s Slot=%s Reason=%s"),
            CharacterIndex,
            *ActionId.ToString (),
            *SourceDefinitionId.ToString (),
            *UEnum::GetValueAsString (SourceEquipmentSlot),
            *GetMON126AvailabilityText (*Action));
        return false;
    }

    if (Action->Definition.SourcePolicy ==
        EGridCombatActionSourcePolicy::QuickItem)
    {
        bool bAccepted = false;
        if (Action->Definition.ResolutionProfile ==
            EGridCombatActionResolutionProfile::Attack)
        {
            FGridPlayerAttackRequest AttackRequest;
            FGridAttackResult AttackResult;
            EGridPlayerAttackRejectReason AttackRejectReason =
                EGridPlayerAttackRejectReason::None;
            bAccepted = RequestCharacterAttackInternal (
                CharacterIndex,
                EGridEquipmentSlot::None,
                false,
                Action,
                AttackRequest,
                AttackResult,
                AttackRejectReason);
            OutResult.AttackRequest = AttackRequest;
            OutResult.AttackResult = AttackResult;
            OutResult.AttackRejectReason = AttackRejectReason;
            if (bAccepted &&
                !CommitQuickItemResourcesAfterAttack (
                    *Action,
                    OutResult.QuickItemResult))
            {
                UE_LOG (
                    LogGridTurnManager,
                    Error,
                    TEXT ("[GridQuickItem] PostAttackCommitFailed Character=%d Action=%s Source=%s"),
                    CharacterIndex,
                    *ActionId.ToString (),
                    *SourceDefinitionId.ToString ());
                bAccepted = false;
            }
            OutResult.RejectReason = bAccepted
                ? EGridCombatActionRequestRejectReason::None
                : AttackRejectReason !=
                        EGridPlayerAttackRejectReason::None
                    ? EGridCombatActionRequestRejectReason::AttackRejected
                    : EGridCombatActionRequestRejectReason::QuickItemRejected;
        }
        else if (Action->Definition.ResolutionProfile ==
            EGridCombatActionResolutionProfile::Effect)
        {
            bAccepted = RequestCharacterQuickItemEffect (
                *Action,
                OutResult.QuickItemResult);
            OutResult.RejectReason = bAccepted
                ? EGridCombatActionRequestRejectReason::None
                : EGridCombatActionRequestRejectReason::QuickItemRejected;
        }
        else
        {
            OutResult.RejectReason =
                EGridCombatActionRequestRejectReason::UnsupportedResolution;
        }

        OutResult.bAccepted = bAccepted;
        UE_LOG (
            LogGridTurnManager,
            Log,
            TEXT ("[GridQuickItem] Accepted=%s Character=%d Action=%s Source=%s APCost=%d ManaCost=%d Quantity=%d->%d Health=%d->%d Mana=%d->%d"),
            bAccepted ? TEXT ("true") : TEXT ("false"),
            CharacterIndex,
            *ActionId.ToString (),
            *SourceDefinitionId.ToString (),
            Action->CurrentActionPointCost,
            Action->CurrentManaCost,
            OutResult.QuickItemResult.SourceQuantityBefore,
            OutResult.QuickItemResult.SourceQuantityAfter,
            OutResult.QuickItemResult.HealthBefore,
            OutResult.QuickItemResult.HealthAfter,
            OutResult.QuickItemResult.ManaBefore,
            OutResult.QuickItemResult.ManaAfter);
        return bAccepted;
    }

    if (Action->Definition.ResolutionProfile !=
            EGridCombatActionResolutionProfile::Attack ||
        (Action->Definition.SourcePolicy !=
                EGridCombatActionSourcePolicy::Equipment &&
            Action->Definition.SourcePolicy !=
                EGridCombatActionSourcePolicy::Universal))
    {
        OutResult.RejectReason =
            EGridCombatActionRequestRejectReason::UnsupportedResolution;
        return false;
    }

    FGridPlayerAttackRequest AttackRequest;
    FGridAttackResult AttackResult;
    EGridPlayerAttackRejectReason AttackRejectReason =
        EGridPlayerAttackRejectReason::None;
    const bool bAccepted = RequestCharacterAttackInternal (
        CharacterIndex,
        Action->SourceEquipmentSlot,
        Action->Definition.SourcePolicy ==
            EGridCombatActionSourcePolicy::Equipment,
        Action,
        AttackRequest,
        AttackResult,
        AttackRejectReason);
    OutResult.bAccepted = bAccepted;
    OutResult.AttackRequest = AttackRequest;
    OutResult.AttackResult = AttackResult;
    OutResult.AttackRejectReason = AttackRejectReason;
    OutResult.RejectReason = bAccepted
        ? EGridCombatActionRequestRejectReason::None
        : EGridCombatActionRequestRejectReason::AttackRejected;
    UE_LOG (
        LogGridTurnManager,
        Log,
        TEXT ("[GridActionCatalog] Accepted=%s Character=%d Action=%s Source=%s Slot=%s APCost=%d ManaCost=%d"),
        bAccepted ? TEXT ("true") : TEXT ("false"),
        CharacterIndex,
        *ActionId.ToString (),
        *SourceDefinitionId.ToString (),
        *UEnum::GetValueAsString (SourceEquipmentSlot),
        Action->CurrentActionPointCost,
        Action->CurrentManaCost);
    return bAccepted;
}

void UGridTurnManagerComponent::LogAvailableCombatActions (
    int32 CharacterIndex) const
{
    const int32 ResolvedCharacterIndex = CharacterIndex == INDEX_NONE &&
        IsValid (PartyPawn) &&
        IsValid (PartyPawn->PartyInventoryComponent)
        ? PartyPawn->PartyInventoryComponent
            ->GetSelectedCharacterIndex ()
        : CharacterIndex;
    TArray<FGridAvailableCombatAction> Actions;
    GetAvailableCombatActions (ResolvedCharacterIndex, Actions);
    UE_LOG (
        LogGridTurnManager,
        Log,
        TEXT ("[GridActionCatalog] Character=%d Count=%d"),
        ResolvedCharacterIndex,
        Actions.Num ());
    for (const FGridAvailableCombatAction& Action : Actions)
    {
        UE_LOG (
            LogGridTurnManager,
            Log,
            TEXT ("[GridActionCatalog] Character=%d Action=%s SourcePolicy=%s Source=%s Slot=%s AP=%d Mana=%d Item=%d/%d Status=%s Target=%s Cell=(%d,%d)"),
            ResolvedCharacterIndex,
            *Action.Definition.ActionId.ToString (),
            *UEnum::GetValueAsString (
                Action.Definition.SourcePolicy),
            *Action.SourceDefinitionId.ToString (),
            *UEnum::GetValueAsString (
                Action.SourceEquipmentSlot),
            Action.CurrentActionPointCost,
            Action.CurrentManaCost,
            Action.CurrentSourceItemQuantityCost,
            Action.CurrentSourceItemQuantity,
            *GetMON126AvailabilityText (Action),
            *Action.SuggestedTargetId.ToString (
                EGuidFormats::Digits),
            Action.SuggestedTargetCell.X,
            Action.SuggestedTargetCell.Y);
    }
}

void UGridTurnManagerComponent::
LogSelectedCharacterAvailableCombatActions () const
{
    LogAvailableCombatActions (INDEX_NONE);
}
