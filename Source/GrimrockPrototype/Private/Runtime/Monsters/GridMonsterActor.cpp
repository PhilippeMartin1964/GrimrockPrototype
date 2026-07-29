#include "Runtime/Monsters/GridMonsterActor.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

DEFINE_LOG_CATEGORY (LogGridMonsterState);

namespace
{
    bool IsCardinalMonsterFacing (EGridEdge Facing)
    {
        return Facing == EGridEdge::North ||
            Facing == EGridEdge::East ||
            Facing == EGridEdge::South ||
            Facing == EGridEdge::West;
    }

    AGridLevelRuntimeActor* FindMonsterRuntimeActor (
        const AGridMonsterActor* Monster)
    {
        if (!IsValid (Monster))
        {
            return nullptr;
        }

        if (AGridLevelRuntimeActor* OwnerRuntime =
            Cast<AGridLevelRuntimeActor> (Monster->GetOwner ()))
        {
            return OwnerRuntime;
        }

        UWorld* World = Monster->GetWorld ();
        if (!World)
        {
            return nullptr;
        }

        for (TActorIterator<AGridLevelRuntimeActor> It (World); It; ++It)
        {
            return *It;
        }
        return nullptr;
    }

    bool RuntimeHasMonsterSpawn (
        const AGridLevelRuntimeActor* RuntimeActor,
        FGuid SpawnObjectId)
    {
        if (!IsValid (RuntimeActor) ||
            !RuntimeActor->LevelAsset ||
            !SpawnObjectId.IsValid ())
        {
            return false;
        }

        return RuntimeActor->LevelAsset->Objects.ContainsByPredicate (
            [SpawnObjectId] (const FGridLevelObjectData& ObjectData)
            {
                return ObjectData.Type == EGridLevelObjectType::MonsterSpawn &&
                    ObjectData.ObjectId == SpawnObjectId;
            });
    }

    int32 CountEnabledDungeonLevels (
        const AGridLevelRuntimeActor* RuntimeActor)
    {
        if (!IsValid (RuntimeActor) || !RuntimeActor->DungeonAsset)
        {
            return 1;
        }

        int32 Count = 0;
        for (const FGridDungeonLevelEntry& Entry :
            RuntimeActor->DungeonAsset->Levels)
        {
            Count += Entry.bEnabled ? 1 : 0;
        }
        return Count;
    }

    EGridMonsterState NormalizeRestoredMonsterState (
        EGridMonsterState SavedState,
        bool bHasLastKnownPartyCell)
    {
        switch (SavedState)
        {
            case EGridMonsterState::Attacking:
            case EGridMonsterState::Repositioning:
                return bHasLastKnownPartyCell
                    ? EGridMonsterState::Pursuing
                    : EGridMonsterState::Alert;

            case EGridMonsterState::Hurt:
                return bHasLastKnownPartyCell
                    ? EGridMonsterState::Alert
                    : EGridMonsterState::Idle;

            case EGridMonsterState::Dead:
                return EGridMonsterState::Idle;

            default:
                return SavedState;
        }
    }

    FString GetMonsterStateText (EGridMonsterState State)
    {
        if (const UEnum* StateEnum = StaticEnum<EGridMonsterState> ())
        {
            return StateEnum->GetNameStringByValue (
                static_cast<int64> (State));
        }
        return TEXT ("Unknown");
    }
}

void AGridMonsterActor::OnConstruction (const FTransform& Transform)
{
    Super::OnConstruction (Transform);

#if WITH_EDITOR
    EnsurePersistentMonsterId ();
#endif

    ApplyDefinitionVisuals ();
    ApplyFacingRotation ();
    if (DeathComponent)
    {
        DeathComponent->InitializeDeathComponent ();
    }
}

bool AGridMonsterActor::EnsureInitialCombatState ()
{
    if (!IsValid (MonsterDefinition) ||
        !MonsterDefinition->IsValidDefinition () ||
        MonsterState == EGridMonsterState::Dead ||
        (DeathComponent && DeathComponent->bDeathCommitted))
    {
        return false;
    }

    if (bCombatStatsInitialized)
    {
        return true;
    }

    CurrentHealth = MonsterDefinition->MaxHealth;
    CurrentPhysicalArmor =
        FMath::Max (0, MonsterDefinition->PhysicalArmor);
    CurrentMagicalArmor =
        FMath::Max (0, MonsterDefinition->MagicalArmor);
    bCombatStatsInitialized = true;
    ResetAnimationSignals ();

    if (DeathComponent)
    {
        DeathComponent->RestoreLivingState ();
    }

    UE_LOG (LogGridMonsterState, Log,
        TEXT ("[GridMonsterState] InitializeFresh Monster=%s HP=%d PhysicalArmor=%d MagicalArmor=%d State=%s"),
        *GetNameSafe (this),
        CurrentHealth,
        CurrentPhysicalArmor,
        CurrentMagicalArmor,
        *GetMonsterStateText (MonsterState));
    return true;
}

