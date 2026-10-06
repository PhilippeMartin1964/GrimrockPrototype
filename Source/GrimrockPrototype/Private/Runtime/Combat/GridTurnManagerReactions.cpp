#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "RPG/StatusEffects/GridCombatStatusApplicationResolver.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"

void UGridTurnManagerComponent::ProcessPartyCharacterReactionEvent(int32 CharacterIndex, const FGridCombatReactionEvent& Event)
{
	if (!bCombatActive || !Event.IsValid() || !IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent) ||
		!PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return;
	}

	FGridCharacterInventoryState& Character = PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (Character.Resources.CurrentHealth <= 0 || !Character.CharacterId.IsValid())
	{
		return;
	}

	TArray<FGridCombatReactionBinding> Bindings;
	if (!FGridCombatReactionResolver::CollectCharacterBindings(
			Character, PartyPawn->PartyInventoryComponent->PartyInventoryState, Bindings))
	{
		UE_LOG(LogGridTurnManager, Warning, TEXT("[RPG03.4] ReactionProjectionFailed Character=%d CharacterId=%s"),
			CharacterIndex, *Character.CharacterId.ToString(EGuidFormats::Digits));
		return;
	}

	TArray<FGridCombatReactionMatch> Matches;
	FGridCombatReactionResolver::ResolveMatches(Bindings, Character.CharacterId, Event, CombatReactionLedger, true, Matches);
	if (Matches.IsEmpty())
	{
		return;
	}

	UGridStatusEffectLifecycleSubsystem* StatusLifecycle = GetWorld() ? GetWorld()->GetSubsystem<UGridStatusEffectLifecycleSubsystem>() : nullptr;
	if (StatusLifecycle)
	{
		StatusLifecycle->BindToTurnManager(this);
	}

	for (const FGridCombatReactionMatch& Match : Matches)
	{
		if (Match.SecondaryDirectDamage > 0)
		{
			ExecuteReactionSecondaryDirectDamage(CharacterIndex, Match);
		}
		if (Match.bConsumeOwningStatus && StatusLifecycle)
		{
			if (Match.OwningStatusSourceId.IsValid())
			{
				StatusLifecycle->ConsumeStatusEffectFromPartyCharacterBySource(
					CharacterIndex, Match.OwningStatusEffectId, Match.OwningStatusSourceId);
			}
			else
			{
				StatusLifecycle->ConsumeStatusEffectFromPartyCharacter(CharacterIndex, Match.OwningStatusEffectId);
			}
		}
		if (!Match.ApplyOwnerStatusEffectId.IsNone() && StatusLifecycle)
		{
			if (UGridStatusEffectDefinitionAsset* Definition =
					const_cast<UGridStatusEffectDefinitionAsset*>(
						FGridCombatStatusApplicationResolver::ResolveDefinition(Match.ApplyOwnerStatusEffectId)))
			{
				FGridStatusEffectApplyResult ApplyResult;
				FString ApplyError;
				StatusLifecycle->TryApplyStatusEffectToPartyCharacter(
					CharacterIndex, Definition, Character.CharacterId, ApplyResult, ApplyError, 1, Match.ApplyOwnerStatusDurationOverride);
			}
		}
		if (!Match.TransferOwnedTargetStatusEffectId.IsNone())
		{
			TransferOwnedTargetStatusFromReaction(CharacterIndex, Match);
		}
		if (Match.CounterAttackWeaponProfile.bUseEquippedWeapon)
		{
			ExecuteReactionCounterAttack(CharacterIndex, Match);
		}
		OnCombatReactionTriggered.Broadcast(Match);
		UE_LOG(LogGridTurnManager, Log,
			TEXT("[RPG03.4] ReactionTriggered Owner=%s Reaction=%s Trigger=%s Action=%s Round=%d ConsumeStatus=%s"),
			*Match.OwnerCombatantId.ToString(EGuidFormats::Digits), *Match.ReactionId.ToString(), *UEnum::GetValueAsString(Match.Event.Trigger),
			*Match.Event.ActionId.ToString(), Match.Event.RoundNumber, Match.bConsumeOwningStatus ? TEXT("true") : TEXT("false"));
	}
}


