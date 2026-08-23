#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Core/GridDirectionUtils.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "Runtime/Monsters/GridMonsterAudioComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterIdleVariationComponent.h"
#include "Runtime/Monsters/GridMonsterVFXComponent.h"
#include "GridMonsterActor.generated.h"

class AGridMonsterActor;
class AGridLevelRuntimeActor;
class UGridMonsterBehaviorComponent;
class UGridMonsterMovementComponent;
class UGridMonsterAnimInstance;
struct FGridRuntimeMonsterState;

DECLARE_LOG_CATEGORY_EXTERN (LogGridMonsterState, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams (
    FGridMonsterDiedSignature,
    AGridMonsterActor*, Monster,
    FIntPoint, DeathCell);

/**
 * Runtime visual representation of a grid monster.
 *
 * Grid position and combat values are authoritative. Dedicated components own
 * movement, behavior and combat sequencing.
 */
UCLASS (BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API AGridMonsterActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridMonsterActor ()
    {
        PrimaryActorTick.bCanEverTick = false;
        PrimaryActorTick.bStartWithTickEnabled = false;

        SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("SceneRoot"));
        SetRootComponent (SceneRoot);

        CollisionComponent = CreateDefaultSubobject<UBoxComponent> (TEXT ("CollisionComponent"));
        CollisionComponent->SetupAttachment (SceneRoot);
        CollisionComponent->InitBoxExtent (FVector (45.0f, 45.0f, 45.0f));
        CollisionComponent->SetRelativeLocation (FVector (0.0f, 0.0f, 45.0f));
        CollisionComponent->SetCollisionEnabled (ECollisionEnabled::QueryOnly);
        CollisionComponent->SetCollisionObjectType (ECC_WorldDynamic);
        CollisionComponent->SetCollisionResponseToAllChannels (ECR_Ignore);
        CollisionComponent->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
        CollisionComponent->SetGenerateOverlapEvents (false);

        SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent> (TEXT ("SkeletalMeshComponent"));
        SkeletalMeshComponent->SetupAttachment (SceneRoot);
        SkeletalMeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
        SkeletalMeshComponent->SetGenerateOverlapEvents (false);
        SkeletalMeshComponent->SetAnimationMode (EAnimationMode::AnimationBlueprint);
        SkeletalMeshComponent->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

        CombatComponent = CreateDefaultSubobject<UGridMonsterCombatComponent> (TEXT ("MonsterCombat"));
        DeathComponent = CreateDefaultSubobject<UGridMonsterDeathComponent> (TEXT ("MonsterDeath"));
        AudioComponent = CreateDefaultSubobject<UGridMonsterAudioComponent> (TEXT ("MonsterAudio"));
        VFXComponent = CreateDefaultSubobject<UGridMonsterVFXComponent> (TEXT ("MonsterVFX"));
        IdleVariationComponent =
            CreateDefaultSubobject<UGridMonsterIdleVariationComponent> (
                TEXT ("MonsterIdleVariations"));

        SetCanBeDamaged (false);
    }

    virtual void OnConstruction (const FTransform& Transform) override;

    virtual void BeginPlay () override
    {
        Super::BeginPlay ();

        EnsureInitialCombatState ();

        ApplyDefinitionVisuals ();
        ApplyFacingRotation ();
        if (AudioComponent)
        {
            AudioComponent->InitializeMonsterAudio ();
            AudioComponent->RefreshIdleAmbienceScheduling ();
        }
        if (VFXComponent)
        {
            VFXComponent->InitializeMonsterVFX ();
        }
        if (IdleVariationComponent)
        {
            IdleVariationComponent->InitializeIdleVariations ();
            IdleVariationComponent->RefreshIdleVariationScheduling ();
        }
    }

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Components")
    TObjectPtr<UBoxComponent> CollisionComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Components")
    TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Components")
    TObjectPtr<UGridMonsterCombatComponent> CombatComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Components")
    TObjectPtr<UGridMonsterDeathComponent> DeathComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Components")
    TObjectPtr<UGridMonsterAudioComponent> AudioComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Components")
    TObjectPtr<UGridMonsterVFXComponent> VFXComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Components")
    TObjectPtr<UGridMonsterIdleVariationComponent>
        IdleVariationComponent;

    UPROPERTY (BlueprintAssignable, Category = "Monster|Death")
    FGridMonsterDiedSignature OnMonsterDied;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Monster|Definition")
    TObjectPtr<UGridMonsterDefinitionAsset> MonsterDefinition = nullptr;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Identity")
    FGuid SpawnObjectId;

    UPROPERTY (EditInstanceOnly, BlueprintReadOnly, Category = "Monster|Identity")
    FGuid PersistentMonsterId;

    UPROPERTY (EditInstanceOnly, BlueprintReadOnly, Category = "Monster|Persistence")
    FName HomeDungeonLevelId = NAME_None;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Persistence")
    bool bRuntimeLevelActive = true;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Grid")
    FIntPoint CurrentCell = FIntPoint::ZeroValue;

    UPROPERTY (EditInstanceOnly, BlueprintReadOnly, Category = "Monster|Grid")
    EGridEdge Facing = EGridEdge::North;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|State")
    EGridMonsterState MonsterState = EGridMonsterState::Idle;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|State")
    int32 CurrentHealth = 0;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|State")
    int32 CurrentPhysicalArmor = 0;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|State")
    int32 CurrentMagicalArmor = 0;

    /** Runtime-only in MON16.1; persistence is introduced by MON16.7. */
    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Status Effects")
    FGridStatusEffectCollection StatusEffects;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|State")
    bool bCombatStatsInitialized = false;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Animation")
    bool bIsMoving = false;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Animation")
    bool bIsTurning = false;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Animation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MoveAlpha = 0.0f;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Animation", meta = (ClampMin = "-1", ClampMax = "1"))
    int32 TurnDirection = 0;

    /** Editable on directly placed BP_MON_RatGiant instances. */
    UPROPERTY (EditInstanceOnly, BlueprintReadOnly, Category = "Monster|Encounter")
    FName EncounterGroupId = NAME_None;

    /** MON14.2 authored route copied from the MonsterSpawn. Execution begins in MON14.3. */
    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Patrol")
    EGridMonsterPatrolMode PatrolMode = EGridMonsterPatrolMode::None;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Patrol")
    TArray<FGridMonsterPatrolWaypoint> PatrolWaypoints;

    /** Disabled monsters never join or propagate a combat encounter. */
    UPROPERTY (EditInstanceOnly, BlueprintReadOnly, Category = "Monster|Encounter")
    bool bMonsterEnabled = true;

    UFUNCTION (BlueprintCallable, Category = "Monster|Initialization")
    bool EnsureInitialCombatState ();

    bool ValidateMonsterDefinition (FString& OutError) const;

    UFUNCTION (BlueprintCallable, Category = "Monster|Initialization")
    bool InitializeMonster (
        UGridMonsterDefinitionAsset* InDefinition,
        FGuid InSpawnObjectId,
        FIntPoint InCell,
        EGridEdge InFacing = EGridEdge::North,
        FName InEncounterGroupId = NAME_None)
    {
        if (!IsValid (InDefinition) || !InDefinition->IsValidDefinition ())
        {
            return false;
        }

        MonsterDefinition = InDefinition;
        SpawnObjectId = InSpawnObjectId;
        bSpawnObjectIdFromMonsterSpawn = InSpawnObjectId.IsValid ();
        if (!SpawnObjectId.IsValid ())
        {
            UE_LOG (LogGridMonsterState, Error,
                TEXT ("[GridMonsterState] Initialize Monster=%s has no MonsterSpawn ObjectId; a stable PersistentMonsterId is required before capture."),
                *GetNameSafe (this));
        }
        CurrentCell = InCell;
        Facing = InFacing == EGridEdge::None ? EGridEdge::North : InFacing;
        EncounterGroupId = InEncounterGroupId;
        MonsterState = EGridMonsterState::Idle;
        PatrolMode = EGridMonsterPatrolMode::None;
        PatrolWaypoints.Reset ();
        ApplySpawnPlacementConfiguration ();
        CurrentHealth = MonsterDefinition->MaxHealth;
        CurrentPhysicalArmor = FMath::Max (0, MonsterDefinition->PhysicalArmor);
        CurrentMagicalArmor = FMath::Max (0, MonsterDefinition->MagicalArmor);
        StatusEffects.Reset ();
        bCombatStatsInitialized = true;
        ResetAnimationSignals ();

        ApplyFacingRotation ();
        ApplyDefinitionVisuals ();
        if (AudioComponent)
        {
            AudioComponent->InitializeMonsterAudio ();
            AudioComponent->RefreshIdleAmbienceScheduling ();
        }
        if (VFXComponent)
        {
            VFXComponent->InitializeMonsterVFX ();
        }
        if (IdleVariationComponent)
        {
            IdleVariationComponent->InitializeIdleVariations ();
            IdleVariationComponent->
                ResetTransientIdleVariationState ();
            IdleVariationComponent->
                RefreshIdleVariationScheduling ();
        }
        return true;
    }

    UFUNCTION (BlueprintPure, Category = "Monster|Identity")
    FGuid ResolvePersistenceId () const;

    UFUNCTION (BlueprintPure, Category = "Monster|Identity")
    bool HasMonsterSpawnIdentity () const;

    UFUNCTION (BlueprintCallable, CallInEditor, Category = "Monster|Identity")
    void EnsurePersistentMonsterId ();

    UFUNCTION (BlueprintCallable, Category = "Monster|Validation")
    bool ValidatePersistenceSetup (UPARAM (ref) FString& OutError) const;

    bool CaptureRuntimeMonsterState (
        FGridRuntimeMonsterState& OutState,
        FName CurrentLevelId) const;

    bool RestoreRuntimeMonsterState (
        const FGridRuntimeMonsterState& State,
        AGridLevelRuntimeActor* RuntimeActor);

    FName ResolveRuntimeDungeonLevelId (FName CurrentLevelId) const;

    UFUNCTION (BlueprintPure, Category = "Monster|Persistence")
    bool IsRuntimeLevelActive () const { return bRuntimeLevelActive; }

    UFUNCTION (BlueprintCallable, Category = "Monster|Visual")
    bool ApplyDefinitionVisuals ()
    {
        if (!IsValid (MonsterDefinition) || !MonsterDefinition->IsValidDefinition () || !SkeletalMeshComponent)
        {
            return false;
        }

        USkeletalMesh* ResolvedMesh = MonsterDefinition->SkeletalMesh.LoadSynchronous ();
        SkeletalMeshComponent->SetSkeletalMesh (ResolvedMesh);
        SkeletalMeshComponent->SetRelativeLocation (MonsterDefinition->VisualOffset);
        SkeletalMeshComponent->SetRelativeRotation (MonsterDefinition->VisualRotationOffset);
        SkeletalMeshComponent->SetRelativeScale3D (MonsterDefinition->VisualScale);

        if (MonsterDefinition->AnimationClass)
        {
            SkeletalMeshComponent->SetAnimationMode (EAnimationMode::AnimationBlueprint);
            SkeletalMeshComponent->SetAnimInstanceClass (MonsterDefinition->AnimationClass.Get ());
        }
        else
        {
            SkeletalMeshComponent->SetAnimInstanceClass (nullptr);
        }

        return true;
    }

    UFUNCTION (BlueprintCallable, Category = "Monster|Validation")
    bool ValidatePresentationSetup (UPARAM (ref) FString& OutError) const
    {
        TArray<FString> Errors;

        if (!IsValid (MonsterDefinition))
        {
            Errors.Add (TEXT ("MonsterDefinition is not assigned."));
        }
        else
        {
            FString DefinitionError;
            if (!MonsterDefinition->ValidateDefinition (DefinitionError))
            {
                Errors.Add (FString::Printf (TEXT ("MonsterDefinition is invalid: %s"), *DefinitionError));
            }

            if (MonsterDefinition->SkeletalMesh.IsNull ())
            {
                Errors.Add (TEXT ("MonsterDefinition.SkeletalMesh is not assigned."));
            }

            if (!MonsterDefinition->AnimationClass)
            {
                Errors.Add (TEXT ("MonsterDefinition.AnimationClass is not assigned."));
            }
        }

        if (!SkeletalMeshComponent)
        {
            Errors.Add (TEXT ("SkeletalMeshComponent is missing."));
        }
        else if (!SkeletalMeshComponent->GetSkeletalMeshAsset ())
        {
            Errors.Add (TEXT ("SkeletalMeshComponent has no resolved skeletal mesh."));
        }

        if (!CombatComponent)
        {
            Errors.Add (TEXT ("MonsterCombat component is missing."));
        }

        if (!DeathComponent)
        {
            Errors.Add (TEXT ("MonsterDeath component is missing."));
        }

        if (Facing == EGridEdge::None)
        {
            Errors.Add (TEXT ("Facing must be a cardinal grid direction."));
        }

        OutError = FString::Join (Errors, TEXT ("\n"));
        return Errors.Num () == 0;
    }

    UFUNCTION (BlueprintCallable, Category = "Monster|Grid")
    void SetGridPose (FIntPoint InCell, EGridEdge InFacing)
    {
        CurrentCell = InCell;
        Facing = InFacing == EGridEdge::None ? EGridEdge::North : InFacing;
        ApplyFacingRotation ();
    }

    UFUNCTION (BlueprintCallable, Category = "Monster|Grid")
    void ApplyFacingRotation ()
    {
        const EGridEdge SafeFacing = Facing == EGridEdge::None ? EGridEdge::North : Facing;
        SetActorRotation (FRotator (0.0f, GridDirectionUtils::ToYaw (SafeFacing), 0.0f));
    }

    UFUNCTION (BlueprintCallable, Category = "Monster|State")
    void SetMonsterState (EGridMonsterState InState)
    {
        if (MonsterState == EGridMonsterState::Dead &&
            InState != EGridMonsterState::Dead)
        {
            return;
        }

        if (InState == EGridMonsterState::Dead)
        {
            MarkDead ();
            return;
        }

        MonsterState = InState;
        if (CollisionComponent)
        {
            CollisionComponent->SetCollisionEnabled (ECollisionEnabled::QueryOnly);
        }
        if (AudioComponent)
        {
            AudioComponent->RefreshIdleAmbienceScheduling ();
        }
        if (IdleVariationComponent)
        {
            IdleVariationComponent->
                RefreshIdleVariationScheduling ();
        }
    }

    UFUNCTION (BlueprintCallable, Category = "Monster|State")
    void SetCurrentHealth (int32 InCurrentHealth)
    {
        const int32 MaxHealth = MonsterDefinition ? FMath::Max (1, MonsterDefinition->MaxHealth) : MAX_int32;
        CurrentHealth = FMath::Clamp (InCurrentHealth, 0, MaxHealth);

        if (CurrentHealth <= 0)
        {
            MarkDead ();
        }
    }

    UFUNCTION (BlueprintCallable, Category = "Monster|State")
    void SetCurrentPhysicalArmor (int32 InArmor)
    {
        CurrentPhysicalArmor = FMath::Max (0, InArmor);
        bCombatStatsInitialized = true;
    }

    UFUNCTION (BlueprintCallable, Category = "Monster|State")
    void SetCurrentMagicalArmor (int32 InArmor)
    {
        CurrentMagicalArmor = FMath::Max (0, InArmor);
        bCombatStatsInitialized = true;
    }

    UFUNCTION (BlueprintCallable, Category = "Monster|Combat")
    void ApplyAttackResult (const FGridAttackResult& Result)
    {
        if (IsDead () || !Result.bHit)
        {
            return;
        }

        CurrentPhysicalArmor = FMath::Max (
            0,
            CurrentPhysicalArmor - Result.PhysicalArmorDamage);
        CurrentMagicalArmor = FMath::Max (
            0,
            CurrentMagicalArmor - Result.MagicalArmorDamage);
        CurrentHealth = FMath::Max (0, CurrentHealth - Result.HealthDamage);

        if (CurrentHealth <= 0)
        {
            MarkDead ();
        }
        else if (Result.GetTotalAppliedDamage () > 0)
        {
            SetMonsterState (EGridMonsterState::Hurt);
            if (AudioComponent)
            {
                AudioComponent->PlayHurt ();
            }
            if (VFXComponent)
            {
                VFXComponent->PlayHurtVFX (Result);
            }
            StartHurtPresentation ();
        }
    }

    UFUNCTION (BlueprintCallable, Category = "Monster|State")
    void MarkDead ();

    /** Starts the optional hit-reaction montage without owning gameplay timing. */
    UFUNCTION (BlueprintCallable, Category = "Monster|Animation")
    bool StartHurtPresentation ();

    /** Stops only the configured hit-reaction montage; gameplay state is unchanged. */
    UFUNCTION (BlueprintCallable, Category = "Monster|Animation")
    void StopHurtPresentation (float BlendOutTime = 0.10f);

    UFUNCTION (BlueprintCallable, Category = "Monster|Animation")
    void SetMovementAnimationState (bool bInMoving, float InMoveAlpha = 0.0f)
    {
        const bool bWasMoving = bIsMoving;
        bIsMoving = bInMoving && MonsterState != EGridMonsterState::Dead;
        MoveAlpha = bIsMoving ? FMath::Clamp (InMoveAlpha, 0.0f, 1.0f) : 0.0f;
        if (bWasMoving != bIsMoving &&
            IdleVariationComponent)
        {
            IdleVariationComponent->
                RefreshIdleVariationScheduling ();
        }
    }

    UFUNCTION (BlueprintCallable, Category = "Monster|Animation")
    void SetTurnAnimationState (int32 InTurnDirection)
    {
        const bool bWasTurning = bIsTurning;
        TurnDirection = MonsterState == EGridMonsterState::Dead
            ? 0
            : FMath::Clamp (InTurnDirection, -1, 1);
        bIsTurning = TurnDirection != 0;
        if (bWasTurning != bIsTurning &&
            IdleVariationComponent)
        {
            IdleVariationComponent->
                RefreshIdleVariationScheduling ();
        }
    }

    UFUNCTION (BlueprintCallable, Category = "Monster|Animation")
    void ResetAnimationSignals ()
    {
        StopHurtPresentation (0.0f);
        bIsMoving = false;
        bIsTurning = false;
        MoveAlpha = 0.0f;
        TurnDirection = 0;
    }

    UFUNCTION (BlueprintPure, Category = "Monster|State")
    bool IsDead () const
    {
        return MonsterState == EGridMonsterState::Dead ||
            (DeathComponent && DeathComponent->bDeathCommitted) ||
            (bCombatStatsInitialized && CurrentHealth <= 0);
    }

    UFUNCTION (BlueprintPure, Category = "Monster|State")
    int32 GetMaxHealth () const
    {
        return MonsterDefinition ? MonsterDefinition->MaxHealth : 0;
    }

    UFUNCTION (BlueprintPure, Category = "Monster|Animation")
    UGridMonsterAnimInstance* GetGridMonsterAnimInstance () const;

private:
    void ApplySpawnPlacementConfiguration ();

    UPROPERTY (Transient)
    bool bSpawnObjectIdFromMonsterSpawn = false;
};

