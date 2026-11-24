#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/Combat/GridAttackPresentationReadModel.h"
#include "Camera/CameraComponent.h"

#include "Core/GridDirectionUtils.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridDualWieldPolicy.h"
#include "Runtime/Combat/GridRangedAmmunitionPolicy.h"
#include "GridCombatPhysicalThrowSourceReadModel.h"
#include "../GridQuickItemUseReadModel.h"
#include "../GridPartyTransactionScope.h"
#include "GridEquippedAttackReadModel.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridQuickItemResolver.h"
#include "Runtime/Combat/GridCombatArmorEffectResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridCombatTargetingResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "CoreGlobals.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
	const FName UnarmedAttackId = TEXT("Attack_Unarmed");

	FGridOffensiveEquipmentProfile MakeUnarmedOffensiveProfile()
	{
		FGridOffensiveEquipmentProfile Profile;
		Profile.AttackId = UnarmedAttackId;
		Profile.AttackDefinition.DamageType = EGridDamageType::Physical;
		Profile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Bludgeoning;
		Profile.AttackDefinition.MinDamage = 1;
		Profile.AttackDefinition.MaxDamage = 3;
		Profile.AttackDefinition.AccuracyBonus = 0;
		Profile.FlatDamageBonus = 0;
		Profile.DamageScalingAttribute = EGridAttackScalingAttribute::Strength;
		Profile.RangeCells = 1;
		return Profile;
	}

	// Transient execution view of the sole canonical weapon source.
	// Used by ordinary attacks AND class actions that borrow equipped damage.
	bool TryResolveOffensiveProfile(
		const UGridItemSubsystem& Registry, const FGridItemInstance& Equipped,
		EGridEquipmentSlot SourceHand, FGridOffensiveEquipmentProfile& Out,
		FGridResolvedItem& OutItem)
	{
		Out = FGridOffensiveEquipmentProfile();
		OutItem = FGridResolvedItem();
		if (!Registry.ResolveItem(Equipped.ItemDefinitionId, Equipped.ItemLevel, OutItem) ||
			!OutItem.bIsArmament || !OutItem.Weapon.HasAttackSourceInHand(SourceHand))
		{
			return false;
		}
		for (const FGridItemActionBinding& Binding : OutItem.AvailableActions)
		{
			FGridCombatActionDefinition Action;
			if (!FGridEquippedAttackReadModel::TryResolve(
					&Registry, Equipped, SourceHand, Binding.ActionId, Action))
			{
				continue;
			}
			Out = Action.OffensiveProfile;
			return Out.IsValid();
		}
		return false;
	}

	int32 GetScalingAttributeValue(const FRPGAttributes& Attributes, EGridAttackScalingAttribute ScalingAttribute)
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

	bool IsCardinalFacing(EGridEdge Facing)
	{
		return Facing == EGridEdge::North || Facing == EGridEdge::East || Facing == EGridEdge::South || Facing == EGridEdge::West;
	}

	FString GetPlayerAttackRejectReasonText(EGridPlayerAttackRejectReason RejectReason)
	{
		const UEnum* RejectReasonEnum = StaticEnum<EGridPlayerAttackRejectReason>();
		return RejectReasonEnum ? RejectReasonEnum->GetNameStringByValue(static_cast<int64>(RejectReason)) : TEXT("Unknown");
	}

	bool IsRPG0392RearArc(const FIntPoint& SourceCell, const AGridMonsterActor* TargetMonster)
	{
		if (!IsValid(TargetMonster))
		{
			return false;
		}
		const FIntPoint TargetCell = TargetMonster->CurrentCell;
		switch (GridDirectionUtils::GetBackward(TargetMonster->Facing))
		{
			case EGridEdge::North: return SourceCell.X == TargetCell.X && SourceCell.Y > TargetCell.Y;
			case EGridEdge::East: return SourceCell.Y == TargetCell.Y && SourceCell.X > TargetCell.X;
			case EGridEdge::South: return SourceCell.X == TargetCell.X && SourceCell.Y < TargetCell.Y;
			case EGridEdge::West: return SourceCell.Y == TargetCell.Y && SourceCell.X < TargetCell.X;
			default: return false;
		}
	}

	bool HasRPG0392PhysicalControl(const AGridMonsterActor* TargetMonster)
	{
		if (!IsValid(TargetMonster))
		{
			return false;
		}
		return TargetMonster->StatusEffects.ActiveEffects.ContainsByPredicate(
			[](const FGridStatusEffectRuntimeState& State)
			{
				return State.IsValid() && IsValid(State.DefinitionAsset) &&
					State.DefinitionAsset->StatusTags.Contains(TEXT("Control.Physical"));
			});
	}

	bool HasRPG0392ActedThisRound(const UGridTurnManagerComponent* TurnManager, const AGridMonsterActor* TargetMonster)
	{
		if (!IsValid(TurnManager) || !IsValid(TargetMonster))
		{
			return false;
		}
		const FGuid Id = TargetMonster->ResolvePersistenceId();
		const FGridCombatantInitiativeEntry* Entry = TurnManager->InitiativeOrder.FindByPredicate(
			[Id](const FGridCombatantInitiativeEntry& Candidate)
			{
				return Candidate.Side == EGridCombatantSide::Monster && Candidate.CombatantId == Id;
			});
		return Entry && Entry->State != EGridCombatantTurnState::Waiting;
	}
}

bool UGridTurnManagerComponent::RequestSelectedCharacterAttack(
	FGridPlayerAttackRequest& OutRequest, FGridAttackResult& OutResult, EGridPlayerAttackRejectReason& OutRejectReason)
{
	const int32 SelectedCharacterIndex =
		IsValid(PartyPawn) && IsValid(PartyPawn->PartyInventoryComponent) ? PartyPawn->PartyInventoryComponent->GetSelectedCharacterIndex() : INDEX_NONE;
	return RequestCharacterAttack(SelectedCharacterIndex, OutRequest, OutResult, OutRejectReason);
}

bool UGridTurnManagerComponent::RequestCharacterAttack(
	int32 AttackerCharacterIndex, FGridPlayerAttackRequest& OutRequest, FGridAttackResult& OutResult, EGridPlayerAttackRejectReason& OutRejectReason)
{
	return RequestCharacterAttackInternal(AttackerCharacterIndex, EGridEquipmentSlot::None, false, nullptr, OutRequest, OutResult, OutRejectReason);
}

bool UGridTurnManagerComponent::RequestCharacterAttackFromSlot(int32 AttackerCharacterIndex, EGridEquipmentSlot RequestedEquipmentSlot,
	FGridPlayerAttackRequest& OutRequest, FGridAttackResult& OutResult, EGridPlayerAttackRejectReason& OutRejectReason)
{
	TArray<FGridAvailableCombatAction> AvailableActions;
	GetAvailableCombatActions(AttackerCharacterIndex, AvailableActions);
	const FGridAvailableCombatAction* FirstAttackFromSlot = AvailableActions.FindByPredicate(
		[RequestedEquipmentSlot](const FGridAvailableCombatAction& Action)
		{
			return Action.SourceEquipmentSlot == RequestedEquipmentSlot && Action.Definition.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
				Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack;
		});
	if (FirstAttackFromSlot)
	{
		return RequestCharacterAttackInternal(
			AttackerCharacterIndex, RequestedEquipmentSlot, true, FirstAttackFromSlot, OutRequest, OutResult, OutRejectReason);
	}

	return RequestCharacterAttackInternal(AttackerCharacterIndex, RequestedEquipmentSlot, true, nullptr, OutRequest, OutResult, OutRejectReason);
}