FGuid AGridMonsterActor::ResolvePersistenceId () const
{
    if (HasMonsterSpawnIdentity ())
    {
        return SpawnObjectId;
    }

    return PersistentMonsterId;
}

bool AGridMonsterActor::HasMonsterSpawnIdentity () const
{
    if (!SpawnObjectId.IsValid ())
    {
        return false;
    }

    if (bSpawnObjectIdFromMonsterSpawn)
    {
        return true;
    }

    return RuntimeHasMonsterSpawn (
        FindMonsterRuntimeActor (this),
        SpawnObjectId);
}

void AGridMonsterActor::EnsurePersistentMonsterId ()
{
#if WITH_EDITOR
    if (PersistentMonsterId.IsValid () ||
        IsTemplate () ||
        HasAnyFlags (RF_ClassDefaultObject | RF_Transient))
    {
        return;
    }

    UWorld* World = GetWorld ();
    if (!World || World->IsGameWorld ())
    {
        return;
    }

    Modify ();
    PersistentMonsterId = FGuid::NewGuid ();
    MarkPackageDirty ();
#endif
}

FName AGridMonsterActor::ResolveRuntimeDungeonLevelId (
    FName CurrentLevelId) const
{
    if (!HomeDungeonLevelId.IsNone ())
    {
        return HomeDungeonLevelId;
    }

    const AGridLevelRuntimeActor* RuntimeActor =
        FindMonsterRuntimeActor (this);
    if (RuntimeHasMonsterSpawn (RuntimeActor, SpawnObjectId) ||
        CountEnabledDungeonLevels (RuntimeActor) <= 1)
    {
        return CurrentLevelId;
    }

    return NAME_None;
}

bool AGridMonsterActor::ValidatePersistenceSetup (
    FString& OutError) const
{
    TArray<FString> Errors;
    const FGuid PersistenceId = ResolvePersistenceId ();
    if (!PersistenceId.IsValid ())
    {
        Errors.Add (TEXT ("PersistentMonsterId is invalid and no MonsterSpawn ObjectId is available."));
    }

    const AGridLevelRuntimeActor* RuntimeActor =
        FindMonsterRuntimeActor (this);
    const FName CurrentLevelId = RuntimeActor
        ? RuntimeActor->CurrentDungeonLevelId
        : NAME_None;
    const FName ResolvedLevelId =
        ResolveRuntimeDungeonLevelId (CurrentLevelId);

    if (CountEnabledDungeonLevels (RuntimeActor) > 1 &&
        !HasMonsterSpawnIdentity () &&
        HomeDungeonLevelId.IsNone ())
    {
        Errors.Add (TEXT ("HomeDungeonLevelId is required for a directly placed monster in a multi-level dungeon."));
    }

    if (PersistenceId.IsValid () && GetWorld ())
    {
        for (TActorIterator<AGridMonsterActor> It (GetWorld ()); It; ++It)
        {
            const AGridMonsterActor* Other = *It;
            if (!IsValid (Other) ||
                Other == this ||
                Other->ResolvePersistenceId () != PersistenceId)
            {
                continue;
            }

            if (Other->ResolveRuntimeDungeonLevelId (CurrentLevelId) ==
                ResolvedLevelId)
            {
                Errors.Add (FString::Printf (
                    TEXT ("PersistenceId %s is duplicated by %s in level %s."),
                    *PersistenceId.ToString (),
                    *GetNameSafe (Other),
                    *ResolvedLevelId.ToString ()));
                break;
            }
        }
    }

    OutError = FString::Join (Errors, TEXT ("\n"));
    return Errors.IsEmpty ();
}