bool UGridTurnManagerComponent::ExecuteReactionSecondaryDirectDamage(
	int32 CharacterIndex, const FGridCombatReactionMatch& Match)
{
	if (Match.SecondaryDirectDamage <= 0 || !IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent) ||
		!PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent.Get();
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
	AGridMonsterActor* TargetMonster = FindCombatMonsterById(Match.Event.TargetCombatantId);
	if (Character.Resources.CurrentHealth <= 0 || !Character.CharacterId.IsValid() ||
		!IsValid(TargetMonster) || TargetMonster->IsDead() || !IsValid(TargetMonster->MonsterDefinition))
	{
		return false;
	}

	int32 ScalingCharacterIndex = CharacterIndex;
	if (Match.OwningStatusSourceId.IsValid())
	{
		const int32 SourceIndex = Inventory->PartyInventoryState.ActiveCharacters.IndexOfByPredicate(
			[&Match](const FGridCharacterInventoryState& Candidate)
			{
				return Candidate.CharacterId == Match.OwningStatusSourceId;
			});
		if (SourceIndex != INDEX_NONE)
		{
			ScalingCharacterIndex = SourceIndex;
		}
	}

	FGridInventoryCharacterSummary ScalingSummary;
	if (!Inventory->GetCharacterSummary(ScalingCharacterIndex, ScalingSummary))
	{
		return false;
	}

	FGridCombatReactionProfile Profile;
	Profile.ReactionId = Match.ReactionId;
	Profile.Trigger = Match.Event.Trigger;
	Profile.SecondaryDirectDamage = Match.SecondaryDirectDamage;
	Profile.SecondaryDirectDamageType = Match.SecondaryDirectDamageType;
	Profile.SecondaryDirectDamagePhysicalSubtype = Match.SecondaryDirectDamagePhysicalSubtype;
	Profile.SecondaryDirectDamageScalingAttribute = Match.SecondaryDirectDamageScalingAttribute;
	Profile.SecondaryDirectDamageAttributeModifierScale = Match.SecondaryDirectDamageAttributeModifierScale;
	const int32 RawDamage =
		FGridCombatReactionResolver::ResolveSecondaryDirectDamageAmount(Profile, ScalingSummary.Attributes);
	if (RawDamage <= 0)
	{
		return false;
	}

	FGridAttackTargetStats Target;
	Target.CurrentHealth = TargetMonster->CurrentHealth;
	Target.PhysicalArmor = TargetMonster->CurrentPhysicalArmor;
	Target.MagicalArmor = TargetMonster->CurrentMagicalArmor;
	Target.ResistancePercent = 0;
	Target.DamageMultiplier = TargetMonster->MonsterDefinition->GetDamageMultiplier(
		Match.SecondaryDirectDamageType, Match.SecondaryDirectDamagePhysicalSubtype);

	TArray<FGridCombatModifierProfile> TargetProfiles;
	FGridResolvedCombatModifiers TargetModifiers;
	if (FGridCombatModifierResolver::CollectStatusModifiers(TargetMonster->StatusEffects, TargetProfiles))
	{
		const FGridCombatModifierContext Context = FGridCombatModifierResolver::MakeAttackContext(
			Match.Event.ActionId, Character.ClassId, Match.Event.SourcePolicy, Match.Event.ActionType,
			Match.SecondaryDirectDamageType, Match.SecondaryDirectDamagePhysicalSubtype, Match.Event.SourceTags);
		FGridCombatModifierResolver::Resolve(TargetProfiles, Context, TargetModifiers);
		FGridCombatModifierResolver::ApplyIncomingAttackModifiers(Target, Match.SecondaryDirectDamageType, TargetModifiers);
	}

	const FGridAttackResult Result = FGridCombatResolver::ResolveDirectDamage(
		Target, Match.SecondaryDirectDamageType, RawDamage, Match.SecondaryDirectDamagePhysicalSubtype);
	if (!Result.bHit)
	{
		return false;
	}

	const bool bPreviousResolutionInProgress = bPlayerAttackResolutionInProgress;
	bPlayerAttackResolutionInProgress = true;
	TargetMonster->ApplyAttackResult(Result);
	bPlayerAttackResolutionInProgress = bPreviousResolutionInProgress;
	if (FGridCombatantInitiativeEntry* Entry =
			FindInitiativeEntry(EGridCombatantSide::Monster, TargetMonster->ResolvePersistenceId()))
	{
		RefreshInitiativeEntryVitals(*Entry);
		if (Entry->State != EGridCombatantTurnState::Defeated)
		{
			OnCombatantStateChanged.Broadcast(*Entry);
		}
	}

	UE_LOG(LogGridTurnManager, Log,
		TEXT("[RPG03.9.4F1] ReactionSecondaryDamage Owner=%s Reaction=%s Target=%s Raw=%d Applied=%d Type=%s"),
		*Character.CharacterId.ToString(EGuidFormats::Digits), *Match.ReactionId.ToString(),
		*TargetMonster->ResolvePersistenceId().ToString(EGuidFormats::Digits), RawDamage, Result.GetTotalAppliedDamage(),
		*UEnum::GetValueAsString(Match.SecondaryDirectDamageType));
	return true;
}

