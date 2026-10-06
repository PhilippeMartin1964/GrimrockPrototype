#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellbookUI.h"
#include "Magic/GridSpellHotbarExecution.h"
#include "Magic/GridSpellPresentationComponent.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGSkillRequirementProjectionService.h"
#include "RPG/RPGSkillCheckService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/StatusEffects/GridStatusEffectControlResolver.h"
#include "RPG/StatusEffects/GridCombatStatusApplicationResolver.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatArmorEffectResolver.h"
#include "Runtime/Combat/GridQuickItemResolver.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatMovementResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridCombatTargetingResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"
#include "UObject/UObjectIterator.h"

namespace
{
	void AddMON126Contribution(const FGridCombatActionDefinition& Definition, FName SourceDefinitionId, const FGuid& SourceRuntimeId,
		EGridEquipmentSlot SourceEquipmentSlot, int32 AvailableSourceQuantity, TArray<FGridCombatActionContribution>& OutContributions)
	{
		FGridCombatActionContribution Contribution;
		Contribution.Definition = Definition;
		Contribution.SourceDefinitionId = SourceDefinitionId;
		Contribution.SourceRuntimeId = SourceRuntimeId;
		Contribution.SourceEquipmentSlot = SourceEquipmentSlot;
		Contribution.AvailableSourceQuantity = FMath::Max(0, AvailableSourceQuantity);
		if (Contribution.IsValid())
		{
			OutContributions.Add(MoveTemp(Contribution));
		}
	}

	FString GetMON126AvailabilityText(const FGridAvailableCombatAction& Action)
	{
		return Action.bEnabled ? TEXT("Enabled") : UEnum::GetValueAsString(Action.AvailabilityReason);
	}

	bool IsMON1285ClassActionSource(EGridCombatActionSourcePolicy SourcePolicy)
	{
		return SourcePolicy == EGridCombatActionSourcePolicy::Ability || SourcePolicy == EGridCombatActionSourcePolicy::Spell;
	}

	bool ResolveEquippedAttackProfile(const UGridItemDefinitionAsset* Definition, FGridOffensiveEquipmentProfile& OutProfile)
	{
		OutProfile = FGridOffensiveEquipmentProfile();
		if (!IsValid(Definition))
		{
			return false;
		}

		for (const FGridCombatActionDefinition& Action : Definition->CombatActions)
		{
			if (Action.IsValid() && Action.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
				Action.ResolutionProfile == EGridCombatActionResolutionProfile::Attack)
			{
				OutProfile = Action.OffensiveProfile;
				return true;
			}
		}
		return false;
	}

	int32 ResolveRPG038AttributeValue(const FRPGAttributes& Attributes, EGridAttackScalingAttribute Attribute)
	{
		switch (Attribute)
		{
			case EGridAttackScalingAttribute::Strength: return Attributes.Strength;
			case EGridAttackScalingAttribute::Dexterity: return Attributes.Dexterity;
			case EGridAttackScalingAttribute::Constitution: return Attributes.Constitution;
			case EGridAttackScalingAttribute::Intelligence: return Attributes.Intelligence;
			case EGridAttackScalingAttribute::Wisdom: return Attributes.Wisdom;
			case EGridAttackScalingAttribute::Charisma: return Attributes.Charisma;
			default: return 10;
		}
	}

	bool IsMON1286ExplicitTargetingPolicy(EGridCombatTargetingPolicy TargetingPolicy)
	{
		return TargetingPolicy == EGridCombatTargetingPolicy::Cell || TargetingPolicy == EGridCombatTargetingPolicy::Area;
	}

	bool IsMON1286TargetedSource(EGridCombatActionSourcePolicy SourcePolicy)
	{
		return SourcePolicy == EGridCombatActionSourcePolicy::Universal || SourcePolicy == EGridCombatActionSourcePolicy::Ability ||
			SourcePolicy == EGridCombatActionSourcePolicy::Spell || SourcePolicy == EGridCombatActionSourcePolicy::QuickItem;
	}

	FText MakeMON1286TargetingReason(const TCHAR* Reason)
	{
		return FText::FromString(Reason ? Reason : TEXT("Cible invalide."));
	}

	bool MatchesRPG0391MonsterTargetFilter(
		const FGridCombatTargetFilterProfile& Filter, const AGridMonsterActor* Monster, const FGuid& ActingSourceId)
	{
		if (!IsValid(Monster))
		{
			return false;
		}
		const FName CategoryId = IsValid(Monster->MonsterDefinition) ? Monster->MonsterDefinition->CategoryId : NAME_None;
		FGridAttackTargetStats TargetStats;
		TargetStats.CurrentHealth = Monster->CurrentHealth;
		TargetStats.PhysicalArmor = Monster->CurrentPhysicalArmor;
		TargetStats.MagicalArmor = Monster->CurrentMagicalArmor;
		const int32 MaximumHealth =
			IsValid(Monster->MonsterDefinition) ? FMath::Max(1, Monster->MonsterDefinition->MaxHealth) : FMath::Max(1, Monster->CurrentHealth);
		return FGridCombatTargetingResolver::MatchesTargetFilter(
			Filter, CategoryId, Monster->StatusEffects, ActingSourceId, &TargetStats, MaximumHealth);
	}

	bool TryBuildUI0143e2ProductionSpell(FName SpellId, FGridSpellDefinition& OutDefinition)
	{
		OutDefinition = FGridSpellDefinition();
		if (SpellId.IsNone())
		{
			return false;
		}

		TArray<FGridSpellDefinition> Definitions;
		FGridProductionSpellLibrary::BuildAll(Definitions);
		const FGridSpellDefinition* Found = Definitions.FindByPredicate(
			[SpellId](const FGridSpellDefinition& Candidate)
			{
				return Candidate.SpellId == SpellId;
			});
		if (!Found || FGridSpellContract::ValidateDefinition(*Found) != EGridSpellValidationError::None)
		{
			return false;
		}
		OutDefinition = *Found;
		return true;
	}