bool UGridTurnManagerComponent::RequestCharacterAttackInternal(int32 AttackerCharacterIndex, EGridEquipmentSlot RequestedEquipmentSlot,
	bool bRequireRequestedEquipmentSlot, const FGridAvailableCombatAction* CombatActionOverride, FGridPlayerAttackRequest& OutRequest,
	FGridAttackResult& OutResult, EGridPlayerAttackRejectReason& OutRejectReason)
{
	OutRequest = FGridPlayerAttackRequest();
	OutResult = FGridAttackResult();
	OutRejectReason = EGridPlayerAttackRejectReason::None;

	if (!bInitialized)
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TurnManagerNotInitialized, OutRejectReason);
	}
	if (!IsValid(RuntimeActor) || !IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::PartyUnavailable, OutRejectReason);
	}
	if (!bCombatActive)
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::CombatInactive, OutRejectReason);
	}
	if (CurrentPhase != EGridCombatPhase::PlayerPhase)
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::NotPlayerPhase, OutRejectReason);
	}
	if (!IsPartyAtRest() || bPlayerAttackResolutionInProgress)
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::PartyBusy, OutRejectReason);
	}

	TArray<FGridCharacterInventoryState>& Characters = PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
	if (!Characters.IsValidIndex(AttackerCharacterIndex))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidAttacker, OutRejectReason);
	}

	FGridCharacterInventoryState& Attacker = Characters[AttackerCharacterIndex];
	if (!Attacker.CharacterId.IsValid())
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidAttacker, OutRejectReason);
	}
	if (Attacker.Resources.CurrentHealth <= 0)
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::AttackerDefeated, OutRejectReason);
	}
	if (!InitiativeOrder.IsEmpty() && !IsActivePlayerCharacter(AttackerCharacterIndex))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::NotActiveCombatant, OutRejectReason);
	}
	if (!IsCardinalFacing(PartyPawn->Facing))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidFacing, OutRejectReason);
	}

	FGridOffensiveEquipmentProfile OffensiveProfile;
	FName OffensiveItemDefinitionId = NAME_None;
	EGridEquipmentSlot OffensiveEquipmentSlot = EGridEquipmentSlot::None;
	TArray<FName> ResolvedAttackSourceTags;
	EGridPlayerAttackRejectReason OffensiveProfileRejectReason = EGridPlayerAttackRejectReason::None;
	const bool bUsesEquippedWeaponAction =
		CombatActionOverride && CombatActionOverride->Definition.WeaponAttackProfile.bUseEquippedWeapon;
	if (CombatActionOverride)
	{
		const bool bValidOverride = CombatActionOverride->IsValid() && CombatActionOverride->bEnabled &&
			CombatActionOverride->CharacterIndex == AttackerCharacterIndex && CombatActionOverride->CharacterId == Attacker.CharacterId &&
			CombatActionOverride->Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
			(bUsesEquippedWeaponAction || CombatActionOverride->Definition.OffensiveProfile.IsValid());
		if (!bValidOverride)
		{
			return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
		}
		if (bUsesEquippedWeaponAction)
		{
			if (!ResolveCombatActionWeaponProfile(PartyPawn->PartyInventoryComponent, AttackerCharacterIndex, CombatActionOverride->Definition,
					OffensiveProfile, OffensiveItemDefinitionId, OffensiveEquipmentSlot, ResolvedAttackSourceTags, OffensiveProfileRejectReason))
			{
				return RejectPlayerAttack(AttackerCharacterIndex, OffensiveProfileRejectReason, OutRejectReason);
			}
		}
		else
		{
			OffensiveProfile = CombatActionOverride->Definition.OffensiveProfile;
			if (CombatActionOverride->Definition.SourcePolicy == EGridCombatActionSourcePolicy::Equipment ||
				CombatActionOverride->Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem)
			{
				OffensiveItemDefinitionId = CombatActionOverride->SourceDefinitionId;
			}
			if (CombatActionOverride->Definition.SourcePolicy == EGridCombatActionSourcePolicy::Equipment)
			{
				OffensiveEquipmentSlot = CombatActionOverride->SourceEquipmentSlot;
			}
		}
	}
	else if (!ResolvePlayerOffensiveProfile(PartyPawn->PartyInventoryComponent, AttackerCharacterIndex, RequestedEquipmentSlot, bRequireRequestedEquipmentSlot,
				 OffensiveProfile, OffensiveItemDefinitionId, OffensiveEquipmentSlot, OffensiveProfileRejectReason))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, OffensiveProfileRejectReason, OutRejectReason);
	}
	// A composite action is only executable while the exact two physical
	// weapons that generated the current catalogue entry are still equipped.
	// Both sources are validated before committing any AP or firing callbacks.
	const bool bDualWieldAction = CombatActionOverride &&
		CombatActionOverride->Definition.ActionId == FGridDualWieldPolicy::ActionId();
	FGridDualWieldPair DualPair;
	if (bDualWieldAction &&
		(CombatActionOverride->Definition.SourcePolicy != EGridCombatActionSourcePolicy::Equipment ||
			CombatActionOverride->SourceEquipmentSlot != EGridEquipmentSlot::MainHand ||
			CombatActionOverride->Definition.ResolutionCount != 2 ||
			!FGridDualWieldPolicy::TryResolve(PartyPawn->PartyInventoryComponent, AttackerCharacterIndex, DualPair) ||
			OffensiveEquipmentSlot != EGridEquipmentSlot::MainHand ||
			OffensiveItemDefinitionId != DualPair.Main.Item.ItemDefinitionId ||
			CombatActionOverride->SourceRuntimeId != DualPair.Main.Item.RuntimeObjectId ||
			CombatActionOverride->Definition.OffensiveProfile.AttackId != DualPair.Main.Profile.AttackId))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
	}
	const UWorld* AttackSourceWorld = GetWorld();
	UGameInstance* AttackSourceGI = AttackSourceWorld ? AttackSourceWorld->GetGameInstance() : nullptr;
	const UGridItemSubsystem* AttackRegistry = AttackSourceGI ? AttackSourceGI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	const bool bRegisteredAttackSource = AttackRegistry &&
		AttackRegistry->ContainsItem(OffensiveItemDefinitionId);
	FGridCombatActionDefinition DirectCanonicalAction;
	bool bRegisteredDetailedEquipmentAttack = false;
	if (bRegisteredAttackSource)
	{
		if (OffensiveEquipmentSlot != EGridEquipmentSlot::None)
		{
			FGridItemInstance ActualSource;
			FGridResolvedItem ResolvedSource;
			if (!PartyPawn->PartyInventoryComponent->GetEquippedItem(
					AttackerCharacterIndex, OffensiveEquipmentSlot, ActualSource) ||
				ActualSource.ItemDefinitionId != OffensiveItemDefinitionId ||
				!AttackRegistry->ResolveItem(ActualSource.ItemDefinitionId, ActualSource.ItemLevel, ResolvedSource) ||
				(!ResolvedSource.bIsArmament && ActualSource.ItemLevel != 0))
			{
				return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
			}
			ResolvedAttackSourceTags = ResolvedSource.Tags;
			if (!ResolvedSource.bIsArmament && CombatActionOverride)
			{
				FGridCombatActionDefinition AuthoredAction;
				if (bUsesEquippedWeaponAction ||
					!AttackRegistry->ResolveDetailedNonArmEquippedAction(ActualSource, OffensiveEquipmentSlot,
						CombatActionOverride->Definition.ActionId, AuthoredAction) ||
					AuthoredAction.ResolutionProfile != EGridCombatActionResolutionProfile::Attack ||
					AuthoredAction.ResourceCosts.SourceItemQuantityCost != CombatActionOverride->CurrentSourceItemQuantityCost ||
					ActualSource.Quantity > ResolvedSource.MaxStack ||
					ActualSource.OwnerType != EGridItemOwnerType::EquipmentSlot ||
					ActualSource.OwnerGuid != Attacker.CharacterId || ActualSource.OwnerCharacterIndex != AttackerCharacterIndex ||
					ActualSource.RuntimeObjectId != CombatActionOverride->SourceRuntimeId)
				{
					return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
				}
				bRegisteredDetailedEquipmentAttack = true;
			}
			if (!CombatActionOverride &&
				!FGridEquippedAttackReadModel::TryResolve(AttackRegistry, ActualSource,
					OffensiveEquipmentSlot, OffensiveProfile.AttackId, DirectCanonicalAction))
			{
				return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
			}
		}
		else if (CombatActionOverride)
		{
			// The action catalogue already projected the canonical QuickItem.
			ResolvedAttackSourceTags = CombatActionOverride->Definition.SourceTags;
		}
	}

	TArray<FGridCombatStatusApplicationProfile> AttackStatusApplications;
	TArray<FGridCombatArmorEffectProfile> AttackArmorEffects;
	if (CombatActionOverride)
	{
		AttackStatusApplications = CombatActionOverride->Definition.StatusApplications;
		AttackArmorEffects = CombatActionOverride->Definition.ArmorEffects;
	}
	else if (bRegisteredAttackSource)
	{
		// The same standard action as HUD/dual wield; its current compact
		// representation is proven lossless by the native retirement gate.
		AttackStatusApplications = DirectCanonicalAction.StatusApplications;
		AttackArmorEffects = DirectCanonicalAction.ArmorEffects;
	}

	TArray<FGridCombatModifierProfile> ChoiceModifiers;
	FGridResolvedCombatModifiers ResolvedAttackModifiers;
	if (FGridCombatModifierResolver::CollectCharacterModifiers(Attacker, ChoiceModifiers))
	{
		const EGridCombatActionSourcePolicy ModifierSourcePolicy = CombatActionOverride ? CombatActionOverride->Definition.SourcePolicy
			: OffensiveEquipmentSlot != EGridEquipmentSlot::None ? EGridCombatActionSourcePolicy::Equipment : EGridCombatActionSourcePolicy::Universal;
		const EGridCombatActionType ModifierActionType = CombatActionOverride ? CombatActionOverride->Definition.ActionType
			: OffensiveProfile.RangeCells > 1 ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
		const FName ModifierSourceDefinitionId = CombatActionOverride ? CombatActionOverride->SourceDefinitionId : OffensiveItemDefinitionId;
		const FGridCombatModifierContext ModifierContext = CombatActionOverride
			? (bUsesEquippedWeaponAction
					? FGridCombatModifierResolver::MakeResolvedActionAttackContext(
						CombatActionOverride->Definition, CombatActionOverride->SourceDefinitionId, OffensiveProfile, ResolvedAttackSourceTags)
					: FGridCombatModifierResolver::MakeActionContext(CombatActionOverride->Definition, CombatActionOverride->SourceDefinitionId))
			: FGridCombatModifierResolver::MakeAttackContext(OffensiveProfile.AttackId, ModifierSourceDefinitionId, ModifierSourcePolicy, ModifierActionType,
				OffensiveProfile.AttackDefinition.DamageType, OffensiveProfile.AttackDefinition.PhysicalSubtype, ResolvedAttackSourceTags);
		FGridCombatModifierResolver::Resolve(ChoiceModifiers, ModifierContext, ResolvedAttackModifiers);
		if (!CombatActionOverride)
		{
			OffensiveProfile.RangeCells =
				FMath::Clamp(OffensiveProfile.RangeCells + ResolvedAttackModifiers.RangeCellsModifier, 1, 32);
		}
	}

	const int32 SafeAttackActionPointCost = FMath::Clamp(CombatActionOverride ? CombatActionOverride->CurrentActionPointCost
																	 : PlayerAttackActionPointCost + ResolvedAttackModifiers.ActionPointCostModifier,
		1, 6);
	if (!CanCharacterSpendActionPoints(AttackerCharacterIndex, SafeAttackActionPointCost))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InsufficientActionPoints, OutRejectReason);
	}

	const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);

	UWorld* World = GetWorld();
	UGridMonsterOccupancySubsystem* Occupancy = World ? World->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr;
	if (!IsValid(Occupancy))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetCellUnavailable, OutRejectReason);
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
		if (!RuntimeActor->TryGetNeighborCell(SearchCell.X, SearchCell.Y, PartyPawn->Facing, NextCellX, NextCellY))
		{
			if (Distance == 1)
			{
				return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetCellUnavailable, OutRejectReason);
			}
			break;
		}
		if (!RuntimeActor->CanMove(SearchCell.X, SearchCell.Y, PartyPawn->Facing))
		{
			return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::PassageBlocked, OutRejectReason);
		}

		SearchCell = FIntPoint(NextCellX, NextCellY);
		if (AGridMonsterActor* Occupant = Occupancy->GetOccupantAtCell(SearchCell))
		{
			TargetMonster = Occupant;
			TargetCell = SearchCell;
			TargetDistance = Distance;
			break;
		}
	}

	if (!IsValid(TargetMonster))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::NoMonsterInFront, OutRejectReason);
	}

	if (TargetDistance > OffensiveProfile.RangeCells)
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetOutOfRange, OutRejectReason);
	}

	if (!IsCombatMonster(TargetMonster))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetNotInEncounter, OutRejectReason);
	}
	if (!TargetMonster->bMonsterEnabled)
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetInactive, OutRejectReason);
	}
	if (!TargetMonster->IsRuntimeLevelActive())
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetInactive, OutRejectReason);
	}
	if (TargetMonster->IsDead())
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetDefeated, OutRejectReason);
	}
	if (!FGridCombatTargetingResolver::IsDirectHostileTargetable(TargetMonster->StatusEffects))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetInactive, OutRejectReason);
	}
	if (CombatActionOverride)
	{
		const FName CategoryId = IsValid(TargetMonster->MonsterDefinition) ? TargetMonster->MonsterDefinition->CategoryId : NAME_None;
		FGridAttackTargetStats FilterTargetStats;
		FilterTargetStats.CurrentHealth = TargetMonster->CurrentHealth;
		FilterTargetStats.PhysicalArmor = TargetMonster->CurrentPhysicalArmor;
		FilterTargetStats.MagicalArmor = TargetMonster->CurrentMagicalArmor;
		const int32 FilterMaximumHealth =
			IsValid(TargetMonster->MonsterDefinition) ? FMath::Max(1, TargetMonster->MonsterDefinition->MaxHealth) : FMath::Max(1, TargetMonster->CurrentHealth);
		if (!FGridCombatTargetingResolver::MatchesTargetFilter(
				CombatActionOverride->Definition.TargetFilter, CategoryId, TargetMonster->StatusEffects, Attacker.CharacterId,
				&FilterTargetStats, FilterMaximumHealth))
		{
			return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetInactive, OutRejectReason);
		}
	}

	const FGuid TargetMonsterId = TargetMonster->ResolvePersistenceId();
	if (!TargetMonsterId.IsValid())
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetInactive, OutRejectReason);
	}

	const int32 GridDistance = FMath::Abs(TargetCell.X - PartyCell.X) + FMath::Abs(TargetCell.Y - PartyCell.Y);
	if (GridDistance < 1 || GridDistance > OffensiveProfile.RangeCells || GridDistance != TargetDistance)
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetOutOfRange, OutRejectReason);
	}
	const bool bResolvedRangedAttack = CombatActionOverride
		? CombatActionOverride->Definition.ActionType == EGridCombatActionType::RangedAttack
		: OffensiveProfile.RangeCells > 1;
	if (bResolvedRangedAttack && RuntimeActor->DoesCombatSmokeBlockLine(PartyCell, TargetCell))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::PassageBlocked, OutRejectReason);
	}

	FGridInventoryCharacterSummary CharacterSummary;
	if (!PartyPawn->PartyInventoryComponent->GetCharacterSummary(AttackerCharacterIndex, CharacterSummary))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidAttacker, OutRejectReason);
	}
	if (!IsValid(TargetMonster->MonsterDefinition))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetInactive, OutRejectReason);
	}

	// RPG03.9.2 target-aware C2 resolution. Cost/range projection happened before target discovery;
	// combat-output modifiers are resolved again with the authoritative target geometry/state.
	if (!ChoiceModifiers.IsEmpty())
	{
		const EGridCombatActionSourcePolicy TargetSourcePolicy = CombatActionOverride ? CombatActionOverride->Definition.SourcePolicy
			: OffensiveEquipmentSlot != EGridEquipmentSlot::None ? EGridCombatActionSourcePolicy::Equipment : EGridCombatActionSourcePolicy::Universal;
		const EGridCombatActionType TargetActionType = CombatActionOverride ? CombatActionOverride->Definition.ActionType
			: OffensiveProfile.RangeCells > 1 ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
		FGridCombatModifierContext TargetedContext = CombatActionOverride
			? (bUsesEquippedWeaponAction
					? FGridCombatModifierResolver::MakeResolvedActionAttackContext(
						CombatActionOverride->Definition, CombatActionOverride->SourceDefinitionId, OffensiveProfile, ResolvedAttackSourceTags)
					: FGridCombatModifierResolver::MakeActionContext(CombatActionOverride->Definition, CombatActionOverride->SourceDefinitionId))
			: FGridCombatModifierResolver::MakeAttackContext(OffensiveProfile.AttackId, OffensiveItemDefinitionId, TargetSourcePolicy, TargetActionType,
				OffensiveProfile.AttackDefinition.DamageType, OffensiveProfile.AttackDefinition.PhysicalSubtype, ResolvedAttackSourceTags);
		const FIntPoint ModifierSourceCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
		FGridCombatModifierResolver::AddTargetContext(TargetedContext,
			CombatActionOverride ? CombatActionOverride->Definition.TargetingPolicy : EGridCombatTargetingPolicy::FirstAxialTarget,
			IsRPG0392RearArc(ModifierSourceCell, TargetMonster), HasRPG0392ActedThisRound(this, TargetMonster),
			HasRPG0392PhysicalControl(TargetMonster));
		FGridCombatModifierResolver::AddTargetStatusContext(TargetedContext, TargetMonster->StatusEffects, Attacker.CharacterId,
			IsValid(TargetMonster->MonsterDefinition) ? TargetMonster->MonsterDefinition->CategoryId : NAME_None,
			IsValid(TargetMonster->MonsterDefinition) ? TargetMonster->MonsterDefinition->SemanticTags : TArray<FName>());
		if (const FGridCombatSurfaceState* TargetSurface =
				RuntimeActor->FindCombatSurfaceAtCell(TargetCell.X, TargetCell.Y))
		{
			FGridCombatModifierResolver::AddTargetSurfaceContext(TargetedContext, TargetSurface->SurfaceType);
		}
		TargetedContext.bPartyStationarySincePreviousActivation = IsPartyStationarySincePreviousActivation(Attacker.CharacterId);
		FGridCombatModifierResolver::Resolve(ChoiceModifiers, TargetedContext, ResolvedAttackModifiers);
	}

	FGridAttackSourceStats Source;
	FGridAttackTargetStats Target;
	FGridAttackDefinition AttackDefinition;
	if (!BuildPlayerAttackResolutionInputs(CharacterSummary, TargetMonster, OffensiveProfile, Source, Target, AttackDefinition))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetInactive, OutRejectReason);
	}
	if (bResolvedRangedAttack && RuntimeActor->IsCombatSmokeAtCell(TargetCell.X, TargetCell.Y))
	{
		Target.Evasion += 2;
	}

	if (CombatActionOverride)
	{
		FGridCombatModifierResolver::ApplyDirectDamageSkillScaling(
			CombatActionOverride->Definition, Attacker.SkillRanks, Source, &CharacterSummary.Attributes);
		FGridQuickItemResolver::ApplyDirectDamageSkillScaling(CombatActionOverride->Definition, Attacker.SkillRanks, Source);
		if (bUsesEquippedWeaponAction)
		{
			Source.RawDamagePercent = CombatActionOverride->Definition.WeaponAttackProfile.WeaponDamagePercent;
		}
	}
	FGridCombatModifierResolver::ApplyOutgoingAttackModifiers(Source, ResolvedAttackModifiers);
	TArray<FGridCombatModifierProfile> TargetStatusModifiers;
	FGridResolvedCombatModifiers TargetResolvedModifiers;
	if (FGridCombatModifierResolver::CollectStatusModifiers(TargetMonster->StatusEffects, TargetStatusModifiers))
	{
		const EGridCombatActionSourcePolicy TargetContextSourcePolicy = CombatActionOverride ? CombatActionOverride->Definition.SourcePolicy
			: OffensiveEquipmentSlot != EGridEquipmentSlot::None ? EGridCombatActionSourcePolicy::Equipment : EGridCombatActionSourcePolicy::Universal;
		const EGridCombatActionType TargetContextActionType = CombatActionOverride ? CombatActionOverride->Definition.ActionType
			: OffensiveProfile.RangeCells > 1 ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
		const FName TargetContextSourceDefinitionId = CombatActionOverride ? CombatActionOverride->SourceDefinitionId : OffensiveItemDefinitionId;
		const FGridCombatModifierContext TargetContext = CombatActionOverride
			? (bUsesEquippedWeaponAction
					? FGridCombatModifierResolver::MakeResolvedActionAttackContext(
						CombatActionOverride->Definition, CombatActionOverride->SourceDefinitionId, OffensiveProfile, ResolvedAttackSourceTags)
					: FGridCombatModifierResolver::MakeActionContext(CombatActionOverride->Definition, CombatActionOverride->SourceDefinitionId))
			: FGridCombatModifierResolver::MakeAttackContext(OffensiveProfile.AttackId, TargetContextSourceDefinitionId, TargetContextSourcePolicy,
				TargetContextActionType, AttackDefinition.DamageType, AttackDefinition.PhysicalSubtype);
		FGridCombatModifierResolver::Resolve(TargetStatusModifiers, TargetContext, TargetResolvedModifiers);
		FGridCombatModifierResolver::ApplyIncomingAttackModifiers(Target, AttackDefinition.DamageType, TargetResolvedModifiers);
	}

	FGridPlayerAttackRequest Request;
	Request.RequestId = FGuid::NewGuid();
	Request.RoundNumber = RoundNumber;
	Request.AttackerCharacterIndex = AttackerCharacterIndex;
	Request.AttackerCharacterId = Attacker.CharacterId;
	Request.TargetMonsterId = TargetMonsterId;
	Request.PartyCell = PartyCell;
	Request.TargetCell = TargetCell;
	Request.PartyFacing = PartyPawn->Facing;
	Request.RangeCells = OffensiveProfile.RangeCells;
	Request.AttackId = OffensiveProfile.AttackId;
	Request.OffensiveItemDefinitionId = OffensiveItemDefinitionId;
	Request.OffensiveEquipmentSlot = OffensiveEquipmentSlot;
	Request.ActionPointCost = SafeAttackActionPointCost;
	Request.PresentationSource = FGridAttackPresentationReadModel::Capture(
		AttackRegistry, PartyPawn->PartyInventoryComponent.Get(), AttackerCharacterIndex,
		OffensiveItemDefinitionId, OffensiveEquipmentSlot, Request.AttackId, bUsesEquippedWeaponAction);
	// Capture both sources before the first AP spend or callback.
	const FGridAttackPresentationSource OffPresentationSource = bDualWieldAction
		? FGridAttackPresentationReadModel::Capture(
			AttackRegistry, PartyPawn->PartyInventoryComponent.Get(), AttackerCharacterIndex,
			DualPair.Off.Item.ItemDefinitionId, EGridEquipmentSlot::OffHand, DualPair.Off.Action.ActionId, false)
		: FGridAttackPresentationSource();
	if (!Request.IsValid())
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::TargetCellUnavailable, OutRejectReason);
	}

	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent.Get();
	const EGridCombatActionSourcePolicy SourcePolicy = CombatActionOverride ? CombatActionOverride->Definition.SourcePolicy
		: OffensiveEquipmentSlot != EGridEquipmentSlot::None                ? EGridCombatActionSourcePolicy::Equipment
																			: EGridCombatActionSourcePolicy::Universal;
	const int32 ManaCost = CombatActionOverride ? FMath::Max(0, CombatActionOverride->CurrentManaCost) : 0;
	const int32 AttackResolutionCount =
		CombatActionOverride ? FMath::Clamp(CombatActionOverride->Definition.ResolutionCount, 1, 8) : 1;
	Request.ActionInstanceId = Request.RequestId;
	Request.ResolutionCount = AttackResolutionCount;
	Request.ResolutionIndex = 0;
	const int32 SubsequentAccuracyModifier =
		CombatActionOverride ? CombatActionOverride->Definition.SubsequentResolutionAccuracyModifier : 0;
	int32 SourceItemQuantityCost = CombatActionOverride ? FMath::Max(0, CombatActionOverride->CurrentSourceItemQuantityCost) : 0;
	// Read one exact owned source for both ammo and the physical throw.
	// A registered item never borrows old DA_Weapon_* flags or ammo IDs.
	const UGridItemSubsystem* AmmoRegistry = World && World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<UGridItemSubsystem>() : nullptr;
	FGridItemInstance AmmoSourceInstance;
	if (SourcePolicy == EGridCombatActionSourcePolicy::Equipment)
	{
		Inventory->GetEquippedItem(AttackerCharacterIndex, OffensiveEquipmentSlot, AmmoSourceInstance);
	}
	else if (SourcePolicy == EGridCombatActionSourcePolicy::QuickItem)
	{
		const FGridInventorySlot* SourceSlot = Attacker.InventorySlots.FindByPredicate(
			[OffensiveItemDefinitionId](const FGridInventorySlot& Slot)
			{
				return !Slot.IsEmpty() && Slot.Item.ItemDefinitionId == OffensiveItemDefinitionId;
			});
		if (SourceSlot)
		{
			AmmoSourceInstance = SourceSlot->Item;
		}
	}
	// A bound offensive consumable uses the ordinary attack transaction. Its
	// inventory debit is the source cost; it supplies neither a weapon
	// projectile nor ammunition. Check every matching stack before the
	// definition-based debit, which can consume from any of those stacks.
	const bool bRegisteredQuickItemAttack =
		AmmoRegistry && SourcePolicy == EGridCombatActionSourcePolicy::QuickItem;
	if (bRegisteredQuickItemAttack)
	{
		FGridResolvedItem Resolved;
		const FGridQuickItemUseReadView Use = FGridQuickItemUseReadModel::Resolve(AmmoRegistry, AmmoSourceInstance);
		if (bUsesEquippedWeaponAction || !Use.bResolved || Use.Action.ActionId != CombatActionOverride->Definition.ActionId ||
			Use.Action.ResolutionProfile != EGridCombatActionResolutionProfile::Attack ||
			SourceItemQuantityCost != 1 || Use.Action.ResourceCosts.SourceItemQuantityCost != SourceItemQuantityCost ||
			!AmmoRegistry->ResolveItem(AmmoSourceInstance.ItemDefinitionId, AmmoSourceInstance.ItemLevel, Resolved) ||
			Resolved.bIsArmament || AmmoSourceInstance.ItemLevel != 0)
		{
			return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
		}
		for (const FGridInventorySlot& Slot : Attacker.InventorySlots)
		{
			if (Slot.bOccupied && Slot.Item.ItemDefinitionId == OffensiveItemDefinitionId &&
				(!Slot.Item.IsValid() || Slot.Item.ItemLevel != 0 || Slot.Item.Quantity > Resolved.MaxStack ||
				 Slot.Item.OwnerType != EGridItemOwnerType::CharacterInventory ||
				 Slot.Item.OwnerGuid != Attacker.CharacterId || Slot.Item.OwnerCharacterIndex != AttackerCharacterIndex))
			{
				return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
			}
		}
	}
	const bool bItemSourceAttack =
		(SourcePolicy == EGridCombatActionSourcePolicy::Equipment && OffensiveEquipmentSlot != EGridEquipmentSlot::None &&
			!bRegisteredDetailedEquipmentAttack) ||
		(SourcePolicy == EGridCombatActionSourcePolicy::QuickItem && !bRegisteredQuickItemAttack);
	const FGridCombatPhysicalThrowSourceReadView ThrowSource = bItemSourceAttack
		? FGridCombatPhysicalThrowSourceReadModel::Resolve(
			AmmoRegistry, nullptr, AmmoSourceInstance,
			SourceItemQuantityCost, bResolvedRangedAttack)
		: FGridCombatPhysicalThrowSourceReadView();
	if (ThrowSource.bRejectAction)
	{
		return RejectPlayerAttack(AttackerCharacterIndex,
			EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
	}
	const bool bPhysicalCombatThrow = ThrowSource.bLaunchPhysicalProjectile;
	if (bPhysicalCombatThrow)
	{
		// Legacy item definitions already forced one unit; canonical thrown
		// weapons retain their ONE-unit cost from the ActionCatalogue.
		SourceItemQuantityCost = 1;
	}
	const FGridRangedAmmunitionRequirement AmmoRequirement =
		FGridRangedAmmunitionPolicy::ResolveForAttack(
			AmmoRegistry, OffensiveItemDefinitionId, AmmoSourceInstance.ItemLevel,
			nullptr, bResolvedRangedAttack && bItemSourceAttack);
	// Preflight before reserving AP, mana or the source item.
	if (!FGridRangedAmmunitionPolicy::HasRequiredAmmunitionForAttack(
			*Inventory, AttackerCharacterIndex, AmmoRequirement))
	{
		return RejectPlayerAttack(AttackerCharacterIndex,
			EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
	}

	FGridItemInstance EquippedSourceItem;
	if (Attacker.Resources.CurrentMana < ManaCost ||
		(SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
			(OffensiveItemDefinitionId.IsNone() || SourceItemQuantityCost <= 0 ||
				Inventory->CountItemDefinitionInCharacterInventory(AttackerCharacterIndex, OffensiveItemDefinitionId) < SourceItemQuantityCost)) ||
		(SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
			(!Inventory->GetEquippedItem(AttackerCharacterIndex, OffensiveEquipmentSlot, EquippedSourceItem) ||
				EquippedSourceItem.ItemDefinitionId != OffensiveItemDefinitionId ||
				(CombatActionOverride && CombatActionOverride->SourceRuntimeId.IsValid() &&
					EquippedSourceItem.RuntimeObjectId != CombatActionOverride->SourceRuntimeId) ||
				FMath::Max(1, EquippedSourceItem.Quantity) < SourceItemQuantityCost ||
				(SourceItemQuantityCost > 0 && bPhysicalCombatThrow && SourceItemQuantityCost != 1))))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
	}

	// Protect the canonical source across synchronous AP, inventory and
	// resolution notifications. Only this transaction may debit it.
	TUniquePtr<FGridPartyTransactionScope> SourceTransaction;
	if (bRegisteredQuickItemAttack || bRegisteredDetailedEquipmentAttack)
	{
		SourceTransaction = MakeUnique<FGridPartyTransactionScope>(Inventory, TArray<int32>{AttackerCharacterIndex});
		if (!SourceTransaction->IsAcquired() || !SourceTransaction->ReserveItem(AmmoSourceInstance.RuntimeObjectId))
		{
			return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
		}
	}

	FGridPlayerCharacterTurnState TurnStateBefore;
	if (!GetPlayerCharacterTurnState(AttackerCharacterIndex, TurnStateBefore) ||
		!SpendPlayerCharacterActionPoints(AttackerCharacterIndex, Request.ActionPointCost))
	{
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InsufficientActionPoints, OutRejectReason);
	}

	const auto RestoreReservedActionPoints = [this, AttackerCharacterIndex, &TurnStateBefore]()
	{
		if (FGridPlayerCharacterTurnState* RestoredTurnState = EnsurePlayerCharacterTurnState(AttackerCharacterIndex))
		{
			*RestoredTurnState = TurnStateBefore;
			BroadcastPlayerCharacterTurnState(*RestoredTurnState);
		}
	};

