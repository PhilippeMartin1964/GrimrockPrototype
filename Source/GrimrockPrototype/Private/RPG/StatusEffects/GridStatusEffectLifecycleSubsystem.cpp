#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectInitiativeResolver.h"
#include "RPG/StatusEffects/GridStatusEffectPeriodicDamageResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

DEFINE_LOG_CATEGORY_STATIC (LogGridStatusEffects, Log, All);

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
    TurnManager->OnTurnOrderChanged.AddUniqueDynamic (
        this,
        &UGridStatusEffectLifecycleSubsystem::HandleTurnOrderChanged);
    RefreshAllInitiativeModifiers ();
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
        TurnManager->OnTurnOrderChanged.RemoveDynamic (
            this,
            &UGridStatusEffectLifecycleSubsystem::HandleTurnOrderChanged);
    }
    BoundTurnManager.Reset ();
    LastObservedRoundNumber = 0;
    bRefreshingInitiativeModifiers = false;
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

bool UGridStatusEffectLifecycleSubsystem::TryApplyStatusEffectToPartyCharacter (
    int32 CharacterIndex,
    UGridStatusEffectDefinitionAsset* Definition,
    const FGuid& SourceId,
    FGridStatusEffectApplyResult& OutResult,
    FString& OutError,
    int32 InitialStackCount,
    int32 DurationOverride,
    int32 PotencyOverride)
{
    OutResult.Reset ();
    UGridTurnManagerComponent* TurnManager = BoundTurnManager.Get ();
    if (!IsValid (Definition))
    {
        OutError = TEXT ("Status effect definition is not assigned.");
        return false;
    }
    if (!IsValid (TurnManager) ||
        !IsValid (TurnManager->PartyPawn) ||
        !IsValid (TurnManager->PartyPawn->PartyInventoryComponent))
    {
        OutError = TEXT ("Party status application requires a bound TurnManager with a party inventory.");
        return false;
    }

    TArray<FGridCharacterInventoryState>& Characters =
        TurnManager->PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
    if (!Characters.IsValidIndex (CharacterIndex))
    {
        OutError = FString::Printf (
            TEXT ("CharacterIndex %d is invalid for status application."),
            CharacterIndex);
        return false;
    }

    const bool bApplied = Characters[CharacterIndex].StatusEffects.TryApply (
        *Definition,
        SourceId,
        InitialStackCount,
        DurationOverride,
        PotencyOverride,
        OutResult,
        OutError);
    if (bApplied && OutResult.DidMutate ())
    {
        RefreshInitiativeModifierForPartyCharacter (CharacterIndex);
    }
    return bApplied;
}