	bool BuildRPG033PartyArmorSnapshot(
		const UGridPartyInventoryComponent* Inventory, int32 CharacterIndex, FGridCombatArmorPoolSnapshot& OutSnapshot)
	{
		OutSnapshot = FGridCombatArmorPoolSnapshot();
		if (!IsValid(Inventory) || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
		{
			return false;
		}

		const FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
		const URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
		if (!FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(Character.ClassId, ClassDefinition))
		{
			ClassDefinition = FRPGAuthoringIdentityResolver::ResolveClassById(Character.ClassId);
		}
		if (!IsValid(ClassDefinition) || !ClassDefinition->IsValidDefinition())
		{
			return false;
		}

		FGridInventoryCharacterSummary Summary;
		if (!Inventory->GetCharacterSummary(CharacterIndex, Summary))
		{
			return false;
		}

		OutSnapshot.CurrentPhysicalArmor = FMath::Max(0, Summary.Resources.CurrentPhysicalArmor);
		OutSnapshot.CurrentMagicalArmor = FMath::Max(0, Summary.Resources.CurrentMagicalArmor);
		OutSnapshot.ReferencePhysicalArmor =
			FMath::Max(0, ClassDefinition->BasePhysicalArmor + Summary.EquipmentStatBonus.ArmorBonus);
		OutSnapshot.ReferenceMagicalArmor = FMath::Max(0, ClassDefinition->BaseMagicalArmor);
		return true;
	}

	bool ResolveRPG033CharacterArmorModifiers(const FGridCharacterInventoryState& Character, const FGridAvailableCombatAction& Action,
		FGridResolvedCombatModifiers& OutModifiers)
	{
		TArray<FGridCombatModifierProfile> Profiles;
		if (!FGridCombatModifierResolver::CollectCharacterModifiers(Character, Profiles))
		{
			OutModifiers.Reset();
			return false;
		}
		FGridCombatModifierResolver::Resolve(
			Profiles, FGridCombatModifierResolver::MakeActionContext(Action.Definition, Action.SourceDefinitionId), OutModifiers);
		return true;
	}
}

void UGridTurnManagerComponent::BuildPlayerCombatActionContributions(int32 CharacterIndex, TArray<FGridCombatActionContribution>& OutContributions) const
{
	OutContributions.Reset();
	const UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	if (!IsValid(Inventory) || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return;
	}

	const FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
	const URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
	if (!FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(Character.ClassId, ClassDefinition))
	{
		ClassDefinition = FRPGAuthoringIdentityResolver::ResolveClassById(Character.ClassId);
	}
	if (ClassDefinition)
	{
		for (const FGridCombatActionDefinition& SourceDefinition : ClassDefinition->CombatActions)
		{
			FGridCombatActionDefinition Definition = SourceDefinition;
			if (Definition.WeaponAttackProfile.bUseEquippedWeapon)
			{
				FGridOffensiveEquipmentProfile ResolvedWeapon;
				FName ResolvedItemDefinitionId = NAME_None;
				EGridEquipmentSlot ResolvedSlot = EGridEquipmentSlot::None;
				TArray<FName> ResolvedItemTags;
				EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
				if (ResolveCombatActionWeaponProfile(
						Inventory, CharacterIndex, Definition, ResolvedWeapon, ResolvedItemDefinitionId, ResolvedSlot, ResolvedItemTags, RejectReason))
				{
					Definition.RangeCells = ResolvedWeapon.RangeCells;
					for (const FName Tag : ResolvedItemTags)
					{
						Definition.SourceTags.AddUnique(Tag);
					}
				}
			}
			AddMON126Contribution(Definition, Definition.SourcePolicy == EGridCombatActionSourcePolicy::Universal ? NAME_None : ClassDefinition->ClassId,
				FGuid(), EGridEquipmentSlot::None, 1, OutContributions);
		}
	}

	// TD07.3.3.7: durable character KnownSpellIds is the sole authority.
	// The hotbar stores identity only; the combat catalogue reconstructs
	// the action from canonical SpellId every time it is queried.
	if (IsValid(PartyPawn))
	{
		{
			TArray<FGridSpellDefinition> ProductionSpells;
			FGridProductionSpellLibrary::BuildAll(ProductionSpells);
			for (const FName SpellId : Character.KnownSpellIds)
			{
				const FGridSpellDefinition* SpellDefinition = ProductionSpells.FindByPredicate(
					[SpellId](const FGridSpellDefinition& Candidate)
					{
						return Candidate.SpellId == SpellId;
					});
				if (!SpellDefinition || FGridSpellContract::ValidateDefinition(*SpellDefinition) != EGridSpellValidationError::None)
				{
					continue;
				}

				const FGridCombatActionDefinition SpellAction = UGridSpellbookUILibrary::MakeSpellCombatActionDefinition(*SpellDefinition);
				const bool bAlreadyContributed = OutContributions.ContainsByPredicate(
					[SpellId](const FGridCombatActionContribution& Existing)
					{
						return Existing.Definition.SourcePolicy == EGridCombatActionSourcePolicy::Spell && Existing.Definition.ActionId == SpellId &&
							Existing.SourceDefinitionId == SpellId;
					});
				if (!bAlreadyContributed)
				{
					AddMON126Contribution(SpellAction, SpellId, FGuid(), EGridEquipmentSlot::None, 1, OutContributions);
				}
			}
		}
	}

	const EGridEquipmentSlot HandSlots[] = { EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand };
	for (const EGridEquipmentSlot HandSlot : HandSlots)
	{
		FGridItemInstance EquippedItem;
		if (!Inventory->GetEquippedItem(CharacterIndex, HandSlot, EquippedItem))
		{
			continue;
		}

		const UGridItemDefinitionAsset* ItemDefinition = Inventory->FindItemDefinition(EquippedItem.ItemDefinitionId);
		if (!IsValid(ItemDefinition))
		{
			continue;
		}
		if (!ItemDefinition->IsValidDefinition() || !ItemDefinition->CanEquipToSlot(HandSlot))
		{
			continue;
		}

		for (const FGridCombatActionDefinition& SourceAction : ItemDefinition->CombatActions)
		{
			FGridCombatActionDefinition Definition = SourceAction;
			for (const FName ItemTag : ItemDefinition->ItemTags)
			{
				if (!ItemTag.IsNone())
				{
					Definition.SourceTags.AddUnique(ItemTag);
				}
			}
			if (Definition.DisplayName.IsEmpty())
			{
				Definition.DisplayName = ItemDefinition->DisplayName;
			}
			if (Definition.Description.IsEmpty())
			{
				Definition.Description = ItemDefinition->Description;
			}
			if (Definition.Icon.IsNull())
			{
				Definition.Icon = ItemDefinition->Icon;
			}
			if (ItemDefinition->IsCombatThrowable() && Definition.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
				Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack)
			{
				Definition.ResourceCosts.SourceItemQuantityCost = 1;
			}
			AddMON126Contribution(Definition, EquippedItem.ItemDefinitionId, EquippedItem.RuntimeObjectId, HandSlot, EquippedItem.Quantity, OutContributions);
		}
	}

	// Unarmed is a manual universal action, not merely an automatic fallback.
	// Keep it in the catalogue even while a valid weapon is equipped so the
	// player can always drag it to the personal hotbar.
	FGridCombatActionDefinition UnarmedAction = FGridCombatActionCatalog::MakeUnarmedAttackDefinition(PlayerAttackActionPointCost);
	UnarmedAction.Icon = UnarmedAttackIcon;
	AddMON126Contribution(UnarmedAction, NAME_None, FGuid(), EGridEquipmentSlot::None, 0, OutContributions);

	// A quick-item contribution is definition-based rather than stack-based,
	// but it only exists while at least one source item remains in inventory.
	TArray<FName> QuickItemDefinitionIds;
	for (const FGridInventorySlot& InventorySlot : Character.InventorySlots)
	{
		if (!InventorySlot.IsEmpty())
		{
			QuickItemDefinitionIds.AddUnique(InventorySlot.Item.ItemDefinitionId);
		}
	}
	QuickItemDefinitionIds.Sort(
		[](const FName Left, const FName Right)
		{
			return Left.ToString() < Right.ToString();
		});

	for (const FName ItemDefinitionId : QuickItemDefinitionIds)
	{
		const UGridItemDefinitionAsset* ItemDefinition = Inventory->FindItemDefinition(ItemDefinitionId);
		FGridCombatActionDefinition QuickItemAction;
		if (!IsValid(ItemDefinition) || !ItemDefinition->BuildInventoryCombatActionDefinition(QuickItemAction))
		{
			continue;
		}

		AddMON126Contribution(QuickItemAction, ItemDefinitionId, FGuid(), EGridEquipmentSlot::None,
			Inventory->CountItemDefinitionInCharacterInventory(CharacterIndex, ItemDefinitionId), OutContributions);
	}
}

void UGridTurnManagerComponent::GetAvailableCombatActions(int32 CharacterIndex, TArray<FGridAvailableCombatAction>& OutActions) const
{
	OutActions.Reset();
	const UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	if (!IsValid(Inventory) || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return;
	}

	const FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
	FGridPlayerCharacterTurnState TurnState;
	const bool bHasTurnState = GetPlayerCharacterTurnState(CharacterIndex, TurnState);

	FGridCombatActionCatalogContext Context;
	Context.CharacterIndex = CharacterIndex;
	Context.CharacterId = Character.CharacterId;
	Context.bCombatActive = bCombatActive;
	Context.bCharacterDefeated = Character.Resources.CurrentHealth <= 0;
	Context.bActiveCombatant = bHasTurnState && TurnState.State == EGridCombatantTurnState::Active && CurrentPhase == EGridCombatPhase::PlayerPhase;
	Context.bPartyBusy = !IsPartyAtRest() || bPlayerAttackResolutionInProgress || IsPartyMotionInProgress();
	Context.RemainingActionPoints = bHasTurnState ? FMath::Max(0, TurnState.RemainingActionPoints) : 0;
	Context.RemainingMobilityActionPoints = FMath::Max(0, PartyMobilityState.RemainingMobilityActionPoints);
	FGridInventoryCharacterSummary CharacterSummary;
	if (Inventory->GetCharacterSummary(CharacterIndex, CharacterSummary))
	{
		Context.CurrentHealth = FMath::Max(0, CharacterSummary.Resources.CurrentHealth);
		Context.MaximumHealth = FMath::Max(0, CharacterSummary.DerivedStats.MaxHealth);
		Context.CurrentMana = FMath::Max(0, CharacterSummary.Resources.CurrentMana);
		Context.MaximumMana = FMath::Max(0, CharacterSummary.DerivedStats.MaxMana);
		Context.CurrentPhysicalArmor = FMath::Max(0, CharacterSummary.Resources.CurrentPhysicalArmor);
		Context.CurrentMagicalArmor = FMath::Max(0, CharacterSummary.Resources.CurrentMagicalArmor);
		FGridCombatArmorPoolSnapshot ArmorSnapshot;
		if (BuildRPG033PartyArmorSnapshot(Inventory, CharacterIndex, ArmorSnapshot))
		{
			Context.ReferencePhysicalArmor = ArmorSnapshot.ReferencePhysicalArmor;
			Context.ReferenceMagicalArmor = ArmorSnapshot.ReferenceMagicalArmor;
		}
		Context.CurrentStatusEffects = Character.StatusEffects;
		Context.ArmorEffectSource = FGridCombatArmorEffectResolver::MakeSourceContext(Character, CharacterSummary.Attributes);
	}
	Context.bEnableQuickItemExecutors = true;
	Context.bEnableClassActionExecutors = true;
	if (!Character.ClassId.IsNone())
	{
		Context.SatisfiedRequirements.Add(Character.ClassId);
	}
	Context.SkillRanks = Character.SkillRanks;
	if (!FGridCombatModifierResolver::CollectCharacterModifiers(Character, Context.CombatModifiers))
	{
		UE_LOG(LogGridTurnManager, Warning, TEXT("[RPG03.1] CombatModifierProjectionFailed Character=%d CharacterId=%s ClassId=%s"), CharacterIndex,
			*Character.CharacterId.ToString(EGuidFormats::Digits), *Character.ClassId.ToString());
		Context.CombatModifiers.Reset();
	}
	for (const TPair<FGridCombatActionCooldownKey, int32>& Cooldown : CombatActionCooldownAvailableRounds)
	{
		if (Cooldown.Key.CharacterId != Character.CharacterId)
		{
			continue;
		}

		const int32 RemainingRounds = GetRemainingCombatActionCooldown(Character.CharacterId, Cooldown.Key.ActionId);
		if (RemainingRounds > 0)
		{
			Context.RemainingCooldownRounds.Add(Cooldown.Key.ActionId, RemainingRounds);
		}
	}

	if (Inventory->PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		const FGridCharacterEquipmentState& Equipment = Inventory->PartyInventoryState.ActiveEquipment[CharacterIndex];
		const EGridEquipmentSlot HandSlots[] = { EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand };
		for (const EGridEquipmentSlot HandSlot : HandSlots)
		{
			const FGridItemInstance* Item = Equipment.GetSlot(HandSlot);
			const UGridItemDefinitionAsset* Definition = Item && Item->IsValid() ? Inventory->FindItemDefinition(Item->ItemDefinitionId) : nullptr;
			if (!IsValid(Definition))
			{
				continue;
			}
			for (const FName ItemTag : Definition->ItemTags)
			{
				Context.SatisfiedRequirements.Add(ItemTag);
			}
			if (Definition->CanProvideAttackFromSlot(HandSlot))
			{
				Context.EquippedOffensiveSourceTagSets.Add(Definition->ItemTags);
				FGridOffensiveEquipmentProfile EquippedProfile;
				if (ResolveEquippedAttackProfile(Definition, EquippedProfile) && EquippedProfile.IsValid())
				{
					Context.EquippedOffensivePhysicalSubtypes.Add(
						EquippedProfile.AttackDefinition.DamageType == EGridDamageType::Physical
							? EquippedProfile.AttackDefinition.PhysicalSubtype
							: EGridPhysicalDamageSubtype::None);
					Context.EquippedOffensiveRangeCells.Add(EquippedProfile.RangeCells);
				}
				else
				{
					Context.EquippedOffensivePhysicalSubtypes.Add(EGridPhysicalDamageSubtype::None);
					Context.EquippedOffensiveRangeCells.Add(0);
				}
			}
		}
	}

	FString SkillRequirementError;
	if (!FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements(Character, Context.SatisfiedRequirements, SkillRequirementError))
	{
		UE_LOG(LogGridTurnManager, Warning, TEXT("[GridActionCatalog] SkillRequirementProjectionFailed Character=%d CharacterId=%s Error=%s"), CharacterIndex,
			*Character.CharacterId.ToString(EGuidFormats::Digits), *SkillRequirementError);
	}

	TArray<FGridCombatActionContribution> Contributions;
	BuildPlayerCombatActionContributions(CharacterIndex, Contributions);
	FGridCombatActionCatalog::Build(Context, Contributions, OutActions);

	const bool bSpellActionsBlocked = FGridStatusEffectControlResolver::Resolve(Character.StatusEffects).bBlockSpellActions;
	for (FGridAvailableCombatAction& Action : OutActions)
	{
		if (bSpellActionsBlocked && Action.bEnabled && Action.Definition.SourcePolicy == EGridCombatActionSourcePolicy::Spell)
		{
			Action.bEnabled = false;
			Action.AvailabilityReason = EGridCombatActionAvailabilityReason::MissingRequirement;
			Action.DisabledReason = FText::FromString(TEXT("Un effet de statut empêche l'utilisation des sorts."));
		}
		ResolveSuggestedCombatActionTarget(Action);
	}
}

void UGridTurnManagerComponent::ResolveSuggestedCombatActionTarget(FGridAvailableCombatAction& Action) const
{
	Action.SuggestedTargetId = FGuid();
	Action.SuggestedTargetCell = FIntPoint::ZeroValue;
	if (!IsValid(RuntimeActor) || !IsValid(PartyPawn) ||
		(Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::FirstAxialTarget &&
			Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::Hostile) ||
		Action.Definition.RangeCells <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	UGridMonsterOccupancySubsystem* Occupancy = World ? World->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr;
	if (!IsValid(Occupancy))
	{
		return;
	}

	FIntPoint SearchCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
	for (int32 Distance = 1; Distance <= Action.Definition.RangeCells; ++Distance)
	{
		int32 NextCellX = INDEX_NONE;
		int32 NextCellY = INDEX_NONE;
		if (!RuntimeActor->TryGetNeighborCell(SearchCell.X, SearchCell.Y, PartyPawn->Facing, NextCellX, NextCellY) ||
			!RuntimeActor->CanMove(SearchCell.X, SearchCell.Y, PartyPawn->Facing))
		{
			return;
		}
		SearchCell = FIntPoint(NextCellX, NextCellY);
		AGridMonsterActor* Monster = Occupancy->GetOccupantAtCell(SearchCell);
		if (!IsValid(Monster))
		{
			continue;
		}
		if (IsCombatMonster(Monster) && Monster->bMonsterEnabled && !Monster->IsDead() &&
			FGridCombatTargetingResolver::IsDirectHostileTargetable(Monster->StatusEffects))
		{
			if (MatchesRPG0391MonsterTargetFilter(Action.Definition.TargetFilter, Monster, Action.CharacterId))
			{
				Action.SuggestedTargetId = Monster->ResolvePersistenceId();
				Action.SuggestedTargetCell = SearchCell;
			}
		}
		return;
	}
}

int32 UGridTurnManagerComponent::GetRemainingCombatActionCooldown(const FGuid& CharacterId, FName ActionId) const
{
	if (!CharacterId.IsValid() || ActionId.IsNone())
	{
		return 0;
	}

	FGridCombatActionCooldownKey Key;
	Key.CharacterId = CharacterId;
	Key.ActionId = ActionId;
	const int32* AvailableRound = CombatActionCooldownAvailableRounds.Find(Key);
	return AvailableRound ? FMath::Max(0, *AvailableRound - RoundNumber) : 0;
}

void UGridTurnManagerComponent::StartCombatActionCooldown(const FGridAvailableCombatAction& Action)
{
	const int32 CooldownRounds = FMath::Max(0, Action.Definition.CooldownRounds);
	if (CooldownRounds <= 0 || !Action.CharacterId.IsValid() || Action.Definition.ActionId.IsNone())
	{
		return;
	}

	FGridCombatActionCooldownKey Key;
	Key.CharacterId = Action.CharacterId;
	Key.ActionId = Action.Definition.ActionId;
	CombatActionCooldownAvailableRounds.Add(Key, RoundNumber + CooldownRounds + 1);
}

void UGridTurnManagerComponent::ResetCombatActionCooldowns()
{
	CombatActionCooldownAvailableRounds.Reset();
}

bool UGridTurnManagerComponent::RequestCharacterQuickItemEffect(const FGridAvailableCombatAction& Action, FGridCombatQuickItemResult& OutResult)
{
	OutResult = FGridCombatQuickItemResult();
	UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	if (!IsValid(Inventory) || !Action.bEnabled || Action.Definition.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem ||
		Action.Definition.ResolutionProfile != EGridCombatActionResolutionProfile::Effect ||
		Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::Self ||
		(!Action.Definition.EffectProfile.IsValid() && Action.Definition.QuickItemScaling.RestoreHealthSkillRankScale <= 0 &&
			Action.Definition.QuickItemScaling.RestoreManaSkillRankScale <= 0 && Action.Definition.StatusApplications.IsEmpty() &&
			Action.Definition.StatusRemovals.IsEmpty() && Action.Definition.ArmorEffects.IsEmpty()) ||
		Action.SourceDefinitionId.IsNone() || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Action.CharacterIndex))
	{
		return false;
	}

	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex];
	FGridInventoryCharacterSummary Summary;
	FGridPlayerCharacterTurnState TurnStateBefore;
	if (!Inventory->GetCharacterSummary(Action.CharacterIndex, Summary) || !GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateBefore))
	{
		return false;
	}

	OutResult.SourceQuantityBefore = Inventory->CountItemDefinitionInCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId);
	OutResult.HealthBefore = Character.Resources.CurrentHealth;
	OutResult.ManaBefore = Character.Resources.CurrentMana;
	const int32 QuantityCost = Action.CurrentSourceItemQuantityCost;
	const int32 ManaCost = Action.CurrentManaCost;
	if (QuantityCost <= 0 || OutResult.SourceQuantityBefore < QuantityCost || OutResult.ManaBefore < ManaCost ||
		!CanCharacterSpendActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost))
	{
		return false;
	}

	FGridResolvedCombatModifiers QuickItemModifiers;
	ResolveRPG033CharacterArmorModifiers(Character, Action, QuickItemModifiers);
	FGridCombatActionEffectProfile EffectiveEffect;
	FGridQuickItemResolver::ResolveEffectProfile(Action.Definition, Character.SkillRanks, QuickItemModifiers, EffectiveEffect);
	OutResult.HealthAfter =
		FMath::Clamp(OutResult.HealthBefore + EffectiveEffect.RestoreHealth, 0, FMath::Max(0, Summary.DerivedStats.MaxHealth));
	OutResult.ManaAfter =
		FMath::Clamp(OutResult.ManaBefore - ManaCost + EffectiveEffect.RestoreMana, 0, FMath::Max(0, Summary.DerivedStats.MaxMana));
	FGridCombatArmorPoolSnapshot ArmorSnapshot;
	const FGridCombatArmorEffectSourceContext ArmorSourceContext =
		FGridCombatArmorEffectResolver::MakeSourceContext(Character, Summary.Attributes);
	const bool bHasArmorSnapshot = BuildRPG033PartyArmorSnapshot(Inventory, Action.CharacterIndex, ArmorSnapshot);
	if (bHasArmorSnapshot)
	{
		FGridCombatArmorEffectResolver::ApplyReferenceModifiers(ArmorSnapshot, QuickItemModifiers);
		OutResult.PhysicalArmorBefore = ArmorSnapshot.CurrentPhysicalArmor;
		OutResult.PhysicalArmorAfter = ArmorSnapshot.CurrentPhysicalArmor;
		OutResult.MagicalArmorBefore = ArmorSnapshot.CurrentMagicalArmor;
		OutResult.MagicalArmorAfter = ArmorSnapshot.CurrentMagicalArmor;
	}
	FGridAttackTargetStats SelfTargetBefore;
	SelfTargetBefore.CurrentHealth = OutResult.HealthBefore;
	SelfTargetBefore.PhysicalArmor = bHasArmorSnapshot ? ArmorSnapshot.CurrentPhysicalArmor : Character.Resources.CurrentPhysicalArmor;
	SelfTargetBefore.MagicalArmor = bHasArmorSnapshot ? ArmorSnapshot.CurrentMagicalArmor : Character.Resources.CurrentMagicalArmor;
	const bool bStatusWouldMutate = FGridCombatStatusApplicationResolver::WouldAnyMutate(
		Action.Definition.StatusApplications, Character.CharacterId, SelfTargetBefore, nullptr, Character.StatusEffects);
	TArray<FName> StatusRemovalIds;
	FGridCombatTargetingResolver::CollectStatusRemovalIds(Character.StatusEffects, Action.Definition.StatusRemovals, StatusRemovalIds);
	const bool bStatusRemovalWouldMutate = !StatusRemovalIds.IsEmpty();
	const bool bArmorWouldMutate =
		bHasArmorSnapshot && FGridCombatArmorEffectResolver::WouldAnyRestore(Action.Definition.ArmorEffects, ArmorSnapshot, QuickItemModifiers, &ArmorSourceContext);
	if (OutResult.HealthAfter <= OutResult.HealthBefore && OutResult.ManaAfter <= OutResult.ManaBefore && !bStatusWouldMutate &&
		!bStatusRemovalWouldMutate && !bArmorWouldMutate)
	{
		return false;
	}

	if (!SpendPlayerCharacterActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost))
	{
		return false;
	}

	Character.Resources.CurrentHealth = OutResult.HealthAfter;
	Character.Resources.CurrentMana = OutResult.ManaAfter;
	if (!Inventory->RemoveItemDefinitionFromCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId, QuantityCost))
	{
		Character.Resources.CurrentHealth = OutResult.HealthBefore;
		Character.Resources.CurrentMana = OutResult.ManaBefore;
		OutResult.HealthAfter = OutResult.HealthBefore;
		OutResult.ManaAfter = OutResult.ManaBefore;
		OutResult.SourceQuantityAfter = OutResult.SourceQuantityBefore;
		if (FGridPlayerCharacterTurnState* RestoredTurnState = EnsurePlayerCharacterTurnState(Action.CharacterIndex))
		{
			*RestoredTurnState = TurnStateBefore;
			BroadcastPlayerCharacterTurnState(*RestoredTurnState);
		}
		return false;
	}

	OutResult.SourceQuantityAfter = Inventory->CountItemDefinitionInCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId);
	if (bHasArmorSnapshot && !Action.Definition.ArmorEffects.IsEmpty())
	{
		const int32 PhysicalBefore = ArmorSnapshot.CurrentPhysicalArmor;
		const int32 MagicalBefore = ArmorSnapshot.CurrentMagicalArmor;
		FGridCombatArmorEffectResolver::ApplyRestoreEffects(Action.Definition.ArmorEffects, ArmorSnapshot, QuickItemModifiers, &ArmorSourceContext);
		const int32 PhysicalRestored = FMath::Max(0, ArmorSnapshot.CurrentPhysicalArmor - PhysicalBefore);
		const int32 MagicalRestored = FMath::Max(0, ArmorSnapshot.CurrentMagicalArmor - MagicalBefore);
		Character.Resources.CurrentPhysicalArmor = FMath::Max(0, Character.Resources.CurrentPhysicalArmor + PhysicalRestored);
		Character.Resources.CurrentMagicalArmor = FMath::Max(0, Character.Resources.CurrentMagicalArmor + MagicalRestored);
		OutResult.PhysicalArmorAfter = ArmorSnapshot.CurrentPhysicalArmor;
		OutResult.MagicalArmorAfter = ArmorSnapshot.CurrentMagicalArmor;
		SelfTargetBefore.PhysicalArmor = ArmorSnapshot.CurrentPhysicalArmor;
		SelfTargetBefore.MagicalArmor = ArmorSnapshot.CurrentMagicalArmor;
	}
	if (!Action.Definition.StatusApplications.IsEmpty())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGridStatusEffectLifecycleSubsystem* StatusLifecycle = World->GetSubsystem<UGridStatusEffectLifecycleSubsystem>())
			{
				StatusLifecycle->BindToTurnManager(this);
				StatusLifecycle->ApplyCombatStatusApplicationsToPartyCharacter(
					Action.CharacterIndex, Action.Definition.StatusApplications, Character.CharacterId, SelfTargetBefore, nullptr);
			}
		}
	}
	if (!Action.Definition.StatusRemovals.IsEmpty())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGridStatusEffectLifecycleSubsystem* StatusLifecycle = World->GetSubsystem<UGridStatusEffectLifecycleSubsystem>())
			{
				StatusLifecycle->BindToTurnManager(this);
				StatusLifecycle->RemoveCombatStatusEffectsFromPartyCharacter(Action.CharacterIndex, Action.Definition.StatusRemovals);
			}
		}
	}
	StartCombatActionCooldown(Action);
	EmitCharacterActionResolvedReaction(Action.CharacterIndex, Action, FGuid::NewGuid());
	Inventory->NotifyPartyInventoryChanged(Action.CharacterIndex);
	if (FGridCombatantInitiativeEntry* Entry = FindInitiativeEntry(EGridCombatantSide::Party, Character.CharacterId))
	{
		RefreshInitiativeEntryVitals(*Entry);
		OnCombatantStateChanged.Broadcast(*Entry);
	}

	FGridPlayerCharacterTurnState TurnStateAfter;
	if (!InitiativeOrder.IsEmpty() && GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateAfter) && TurnStateAfter.RemainingActionPoints <= 0 &&
		IsActivePlayerCharacter(Action.CharacterIndex))
	{
		FinishActivePlayerTurn();
	}
	return true;
}

