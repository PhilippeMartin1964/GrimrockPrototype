#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "../GridExplorationItemReadModel.h"
#include "Runtime/Combat/GridCatalystAreaResolver.h"
#include "Runtime/Combat/GridCombatEventFeedFormatter.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatSurfaceResolver.h"
#include "Runtime/GridEventFeedPublisher.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/UILocalizationText.h"

namespace
{
	FGridAvailableCombatAction MakeExplorationSurfaceAction(const FGridAvailableCombatAction& Action,
		const FGridCombatActionDefinition& FreshDefinition, int32 QuantityBefore)
	{
		FGridAvailableCombatAction FreshAction = Action;
		FreshAction.Definition = FreshDefinition;
		FreshAction.CurrentActionPointCost = 0;
		FreshAction.CurrentManaCost = 0;
		FreshAction.CurrentSourceItemQuantityCost = 1;
		FreshAction.CurrentSourceItemQuantity = QuantityBefore;
		FreshAction.bEnabled = true;
		FreshAction.AvailabilityReason = EGridCombatActionAvailabilityReason::None;
		return FreshAction;
	}

	bool PrepareExplorationSurfaceTargeting(bool bCombatActive, const AGridLevelRuntimeActor* RuntimeActor,
		const AGrimrockPartyPawn* PartyPawn, bool bApprovedAction, const FGridAvailableCombatAction& Action,
		FIntPoint TargetCell, FGridCombatActionTargetingPreview& OutPreview)
	{
		OutPreview = FGridCombatActionTargetingPreview();
		OutPreview.TargetCell = TargetCell;
		OutPreview.Action = Action;
		const UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
		if (bCombatActive || !IsValid(RuntimeActor) || !bApprovedAction || !Action.bEnabled ||
			!Action.IsValid() || !IsValid(Inventory) ||
			!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Action.CharacterIndex) ||
			Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex].CharacterId != Action.CharacterId)
		{
			OutPreview.InvalidReason = FUILocalizationText::Get(TEXT("Combat.Targeting.Invalid.ActionUnavailable"));
			return false;
		}
		return true;
	}
}

bool UGridTurnManagerComponent::IsExplorationCatalystAction(const FGridCombatActionDefinition& Definition)
{
	return FGridCatalystAreaResolver::IsApprovedAction(Definition) &&
		Definition.bUsableDuringExploration &&
		Definition.ResourceCosts.SourceItemQuantityCost == 1 &&
		Definition.ResourceCosts.ManaCost == 0 &&
		Definition.SurfaceEffects.IsEmpty() && Definition.SurfaceConversions.IsEmpty() &&
		Definition.StatusApplications.IsEmpty() && Definition.StatusRemovals.IsEmpty() &&
		Definition.ArmorEffects.IsEmpty() && Definition.MovementEffects.IsEmpty() &&
		!Definition.TrapEffect.bPlaceTrap && !Definition.bRelocatePartyToTargetCell;
}

bool UGridTurnManagerComponent::BuildExplorationCatalystTargetingPreview(
	const FGridAvailableCombatAction& Action, FIntPoint TargetCell,
	FGridCombatActionTargetingPreview& OutPreview) const
{
	return PrepareExplorationSurfaceTargeting(bCombatActive, RuntimeActor, PartyPawn,
		IsExplorationCatalystAction(Action.Definition), Action, TargetCell, OutPreview) &&
		BuildTargetingPreviewForAction(Action, TargetCell, OutPreview);
}