bool UGridStatusEffectLifecycleSubsystem::TryApplyStatusEffectToMonster (
    AGridMonsterActor* Monster,
    UGridStatusEffectDefinitionAsset* Definition,
    const FGuid& SourceId,
    FGridStatusEffectApplyResult& OutResult,
    FString& OutError,
    int32 InitialStackCount,
    int32 DurationOverride,
    int32 PotencyOverride)
{
    OutResult.Reset ();
    if (!IsValid (Monster))
    {
        OutError = TEXT ("Monster status application requires a valid monster.");
        return false;
    }
    if (!IsValid (Definition))
    {
        OutError = TEXT ("Status effect definition is not assigned.");
        return false;
    }

    const bool bApplied = Monster->StatusEffects.TryApply (
        *Definition,
        SourceId,
        InitialStackCount,
        DurationOverride,
        PotencyOverride,
        OutResult,
        OutError);
    if (bApplied && OutResult.DidMutate ())
    {
        RefreshInitiativeModifierForMonster (Monster);
    }
    return bApplied;
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

        FGridCharacterInventoryState& Character = Characters[Combatant.CharacterIndex];
        ApplyPeriodicDamageToCharacter (
            Character,
            Combatant.CharacterIndex,
            EGridStatusEffectDurationUnit::Turns);
        Character.StatusEffects.AdvanceDuration (
            EGridStatusEffectDurationUnit::Turns,
            AdvanceResult);
        RefreshInitiativeModifierForPartyCharacter (Combatant.CharacterIndex);
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
            ApplyPeriodicDamageToMonster (
                Monster,
                EGridStatusEffectDurationUnit::Turns);
            Monster->StatusEffects.AdvanceDuration (
                EGridStatusEffectDurationUnit::Turns,
                AdvanceResult);
            RefreshInitiativeModifierForMonster (Monster);
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

void UGridStatusEffectLifecycleSubsystem::HandleTurnOrderChanged ()
{
    if (!bRefreshingInitiativeModifiers)
    {
        RefreshAllInitiativeModifiers ();
    }
}

void UGridStatusEffectLifecycleSubsystem::RefreshAllInitiativeModifiers ()
{
    UGridTurnManagerComponent* TurnManager = BoundTurnManager.Get ();
    if (!IsValid (TurnManager) || bRefreshingInitiativeModifiers ||
        TurnManager->InitiativeOrder.IsEmpty ())
    {
        return;
    }

    TGuardValue<bool> Guard (bRefreshingInitiativeModifiers, true);
    const TArray<FGridCombatantInitiativeEntry> InitiativeSnapshot =
        TurnManager->InitiativeOrder;
    for (const FGridCombatantInitiativeEntry& Entry : InitiativeSnapshot)
    {
        int32 Modifier = 0;
        if (Entry.Side == EGridCombatantSide::Party)
        {
            if (IsValid (TurnManager->PartyPawn) &&
                IsValid (TurnManager->PartyPawn->PartyInventoryComponent))
            {
                const TArray<FGridCharacterInventoryState>& Characters =
                    TurnManager->PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
                if (Characters.IsValidIndex (Entry.CharacterIndex))
                {
                    Modifier = FGridStatusEffectInitiativeResolver::ComputeModifier (
                        Characters[Entry.CharacterIndex].StatusEffects);
                }
            }
        }
        else if (Entry.Side == EGridCombatantSide::Monster)
        {
            for (const AGridMonsterActor* Monster : TurnManager->CombatMonsters)
            {
                if (IsValid (Monster) &&
                    Monster->ResolvePersistenceId () == Entry.CombatantId)
                {
                    Modifier = FGridStatusEffectInitiativeResolver::ComputeModifier (
                        Monster->StatusEffects);
                    break;
                }
            }
        }

        TurnManager->SetCombatantInitiativeModifier (
            Entry.Side,
            Entry.CombatantId,
            Modifier);
    }
}

void UGridStatusEffectLifecycleSubsystem::RefreshInitiativeModifierForPartyCharacter (
    int32 CharacterIndex)
{
    UGridTurnManagerComponent* TurnManager = BoundTurnManager.Get ();
    if (!IsValid (TurnManager) || bRefreshingInitiativeModifiers ||
        !IsValid (TurnManager->PartyPawn) ||
        !IsValid (TurnManager->PartyPawn->PartyInventoryComponent))
    {
        return;
    }

    const TArray<FGridCharacterInventoryState>& Characters =
        TurnManager->PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
    if (!Characters.IsValidIndex (CharacterIndex))
    {
        return;
    }

    FGuid CombatantId;
    for (const FGridCombatantInitiativeEntry& Entry : TurnManager->InitiativeOrder)
    {
        if (Entry.Side == EGridCombatantSide::Party &&
            Entry.CharacterIndex == CharacterIndex)
        {
            CombatantId = Entry.CombatantId;
            break;
        }
    }
    if (!CombatantId.IsValid ())
    {
        return;
    }

    const int32 Modifier =
        FGridStatusEffectInitiativeResolver::ComputeModifier (
            Characters[CharacterIndex].StatusEffects);
    TGuardValue<bool> Guard (bRefreshingInitiativeModifiers, true);
    TurnManager->SetCombatantInitiativeModifier (
        EGridCombatantSide::Party,
        CombatantId,
        Modifier);
}

void UGridStatusEffectLifecycleSubsystem::RefreshInitiativeModifierForMonster (
    AGridMonsterActor* Monster)
{
    UGridTurnManagerComponent* TurnManager = BoundTurnManager.Get ();
    if (!IsValid (TurnManager) || bRefreshingInitiativeModifiers ||
        !IsValid (Monster))
    {
        return;
    }

    const FGuid CombatantId = Monster->ResolvePersistenceId ();
    if (!CombatantId.IsValid ())
    {
        return;
    }

    const int32 Modifier =
        FGridStatusEffectInitiativeResolver::ComputeModifier (
            Monster->StatusEffects);
    TGuardValue<bool> Guard (bRefreshingInitiativeModifiers, true);
    TurnManager->SetCombatantInitiativeModifier (
        EGridCombatantSide::Monster,
        CombatantId,
        Modifier);
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
            for (int32 CharacterIndex = 0; CharacterIndex < Characters.Num (); ++CharacterIndex)
            {
                FGridCharacterInventoryState& Character = Characters[CharacterIndex];
                ApplyPeriodicDamageToCharacter (
                    Character,
                    CharacterIndex,
                    EGridStatusEffectDurationUnit::Rounds);
                FGridStatusEffectAdvanceResult AdvanceResult;
                Character.StatusEffects.AdvanceDuration (
                    EGridStatusEffectDurationUnit::Rounds,
                    AdvanceResult);
                RefreshInitiativeModifierForPartyCharacter (CharacterIndex);
            }
        }

        // Existing TurnManager precedence checks party defeat before victory.
        // Preserve that ordering if a round-boundary DoT defeats the party.
        if (TurnManager->bCombatActive && !HasLivingPartyCharacter ())
        {
            TurnManager->ForceDefeat ();
            return;
        }

        const TArray<TObjectPtr<AGridMonsterActor>> MonsterSnapshot =
            TurnManager->CombatMonsters;
        for (AGridMonsterActor* Monster : MonsterSnapshot)
        {
            if (!IsValid (Monster) || Monster->IsDead ())
            {
                continue;
            }
            ApplyPeriodicDamageToMonster (
                Monster,
                EGridStatusEffectDurationUnit::Rounds);
            FGridStatusEffectAdvanceResult AdvanceResult;
            Monster->StatusEffects.AdvanceDuration (
                EGridStatusEffectDurationUnit::Rounds,
                AdvanceResult);
            if (TurnManager->bCombatActive)
            {
                RefreshInitiativeModifierForMonster (Monster);
            }
            if (!TurnManager->bCombatActive)
            {
                return;
            }
        }
    }
}