bool UGridTurnManagerComponent::RequestCharacterClassActionEffect(const FGridAvailableCombatAction& Action, FGridCombatClassActionResult& OutResult)
{
	OutResult = FGridCombatClassActionResult();
	UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	if (!IsValid(Inventory) || !Action.bEnabled || !IsMON1285ClassActionSource(Action.Definition.SourcePolicy) ||
		Action.Definition.ResolutionProfile != EGridCombatActionResolutionProfile::Effect ||
		Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::Self ||
		(!Action.Definition.EffectProfile.IsValid() && Action.Definition.StatusApplications.IsEmpty() && Action.Definition.StatusRemovals.IsEmpty() &&
			Action.Definition.ArmorEffects.IsEmpty() && Action.Definition.MovementEffects.IsEmpty()) ||
		Action.CurrentSourceItemQuantityCost != 0 || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Action.CharacterIndex))
	{
		return false;
	}

	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex];
	FGridInventoryCharacterSummary Summary;
	FGridPlayerCharacterTurnState TurnStateBefore;
	if (!Inventory->GetCharacterSummary(Action.CharacterIndex, Summary) || !GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateBefore) ||
		!CanCharacterSpendActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost))
	{
		return false;
	}

	OutResult.HealthBefore = Character.Resources.CurrentHealth;
	OutResult.ManaBefore = Character.Resources.CurrentMana;
	if (OutResult.ManaBefore < Action.CurrentManaCost)
	{
		return false;
	}
	OutResult.HealthAfter = FMath::Clamp(
		OutResult.HealthBefore + Action.Definition.EffectProfile.ResolveHealthRestore(Summary.DerivedStats.MaxHealth),
		0, FMath::Max(0, Summary.DerivedStats.MaxHealth));
	OutResult.ManaAfter = FMath::Clamp(
		OutResult.ManaBefore - Action.CurrentManaCost + Action.Definition.EffectProfile.ResolveManaRestore(Summary.DerivedStats.MaxMana),
		0, FMath::Max(0, Summary.DerivedStats.MaxMana));
	FGridCombatArmorPoolSnapshot ArmorSnapshot;
	FGridResolvedCombatModifiers ArmorModifiers;
	const FGridCombatArmorEffectSourceContext ArmorSourceContext =
		FGridCombatArmorEffectResolver::MakeSourceContext(Character, Summary.Attributes);
	const bool bHasArmorSnapshot = BuildRPG033PartyArmorSnapshot(Inventory, Action.CharacterIndex, ArmorSnapshot);
	if (bHasArmorSnapshot)
	{
		ResolveRPG033CharacterArmorModifiers(Character, Action, ArmorModifiers);
		FGridCombatArmorEffectResolver::ApplyReferenceModifiers(ArmorSnapshot, ArmorModifiers);
		OutResult.PhysicalArmorBefore = ArmorSnapshot.CurrentPhysicalArmor;
		OutResult.PhysicalArmorAfter = ArmorSnapshot.CurrentPhysicalArmor;
		OutResult.MagicalArmorBefore = ArmorSnapshot.CurrentMagicalArmor;
		OutResult.MagicalArmorAfter = ArmorSnapshot.CurrentMagicalArmor;
	}
	FGridAttackTargetStats SelfTargetBefore;
	SelfTargetBefore.CurrentHealth = OutResult.HealthBefore;
	SelfTargetBefore.PhysicalArmor = bHasArmorSnapshot ? ArmorSnapshot.CurrentPhysicalArmor : Character.Resources.CurrentPhysicalArmor;
	SelfTargetBefore.MagicalArmor = bHasArmorSnapshot ? ArmorSnapshot.CurrentMagicalArmor : Character.Resources.CurrentMagicalArmor;
	const bool bStatusWouldMutate = FGridCombatStatusApplicationResolver::WouldAnyMutate(
		Action.Definition.StatusApplications, Character.CharacterId, SelfTargetBefore, nullptr, Character.StatusEffects);
	TArray<FName> StatusRemovalIds;
	FGridCombatTargetingResolver::CollectStatusRemovalIds(Character.StatusEffects, Action.Definition.StatusRemovals, StatusRemovalIds);
	const bool bStatusRemovalWouldMutate = !StatusRemovalIds.IsEmpty();
	const bool bArmorWouldMutate =
		bHasArmorSnapshot && FGridCombatArmorEffectResolver::WouldAnyRestore(Action.Definition.ArmorEffects, ArmorSnapshot, ArmorModifiers, &ArmorSourceContext);
	FGridCombatMovementResolution MovementResolution;
	EGridPartyMovementRejectReason MovementRejectReason = EGridPartyMovementRejectReason::None;
	const bool bHasMovement = !Action.Definition.MovementEffects.IsEmpty();
	if (Action.Definition.MovementEffects.Num() > 1 ||
		(bHasMovement && !CanResolvePartyActionMovement(
			Action.CharacterIndex, Action.Definition.MovementEffects[0], MovementResolution, MovementRejectReason)))
	{
		return false;
	}
	if (OutResult.HealthAfter <= OutResult.HealthBefore && OutResult.ManaAfter <= OutResult.ManaBefore && !bStatusWouldMutate &&
		!bStatusRemovalWouldMutate && !bArmorWouldMutate && !bHasMovement)
	{
		return false;
	}

	if (!SpendPlayerCharacterActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost))
	{
		return false;
	}
	if (bHasMovement)
	{
		if (!StartPartyActionMovement(Action.CharacterIndex, Action.Definition.MovementEffects[0], MovementResolution, MovementRejectReason))
		{
			if (FGridPlayerCharacterTurnState* RestoredTurnState = EnsurePlayerCharacterTurnState(Action.CharacterIndex))
			{
				*RestoredTurnState = TurnStateBefore;
				BroadcastPlayerCharacterTurnState(*RestoredTurnState);
			}
			return false;
		}
		OutResult.bMovementStarted = true;
		OutResult.MovementFromCell = MovementResolution.FromCell;
		OutResult.MovementToCell = MovementResolution.ToCell;
	}

	Character.Resources.CurrentHealth = OutResult.HealthAfter;
	Character.Resources.CurrentMana = OutResult.ManaAfter;
	if (bHasArmorSnapshot && !Action.Definition.ArmorEffects.IsEmpty())
	{
		const int32 PhysicalBefore = ArmorSnapshot.CurrentPhysicalArmor;
		const int32 MagicalBefore = ArmorSnapshot.CurrentMagicalArmor;
		FGridCombatArmorEffectResolver::ApplyRestoreEffects(Action.Definition.ArmorEffects, ArmorSnapshot, ArmorModifiers, &ArmorSourceContext);
		const int32 PhysicalRestored = FMath::Max(0, ArmorSnapshot.CurrentPhysicalArmor - PhysicalBefore);
		const int32 MagicalRestored = FMath::Max(0, ArmorSnapshot.CurrentMagicalArmor - MagicalBefore);
		Character.Resources.CurrentPhysicalArmor = FMath::Max(0, Character.Resources.CurrentPhysicalArmor + PhysicalRestored);
		Character.Resources.CurrentMagicalArmor = FMath::Max(0, Character.Resources.CurrentMagicalArmor + MagicalRestored);
		OutResult.PhysicalArmorAfter = ArmorSnapshot.CurrentPhysicalArmor;
		OutResult.MagicalArmorAfter = ArmorSnapshot.CurrentMagicalArmor;
		SelfTargetBefore.PhysicalArmor = ArmorSnapshot.CurrentPhysicalArmor;
		SelfTargetBefore.MagicalArmor = ArmorSnapshot.CurrentMagicalArmor;
	}
	if (!Action.Definition.StatusApplications.IsEmpty())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGridStatusEffectLifecycleSubsystem* StatusLifecycle = World->GetSubsystem<UGridStatusEffectLifecycleSubsystem>())
			{
				StatusLifecycle->BindToTurnManager(this);
				StatusLifecycle->ApplyCombatStatusApplicationsToPartyCharacter(
					Action.CharacterIndex, Action.Definition.StatusApplications, Character.CharacterId, SelfTargetBefore, nullptr);
			}
		}
	}
	if (!Action.Definition.StatusRemovals.IsEmpty())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGridStatusEffectLifecycleSubsystem* StatusLifecycle = World->GetSubsystem<UGridStatusEffectLifecycleSubsystem>())
			{
				StatusLifecycle->BindToTurnManager(this);
				StatusLifecycle->RemoveCombatStatusEffectsFromPartyCharacter(Action.CharacterIndex, Action.Definition.StatusRemovals);
			}
		}
	}
	StartCombatActionCooldown(Action);
	EmitCharacterActionResolvedReaction(Action.CharacterIndex, Action, FGuid::NewGuid());
	Inventory->NotifyPartyInventoryChanged(Action.CharacterIndex);
	if (FGridCombatantInitiativeEntry* Entry = FindInitiativeEntry(EGridCombatantSide::Party, Character.CharacterId))
	{
		RefreshInitiativeEntryVitals(*Entry);
		OnCombatantStateChanged.Broadcast(*Entry);
	}

	FGridPlayerCharacterTurnState TurnStateAfter;
	if (!IsPartyMotionInProgress() && !InitiativeOrder.IsEmpty() && GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateAfter) &&
		TurnStateAfter.RemainingActionPoints <= 0 && IsActivePlayerCharacter(Action.CharacterIndex))
	{
		FinishActivePlayerTurn();
	}
	return true;
}

bool UGridTurnManagerComponent::RequestCharacterBatchPartyEffect(
	const FGridAvailableCombatAction& Action, const TArray<int32>& ExplicitTargetCharacterIndices, FGridCombatActionRequestResult& OutResult)
{
	UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	const bool bQuickItem = Action.Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem;
	const bool bClassAction = IsMON1285ClassActionSource(Action.Definition.SourcePolicy);
	if (!IsValid(Inventory) || !Action.bEnabled || (!bQuickItem && !bClassAction) ||
		Action.Definition.ResolutionProfile != EGridCombatActionResolutionProfile::Effect ||
		(Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::Ally &&
			Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::AllyOrHostile &&
			Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::Party &&
			Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::FrontRowParty) ||
		!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Action.CharacterIndex))
	{
		return false;
	}

	FGridCharacterInventoryState& SourceCharacter = Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex];
	FGridInventoryCharacterSummary SourceSummary;
	FGridPlayerCharacterTurnState TurnStateBefore;
	if (!Inventory->GetCharacterSummary(Action.CharacterIndex, SourceSummary) ||
		!GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateBefore) ||
		!CanCharacterSpendActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost) ||
		SourceCharacter.Resources.CurrentMana < Action.CurrentManaCost)
	{
		return false;
	}

	TArray<int32> TargetIndices;
	FGridCombatTargetingResolver::CollectPartyTargets(Inventory->PartyInventoryState, Action.Definition.TargetingPolicy,
		Action.CharacterIndex, ExplicitTargetCharacterIndices, 3, TargetIndices);
	if (Action.Definition.MaximumResolvedTargets > 0 && TargetIndices.Num() > Action.Definition.MaximumResolvedTargets)
	{
		TargetIndices.SetNum(Action.Definition.MaximumResolvedTargets);
	}
	if (TargetIndices.IsEmpty())
	{
		return false;
	}

	TArray<FGridCombatModifierProfile> SourceProfiles;
	FGridResolvedCombatModifiers SourceModifiers;
	if (FGridCombatModifierResolver::CollectCharacterModifiers(SourceCharacter, SourceProfiles))
	{
		FGridCombatModifierResolver::Resolve(SourceProfiles,
			FGridCombatModifierResolver::MakeActionContext(Action.Definition, Action.SourceDefinitionId), SourceModifiers);
	}

	const FGridQuickItemSecondaryEffectProjection SecondaryProjection =
		bQuickItem ? FGridQuickItemResolver::ResolveSecondaryEffect(SourceModifiers) : FGridQuickItemSecondaryEffectProjection();
	if (Action.Definition.TargetingPolicy == EGridCombatTargetingPolicy::Ally ||
		Action.Definition.TargetingPolicy == EGridCombatTargetingPolicy::AllyOrHostile)
	{
		const int32 MaximumExplicitTargets = 1 + (SecondaryProjection.IsEnabled() ? SecondaryProjection.TargetCount : 0);
		if (TargetIndices.Num() > MaximumExplicitTargets)
		{
			return false;
		}
	}

	FGridCombatActionEffectProfile BaseEffect = Action.Definition.EffectProfile;
	if (bQuickItem)
	{
		FGridQuickItemResolver::ResolveEffectProfile(Action.Definition, SourceCharacter.SkillRanks, SourceModifiers, BaseEffect);
	}
	const FGridCombatArmorEffectSourceContext ArmorSourceContext =
		FGridCombatArmorEffectResolver::MakeSourceContext(SourceCharacter, SourceSummary.Attributes);

	auto ScalePositive = [](int32 Value, int32 Percent)
	{
		return Value <= 0 ? 0 : FMath::Max(1, FMath::RoundToInt(static_cast<float>(Value) * static_cast<float>(Percent) / 100.0f));
	};

	auto BuildTargetPayloads = [&](int32 TargetOrdinal, FGridCombatActionEffectProfile& OutEffect,
								   TArray<FGridCombatArmorEffectProfile>& OutArmorEffects,
								   TArray<FGridCombatStatusApplicationProfile>& OutStatusApplications,
								   TArray<FGridCombatStatusRemovalProfile>& OutStatusRemovals)
	{
		OutEffect = BaseEffect;
		OutArmorEffects = Action.Definition.ArmorEffects;
		OutStatusApplications.Reset();
		for (const FGridCombatStatusApplicationProfile& Profile : Action.Definition.StatusApplications)
		{
			if (FGridCombatTargetingResolver::ShouldApplyStatusApplication(Profile, TargetOrdinal))
			{
				OutStatusApplications.Add(Profile);
			}
		}
		OutStatusRemovals = Action.Definition.StatusRemovals;

		if (TargetOrdinal <= 0 || !SecondaryProjection.IsEnabled())
		{
			return;
		}

		OutEffect.RestoreHealth = ScalePositive(OutEffect.RestoreHealth, SecondaryProjection.MagnitudePercent);
		OutEffect.RestoreMana = ScalePositive(OutEffect.RestoreMana, SecondaryProjection.MagnitudePercent);
		OutEffect.RestoreHealthMaximumPercent =
			ScalePositive(OutEffect.RestoreHealthMaximumPercent, SecondaryProjection.MagnitudePercent);
		OutEffect.RestoreManaMaximumPercent =
			ScalePositive(OutEffect.RestoreManaMaximumPercent, SecondaryProjection.MagnitudePercent);
		for (FGridCombatArmorEffectProfile& Armor : OutArmorEffects)
		{
			if (Armor.Operation == EGridCombatArmorEffectOperation::Restore)
			{
				Armor.Amount = ScalePositive(Armor.Amount, SecondaryProjection.MagnitudePercent);
			}
		}
		for (FGridCombatStatusApplicationProfile& Status : OutStatusApplications)
		{
			if (Status.DurationOverride == 0)
			{
				continue;
			}
			int32 Duration = Status.DurationOverride;
			if (Duration == INDEX_NONE)
			{
				if (const UGridStatusEffectDefinitionAsset* Definition =
						FGridCombatStatusApplicationResolver::ResolveDefinition(Status.StatusEffectId))
				{
					Duration = Definition->DefaultDuration;
				}
			}
			if (Duration > 0)
			{
				Status.DurationOverride = FMath::Max(
					1, FMath::RoundToInt(static_cast<float>(Duration) * static_cast<float>(SecondaryProjection.DurationPercent) / 100.0f));
			}
		}
		for (FGridCombatStatusRemovalProfile& Removal : OutStatusRemovals)
		{
			Removal.MaximumRemovals = FMath::Clamp(
				ScalePositive(Removal.MaximumRemovals, SecondaryProjection.MagnitudePercent), 1, 16);
		}
	};

	bool bAnyMutation = false;
	for (int32 TargetOrdinal = 0; TargetOrdinal < TargetIndices.Num() && !bAnyMutation; ++TargetOrdinal)
	{
		const int32 TargetIndex = TargetIndices[TargetOrdinal];
		if (!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(TargetIndex))
		{
			return false;
		}
		FGridCharacterInventoryState& TargetCharacter = Inventory->PartyInventoryState.ActiveCharacters[TargetIndex];
		FGridInventoryCharacterSummary TargetSummary;
		if (!Inventory->GetCharacterSummary(TargetIndex, TargetSummary) || TargetCharacter.Resources.CurrentHealth <= 0)
		{
			return false;
		}

		FGridCombatActionEffectProfile TargetEffect;
		TArray<FGridCombatArmorEffectProfile> ArmorEffects;
		TArray<FGridCombatStatusApplicationProfile> StatusApplications;
		TArray<FGridCombatStatusRemovalProfile> StatusRemovals;
		BuildTargetPayloads(TargetOrdinal, TargetEffect, ArmorEffects, StatusApplications, StatusRemovals);

		const int32 HealthAfter = FMath::Clamp(
			TargetCharacter.Resources.CurrentHealth + TargetEffect.ResolveHealthRestore(TargetSummary.DerivedStats.MaxHealth),
			0, FMath::Max(0, TargetSummary.DerivedStats.MaxHealth));
		const int32 ManaAfter = FMath::Clamp(
			TargetCharacter.Resources.CurrentMana + TargetEffect.ResolveManaRestore(TargetSummary.DerivedStats.MaxMana),
			0, FMath::Max(0, TargetSummary.DerivedStats.MaxMana));
		if (HealthAfter > TargetCharacter.Resources.CurrentHealth || ManaAfter > TargetCharacter.Resources.CurrentMana)
		{
			bAnyMutation = true;
			break;
		}

		FGridAttackTargetStats TargetBefore;
		TargetBefore.CurrentHealth = TargetCharacter.Resources.CurrentHealth;
		TargetBefore.PhysicalArmor = TargetCharacter.Resources.CurrentPhysicalArmor;
		TargetBefore.MagicalArmor = TargetCharacter.Resources.CurrentMagicalArmor;
		if (FGridCombatStatusApplicationResolver::WouldAnyMutate(
				StatusApplications, SourceCharacter.CharacterId, TargetBefore, nullptr, TargetCharacter.StatusEffects))
		{
			bAnyMutation = true;
			break;
		}
		TArray<FName> Removable;
		FGridCombatTargetingResolver::CollectStatusRemovalIds(
			TargetCharacter.StatusEffects, StatusRemovals, Removable, EGridCombatStatusRemovalTargetSide::Party);
		if (!Removable.IsEmpty())
		{
			bAnyMutation = true;
			break;
		}

		FGridCombatArmorPoolSnapshot ArmorSnapshot;
		if (BuildRPG033PartyArmorSnapshot(Inventory, TargetIndex, ArmorSnapshot))
		{
			TArray<FGridCombatModifierProfile> TargetProfiles;
			FGridResolvedCombatModifiers TargetModifiers;
			if (FGridCombatModifierResolver::CollectCharacterModifiers(TargetCharacter, TargetProfiles))
			{
				FGridCombatModifierResolver::Resolve(TargetProfiles,
					FGridCombatModifierResolver::MakeActionContext(Action.Definition, Action.SourceDefinitionId), TargetModifiers);
			}
			TargetModifiers.PositiveEffectPercentModifier = SourceModifiers.PositiveEffectPercentModifier;
			FGridCombatArmorEffectResolver::ApplyReferenceModifiers(ArmorSnapshot, TargetModifiers);
			if (FGridCombatArmorEffectResolver::WouldAnyRestore(ArmorEffects, ArmorSnapshot, TargetModifiers, &ArmorSourceContext))
			{
				bAnyMutation = true;
			}
		}
	}
	if (!bAnyMutation)
	{
		return false;
	}

	const int32 QuantityBefore =
		bQuickItem ? Inventory->CountItemDefinitionInCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId) : 0;
	if (bQuickItem &&
		(Action.SourceDefinitionId.IsNone() || Action.CurrentSourceItemQuantityCost <= 0 ||
			QuantityBefore < Action.CurrentSourceItemQuantityCost))
	{
		return false;
	}

	if (!SpendPlayerCharacterActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost))
	{
		return false;
	}
	const int32 SourceManaBefore = SourceCharacter.Resources.CurrentMana;
	SourceCharacter.Resources.CurrentMana = FMath::Max(0, SourceManaBefore - Action.CurrentManaCost);
	if (bQuickItem &&
		!Inventory->RemoveItemDefinitionFromCharacterInventory(
			Action.CharacterIndex, Action.SourceDefinitionId, Action.CurrentSourceItemQuantityCost))
	{
		SourceCharacter.Resources.CurrentMana = SourceManaBefore;
		if (FGridPlayerCharacterTurnState* RestoredTurnState = EnsurePlayerCharacterTurnState(Action.CharacterIndex))
		{
			*RestoredTurnState = TurnStateBefore;
			BroadcastPlayerCharacterTurnState(*RestoredTurnState);
		}
		return false;
	}

	UGridStatusEffectLifecycleSubsystem* StatusLifecycle =
		GetWorld() ? GetWorld()->GetSubsystem<UGridStatusEffectLifecycleSubsystem>() : nullptr;
	if (StatusLifecycle)
	{
		StatusLifecycle->BindToTurnManager(this);
	}

	OutResult.TargetedActionResult.TargetCharacterIndices = TargetIndices;
	for (int32 TargetOrdinal = 0; TargetOrdinal < TargetIndices.Num(); ++TargetOrdinal)
	{
		const int32 TargetIndex = TargetIndices[TargetOrdinal];
		FGridCharacterInventoryState& TargetCharacter = Inventory->PartyInventoryState.ActiveCharacters[TargetIndex];
		FGridInventoryCharacterSummary TargetSummary;
		if (!Inventory->GetCharacterSummary(TargetIndex, TargetSummary))
		{
			continue;
		}

		FGridCombatActionEffectProfile TargetEffect;
		TArray<FGridCombatArmorEffectProfile> ArmorEffects;
		TArray<FGridCombatStatusApplicationProfile> StatusApplications;
		TArray<FGridCombatStatusRemovalProfile> StatusRemovals;
		BuildTargetPayloads(TargetOrdinal, TargetEffect, ArmorEffects, StatusApplications, StatusRemovals);

		TargetCharacter.Resources.CurrentHealth = FMath::Clamp(
			TargetCharacter.Resources.CurrentHealth + TargetEffect.ResolveHealthRestore(TargetSummary.DerivedStats.MaxHealth),
			0, FMath::Max(0, TargetSummary.DerivedStats.MaxHealth));
		TargetCharacter.Resources.CurrentMana = FMath::Clamp(
			TargetCharacter.Resources.CurrentMana + TargetEffect.ResolveManaRestore(TargetSummary.DerivedStats.MaxMana),
			0, FMath::Max(0, TargetSummary.DerivedStats.MaxMana));

		FGridAttackTargetStats TargetBefore;
		TargetBefore.CurrentHealth = TargetCharacter.Resources.CurrentHealth;
		TargetBefore.PhysicalArmor = TargetCharacter.Resources.CurrentPhysicalArmor;
		TargetBefore.MagicalArmor = TargetCharacter.Resources.CurrentMagicalArmor;

		FGridCombatArmorPoolSnapshot ArmorSnapshot;
		if (BuildRPG033PartyArmorSnapshot(Inventory, TargetIndex, ArmorSnapshot) && !ArmorEffects.IsEmpty())
		{
			TArray<FGridCombatModifierProfile> TargetProfiles;
			FGridResolvedCombatModifiers TargetModifiers;
			if (FGridCombatModifierResolver::CollectCharacterModifiers(TargetCharacter, TargetProfiles))
			{
				FGridCombatModifierResolver::Resolve(TargetProfiles,
					FGridCombatModifierResolver::MakeActionContext(Action.Definition, Action.SourceDefinitionId), TargetModifiers);
			}
			TargetModifiers.PositiveEffectPercentModifier = SourceModifiers.PositiveEffectPercentModifier;
			FGridCombatArmorEffectResolver::ApplyReferenceModifiers(ArmorSnapshot, TargetModifiers);
			const int32 PhysicalBefore = ArmorSnapshot.CurrentPhysicalArmor;
			const int32 MagicalBefore = ArmorSnapshot.CurrentMagicalArmor;
			FGridCombatArmorEffectResolver::ApplyRestoreEffects(
				ArmorEffects, ArmorSnapshot, TargetModifiers, &ArmorSourceContext);
			TargetCharacter.Resources.CurrentPhysicalArmor =
				FMath::Max(0, TargetCharacter.Resources.CurrentPhysicalArmor + FMath::Max(0, ArmorSnapshot.CurrentPhysicalArmor - PhysicalBefore));
			TargetCharacter.Resources.CurrentMagicalArmor =
				FMath::Max(0, TargetCharacter.Resources.CurrentMagicalArmor + FMath::Max(0, ArmorSnapshot.CurrentMagicalArmor - MagicalBefore));
			TargetBefore.PhysicalArmor = ArmorSnapshot.CurrentPhysicalArmor;
			TargetBefore.MagicalArmor = ArmorSnapshot.CurrentMagicalArmor;
		}

		if (StatusLifecycle)
		{
			if (!StatusApplications.IsEmpty())
			{
				StatusLifecycle->ApplyCombatStatusApplicationsToPartyCharacter(
					TargetIndex, StatusApplications, SourceCharacter.CharacterId, TargetBefore, nullptr);
			}
			if (!StatusRemovals.IsEmpty())
			{
				StatusLifecycle->RemoveCombatStatusEffectsFromPartyCharacter(TargetIndex, StatusRemovals);
			}
		}

		Inventory->NotifyPartyInventoryChanged(TargetIndex);
		if (FGridCombatantInitiativeEntry* TargetEntry = FindInitiativeEntry(EGridCombatantSide::Party, TargetCharacter.CharacterId))
		{
			RefreshInitiativeEntryVitals(*TargetEntry);
			OnCombatantStateChanged.Broadcast(*TargetEntry);
		}
	}

	StartCombatActionCooldown(Action);
	EmitCharacterActionResolvedReaction(Action.CharacterIndex, Action, FGuid::NewGuid());
	if (bQuickItem)
	{
		OutResult.QuickItemResult.SourceQuantityBefore = QuantityBefore;
		OutResult.QuickItemResult.SourceQuantityAfter =
			Inventory->CountItemDefinitionInCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId);
		OutResult.QuickItemResult.ManaBefore = SourceManaBefore;
		OutResult.QuickItemResult.ManaAfter = SourceCharacter.Resources.CurrentMana;
	}
	else
	{
		OutResult.ClassActionResult.ManaBefore = SourceManaBefore;
		OutResult.ClassActionResult.ManaAfter = SourceCharacter.Resources.CurrentMana;
	}

	FGridPlayerCharacterTurnState TurnStateAfter;
	if (!InitiativeOrder.IsEmpty() && GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateAfter) &&
		TurnStateAfter.RemainingActionPoints <= 0 && IsActivePlayerCharacter(Action.CharacterIndex))
	{
		FinishActivePlayerTurn();
	}
	return true;
}