bool UGridTurnManagerComponent::RequestExplorationCatalystActionAtCell(
	const FGridAvailableCombatAction& Action, FIntPoint TargetCell,
	FGridCombatActionRequestResult& OutResult)
{
	OutResult = FGridCombatActionRequestResult();
	OutResult.Action = Action;
	UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	if (bCombatActive || !IsValid(RuntimeActor) || !IsValid(Inventory) ||
		!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Action.CharacterIndex) ||
		!FGridCatalystAreaResolver::IsApprovedItem(Action.SourceDefinitionId, Action.Definition.ActionId) ||
		!IsExplorationCatalystAction(Action.Definition))
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::ActionUnavailable;
		return false;
	}

	// Re-read the EXACT owned item; never trust the HUD's cached definition.
	auto Source = FGridExplorationItemReadModel::ResolveSurface(
		Inventory, Action.CharacterIndex, Action.SourceDefinitionId, Action.Definition.ActionId, true);
	const FGridCombatActionDefinition& FreshDefinition = Source.Action;
	const FGridCharacterInventoryState& Character =
		Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex];
	if (!Source.bResolved ||
		!FGridCatalystAreaResolver::IsApprovedItem(Action.SourceDefinitionId, FreshDefinition.ActionId) ||
		!IsExplorationCatalystAction(FreshDefinition) ||
		FreshDefinition.ActionId != Action.Definition.ActionId ||
		Character.CharacterId != Action.CharacterId || Character.Resources.CurrentHealth <= 0)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::QuickItemRejected;
		return false;
	}
	const int32 QuantityBefore =
		Inventory->CountItemDefinitionInCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId);
	if (QuantityBefore < 1)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::QuickItemRejected;
		return false;
	}

	const FGridAvailableCombatAction FreshAction = MakeExplorationSurfaceAction(Action, FreshDefinition, QuantityBefore);
	OutResult.Action = FreshAction;
	FGridCombatActionTargetingPreview Checked;
	if (!BuildExplorationCatalystTargetingPreview(FreshAction, TargetCell, Checked) ||
		Checked.AffectedCells.IsEmpty() || !RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel())
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidTarget;
		return false;
	}

	TArray<FGridCombatModifierProfile> Profiles;
	FGridResolvedCombatModifiers Modifiers;
	if (FGridCombatModifierResolver::CollectCharacterModifiers(Character, Profiles))
	{
		FGridCombatModifierResolver::Resolve(Profiles,
			FGridCombatModifierResolver::MakeActionContext(FreshDefinition, Action.SourceDefinitionId), Modifiers);
	}
	TArray<FGridCatalystCellSnapshot> Snapshot;
	Snapshot.Reserve(Checked.AffectedCells.Num());
	for (const FIntPoint& Cell : Checked.AffectedCells)
	{
		FGridCatalystCellSnapshot& Entry = Snapshot.AddDefaulted_GetRef();
		Entry.Cell = Cell;
		if (const FGridCombatSurfaceState* Existing = RuntimeActor->FindCombatSurfaceAtCell(Cell.X, Cell.Y))
		{
			Entry.Surface = *Existing;
			Entry.bHasSurface = Existing->IsValid();
		}
	}
	TArray<FGridCatalystCellOutcome> Plan;
	if (!FGridCatalystAreaResolver::ResolveSnapshot(
			Snapshot, FreshDefinition.SurfaceInteraction, Modifiers, Plan))
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::UnsupportedResolution;
		return false;
	}

	// Defensive inventory snapshot: failed commit restores the exact item,
	// stack and hotbar ownership state, without inventing a new item GUID.
	const FGridPartyInventoryState InventoryBefore = Inventory->PartyInventoryState;
	if (!Inventory->RemoveItemDefinitionFromCharacterInventory(
			Action.CharacterIndex, Action.SourceDefinitionId, 1))
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::QuickItemRejected;
		return false;
	}
	RuntimeActor->BeginCatalystSurfaceVisualBatch();
	const bool bCommitted = RuntimeActor->CommitCatalystAreaPlan(
		Snapshot, Plan, Character.CharacterId, FreshDefinition.ActionId);
	ensureMsgf(bCommitted, TEXT("Catalyst: a prevalidated game-thread area plan must commit."));
	if (!bCommitted)
	{
		RuntimeActor->CancelCatalystSurfaceVisualBatch();
		Inventory->PartyInventoryState = InventoryBefore;
		Inventory->NotifyPartyInventoryChanged(Action.CharacterIndex);
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidTarget;
		return false;
	}

	ApplyCatalystBurstDamage(Plan);
	// One flight, one impact and independent per-cell visual wave.
	RuntimeActor->PlaySurfaceCatalystArea(
		FIntPoint(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY), TargetCell, Plan,
		Source.Mesh.LoadSynchronous());
	GridEventFeedPublisher::PublishEvent(this,
		FGridCombatEventFeedFormatter::MakeCatalystLaunch(
			ResolveCharacterDisplayName(Action.CharacterIndex), Plan.Num()));

	OutResult.bAccepted = true;
	OutResult.RejectReason = EGridCombatActionRequestRejectReason::None;
	OutResult.TargetedActionResult.TargetCell = TargetCell;
	OutResult.TargetedActionResult.AffectedCells = Checked.AffectedCells;
	OutResult.QuickItemResult.SourceQuantityBefore = QuantityBefore;
	OutResult.QuickItemResult.SourceQuantityAfter =
		Inventory->CountItemDefinitionInCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId);
	UE_LOG(LogGridTurnManager, Log,
		TEXT("[ARMEMENT01.17.3B] ExplorationCatalyst Character=%d Cell=(%d,%d) Cells=%d Items=%d->%d"),
		Action.CharacterIndex, TargetCell.X, TargetCell.Y, Checked.AffectedCells.Num(),
		QuantityBefore, OutResult.QuickItemResult.SourceQuantityAfter);
	return true;
}