AGridThrownItemActor* PreparedThrownItem = nullptr;
	AGridThrownItemActor* PreparedAmmunitionItem = nullptr;
	bool bSourceCommitted = true;
	// A consumed ranged ammunition unit is represented by a recoverable world
	// projectile. Prepare it before inventory mutation, avoiding lost arrows
	// when spawning fails; gameplay still resolves damage exactly once.
	if (bResolvedRangedAttack && !AmmoRequirement.AmmunitionItemId.IsNone())
	{
		const FName AmmunitionId = AmmoRequirement.AmmunitionItemId;
		const float LaunchSpeed = AmmoRequirement.AmmunitionLaunchSpeed;
		bSourceCommitted = IsValid(RuntimeActor) && AmmoRequirement.bRegisteredAmmunition &&
			FMath::IsFinite(LaunchSpeed) && LaunchSpeed > KINDA_SMALL_NUMBER;
		if (bSourceCommitted)
		{
			FGridItemInstance ProjectileItem;
			ProjectileItem.RuntimeObjectId = FGuid::NewGuid();
			ProjectileItem.ItemDefinitionId = AmmunitionId;
			ProjectileItem.ItemLevel = 0;
			ProjectileItem.Quantity = 1;
			ProjectileItem.OwnerType = EGridItemOwnerType::World;
			ProjectileItem.OwnerCharacterIndex = INDEX_NONE;
			ProjectileItem.EquipmentSlot = EGridEquipmentSlot::None;
			const FVector ViewOrigin = PartyPawn->Camera ? PartyPawn->Camera->GetComponentLocation() : PartyPawn->GetActorLocation();
			const FVector TargetLocation = TargetMonster->GetComponentsBoundingBox().GetCenter();
			const FVector InitialDirection = (TargetLocation - ViewOrigin).GetSafeNormal();
			const FVector StartLocation = ViewOrigin + InitialDirection * 60.0f - FVector::UpVector * 12.0f;
			const FVector LaunchDirection = (TargetLocation - StartLocation).GetSafeNormal();
			PreparedAmmunitionItem = RuntimeActor->SpawnThrownItemProjectile(
				ProjectileItem, StartLocation, LaunchDirection * LaunchSpeed, PartyCell.X, PartyCell.Y, true);
			bSourceCommitted = IsValid(PreparedAmmunitionItem);
		}
		if (!bSourceCommitted)
		{
			UE_LOG(LogGridTurnManager, Warning, TEXT("[GridRangedAmmo] LaunchFailed Weapon=%s Ammo=%s Character=%d"),
				*OffensiveItemDefinitionId.ToString(), *AmmunitionId.ToString(), AttackerCharacterIndex);
		}
	}
	if (bSourceCommitted && SourcePolicy == EGridCombatActionSourcePolicy::Equipment && SourceItemQuantityCost > 0)
	{
		if (bPhysicalCombatThrow)
		{
			PreparedThrownItem = PartyPawn->TryLaunchEquippedItemForAttack(
				AttackerCharacterIndex, OffensiveEquipmentSlot, OffensiveItemDefinitionId, TargetMonster->GetActorLocation(), PartyCell);
			bSourceCommitted = IsValid(PreparedThrownItem);
		}
		else
		{
			const FGuid RuntimeId = CombatActionOverride ? CombatActionOverride->SourceRuntimeId : FGuid();
			bSourceCommitted = SourceTransaction
				? Inventory->ConsumeEquippedItemQuantityInternal(AttackerCharacterIndex, OffensiveEquipmentSlot,
					OffensiveItemDefinitionId, RuntimeId, SourceItemQuantityCost)
				: Inventory->TryConsumeEquippedItemQuantityForCombatAction(AttackerCharacterIndex, OffensiveEquipmentSlot,
					OffensiveItemDefinitionId, RuntimeId, SourceItemQuantityCost);
		}
	}
	else if (bSourceCommitted && SourcePolicy == EGridCombatActionSourcePolicy::QuickItem)
	{
		if (bPhysicalCombatThrow)
		{
			PreparedThrownItem =
				PartyPawn->TryLaunchInventoryItemForAttack(AttackerCharacterIndex, OffensiveItemDefinitionId, TargetMonster->GetActorLocation(), PartyCell);
			bSourceCommitted = IsValid(PreparedThrownItem);
		}
		if (bSourceCommitted)
		{
			bSourceCommitted = SourceTransaction
				? Inventory->RemoveItemDefinitionFromCharacterInventoryInternal(AttackerCharacterIndex, OffensiveItemDefinitionId, SourceItemQuantityCost)
				: Inventory->RemoveItemDefinitionFromCharacterInventory(AttackerCharacterIndex, OffensiveItemDefinitionId, SourceItemQuantityCost);
		}
	}

	// A catalogue-authorized ranged weapon has no other consumable source
	// cost. Consume exactly one ammunition unit only after action acceptance,
	// never on a refused target or failed throw.
	if (bSourceCommitted)
	{
		bSourceCommitted = FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunitionForAttack(
			*Inventory, AttackerCharacterIndex, AmmoRequirement);
	}

	if (!bSourceCommitted)
	{
		if (IsValid(PreparedAmmunitionItem))
		{
			PreparedAmmunitionItem->Destroy();
		}
		if (IsValid(PreparedThrownItem))
		{
			PreparedThrownItem->Destroy();
		}
		RestoreReservedActionPoints();
		return RejectPlayerAttack(AttackerCharacterIndex, EGridPlayerAttackRejectReason::InvalidOffensiveEquipment, OutRejectReason);
	}

	Attacker.Resources.CurrentMana = FMath::Max(0, Attacker.Resources.CurrentMana - ManaCost);
	Request.PreparedThrownItemActor = IsValid(PreparedAmmunitionItem) ? PreparedAmmunitionItem : PreparedThrownItem;
	const bool bCooldownStarted = CombatActionOverride && CombatActionOverride->Definition.CooldownRounds > 0;
	if (CombatActionOverride)
	{
		StartCombatActionCooldown(*CombatActionOverride);
	}
	FGridAttackResult Result = FGridCombatResolver::ResolveAttack(Source, Target, AttackDefinition, CombatRandomStream);
	const FGridCombatArmorEffectSourceContext ArmorSourceContext =
		FGridCombatArmorEffectResolver::MakeSourceContext(Attacker, CharacterSummary.Attributes);
	FGridCombatArmorPoolSnapshot TargetArmorSnapshot;
	TargetArmorSnapshot.CurrentPhysicalArmor = Target.PhysicalArmor;
	TargetArmorSnapshot.CurrentMagicalArmor = Target.MagicalArmor;
	TargetArmorSnapshot.ReferencePhysicalArmor =
		IsValid(TargetMonster->MonsterDefinition) ? FMath::Max(0, TargetMonster->MonsterDefinition->PhysicalArmor) : Target.PhysicalArmor;
	TargetArmorSnapshot.ReferenceMagicalArmor =
		IsValid(TargetMonster->MonsterDefinition) ? FMath::Max(0, TargetMonster->MonsterDefinition->MagicalArmor) : Target.MagicalArmor;
	FGridCombatArmorEffectResolver::ApplyReferenceModifiers(TargetArmorSnapshot, TargetResolvedModifiers);
	FGridCombatArmorEffectResolver::ApplyAttackDamageEffects(
		AttackArmorEffects, TargetArmorSnapshot, TargetResolvedModifiers, Result, &ArmorSourceContext);

	OutRequest = Request;
	OutResult = Result;
	OutRejectReason = EGridPlayerAttackRejectReason::None;
	LastPlayerAttackRequest = Request;
	LastPlayerAttackResult = Result;
	LastPlayerAttackRejectReason = EGridPlayerAttackRejectReason::None;

	UE_LOG(LogGridTurnManager, Log,
		TEXT(
			"[GridPlayerAttack] Accepted=true Request=%s Round=%d Attacker=%d Character=%s Target=%s PartyCell=(%d,%d) TargetCell=(%d,%d) Facing=%s Range=%d Attack=%s Item=%s Slot=%s APCost=%d"),
		*Request.RequestId.ToString(EGuidFormats::Digits), Request.RoundNumber, Request.AttackerCharacterIndex,
		*Request.AttackerCharacterId.ToString(EGuidFormats::Digits), *Request.TargetMonsterId.ToString(EGuidFormats::Digits), Request.PartyCell.X,
		Request.PartyCell.Y, Request.TargetCell.X, Request.TargetCell.Y, *UEnum::GetValueAsString(Request.PartyFacing), Request.RangeCells,
		*Request.AttackId.ToString(), *Request.OffensiveItemDefinitionId.ToString(), *UEnum::GetValueAsString(Request.OffensiveEquipmentSlot),
		Request.ActionPointCost);
	++PlayerAttackRequestedBroadcastCount;
	OnPlayerAttackRequested.Broadcast(Request);

	FGridCombatLogEntry AttackEntry;
	AttackEntry.RoundNumber = RoundNumber;
	AttackEntry.Phase = CurrentPhase;
	AttackEntry.Type = Result.bHit ? EGridCombatLogEntryType::AttackHit : EGridCombatLogEntryType::AttackMiss;
	AttackEntry.SourceId = FName(*Request.AttackerCharacterId.ToString(EGuidFormats::Digits));
	AttackEntry.SourceDisplayName = CharacterSummary.DisplayName;
	AttackEntry.TargetId = FName(*TargetMonsterId.ToString(EGuidFormats::Digits));
	AttackEntry.TargetDisplayName = ResolveMonsterDisplayName(TargetMonster);
	AttackEntry.TargetCharacterIndex = INDEX_NONE;
	AttackEntry.AttackId = Request.AttackId;
	AttackEntry.OffensiveItemDefinitionId = Request.OffensiveItemDefinitionId;
	AttackEntry.OffensiveEquipmentSlot = Request.OffensiveEquipmentSlot;
	AttackEntry.AttackResult = Result;
	AttackEntry.bTargetDefeated = Result.TargetHealthBefore > 0 && Result.TargetHealthAfter <= 0;
	AttackEntry.Message =
		FGridCombatLogFormatter::FormatPlayerAttack(AttackEntry.SourceDisplayName, AttackEntry.TargetDisplayName, AttackEntry.AttackId, Result);
	AppendCombatLogEntry(AttackEntry);

	bPlayerAttackResolutionInProgress = true;
	TargetMonster->ApplyAttackResult(Result);
	if (!AttackStatusApplications.IsEmpty())
	{
		if (UWorld* StatusWorld = GetWorld())
		{
			if (UGridStatusEffectLifecycleSubsystem* StatusLifecycle = StatusWorld->GetSubsystem<UGridStatusEffectLifecycleSubsystem>())
			{
				StatusLifecycle->BindToTurnManager(this);
				StatusLifecycle->ApplyCombatStatusApplicationsToMonster(
					TargetMonster, AttackStatusApplications, Attacker.CharacterId, Target, &Result);
			}
		}
	}
	if (FGridCombatantInitiativeEntry* TargetEntry = FindInitiativeEntry(EGridCombatantSide::Monster, TargetMonsterId))
	{
		const int32 PreviousHealth = TargetEntry->CurrentHealth;
		RefreshInitiativeEntryVitals(*TargetEntry);
		if (TargetEntry->CurrentHealth != PreviousHealth && TargetEntry->State != EGridCombatantTurnState::Defeated)
		{
			OnCombatantStateChanged.Broadcast(*TargetEntry);
		}
	}
	const EGridCombatActionSourcePolicy ReactionSourcePolicy = CombatActionOverride ? CombatActionOverride->Definition.SourcePolicy
		: OffensiveEquipmentSlot != EGridEquipmentSlot::None ? EGridCombatActionSourcePolicy::Equipment : EGridCombatActionSourcePolicy::Universal;
	const EGridCombatActionType ReactionActionType = CombatActionOverride ? CombatActionOverride->Definition.ActionType
		: OffensiveProfile.RangeCells > 1 ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
	const FGuid ReactionActionInstanceId = Request.RequestId;
	EmitPlayerAttackReactionEvents(AttackerCharacterIndex, Request, Result, ReactionSourcePolicy, ReactionActionType,
		ReactionActionInstanceId, false, AttackResolutionCount == 1, ResolvedAttackSourceTags,
		OffensiveEquipmentSlot != EGridEquipmentSlot::None);
	++PlayerAttackResolvedBroadcastCount;
	bPlayerAttackResolutionInProgress = false;
	OnPlayerAttackResolved.Broadcast(Request, TargetMonster, Result);

	if (bCollectRuntimeMetrics)
	{
		++RuntimeMetrics.AttacksResolved;
	}

	for (int32 ResolutionIndex = 1; ResolutionIndex < AttackResolutionCount && IsValid(TargetMonster) && !TargetMonster->IsDead(); ++ResolutionIndex)
	{
		// No stale OffHand identity: a reaction or delegate might change
		// equipment after the first hit. Never strike with a removed item.
		if (bDualWieldAction)
		{
			FGridDualWieldPair CurrentPair;
			if (!FGridDualWieldPolicy::TryResolve(Inventory, AttackerCharacterIndex, CurrentPair) ||
				CurrentPair.Main.Item.RuntimeObjectId != DualPair.Main.Item.RuntimeObjectId ||
				CurrentPair.Off.Item.RuntimeObjectId != DualPair.Off.Item.RuntimeObjectId ||
					CurrentPair.Main.Item.ItemLevel != DualPair.Main.Item.ItemLevel ||
					CurrentPair.Off.Item.ItemLevel != DualPair.Off.Item.ItemLevel ||
					CurrentPair.Off.Profile.AttackId != DualPair.Off.Profile.AttackId)
			{
				break;
			}
		}
		const FGridOffensiveEquipmentProfile& RepeatProfile = bDualWieldAction ? DualPair.Off.Profile : OffensiveProfile;
		FGridInventoryCharacterSummary RepeatCharacterSummary = CharacterSummary;
		if (bDualWieldAction && !Inventory->GetCharacterSummary(AttackerCharacterIndex, RepeatCharacterSummary))
		{
			break;
		}
		FGridAttackSourceStats RepeatSource;
		FGridAttackTargetStats RepeatTarget;
		FGridAttackDefinition RepeatAttackDefinition;
		if (!BuildPlayerAttackResolutionInputs(RepeatCharacterSummary, TargetMonster, RepeatProfile, RepeatSource, RepeatTarget, RepeatAttackDefinition))
		{
			break;
		}
		if (bResolvedRangedAttack && RuntimeActor->IsCombatSmokeAtCell(TargetCell.X, TargetCell.Y))
		{
			RepeatTarget.Evasion += 2;
		}
		FGridResolvedCombatModifiers RepeatOutgoingModifiers = ResolvedAttackModifiers;
		if (bDualWieldAction)
		{
			// Never carry MainHand-specific bonuses, stealth consumed on the
			// first hit, or its weapon tags onto the secondary weapon.
			RepeatOutgoingModifiers.Reset();
			TArray<FGridCombatModifierProfile> OffHandModifiers;
			if (FGridCombatModifierResolver::CollectCharacterModifiers(Attacker, OffHandModifiers))
			{
				FGridCombatModifierContext OffHandContext = FGridCombatModifierResolver::MakeAttackContext(
					DualPair.Off.Action.ActionId, DualPair.Off.Item.ItemDefinitionId, EGridCombatActionSourcePolicy::Equipment,
					EGridCombatActionType::MeleeAttack, RepeatAttackDefinition.DamageType, RepeatAttackDefinition.PhysicalSubtype,
					DualPair.Off.SourceTags);
				FGridCombatModifierResolver::AddTargetContext(OffHandContext, EGridCombatTargetingPolicy::FirstAxialTarget,
					IsRPG0392RearArc(PartyCell, TargetMonster), HasRPG0392ActedThisRound(this, TargetMonster),
					HasRPG0392PhysicalControl(TargetMonster));
				FGridCombatModifierResolver::AddTargetStatusContext(OffHandContext, TargetMonster->StatusEffects,
					Attacker.CharacterId, TargetMonster->MonsterDefinition->CategoryId,
					TargetMonster->MonsterDefinition->SemanticTags);
				if (const FGridCombatSurfaceState* Surface = RuntimeActor->FindCombatSurfaceAtCell(TargetCell.X, TargetCell.Y))
				{
					FGridCombatModifierResolver::AddTargetSurfaceContext(OffHandContext, Surface->SurfaceType);
				}
				OffHandContext.bPartyStationarySincePreviousActivation = IsPartyStationarySincePreviousActivation(Attacker.CharacterId);
				FGridCombatModifierResolver::Resolve(OffHandModifiers, OffHandContext, RepeatOutgoingModifiers);
			}
			FGridCombatModifierResolver::ApplyDirectDamageSkillScaling(
				DualPair.Off.Action, Attacker.SkillRanks, RepeatSource, &RepeatCharacterSummary.Attributes);
			RepeatSource.RawDamagePercent = FGridDualWieldPolicy::OffHandDamagePercent;
		}
		else if (CombatActionOverride)
		{
			FGridCombatModifierResolver::ApplyDirectDamageSkillScaling(
				CombatActionOverride->Definition, Attacker.SkillRanks, RepeatSource, &CharacterSummary.Attributes);
			FGridQuickItemResolver::ApplyDirectDamageSkillScaling(CombatActionOverride->Definition, Attacker.SkillRanks, RepeatSource);
			if (bUsesEquippedWeaponAction)
			{
				RepeatSource.RawDamagePercent = CombatActionOverride->Definition.WeaponAttackProfile.WeaponDamagePercent;
			}
		}
		FGridCombatModifierResolver::ApplyOutgoingAttackModifiers(RepeatSource, RepeatOutgoingModifiers);
		RepeatSource.Accuracy += SubsequentAccuracyModifier;

		TArray<FGridCombatModifierProfile> RepeatTargetStatusProfiles;
		FGridResolvedCombatModifiers RepeatTargetModifiers;
		if (FGridCombatModifierResolver::CollectStatusModifiers(TargetMonster->StatusEffects, RepeatTargetStatusProfiles))
		{
			const FGridCombatModifierContext RepeatTargetContext = bDualWieldAction
				? FGridCombatModifierResolver::MakeAttackContext(
					DualPair.Off.Action.ActionId, DualPair.Off.Item.ItemDefinitionId, EGridCombatActionSourcePolicy::Equipment,
					EGridCombatActionType::MeleeAttack, RepeatAttackDefinition.DamageType, RepeatAttackDefinition.PhysicalSubtype,
					DualPair.Off.SourceTags)
				: CombatActionOverride
				? (bUsesEquippedWeaponAction
						? FGridCombatModifierResolver::MakeResolvedActionAttackContext(
							CombatActionOverride->Definition, CombatActionOverride->SourceDefinitionId, OffensiveProfile, ResolvedAttackSourceTags)
						: FGridCombatModifierResolver::MakeActionContext(CombatActionOverride->Definition, CombatActionOverride->SourceDefinitionId))
				: FGridCombatModifierResolver::MakeAttackContext(OffensiveProfile.AttackId, OffensiveItemDefinitionId, ReactionSourcePolicy,
					ReactionActionType, RepeatAttackDefinition.DamageType, RepeatAttackDefinition.PhysicalSubtype);
			FGridCombatModifierResolver::Resolve(RepeatTargetStatusProfiles, RepeatTargetContext, RepeatTargetModifiers);
			FGridCombatModifierResolver::ApplyIncomingAttackModifiers(
				RepeatTarget, RepeatAttackDefinition.DamageType, RepeatTargetModifiers);
		}

		FGridAttackResult RepeatResult =
			FGridCombatResolver::ResolveAttack(RepeatSource, RepeatTarget, RepeatAttackDefinition, CombatRandomStream);
		FGridCombatArmorPoolSnapshot RepeatArmorSnapshot;
		RepeatArmorSnapshot.CurrentPhysicalArmor = RepeatTarget.PhysicalArmor;
		RepeatArmorSnapshot.CurrentMagicalArmor = RepeatTarget.MagicalArmor;
		RepeatArmorSnapshot.ReferencePhysicalArmor = IsValid(TargetMonster->MonsterDefinition)
			? FMath::Max(0, TargetMonster->MonsterDefinition->PhysicalArmor)
			: RepeatTarget.PhysicalArmor;
		RepeatArmorSnapshot.ReferenceMagicalArmor = IsValid(TargetMonster->MonsterDefinition)
			? FMath::Max(0, TargetMonster->MonsterDefinition->MagicalArmor)
			: RepeatTarget.MagicalArmor;
		FGridCombatArmorEffectResolver::ApplyReferenceModifiers(RepeatArmorSnapshot, RepeatTargetModifiers);
		FGridCombatArmorEffectResolver::ApplyAttackDamageEffects(
			bDualWieldAction ? DualPair.Off.Action.ArmorEffects : AttackArmorEffects,
			RepeatArmorSnapshot, RepeatTargetModifiers, RepeatResult, &ArmorSourceContext);

		FGridPlayerAttackRequest RepeatRequest = Request;
		RepeatRequest.RequestId = FGuid::NewGuid();
		RepeatRequest.ResolutionIndex = ResolutionIndex;
		if (bDualWieldAction)
		{
			// One accepted 3-AP action, but separate real attack metadata.
			// Keep the parent action's AP cost for trace compatibility:
			// SpendPlayerCharacterActionPoints runs only once above.
			RepeatRequest.AttackId = RepeatProfile.AttackId;
			RepeatRequest.OffensiveItemDefinitionId = DualPair.Off.Item.ItemDefinitionId;
			RepeatRequest.OffensiveEquipmentSlot = EGridEquipmentSlot::OffHand;
			RepeatRequest.PresentationSource = OffPresentationSource;
			RepeatRequest.PreparedThrownItemActor = nullptr;
		}
		OutRequest = RepeatRequest;
		OutResult = RepeatResult;
		LastPlayerAttackRequest = RepeatRequest;
		LastPlayerAttackResult = RepeatResult;

		++PlayerAttackRequestedBroadcastCount;
		OnPlayerAttackRequested.Broadcast(RepeatRequest);

		FGridCombatLogEntry RepeatEntry;
		RepeatEntry.RoundNumber = RoundNumber;
		RepeatEntry.Phase = CurrentPhase;
		RepeatEntry.Type = RepeatResult.bHit ? EGridCombatLogEntryType::AttackHit : EGridCombatLogEntryType::AttackMiss;
		RepeatEntry.SourceId = FName(*RepeatRequest.AttackerCharacterId.ToString(EGuidFormats::Digits));
		RepeatEntry.SourceDisplayName = CharacterSummary.DisplayName;
		RepeatEntry.TargetId = FName(*TargetMonsterId.ToString(EGuidFormats::Digits));
		RepeatEntry.TargetDisplayName = ResolveMonsterDisplayName(TargetMonster);
		RepeatEntry.TargetCharacterIndex = INDEX_NONE;
		RepeatEntry.AttackId = RepeatRequest.AttackId;
		RepeatEntry.OffensiveItemDefinitionId = RepeatRequest.OffensiveItemDefinitionId;
		RepeatEntry.OffensiveEquipmentSlot = RepeatRequest.OffensiveEquipmentSlot;
		RepeatEntry.AttackResult = RepeatResult;
		RepeatEntry.bTargetDefeated = RepeatResult.TargetHealthBefore > 0 && RepeatResult.TargetHealthAfter <= 0;
		RepeatEntry.Message = FGridCombatLogFormatter::FormatPlayerAttack(
			RepeatEntry.SourceDisplayName, RepeatEntry.TargetDisplayName, RepeatEntry.AttackId, RepeatResult);
		AppendCombatLogEntry(RepeatEntry);

		bPlayerAttackResolutionInProgress = true;
		TargetMonster->ApplyAttackResult(RepeatResult);
		const TArray<FGridCombatStatusApplicationProfile>& RepeatStatusApplications =
			bDualWieldAction ? DualPair.Off.Action.StatusApplications : AttackStatusApplications;
		if (!RepeatStatusApplications.IsEmpty())
		{
			if (UWorld* StatusWorld = GetWorld())
			{
				if (UGridStatusEffectLifecycleSubsystem* StatusLifecycle = StatusWorld->GetSubsystem<UGridStatusEffectLifecycleSubsystem>())
				{
					StatusLifecycle->BindToTurnManager(this);
					StatusLifecycle->ApplyCombatStatusApplicationsToMonster(
						TargetMonster, RepeatStatusApplications, Attacker.CharacterId, RepeatTarget, &RepeatResult);
				}
			}
		}
		if (FGridCombatantInitiativeEntry* RepeatTargetEntry = FindInitiativeEntry(EGridCombatantSide::Monster, TargetMonsterId))
		{
			RefreshInitiativeEntryVitals(*RepeatTargetEntry);
			if (RepeatTargetEntry->State != EGridCombatantTurnState::Defeated)
			{
				OnCombatantStateChanged.Broadcast(*RepeatTargetEntry);
			}
		}
		EmitPlayerAttackReactionEvents(AttackerCharacterIndex, RepeatRequest, RepeatResult, ReactionSourcePolicy,
			ReactionActionType, ReactionActionInstanceId, false, false,
			bDualWieldAction ? DualPair.Off.SourceTags : ResolvedAttackSourceTags,
			bDualWieldAction || OffensiveEquipmentSlot != EGridEquipmentSlot::None);
		++PlayerAttackResolvedBroadcastCount;
		bPlayerAttackResolutionInProgress = false;
		OnPlayerAttackResolved.Broadcast(RepeatRequest, TargetMonster, RepeatResult);
		if (bCollectRuntimeMetrics)
		{
			++RuntimeMetrics.AttacksResolved;
		}
	}

	if (AttackResolutionCount > 1 && CombatActionOverride)
	{
		EmitCharacterActionResolvedReaction(AttackerCharacterIndex, *CombatActionOverride, ReactionActionInstanceId, ResolvedAttackSourceTags);
	}
	if ((ManaCost > 0 || SourceItemQuantityCost > 0 || bCooldownStarted) && IsValid(Inventory))
	{
		Inventory->NotifyPartyInventoryChanged(AttackerCharacterIndex);
	}

	SourceTransaction.Reset();
	if (bPendingVictoryAfterPlayerAttack)
	{
		bPendingVictoryAfterPlayerAttack = false;
		FinishCombat(EGridCombatPhase::Victory);
	}
	else if (!InitiativeOrder.IsEmpty())
	{
		FGridPlayerCharacterTurnState TurnState;
		if (GetPlayerCharacterTurnState(AttackerCharacterIndex, TurnState) && TurnState.RemainingActionPoints <= 0 &&
			IsActivePlayerCharacter(AttackerCharacterIndex))
		{
			// A monster must not start its next turn between two audible
			// impacts of a single accepted multi-resolution player action.
			if (bSequencedAttackFeedPending)
			{
				bFinishPlayerTurnAfterSequencedAttack = true;
			}
			else
			{
				FinishActivePlayerTurn();
			}
		}
	}
	return true;
}