void UGridStatusEffectLifecycleSubsystem::ApplyPeriodicDamageToCharacter (
    FGridCharacterInventoryState& Character,
    int32 CharacterIndex,
    EGridStatusEffectDurationUnit DurationUnit)
{
    UGridTurnManagerComponent* TurnManager = BoundTurnManager.Get ();
    UGridPartyInventoryComponent* Inventory =
        IsValid (TurnManager) && IsValid (TurnManager->PartyPawn)
            ? TurnManager->PartyPawn->PartyInventoryComponent.Get ()
            : nullptr;
    if (!IsValid (Inventory) || Character.DerivedStats.CurrentHealth <= 0)
    {
        return;
    }

    const FGridDamageResistanceSet Resistances =
        Inventory->ComputeCharacterEquipmentResistances (CharacterIndex);
    for (const FGridStatusEffectRuntimeState& State : Character.StatusEffects.ActiveEffects)
    {
        if (Character.DerivedStats.CurrentHealth <= 0)
        {
            break;
        }
        if (State.DurationUnit != DurationUnit ||
            !IsValid (State.DefinitionAsset) ||
            !State.DefinitionAsset->PeriodicDamage.IsEnabled ())
        {
            continue;
        }

        FGridAttackTargetStats Target;
        Target.CurrentHealth = Character.DerivedStats.CurrentHealth;
        Target.PhysicalArmor = Character.DerivedStats.PhysicalArmor;
        Target.MagicalArmor = Character.DerivedStats.MagicalArmor;
        Target.ResistancePercent = FGridCombatResolver::GetResistancePercent (
            Resistances,
            State.DefinitionAsset->PeriodicDamage.DamageType);
        Target.DamageMultiplier = 1.0f;

        FGridStatusEffectPeriodicDamageResolution Resolution;
        FString Error;
        if (!FGridStatusEffectPeriodicDamageResolver::Resolve (
                State,
                Target,
                Resolution,
                Error))
        {
            UE_LOG (
                LogGridStatusEffects,
                Verbose,
                TEXT ("[MON16.3] PeriodicDamage skipped Target=Party Character=%d Effect=%s Reason=%s"),
                CharacterIndex,
                *State.EffectId.ToString (),
                *Error);
            continue;
        }

        const FGridAttackResult& Damage = Resolution.DamageResult;
        if (Damage.bHit)
        {
            Character.DerivedStats.PhysicalArmor = FMath::Max (
                0,
                Character.DerivedStats.PhysicalArmor - Damage.PhysicalArmorDamage);
            Character.DerivedStats.MagicalArmor = FMath::Max (
                0,
                Character.DerivedStats.MagicalArmor - Damage.MagicalArmorDamage);
            Character.DerivedStats.CurrentHealth = FMath::Max (
                0,
                Character.DerivedStats.CurrentHealth - Damage.HealthDamage);
        }

        UE_LOG (
            LogGridStatusEffects,
            Log,
            TEXT ("[MON16.3] PeriodicDamage Target=Party Character=%d Effect=%s Type=%s Stacks=%d Raw=%d Resistance=%d PhysicalArmor=%d MagicalArmor=%d Health=%d HP=%d->%d"),
            CharacterIndex,
            *Resolution.EffectId.ToString (),
            *UEnum::GetValueAsString (Resolution.DamageType),
            Resolution.StackCount,
            Resolution.RawDamage,
            Damage.ResistancePercent,
            Damage.PhysicalArmorDamage,
            Damage.MagicalArmorDamage,
            Damage.HealthDamage,
            Damage.TargetHealthBefore,
            Damage.TargetHealthAfter);
    }
}