bool AGridMonsterActor::CaptureRuntimeMonsterState (
    FGridRuntimeMonsterState& OutState,
    FName CurrentLevelId) const
{
    OutState = FGridRuntimeMonsterState ();

    const FGuid PersistenceId = ResolvePersistenceId ();
    if (!PersistenceId.IsValid ())
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterState] Capture skipped Level=%s Monster=%s Reason=InvalidPersistenceId"),
            *CurrentLevelId.ToString (),
            *GetNameSafe (this));
        return false;
    }

    const FName MonsterLevelId =
        ResolveRuntimeDungeonLevelId (CurrentLevelId);
    if (MonsterLevelId.IsNone ())
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterState] Capture skipped Level=%s Monster=%s PersistenceId=%s Reason=MissingHomeDungeonLevelId"),
            *CurrentLevelId.ToString (),
            *GetNameSafe (this),
            *PersistenceId.ToString ());
        return false;
    }
    if (MonsterLevelId != CurrentLevelId)
    {
        return false;
    }

    if (!IsValid (MonsterDefinition) ||
        !MonsterDefinition->IsValidDefinition () ||
        MonsterDefinition->MonsterId.IsNone ())
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterState] Capture skipped Level=%s Monster=%s PersistenceId=%s Reason=InvalidDefinition"),
            *CurrentLevelId.ToString (),
            *GetNameSafe (this),
            *PersistenceId.ToString ());
        return false;
    }

    const AGridLevelRuntimeActor* RuntimeActor =
        FindMonsterRuntimeActor (this);
    if (RuntimeActor &&
        (!RuntimeActor->IsValidCell (CurrentCell.X, CurrentCell.Y) ||
            !RuntimeActor->IsWalkableCell (CurrentCell.X, CurrentCell.Y)))
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterState] Capture skipped Level=%s Monster=%s PersistenceId=%s Cell=(%d,%d) Reason=InvalidCell"),
            *CurrentLevelId.ToString (),
            *GetNameSafe (this),
            *PersistenceId.ToString (),
            CurrentCell.X,
            CurrentCell.Y);
        return false;
    }

    const bool bDead = IsDead ();
    const UGridMonsterBehaviorComponent* Behavior =
        FindComponentByClass<UGridMonsterBehaviorComponent> ();

    OutState.PersistenceId = PersistenceId;
    OutState.SpawnObjectId =
        HasMonsterSpawnIdentity () ? SpawnObjectId : FGuid ();
    OutState.MonsterDefinitionId = MonsterDefinition->MonsterId;
    OutState.DungeonLevelId = MonsterLevelId;
    OutState.CellX = CurrentCell.X;
    OutState.CellY = CurrentCell.Y;
    OutState.Facing =
        IsCardinalMonsterFacing (Facing) ? Facing : EGridEdge::North;
    OutState.MonsterState =
        bDead ? EGridMonsterState::Dead : MonsterState;
    OutState.CurrentHealth = bDead
        ? 0
        : FMath::Clamp (
            CurrentHealth,
            1,
            FMath::Max (1, MonsterDefinition->MaxHealth));
    OutState.CurrentPhysicalArmor = FMath::Clamp (
        CurrentPhysicalArmor,
        0,
        FMath::Max (0, MonsterDefinition->PhysicalArmor));
    OutState.CurrentMagicalArmor = FMath::Clamp (
        CurrentMagicalArmor,
        0,
        FMath::Max (0, MonsterDefinition->MagicalArmor));
    OutState.bMonsterEnabled = bMonsterEnabled;
    OutState.EncounterGroupId = EncounterGroupId;
    OutState.bHasLastKnownPartyCell =
        Behavior && Behavior->bHasLastKnownPartyCell;
    OutState.LastKnownPartyCell = Behavior
        ? Behavior->LastKnownPartyCell
        : FIntPoint::ZeroValue;
    OutState.bIsDead = bDead;

    UE_LOG (LogGridMonsterState, Log,
        TEXT ("[GridMonsterState] Capture Level=%s Monster=%s PersistenceId=%s SpawnObjectId=%s Cell=(%d,%d) State=%s HP=%d Dead=%s"),
        *CurrentLevelId.ToString (),
        *GetNameSafe (this),
        *PersistenceId.ToString (),
        *OutState.SpawnObjectId.ToString (),
        OutState.CellX,
        OutState.CellY,
        *GetMonsterStateText (OutState.MonsterState),
        OutState.CurrentHealth,
        bDead ? TEXT ("true") : TEXT ("false"));
    return true;
}