bool UGridTurnManagerComponent::BuildPlayerAttackResolutionInputs(const FGridInventoryCharacterSummary& CharacterSummary,
	const AGridMonsterActor* TargetMonster, const FGridOffensiveEquipmentProfile& OffensiveProfile, FGridAttackSourceStats& OutSource,
	FGridAttackTargetStats& OutTarget, FGridAttackDefinition& OutAttackDefinition) const
{
	OutSource = FGridAttackSourceStats();
	OutTarget = FGridAttackTargetStats();
	OutAttackDefinition = FGridAttackDefinition();
	if (!IsValid(TargetMonster) || !IsValid(TargetMonster->MonsterDefinition) || !OffensiveProfile.IsValid())
	{
		return false;
	}

	OutSource.Accuracy = CharacterSummary.DerivedStats.Accuracy;
	OutSource.DamageBonus = OffensiveProfile.FlatDamageBonus;
	if (OffensiveProfile.DamageScalingAttribute != EGridAttackScalingAttribute::None)
	{
		OutSource.DamageBonus +=
			URPGCharacterRulesLibrary::GetAttributeModifier(GetScalingAttributeValue(CharacterSummary.Attributes, OffensiveProfile.DamageScalingAttribute));
	}

	OutAttackDefinition = OffensiveProfile.AttackDefinition;

	OutTarget.Evasion = TargetMonster->MonsterDefinition->Evasion;
	OutTarget.CurrentHealth = TargetMonster->CurrentHealth;
	OutTarget.PhysicalArmor = TargetMonster->CurrentPhysicalArmor;
	OutTarget.MagicalArmor = TargetMonster->CurrentMagicalArmor;
	OutTarget.ResistancePercent = 0;
	OutTarget.DamageMultiplier = TargetMonster->MonsterDefinition->GetDamageMultiplier(OutAttackDefinition.DamageType, OutAttackDefinition.PhysicalSubtype);
	return OutAttackDefinition.IsValid();
}