bool UGridTurnManagerComponent::ExecuteReactionCounterAttack(int32 CharacterIndex, const FGridCombatReactionMatch& Match)
{
	if (!bCombatActive || Match.CounterAttackActionId.IsNone() || !Match.CounterAttackWeaponProfile.bUseEquippedWeapon ||
		!IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent) ||
		!PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent.Get();
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
	AGridMonsterActor* TargetMonster = FindCombatMonsterById(Match.Event.SourceCombatantId);
	if (Character.Resources.CurrentHealth <= 0 || !IsValid(TargetMonster) || TargetMonster->IsDead() || !IsValid(TargetMonster->MonsterDefinition))
	{
		return false;
	}

	const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
	const int32 Distance =
		FMath::Abs(TargetMonster->CurrentCell.X - PartyCell.X) + FMath::Abs(TargetMonster->CurrentCell.Y - PartyCell.Y);
	if (Distance < 1 || Distance > Match.CounterAttackRangeCells)
	{
		return false;
	}

	FGridCombatActionDefinition ReactionAction;
	ReactionAction.ActionId = Match.CounterAttackActionId;
	ReactionAction.DisplayName = FText::FromName(Match.CounterAttackActionId);
	ReactionAction.ActionType = EGridCombatActionType::MeleeAttack;
	ReactionAction.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
	ReactionAction.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
	ReactionAction.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	ReactionAction.ActionPointCost = 1; // Structural action definition only; the reaction transaction spends no PA.
	ReactionAction.RangeCells = Match.CounterAttackRangeCells;
	ReactionAction.WeaponAttackProfile = Match.CounterAttackWeaponProfile;

	FGridOffensiveEquipmentProfile OffensiveProfile;
	FName OffensiveItemDefinitionId = NAME_None;
	EGridEquipmentSlot OffensiveEquipmentSlot = EGridEquipmentSlot::None;
	TArray<FName> OffensiveItemTags;
	EGridPlayerAttackRejectReason ResolveReason = EGridPlayerAttackRejectReason::None;
	if (!ResolveCombatActionWeaponProfile(Inventory, CharacterIndex, ReactionAction, OffensiveProfile, OffensiveItemDefinitionId,
			OffensiveEquipmentSlot, OffensiveItemTags, ResolveReason))
	{
		return false;
	}

	FGridInventoryCharacterSummary CharacterSummary;
	if (!Inventory->GetCharacterSummary(CharacterIndex, CharacterSummary))
	{
		return false;
	}

	FGridAttackSourceStats Source;
	FGridAttackTargetStats Target;
	FGridAttackDefinition AttackDefinition;
	if (!BuildPlayerAttackResolutionInputs(CharacterSummary, TargetMonster, OffensiveProfile, Source, Target, AttackDefinition))
	{
		return false;
	}
	Source.RawDamagePercent = Match.CounterAttackWeaponProfile.WeaponDamagePercent;

	const FGridCombatModifierContext AttackContext =
		FGridCombatModifierResolver::MakeResolvedActionAttackContext(
			ReactionAction, Character.ClassId, OffensiveProfile, OffensiveItemTags);

	TArray<FGridCombatModifierProfile> SourceProfiles;
	FGridResolvedCombatModifiers SourceModifiers;
	if (FGridCombatModifierResolver::CollectCharacterModifiers(Character, SourceProfiles))
	{
		FGridCombatModifierResolver::Resolve(SourceProfiles, AttackContext, SourceModifiers);
		FGridCombatModifierResolver::ApplyOutgoingAttackModifiers(Source, SourceModifiers);
	}

	TArray<FGridCombatModifierProfile> TargetProfiles;
	FGridResolvedCombatModifiers TargetModifiers;
	if (FGridCombatModifierResolver::CollectStatusModifiers(TargetMonster->StatusEffects, TargetProfiles))
	{
		FGridCombatModifierResolver::Resolve(TargetProfiles, AttackContext, TargetModifiers);
		FGridCombatModifierResolver::ApplyIncomingAttackModifiers(Target, AttackDefinition.DamageType, TargetModifiers);
	}

	const FGridAttackResult Result = FGridCombatResolver::ResolveAttack(Source, Target, AttackDefinition, CombatRandomStream);

	FGridPlayerAttackRequest Request;
	Request.RequestId = FGuid::NewGuid();
	Request.RoundNumber = FMath::Max(1, RoundNumber);
	Request.AttackerCharacterIndex = CharacterIndex;
	Request.AttackerCharacterId = Character.CharacterId;
	Request.TargetMonsterId = TargetMonster->ResolvePersistenceId();
	Request.PartyCell = PartyCell;
	Request.TargetCell = TargetMonster->CurrentCell;
	Request.PartyFacing = PartyPawn->Facing;
	Request.RangeCells = Match.CounterAttackRangeCells;
	Request.AttackId = Match.CounterAttackActionId;
	Request.OffensiveItemDefinitionId = OffensiveItemDefinitionId;
	Request.OffensiveEquipmentSlot = OffensiveEquipmentSlot;
	Request.ActionPointCost = 0;

	++PlayerAttackRequestedBroadcastCount;
	OnPlayerAttackRequested.Broadcast(Request);

	FGridCombatLogEntry AttackEntry;
	AttackEntry.RoundNumber = RoundNumber;
	AttackEntry.Phase = CurrentPhase;
	AttackEntry.Type = Result.bHit ? EGridCombatLogEntryType::AttackHit : EGridCombatLogEntryType::AttackMiss;
	AttackEntry.SourceId = FName(*Character.CharacterId.ToString(EGuidFormats::Digits));
	AttackEntry.SourceDisplayName = CharacterSummary.DisplayName;
	AttackEntry.TargetId = FName(*Request.TargetMonsterId.ToString(EGuidFormats::Digits));
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
	if (FGridCombatantInitiativeEntry* TargetEntry =
			FindInitiativeEntry(EGridCombatantSide::Monster, Request.TargetMonsterId))
	{
		const int32 PreviousHealth = TargetEntry->CurrentHealth;
		RefreshInitiativeEntryVitals(*TargetEntry);
		if (TargetEntry->CurrentHealth != PreviousHealth && TargetEntry->State != EGridCombatantTurnState::Defeated)
		{
			OnCombatantStateChanged.Broadcast(*TargetEntry);
		}
	}

	EmitPlayerAttackReactionEvents(CharacterIndex, Request, Result, EGridCombatActionSourcePolicy::Ability,
		EGridCombatActionType::MeleeAttack, Request.RequestId, true, true, OffensiveItemTags,
		OffensiveEquipmentSlot != EGridEquipmentSlot::None);
	++PlayerAttackResolvedBroadcastCount;
	bPlayerAttackResolutionInProgress = false;
	OnPlayerAttackResolved.Broadcast(Request, TargetMonster, Result);
	if (bCollectRuntimeMetrics)
	{
		++RuntimeMetrics.AttacksResolved;
	}

	if (bPendingVictoryAfterPlayerAttack)
	{
		bPendingVictoryAfterPlayerAttack = false;
		FinishCombat(EGridCombatPhase::Victory);
	}
	return true;
}