bool AGridMonsterActor::RestoreRuntimeMonsterState (
    const FGridRuntimeMonsterState& State,
    AGridLevelRuntimeActor* RuntimeActor)
{
    const FGuid PersistenceId = ResolvePersistenceId ();
    if (!IsValid (RuntimeActor) ||
        !PersistenceId.IsValid () ||
        State.PersistenceId != PersistenceId)
    {
        return false;
    }

    if (AudioComponent)
    {
        AudioComponent->InitializeMonsterAudio ();
        AudioComponent->ResetTransientAudioState ();
    }
    if (VFXComponent)
    {
        VFXComponent->InitializeMonsterVFX ();
        VFXComponent->ResetTransientVFXState ();
    }
    if (IdleVariationComponent)
    {
        IdleVariationComponent->InitializeIdleVariations ();
        IdleVariationComponent->
            ResetTransientIdleVariationState ();
    }

    const FName DefinitionId = MonsterDefinition
        ? MonsterDefinition->MonsterId
        : NAME_None;
    if (!IsValid (MonsterDefinition) ||
        DefinitionId != State.MonsterDefinitionId)
    {
        UE_LOG (LogGridMonsterState, Warning,
            TEXT ("[GridMonsterState] DefinitionMismatch Level=%s Monster=%s PersistenceId=%s Saved=%s Actor=%s"),
            *State.DungeonLevelId.ToString (),
            *GetNameSafe (this),
            *PersistenceId.ToString (),
            *State.MonsterDefinitionId.ToString (),
            *DefinitionId.ToString ());
        return false;
    }

    const FIntPoint RestoredCell (State.CellX, State.CellY);
    if (!RuntimeActor->IsValidCell (RestoredCell.X, RestoredCell.Y) ||
        !RuntimeActor->IsWalkableCell (RestoredCell.X, RestoredCell.Y))
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterState] Restore skipped Level=%s Monster=%s PersistenceId=%s Cell=(%d,%d) Reason=InvalidCell"),
            *State.DungeonLevelId.ToString (),
            *GetNameSafe (this),
            *PersistenceId.ToString (),
            RestoredCell.X,
            RestoredCell.Y);
        return false;
    }

    if (CombatComponent)
    {
        CombatComponent->CancelAttackPresentation ();
    }

    UGridMonsterMovementComponent* Movement =
        FindComponentByClass<UGridMonsterMovementComponent> ();
    if (Movement)
    {
        Movement->CancelCurrentAction ();
        Movement->ReleaseOccupancy ();
    }

    UGridMonsterOccupancySubsystem* Occupancy =
        GetWorld ()
            ? GetWorld ()->GetSubsystem<UGridMonsterOccupancySubsystem> ()
            : nullptr;
    if (Occupancy)
    {
        Occupancy->UnregisterMonster (this);
    }

    ResetAnimationSignals ();
    SetActorHiddenInGame (false);
    SetActorEnableCollision (true);
    if (SkeletalMeshComponent)
    {
        SkeletalMeshComponent->SetVisibility (true, true);
    }

    CurrentCell = RestoredCell;
    Facing = IsCardinalMonsterFacing (State.Facing)
        ? State.Facing
        : EGridEdge::North;
    CurrentPhysicalArmor = FMath::Clamp (
        State.CurrentPhysicalArmor,
        0,
        FMath::Max (0, MonsterDefinition->PhysicalArmor));
    CurrentMagicalArmor = FMath::Clamp (
        State.CurrentMagicalArmor,
        0,
        FMath::Max (0, MonsterDefinition->MagicalArmor));
    bCombatStatsInitialized = true;
    bMonsterEnabled = State.bMonsterEnabled;
    EncounterGroupId = State.EncounterGroupId;
    bRuntimeLevelActive = true;
    SetActorLocation (RuntimeActor->GetCellCenterWorld (
        CurrentCell.X,
        CurrentCell.Y));
    ApplyFacingRotation ();

    UGridMonsterBehaviorComponent* Behavior =
        FindComponentByClass<UGridMonsterBehaviorComponent> ();
    if (Behavior)
    {
        Behavior->bCanSeeParty = false;
        Behavior->bCanHearParty = false;
        Behavior->bHasLastKnownPartyCell =
            State.bHasLastKnownPartyCell;
        Behavior->LastKnownPartyCell = State.LastKnownPartyCell;
        Behavior->bLastPathFound = false;
        Behavior->LastPath.Reset ();
        Behavior->LastPathGoal = FIntPoint::ZeroValue;
        Behavior->LastVisitedCellCount = 0;
    }

    if (DeathComponent)
    {
        DeathComponent->InitializeDeathComponent (RuntimeActor);
    }

    const bool bRestoreDead =
        State.bIsDead ||
        State.CurrentHealth <= 0 ||
        State.MonsterState == EGridMonsterState::Dead;
    if (bRestoreDead)
    {
        CurrentHealth = 0;
        MonsterState = EGridMonsterState::Dead;
        if (DeathComponent)
        {
            DeathComponent->RestoreCommittedDeathState (
                RestoredCell,
                true);
        }
        else if (CollisionComponent)
        {
            CollisionComponent->SetCollisionEnabled (
                ECollisionEnabled::NoCollision);
        }

        UE_LOG (LogGridMonsterState, Log,
            TEXT ("[GridMonsterState] RestoreDead Level=%s Monster=%s PersistenceId=%s Cell=(%d,%d)"),
            *State.DungeonLevelId.ToString (),
            *GetNameSafe (this),
            *PersistenceId.ToString (),
            CurrentCell.X,
            CurrentCell.Y);
        return true;
    }

    if (DeathComponent)
    {
        DeathComponent->RestoreLivingState ();
    }

    CurrentHealth = FMath::Clamp (
        State.CurrentHealth,
        1,
        FMath::Max (1, MonsterDefinition->MaxHealth));
    MonsterState = NormalizeRestoredMonsterState (
        State.MonsterState,
        State.bHasLastKnownPartyCell);

    bool bOccupationRestored = true;
    if (bMonsterEnabled)
    {
        if (CollisionComponent)
        {
            CollisionComponent->SetCollisionEnabled (
                ECollisionEnabled::QueryOnly);
        }

        if (Movement)
        {
            bOccupationRestored =
                Movement->InitializeMovement (RuntimeActor);
        }
        else
        {
            bOccupationRestored =
                Occupancy &&
                Occupancy->RegisterMonster (this, CurrentCell);
        }

        if (Behavior)
        {
            Behavior->Activate ();
            Behavior->InitializeBehavior (RuntimeActor, nullptr);
            Behavior->bCanSeeParty = false;
            Behavior->bCanHearParty = false;
            Behavior->bHasLastKnownPartyCell =
                State.bHasLastKnownPartyCell;
            Behavior->LastKnownPartyCell =
                State.LastKnownPartyCell;
            Behavior->LastPath.Reset ();
            Behavior->bLastPathFound = false;
        }
    }
    else
    {
        if (CollisionComponent)
        {
            CollisionComponent->SetCollisionEnabled (
                ECollisionEnabled::NoCollision);
        }
        if (Behavior)
        {
            Behavior->Deactivate ();
        }
    }

    if (!bOccupationRestored)
    {
        if (CollisionComponent)
        {
            CollisionComponent->SetCollisionEnabled (
                ECollisionEnabled::NoCollision);
        }
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterState] RestoreAlive Level=%s Monster=%s PersistenceId=%s Cell=(%d,%d) Result=OccupancyConflict"),
            *State.DungeonLevelId.ToString (),
            *GetNameSafe (this),
            *PersistenceId.ToString (),
            CurrentCell.X,
            CurrentCell.Y);
        return false;
    }

    UE_LOG (LogGridMonsterState, Log,
        TEXT ("[GridMonsterState] RestoreAlive Level=%s Monster=%s PersistenceId=%s Cell=(%d,%d) State=%s HP=%d Enabled=%s"),
        *State.DungeonLevelId.ToString (),
        *GetNameSafe (this),
        *PersistenceId.ToString (),
        CurrentCell.X,
        CurrentCell.Y,
        *GetMonsterStateText (MonsterState),
        CurrentHealth,
        bMonsterEnabled ? TEXT ("true") : TEXT ("false"));
    if (AudioComponent)
    {
        AudioComponent->RefreshIdleAmbienceScheduling ();
    }
    if (IdleVariationComponent)
    {
        IdleVariationComponent->
            RefreshIdleVariationScheduling ();
    }
    return true;
}

void AGridMonsterActor::MarkDead ()
{
    if (IdleVariationComponent)
    {
        IdleVariationComponent->StopIdleVariations ();
    }
    if (AudioComponent)
    {
        AudioComponent->StopIdleAmbience ();
    }
    CurrentHealth = 0;
    MonsterState = EGridMonsterState::Dead;
    ResetAnimationSignals ();

    if (CombatComponent)
    {
        CombatComponent->CancelAttackPresentation ();
    }

    if (DeathComponent)
    {
        DeathComponent->CommitDeath ();
    }
    else if (CollisionComponent)
    {
        CollisionComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    }
}