bool UGridTurnManagerComponent::ResolveCombatActionWeaponProfile(const UGridPartyInventoryComponent* PartyInventory, int32 AttackerCharacterIndex,
	const FGridCombatActionDefinition& ActionDefinition, FGridOffensiveEquipmentProfile& OutProfile, FName& OutItemDefinitionId,
	EGridEquipmentSlot& OutEquipmentSlot, TArray<FName>& OutItemTags, EGridPlayerAttackRejectReason& OutRejectReason) const
{
	OutProfile = FGridOffensiveEquipmentProfile();
	OutItemDefinitionId = NAME_None;
	OutEquipmentSlot = EGridEquipmentSlot::None;
	OutItemTags.Reset();
	OutRejectReason = EGridPlayerAttackRejectReason::None;
	if (!IsValid(PartyInventory) || !ActionDefinition.WeaponAttackProfile.bUseEquippedWeapon ||
		ActionDefinition.ResolutionProfile != EGridCombatActionResolutionProfile::Attack)
	{
		OutRejectReason = EGridPlayerAttackRejectReason::InvalidOffensiveEquipment;
		return false;
	}
	const UGridItemSubsystem* Canonical = nullptr;
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			Canonical = GI->GetSubsystem<UGridItemSubsystem>();
		}
	}

	const EGridEquipmentSlot HandSlots[] = { EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand };
	for (const EGridEquipmentSlot HandSlot : HandSlots)
	{
		FGridItemInstance EquippedItem;
		if (!PartyInventory->GetEquippedItem(AttackerCharacterIndex, HandSlot, EquippedItem))
		{
			continue;
		}

		if (Canonical && Canonical->ContainsItem(EquippedItem.ItemDefinitionId))
		{
			FGridResolvedItem Item;
			if (!Canonical->ResolveItem(EquippedItem.ItemDefinitionId, EquippedItem.ItemLevel, Item))
			{
				OutRejectReason = EGridPlayerAttackRejectReason::InvalidOffensiveEquipment;
				return false;
			}
			if (!Item.bIsArmament || !Item.Weapon.HasAttackSourceInHand(HandSlot) ||
				!ActionDefinition.WeaponAttackProfile.MatchesItemTags(Item.Tags))
			{
				continue;
			}
			if (!TryResolveOffensiveProfile(
					*Canonical, EquippedItem, HandSlot, OutProfile, Item) ||
				!ActionDefinition.WeaponAttackProfile.ApplyToOffensiveProfile(
					ActionDefinition.ActionId, ActionDefinition.RangeCells, OutProfile))
			{
				OutRejectReason = EGridPlayerAttackRejectReason::InvalidOffensiveEquipment;
				return false;
			}
			OutItemDefinitionId = EquippedItem.ItemDefinitionId;
			OutEquipmentSlot = HandSlot;
			OutItemTags = Item.Tags;
			return true;
		}

		OutRejectReason = EGridPlayerAttackRejectReason::EquippedItemDefinitionUnavailable;
		return false;
	}

	if (ActionDefinition.WeaponAttackProfile.bAllowUnarmed)
	{
		OutProfile = MakeUnarmedOffensiveProfile();
		if (ActionDefinition.WeaponAttackProfile.ApplyToOffensiveProfile(ActionDefinition.ActionId, ActionDefinition.RangeCells, OutProfile))
		{
			return true;
		}
	}

	OutRejectReason = EGridPlayerAttackRejectReason::InvalidOffensiveEquipment;
	return false;
}

