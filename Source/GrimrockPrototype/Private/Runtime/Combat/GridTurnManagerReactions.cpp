#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
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
	if (!FGridCombatReactionResolver::CollectCharacterBindings(Character, Bindings))
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
		if (Match.bConsumeOwningStatus && StatusLifecycle)
		{
			StatusLifecycle->ConsumeStatusEffectFromPartyCharacter(CharacterIndex, Match.OwningStatusEffectId);
		}
		OnCombatReactionTriggered.Broadcast(Match);
		UE_LOG(LogGridTurnManager, Log,
			TEXT("[RPG03.4] ReactionTriggered Owner=%s Reaction=%s Trigger=%s Action=%s Round=%d ConsumeStatus=%s"),
			*Match.OwnerCombatantId.ToString(EGuidFormats::Digits), *Match.ReactionId.ToString(), *UEnum::GetValueAsString(Match.Event.Trigger),
			*Match.Event.ActionId.ToString(), Match.Event.RoundNumber, Match.bConsumeOwningStatus ? TEXT("true") : TEXT("false"));
	}
}

void UGridTurnManagerComponent::EmitPlayerAttackReactionEvents(int32 CharacterIndex, const FGridPlayerAttackRequest& Request,
	const FGridAttackResult& Result, EGridCombatActionSourcePolicy SourcePolicy, EGridCombatActionType ActionType, const FGuid& ActionInstanceId,
	bool bReactionGenerated, bool bEmitActionResolved)
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
	int32 CharacterIndex, const FGridAvailableCombatAction& Action, const FGuid& ActionInstanceId)
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
	ProcessPartyCharacterReactionEvent(CharacterIndex, Event);
}