/**
 * Native bridge used as the parent class of every monster Animation Blueprint.
 * It mirrors gameplay state from AGridMonsterActor without owning gameplay logic.
 */
UCLASS (Transient, BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridMonsterAnimInstance : public UAnimInstance
{
    GENERATED_BODY ()

public:
    virtual void NativeInitializeAnimation () override
    {
        Super::NativeInitializeAnimation ();
        CachedMonsterActor = Cast<AGridMonsterActor> (GetOwningActor ());
        SynchronizeFromMonster ();
    }

    virtual void NativeUpdateAnimation (float DeltaSeconds) override
    {
        Super::NativeUpdateAnimation (DeltaSeconds);

        if (!IsValid (CachedMonsterActor))
        {
            CachedMonsterActor = Cast<AGridMonsterActor> (GetOwningActor ());
        }

        SynchronizeFromMonster ();
    }

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Animation")
    EGridMonsterState MonsterState = EGridMonsterState::Idle;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Animation")
    bool bIsMoving = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Animation")
    bool bIsTurning = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Animation")
    bool bIsDead = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Animation")
    float MoveAlpha = 0.0f;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Animation")
    int32 TurnDirection = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Animation")
    int32 CurrentHealth = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Animation")
    int32 MaxHealth = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Animation")
    FIntPoint CurrentCell = FIntPoint::ZeroValue;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Animation")
    EGridEdge Facing = EGridEdge::North;

    UFUNCTION (BlueprintPure, Category = "Monster|Animation")
    AGridMonsterActor* GetMonsterActor () const
    {
        return CachedMonsterActor;
    }

private:
    UPROPERTY (Transient)
    TObjectPtr<AGridMonsterActor> CachedMonsterActor = nullptr;

    void SynchronizeFromMonster ()
    {
        if (!IsValid (CachedMonsterActor))
        {
            MonsterState = EGridMonsterState::Idle;
            bIsMoving = false;
            bIsTurning = false;
            bIsDead = false;
            MoveAlpha = 0.0f;
            TurnDirection = 0;
            CurrentHealth = 0;
            MaxHealth = 0;
            CurrentCell = FIntPoint::ZeroValue;
            Facing = EGridEdge::North;
            return;
        }

        MonsterState = CachedMonsterActor->MonsterState;
        bIsMoving = CachedMonsterActor->bIsMoving;
        bIsTurning = CachedMonsterActor->bIsTurning;
        bIsDead = CachedMonsterActor->IsDead ();
        MoveAlpha = CachedMonsterActor->MoveAlpha;
        TurnDirection = CachedMonsterActor->TurnDirection;
        CurrentHealth = CachedMonsterActor->CurrentHealth;
        MaxHealth = CachedMonsterActor->GetMaxHealth ();
        CurrentCell = CachedMonsterActor->CurrentCell;
        Facing = CachedMonsterActor->Facing;
    }
};

inline UGridMonsterAnimInstance* AGridMonsterActor::GetGridMonsterAnimInstance () const
{
    return SkeletalMeshComponent
        ? Cast<UGridMonsterAnimInstance> (SkeletalMeshComponent->GetAnimInstance ())
        : nullptr;
}