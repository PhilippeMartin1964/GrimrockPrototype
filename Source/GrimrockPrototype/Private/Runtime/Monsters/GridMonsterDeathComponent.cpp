#include "Runtime/Monsters/GridMonsterDeathComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterAudioComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterLootResolver.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC (LogGridMonsterDeath, Log, All);
DEFINE_LOG_CATEGORY_STATIC (LogGridMonsterLoot, Log, All);

namespace
{
    constexpr uint32 MON8LootSeedSalt = 0x4D4F4E38u;

    int32 BuildMON8LootSeed (const AGridMonsterActor* Monster)
    {
        uint32 Seed = MON8LootSeedSalt;
        if (Monster)
        {
            Seed = HashCombine (
                Seed,
                GetTypeHash (Monster->ResolvePersistenceId ()));
            Seed = HashCombine (
                Seed,
                GetTypeHash (Monster->MonsterDefinition
                    ? Monster->MonsterDefinition->MonsterId
                    : NAME_None));
        }
        return static_cast<int32> (Seed);
    }

    FVector GetMON8LootLocalOffset (int32 LootIndex)
    {
        static const FVector Offsets[] = {
            FVector (0.0f, 0.0f, 0.0f),
            FVector (20.0f, 0.0f, 0.0f),
            FVector (-20.0f, 0.0f, 0.0f),
            FVector (0.0f, 20.0f, 0.0f),
            FVector (0.0f, -20.0f, 0.0f),
            FVector (20.0f, 20.0f, 0.0f),
            FVector (-20.0f, 20.0f, 0.0f),
            FVector (20.0f, -20.0f, 0.0f),
            FVector (-20.0f, -20.0f, 0.0f)
        };
        return Offsets[LootIndex % UE_ARRAY_COUNT (Offsets)];
    }
}

UGridMonsterDeathComponent::UGridMonsterDeathComponent ()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UGridMonsterDeathComponent::InitializeDeathComponent (
    AGridLevelRuntimeActor* InRuntimeActor)
{
    if (!IsValid (OwnerMonster))
    {
        OwnerMonster = Cast<AGridMonsterActor> (GetOwner ());
    }
    if (!IsValid (OwnerMonster))
    {
        return false;
    }

    RuntimeActor = IsValid (InRuntimeActor) ? InRuntimeActor : nullptr;
    if (!RuntimeActor)
    {
        if (const UGridMonsterMovementComponent* Movement =
            OwnerMonster->FindComponentByClass<UGridMonsterMovementComponent> ())
        {
            RuntimeActor = Movement->RuntimeActor;
        }
    }
    if (!RuntimeActor)
    {
        RuntimeActor = FindRuntimeActor ();
    }
    return true;
}