void UGridTurnManagerComponent::ApplyIncomingPartyDamageInterception(int32 TargetCharacterIndex, const FGridMonsterAttackDefinition& Attack,
	const FGridAttackTargetStats& TargetBefore, const FGuid& ActionInstanceId, FGridAttackResult& InOutResult)
{
	if (!InOutResult.bHit || InOutResult.DamageType != EGridDamageType::Physical || InOutResult.DamageAfterModifiers <= 0 ||
		!ActionInstanceId.IsValid() || !IsValid(CurrentMonster) || !IsValid(CurrentCombatComponent) ||
		!IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
	{
		return;
	}

	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent.Get();
	if (!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(TargetCharacterIndex))
	{
		return;
	}

	const FGuid SourceId = CurrentMonster->ResolvePersistenceId();
	const FGuid TargetId = ResolvePlayerCombatantId(TargetCharacterIndex);
	if (!SourceId.IsValid() || !TargetId.IsValid())
	{
		return;
	}

	FGridCombatReactionEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.ActionInstanceId = ActionInstanceId;
	Event.RoundNumber = FMath::Max(1, RoundNumber);
	Event.Trigger = EGridCombatReactionTrigger::IncomingAttackHit;
	Event.SourceCombatantId = SourceId;
	Event.TargetCombatantId = TargetId;
	Event.ActionId = Attack.AttackId;
	Event.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	Event.ActionType = Attack.IsRangedAttack() ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
	Event.DamageType = InOutResult.DamageType;

	const int32 FrontLineCount = FMath::Max(0, CurrentCombatComponent->FrontLineSlotCount);
	for (int32 OwnerIndex = 0; OwnerIndex < Inventory->PartyInventoryState.ActiveCharacters.Num(); ++OwnerIndex)
	{
		if (OwnerIndex == TargetCharacterIndex)
		{
			continue;
		}

		FGridCharacterInventoryState& Owner = Inventory->PartyInventoryState.ActiveCharacters[OwnerIndex];
		if (Owner.Resources.CurrentHealth <= 0 || !Owner.CharacterId.IsValid())
		{
			continue;
		}

		TArray<FGridCombatReactionBinding> Bindings;
		if (!FGridCombatReactionResolver::CollectCharacterBindings(
				Owner, Inventory->PartyInventoryState, Bindings))
		{
			continue;
		}

		for (const FGridCombatReactionBinding& Binding : Bindings)
		{
			const FGridCombatReactionProfile& Profile = Binding.Profile;
			if (Profile.InterceptFinalDamagePercent <= 0 ||
				(Profile.bRequireOwnerFrontRow && OwnerIndex >= FrontLineCount) ||
				(Profile.bRequireEventTargetFrontRow && TargetCharacterIndex >= FrontLineCount) ||
				!FGridCombatReactionResolver::Matches(Profile, Event) ||
				!CombatReactionLedger.CanTrigger(Profile, Owner.CharacterId, Event))
			{
				continue;
			}

			const int32 RedirectedDamage = static_cast<int32>(
				static_cast<int64>(InOutResult.DamageAfterModifiers) * Profile.InterceptFinalDamagePercent / 100);
			if (RedirectedDamage <= 0 || !CombatReactionLedger.Commit(Profile, Owner.CharacterId, Event))
			{
				continue;
			}

			const int32 RetainedDamage = FMath::Max(0, InOutResult.DamageAfterModifiers - RedirectedDamage);
			InOutResult.DamageAfterModifiers = RetainedDamage;
			InOutResult.PhysicalArmorDamage = FMath::Min(FMath::Max(0, TargetBefore.PhysicalArmor), RetainedDamage);
			InOutResult.MagicalArmorDamage = 0;
			InOutResult.HealthDamage = FMath::Max(0, RetainedDamage - InOutResult.PhysicalArmorDamage);
			InOutResult.TargetHealthAfter = FMath::Max(0, TargetBefore.CurrentHealth - InOutResult.HealthDamage);

			const int32 OwnerHealthBefore = Owner.Resources.CurrentHealth;
			const int32 OwnerArmorDamage = FMath::Min(FMath::Max(0, Owner.Resources.CurrentPhysicalArmor), RedirectedDamage);
			const int32 OwnerHealthDamage = FMath::Max(0, RedirectedDamage - OwnerArmorDamage);
			Owner.Resources.CurrentPhysicalArmor = FMath::Max(0, Owner.Resources.CurrentPhysicalArmor - OwnerArmorDamage);
			Owner.Resources.CurrentHealth = FMath::Max(0, Owner.Resources.CurrentHealth - OwnerHealthDamage);
			Inventory->NotifyPartyInventoryChanged(OwnerIndex);
			RefreshPlayerCharacterVitalState(OwnerIndex);

			FGridCombatReactionMatch Match;
			Match.ReactionId = Profile.ReactionId;
			Match.OwnerCombatantId = Owner.CharacterId;
			Match.OwningStatusEffectId = Binding.OwningStatusEffectId;
			Match.InterceptFinalDamagePercent = Profile.InterceptFinalDamagePercent;
			Match.bRequireOwnerFrontRow = Profile.bRequireOwnerFrontRow;
			Match.bRequireEventTargetFrontRow = Profile.bRequireEventTargetFrontRow;
			Match.Event = Event;
			OnCombatReactionTriggered.Broadcast(Match);

			if (OwnerHealthBefore > 0 && Owner.Resources.CurrentHealth <= 0)
			{
				FGridCombatLogEntry DefeatedEntry;
				DefeatedEntry.RoundNumber = RoundNumber;
				DefeatedEntry.Phase = CurrentPhase;
				DefeatedEntry.Type = EGridCombatLogEntryType::CharacterDefeated;
				DefeatedEntry.SourceId = ResolveMonsterLogId(CurrentMonster);
				DefeatedEntry.SourceDisplayName = ResolveMonsterDisplayName(CurrentMonster);
				DefeatedEntry.TargetCharacterIndex = OwnerIndex;
				DefeatedEntry.TargetDisplayName = Owner.DisplayName;
				DefeatedEntry.bTargetDefeated = true;
				DefeatedEntry.Message = FGridCombatLogFormatter::FormatCharacterDefeated(Owner.DisplayName);
				AppendCombatLogEntry(DefeatedEntry);
			}

			UE_LOG(LogGridTurnManager, Log,
				TEXT("[RPG03.9.1] DamageIntercept Owner=%s Target=%s Reaction=%s Redirected=%d Retained=%d"),
				*Owner.CharacterId.ToString(EGuidFormats::Digits), *TargetId.ToString(EGuidFormats::Digits),
				*Profile.ReactionId.ToString(), RedirectedDamage, RetainedDamage);
			return; // One deterministic interceptor per incoming attack.
		}
	}
}

void UGridTurnManagerComponent::EmitMonsterDefeatedReactionEvents(AGridMonsterActor* Monster)
{
	if (!bCombatActive || !IsValid(Monster) || !IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
	{
		return;
	}
	const FGuid TargetId = Monster->ResolvePersistenceId();
	if (!TargetId.IsValid())
	{
		return;
	}

	const TArray<FGridCharacterInventoryState>& Characters =
		PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
	for (int32 CharacterIndex = 0; CharacterIndex < Characters.Num(); ++CharacterIndex)
	{
		const FGridCharacterInventoryState& Character = Characters[CharacterIndex];
		if (Character.Resources.CurrentHealth <= 0 || !Character.CharacterId.IsValid())
		{
			continue;
		}

		FGridCombatReactionEvent Event;
		Event.EventId = FGuid::NewGuid();
		Event.ActionInstanceId = FGuid::NewGuid();
		Event.RoundNumber = FMath::Max(1, RoundNumber);
		Event.Trigger = EGridCombatReactionTrigger::OwnedStatusTargetDefeated;
		Event.SourceCombatantId = Character.CharacterId;
		Event.TargetCombatantId = TargetId;
		for (const FGridStatusEffectRuntimeState& State : Monster->StatusEffects.ActiveEffects)
		{
			if (State.IsValid() && State.SourceId == Character.CharacterId)
			{
				Event.TargetStatusEffectIdsFromOwner.AddUnique(State.EffectId);
			}
		}
		ProcessPartyCharacterReactionEvent(CharacterIndex, Event);
	}
}

bool UGridTurnManagerComponent::TransferOwnedTargetStatusFromReaction(int32 CharacterIndex, const FGridCombatReactionMatch& Match)
{
	if (Match.TransferOwnedTargetStatusEffectId.IsNone() || !IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent) ||
		!PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return false;
	}
	FGridCharacterInventoryState& Character =
		PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (Character.Resources.CurrentHealth <= 0 || !Character.CharacterId.IsValid())
	{
		return false;
	}

	const AGridMonsterActor* DefeatedTarget = FindCombatMonsterById(Match.Event.TargetCombatantId);
	const FIntPoint OriginCell = IsValid(DefeatedTarget)
		? DefeatedTarget->CurrentCell
		: FIntPoint(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
	AGridMonsterActor* BestTarget = nullptr;
	int32 BestDistance = MAX_int32;
	FString BestKey;
	for (AGridMonsterActor* Candidate : CombatMonsters)
	{
		if (!IsValid(Candidate) || Candidate->IsDead() || !Candidate->bMonsterEnabled || !Candidate->IsRuntimeLevelActive() ||
			Candidate->ResolvePersistenceId() == Match.Event.TargetCombatantId)
		{
			continue;
		}
		const int32 Distance =
			FMath::Abs(Candidate->CurrentCell.X - OriginCell.X) + FMath::Abs(Candidate->CurrentCell.Y - OriginCell.Y);
		if (Distance < 1 || Distance > Match.TransferTargetRangeCells)
		{
			continue;
		}
		const FString Key = Candidate->ResolvePersistenceId().ToString(EGuidFormats::Digits);
		if (!BestTarget || Distance < BestDistance || (Distance == BestDistance && Key < BestKey))
		{
			BestTarget = Candidate;
			BestDistance = Distance;
			BestKey = Key;
		}
	}
	if (!IsValid(BestTarget))
	{
		return false;
	}

	UGridStatusEffectLifecycleSubsystem* StatusLifecycle =
		GetWorld() ? GetWorld()->GetSubsystem<UGridStatusEffectLifecycleSubsystem>() : nullptr;
	UGridStatusEffectDefinitionAsset* Definition =
		const_cast<UGridStatusEffectDefinitionAsset*>(
			FGridCombatStatusApplicationResolver::ResolveDefinition(Match.TransferOwnedTargetStatusEffectId));
	if (!StatusLifecycle || !IsValid(Definition))
	{
		return false;
	}
	StatusLifecycle->BindToTurnManager(this);
	FGridStatusEffectApplyResult ApplyResult;
	FString ApplyError;
	const bool bApplied = StatusLifecycle->TryApplyStatusEffectToMonster(
		BestTarget, Definition, Character.CharacterId, ApplyResult, ApplyError, 1, Match.TransferStatusDurationOverride);
	if (bApplied)
	{
		UE_LOG(LogGridTurnManager, Log,
			TEXT("[RPG03.9] TargetStatusTransferred Owner=%s Effect=%s Target=%s Distance=%d DurationOverride=%d"),
			*Character.CharacterId.ToString(EGuidFormats::Digits), *Match.TransferOwnedTargetStatusEffectId.ToString(),
			*BestTarget->ResolvePersistenceId().ToString(EGuidFormats::Digits), BestDistance, Match.TransferStatusDurationOverride);
	}
	return bApplied;
}

void UGridTurnManagerComponent::EmitPlayerAttackReactionEvents(int32 CharacterIndex, const FGridPlayerAttackRequest& Request,
	const FGridAttackResult& Result, EGridCombatActionSourcePolicy SourcePolicy, EGridCombatActionType ActionType, const FGuid& ActionInstanceId,
	bool bReactionGenerated, bool bEmitActionResolved, const TArray<FName>& SourceTags, bool bWeaponAttack)
{
	if (!ActionInstanceId.IsValid())
	{
		return;
	}

	FGridCombatReactionEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.ActionInstanceId = ActionInstanceId;
	Event.RoundNumber = FMath::Max(1, RoundNumber);
	Event.Trigger = Result.bHit ? EGridCombatReactionTrigger::AttackHit : EGridCombatReactionTrigger::AttackMiss;
	Event.SourceCombatantId = Request.AttackerCharacterId;
	Event.TargetCombatantId = Request.TargetMonsterId;
	Event.ActionId = Request.AttackId;
	Event.SourcePolicy = SourcePolicy;
	Event.ActionType = ActionType;
	Event.DamageType = Result.DamageType;
	Event.SourceTags = SourceTags;
	Event.bOffensiveAction = true;
	Event.bWeaponAttack = bWeaponAttack;
	Event.bAppliedDamage = Result.GetTotalAppliedDamage() > 0;
	Event.bReactionGenerated = bReactionGenerated;
	ProcessPartyCharacterReactionEvent(CharacterIndex, Event);

	if (bEmitActionResolved)
	{
		FGridCombatReactionEvent ResolvedEvent = Event;
		ResolvedEvent.EventId = FGuid::NewGuid();
		ResolvedEvent.Trigger = EGridCombatReactionTrigger::ActionResolved;
		ProcessPartyCharacterReactionEvent(CharacterIndex, ResolvedEvent);
	}

	if (Result.TargetHealthBefore > 0 && Result.TargetHealthAfter <= 0)
	{
		FGridCombatReactionEvent KillEvent = Event;
		KillEvent.EventId = FGuid::NewGuid();
		KillEvent.Trigger = EGridCombatReactionTrigger::TargetDefeated;
		ProcessPartyCharacterReactionEvent(CharacterIndex, KillEvent);
	}
}

void UGridTurnManagerComponent::EmitMonsterAttackReactionEvents(
	int32 TargetCharacterIndex, const FGridMonsterAttackDefinition& Attack, const FGridAttackResult& Result, const FGuid& ActionInstanceId)
{
	if (!ActionInstanceId.IsValid() || !IsValid(CurrentMonster))
	{
		return;
	}

	const FGuid TargetId = ResolvePlayerCombatantId(TargetCharacterIndex);
	const FGuid SourceId = CurrentMonster->ResolvePersistenceId();
	if (!SourceId.IsValid() || !TargetId.IsValid())
	{
		return;
	}

	FGridCombatReactionEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.ActionInstanceId = ActionInstanceId;
	Event.RoundNumber = FMath::Max(1, RoundNumber);
	Event.Trigger = Result.bHit ? EGridCombatReactionTrigger::AttackHit : EGridCombatReactionTrigger::AttackMiss;
	Event.SourceCombatantId = SourceId;
	Event.TargetCombatantId = TargetId;
	Event.ActionId = Attack.AttackId;
	Event.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	Event.ActionType = Attack.IsRangedAttack() ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
	Event.DamageType = Result.DamageType;
	Event.bOffensiveAction = true;
	Event.bAppliedDamage = Result.GetTotalAppliedDamage() > 0;
	ProcessPartyCharacterReactionEvent(TargetCharacterIndex, Event);

	if (Result.GetTotalAppliedDamage() > 0)
	{
		FGridCombatReactionEvent DamageEvent = Event;
		DamageEvent.EventId = FGuid::NewGuid();
		DamageEvent.Trigger = EGridCombatReactionTrigger::DirectDamageReceived;
		ProcessPartyCharacterReactionEvent(TargetCharacterIndex, DamageEvent);
	}
}

void UGridTurnManagerComponent::EmitCharacterActionResolvedReaction(
	int32 CharacterIndex, const FGridAvailableCombatAction& Action, const FGuid& ActionInstanceId, const TArray<FName>& SourceTags)
{
	if (!ActionInstanceId.IsValid() || !IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent) ||
		!PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return;
	}

	const FGridCharacterInventoryState& Character = PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	FGridCombatReactionEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.ActionInstanceId = ActionInstanceId;
	Event.RoundNumber = FMath::Max(1, RoundNumber);
	Event.Trigger = EGridCombatReactionTrigger::ActionResolved;
	Event.SourceCombatantId = Character.CharacterId;
	Event.TargetCombatantId = Character.CharacterId;
	Event.ActionId = Action.Definition.ActionId;
	Event.SourcePolicy = Action.Definition.SourcePolicy;
	Event.ActionType = Action.Definition.ActionType;
	Event.DamageType = Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack
		? Action.Definition.OffensiveProfile.AttackDefinition.DamageType
		: EGridDamageType::Physical;
	Event.SourceTags = Action.Definition.SourceTags;
	for (const FName SourceTag : SourceTags)
	{
		if (!SourceTag.IsNone())
		{
			Event.SourceTags.AddUnique(SourceTag);
		}
	}
	Event.bOffensiveAction =
		Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack ||
		Action.Definition.TargetingPolicy == EGridCombatTargetingPolicy::Hostile;
	ProcessPartyCharacterReactionEvent(CharacterIndex, Event);
}