bool UGridTurnManagerComponent::RequestCharacterClassActionAttack(const FGridAvailableCombatAction& Action, FGridPlayerAttackRequest& OutRequest,
	FGridAttackResult& OutResult, EGridPlayerAttackRejectReason& OutRejectReason, FGridCombatClassActionResult& OutClassResult)
{
	OutRequest = FGridPlayerAttackRequest();
	OutResult = FGridAttackResult();
	OutRejectReason = EGridPlayerAttackRejectReason::None;
	OutClassResult = FGridCombatClassActionResult();
	UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	if (!IsValid(Inventory) || !Action.bEnabled || !IsMON1285ClassActionSource(Action.Definition.SourcePolicy) ||
		Action.Definition.ResolutionProfile != EGridCombatActionResolutionProfile::Attack ||
		Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::FirstAxialTarget || Action.CurrentSourceItemQuantityCost != 0 ||
		!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Action.CharacterIndex))
	{
		return false;
	}

	const FGridCharacterInventoryState& CharacterBefore = Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex];
	OutClassResult.HealthBefore = CharacterBefore.Resources.CurrentHealth;
	OutClassResult.HealthAfter = OutClassResult.HealthBefore;
	OutClassResult.ManaBefore = CharacterBefore.Resources.CurrentMana;
	OutClassResult.ManaAfter = OutClassResult.ManaBefore;
	if (OutClassResult.ManaBefore < Action.CurrentManaCost)
	{
		return false;
	}

	if (!RequestCharacterAttackInternal(Action.CharacterIndex, EGridEquipmentSlot::None, false, &Action, OutRequest, OutResult, OutRejectReason))
	{
		return false;
	}

	if (IsValid(Inventory) && Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Action.CharacterIndex))
	{
		OutClassResult.ManaAfter = Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex].Resources.CurrentMana;
	}
	return true;
}

bool UGridTurnManagerComponent::BuildTargetingPreviewForAction(
	const FGridAvailableCombatAction& Action, const FIntPoint& TargetCell, FGridCombatActionTargetingPreview& OutPreview) const
{
	OutPreview = FGridCombatActionTargetingPreview();
	OutPreview.Action = Action;
	OutPreview.TargetCell = TargetCell;

	if (!IsValid(RuntimeActor) || !IsValid(PartyPawn))
	{
		OutPreview.InvalidReason = MakeMON1286TargetingReason(TEXT("Le niveau ou le groupe est indisponible."));
		return false;
	}
	const bool bCardinalFacing = PartyPawn->Facing == EGridEdge::North || PartyPawn->Facing == EGridEdge::East || PartyPawn->Facing == EGridEdge::South ||
		PartyPawn->Facing == EGridEdge::West;
	if (!bCardinalFacing)
	{
		OutPreview.InvalidReason = MakeMON1286TargetingReason(TEXT("L'orientation du groupe est invalide."));
		return false;
	}
	if (!Action.IsValid() || !Action.bEnabled)
	{
		OutPreview.InvalidReason = Action.DisabledReason.IsEmpty() ? MakeMON1286TargetingReason(TEXT("Cette action est indisponible.")) : Action.DisabledReason;
		return false;
	}

	const bool bAttackResolution = Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
		(Action.Definition.OffensiveProfile.IsValid() || Action.Definition.WeaponAttackProfile.bUseEquippedWeapon);
	const bool bSurfaceEffectResolution =
		Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect && !Action.Definition.SurfaceEffects.IsEmpty();
	const bool bTrapEffectResolution =
		Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect && Action.Definition.TrapEffect.bPlaceTrap;
	const bool bRelocationEffectResolution =
		Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect && Action.Definition.bRelocatePartyToTargetCell;
	if (!IsMON1286TargetedSource(Action.Definition.SourcePolicy) || !IsMON1286ExplicitTargetingPolicy(Action.Definition.TargetingPolicy) ||
		(!bAttackResolution && !bSurfaceEffectResolution && !bTrapEffectResolution && !bRelocationEffectResolution))
	{
		OutPreview.InvalidReason = MakeMON1286TargetingReason(TEXT("Cette action ne prend pas de cible cellule ou zone."));
		return false;
	}
	if (!RuntimeActor->IsValidCell(TargetCell.X, TargetCell.Y) || RuntimeActor->GetCell(TargetCell.X, TargetCell.Y).CellType == EGridCellType::Empty)
	{
		OutPreview.InvalidReason = MakeMON1286TargetingReason(TEXT("Cette cellule n'appartient pas au niveau jouable."));
		return false;
	}
	if (bTrapEffectResolution && !RuntimeActor->IsWalkableCell(TargetCell.X, TargetCell.Y))
	{
		OutPreview.InvalidReason = MakeMON1286TargetingReason(TEXT("Un piège doit être posé sur une cellule marchable."));
		return false;
	}
	if (bRelocationEffectResolution && !RuntimeActor->IsWalkableCell(TargetCell.X, TargetCell.Y))
	{
		OutPreview.InvalidReason = MakeMON1286TargetingReason(TEXT("La destination doit être une cellule marchable."));
		return false;
	}

	const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
	const int32 TargetDistance = FMath::Abs(TargetCell.X - PartyCell.X) + FMath::Abs(TargetCell.Y - PartyCell.Y);
	if (TargetDistance > Action.Definition.RangeCells)
	{
		OutPreview.InvalidReason = MakeMON1286TargetingReason(TEXT("Cette cellule est hors de portée."));
		return false;
	}
	if (Action.Definition.bRequiresLineOfSight)
	{
		const bool bGridLineClear = FGridMonsterPerception::HasStraightLineOfSight(
			PartyCell, TargetCell, Action.Definition.RangeCells,
			[this](const FIntPoint& From, const FIntPoint& To)
			{
				const EGridEdge Direction = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(From, To);
				return IsValid(RuntimeActor) && Direction != EGridEdge::None &&
					RuntimeActor->CanMove(From.X, From.Y, Direction);
			});
		const bool bSmokeBlocked = Action.Definition.ActionType == EGridCombatActionType::RangedAttack &&
			RuntimeActor->DoesCombatSmokeBlockLine(PartyCell, TargetCell);
		if (!bGridLineClear || bSmokeBlocked)
		{
			OutPreview.InvalidReason = MakeMON1286TargetingReason(TEXT("La ligne de vue vers cette cellule est bloquée."));
			return false;
		}
	}

	if (Action.Definition.TargetingPolicy == EGridCombatTargetingPolicy::Cell)
	{
		OutPreview.AffectedCells.Add(TargetCell);
	}
	else
	{
		const int32 Radius = Action.Definition.AreaRadiusCells;
		for (int32 Y = TargetCell.Y - Radius; Y <= TargetCell.Y + Radius; ++Y)
		{
			for (int32 X = TargetCell.X - Radius; X <= TargetCell.X + Radius; ++X)
			{
				const int32 Distance = FMath::Abs(X - TargetCell.X) + FMath::Abs(Y - TargetCell.Y);
				if (Distance > Radius || !RuntimeActor->IsValidCell(X, Y) || RuntimeActor->GetCell(X, Y).CellType == EGridCellType::Empty)
				{
					continue;
				}
				OutPreview.AffectedCells.Add(FIntPoint(X, Y));
			}
		}
	}
	OutPreview.AffectedCells.Sort(
		[TargetCell](const FIntPoint& Left, const FIntPoint& Right)
		{
			const int32 LeftDistance = FMath::Abs(Left.X - TargetCell.X) + FMath::Abs(Left.Y - TargetCell.Y);
			const int32 RightDistance = FMath::Abs(Right.X - TargetCell.X) + FMath::Abs(Right.Y - TargetCell.Y);
			if (LeftDistance != RightDistance)
			{
				return LeftDistance < RightDistance;
			}
			return Left.Y == Right.Y ? Left.X < Right.X : Left.Y < Right.Y;
		});

	UWorld* World = GetWorld();
	UGridMonsterOccupancySubsystem* Occupancy = World ? World->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr;
	if (!IsValid(Occupancy))
	{
		OutPreview.InvalidReason = MakeMON1286TargetingReason(TEXT("L'occupation du niveau est indisponible."));
		return false;
	}
	if (bRelocationEffectResolution &&
		!FGridCombatMovementResolver::CanRelocatePartyToSelectedCell(
			PartyCell, TargetCell, Action.Definition.RangeCells, RuntimeActor, Occupancy))
	{
		OutPreview.InvalidReason =
			MakeMON1286TargetingReason(TEXT("La destination est occupée ou séparée par un mur, une porte fermée ou une limite de niveau."));
		return false;
	}

	for (const FIntPoint& Cell : OutPreview.AffectedCells)
	{
		AGridMonsterActor* Monster = Occupancy->GetOccupantAtCell(Cell);
		if (!IsValid(Monster) || !IsCombatMonster(Monster) || !Monster->bMonsterEnabled || !Monster->IsRuntimeLevelActive() || Monster->IsDead())
		{
			continue;
		}
		if (Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::Area &&
			!FGridCombatTargetingResolver::IsDirectHostileTargetable(Monster->StatusEffects))
		{
			continue;
		}
		if (!MatchesRPG0391MonsterTargetFilter(Action.Definition.TargetFilter, Monster, Action.CharacterId))
		{
			continue;
		}
		const FGuid MonsterId = Monster->ResolvePersistenceId();
		if (MonsterId.IsValid())
		{
			OutPreview.TargetMonsterIds.Add(MonsterId);
			if (Action.Definition.MaximumResolvedTargets > 0 &&
				OutPreview.TargetMonsterIds.Num() >= Action.Definition.MaximumResolvedTargets)
			{
				break;
			}
		}
	}


	if (bAttackResolution && Action.Definition.ChainJumpRangeCells > 0 && !OutPreview.TargetMonsterIds.IsEmpty())
	{
		const FGuid PrimaryTargetId = OutPreview.TargetMonsterIds[0];
		TArray<FGridCombatChainTargetCandidate> Candidates;
		for (AGridMonsterActor* CandidateMonster : CombatMonsters)
		{
			if (!IsValid(CandidateMonster) || !CandidateMonster->bMonsterEnabled || !CandidateMonster->IsRuntimeLevelActive() ||
				CandidateMonster->IsDead() ||
				!FGridCombatTargetingResolver::IsDirectHostileTargetable(CandidateMonster->StatusEffects) ||
				!MatchesRPG0391MonsterTargetFilter(Action.Definition.TargetFilter, CandidateMonster, Action.CharacterId))
			{
				continue;
			}
			FGridCombatChainTargetCandidate Candidate;
			Candidate.TargetId = CandidateMonster->ResolvePersistenceId();
			Candidate.Cell = CandidateMonster->CurrentCell;
			if (Candidate.IsValid())
			{
				Candidates.Add(Candidate);
			}
		}

		TArray<FGridCombatChainTargetCandidate> ChainTargets;
		FGridCombatTargetingResolver::BuildDeterministicChain(
			PrimaryTargetId, TargetCell, Candidates, Action.Definition.ChainJumpRangeCells,
			Action.Definition.MaximumResolvedTargets, ChainTargets);
		if (!ChainTargets.IsEmpty())
		{
			OutPreview.TargetMonsterIds.Reset(ChainTargets.Num());
			for (const FGridCombatChainTargetCandidate& ChainTarget : ChainTargets)
			{
				OutPreview.TargetMonsterIds.Add(ChainTarget.TargetId);
				OutPreview.AffectedCells.AddUnique(ChainTarget.Cell);
			}
		}
	}

	if (bAttackResolution && OutPreview.TargetMonsterIds.IsEmpty() &&
		Action.Definition.SurfaceEffects.IsEmpty() && !Action.Definition.TrapEffect.bPlaceTrap)
	{
		OutPreview.InvalidReason = MakeMON1286TargetingReason(Action.Definition.TargetingPolicy == EGridCombatTargetingPolicy::Area
				? TEXT("Cette zone ne contient aucune cible éligible.")
				: TEXT("Cette cellule ne contient aucune cible éligible."));
		return false;
	}

	OutPreview.bValid = true;
	OutPreview.InvalidReason = FText::GetEmpty();
	return true;
}

bool UGridTurnManagerComponent::BuildCombatActionTargetingPreview(int32 CharacterIndex, FName ActionId, EGridCombatActionSourcePolicy SourcePolicy,
	FName SourceDefinitionId, EGridEquipmentSlot SourceEquipmentSlot, FIntPoint TargetCell, FGridCombatActionTargetingPreview& OutPreview) const
{
	OutPreview = FGridCombatActionTargetingPreview();
	TArray<FGridAvailableCombatAction> AvailableActions;
	GetAvailableCombatActions(CharacterIndex, AvailableActions);
	const FGridAvailableCombatAction* Action = AvailableActions.FindByPredicate(
		[ActionId, SourcePolicy, SourceDefinitionId, SourceEquipmentSlot](const FGridAvailableCombatAction& Candidate)
		{
			return Candidate.MatchesSource(ActionId, SourcePolicy, SourceDefinitionId, SourceEquipmentSlot);
		});
	if (!Action)
	{
		OutPreview.TargetCell = TargetCell;
		OutPreview.InvalidReason = MakeMON1286TargetingReason(TEXT("Cette action n'est plus disponible."));
		return false;
	}
	return BuildTargetingPreviewForAction(*Action, TargetCell, OutPreview);
}