bool UGridMonsterDeathComponent::CommitDeath ()
{
    if (bDeathCommitted)
    {
        return false;
    }
    if (!InitializeDeathComponent (RuntimeActor))
    {
        return false;
    }

    // Commit the guard before calling any external gameplay hook.
    bDeathCommitted = true;
    DeathCell = OwnerMonster->CurrentCell;

    if (UGridMonsterCombatComponent* Combat =
        OwnerMonster->FindComponentByClass<UGridMonsterCombatComponent> ())
    {
        Combat->CancelAttackPresentation ();
    }
    if (OwnerMonster->AudioComponent)
    {
        OwnerMonster->AudioComponent->PlayDeath ();
    }
    if (OwnerMonster->VFXComponent)
    {
        OwnerMonster->VFXComponent->PlayDeathVFX ();
    }

    bool bOccupancyReleased = false;
    if (UGridMonsterMovementComponent* Movement =
        OwnerMonster->FindComponentByClass<UGridMonsterMovementComponent> ())
    {
        Movement->CancelCurrentAction ();
        Movement->HandleOwnerDeath ();
        bOccupancyReleased = true;
    }
    else
    {
        UE_LOG (LogGridMonsterDeath, Warning,
            TEXT ("[GridMonsterDeath] Monster=%s Reason=MissingMonsterMovement; continuing death."),
            *GetNameSafe (OwnerMonster));
    }

    if (OwnerMonster->CollisionComponent)
    {
        OwnerMonster->CollisionComponent->SetCollisionEnabled (
            ECollisionEnabled::NoCollision);
    }

    GenerateAndPlaceLoot ();

    bool bLinksExecuted = false;
    if (RuntimeActor && OwnerMonster->HasMonsterSpawnIdentity ())
    {
        ++LinkExecutionAttemptCount;
        bLinksExecuted = RuntimeActor->ExecuteLinksFromRuntimeObject (
            OwnerMonster->SpawnObjectId,
            EGridObjectEvent::MonsterDied);
        UE_LOG (LogGridMonsterDeath, Log,
            TEXT ("[GridMonsterDeath] Links Monster=%s SourceId=%s Event=MonsterDied Executed=%s"),
            *GetNameSafe (OwnerMonster),
            *OwnerMonster->SpawnObjectId.ToString (),
            bLinksExecuted ? TEXT ("true") : TEXT ("false"));
    }
    else
    {
        UE_LOG (LogGridMonsterDeath, Log,
            TEXT ("[GridMonsterDeath] Links Monster=%s SourceId=%s Event=MonsterDied Executed=false Reason=%s"),
            *GetNameSafe (OwnerMonster),
            *OwnerMonster->SpawnObjectId.ToString (),
            RuntimeActor ? TEXT ("InvalidSpawnObjectId") : TEXT ("MissingRuntimeActor"));
    }

    ++LogicalDeathEventCount;
    OwnerMonster->OnMonsterDied.Broadcast (OwnerMonster, DeathCell);
    UE_LOG (LogGridMonsterDeath, Log,
        TEXT ("[GridMonsterDeath] Broadcast Monster=%s DeathCell=(%d,%d)"),
        *GetNameSafe (OwnerMonster),
        DeathCell.X,
        DeathCell.Y);

    StartDeathPresentation ();

    UE_LOG (LogGridMonsterDeath, Log,
        TEXT ("[GridMonsterDeath] Commit Monster=%s Cell=(%d,%d) SpawnObjectId=%s OccupancyReleased=%s"),
        *GetNameSafe (OwnerMonster),
        DeathCell.X,
        DeathCell.Y,
        *OwnerMonster->SpawnObjectId.ToString (),
        bOccupancyReleased ? TEXT ("true") : TEXT ("false"));
    return true;
}

void UGridMonsterDeathComponent::RestoreCommittedDeathState (
    FIntPoint InDeathCell,
    bool bRestorePresentationPose)
{
    if (!InitializeDeathComponent (RuntimeActor))
    {
        return;
    }

    if (UWorld* World = GetWorld ())
    {
        World->GetTimerManager ().ClearTimer (
            DeathPresentationTimerHandle);
    }

    if (UGridMonsterCombatComponent* Combat =
        OwnerMonster->FindComponentByClass<UGridMonsterCombatComponent> ())
    {
        Combat->CancelAttackPresentation ();
    }
    if (OwnerMonster->AudioComponent)
    {
        OwnerMonster->AudioComponent->StopAllMonsterAudio ();
    }
    if (OwnerMonster->VFXComponent)
    {
        OwnerMonster->VFXComponent->StopAllMonsterVFX ();
    }

    bool bReleasedByMovement = false;
    if (UGridMonsterMovementComponent* Movement =
        OwnerMonster->FindComponentByClass<UGridMonsterMovementComponent> ())
    {
        Movement->CancelCurrentAction ();
        Movement->HandleOwnerDeath ();
        bReleasedByMovement = true;
    }

    if (!bReleasedByMovement)
    {
        if (UGridMonsterOccupancySubsystem* Occupancy =
            GetWorld ()
                ? GetWorld ()->GetSubsystem<UGridMonsterOccupancySubsystem> ()
                : nullptr)
        {
            Occupancy->UnregisterMonster (OwnerMonster);
        }
    }

    bDeathCommitted = true;
    bLootGenerated = true;
    bDeathPresentationActive = false;
    DeathCell = InDeathCell;

    OwnerMonster->CurrentCell = InDeathCell;
    OwnerMonster->CurrentHealth = 0;
    OwnerMonster->MonsterState = EGridMonsterState::Dead;
    OwnerMonster->ResetAnimationSignals ();
    OwnerMonster->SetActorEnableCollision (false);
    if (OwnerMonster->CollisionComponent)
    {
        OwnerMonster->CollisionComponent->SetCollisionEnabled (
            ECollisionEnabled::NoCollision);
    }

    if (bRestorePresentationPose)
    {
        OwnerMonster->SetActorHiddenInGame (false);
        if (OwnerMonster->SkeletalMeshComponent)
        {
            OwnerMonster->SkeletalMeshComponent->SetVisibility (
                true,
                true);
        }
    }
}

