#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Magic/GridPartySpellbookComponent.h"
#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellbookUI.h"
#include "Magic/GridSpellHotbarExecution.h"
#include "Magic/GridSpellPresentationComponent.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGSkillRequirementProjectionService.h"
#include "RPG/StatusEffects/GridStatusEffectControlResolver.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "UObject/UObjectIterator.h"

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

    bool TryBuildUI0143e2ProductionSpell (
        FName SpellId,
        FGridSpellDefinition& OutDefinition)
    {
        OutDefinition = FGridSpellDefinition ();
        if (SpellId.IsNone ())
        {
            return false;
        }

        TArray<FGridSpellDefinition> Definitions;
        FGridProductionSpellLibrary::BuildAll (Definitions);
        const FGridSpellDefinition* Found =
            Definitions.FindByPredicate (
                [SpellId] (const FGridSpellDefinition& Candidate)
                {
                    return Candidate.SpellId == SpellId;
                });
        if (!Found ||
            FGridSpellContract::ValidateDefinition (*Found) !=
                EGridSpellValidationError::None)
        {
            return false;
        }
        OutDefinition = *Found;
        return true;
    }

    const UGridStatusEffectDefinitionAsset*
    ResolveUI0143e2StatusDefinition (FName EffectId)
    {
        if (EffectId.IsNone ())
        {
            return nullptr;
        }

        for (TObjectIterator<UGridStatusEffectDefinitionAsset> It; It; ++It)
        {
            UGridStatusEffectDefinitionAsset* Definition = *It;
            if (IsValid (Definition) &&
                Definition->GetPrimaryAssetId ().PrimaryAssetName == EffectId &&
                Definition->IsValidDefinition ())
            {
                return Definition;
            }
        }
        return nullptr;
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

    // UI01.4.3e: the runtime Spellbook is the authoritative source of known
    // spell actions. The hotbar stores identity only; the combat catalogue
    // must reconstruct the action from SpellId every time it is queried.
    if (IsValid (PartyPawn))
    {
        const UGridPartySpellbookComponent* SpellbookComponent =
            PartyPawn->FindComponentByClass<UGridPartySpellbookComponent> ();
        const FGridCharacterSpellbookState* CharacterSpellbook =
            IsValid (SpellbookComponent)
                ? SpellbookComponent->SpellbookState.FindSpellbook (
                    Character.CharacterId)
                : nullptr;
        if (CharacterSpellbook)
        {
            TArray<FGridSpellDefinition> ProductionSpells;
            FGridProductionSpellLibrary::BuildAll (ProductionSpells);
            for (const FName SpellId : CharacterSpellbook->KnownSpellIds)
            {
                const FGridSpellDefinition* SpellDefinition =
                    ProductionSpells.FindByPredicate (
                        [SpellId] (const FGridSpellDefinition& Candidate)
                        {
                            return Candidate.SpellId == SpellId;
                        });
                if (!SpellDefinition ||
                    FGridSpellContract::ValidateDefinition (
                        *SpellDefinition) != EGridSpellValidationError::None)
                {
                    continue;
                }

                const FGridCombatActionDefinition SpellAction =
                    UGridSpellbookUILibrary::
                        MakeSpellCombatActionDefinition (*SpellDefinition);
                const bool bAlreadyContributed =
                    OutContributions.ContainsByPredicate (
                        [SpellId] (
                            const FGridCombatActionContribution& Existing)
                        {
                            return Existing.Definition.SourcePolicy ==
                                    EGridCombatActionSourcePolicy::Spell &&
                                Existing.Definition.ActionId == SpellId &&
                                Existing.SourceDefinitionId == SpellId;
                        });
                if (!bAlreadyContributed)
                {
                    AddMON126Contribution (
                        SpellAction,
                        SpellId,
                        FGuid (),
                        EGridEquipmentSlot::None,
                        1,
                        OutContributions);
                }
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
                if (ItemDefinition->bThrowable &&
                    Definition.SourcePolicy ==
                        EGridCombatActionSourcePolicy::Equipment &&
                    Definition.ResolutionProfile ==
                        EGridCombatActionResolutionProfile::Attack)
                {
                    Definition.ResourceCosts.SourceItemQuantityCost = 1;
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
    for (const TPair<FGridCombatActionCooldownKey, int32>& Cooldown :
        CombatActionCooldownAvailableRounds)
    {
        if (Cooldown.Key.CharacterId != Character.CharacterId)
        {
            continue;
        }

        const int32 RemainingRounds =
            GetRemainingCombatActionCooldown (
                Character.CharacterId,
                Cooldown.Key.ActionId);
        if (RemainingRounds > 0)
        {
            Context.RemainingCooldownRounds.Add (
                Cooldown.Key.ActionId,
                RemainingRounds);
        }
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

    FString SkillRequirementError;
    if (!FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements (
            Character,
            Context.SatisfiedRequirements,
            SkillRequirementError))
    {
        UE_LOG (
            LogGridTurnManager,
            Warning,
            TEXT ("[GridActionCatalog] SkillRequirementProjectionFailed Character=%d CharacterId=%s Error=%s"),
            CharacterIndex,
            *Character.CharacterId.ToString (EGuidFormats::Digits),
            *SkillRequirementError);
    }

    TArray<FGridCombatActionContribution> Contributions;
    BuildPlayerCombatActionContributions (
        CharacterIndex,
        Contributions);
    FGridCombatActionCatalog::Build (
        Context,
        Contributions,
        OutActions);

    const bool bSpellActionsBlocked =
        FGridStatusEffectControlResolver::Resolve (
            Character.StatusEffects).bBlockSpellActions;
    for (FGridAvailableCombatAction& Action : OutActions)
    {
        if (bSpellActionsBlocked &&
            Action.bEnabled &&
            Action.Definition.SourcePolicy ==
                EGridCombatActionSourcePolicy::Spell)
        {
            Action.bEnabled = false;
            Action.AvailabilityReason =
                EGridCombatActionAvailabilityReason::MissingRequirement;
            Action.DisabledReason = FText::FromString (
                TEXT ("Un effet de statut empêche l'utilisation des sorts."));
        }
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

int32 UGridTurnManagerComponent::GetRemainingCombatActionCooldown (
    const FGuid& CharacterId,
    FName ActionId) const
{
    if (!CharacterId.IsValid () || ActionId.IsNone ())
    {
        return 0;
    }

    FGridCombatActionCooldownKey Key;
    Key.CharacterId = CharacterId;
    Key.ActionId = ActionId;
    const int32* AvailableRound =
        CombatActionCooldownAvailableRounds.Find (Key);
    return AvailableRound
        ? FMath::Max (0, *AvailableRound - RoundNumber)
        : 0;
}

void UGridTurnManagerComponent::StartCombatActionCooldown (
    const FGridAvailableCombatAction& Action)
{
    const int32 CooldownRounds =
        FMath::Max (0, Action.Definition.CooldownRounds);
    if (CooldownRounds <= 0 ||
        !Action.CharacterId.IsValid () ||
        Action.Definition.ActionId.IsNone ())
    {
        return;
    }

    FGridCombatActionCooldownKey Key;
    Key.CharacterId = Action.CharacterId;
    Key.ActionId = Action.Definition.ActionId;
    CombatActionCooldownAvailableRounds.Add (
        Key,
        RoundNumber + CooldownRounds + 1);
}

void UGridTurnManagerComponent::ResetCombatActionCooldowns ()
{
    CombatActionCooldownAvailableRounds.Reset ();
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
        OutResult.HealthAfter = OutResult.HealthBefore;
        OutResult.ManaAfter = OutResult.ManaBefore;
        OutResult.SourceQuantityAfter =
            OutResult.SourceQuantityBefore;
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
    StartCombatActionCooldown (Action);
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
    StartCombatActionCooldown (Action);
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

    const FGridCharacterInventoryState& CharacterBefore =
        Inventory->PartyInventoryState.ActiveCharacters[
            Action.CharacterIndex];
    OutClassResult.HealthBefore =
        CharacterBefore.DerivedStats.CurrentHealth;
    OutClassResult.HealthAfter = OutClassResult.HealthBefore;
    OutClassResult.ManaBefore =
        CharacterBefore.DerivedStats.CurrentMana;
    OutClassResult.ManaAfter = OutClassResult.ManaBefore;
    if (OutClassResult.ManaBefore < Action.CurrentManaCost)
    {
        return false;
    }

    if (!RequestCharacterAttackInternal (
            Action.CharacterIndex,
            EGridEquipmentSlot::None,
            false,
            &Action,
            OutRequest,
            OutResult,
            OutRejectReason))
    {
        return false;
    }

    if (IsValid (Inventory) &&
        Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex (
            Action.CharacterIndex))
    {
        OutClassResult.ManaAfter =
            Inventory->PartyInventoryState.ActiveCharacters[
                Action.CharacterIndex].DerivedStats.CurrentMana;
    }
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
    StartCombatActionCooldown (Action);
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

    UGridPartyInventoryComponent* SpellInventory = IsValid (PartyPawn)
        ? PartyPawn->PartyInventoryComponent.Get ()
        : nullptr;
    UGridPartySpellbookComponent* SpellbookComponent = IsValid (PartyPawn)
        ? PartyPawn->FindComponentByClass<UGridPartySpellbookComponent> ()
        : nullptr;
    const FGridCharacterInventoryState* SpellCharacter =
        IsValid (SpellInventory) &&
        SpellInventory->PartyInventoryState.ActiveCharacters.IsValidIndex (
            CharacterIndex)
            ? &SpellInventory->PartyInventoryState.ActiveCharacters[
                CharacterIndex]
            : nullptr;
    const FGridCharacterSpellbookState* CharacterSpellbook =
        SpellCharacter && IsValid (SpellbookComponent)
            ? SpellbookComponent->SpellbookState.FindSpellbook (
                SpellCharacter->CharacterId)
            : nullptr;
    const bool bSpellbookBackedAction =
        Action->Definition.SourcePolicy ==
            EGridCombatActionSourcePolicy::Spell &&
        Action->SourceDefinitionId == Action->Definition.ActionId &&
        CharacterSpellbook &&
        CharacterSpellbook->KnowsSpell (Action->Definition.ActionId);

    if (bSpellbookBackedAction)
    {
        FGridSpellDefinition SpellDefinition;
        FGridPlayerCharacterTurnState TurnStateBefore;
        if (!SpellCharacter ||
            !TryBuildUI0143e2ProductionSpell (
                Action->Definition.ActionId,
                SpellDefinition) ||
            !GetPlayerCharacterTurnState (
                CharacterIndex,
                TurnStateBefore))
        {
            OutResult.RejectReason =
                EGridCombatActionRequestRejectReason::ClassActionRejected;
            return false;
        }

        FGridSpellCastRequest CastRequest;
        CastRequest.CasterCharacterId = SpellCharacter->CharacterId;
        CastRequest.SpellId = SpellDefinition.SpellId;

        FGridSpellTargetingContext TargetingContext;
        TargetingContext.CasterCell = FIntPoint (
            PartyPawn->CurrentCellX,
            PartyPawn->CurrentCellY);
        TargetingContext.bLineOfSightClear = true;

        AGridMonsterActor* TargetMonster = nullptr;
        if (SpellDefinition.TargetingPolicy ==
                EGridCombatTargetingPolicy::Self ||
            SpellDefinition.TargetingPolicy ==
                EGridCombatTargetingPolicy::Ally)
        {
            // UI01.4.3e.2: a direct Ally hotbar cast deterministically targets
            // its caster until a dedicated party-portrait target selector is
            // introduced. No target heuristics or hidden selection are used.
            CastRequest.Target.TargetId = SpellCharacter->CharacterId;
            CastRequest.Target.GridCell = TargetingContext.CasterCell;
            CastRequest.Target.bHasGridCell = true;
            TargetingContext.ResolvedTargetId = SpellCharacter->CharacterId;
            TargetingContext.ResolvedTargetCell = TargetingContext.CasterCell;
            TargetingContext.bHasResolvedTargetCell = true;
            TargetingContext.bResolvedTargetIsAlly = true;
        }
        else if (SpellDefinition.TargetingPolicy ==
            EGridCombatTargetingPolicy::FirstAxialTarget)
        {
            if (!Action->SuggestedTargetId.IsValid ())
            {
                OutResult.RejectReason =
                    EGridCombatActionRequestRejectReason::InvalidTarget;
                UE_LOG (
                    LogGridTurnManager,
                    Log,
                    TEXT ("[GridSpellAction] Accepted=false Character=%d Spell=%s Reason=NoSuggestedHostileTarget"),
                    CharacterIndex,
                    *SpellDefinition.SpellId.ToString ());
                return false;
            }
            TargetMonster = FindCombatMonsterById (
                Action->SuggestedTargetId);
            if (!IsValid (TargetMonster) ||
                !TargetMonster->bMonsterEnabled ||
                !TargetMonster->IsRuntimeLevelActive () ||
                TargetMonster->IsDead ())
            {
                OutResult.RejectReason =
                    EGridCombatActionRequestRejectReason::InvalidTarget;
                return false;
            }

            CastRequest.Target.TargetId = Action->SuggestedTargetId;
            CastRequest.Target.GridCell = Action->SuggestedTargetCell;
            CastRequest.Target.bHasGridCell = true;
            TargetingContext.ResolvedTargetId = Action->SuggestedTargetId;
            TargetingContext.ResolvedTargetCell = Action->SuggestedTargetCell;
            TargetingContext.bHasResolvedTargetCell = true;
            TargetingContext.bResolvedTargetIsHostile = true;
        }
        else
        {
            OutResult.RejectReason =
                EGridCombatActionRequestRejectReason::UnsupportedResolution;
            return false;
        }

        const int32 TargetMaxHealth = TargetMonster
            ? IsValid (TargetMonster->MonsterDefinition)
                ? FMath::Max (
                    1,
                    TargetMonster->MonsterDefinition->MaxHealth)
                : FMath::Max (1, TargetMonster->CurrentHealth)
            : FMath::Max (1, SpellCharacter->DerivedStats.MaxHealth);
        const int32 TargetCurrentHealth = TargetMonster
            ? TargetMonster->CurrentHealth
            : SpellCharacter->DerivedStats.CurrentHealth;
        const FGridStatusEffectCollection TargetStatusEffects = TargetMonster
            ? TargetMonster->StatusEffects
            : SpellCharacter->StatusEffects;

        FGridSpellHotbarExecutionResult Execution;
        const bool bExecuted =
            FGridSpellHotbarExecutionService::TryExecute (
                SpellDefinition,
                CastRequest,
                TargetingContext,
                *CharacterSpellbook,
                SpellCharacter->DerivedStats,
                TurnStateBefore,
                TargetMaxHealth,
                TargetCurrentHealth,
                TargetStatusEffects,
                [] (FName EffectId)
                    -> const UGridStatusEffectDefinitionAsset*
                {
                    return ResolveUI0143e2StatusDefinition (EffectId);
                },
                Execution);
        if (!bExecuted)
        {
            OutResult.RejectReason =
                Execution.PipelineRejectStage ==
                        EGridSpellCastPipelineRejectStage::Targeting
                    ? EGridCombatActionRequestRejectReason::InvalidTarget
                    : EGridCombatActionRequestRejectReason::ClassActionRejected;
            UE_LOG (
                LogGridTurnManager,
                Log,
                TEXT ("[GridSpellAction] Accepted=false Character=%d Spell=%s PipelineStage=%s Targeting=%s Transaction=%s Effect=%s Error=%s"),
                CharacterIndex,
                *SpellDefinition.SpellId.ToString (),
                *UEnum::GetValueAsString (Execution.PipelineRejectStage),
                *UEnum::GetValueAsString (Execution.TargetingRejectReason),
                *UEnum::GetValueAsString (Execution.TransactionRejectReason),
                *UEnum::GetValueAsString (Execution.EffectRejectReason),
                *Execution.Error);
            return false;
        }

        FGridPlayerCharacterTurnState* AuthoritativeTurnState =
            FindPlayerCharacterTurnState (SpellCharacter->CharacterId);
        if (!AuthoritativeTurnState ||
            !SpellInventory->PartyInventoryState.ActiveCharacters
                .IsValidIndex (CharacterIndex))
        {
            OutResult.RejectReason =
                EGridCombatActionRequestRejectReason::ClassActionRejected;
            return false;
        }

        FGridCharacterInventoryState& MutableCharacter =
            SpellInventory->PartyInventoryState.ActiveCharacters[
                CharacterIndex];
        OutResult.ClassActionResult.HealthBefore =
            MutableCharacter.DerivedStats.CurrentHealth;
        OutResult.ClassActionResult.ManaBefore =
            MutableCharacter.DerivedStats.CurrentMana;

        MutableCharacter.DerivedStats = Execution.CasterStats;
        *AuthoritativeTurnState = Execution.CasterTurnState;
        if (!TargetMonster)
        {
            MutableCharacter.DerivedStats.CurrentHealth =
                Execution.TargetCurrentHealth;
            MutableCharacter.StatusEffects =
                Execution.TargetStatusEffects;
        }
        BroadcastPlayerCharacterTurnState (*AuthoritativeTurnState);

        if (TargetMonster)
        {
            TargetMonster->StatusEffects = Execution.TargetStatusEffects;
            const bool bPreviousResolutionInProgress =
                bPlayerAttackResolutionInProgress;
            bPlayerAttackResolutionInProgress = true;
            TargetMonster->SetCurrentHealth (
                Execution.TargetCurrentHealth);
            bPlayerAttackResolutionInProgress =
                bPreviousResolutionInProgress;

            OutResult.TargetedActionResult.TargetCell =
                Execution.ResolvedTarget.GridCell;
            OutResult.TargetedActionResult.AffectedCells.Add (
                Execution.ResolvedTarget.GridCell);
            OutResult.TargetedActionResult.TargetMonsterIds.Add (
                TargetMonster->ResolvePersistenceId ());
            if (!TargetMonster->IsDead ())
            {
                if (FGridCombatantInitiativeEntry* TargetEntry =
                    FindInitiativeEntry (
                        EGridCombatantSide::Monster,
                        TargetMonster->ResolvePersistenceId ()))
                {
                    RefreshInitiativeEntryVitals (*TargetEntry);
                    OnCombatantStateChanged.Broadcast (*TargetEntry);
                }
            }
        }

        StartCombatActionCooldown (*Action);
        SpellInventory->NotifyPartyInventoryChanged (CharacterIndex);
        if (FGridCombatantInitiativeEntry* Entry = FindInitiativeEntry (
                EGridCombatantSide::Party,
                MutableCharacter.CharacterId))
        {
            RefreshInitiativeEntryVitals (*Entry);
            OnCombatantStateChanged.Broadcast (*Entry);
        }
        if (UWorld* World = GetWorld ())
        {
            if (UGridStatusEffectLifecycleSubsystem* StatusLifecycle =
                World->GetSubsystem<UGridStatusEffectLifecycleSubsystem> ())
            {
                StatusLifecycle->RefreshAllInitiativeModifiers ();
            }
        }

        const FVector SourceWorldLocation = PartyPawn->GetActorLocation ();
        const FVector TargetWorldLocation = TargetMonster
            ? TargetMonster->GetActorLocation ()
            : SourceWorldLocation;
        FGridSpellPresentationProfile PresentationProfile;
        if (FGridProductionSpellLibrary::TryBuildPresentationProfile (
                SpellDefinition.SpellId,
                PresentationProfile))
        {
            FGridSpellPresentationPlan PresentationPlan;
            if (FGridSpellPresentationService::BuildPlan (
                    SpellDefinition,
                    Execution.ResolvedTarget,
                    PresentationProfile,
                    SourceWorldLocation,
                    TargetWorldLocation,
                    PresentationPlan))
            {
                UGridSpellPresentationComponent* PresentationComponent =
                    PartyPawn->FindComponentByClass<
                        UGridSpellPresentationComponent> ();
                if (!IsValid (PresentationComponent))
                {
                    PresentationComponent =
                        NewObject<UGridSpellPresentationComponent> (
                            PartyPawn,
                            TEXT ("GridSpellPresentationRuntime"));
                    if (IsValid (PresentationComponent))
                    {
                        PartyPawn->AddInstanceComponent (
                            PresentationComponent);
                        PresentationComponent->RegisterComponent ();
                    }
                }
                if (IsValid (PresentationComponent))
                {
                    PresentationComponent->PresentSpell (
                        PresentationPlan,
                        PresentationProfile);
                }
            }
        }

        OutResult.ClassActionResult.HealthAfter =
            MutableCharacter.DerivedStats.CurrentHealth;
        OutResult.ClassActionResult.ManaAfter =
            MutableCharacter.DerivedStats.CurrentMana;
        OutResult.bAccepted = true;
        OutResult.RejectReason =
            EGridCombatActionRequestRejectReason::None;

        UE_LOG (
            LogGridTurnManager,
            Log,
            TEXT ("[GridSpellAction] Accepted=true Character=%d Spell=%s Target=%s AP=%d Mana=%d Health=%d->%d ManaState=%d->%d Damage=%d Healing=%d"),
            CharacterIndex,
            *SpellDefinition.SpellId.ToString (),
            *Execution.ResolvedTarget.TargetId.ToString (
                EGuidFormats::Digits),
            Execution.CostReceipt.ActionPointsSpent,
            Execution.CostReceipt.ManaSpent,
            OutResult.ClassActionResult.HealthBefore,
            OutResult.ClassActionResult.HealthAfter,
            OutResult.ClassActionResult.ManaBefore,
            OutResult.ClassActionResult.ManaAfter,
            Execution.EffectResult.TotalDamage,
            Execution.EffectResult.TotalHealing);

        if (bPendingVictoryAfterPlayerAttack)
        {
            bPendingVictoryAfterPlayerAttack = false;
            FinishCombat (EGridCombatPhase::Victory);
        }
        else if (!InitiativeOrder.IsEmpty () &&
            AuthoritativeTurnState->RemainingActionPoints <= 0 &&
            IsActivePlayerCharacter (CharacterIndex))
        {
            FinishActivePlayerTurn ();
        }
        return true;
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
            UGridPartyInventoryComponent* Inventory = IsValid (PartyPawn)
                ? PartyPawn->PartyInventoryComponent.Get ()
                : nullptr;
            if (!IsValid (Inventory) ||
                !Inventory->PartyInventoryState.ActiveCharacters
                    .IsValidIndex (CharacterIndex))
            {
                OutResult.RejectReason =
                    EGridCombatActionRequestRejectReason::QuickItemRejected;
                return false;
            }
            OutResult.QuickItemResult.SourceQuantityBefore =
                Inventory->CountItemDefinitionInCharacterInventory (
                    CharacterIndex,
                    Action->SourceDefinitionId);
            const FGridCharacterInventoryState& CharacterBefore =
                Inventory->PartyInventoryState.ActiveCharacters[
                    CharacterIndex];
            OutResult.QuickItemResult.HealthBefore =
                CharacterBefore.DerivedStats.CurrentHealth;
            OutResult.QuickItemResult.HealthAfter =
                CharacterBefore.DerivedStats.CurrentHealth;
            OutResult.QuickItemResult.ManaBefore =
                CharacterBefore.DerivedStats.CurrentMana;
            OutResult.QuickItemResult.ManaAfter =
                OutResult.QuickItemResult.ManaBefore;

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
            if (bAccepted)
            {
                const bool bCharacterStillAvailable =
                    IsValid (Inventory) &&
                    Inventory->PartyInventoryState.ActiveCharacters
                        .IsValidIndex (CharacterIndex);
                if (bCharacterStillAvailable)
                {
                    const FGridCharacterInventoryState& CharacterAfter =
                        Inventory->PartyInventoryState.ActiveCharacters[
                            CharacterIndex];
                    OutResult.QuickItemResult.SourceQuantityAfter =
                        Inventory->
                            CountItemDefinitionInCharacterInventory (
                                CharacterIndex,
                                Action->SourceDefinitionId);
                    OutResult.QuickItemResult.HealthAfter =
                        CharacterAfter.DerivedStats.CurrentHealth;
                    OutResult.QuickItemResult.ManaAfter =
                        CharacterAfter.DerivedStats.CurrentMana;
                }
            }
            else
            {
                OutResult.QuickItemResult.SourceQuantityAfter =
                    OutResult.QuickItemResult.SourceQuantityBefore;
                OutResult.QuickItemResult.ManaAfter =
                    OutResult.QuickItemResult.ManaBefore;
                UE_LOG (
                    LogGridTurnManager,
                    Verbose,
                    TEXT ("[GridQuickItem] AttackRejectedWithoutCommit Character=%d Action=%s Source=%s"),
                    CharacterIndex,
                    *ActionId.ToString (),
                    *SourceDefinitionId.ToString ());
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