bool UGridTurnManagerComponent::RequestCharacterTargetedAttack(
	const FGridAvailableCombatAction& Action, const FGridCombatActionTargetingPreview& Preview, FGridCombatActionRequestResult& OutResult)
{
	UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	const bool bAttackResolution = Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack;
	const bool bSurfaceResolution = !Action.Definition.SurfaceEffects.IsEmpty();
	const bool bTrapResolution = Action.Definition.TrapEffect.bPlaceTrap;
	if (!IsValid(Inventory) || !Preview.bValid || (!bAttackResolution && !bSurfaceResolution && !bTrapResolution) ||
		(bAttackResolution && Preview.TargetMonsterIds.IsEmpty() && !bSurfaceResolution && !bTrapResolution) ||
		!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Action.CharacterIndex))
	{
		return false;
	}

	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex];
	const FIntPoint PartyCellForTargetedAction(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
	if (Action.Definition.ActionType == EGridCombatActionType::RangedAttack && IsValid(RuntimeActor) &&
		RuntimeActor->DoesCombatSmokeBlockLine(PartyCellForTargetedAction, Preview.TargetCell))
	{
		return false;
	}
	FGridInventoryCharacterSummary CharacterSummary;
	FGridPlayerCharacterTurnState TurnStateBefore;
	if (!Inventory->GetCharacterSummary(Action.CharacterIndex, CharacterSummary) || !GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateBefore) ||
		!CanCharacterSpendActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost) || Character.Resources.CurrentMana < Action.CurrentManaCost)
	{
		return false;
	}

	FGridOffensiveEquipmentProfile EffectiveOffensiveProfile = Action.Definition.OffensiveProfile;
	FName EffectiveOffensiveItemDefinitionId = NAME_None;
	EGridEquipmentSlot EffectiveOffensiveEquipmentSlot = EGridEquipmentSlot::None;
	TArray<FName> EffectiveOffensiveSourceTags;
	const bool bUsesEquippedWeaponAction = bAttackResolution && Action.Definition.WeaponAttackProfile.bUseEquippedWeapon;
	if (bUsesEquippedWeaponAction)
	{
		EGridPlayerAttackRejectReason WeaponRejectReason = EGridPlayerAttackRejectReason::None;
		if (!ResolveCombatActionWeaponProfile(Inventory, Action.CharacterIndex, Action.Definition, EffectiveOffensiveProfile,
				EffectiveOffensiveItemDefinitionId, EffectiveOffensiveEquipmentSlot, EffectiveOffensiveSourceTags, WeaponRejectReason))
		{
			return false;
		}
	}

	TArray<FGridCombatModifierProfile> ChoiceModifiers;
	FGridResolvedCombatModifiers ResolvedModifiers;
	const FGridCombatArmorEffectSourceContext ArmorSourceContext =
		FGridCombatArmorEffectResolver::MakeSourceContext(Character, CharacterSummary.Attributes);
	if (FGridCombatModifierResolver::CollectCharacterModifiers(Character, ChoiceModifiers))
	{
		const FGridCombatModifierContext ModifierContext = bUsesEquippedWeaponAction
			? FGridCombatModifierResolver::MakeResolvedActionAttackContext(
				Action.Definition, Action.SourceDefinitionId, EffectiveOffensiveProfile, EffectiveOffensiveSourceTags)
			: FGridCombatModifierResolver::MakeActionContext(Action.Definition, Action.SourceDefinitionId);
		FGridCombatModifierResolver::Resolve(ChoiceModifiers, ModifierContext, ResolvedModifiers);
	}

	TArray<AGridMonsterActor*> TargetMonsters;
	TargetMonsters.Reserve(Preview.TargetMonsterIds.Num());
	for (const FGuid& TargetId : Preview.TargetMonsterIds)
	{
		AGridMonsterActor* TargetMonster = FindCombatMonsterById(TargetId);
		if (!IsValid(TargetMonster) || !TargetMonster->bMonsterEnabled || !TargetMonster->IsRuntimeLevelActive() ||
			TargetMonster->IsDead() || !IsValid(TargetMonster->MonsterDefinition))
		{
			return false;
		}
		TargetMonsters.Add(TargetMonster);
	}

	const bool bQuickItem = Action.Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem;
	const int32 QuantityBefore = bQuickItem ? Inventory->CountItemDefinitionInCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId) : 0;
	if (bQuickItem &&
		(Action.SourceDefinitionId.IsNone() || Action.CurrentSourceItemQuantityCost <= 0 || QuantityBefore < Action.CurrentSourceItemQuantityCost))
	{
		return false;
	}

	if (!SpendPlayerCharacterActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost))
	{
		return false;
	}
	if (bQuickItem &&
		!Inventory->RemoveItemDefinitionFromCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId, Action.CurrentSourceItemQuantityCost))
	{
		if (FGridPlayerCharacterTurnState* RestoredTurnState = EnsurePlayerCharacterTurnState(Action.CharacterIndex))
		{
			*RestoredTurnState = TurnStateBefore;
			BroadcastPlayerCharacterTurnState(*RestoredTurnState);
		}
		return false;
	}

	const int32 ManaBefore = Character.Resources.CurrentMana;
	Character.Resources.CurrentMana = FMath::Max(0, ManaBefore - Action.CurrentManaCost);
	StartCombatActionCooldown(Action);
	Inventory->NotifyPartyInventoryChanged(Action.CharacterIndex);

	OutResult.TargetedActionResult.TargetCell = Preview.TargetCell;
	OutResult.TargetedActionResult.AffectedCells = Preview.AffectedCells;
	OutResult.TargetedActionResult.TargetMonsterIds =
		bAttackResolution ? Preview.TargetMonsterIds : TArray<FGuid>();
	if (bQuickItem)
	{
		OutResult.QuickItemResult.SourceQuantityBefore = QuantityBefore;
		OutResult.QuickItemResult.SourceQuantityAfter = Inventory->CountItemDefinitionInCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId);
		OutResult.QuickItemResult.HealthBefore = Character.Resources.CurrentHealth;
		OutResult.QuickItemResult.HealthAfter = Character.Resources.CurrentHealth;
		OutResult.QuickItemResult.ManaBefore = ManaBefore;
		OutResult.QuickItemResult.ManaAfter = Character.Resources.CurrentMana;
	}
	else
	{
		OutResult.ClassActionResult.HealthBefore = Character.Resources.CurrentHealth;
		OutResult.ClassActionResult.HealthAfter = Character.Resources.CurrentHealth;
		OutResult.ClassActionResult.ManaBefore = ManaBefore;
		OutResult.ClassActionResult.ManaAfter = Character.Resources.CurrentMana;
	}

	const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
	const FGuid ReactionActionInstanceId = FGuid::NewGuid();
	bPlayerAttackResolutionInProgress = true;
	for (int32 Index = 0; Index < TargetMonsters.Num(); ++Index)
	{
		AGridMonsterActor* TargetMonster = TargetMonsters[Index];
		const FGuid TargetMonsterId = Preview.TargetMonsterIds[Index];
		const FIntPoint TargetMonsterCell = TargetMonster->CurrentCell;

		for (int32 ResolutionIndex = 0;
			ResolutionIndex < Action.Definition.ResolutionCount && IsValid(TargetMonster) && !TargetMonster->IsDead();
			++ResolutionIndex)
		{
			FGridAttackSourceStats Source;
			FGridAttackTargetStats Target;
			FGridAttackDefinition AttackDefinition;
			if (!BuildPlayerAttackResolutionInputs(CharacterSummary, TargetMonster, EffectiveOffensiveProfile, Source, Target, AttackDefinition))
			{
				break;
			}
			FGridCombatModifierResolver::ApplyDirectDamageSkillScaling(Action.Definition, Character.SkillRanks, Source);
			FGridQuickItemResolver::ApplyDirectDamageSkillScaling(Action.Definition, Character.SkillRanks, Source);
			if (bUsesEquippedWeaponAction)
			{
				Source.RawDamagePercent = Action.Definition.WeaponAttackProfile.WeaponDamagePercent;
			}
			FGridResolvedCombatModifiers TargetedSourceModifiers = ResolvedModifiers;
			if (!ChoiceModifiers.IsEmpty())
			{
				FGridCombatModifierContext TargetedSourceContext = bUsesEquippedWeaponAction
					? FGridCombatModifierResolver::MakeResolvedActionAttackContext(
						Action.Definition, Action.SourceDefinitionId, EffectiveOffensiveProfile, EffectiveOffensiveSourceTags)
					: FGridCombatModifierResolver::MakeActionContext(Action.Definition, Action.SourceDefinitionId);
				FGridCombatModifierResolver::AddTargetStatusContext(TargetedSourceContext, TargetMonster->StatusEffects, Character.CharacterId,
					IsValid(TargetMonster->MonsterDefinition) ? TargetMonster->MonsterDefinition->CategoryId : NAME_None,
					IsValid(TargetMonster->MonsterDefinition) ? TargetMonster->MonsterDefinition->SemanticTags : TArray<FName>());
				TargetedSourceContext.bPartyStationarySincePreviousActivation =
					IsPartyStationarySincePreviousActivation(Character.CharacterId);
				FGridCombatModifierResolver::Resolve(ChoiceModifiers, TargetedSourceContext, TargetedSourceModifiers);
			}
			FGridCombatModifierResolver::ApplyOutgoingAttackModifiers(Source, TargetedSourceModifiers);
			if (Action.Definition.ActionType == EGridCombatActionType::RangedAttack && IsValid(RuntimeActor) &&
				RuntimeActor->IsCombatSmokeAtCell(TargetMonster->CurrentCell.X, TargetMonster->CurrentCell.Y))
			{
				Target.Evasion += 2;
			}
			if (ResolutionIndex > 0)
			{
				Source.Accuracy += Action.Definition.SubsequentResolutionAccuracyModifier;
			}

			TArray<FGridCombatModifierProfile> TargetStatusModifiers;
			FGridResolvedCombatModifiers TargetResolvedModifiers;
			if (FGridCombatModifierResolver::CollectStatusModifiers(TargetMonster->StatusEffects, TargetStatusModifiers))
			{
				const FGridCombatModifierContext TargetModifierContext = bUsesEquippedWeaponAction
					? FGridCombatModifierResolver::MakeResolvedActionAttackContext(
						Action.Definition, Action.SourceDefinitionId, EffectiveOffensiveProfile, EffectiveOffensiveSourceTags)
					: FGridCombatModifierResolver::MakeActionContext(Action.Definition, Action.SourceDefinitionId);
				FGridCombatModifierResolver::Resolve(TargetStatusModifiers, TargetModifierContext, TargetResolvedModifiers);
				FGridCombatModifierResolver::ApplyIncomingAttackModifiers(Target, AttackDefinition.DamageType, TargetResolvedModifiers);
			}

			FGridPlayerAttackRequest Request;
			Request.RequestId = FGuid::NewGuid();
			Request.RoundNumber = RoundNumber;
			Request.AttackerCharacterIndex = Action.CharacterIndex;
			Request.AttackerCharacterId = Character.CharacterId;
			Request.TargetMonsterId = TargetMonsterId;
			Request.PartyCell = PartyCell;
			Request.TargetCell = TargetMonsterCell;
			Request.PartyFacing = PartyPawn->Facing;
			Request.RangeCells = Action.Definition.RangeCells;
			Request.AttackId = EffectiveOffensiveProfile.AttackId;
			Request.OffensiveItemDefinitionId = bQuickItem ? Action.SourceDefinitionId : EffectiveOffensiveItemDefinitionId;
			Request.OffensiveEquipmentSlot = bQuickItem ? EGridEquipmentSlot::None : EffectiveOffensiveEquipmentSlot;
			Request.ActionPointCost = Action.CurrentActionPointCost;

			FGridAttackResult AttackResult =
				FGridCombatResolver::ResolveAttack(Source, Target, AttackDefinition, CombatRandomStream);
			FGridCombatArmorPoolSnapshot TargetArmorSnapshot;
			TargetArmorSnapshot.CurrentPhysicalArmor = Target.PhysicalArmor;
			TargetArmorSnapshot.CurrentMagicalArmor = Target.MagicalArmor;
			TargetArmorSnapshot.ReferencePhysicalArmor = FMath::Max(0, TargetMonster->MonsterDefinition->PhysicalArmor);
			TargetArmorSnapshot.ReferenceMagicalArmor = FMath::Max(0, TargetMonster->MonsterDefinition->MagicalArmor);
			FGridCombatArmorEffectResolver::ApplyReferenceModifiers(TargetArmorSnapshot, TargetResolvedModifiers);
			FGridCombatArmorEffectResolver::ApplyAttackDamageEffects(
				Action.Definition.ArmorEffects, TargetArmorSnapshot, TargetResolvedModifiers, AttackResult, &ArmorSourceContext);

			OutResult.TargetedActionResult.AttackRequests.Add(Request);
			OutResult.TargetedActionResult.AttackResults.Add(AttackResult);
			if (OutResult.TargetedActionResult.AttackResults.Num() == 1)
			{
				OutResult.AttackRequest = Request;
				OutResult.AttackResult = AttackResult;
			}
			LastPlayerAttackRequest = Request;
			LastPlayerAttackResult = AttackResult;
			LastPlayerAttackRejectReason = EGridPlayerAttackRejectReason::None;

			++PlayerAttackRequestedBroadcastCount;
			OnPlayerAttackRequested.Broadcast(Request);

			FGridCombatLogEntry AttackEntry;
			AttackEntry.RoundNumber = RoundNumber;
			AttackEntry.Phase = CurrentPhase;
			AttackEntry.Type = AttackResult.bHit ? EGridCombatLogEntryType::AttackHit : EGridCombatLogEntryType::AttackMiss;
			AttackEntry.SourceId = FName(*Character.CharacterId.ToString(EGuidFormats::Digits));
			AttackEntry.SourceDisplayName = CharacterSummary.DisplayName;
			AttackEntry.TargetId = FName(*TargetMonsterId.ToString(EGuidFormats::Digits));
			AttackEntry.TargetDisplayName = ResolveMonsterDisplayName(TargetMonster);
			AttackEntry.TargetCharacterIndex = INDEX_NONE;
			AttackEntry.AttackId = Request.AttackId;
			AttackEntry.OffensiveItemDefinitionId = Request.OffensiveItemDefinitionId;
			AttackEntry.OffensiveEquipmentSlot = Request.OffensiveEquipmentSlot;
			AttackEntry.AttackResult = AttackResult;
			AttackEntry.bTargetDefeated = AttackResult.TargetHealthBefore > 0 && AttackResult.TargetHealthAfter <= 0;
			AttackEntry.Message =
				FGridCombatLogFormatter::FormatPlayerAttack(AttackEntry.SourceDisplayName, AttackEntry.TargetDisplayName, AttackEntry.AttackId, AttackResult);
			AppendCombatLogEntry(AttackEntry);

			TargetMonster->ApplyAttackResult(AttackResult);
			if (!Action.Definition.StatusApplications.IsEmpty())
			{
				TArray<FGridCombatStatusApplicationProfile> ApplicableStatusProfiles;
				for (const FGridCombatStatusApplicationProfile& Profile : Action.Definition.StatusApplications)
				{
					if (FGridCombatTargetingResolver::ShouldApplyStatusApplication(Profile, Index))
					{
						ApplicableStatusProfiles.Add(Profile);
					}
				}
				if (!ApplicableStatusProfiles.IsEmpty())
				{
					if (UWorld* World = GetWorld())
					{
						if (UGridStatusEffectLifecycleSubsystem* StatusLifecycle = World->GetSubsystem<UGridStatusEffectLifecycleSubsystem>())
						{
							StatusLifecycle->BindToTurnManager(this);
							StatusLifecycle->ApplyCombatStatusApplicationsToMonster(
								TargetMonster, ApplicableStatusProfiles, Character.CharacterId, Target, &AttackResult);
						}
					}
				}
			}
			if (ResolutionIndex == 0 && !Action.Definition.StatusRemovals.IsEmpty())
			{
				if (UWorld* World = GetWorld())
				{
					if (UGridStatusEffectLifecycleSubsystem* StatusLifecycle = World->GetSubsystem<UGridStatusEffectLifecycleSubsystem>())
					{
						StatusLifecycle->BindToTurnManager(this);
						StatusLifecycle->RemoveCombatStatusEffectsFromMonster(TargetMonster, Action.Definition.StatusRemovals);
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
			EmitPlayerAttackReactionEvents(Action.CharacterIndex, Request, AttackResult, Action.Definition.SourcePolicy,
				Action.Definition.ActionType, ReactionActionInstanceId, false, false);
			++PlayerAttackResolvedBroadcastCount;
			OnPlayerAttackResolved.Broadcast(Request, TargetMonster, AttackResult);
			if (bCollectRuntimeMetrics)
			{
				++RuntimeMetrics.AttacksResolved;
			}
		}
	}

	if (bAttackResolution && Action.Definition.bAffectsAlliesInArea && Preview.AffectedCells.Contains(PartyCell))
	{
		for (int32 TargetCharacterIndex = 0;
			TargetCharacterIndex < Inventory->PartyInventoryState.ActiveCharacters.Num();
			++TargetCharacterIndex)
		{
			FGridCharacterInventoryState& FriendlyCharacter =
				Inventory->PartyInventoryState.ActiveCharacters[TargetCharacterIndex];
			if (FriendlyCharacter.Resources.CurrentHealth <= 0)
			{
				continue;
			}
			OutResult.TargetedActionResult.TargetCharacterIndices.AddUnique(TargetCharacterIndex);

			FGridInventoryCharacterSummary FriendlySummary;
			if (!Inventory->GetCharacterSummary(TargetCharacterIndex, FriendlySummary))
			{
				continue;
			}

			for (int32 ResolutionIndex = 0;
				ResolutionIndex < Action.Definition.ResolutionCount && FriendlyCharacter.Resources.CurrentHealth > 0;
				++ResolutionIndex)
			{
				FGridAttackSourceStats FriendlySource;
				FriendlySource.Accuracy = CharacterSummary.DerivedStats.Accuracy;
				FriendlySource.DamageBonus = EffectiveOffensiveProfile.FlatDamageBonus;
				if (EffectiveOffensiveProfile.DamageScalingAttribute != EGridAttackScalingAttribute::None)
				{
					FriendlySource.DamageBonus += URPGCharacterRulesLibrary::GetAttributeModifier(
						ResolveRPG038AttributeValue(CharacterSummary.Attributes, EffectiveOffensiveProfile.DamageScalingAttribute));
				}
				if (bUsesEquippedWeaponAction)
				{
					FriendlySource.RawDamagePercent = Action.Definition.WeaponAttackProfile.WeaponDamagePercent;
				}
				FGridCombatModifierResolver::ApplyDirectDamageSkillScaling(Action.Definition, Character.SkillRanks, FriendlySource);
				FGridQuickItemResolver::ApplyDirectDamageSkillScaling(Action.Definition, Character.SkillRanks, FriendlySource);
				FGridCombatModifierResolver::ApplyOutgoingAttackModifiers(FriendlySource, ResolvedModifiers);
				FGridCombatModifierResolver::ApplyFriendlyDirectDamageModifiers(
					FriendlySource, ResolvedModifiers, TargetCharacterIndex == Action.CharacterIndex);
				if (ResolutionIndex > 0)
				{
					FriendlySource.Accuracy += Action.Definition.SubsequentResolutionAccuracyModifier;
				}

				FGridAttackTargetStats FriendlyTarget;
				FriendlyTarget.Evasion = FriendlySummary.DerivedStats.Evasion;
				FriendlyTarget.CurrentHealth = FriendlyCharacter.Resources.CurrentHealth;
				FriendlyTarget.PhysicalArmor = FriendlyCharacter.Resources.CurrentPhysicalArmor;
				FriendlyTarget.MagicalArmor = FriendlyCharacter.Resources.CurrentMagicalArmor;
				FriendlyTarget.DamageMultiplier = 1.0f;
				const FGridDamageResistanceSet BaseResistances =
					Inventory->ComputeCharacterEquipmentResistances(TargetCharacterIndex);
				FriendlyTarget.ResistancePercent =
					FGridCombatResolver::GetResistancePercent(BaseResistances, EffectiveOffensiveProfile.AttackDefinition.DamageType);

				TArray<FGridCombatModifierProfile> FriendlyProfiles;
				FGridResolvedCombatModifiers FriendlyModifiers;
				if (FGridCombatModifierResolver::CollectCharacterModifiers(FriendlyCharacter, FriendlyProfiles))
				{
					const FGridCombatModifierContext FriendlyModifierContext = bUsesEquippedWeaponAction
						? FGridCombatModifierResolver::MakeResolvedActionAttackContext(
							Action.Definition, Action.SourceDefinitionId, EffectiveOffensiveProfile, EffectiveOffensiveSourceTags)
						: FGridCombatModifierResolver::MakeActionContext(Action.Definition, Action.SourceDefinitionId);
					FGridCombatModifierResolver::Resolve(FriendlyProfiles, FriendlyModifierContext, FriendlyModifiers);
					FGridCombatModifierResolver::ApplyIncomingAttackModifiers(
						FriendlyTarget, EffectiveOffensiveProfile.AttackDefinition.DamageType, FriendlyModifiers);
				}

				FGridAttackResult FriendlyResult =
					FGridCombatResolver::ResolveAttack(FriendlySource, FriendlyTarget, EffectiveOffensiveProfile.AttackDefinition, CombatRandomStream);

				FGridCombatArmorPoolSnapshot FriendlyArmorSnapshot;
				if (BuildRPG033PartyArmorSnapshot(Inventory, TargetCharacterIndex, FriendlyArmorSnapshot))
				{
					FGridCombatArmorEffectResolver::ApplyReferenceModifiers(FriendlyArmorSnapshot, FriendlyModifiers);
					FGridCombatArmorEffectResolver::ApplyAttackDamageEffects(
						Action.Definition.ArmorEffects, FriendlyArmorSnapshot, FriendlyModifiers, FriendlyResult, &ArmorSourceContext);
				}

				if (FriendlyResult.bHit)
				{
					FriendlyCharacter.Resources.CurrentPhysicalArmor =
						FMath::Max(0, FriendlyCharacter.Resources.CurrentPhysicalArmor - FriendlyResult.PhysicalArmorDamage);
					FriendlyCharacter.Resources.CurrentMagicalArmor =
						FMath::Max(0, FriendlyCharacter.Resources.CurrentMagicalArmor - FriendlyResult.MagicalArmorDamage);
					FriendlyCharacter.Resources.CurrentHealth =
						FMath::Max(0, FriendlyCharacter.Resources.CurrentHealth - FriendlyResult.HealthDamage);
				}

				if (!Action.Definition.StatusApplications.IsEmpty() && FriendlyCharacter.Resources.CurrentHealth > 0)
				{
					TArray<FGridCombatStatusApplicationProfile> FriendlyStatusProfiles;
					const int32 FriendlyTargetOrdinal = TargetMonsters.Num() + TargetCharacterIndex;
					for (const FGridCombatStatusApplicationProfile& Profile : Action.Definition.StatusApplications)
					{
						if (FGridCombatTargetingResolver::ShouldApplyStatusApplication(Profile, FriendlyTargetOrdinal))
						{
							FriendlyStatusProfiles.Add(Profile);
						}
					}
					if (!FriendlyStatusProfiles.IsEmpty())
					{
						if (UWorld* World = GetWorld())
						{
							if (UGridStatusEffectLifecycleSubsystem* StatusLifecycle =
									World->GetSubsystem<UGridStatusEffectLifecycleSubsystem>())
							{
								StatusLifecycle->BindToTurnManager(this);
								StatusLifecycle->ApplyCombatStatusApplicationsToPartyCharacter(
									TargetCharacterIndex, FriendlyStatusProfiles, Character.CharacterId, FriendlyTarget, &FriendlyResult);
							}
						}
					}
				}

				Inventory->NotifyPartyInventoryChanged(TargetCharacterIndex);
				if (FGridCombatantInitiativeEntry* FriendlyEntry =
						FindInitiativeEntry(EGridCombatantSide::Party, FriendlyCharacter.CharacterId))
				{
					RefreshInitiativeEntryVitals(*FriendlyEntry);
					OnCombatantStateChanged.Broadcast(*FriendlyEntry);
				}
				if (bCollectRuntimeMetrics)
				{
					++RuntimeMetrics.AttacksResolved;
				}
			}
		}
	}

	if (bSurfaceResolution && IsValid(RuntimeActor))
	{
		for (const FIntPoint& Cell : Preview.AffectedCells)
		{
			for (const FGridCombatSurfaceEffectProfile& SurfaceProfile : Action.Definition.SurfaceEffects)
			{
				RuntimeActor->ApplyCombatSurfaceAtCell(
					Cell.X, Cell.Y, SurfaceProfile, Character.CharacterId, Action.Definition.ActionId, ResolvedModifiers);
			}
		}
	}
	if (bTrapResolution && IsValid(RuntimeActor))
	{
		int32 ResolvedTrapDamage = FMath::Max(0, Action.Definition.TrapEffect.BaseDamage);
		if (Action.Definition.TrapEffect.DamageScalingAttribute != EGridAttackScalingAttribute::None)
		{
			const int32 AttributeValue =
				ResolveRPG038AttributeValue(CharacterSummary.Attributes, Action.Definition.TrapEffect.DamageScalingAttribute);
			ResolvedTrapDamage = FMath::Max(0, ResolvedTrapDamage + URPGCharacterRulesLibrary::GetAttributeModifier(AttributeValue));
		}
		if (!RuntimeActor->ApplyCombatTrapAtCell(Preview.TargetCell.X, Preview.TargetCell.Y, Action.Definition.TrapEffect,
				Character.CharacterId, Action.Definition.ActionId, ResolvedTrapDamage))
		{
			return false;
		}
	}
	bPlayerAttackResolutionInProgress = false;
	EmitCharacterActionResolvedReaction(Action.CharacterIndex, Action, ReactionActionInstanceId, EffectiveOffensiveSourceTags);

	if (bPendingVictoryAfterPlayerAttack)
	{
		bPendingVictoryAfterPlayerAttack = false;
		FinishCombat(EGridCombatPhase::Victory);
	}
	else if (!InitiativeOrder.IsEmpty())
	{
		FGridPlayerCharacterTurnState TurnStateAfter;
		if (GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateAfter) && TurnStateAfter.RemainingActionPoints <= 0 &&
			IsActivePlayerCharacter(Action.CharacterIndex))
		{
			FinishActivePlayerTurn();
		}
	}
	return true;
}

bool UGridTurnManagerComponent::RequestCharacterCombatActionAtCell(int32 CharacterIndex, FName ActionId, EGridCombatActionSourcePolicy SourcePolicy,
	FName SourceDefinitionId, EGridEquipmentSlot SourceEquipmentSlot, FIntPoint TargetCell, FGridCombatActionRequestResult& OutResult)
{
	OutResult = FGridCombatActionRequestResult();
	if (!bInitialized)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::TurnManagerNotInitialized;
		return false;
	}

	FGridCombatActionTargetingPreview Preview;
	if (!BuildCombatActionTargetingPreview(CharacterIndex, ActionId, SourcePolicy, SourceDefinitionId, SourceEquipmentSlot, TargetCell, Preview))
	{
		OutResult.Action = Preview.Action;
		OutResult.RejectReason = Preview.Action.Definition.ActionId.IsNone() ? EGridCombatActionRequestRejectReason::InvalidAction
			: Preview.Action.bEnabled                                        ? EGridCombatActionRequestRejectReason::InvalidTarget
																			 : EGridCombatActionRequestRejectReason::ActionUnavailable;
		return false;
	}

	OutResult.Action = Preview.Action;
	const bool bAccepted =
		Preview.Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
			Preview.Action.Definition.bRelocatePartyToTargetCell
		? RequestCharacterTargetedCellEffect(Preview.Action, Preview, OutResult)
		: RequestCharacterTargetedAttack(Preview.Action, Preview, OutResult);
	OutResult.bAccepted = bAccepted;
	OutResult.RejectReason = bAccepted ? EGridCombatActionRequestRejectReason::None : EGridCombatActionRequestRejectReason::InvalidTarget;
	UE_LOG(LogGridTurnManager, Log,
		TEXT("[GridTargetedAction] Accepted=%s Character=%d Action=%s TargetCell=(%d,%d) AreaCells=%d Targets=%d APCost=%d ManaCost=%d"),
		bAccepted ? TEXT("true") : TEXT("false"), CharacterIndex, *ActionId.ToString(), TargetCell.X, TargetCell.Y, Preview.AffectedCells.Num(),
		Preview.TargetMonsterIds.Num(), Preview.Action.CurrentActionPointCost, Preview.Action.CurrentManaCost);
	return bAccepted;
}

