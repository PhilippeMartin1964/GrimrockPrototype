#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "RPG/RPGClassAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatResolver.h"
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

    bool IsMON1285ClassActionSource (
        EGridCombatActionSourcePolicy SourcePolicy)
    {
        return SourcePolicy == EGridCombatActionSourcePolicy::Ability ||
            SourcePolicy == EGridCombatActionSourcePolicy::Spell;
    }

    bool IsMON1286ExplicitTargetingPolicy (
        EGridCombatTargetingPolicy TargetingPolicy)
    {
        return TargetingPolicy == EGridCombatTargetingPolicy::Cell ||
            TargetingPolicy == EGridCombatTargetingPolicy::Area;
    }

    bool IsMON1286TargetedSource (
        EGridCombatActionSourcePolicy SourcePolicy)
    {
        return SourcePolicy == EGridCombatActionSourcePolicy::Universal ||
            SourcePolicy == EGridCombatActionSourcePolicy::Ability ||
            SourcePolicy == EGridCombatActionSourcePolicy::Spell ||
            SourcePolicy == EGridCombatActionSourcePolicy::QuickItem;
    }

    FText MakeMON1286TargetingReason (const TCHAR* Reason)
    {
        return FText::FromString (Reason ? Reason : TEXT ("Cible invalide."));
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
                    Definition.SourcePolicy ==
                            EGridCombatActionSourcePolicy::Universal
                        ? NAME_None
                        : ClassDefinition->ClassId,
                    FGuid (),
                    EGridEquipmentSlot::None,
                    1,
                    OutContributions);
            }
        }
    }

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
            continue;
        }
        if (!ItemDefinition->IsValidDefinition () ||
            !ItemDefinition->CompatibleEquipmentSlots.Contains (HandSlot))
        {
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
            AddMON126Contribution (
                LegacyDefinition,
                EquippedItem.ItemDefinitionId,
                EquippedItem.RuntimeObjectId,
                HandSlot,
                EquippedItem.Quantity,
                OutContributions);
        }
    }

    // Unarmed is a manual universal action, not merely an automatic fallback.
    // Keep it in the catalogue even while a valid weapon is equipped so the
    // player can always drag it to the personal hotbar.
    FGridCombatActionDefinition UnarmedAction =
        FGridCombatActionCatalog::MakeUnarmedAttackDefinition (
            PlayerAttackActionPointCost);
    UnarmedAction.Icon = UnarmedAttackIcon;
    AddMON126Contribution (
        UnarmedAction,
        NAME_None,
        FGuid (),
        EGridEquipmentSlot::None,
        0,
        OutContributions);

    // A quick-item contribution is definition-based rather than stack-based,
    // but it only exists while at least one source item remains in inventory.
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
            !ItemDefinition->BuildInventoryCombatActionDefinition (
                PlayerAttackActionPointCost,
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
    Context.bEnableClassActionExecutors = true;
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
    Inventory->NotifyPartyInventoryChanged (Action.CharacterIndex);
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
    Inventory->NotifyPartyInventoryChanged (Action.CharacterIndex);
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

bool UGridTurnManagerComponent::RequestCharacterClassActionEffect (
    const FGridAvailableCombatAction& Action,
    FGridCombatClassActionResult& OutResult)
{
    OutResult = FGridCombatClassActionResult ();
    UGridPartyInventoryComponent* Inventory = IsValid (PartyPawn)
        ? PartyPawn->PartyInventoryComponent.Get ()
        : nullptr;
    if (!IsValid (Inventory) ||
        !Action.bEnabled ||
        !IsMON1285ClassActionSource (
            Action.Definition.SourcePolicy) ||
        Action.Definition.ResolutionProfile !=
            EGridCombatActionResolutionProfile::Effect ||
        Action.Definition.TargetingPolicy !=
            EGridCombatTargetingPolicy::Self ||
        !Action.Definition.EffectProfile.IsValid () ||
        Action.CurrentSourceItemQuantityCost != 0 ||
        !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex (
            Action.CharacterIndex))
    {
        return false;
    }

    FGridCharacterInventoryState& Character =
        Inventory->PartyInventoryState.ActiveCharacters[
            Action.CharacterIndex];
    FGridInventoryCharacterSummary Summary;
    if (!Inventory->GetCharacterSummary (
            Action.CharacterIndex,
            Summary) ||
        !CanCharacterSpendActionPoints (
            Action.CharacterIndex,
            Action.CurrentActionPointCost))
    {
        return false;
    }

    OutResult.HealthBefore = Character.DerivedStats.CurrentHealth;
    OutResult.ManaBefore = Character.DerivedStats.CurrentMana;
    if (OutResult.ManaBefore < Action.CurrentManaCost)
    {
        return false;
    }
    OutResult.HealthAfter = FMath::Clamp (
        OutResult.HealthBefore +
            Action.Definition.EffectProfile.RestoreHealth,
        0,
        FMath::Max (0, Summary.DerivedStats.MaxHealth));
    OutResult.ManaAfter = FMath::Clamp (
        OutResult.ManaBefore - Action.CurrentManaCost +
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
    Inventory->NotifyPartyInventoryChanged (Action.CharacterIndex);
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

bool UGridTurnManagerComponent::RequestCharacterClassActionAttack (
    const FGridAvailableCombatAction& Action,
    FGridPlayerAttackRequest& OutRequest,
    FGridAttackResult& OutResult,
    EGridPlayerAttackRejectReason& OutRejectReason,
    FGridCombatClassActionResult& OutClassResult)
{
    OutRequest = FGridPlayerAttackRequest ();
    OutResult = FGridAttackResult ();
    OutRejectReason = EGridPlayerAttackRejectReason::None;
    OutClassResult = FGridCombatClassActionResult ();
    UGridPartyInventoryComponent* Inventory = IsValid (PartyPawn)
        ? PartyPawn->PartyInventoryComponent.Get ()
        : nullptr;
    if (!IsValid (Inventory) ||
        !Action.bEnabled ||
        !IsMON1285ClassActionSource (
            Action.Definition.SourcePolicy) ||
        Action.Definition.ResolutionProfile !=
            EGridCombatActionResolutionProfile::Attack ||
        Action.Definition.TargetingPolicy !=
            EGridCombatTargetingPolicy::FirstAxialTarget ||
        Action.CurrentSourceItemQuantityCost != 0 ||
        !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex (
            Action.CharacterIndex))
    {
        return false;
    }

    FGridCharacterInventoryState& Character =
        Inventory->PartyInventoryState.ActiveCharacters[
            Action.CharacterIndex];
    OutClassResult.HealthBefore =
        Character.DerivedStats.CurrentHealth;
    OutClassResult.HealthAfter = OutClassResult.HealthBefore;
    OutClassResult.ManaBefore = Character.DerivedStats.CurrentMana;
    OutClassResult.ManaAfter = OutClassResult.ManaBefore;
    if (OutClassResult.ManaBefore < Action.CurrentManaCost)
    {
        return false;
    }

    // Reserve mana before the attack broadcasts its presentation events.
    // Every rejection rolls it back; AP are spent only by the accepted
    // attack pipeline after target validation.
    Character.DerivedStats.CurrentMana = FMath::Max (
        0,
        OutClassResult.ManaBefore - Action.CurrentManaCost);
    if (!RequestCharacterAttackInternal (
            Action.CharacterIndex,
            EGridEquipmentSlot::None,
            false,
            &Action,
            OutRequest,
            OutResult,
            OutRejectReason))
    {
        Character.DerivedStats.CurrentMana =
            OutClassResult.ManaBefore;
        Inventory->NotifyPartyInventoryChanged (Action.CharacterIndex);
        return false;
    }

    OutClassResult.ManaAfter = Character.DerivedStats.CurrentMana;
    Inventory->NotifyPartyInventoryChanged (Action.CharacterIndex);
    return true;
}

bool UGridTurnManagerComponent::BuildTargetingPreviewForAction (
    const FGridAvailableCombatAction& Action,
    const FIntPoint& TargetCell,
    FGridCombatActionTargetingPreview& OutPreview) const
{
    OutPreview = FGridCombatActionTargetingPreview ();
    OutPreview.Action = Action;
    OutPreview.TargetCell = TargetCell;

    if (!IsValid (RuntimeActor) || !IsValid (PartyPawn))
    {
        OutPreview.InvalidReason = MakeMON1286TargetingReason (
            TEXT ("Le niveau ou le groupe est indisponible."));
        return false;
    }
    const bool bCardinalFacing =
        PartyPawn->Facing == EGridEdge::North ||
        PartyPawn->Facing == EGridEdge::East ||
        PartyPawn->Facing == EGridEdge::South ||
        PartyPawn->Facing == EGridEdge::West;
    if (!bCardinalFacing)
    {
        OutPreview.InvalidReason = MakeMON1286TargetingReason (
            TEXT ("L'orientation du groupe est invalide."));
        return false;
    }
    if (!Action.IsValid () || !Action.bEnabled)
    {
        OutPreview.InvalidReason = Action.DisabledReason.IsEmpty ()
            ? MakeMON1286TargetingReason (TEXT ("Cette action est indisponible."))
            : Action.DisabledReason;
        return false;
    }
    if (!IsMON1286TargetedSource (Action.Definition.SourcePolicy) ||
        Action.Definition.ResolutionProfile !=
            EGridCombatActionResolutionProfile::Attack ||
        !IsMON1286ExplicitTargetingPolicy (
            Action.Definition.TargetingPolicy) ||
        !Action.Definition.OffensiveProfile.IsValid ())
    {
        OutPreview.InvalidReason = MakeMON1286TargetingReason (
            TEXT ("Cette action ne prend pas de cible cellule ou zone."));
        return false;
    }
    if (!RuntimeActor->IsValidCell (TargetCell.X, TargetCell.Y) ||
        RuntimeActor->GetCell (TargetCell.X, TargetCell.Y).CellType ==
            EGridCellType::Empty)
    {
        OutPreview.InvalidReason = MakeMON1286TargetingReason (
            TEXT ("Cette cellule n'appartient pas au niveau jouable."));
        return false;
    }

    const FIntPoint PartyCell (
        PartyPawn->CurrentCellX,
        PartyPawn->CurrentCellY);
    const int32 TargetDistance =
        FMath::Abs (TargetCell.X - PartyCell.X) +
        FMath::Abs (TargetCell.Y - PartyCell.Y);
    if (TargetDistance > Action.Definition.RangeCells)
    {
        OutPreview.InvalidReason = MakeMON1286TargetingReason (
            TEXT ("Cette cellule est hors de portée."));
        return false;
    }

    if (Action.Definition.TargetingPolicy ==
        EGridCombatTargetingPolicy::Cell)
    {
        OutPreview.AffectedCells.Add (TargetCell);
    }
    else
    {
        const int32 Radius = Action.Definition.AreaRadiusCells;
        for (int32 Y = TargetCell.Y - Radius;
            Y <= TargetCell.Y + Radius;
            ++Y)
        {
            for (int32 X = TargetCell.X - Radius;
                X <= TargetCell.X + Radius;
                ++X)
            {
                const int32 Distance =
                    FMath::Abs (X - TargetCell.X) +
                    FMath::Abs (Y - TargetCell.Y);
                if (Distance > Radius ||
                    !RuntimeActor->IsValidCell (X, Y) ||
                    RuntimeActor->GetCell (X, Y).CellType ==
                        EGridCellType::Empty)
                {
                    continue;
                }
                OutPreview.AffectedCells.Add (FIntPoint (X, Y));
            }
        }
    }

    UWorld* World = GetWorld ();
    UGridMonsterOccupancySubsystem* Occupancy = World
        ? World->GetSubsystem<UGridMonsterOccupancySubsystem> ()
        : nullptr;
    if (!IsValid (Occupancy))
    {
        OutPreview.InvalidReason = MakeMON1286TargetingReason (
            TEXT ("L'occupation du niveau est indisponible."));
        return false;
    }

    for (const FIntPoint& Cell : OutPreview.AffectedCells)
    {
        AGridMonsterActor* Monster = Occupancy->GetOccupantAtCell (Cell);
        if (!IsValid (Monster) ||
            !IsCombatMonster (Monster) ||
            !Monster->bMonsterEnabled ||
            !Monster->IsRuntimeLevelActive () ||
            Monster->IsDead ())
        {
            continue;
        }
        const FGuid MonsterId = Monster->ResolvePersistenceId ();
        if (MonsterId.IsValid ())
        {
            OutPreview.TargetMonsterIds.Add (MonsterId);
        }
    }

    if (OutPreview.TargetMonsterIds.IsEmpty ())
    {
        OutPreview.InvalidReason = MakeMON1286TargetingReason (
            Action.Definition.TargetingPolicy ==
                    EGridCombatTargetingPolicy::Area
                ? TEXT ("Cette zone ne contient aucun ennemi vivant.")
                : TEXT ("Cette cellule ne contient aucun ennemi vivant."));
        return false;
    }

    OutPreview.bValid = true;
    OutPreview.InvalidReason = FText::GetEmpty ();
    return true;
}

bool UGridTurnManagerComponent::BuildCombatActionTargetingPreview (
    int32 CharacterIndex,
    FName ActionId,
    EGridCombatActionSourcePolicy SourcePolicy,
    FName SourceDefinitionId,
    EGridEquipmentSlot SourceEquipmentSlot,
    FIntPoint TargetCell,
    FGridCombatActionTargetingPreview& OutPreview) const
{
    OutPreview = FGridCombatActionTargetingPreview ();
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
        OutPreview.TargetCell = TargetCell;
        OutPreview.InvalidReason = MakeMON1286TargetingReason (
            TEXT ("Cette action n'est plus disponible."));
        return false;
    }
    return BuildTargetingPreviewForAction (
        *Action,
        TargetCell,
        OutPreview);
}

bool UGridTurnManagerComponent::RequestCharacterTargetedAttack (
    const FGridAvailableCombatAction& Action,
    const FGridCombatActionTargetingPreview& Preview,
    FGridCombatActionRequestResult& OutResult)
{
    UGridPartyInventoryComponent* Inventory = IsValid (PartyPawn)
        ? PartyPawn->PartyInventoryComponent.Get ()
        : nullptr;
    if (!IsValid (Inventory) ||
        !Preview.bValid ||
        Preview.TargetMonsterIds.IsEmpty () ||
        !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex (
            Action.CharacterIndex))
    {
        return false;
    }

    FGridCharacterInventoryState& Character =
        Inventory->PartyInventoryState.ActiveCharacters[
            Action.CharacterIndex];
    FGridInventoryCharacterSummary CharacterSummary;
    FGridPlayerCharacterTurnState TurnStateBefore;
    if (!Inventory->GetCharacterSummary (
            Action.CharacterIndex,
            CharacterSummary) ||
        !GetPlayerCharacterTurnState (
            Action.CharacterIndex,
            TurnStateBefore) ||
        !CanCharacterSpendActionPoints (
            Action.CharacterIndex,
            Action.CurrentActionPointCost) ||
        Character.DerivedStats.CurrentMana < Action.CurrentManaCost)
    {
        return false;
    }

    TArray<AGridMonsterActor*> TargetMonsters;
    TArray<FGridAttackSourceStats> Sources;
    TArray<FGridAttackTargetStats> Targets;
    TArray<FGridAttackDefinition> AttackDefinitions;
    TargetMonsters.Reserve (Preview.TargetMonsterIds.Num ());
    Sources.Reserve (Preview.TargetMonsterIds.Num ());
    Targets.Reserve (Preview.TargetMonsterIds.Num ());
    AttackDefinitions.Reserve (Preview.TargetMonsterIds.Num ());
    for (const FGuid& TargetId : Preview.TargetMonsterIds)
    {
        AGridMonsterActor* TargetMonster = FindCombatMonsterById (TargetId);
        if (!IsValid (TargetMonster) ||
            !TargetMonster->bMonsterEnabled ||
            !TargetMonster->IsRuntimeLevelActive () ||
            TargetMonster->IsDead ())
        {
            return false;
        }

        FGridAttackSourceStats Source;
        FGridAttackTargetStats Target;
        FGridAttackDefinition AttackDefinition;
        if (!BuildPlayerAttackResolutionInputs (
                CharacterSummary,
                TargetMonster,
                Action.Definition.OffensiveProfile,
                Source,
                Target,
                AttackDefinition))
        {
            return false;
        }
        TargetMonsters.Add (TargetMonster);
        Sources.Add (Source);
        Targets.Add (Target);
        AttackDefinitions.Add (AttackDefinition);
    }

    const bool bQuickItem = Action.Definition.SourcePolicy ==
        EGridCombatActionSourcePolicy::QuickItem;
    const int32 QuantityBefore = bQuickItem
        ? Inventory->CountItemDefinitionInCharacterInventory (
            Action.CharacterIndex,
            Action.SourceDefinitionId)
        : 0;
    if (bQuickItem &&
        (Action.SourceDefinitionId.IsNone () ||
            Action.CurrentSourceItemQuantityCost <= 0 ||
            QuantityBefore < Action.CurrentSourceItemQuantityCost))
    {
        return false;
    }

    if (!SpendPlayerCharacterActionPoints (
            Action.CharacterIndex,
            Action.CurrentActionPointCost))
    {
        return false;
    }
    if (bQuickItem &&
        !Inventory->RemoveItemDefinitionFromCharacterInventory (
            Action.CharacterIndex,
            Action.SourceDefinitionId,
            Action.CurrentSourceItemQuantityCost))
    {
        if (FGridPlayerCharacterTurnState* RestoredTurnState =
            EnsurePlayerCharacterTurnState (Action.CharacterIndex))
        {
            *RestoredTurnState = TurnStateBefore;
            BroadcastPlayerCharacterTurnState (*RestoredTurnState);
        }
        return false;
    }

    const int32 ManaBefore = Character.DerivedStats.CurrentMana;
    Character.DerivedStats.CurrentMana = FMath::Max (
        0,
        ManaBefore - Action.CurrentManaCost);
    Inventory->NotifyPartyInventoryChanged (Action.CharacterIndex);

    OutResult.TargetedActionResult.TargetCell = Preview.TargetCell;
    OutResult.TargetedActionResult.AffectedCells = Preview.AffectedCells;
    OutResult.TargetedActionResult.TargetMonsterIds =
        Preview.TargetMonsterIds;
    if (bQuickItem)
    {
        OutResult.QuickItemResult.SourceQuantityBefore = QuantityBefore;
        OutResult.QuickItemResult.SourceQuantityAfter =
            Inventory->CountItemDefinitionInCharacterInventory (
                Action.CharacterIndex,
                Action.SourceDefinitionId);
        OutResult.QuickItemResult.HealthBefore =
            Character.DerivedStats.CurrentHealth;
        OutResult.QuickItemResult.HealthAfter =
            Character.DerivedStats.CurrentHealth;
        OutResult.QuickItemResult.ManaBefore = ManaBefore;
        OutResult.QuickItemResult.ManaAfter =
            Character.DerivedStats.CurrentMana;
    }
    else
    {
        OutResult.ClassActionResult.HealthBefore =
            Character.DerivedStats.CurrentHealth;
        OutResult.ClassActionResult.HealthAfter =
            Character.DerivedStats.CurrentHealth;
        OutResult.ClassActionResult.ManaBefore = ManaBefore;
        OutResult.ClassActionResult.ManaAfter =
            Character.DerivedStats.CurrentMana;
    }

    const FIntPoint PartyCell (
        PartyPawn->CurrentCellX,
        PartyPawn->CurrentCellY);
    bPlayerAttackResolutionInProgress = true;
    for (int32 Index = 0; Index < TargetMonsters.Num (); ++Index)
    {
        AGridMonsterActor* TargetMonster = TargetMonsters[Index];
        const FGuid TargetMonsterId = Preview.TargetMonsterIds[Index];
        const FIntPoint TargetMonsterCell = TargetMonster->CurrentCell;

        FGridPlayerAttackRequest Request;
        Request.RequestId = FGuid::NewGuid ();
        Request.RoundNumber = RoundNumber;
        Request.AttackerCharacterIndex = Action.CharacterIndex;
        Request.AttackerCharacterId = Character.CharacterId;
        Request.TargetMonsterId = TargetMonsterId;
        Request.PartyCell = PartyCell;
        Request.TargetCell = TargetMonsterCell;
        Request.PartyFacing = PartyPawn->Facing;
        Request.RangeCells = Action.Definition.RangeCells;
        Request.AttackId = Action.Definition.OffensiveProfile.AttackId;
        Request.OffensiveItemDefinitionId = bQuickItem
            ? Action.SourceDefinitionId
            : NAME_None;
        Request.OffensiveEquipmentSlot = EGridEquipmentSlot::None;
        Request.ActionPointCost = Action.CurrentActionPointCost;

        const FGridAttackResult AttackResult =
            FGridCombatResolver::ResolveAttack (
                Sources[Index],
                Targets[Index],
                AttackDefinitions[Index],
                CombatRandomStream);
        OutResult.TargetedActionResult.AttackRequests.Add (Request);
        OutResult.TargetedActionResult.AttackResults.Add (AttackResult);
        if (Index == 0)
        {
            OutResult.AttackRequest = Request;
            OutResult.AttackResult = AttackResult;
        }
        LastPlayerAttackRequest = Request;
        LastPlayerAttackResult = AttackResult;
        LastPlayerAttackRejectReason = EGridPlayerAttackRejectReason::None;

        ++PlayerAttackRequestedBroadcastCount;
        OnPlayerAttackRequested.Broadcast (Request);

        FGridCombatLogEntry AttackEntry;
        AttackEntry.RoundNumber = RoundNumber;
        AttackEntry.Phase = CurrentPhase;
        AttackEntry.Type = AttackResult.bHit
            ? EGridCombatLogEntryType::AttackHit
            : EGridCombatLogEntryType::AttackMiss;
        AttackEntry.SourceId = FName (*Character.CharacterId.ToString (
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
        AttackEntry.AttackResult = AttackResult;
        AttackEntry.bTargetDefeated =
            AttackResult.TargetHealthBefore > 0 &&
            AttackResult.TargetHealthAfter <= 0;
        AttackEntry.Message = FGridCombatLogFormatter::FormatPlayerAttack (
            AttackEntry.SourceDisplayName,
            AttackEntry.TargetDisplayName,
            AttackEntry.AttackId,
            AttackResult);
        AppendCombatLogEntry (AttackEntry);

        TargetMonster->ApplyAttackResult (AttackResult);
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
        OnPlayerAttackResolved.Broadcast (
            Request,
            TargetMonster,
            AttackResult);
        if (bCollectRuntimeMetrics)
        {
            ++RuntimeMetrics.AttacksResolved;
        }
    }
    bPlayerAttackResolutionInProgress = false;

    if (bPendingVictoryAfterPlayerAttack)
    {
        bPendingVictoryAfterPlayerAttack = false;
        FinishCombat (EGridCombatPhase::Victory);
    }
    else if (!InitiativeOrder.IsEmpty ())
    {
        FGridPlayerCharacterTurnState TurnStateAfter;
        if (GetPlayerCharacterTurnState (
                Action.CharacterIndex,
                TurnStateAfter) &&
            TurnStateAfter.RemainingActionPoints <= 0 &&
            IsActivePlayerCharacter (Action.CharacterIndex))
        {
            FinishActivePlayerTurn ();
        }
    }
    return true;
}

bool UGridTurnManagerComponent::RequestCharacterCombatActionAtCell (
    int32 CharacterIndex,
    FName ActionId,
    EGridCombatActionSourcePolicy SourcePolicy,
    FName SourceDefinitionId,
    EGridEquipmentSlot SourceEquipmentSlot,
    FIntPoint TargetCell,
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

    FGridCombatActionTargetingPreview Preview;
    if (!BuildCombatActionTargetingPreview (
            CharacterIndex,
            ActionId,
            SourcePolicy,
            SourceDefinitionId,
            SourceEquipmentSlot,
            TargetCell,
            Preview))
    {
        OutResult.Action = Preview.Action;
        OutResult.RejectReason = Preview.Action.Definition.ActionId.IsNone ()
            ? EGridCombatActionRequestRejectReason::InvalidAction
            : Preview.Action.bEnabled
                ? EGridCombatActionRequestRejectReason::InvalidTarget
                : EGridCombatActionRequestRejectReason::ActionUnavailable;
        return false;
    }

    OutResult.Action = Preview.Action;
    const bool bAccepted = RequestCharacterTargetedAttack (
        Preview.Action,
        Preview,
        OutResult);
    OutResult.bAccepted = bAccepted;
    OutResult.RejectReason = bAccepted
        ? EGridCombatActionRequestRejectReason::None
        : EGridCombatActionRequestRejectReason::InvalidTarget;
    UE_LOG (
        LogGridTurnManager,
        Log,
        TEXT ("[GridTargetedAction] Accepted=%s Character=%d Action=%s TargetCell=(%d,%d) AreaCells=%d Targets=%d APCost=%d ManaCost=%d"),
        bAccepted ? TEXT ("true") : TEXT ("false"),
        CharacterIndex,
        *ActionId.ToString (),
        TargetCell.X,
        TargetCell.Y,
        Preview.AffectedCells.Num (),
        Preview.TargetMonsterIds.Num (),
        Preview.Action.CurrentActionPointCost,
        Preview.Action.CurrentManaCost);
    return bAccepted;
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

    if (IsMON1286ExplicitTargetingPolicy (
        Action->Definition.TargetingPolicy))
    {
        OutResult.RejectReason =
            EGridCombatActionRequestRejectReason::TargetRequired;
        UE_LOG (
            LogGridTurnManager,
            Log,
            TEXT ("[GridActionCatalog] Accepted=false Character=%d Action=%s Source=%s Slot=%s Reason=TargetRequired"),
            CharacterIndex,
            *ActionId.ToString (),
            *SourceDefinitionId.ToString (),
            *UEnum::GetValueAsString (SourceEquipmentSlot));
        return false;
    }

    if (IsMON1285ClassActionSource (
        Action->Definition.SourcePolicy))
    {
        bool bAccepted = false;
        if (Action->Definition.ResolutionProfile ==
            EGridCombatActionResolutionProfile::Attack)
        {
            bAccepted = RequestCharacterClassActionAttack (
                *Action,
                OutResult.AttackRequest,
                OutResult.AttackResult,
                OutResult.AttackRejectReason,
                OutResult.ClassActionResult);
            OutResult.RejectReason = bAccepted
                ? EGridCombatActionRequestRejectReason::None
                : OutResult.AttackRejectReason !=
                        EGridPlayerAttackRejectReason::None
                    ? EGridCombatActionRequestRejectReason::AttackRejected
                    : EGridCombatActionRequestRejectReason::
                        ClassActionRejected;
        }
        else if (Action->Definition.ResolutionProfile ==
            EGridCombatActionResolutionProfile::Effect)
        {
            bAccepted = RequestCharacterClassActionEffect (
                *Action,
                OutResult.ClassActionResult);
            OutResult.RejectReason = bAccepted
                ? EGridCombatActionRequestRejectReason::None
                : EGridCombatActionRequestRejectReason::
                    ClassActionRejected;
        }
        else
        {
            OutResult.RejectReason =
                EGridCombatActionRequestRejectReason::
                    UnsupportedResolution;
        }

        OutResult.bAccepted = bAccepted;
        UE_LOG (
            LogGridTurnManager,
            Log,
            TEXT ("[GridClassAction] Accepted=%s Character=%d Action=%s Source=%s APCost=%d ManaCost=%d Health=%d->%d Mana=%d->%d"),
            bAccepted ? TEXT ("true") : TEXT ("false"),
            CharacterIndex,
            *ActionId.ToString (),
            *SourceDefinitionId.ToString (),
            Action->CurrentActionPointCost,
            Action->CurrentManaCost,
            OutResult.ClassActionResult.HealthBefore,
            OutResult.ClassActionResult.HealthAfter,
            OutResult.ClassActionResult.ManaBefore,
            OutResult.ClassActionResult.ManaAfter);
        return bAccepted;
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