bool UGridTurnManagerComponent::ResolvePlayerOffensiveProfile(const UGridPartyInventoryComponent* PartyInventory, int32 AttackerCharacterIndex,
	EGridEquipmentSlot RequestedEquipmentSlot, bool bRequireRequestedEquipmentSlot, FGridOffensiveEquipmentProfile& OutProfile, FName& OutItemDefinitionId,
	EGridEquipmentSlot& OutEquipmentSlot, EGridPlayerAttackRejectReason& OutRejectReason) const
{
	OutProfile = FGridOffensiveEquipmentProfile();
	OutItemDefinitionId = NAME_None;
	OutEquipmentSlot = EGridEquipmentSlot::None;
	OutRejectReason = EGridPlayerAttackRejectReason::None;
	if (!IsValid(PartyInventory))
	{
		OutRejectReason = EGridPlayerAttackRejectReason::PartyUnavailable;
		return false;
	}
	const UGridItemSubsystem* Canonical = nullptr;
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			Canonical = GI->GetSubsystem<UGridItemSubsystem>();
		}
	}

	if (bRequireRequestedEquipmentSlot)
	{
		if (RequestedEquipmentSlot != EGridEquipmentSlot::MainHand && RequestedEquipmentSlot != EGridEquipmentSlot::OffHand)
		{
			OutRejectReason = EGridPlayerAttackRejectReason::InvalidOffensiveEquipment;
			return false;
		}

		FGridItemInstance EquippedItem;
		if (!PartyInventory->GetEquippedItem(AttackerCharacterIndex, RequestedEquipmentSlot, EquippedItem))
		{
			OutProfile = MakeUnarmedOffensiveProfile();
			return true;
		}

		if (Canonical && Canonical->ContainsItem(EquippedItem.ItemDefinitionId))
		{
			FGridResolvedItem Item;
			if (!TryResolveOffensiveProfile(
					*Canonical, EquippedItem, RequestedEquipmentSlot, OutProfile, Item))
			{
				OutRejectReason = EGridPlayerAttackRejectReason::InvalidOffensiveEquipment;
				return false;
			}
			OutItemDefinitionId = EquippedItem.ItemDefinitionId;
			OutEquipmentSlot = RequestedEquipmentSlot;
			return true;
		}

		OutRejectReason = EGridPlayerAttackRejectReason::EquippedItemDefinitionUnavailable;
		return false;
	}

	const EGridEquipmentSlot HandSlots[] = { EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand };
	for (const EGridEquipmentSlot HandSlot : HandSlots)
	{
		FGridItemInstance EquippedItem;
		if (!PartyInventory->GetEquippedItem(AttackerCharacterIndex, HandSlot, EquippedItem))
		{
			continue;
		}

		if (Canonical && Canonical->ContainsItem(EquippedItem.ItemDefinitionId))
		{
			FGridResolvedItem Item;
			if (!Canonical->ResolveItem(EquippedItem.ItemDefinitionId, EquippedItem.ItemLevel, Item))
			{
				OutRejectReason = EGridPlayerAttackRejectReason::InvalidOffensiveEquipment;
				return false;
			}
			if (!Item.bIsArmament || !Item.Weapon.HasAttackSourceInHand(HandSlot))
			{
				continue;
			}
			if (!TryResolveOffensiveProfile(
					*Canonical, EquippedItem, HandSlot, OutProfile, Item))
			{
				OutRejectReason = EGridPlayerAttackRejectReason::InvalidOffensiveEquipment;
				return false;
			}
			OutItemDefinitionId = EquippedItem.ItemDefinitionId;
			OutEquipmentSlot = HandSlot;
			return true;
		}

		OutRejectReason = EGridPlayerAttackRejectReason::EquippedItemDefinitionUnavailable;
		return false;
	}

	OutProfile = MakeUnarmedOffensiveProfile();
	return true;
}

bool UGridTurnManagerComponent::RejectPlayerAttack(
	int32 AttackerCharacterIndex, EGridPlayerAttackRejectReason RejectReason, EGridPlayerAttackRejectReason& OutRejectReason)
{
	OutRejectReason = RejectReason;
	LastPlayerAttackRejectReason = RejectReason;
	UE_LOG(LogGridTurnManager, Log, TEXT("[GridPlayerAttack] Accepted=false Reason=%s Round=%d Phase=%s Attacker=%d"),
		*GetPlayerAttackRejectReasonText(RejectReason), RoundNumber, *UEnum::GetValueAsString(CurrentPhase), AttackerCharacterIndex);
	++PlayerAttackRejectedBroadcastCount;
	OnPlayerAttackRejected.Broadcast(AttackerCharacterIndex, RejectReason);
	return false;
}

bool UGridTurnManagerComponent::IsCombatMonster(const AGridMonsterActor* Monster) const
{
	return IsValid(Monster) &&
		CombatMonsters.ContainsByPredicate(
			[Monster](const TObjectPtr<AGridMonsterActor>& Candidate)
			{
				return Candidate.Get() == Monster;
			});
}