bool UGridTurnManagerComponent::RequestCharacterTargetedCellEffect(
	const FGridAvailableCombatAction& Action, const FGridCombatActionTargetingPreview& Preview, FGridCombatActionRequestResult& OutResult)
{
	UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	if (!IsValid(Inventory) || !IsValid(RuntimeActor) || !Action.bEnabled || !Preview.bValid ||
		!IsMON1285ClassActionSource(Action.Definition.SourcePolicy) ||
		Action.Definition.ResolutionProfile != EGridCombatActionResolutionProfile::Effect ||
		Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::Cell || !Action.Definition.bRelocatePartyToTargetCell ||
		!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Action.CharacterIndex))
	{
		return false;
	}

	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex];
	FGridPlayerCharacterTurnState TurnStateBefore;
	if (!GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateBefore) ||
		!CanCharacterSpendActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost) ||
		Character.Resources.CurrentMana < Action.CurrentManaCost)
	{
		return false;
	}

	UWorld* World = GetWorld();
	UGridMonsterOccupancySubsystem* Occupancy = World ? World->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr;
	const FIntPoint SourceCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
	if (!IsValid(Occupancy) ||
		!FGridCombatMovementResolver::CanRelocatePartyToSelectedCell(
			SourceCell, Preview.TargetCell, Action.Definition.RangeCells, RuntimeActor, Occupancy))
	{
		return false;
	}

	if (!SpendPlayerCharacterActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost))
	{
		return false;
	}
	const int32 ManaBefore = Character.Resources.CurrentMana;
	Character.Resources.CurrentMana = FMath::Max(0, ManaBefore - Action.CurrentManaCost);

	if (!PartyPawn->ApplyAuthorizedGridRelocation(Preview.TargetCell))
	{
		Character.Resources.CurrentMana = ManaBefore;
		if (FGridPlayerCharacterTurnState* RestoredTurnState = EnsurePlayerCharacterTurnState(Action.CharacterIndex))
		{
			*RestoredTurnState = TurnStateBefore;
			BroadcastPlayerCharacterTurnState(*RestoredTurnState);
		}
		return false;
	}

	// Deliberately do not call HandlePartyCellChanged/TryExecuteRelocationAtCell:
	// direct combat relocation must not trigger another teleporter or level transition.
	RuntimeActor->RevealMapAroundCell(Preview.TargetCell.X, Preview.TargetCell.Y);
	StartCombatActionCooldown(Action);
	EmitCharacterActionResolvedReaction(Action.CharacterIndex, Action, FGuid::NewGuid());
	Inventory->NotifyPartyInventoryChanged(Action.CharacterIndex);

	OutResult.TargetedActionResult.TargetCell = Preview.TargetCell;
	OutResult.TargetedActionResult.AffectedCells = { Preview.TargetCell };
	OutResult.ClassActionResult.ManaBefore = ManaBefore;
	OutResult.ClassActionResult.ManaAfter = Character.Resources.CurrentMana;

	FGridPlayerCharacterTurnState TurnStateAfter;
	if (!InitiativeOrder.IsEmpty() && GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateAfter) &&
		TurnStateAfter.RemainingActionPoints <= 0 && IsActivePlayerCharacter(Action.CharacterIndex))
	{
		FinishActivePlayerTurn();
	}
	return true;
}