void UGridMonsterDeathComponent::RestoreLivingState ()
{
    if (!InitializeDeathComponent (RuntimeActor))
    {
        return;
    }

    if (UWorld* World = GetWorld ())
    {
        World->GetTimerManager ().ClearTimer (
            DeathPresentationTimerHandle);
    }

    bDeathCommitted = false;
    bLootGenerated = false;
    bDeathPresentationActive = false;
    DeathCell = FIntPoint::ZeroValue;
    GeneratedLoot.Reset ();
    PlacedLootCount = 0;
    FailedLootCount = 0;
    LogicalDeathEventCount = 0;
    LinkExecutionAttemptCount = 0;
}

void UGridMonsterDeathComponent::GenerateAndPlaceLoot ()
{
    if (bLootGenerated)
    {
        return;
    }
    bLootGenerated = true;

    if (!OwnerMonster || !OwnerMonster->MonsterDefinition)
    {
        return;
    }

    const TArray<FGridMonsterLootEntry>& LootTable =
        OwnerMonster->MonsterDefinition->LootTable;
    const TArray<FGridMonsterLootRollResult> Results =
        FGridMonsterLootResolver::ResolveLoot (
            LootTable,
            BuildMON8LootSeed (OwnerMonster));
    int32 DroppedLootCount = 0;

    for (const FGridMonsterLootRollResult& Result : Results)
    {
        const float DropChance = LootTable.IsValidIndex (Result.EntryIndex)
            ? LootTable[Result.EntryIndex].DropChance
            : 0.0f;
        UE_LOG (LogGridMonsterLoot, Log,
            TEXT ("[GridMonsterLoot] Roll Monster=%s Entry=%d Item=%s Chance=%.3f Roll=%.3f Dropped=%s Quantity=%d"),
            *GetNameSafe (OwnerMonster),
            Result.EntryIndex,
            *Result.ItemDefinitionId.ToString (),
            DropChance,
            Result.DropRoll,
            Result.bDropped ? TEXT ("true") : TEXT ("false"),
            Result.Quantity);

        if (!Result.bDropped)
        {
            UE_LOG (LogGridMonsterLoot, Log,
                TEXT ("[GridMonsterLoot] NoDrop Monster=%s Entry=%d Item=%s Chance=%.3f Roll=%.3f"),
                *GetNameSafe (OwnerMonster),
                Result.EntryIndex,
                *Result.ItemDefinitionId.ToString (),
                DropChance,
                Result.DropRoll);
            continue;
        }

        ++DroppedLootCount;
        if (!LootTable.IsValidIndex (Result.EntryIndex))
        {
            ++FailedLootCount;
            continue;
        }

        const FGridMonsterLootEntry& Entry =
            LootTable[Result.EntryIndex];
        UGridItemDefinitionAsset* ItemDefinition =
            Entry.ItemDefinitionAsset;

        FGridItemInstance ItemInstance;
        ItemInstance.RuntimeObjectId = FGuid::NewGuid ();
        ItemInstance.ItemDefinitionId = Result.ItemDefinitionId;
        ItemInstance.DisplayName = ItemDefinition
            ? ItemDefinition->DisplayName
            : FText::FromName (Result.ItemDefinitionId);
        ItemInstance.Quantity = Result.Quantity;
        ItemInstance.Weight =
            ItemDefinition ? ItemDefinition->Weight : 0.0f;
        ItemInstance.OwnerType = EGridItemOwnerType::World;
        ItemInstance.OwnerGuid = FGuid ();
        ItemInstance.OwnerCharacterIndex = INDEX_NONE;
        ItemInstance.EquipmentSlot = EGridEquipmentSlot::None;
        ItemInstance.bLightsEnabled =
            ItemDefinition && ItemDefinition->bDefaultLightEnabled;

        const FVector LocalOffset =
            GetMON8LootLocalOffset (PlacedLootCount);
        if (RuntimeActor)
        {
            ItemInstance.LastWorldTransform = FTransform (
                FRotator::ZeroRotator,
                RuntimeActor->GetCellCenterWorld (
                    DeathCell.X,
                    DeathCell.Y,
                    12.0f) + LocalOffset,
                FVector::OneVector);
        }

        const bool bPlaced = RuntimeActor &&
            RuntimeActor->TryDropItemInstanceAtCell (
                ItemInstance,
                ItemDefinition,
                DeathCell.X,
                DeathCell.Y,
                EGridEdge::None,
                LocalOffset);
        if (!bPlaced)
        {
            ++FailedLootCount;
            UE_LOG (LogGridMonsterLoot, Warning,
                TEXT ("[GridMonsterLoot] PlacementFailed Monster=%s Item=%s Cell=(%d,%d) Reason=%s"),
                *GetNameSafe (OwnerMonster),
                *Result.ItemDefinitionId.ToString (),
                DeathCell.X,
                DeathCell.Y,
                RuntimeActor
                    ? TEXT ("UnresolvedDefinitionOrInvalidCell")
                    : TEXT ("MissingRuntimeActor"));
            continue;
        }

        GeneratedLoot.Add (ItemInstance);
        ++PlacedLootCount;
        UE_LOG (LogGridMonsterLoot, Log,
            TEXT ("[GridMonsterLoot] Placed Monster=%s Item=%s Quantity=%d Cell=(%d,%d) RuntimeId=%s"),
            *GetNameSafe (OwnerMonster),
            *Result.ItemDefinitionId.ToString (),
            Result.Quantity,
            DeathCell.X,
            DeathCell.Y,
            *ItemInstance.RuntimeObjectId.ToString ());
    }

    UE_LOG (LogGridMonsterLoot, Log,
        TEXT ("[GridMonsterLoot] Summary Monster=%s Evaluated=%d Dropped=%d Placed=%d Failed=%d"),
        *GetNameSafe (OwnerMonster),
        Results.Num (),
        DroppedLootCount,
        PlacedLootCount,
        FailedLootCount);
}