void UGridStatusEffectLifecycleSubsystem::ApplyPeriodicDamageToMonster (
    AGridMonsterActor* Monster,
    EGridStatusEffectDurationUnit DurationUnit)
{
    if (!IsValid (Monster) || Monster->IsDead ())
    {
        return;
    }

    for (const FGridStatusEffectRuntimeState& State : Monster->StatusEffects.ActiveEffects)
    {
        if (Monster->IsDead ())
        {
            break;
        }
        if (State.DurationUnit != DurationUnit ||
            !IsValid (State.DefinitionAsset) ||
            !State.DefinitionAsset->PeriodicDamage.IsEnabled ())
        {
            continue;
        }

        FGridAttackTargetStats Target;
        Target.CurrentHealth = Monster->CurrentHealth;
        Target.PhysicalArmor = Monster->CurrentPhysicalArmor;
        Target.MagicalArmor = Monster->CurrentMagicalArmor;
        Target.ResistancePercent = 0;
        Target.DamageMultiplier = IsValid (Monster->MonsterDefinition)
            ? Monster->MonsterDefinition->GetDamageMultiplier (
                State.DefinitionAsset->PeriodicDamage.DamageType,
                EGridPhysicalDamageSubtype::None)
            : 1.0f;

        FGridStatusEffectPeriodicDamageResolution Resolution;
        FString Error;
        if (!FGridStatusEffectPeriodicDamageResolver::Resolve (
                State,
                Target,
                Resolution,
                Error))
        {
            UE_LOG (
                LogGridStatusEffects,
                Verbose,
                TEXT ("[MON16.3] PeriodicDamage skipped Target=Monster Monster=%s Effect=%s Reason=%s"),
                *GetNameSafe (Monster),
                *State.EffectId.ToString (),
                *Error);
            continue;
        }

        Monster->ApplyAttackResult (Resolution.DamageResult);
        const FGridAttackResult& Damage = Resolution.DamageResult;
        UE_LOG (
            LogGridStatusEffects,
            Log,
            TEXT ("[MON16.3] PeriodicDamage Target=Monster Monster=%s Effect=%s Type=%s Stacks=%d Raw=%d Multiplier=%.3f PhysicalArmor=%d MagicalArmor=%d Health=%d HP=%d->%d"),
            *GetNameSafe (Monster),
            *Resolution.EffectId.ToString (),
            *UEnum::GetValueAsString (Resolution.DamageType),
            Resolution.StackCount,
            Resolution.RawDamage,
            Damage.DamageMultiplier,
            Damage.PhysicalArmorDamage,
            Damage.MagicalArmorDamage,
            Damage.HealthDamage,
            Damage.TargetHealthBefore,
            Damage.TargetHealthAfter);
    }
}

bool UGridStatusEffectLifecycleSubsystem::HasLivingPartyCharacter () const
{
    UGridTurnManagerComponent* TurnManager = BoundTurnManager.Get ();
    if (!IsValid (TurnManager) ||
        !IsValid (TurnManager->PartyPawn) ||
        !IsValid (TurnManager->PartyPawn->PartyInventoryComponent))
    {
        return false;
    }

    for (const FGridCharacterInventoryState& Character :
        TurnManager->PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters)
    {
        if (Character.DerivedStats.CurrentHealth > 0)
        {
            return true;
        }
    }
    return false;
}