bool UGridTurnManagerComponent::RequestCharacterHostileEffect(
	const FGridAvailableCombatAction& Action, const FGuid& TargetMonsterId, FGridCombatActionRequestResult& OutResult)
{
	UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	const bool bQuickItem = Action.Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem;
	const bool bClassAction = IsMON1285ClassActionSource(Action.Definition.SourcePolicy);
	AGridMonsterActor* TargetMonster = FindCombatMonsterById(TargetMonsterId);
	if (!IsValid(Inventory) || !Action.bEnabled || (!bQuickItem && !bClassAction) ||
		Action.Definition.ResolutionProfile != EGridCombatActionResolutionProfile::Effect ||
		(Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::Hostile &&
			Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::AllyOrHostile) ||
		!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Action.CharacterIndex) ||
		!IsValid(TargetMonster) || !TargetMonster->bMonsterEnabled || !TargetMonster->IsRuntimeLevelActive() || TargetMonster->IsDead())
	{
		return false;
	}

	FGridCharacterInventoryState& SourceCharacter = Inventory->PartyInventoryState.ActiveCharacters[Action.CharacterIndex];
	if (!SourceCharacter.CharacterId.IsValid() || SourceCharacter.Resources.CurrentHealth <= 0 ||
		!FGridCombatTargetingResolver::IsDirectHostileTargetable(TargetMonster->StatusEffects))
	{
		return false;
	}
	if (!MatchesRPG0391MonsterTargetFilter(Action.Definition.TargetFilter, TargetMonster, SourceCharacter.CharacterId))
	{
		return false;
	}

	const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
	const int32 Distance = FMath::Abs(TargetMonster->CurrentCell.X - PartyCell.X) + FMath::Abs(TargetMonster->CurrentCell.Y - PartyCell.Y);
	if (Distance < 1 || Distance > Action.Definition.RangeCells)
	{
		return false;
	}

	FGridInventoryCharacterSummary SourceSummary;
	FGridPlayerCharacterTurnState TurnStateBefore;
	if (!Inventory->GetCharacterSummary(Action.CharacterIndex, SourceSummary) ||
		!GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateBefore) ||
		!CanCharacterSpendActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost) ||
		SourceCharacter.Resources.CurrentMana < Action.CurrentManaCost)
	{
		return false;
	}

	TArray<FGridCombatModifierProfile> SourceProfiles;
	FGridResolvedCombatModifiers SourceModifiers;
	if (FGridCombatModifierResolver::CollectCharacterModifiers(SourceCharacter, SourceProfiles))
	{
		FGridCombatModifierResolver::Resolve(SourceProfiles,
			FGridCombatModifierResolver::MakeActionContext(Action.Definition, Action.SourceDefinitionId), SourceModifiers);
	}

	const URPGSkillAsset* SkillCheckDefinition = nullptr;
	int32 SkillCheckDifficulty = 0;
	if (Action.Definition.SkillCheck.IsEnabled())
	{
		SkillCheckDefinition =
			FRPGSkillRequirementProjectionService::ResolveDefinitionBySkillId(Action.Definition.SkillCheck.SkillId);
		SkillCheckDifficulty = Action.Definition.SkillCheck.bUseTargetDifficulty && IsValid(TargetMonster->MonsterDefinition)
			? TargetMonster->MonsterDefinition->SkillCheckDifficulty
			: Action.Definition.SkillCheck.FixedDifficulty;
		if (!IsValid(SkillCheckDefinition) || SkillCheckDifficulty <= 0)
		{
			return false;
		}
	}

	FGridCombatArmorPoolSnapshot ArmorSnapshot;
	ArmorSnapshot.CurrentPhysicalArmor = FMath::Max(0, TargetMonster->CurrentPhysicalArmor);
	ArmorSnapshot.CurrentMagicalArmor = FMath::Max(0, TargetMonster->CurrentMagicalArmor);
	ArmorSnapshot.ReferencePhysicalArmor = IsValid(TargetMonster->MonsterDefinition)
		? FMath::Max(0, TargetMonster->MonsterDefinition->PhysicalArmor)
		: ArmorSnapshot.CurrentPhysicalArmor;
	ArmorSnapshot.ReferenceMagicalArmor = IsValid(TargetMonster->MonsterDefinition)
		? FMath::Max(0, TargetMonster->MonsterDefinition->MagicalArmor)
		: ArmorSnapshot.CurrentMagicalArmor;

	TArray<FGridCombatModifierProfile> TargetProfiles;
	FGridResolvedCombatModifiers TargetModifiers;
	if (FGridCombatModifierResolver::CollectStatusModifiers(TargetMonster->StatusEffects, TargetProfiles))
	{
		FGridCombatModifierResolver::Resolve(TargetProfiles,
			FGridCombatModifierResolver::MakeActionContext(Action.Definition, Action.SourceDefinitionId), TargetModifiers);
		FGridCombatArmorEffectResolver::ApplyReferenceModifiers(ArmorSnapshot, TargetModifiers);
	}

	const FGridCombatArmorEffectSourceContext ArmorSourceContext =
		FGridCombatArmorEffectResolver::MakeSourceContext(SourceCharacter, SourceSummary.Attributes);
	FGridAttackTargetStats TargetBefore;
	TargetBefore.CurrentHealth = TargetMonster->CurrentHealth;
	TargetBefore.PhysicalArmor = ArmorSnapshot.CurrentPhysicalArmor;
	TargetBefore.MagicalArmor = ArmorSnapshot.CurrentMagicalArmor;

	const bool bArmorWouldMutate = FGridCombatArmorEffectResolver::WouldAnyDirectDamage(
		Action.Definition.ArmorEffects, ArmorSnapshot, TargetModifiers, &ArmorSourceContext);
	const bool bStatusWouldMutate = FGridCombatStatusApplicationResolver::WouldAnyMutate(
		Action.Definition.StatusApplications, SourceCharacter.CharacterId, TargetBefore, nullptr, TargetMonster->StatusEffects);
	TArray<FName> RemovableIds;
	FGridCombatTargetingResolver::CollectStatusRemovalIds(
		TargetMonster->StatusEffects, Action.Definition.StatusRemovals, RemovableIds, EGridCombatStatusRemovalTargetSide::Hostile);
	if (!bArmorWouldMutate && !bStatusWouldMutate && RemovableIds.IsEmpty())
	{
		return false;
	}

	const int32 QuantityBefore =
		bQuickItem ? Inventory->CountItemDefinitionInCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId) : 0;
	if (bQuickItem &&
		(Action.SourceDefinitionId.IsNone() || Action.CurrentSourceItemQuantityCost <= 0 ||
			QuantityBefore < Action.CurrentSourceItemQuantityCost))
	{
		return false;
	}
	if (!SpendPlayerCharacterActionPoints(Action.CharacterIndex, Action.CurrentActionPointCost))
	{
		return false;
	}

	const int32 ManaBefore = SourceCharacter.Resources.CurrentMana;
	SourceCharacter.Resources.CurrentMana = FMath::Max(0, ManaBefore - Action.CurrentManaCost);
	if (bQuickItem &&
		!Inventory->RemoveItemDefinitionFromCharacterInventory(
			Action.CharacterIndex, Action.SourceDefinitionId, Action.CurrentSourceItemQuantityCost))
	{
		SourceCharacter.Resources.CurrentMana = ManaBefore;
		if (FGridPlayerCharacterTurnState* RestoredTurnState = EnsurePlayerCharacterTurnState(Action.CharacterIndex))
		{
			*RestoredTurnState = TurnStateBefore;
			BroadcastPlayerCharacterTurnState(*RestoredTurnState);
		}
		return false;
	}

	bool bSkillCheckSucceeded = true;
	FRPGSkillCheckResult SkillCheckResult;
	if (Action.Definition.SkillCheck.IsEnabled())
	{
		if (!FRPGSkillCheckService::TryResolveSkillCheck(
				SourceCharacter, SkillCheckDefinition, SkillCheckDifficulty, CombatRandomStream, SkillCheckResult))
		{
			return false;
		}
		bSkillCheckSucceeded = SkillCheckResult.bSuccess;
		UE_LOG(LogGridTurnManager, Log,
			TEXT("[RPG03.9.2] SkillCheck Action=%s Skill=%s Roll=%d Rank=%d Attribute=%d Progression=%d Total=%d DC=%d Success=%s SafeFailure=%s"),
			*Action.Definition.ActionId.ToString(), *SkillCheckResult.SkillId.ToString(), SkillCheckResult.Roll, SkillCheckResult.Rank,
			SkillCheckResult.AttributeModifier, SkillCheckResult.ProgressionModifier, SkillCheckResult.Total, SkillCheckResult.Difficulty,
			bSkillCheckSucceeded ? TEXT("true") : TEXT("false"), SkillCheckResult.bSafeFailure ? TEXT("true") : TEXT("false"));
	}

	if (bSkillCheckSucceeded && !Action.Definition.ArmorEffects.IsEmpty())
	{
		FGridCombatArmorEffectResolver::ApplyDirectDamageEffects(
			Action.Definition.ArmorEffects, ArmorSnapshot, TargetModifiers, &ArmorSourceContext);
		TargetMonster->SetCurrentPhysicalArmor(ArmorSnapshot.CurrentPhysicalArmor);
		TargetMonster->SetCurrentMagicalArmor(ArmorSnapshot.CurrentMagicalArmor);
		TargetBefore.PhysicalArmor = ArmorSnapshot.CurrentPhysicalArmor;
		TargetBefore.MagicalArmor = ArmorSnapshot.CurrentMagicalArmor;
	}

	UGridStatusEffectLifecycleSubsystem* StatusLifecycle =
		GetWorld() ? GetWorld()->GetSubsystem<UGridStatusEffectLifecycleSubsystem>() : nullptr;
	if (StatusLifecycle && bSkillCheckSucceeded)
	{
		StatusLifecycle->BindToTurnManager(this);
		if (!Action.Definition.StatusApplications.IsEmpty())
		{
			StatusLifecycle->ApplyCombatStatusApplicationsToMonster(
				TargetMonster, Action.Definition.StatusApplications, SourceCharacter.CharacterId, TargetBefore, nullptr);
		}
		if (!Action.Definition.StatusRemovals.IsEmpty())
		{
			StatusLifecycle->RemoveCombatStatusEffectsFromMonster(TargetMonster, Action.Definition.StatusRemovals);
		}
	}

	StartCombatActionCooldown(Action);
	EmitCharacterActionResolvedReaction(Action.CharacterIndex, Action, FGuid::NewGuid());
	Inventory->NotifyPartyInventoryChanged(Action.CharacterIndex);
	if (FGridCombatantInitiativeEntry* TargetEntry = FindInitiativeEntry(EGridCombatantSide::Monster, TargetMonsterId))
	{
		RefreshInitiativeEntryVitals(*TargetEntry);
		OnCombatantStateChanged.Broadcast(*TargetEntry);
	}

	OutResult.TargetedActionResult.TargetMonsterIds.Add(TargetMonsterId);
	OutResult.TargetedActionResult.TargetCell = TargetMonster->CurrentCell;
	OutResult.TargetedActionResult.AffectedCells.Add(TargetMonster->CurrentCell);
	if (bQuickItem)
	{
		OutResult.QuickItemResult.SourceQuantityBefore = QuantityBefore;
		OutResult.QuickItemResult.SourceQuantityAfter =
			Inventory->CountItemDefinitionInCharacterInventory(Action.CharacterIndex, Action.SourceDefinitionId);
		OutResult.QuickItemResult.ManaBefore = ManaBefore;
		OutResult.QuickItemResult.ManaAfter = SourceCharacter.Resources.CurrentMana;
	}
	else
	{
		OutResult.ClassActionResult.ManaBefore = ManaBefore;
		OutResult.ClassActionResult.ManaAfter = SourceCharacter.Resources.CurrentMana;
	}

	FGridPlayerCharacterTurnState TurnStateAfter;
	if (!InitiativeOrder.IsEmpty() && GetPlayerCharacterTurnState(Action.CharacterIndex, TurnStateAfter) &&
		TurnStateAfter.RemainingActionPoints <= 0 && IsActivePlayerCharacter(Action.CharacterIndex))
	{
		FinishActivePlayerTurn();
	}
	return true;
}

bool UGridTurnManagerComponent::RequestCharacterCombatActionOnPartyTargets(int32 CharacterIndex, FName ActionId,
	EGridCombatActionSourcePolicy SourcePolicy, FName SourceDefinitionId, EGridEquipmentSlot SourceEquipmentSlot,
	const TArray<int32>& TargetCharacterIndices, FGridCombatActionRequestResult& OutResult)
{
	OutResult = FGridCombatActionRequestResult();
	if (!bInitialized)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::TurnManagerNotInitialized;
		return false;
	}

	TArray<FGridAvailableCombatAction> AvailableActions;
	GetAvailableCombatActions(CharacterIndex, AvailableActions);
	const FGridAvailableCombatAction* Action = AvailableActions.FindByPredicate(
		[ActionId, SourcePolicy, SourceDefinitionId, SourceEquipmentSlot](const FGridAvailableCombatAction& Candidate)
		{
			return Candidate.MatchesSource(ActionId, SourcePolicy, SourceDefinitionId, SourceEquipmentSlot);
		});
	if (!Action)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidAction;
		return false;
	}
	OutResult.Action = *Action;
	if (!Action->bEnabled)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::ActionUnavailable;
		return false;
	}
	if ((Action->Definition.TargetingPolicy != EGridCombatTargetingPolicy::Ally &&
			Action->Definition.TargetingPolicy != EGridCombatTargetingPolicy::AllyOrHostile) ||
		Action->Definition.ResolutionProfile != EGridCombatActionResolutionProfile::Effect)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidTarget;
		return false;
	}

	const bool bAccepted = RequestCharacterBatchPartyEffect(*Action, TargetCharacterIndices, OutResult);
	OutResult.bAccepted = bAccepted;
	OutResult.RejectReason = bAccepted ? EGridCombatActionRequestRejectReason::None
		: Action->Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem
		? EGridCombatActionRequestRejectReason::QuickItemRejected
		: EGridCombatActionRequestRejectReason::ClassActionRejected;
	return bAccepted;
}

bool UGridTurnManagerComponent::RequestCharacterCombatActionOnMonsterTarget(int32 CharacterIndex, FName ActionId,
	EGridCombatActionSourcePolicy SourcePolicy, FName SourceDefinitionId, EGridEquipmentSlot SourceEquipmentSlot,
	FGuid TargetMonsterId, FGridCombatActionRequestResult& OutResult)
{
	OutResult = FGridCombatActionRequestResult();
	if (!bInitialized)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::TurnManagerNotInitialized;
		return false;
	}

	TArray<FGridAvailableCombatAction> AvailableActions;
	GetAvailableCombatActions(CharacterIndex, AvailableActions);
	const FGridAvailableCombatAction* Action = AvailableActions.FindByPredicate(
		[ActionId, SourcePolicy, SourceDefinitionId, SourceEquipmentSlot](const FGridAvailableCombatAction& Candidate)
		{
			return Candidate.MatchesSource(ActionId, SourcePolicy, SourceDefinitionId, SourceEquipmentSlot);
		});
	if (!Action)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidAction;
		return false;
	}
	OutResult.Action = *Action;
	if (!Action->bEnabled)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::ActionUnavailable;
		return false;
	}
	if ((Action->Definition.TargetingPolicy != EGridCombatTargetingPolicy::Hostile &&
			Action->Definition.TargetingPolicy != EGridCombatTargetingPolicy::AllyOrHostile) ||
		Action->Definition.ResolutionProfile != EGridCombatActionResolutionProfile::Effect || !TargetMonsterId.IsValid())
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidTarget;
		return false;
	}

	const bool bAccepted = RequestCharacterHostileEffect(*Action, TargetMonsterId, OutResult);
	OutResult.bAccepted = bAccepted;
	OutResult.RejectReason = bAccepted ? EGridCombatActionRequestRejectReason::None
		: Action->Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem
		? EGridCombatActionRequestRejectReason::QuickItemRejected
		: EGridCombatActionRequestRejectReason::ClassActionRejected;
	return bAccepted;
}