bool UGridTurnManagerComponent::IsExplorationOilSlickAction(const FGridCombatActionDefinition& Definition)
{
	return Definition.ActionId == FName(TEXT("Action_Alchemist_OilSlick")) &&
		Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
		Definition.TargetingPolicy == EGridCombatTargetingPolicy::Area &&
		Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		Definition.SurfaceEffects.Num() == 1 &&
		Definition.SurfaceEffects[0].SurfaceType == EGridCombatSurfaceType::Oil &&
		Definition.SurfaceEffects[0].IsValid() &&
		Definition.ResourceCosts.SourceItemQuantityCost == 1 &&
		Definition.ResourceCosts.ManaCost == 0 &&
		Definition.SurfaceConversions.IsEmpty() &&
		Definition.SurfaceInteraction == EGridCombatSurfaceInteraction::None &&
		Definition.StatusApplications.IsEmpty() &&
		Definition.StatusRemovals.IsEmpty() &&
		Definition.ArmorEffects.IsEmpty() &&
		Definition.MovementEffects.IsEmpty() &&
		!Definition.TrapEffect.bPlaceTrap &&
		!Definition.bRelocatePartyToTargetCell;
}

bool UGridTurnManagerComponent::BuildExplorationOilTargetingPreview(
	const FGridAvailableCombatAction& Action, FIntPoint TargetCell,
	FGridCombatActionTargetingPreview& OutPreview) const
{
	return PrepareExplorationSurfaceTargeting(bCombatActive, RuntimeActor, PartyPawn,
		IsExplorationOilSlickAction(Action.Definition), Action, TargetCell, OutPreview) &&
		BuildTargetingPreviewForAction(Action, TargetCell, OutPreview);
}

