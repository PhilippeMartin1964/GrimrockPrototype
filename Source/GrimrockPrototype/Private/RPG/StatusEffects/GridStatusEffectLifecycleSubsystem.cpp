#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"

void UGridStatusEffectLifecycleSubsystem::Initialize (FSubsystemCollectionBase& Collection)
{
    Super::Initialize (Collection);
    if (UWorld* World = GetWorld ())
    {
        ActorSpawnedDelegateHandle = World->AddOnActorSpawnedHandler (
            FOnActorSpawned::FDelegate::CreateUObject (
                this,
                &UGridStatusEffectLifecycleSubsystem::HandleActorSpawned));
    }
}

void UGridStatusEffectLifecycleSubsystem::Deinitialize ()
{
    UnbindFromTurnManager ();
    if (UWorld* World = GetWorld (); World && ActorSpawnedDelegateHandle.IsValid ())
    {
        World->RemoveOnActorSpawnedHandler (ActorSpawnedDelegateHandle);
    }
    ActorSpawnedDelegateHandle.Reset ();
    Super::Deinitialize ();
}

void UGridStatusEffectLifecycleSubsystem::OnWorldBeginPlay (UWorld& InWorld)
{
    Super::OnWorldBeginPlay (InWorld);
    if (BoundTurnManager.IsValid ())
    {
        return;
    }
    for (TActorIterator<AActor> It (&InWorld); It; ++It)
    {
        TryBindFromActor (*It);
        if (BoundTurnManager.IsValid ())
        {
            return;
        }
    }
}

bool UGridStatusEffectLifecycleSubsystem::DoesSupportWorldType (EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game ||
        WorldType == EWorldType::PIE ||
        WorldType == EWorldType::GamePreview;
}

void UGridStatusEffectLifecycleSubsystem::BindToTurnManager (UGridTurnManagerComponent* TurnManager)
{
    if (!IsValid (TurnManager) || BoundTurnManager.Get () == TurnManager)
    {
        return;
    }

    UnbindFromTurnManager ();
    BoundTurnManager = TurnManager;
    LastObservedRoundNumber = FMath::Max (0, TurnManager->RoundNumber);
    TurnManager->OnCombatantStateChanged.AddUniqueDynamic (
        this,
        &UGridStatusEffectLifecycleSubsystem::HandleCombatantStateChanged);
    TurnManager->OnRoundStarted.AddUniqueDynamic (
        this,
        &UGridStatusEffectLifecycleSubsystem::HandleRoundStarted);
    TurnManager->OnCombatEnded.AddUniqueDynamic (
        this,
        &UGridStatusEffectLifecycleSubsystem::HandleCombatEnded);
}

void UGridStatusEffectLifecycleSubsystem::UnbindFromTurnManager ()
{
    if (UGridTurnManagerComponent* TurnManager = BoundTurnManager.Get ())
    {
        TurnManager->OnCombatantStateChanged.RemoveDynamic (
            this,
            &UGridStatusEffectLifecycleSubsystem::HandleCombatantStateChanged);
        TurnManager->OnRoundStarted.RemoveDynamic (
            this,
            &UGridStatusEffectLifecycleSubsystem::HandleRoundStarted);
        TurnManager->OnCombatEnded.RemoveDynamic (
            this,
            &UGridStatusEffectLifecycleSubsystem::HandleCombatEnded);
    }
    BoundTurnManager.Reset ();
    LastObservedRoundNumber = 0;
}

void UGridStatusEffectLifecycleSubsystem::TryBindFromActor (AActor* Actor)
{
    if (!IsValid (Actor) || BoundTurnManager.IsValid ())
    {
        return;
    }
    BindToTurnManager (Actor->FindComponentByClass<UGridTurnManagerComponent> ());
}

void UGridStatusEffectLifecycleSubsystem::HandleActorSpawned (AActor* Actor)
{
    TryBindFromActor (Actor);
}

void UGridStatusEffectLifecycleSubsystem::HandleCombatantStateChanged (
    FGridCombatantInitiativeEntry Combatant)
{
    if (Combatant.State != EGridCombatantTurnState::Completed &&
        Combatant.State != EGridCombatantTurnState::Incapacitated)
    {
        return;
    }

    UGridTurnManagerComponent* TurnManager = BoundTurnManager.Get ();
    if (!IsValid (TurnManager))
    {
        return;
    }

    FGridStatusEffectAdvanceResult AdvanceResult;
    if (Combatant.Side == EGridCombatantSide::Party)
    {
        if (!IsValid (TurnManager->PartyPawn) || !IsValid (TurnManager->PartyPawn->PartyInventoryComponent))
        {
            return;
        }
        TArray<FGridCharacterInventoryState>& Characters =
            TurnManager->PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
        if (!Characters.IsValidIndex (Combatant.CharacterIndex))
        {
            return;
        }
        Characters[Combatant.CharacterIndex].StatusEffects.AdvanceDuration (
            EGridStatusEffectDurationUnit::Turns,
            AdvanceResult);
        return;
    }

    if (Combatant.Side != EGridCombatantSide::Monster || !Combatant.CombatantId.IsValid ())
    {
        return;
    }
    for (AGridMonsterActor* Monster : TurnManager->CombatMonsters)
    {
        if (IsValid (Monster) && Monster->ResolvePersistenceId () == Combatant.CombatantId)
        {
            Monster->StatusEffects.AdvanceDuration (
                EGridStatusEffectDurationUnit::Turns,
                AdvanceResult);
            return;
        }
    }
}

void UGridStatusEffectLifecycleSubsystem::HandleRoundStarted (int32 RoundNumber)
{
    if (RoundNumber <= 0)
    {
        return;
    }
    if (LastObservedRoundNumber <= 0 || RoundNumber <= LastObservedRoundNumber)
    {
        LastObservedRoundNumber = RoundNumber;
        return;
    }

    const int32 BoundaryCount = RoundNumber - LastObservedRoundNumber;
    LastObservedRoundNumber = RoundNumber;
    AdvanceAllRoundEffects (BoundaryCount);
}

void UGridStatusEffectLifecycleSubsystem::HandleCombatEnded (EGridCombatPhase ResultPhase)
{
    LastObservedRoundNumber = 0;
}

void UGridStatusEffectLifecycleSubsystem::AdvanceAllRoundEffects (int32 BoundaryCount)
{
    UGridTurnManagerComponent* TurnManager = BoundTurnManager.Get ();
    if (!IsValid (TurnManager) || BoundaryCount <= 0)
    {
        return;
    }

    for (int32 BoundaryIndex = 0; BoundaryIndex < BoundaryCount; ++BoundaryIndex)
    {
        if (IsValid (TurnManager->PartyPawn) && IsValid (TurnManager->PartyPawn->PartyInventoryComponent))
        {
            TArray<FGridCharacterInventoryState>& Characters =
                TurnManager->PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
            for (FGridCharacterInventoryState& Character : Characters)
            {
                FGridStatusEffectAdvanceResult AdvanceResult;
                Character.StatusEffects.AdvanceDuration (
                    EGridStatusEffectDurationUnit::Rounds,
                    AdvanceResult);
            }
        }

        for (AGridMonsterActor* Monster : TurnManager->CombatMonsters)
        {
            if (!IsValid (Monster))
            {
                continue;
            }
            FGridStatusEffectAdvanceResult AdvanceResult;
            Monster->StatusEffects.AdvanceDuration (
                EGridStatusEffectDurationUnit::Rounds,
                AdvanceResult);
        }
    }
}