bool UGridTurnManagerComponent::RequestCharacterCombatAction(int32 CharacterIndex, FName ActionId, EGridCombatActionSourcePolicy SourcePolicy,
	FName SourceDefinitionId, EGridEquipmentSlot SourceEquipmentSlot, FGridCombatActionRequestResult& OutResult)
{
	OutResult = FGridCombatActionRequestResult();
	if (!bInitialized)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::TurnManagerNotInitialized;
		return false;
	}

	TArray<FGridAvailableCombatAction> AvailableActions;
	GetAvailableCombatActions(CharacterIndex, AvailableActions);
	const FGridAvailableCombatAction* Action = AvailableActions.FindByPredicate(
		[ActionId, SourcePolicy, SourceDefinitionId, SourceEquipmentSlot](const FGridAvailableCombatAction& Candidate)
		{
			return Candidate.MatchesSource(ActionId, SourcePolicy, SourceDefinitionId, SourceEquipmentSlot);
		});
	if (!Action)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidAction;
		UE_LOG(LogGridTurnManager, Log, TEXT("[GridActionCatalog] Accepted=false Character=%d Action=%s Source=%s Slot=%s Reason=InvalidAction"),
			CharacterIndex, *ActionId.ToString(), *SourceDefinitionId.ToString(), *UEnum::GetValueAsString(SourceEquipmentSlot));
		return false;
	}

	OutResult.Action = *Action;
	if (!Action->bEnabled)
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::ActionUnavailable;
		UE_LOG(LogGridTurnManager, Log, TEXT("[GridActionCatalog] Accepted=false Character=%d Action=%s Source=%s Slot=%s Reason=%s"), CharacterIndex,
			*ActionId.ToString(), *SourceDefinitionId.ToString(), *UEnum::GetValueAsString(SourceEquipmentSlot), *GetMON126AvailabilityText(*Action));
		return false;
	}

	const bool bLegacySpellbookAlly =
		Action->Definition.TargetingPolicy == EGridCombatTargetingPolicy::Ally &&
		Action->Definition.SourcePolicy == EGridCombatActionSourcePolicy::Spell &&
		Action->SourceDefinitionId == Action->Definition.ActionId;
	if (IsMON1286ExplicitTargetingPolicy(Action->Definition.TargetingPolicy) ||
		Action->Definition.TargetingPolicy == EGridCombatTargetingPolicy::AllyOrHostile ||
		(Action->Definition.TargetingPolicy == EGridCombatTargetingPolicy::Ally && !bLegacySpellbookAlly))
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::TargetRequired;
		UE_LOG(LogGridTurnManager, Log, TEXT("[GridActionCatalog] Accepted=false Character=%d Action=%s Source=%s Slot=%s Reason=TargetRequired"),
			CharacterIndex, *ActionId.ToString(), *SourceDefinitionId.ToString(), *UEnum::GetValueAsString(SourceEquipmentSlot));
		return false;
	}

	if ((Action->Definition.TargetingPolicy == EGridCombatTargetingPolicy::Party ||
			Action->Definition.TargetingPolicy == EGridCombatTargetingPolicy::FrontRowParty) &&
		Action->Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect)
	{
		const TArray<int32> NoExplicitTargets;
		const bool bAccepted = RequestCharacterBatchPartyEffect(*Action, NoExplicitTargets, OutResult);
		OutResult.bAccepted = bAccepted;
		OutResult.RejectReason = bAccepted ? EGridCombatActionRequestRejectReason::None
			: Action->Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem
			? EGridCombatActionRequestRejectReason::QuickItemRejected
			: EGridCombatActionRequestRejectReason::ClassActionRejected;
		return bAccepted;
	}

	if (Action->Definition.TargetingPolicy == EGridCombatTargetingPolicy::Hostile &&
		Action->Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect)
	{
		if (!Action->SuggestedTargetId.IsValid())
		{
			OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidTarget;
			return false;
		}
		const bool bAccepted = RequestCharacterHostileEffect(*Action, Action->SuggestedTargetId, OutResult);
		OutResult.bAccepted = bAccepted;
		OutResult.RejectReason = bAccepted ? EGridCombatActionRequestRejectReason::None
			: Action->Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem
			? EGridCombatActionRequestRejectReason::QuickItemRejected
			: EGridCombatActionRequestRejectReason::ClassActionRejected;
		return bAccepted;
	}

	UGridPartyInventoryComponent* SpellInventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	const FGridCharacterInventoryState* SpellCharacter =
		IsValid(SpellInventory) && SpellInventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex)
		? &SpellInventory->PartyInventoryState.ActiveCharacters[CharacterIndex]
		: nullptr;
	const bool bSpellbookBackedAction = Action->Definition.SourcePolicy == EGridCombatActionSourcePolicy::Spell &&
		Action->SourceDefinitionId == Action->Definition.ActionId && SpellCharacter && SpellCharacter->KnownSpellIds.Contains(Action->Definition.ActionId);

	if (bSpellbookBackedAction)
	{
		FGridSpellDefinition SpellDefinition;
		FGridCharacterSpellbookState CharacterSpellbook;
		CharacterSpellbook.CharacterId = SpellCharacter->CharacterId;
		CharacterSpellbook.KnownSpellIds = SpellCharacter->KnownSpellIds;
		FGridPlayerCharacterTurnState TurnStateBefore;
		if (!SpellCharacter || !TryBuildUI0143e2ProductionSpell(Action->Definition.ActionId, SpellDefinition) ||
			!GetPlayerCharacterTurnState(CharacterIndex, TurnStateBefore))
		{
			OutResult.RejectReason = EGridCombatActionRequestRejectReason::ClassActionRejected;
			return false;
		}

		FGridSpellCastRequest CastRequest;
		CastRequest.CasterCharacterId = SpellCharacter->CharacterId;
		CastRequest.SpellId = SpellDefinition.SpellId;

		FGridSpellTargetingContext TargetingContext;
		TargetingContext.CasterCell = FIntPoint(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
		TargetingContext.bLineOfSightClear = true;

		AGridMonsterActor* TargetMonster = nullptr;
		if (SpellDefinition.TargetingPolicy == EGridCombatTargetingPolicy::Self || SpellDefinition.TargetingPolicy == EGridCombatTargetingPolicy::Ally)
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
		else if (SpellDefinition.TargetingPolicy == EGridCombatTargetingPolicy::FirstAxialTarget)
		{
			if (!Action->SuggestedTargetId.IsValid())
			{
				OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidTarget;
				UE_LOG(LogGridTurnManager, Log, TEXT("[GridSpellAction] Accepted=false Character=%d Spell=%s Reason=NoSuggestedHostileTarget"), CharacterIndex,
					*SpellDefinition.SpellId.ToString());
				return false;
			}
			TargetMonster = FindCombatMonsterById(Action->SuggestedTargetId);
			if (!IsValid(TargetMonster) || !TargetMonster->bMonsterEnabled || !TargetMonster->IsRuntimeLevelActive() || TargetMonster->IsDead() ||
				!FGridCombatTargetingResolver::IsDirectHostileTargetable(TargetMonster->StatusEffects) ||
				!MatchesRPG0391MonsterTargetFilter(Action->Definition.TargetFilter, TargetMonster, Action->CharacterId))
			{
				OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidTarget;
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
			OutResult.RejectReason = EGridCombatActionRequestRejectReason::UnsupportedResolution;
			return false;
		}

		if (SpellDefinition.bRequiresLineOfSight && TargetingContext.bHasResolvedTargetCell && IsValid(RuntimeActor))
		{
			TargetingContext.bLineOfSightClear =
				!RuntimeActor->DoesCombatSmokeBlockLine(TargetingContext.CasterCell, TargetingContext.ResolvedTargetCell);
		}

		const int32 TargetMaxHealth = TargetMonster ? IsValid(TargetMonster->MonsterDefinition) ? FMath::Max(1, TargetMonster->MonsterDefinition->MaxHealth)
																								: FMath::Max(1, TargetMonster->CurrentHealth)
													: FMath::Max(1, SpellCharacter->DerivedStats.MaxHealth);
		const int32 TargetCurrentHealth = TargetMonster ? TargetMonster->CurrentHealth : SpellCharacter->Resources.CurrentHealth;
		const FGridStatusEffectCollection TargetStatusEffects = TargetMonster ? TargetMonster->StatusEffects : SpellCharacter->StatusEffects;

		FGridResolvedCombatModifiers SpellModifiers;
		TArray<FGridCombatModifierProfile> SpellModifierProfiles;
		if (FGridCombatModifierResolver::CollectCharacterModifiers(*SpellCharacter, SpellModifierProfiles))
		{
			FGridCombatModifierContext SpellContext =
				FGridCombatModifierResolver::MakeActionContext(Action->Definition, Action->SourceDefinitionId);
			if (TargetMonster && IsValid(TargetMonster->MonsterDefinition))
			{
				FGridCombatModifierResolver::AddTargetStatusContext(
					SpellContext, TargetMonster->StatusEffects, SpellCharacter->CharacterId,
					TargetMonster->MonsterDefinition->CategoryId, TargetMonster->MonsterDefinition->SemanticTags);
			}
			FGridCombatModifierResolver::Resolve(SpellModifierProfiles, SpellContext, SpellModifiers);
		}

		FGridSpellHotbarExecutionResult Execution;
		const bool bExecuted = FGridSpellHotbarExecutionService::TryExecute(
			SpellDefinition, CastRequest, TargetingContext, CharacterSpellbook, SpellCharacter->Resources, TurnStateBefore, TargetMaxHealth,
			TargetCurrentHealth, TargetStatusEffects,
			[](FName EffectId) -> const UGridStatusEffectDefinitionAsset*
			{
				return FGridCombatStatusApplicationResolver::ResolveDefinition(EffectId);
			},
			Execution, SpellModifiers.OutgoingDamagePercentModifier);
		if (!bExecuted)
		{
			OutResult.RejectReason = Execution.PipelineRejectStage == EGridSpellCastPipelineRejectStage::Targeting
				? EGridCombatActionRequestRejectReason::InvalidTarget
				: EGridCombatActionRequestRejectReason::ClassActionRejected;
			UE_LOG(LogGridTurnManager, Log,
				TEXT("[GridSpellAction] Accepted=false Character=%d Spell=%s PipelineStage=%s Targeting=%s Transaction=%s Effect=%s Error=%s"), CharacterIndex,
				*SpellDefinition.SpellId.ToString(), *UEnum::GetValueAsString(Execution.PipelineRejectStage),
				*UEnum::GetValueAsString(Execution.TargetingRejectReason), *UEnum::GetValueAsString(Execution.TransactionRejectReason),
				*UEnum::GetValueAsString(Execution.EffectRejectReason), *Execution.Error);
			return false;
		}

		FGridPlayerCharacterTurnState* AuthoritativeTurnState = FindPlayerCharacterTurnState(SpellCharacter->CharacterId);
		if (!AuthoritativeTurnState || !SpellInventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
		{
			OutResult.RejectReason = EGridCombatActionRequestRejectReason::ClassActionRejected;
			return false;
		}

		FGridCharacterInventoryState& MutableCharacter = SpellInventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
		OutResult.ClassActionResult.HealthBefore = MutableCharacter.Resources.CurrentHealth;
		OutResult.ClassActionResult.ManaBefore = MutableCharacter.Resources.CurrentMana;

		MutableCharacter.Resources = Execution.CasterResources;
		*AuthoritativeTurnState = Execution.CasterTurnState;
		if (!TargetMonster)
		{
			MutableCharacter.Resources.CurrentHealth = Execution.TargetCurrentHealth;
			MutableCharacter.StatusEffects = Execution.TargetStatusEffects;
		}
		BroadcastPlayerCharacterTurnState(*AuthoritativeTurnState);

		if (TargetMonster)
		{
			TargetMonster->StatusEffects = Execution.TargetStatusEffects;
			const bool bPreviousResolutionInProgress = bPlayerAttackResolutionInProgress;
			bPlayerAttackResolutionInProgress = true;
			TargetMonster->SetCurrentHealth(Execution.TargetCurrentHealth);
			bPlayerAttackResolutionInProgress = bPreviousResolutionInProgress;

			OutResult.TargetedActionResult.TargetCell = Execution.ResolvedTarget.GridCell;
			OutResult.TargetedActionResult.AffectedCells.Add(Execution.ResolvedTarget.GridCell);
			OutResult.TargetedActionResult.TargetMonsterIds.Add(TargetMonster->ResolvePersistenceId());
			if (!TargetMonster->IsDead())
			{
				if (FGridCombatantInitiativeEntry* TargetEntry = FindInitiativeEntry(EGridCombatantSide::Monster, TargetMonster->ResolvePersistenceId()))
				{
					RefreshInitiativeEntryVitals(*TargetEntry);
					OnCombatantStateChanged.Broadcast(*TargetEntry);
				}
			}
		}

		StartCombatActionCooldown(*Action);
		EmitCharacterActionResolvedReaction(CharacterIndex, *Action, FGuid::NewGuid());
		SpellInventory->NotifyPartyInventoryChanged(CharacterIndex);
		if (FGridCombatantInitiativeEntry* Entry = FindInitiativeEntry(EGridCombatantSide::Party, MutableCharacter.CharacterId))
		{
			RefreshInitiativeEntryVitals(*Entry);
			OnCombatantStateChanged.Broadcast(*Entry);
		}
		if (UWorld* World = GetWorld())
		{
			if (UGridStatusEffectLifecycleSubsystem* StatusLifecycle = World->GetSubsystem<UGridStatusEffectLifecycleSubsystem>())
			{
				StatusLifecycle->RefreshAllInitiativeModifiers();
			}
		}

		const FVector SourceWorldLocation = PartyPawn->GetActorLocation();
		const FVector TargetWorldLocation = TargetMonster ? TargetMonster->GetActorLocation() : SourceWorldLocation;
		FGridSpellPresentationProfile PresentationProfile;
		if (FGridProductionSpellLibrary::TryBuildPresentationProfile(SpellDefinition.SpellId, PresentationProfile))
		{
			FGridSpellPresentationPlan PresentationPlan;
			if (FGridSpellPresentationService::BuildPlan(
					SpellDefinition, Execution.ResolvedTarget, PresentationProfile, SourceWorldLocation, TargetWorldLocation, PresentationPlan))
			{
				UGridSpellPresentationComponent* PresentationComponent = PartyPawn->FindComponentByClass<UGridSpellPresentationComponent>();
				if (!IsValid(PresentationComponent))
				{
					PresentationComponent = NewObject<UGridSpellPresentationComponent>(PartyPawn, TEXT("GridSpellPresentationRuntime"));
					if (IsValid(PresentationComponent))
					{
						PartyPawn->AddInstanceComponent(PresentationComponent);
						PresentationComponent->RegisterComponent();
					}
				}
				if (IsValid(PresentationComponent))
				{
					PresentationComponent->PresentSpell(PresentationPlan, PresentationProfile);
				}
			}
		}

		OutResult.ClassActionResult.HealthAfter = MutableCharacter.Resources.CurrentHealth;
		OutResult.ClassActionResult.ManaAfter = MutableCharacter.Resources.CurrentMana;
		OutResult.bAccepted = true;
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::None;

		UE_LOG(LogGridTurnManager, Log,
			TEXT("[GridSpellAction] Accepted=true Character=%d Spell=%s Target=%s AP=%d Mana=%d Health=%d->%d ManaState=%d->%d Damage=%d Healing=%d"),
			CharacterIndex, *SpellDefinition.SpellId.ToString(), *Execution.ResolvedTarget.TargetId.ToString(EGuidFormats::Digits),
			Execution.CostReceipt.ActionPointsSpent, Execution.CostReceipt.ManaSpent, OutResult.ClassActionResult.HealthBefore,
			OutResult.ClassActionResult.HealthAfter, OutResult.ClassActionResult.ManaBefore, OutResult.ClassActionResult.ManaAfter,
			Execution.EffectResult.TotalDamage, Execution.EffectResult.TotalHealing);

		if (bPendingVictoryAfterPlayerAttack)
		{
			bPendingVictoryAfterPlayerAttack = false;
			FinishCombat(EGridCombatPhase::Victory);
		}
		else if (!InitiativeOrder.IsEmpty() && AuthoritativeTurnState->RemainingActionPoints <= 0 && IsActivePlayerCharacter(CharacterIndex))
		{
			FinishActivePlayerTurn();
		}
		return true;
	}

	if (IsMON1285ClassActionSource(Action->Definition.SourcePolicy))
	{
		bool bAccepted = false;
		if (Action->Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack)
		{
			bAccepted = RequestCharacterClassActionAttack(
				*Action, OutResult.AttackRequest, OutResult.AttackResult, OutResult.AttackRejectReason, OutResult.ClassActionResult);
			OutResult.RejectReason = bAccepted                                        ? EGridCombatActionRequestRejectReason::None
				: OutResult.AttackRejectReason != EGridPlayerAttackRejectReason::None ? EGridCombatActionRequestRejectReason::AttackRejected
																					  : EGridCombatActionRequestRejectReason::ClassActionRejected;
		}
		else if (Action->Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect)
		{
			bAccepted = RequestCharacterClassActionEffect(*Action, OutResult.ClassActionResult);
			OutResult.RejectReason = bAccepted ? EGridCombatActionRequestRejectReason::None : EGridCombatActionRequestRejectReason::ClassActionRejected;
		}
		else
		{
			OutResult.RejectReason = EGridCombatActionRequestRejectReason::UnsupportedResolution;
		}

		OutResult.bAccepted = bAccepted;
		UE_LOG(LogGridTurnManager, Log, TEXT("[GridClassAction] Accepted=%s Character=%d Action=%s Source=%s APCost=%d ManaCost=%d Health=%d->%d Mana=%d->%d"),
			bAccepted ? TEXT("true") : TEXT("false"), CharacterIndex, *ActionId.ToString(), *SourceDefinitionId.ToString(), Action->CurrentActionPointCost,
			Action->CurrentManaCost, OutResult.ClassActionResult.HealthBefore, OutResult.ClassActionResult.HealthAfter, OutResult.ClassActionResult.ManaBefore,
			OutResult.ClassActionResult.ManaAfter);
		return bAccepted;
	}

	if (Action->Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem)
	{
		bool bAccepted = false;
		if (Action->Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack)
		{
			UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
			if (!IsValid(Inventory) || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
			{
				OutResult.RejectReason = EGridCombatActionRequestRejectReason::QuickItemRejected;
				return false;
			}
			OutResult.QuickItemResult.SourceQuantityBefore = Inventory->CountItemDefinitionInCharacterInventory(CharacterIndex, Action->SourceDefinitionId);
			const FGridCharacterInventoryState& CharacterBefore = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
			OutResult.QuickItemResult.HealthBefore = CharacterBefore.Resources.CurrentHealth;
			OutResult.QuickItemResult.HealthAfter = CharacterBefore.Resources.CurrentHealth;
			OutResult.QuickItemResult.ManaBefore = CharacterBefore.Resources.CurrentMana;
			OutResult.QuickItemResult.ManaAfter = OutResult.QuickItemResult.ManaBefore;

			FGridPlayerAttackRequest AttackRequest;
			FGridAttackResult AttackResult;
			EGridPlayerAttackRejectReason AttackRejectReason = EGridPlayerAttackRejectReason::None;
			bAccepted =
				RequestCharacterAttackInternal(CharacterIndex, EGridEquipmentSlot::None, false, Action, AttackRequest, AttackResult, AttackRejectReason);
			OutResult.AttackRequest = AttackRequest;
			OutResult.AttackResult = AttackResult;
			OutResult.AttackRejectReason = AttackRejectReason;
			if (bAccepted)
			{
				const bool bCharacterStillAvailable = IsValid(Inventory) && Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex);
				if (bCharacterStillAvailable)
				{
					const FGridCharacterInventoryState& CharacterAfter = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
					OutResult.QuickItemResult.SourceQuantityAfter =
						Inventory->CountItemDefinitionInCharacterInventory(CharacterIndex, Action->SourceDefinitionId);
					OutResult.QuickItemResult.HealthAfter = CharacterAfter.Resources.CurrentHealth;
					OutResult.QuickItemResult.ManaAfter = CharacterAfter.Resources.CurrentMana;
				}
			}
			else
			{
				OutResult.QuickItemResult.SourceQuantityAfter = OutResult.QuickItemResult.SourceQuantityBefore;
				OutResult.QuickItemResult.ManaAfter = OutResult.QuickItemResult.ManaBefore;
				UE_LOG(LogGridTurnManager, Verbose, TEXT("[GridQuickItem] AttackRejectedWithoutCommit Character=%d Action=%s Source=%s"), CharacterIndex,
					*ActionId.ToString(), *SourceDefinitionId.ToString());
			}
			OutResult.RejectReason = bAccepted                              ? EGridCombatActionRequestRejectReason::None
				: AttackRejectReason != EGridPlayerAttackRejectReason::None ? EGridCombatActionRequestRejectReason::AttackRejected
																			: EGridCombatActionRequestRejectReason::QuickItemRejected;
		}
		else if (Action->Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect)
		{
			bAccepted = RequestCharacterQuickItemEffect(*Action, OutResult.QuickItemResult);
			OutResult.RejectReason = bAccepted ? EGridCombatActionRequestRejectReason::None : EGridCombatActionRequestRejectReason::QuickItemRejected;
		}
		else
		{
			OutResult.RejectReason = EGridCombatActionRequestRejectReason::UnsupportedResolution;
		}

		OutResult.bAccepted = bAccepted;
		UE_LOG(LogGridTurnManager, Log,
			TEXT("[GridQuickItem] Accepted=%s Character=%d Action=%s Source=%s APCost=%d ManaCost=%d Quantity=%d->%d Health=%d->%d Mana=%d->%d"),
			bAccepted ? TEXT("true") : TEXT("false"), CharacterIndex, *ActionId.ToString(), *SourceDefinitionId.ToString(), Action->CurrentActionPointCost,
			Action->CurrentManaCost, OutResult.QuickItemResult.SourceQuantityBefore, OutResult.QuickItemResult.SourceQuantityAfter,
			OutResult.QuickItemResult.HealthBefore, OutResult.QuickItemResult.HealthAfter, OutResult.QuickItemResult.ManaBefore,
			OutResult.QuickItemResult.ManaAfter);
		return bAccepted;
	}

	if (Action->Definition.ResolutionProfile != EGridCombatActionResolutionProfile::Attack ||
		(Action->Definition.SourcePolicy != EGridCombatActionSourcePolicy::Equipment &&
			Action->Definition.SourcePolicy != EGridCombatActionSourcePolicy::Universal))
	{
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::UnsupportedResolution;
		return false;
	}

	FGridPlayerAttackRequest AttackRequest;
	FGridAttackResult AttackResult;
	EGridPlayerAttackRejectReason AttackRejectReason = EGridPlayerAttackRejectReason::None;
	const bool bAccepted = RequestCharacterAttackInternal(CharacterIndex, Action->SourceEquipmentSlot,
		Action->Definition.SourcePolicy == EGridCombatActionSourcePolicy::Equipment, Action, AttackRequest, AttackResult, AttackRejectReason);
	OutResult.bAccepted = bAccepted;
	OutResult.AttackRequest = AttackRequest;
	OutResult.AttackResult = AttackResult;
	OutResult.AttackRejectReason = AttackRejectReason;
	OutResult.RejectReason = bAccepted ? EGridCombatActionRequestRejectReason::None : EGridCombatActionRequestRejectReason::AttackRejected;
	UE_LOG(LogGridTurnManager, Log, TEXT("[GridActionCatalog] Accepted=%s Character=%d Action=%s Source=%s Slot=%s APCost=%d ManaCost=%d"),
		bAccepted ? TEXT("true") : TEXT("false"), CharacterIndex, *ActionId.ToString(), *SourceDefinitionId.ToString(),
		*UEnum::GetValueAsString(SourceEquipmentSlot), Action->CurrentActionPointCost, Action->CurrentManaCost);
	return bAccepted;
}

void UGridTurnManagerComponent::LogAvailableCombatActions(int32 CharacterIndex) const
{
	const int32 ResolvedCharacterIndex = CharacterIndex == INDEX_NONE && IsValid(PartyPawn) && IsValid(PartyPawn->PartyInventoryComponent)
		? PartyPawn->PartyInventoryComponent->GetSelectedCharacterIndex()
		: CharacterIndex;
	TArray<FGridAvailableCombatAction> Actions;
	GetAvailableCombatActions(ResolvedCharacterIndex, Actions);
	UE_LOG(LogGridTurnManager, Log, TEXT("[GridActionCatalog] Character=%d Count=%d"), ResolvedCharacterIndex, Actions.Num());
	for (const FGridAvailableCombatAction& Action : Actions)
	{
		UE_LOG(LogGridTurnManager, Log,
			TEXT("[GridActionCatalog] Character=%d Action=%s SourcePolicy=%s Source=%s Slot=%s AP=%d Mana=%d Item=%d/%d Status=%s Target=%s Cell=(%d,%d)"),
			ResolvedCharacterIndex, *Action.Definition.ActionId.ToString(), *UEnum::GetValueAsString(Action.Definition.SourcePolicy),
			*Action.SourceDefinitionId.ToString(), *UEnum::GetValueAsString(Action.SourceEquipmentSlot), Action.CurrentActionPointCost, Action.CurrentManaCost,
			Action.CurrentSourceItemQuantityCost, Action.CurrentSourceItemQuantity, *GetMON126AvailabilityText(Action),
			*Action.SuggestedTargetId.ToString(EGuidFormats::Digits), Action.SuggestedTargetCell.X, Action.SuggestedTargetCell.Y);
	}
}

void UGridTurnManagerComponent::LogSelectedCharacterAvailableCombatActions() const
{
	LogAvailableCombatActions(INDEX_NONE);
}