bool UGridTurnManagerComponent::RequestExplorationOilActionAtCell(
	const FGridAvailableCombatAction& Action, FIntPoint TargetCell,
	FGridCombatActionRequestResult& OutResult)
{
	OutResult = FGridCombatActionRequestResult();
	OutResult.Action = Action;
	UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	if (bCombatActive || !IsValid(RuntimeActor) || !IsValid(Inventory) ||
		!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Action.CharacterIndex) ||
		!IsExplorationOilSlickAction(Action.Definition))
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::ActionUnavailable;
		return false;
	}

	// Never trust a cached HUD definition: resolve the actual owned item afresh.
	auto Source = FGridExplorationItemReadModel::ResolveSurface(
		Inventory, Action.CharacterIndex, Action.SourceDefinitionId, Action.Definition.ActionId, true);
	const FGridCombatActionDefinition& FreshDefinition = Source.Action;
	if (!Source.bResolved ||
		!IsExplorationOilSlickAction(FreshDefinition) ||
		FreshDefinition.ActionId != Action.Definition.ActionId ||
		Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex].CharacterId != Action.CharacterId ||
		Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex].Resources.CurrentHealth <= 0)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::QuickItemRejected;
		return false;
	}

	const int32 QuantityBefore = Inventory->CountItemDefinitionInCharacterInventory(
		Action.CharacterIndex, Action.SourceDefinitionId);
	if (QuantityBefore < 1)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::QuickItemRejected;
		return false;
	}
	const FGridAvailableCombatAction FreshAction = MakeExplorationSurfaceAction(Action, FreshDefinition, QuantityBefore);
	OutResult.Action = FreshAction;

	FGridCombatActionTargetingPreview Checked;
	if (!BuildExplorationOilTargetingPreview(FreshAction, TargetCell, Checked))
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidTarget;
		return false;
	}
	TArray<FIntPoint> ValidCells;
	for (const FIntPoint& Cell : Checked.AffectedCells)
	{
		if (RuntimeActor->IsWalkableCell(Cell.X, Cell.Y))
		{
			ValidCells.Add(Cell);
		}
	}
	if (ValidCells.IsEmpty() || !RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel())
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidTarget;
		return false;
	}

	// Validate the exact RPG03.6 payload before consuming inventory.
	const FGridCharacterInventoryState& Character =
		Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex];
	TArray<FGridCombatModifierProfile> Profiles;
	FGridResolvedCombatModifiers Modifiers;
	if (FGridCombatModifierResolver::CollectCharacterModifiers(Character, Profiles))
	{
		FGridCombatModifierResolver::Resolve(Profiles,
			FGridCombatModifierResolver::MakeActionContext(FreshDefinition, Action.SourceDefinitionId), Modifiers);
	}
	FGridCombatSurfaceState Prototype;
	if (!FGridCombatSurfaceResolver::BuildState(
		FreshDefinition.SurfaceEffects[0], Character.CharacterId, FreshDefinition.ActionId, Modifiers, Prototype))
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::UnsupportedResolution;
		return false;
	}
	const FGuid CharacterId = Character.CharacterId;
	if (!Inventory->RemoveItemDefinitionFromCharacterInventory(
		Action.CharacterIndex, Action.SourceDefinitionId, 1))
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::QuickItemRejected;
		return false;
	}

	RuntimeActor->BeginOilFlaskVisualBatch();
	int32 AppliedCells = 0;
	for (const FIntPoint& Cell : ValidCells)
	{
		if (RuntimeActor->ApplyCombatSurfaceAtCell(
			Cell.X, Cell.Y, FreshDefinition.SurfaceEffects[0], CharacterId, FreshDefinition.ActionId, Modifiers))
		{
			++AppliedCells;
		}
	}
	// The validated payload, cells and allocated runtime state make all applications
	// deterministic on this game thread. No combat turn and no action points are used.
	ensureMsgf(AppliedCells == ValidCells.Num(), TEXT("Exploration oil: prevalidated surfaces must all apply."));
	if (AppliedCells == 0) RuntimeActor->FinishOilFlaskVisualBatch();
	if (AppliedCells > 0)
	{
		RuntimeActor->PlayOilFlaskThrowAtCell(
			FIntPoint(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY), TargetCell,
			Source.Mesh.LoadSynchronous());
		GridEventFeedPublisher::PublishEvent(this,
			FGridCombatEventFeedFormatter::MakeOilSpread(FreshDefinition.ActionId, AppliedCells));
	}

	OutResult.bAccepted = AppliedCells == ValidCells.Num();
	OutResult.RejectReason = OutResult.bAccepted ? EGridCombatActionRequestRejectReason::None :
		EGridCombatActionRequestRejectReason::InvalidTarget;
	OutResult.TargetedActionResult.TargetCell = TargetCell;
	OutResult.TargetedActionResult.AffectedCells = ValidCells;
	OutResult.QuickItemResult.SourceQuantityBefore = QuantityBefore;
	OutResult.QuickItemResult.SourceQuantityAfter =
		Inventory->CountItemDefinitionInCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId);
	UE_LOG(LogGridTurnManager, Log,
		TEXT("[ARMEMENT01.17.1.7] ExplorationOil Accepted=%s Character=%d Cell=(%d,%d) Cells=%d Quantity=%d->%d"),
		OutResult.bAccepted ? TEXT("true") : TEXT("false"), Action.CharacterIndex, TargetCell.X, TargetCell.Y,
		AppliedCells, QuantityBefore, OutResult.QuickItemResult.SourceQuantityAfter);
	return OutResult.bAccepted;
}