void UGridMonsterDeathComponent::StartDeathPresentation ()
{
    if (!OwnerMonster || !OwnerMonster->MonsterDefinition)
    {
        return;
    }

    UAnimMontage* Montage =
        OwnerMonster->MonsterDefinition->DeathMontage.LoadSynchronous ();
    UAnimInstance* AnimInstance =
        OwnerMonster->SkeletalMeshComponent
            ? OwnerMonster->SkeletalMeshComponent->GetAnimInstance ()
            : nullptr;
    if (!Montage || !AnimInstance ||
        AnimInstance->Montage_Play (Montage) <= 0.0f)
    {
        bDeathPresentationActive = false;
        return;
    }

    bDeathPresentationActive = true;
    if (UWorld* World = GetWorld ())
    {
        World->GetTimerManager ().SetTimer (
            DeathPresentationTimerHandle,
            this,
            &UGridMonsterDeathComponent::NotifyDeathPresentationComplete,
            FMath::Max (
                0.01f,
                OwnerMonster->MonsterDefinition->DeathExpectedDuration),
            false);
    }
}

void UGridMonsterDeathComponent::NotifyDeathPresentationComplete ()
{
    if (UWorld* World = GetWorld ())
    {
        World->GetTimerManager ().ClearTimer (DeathPresentationTimerHandle);
    }
    bDeathPresentationActive = false;
}

void UGridMonsterDeathComponent::DebugKillMonster ()
{
#if !UE_BUILD_SHIPPING
    if (!InitializeDeathComponent (RuntimeActor))
    {
        return;
    }
    OwnerMonster->MarkDead ();
#endif
}

AGridLevelRuntimeActor* UGridMonsterDeathComponent::FindRuntimeActor () const
{
    UWorld* World = GetWorld ();
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
