#include "Runtime/Monsters/GridMonsterDeathComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "RPG/RPGExperienceRewardService.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterAudioComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterIdleVariationComponent.h"
#include "Runtime/Monsters/GridMonsterLootResolver.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridMonsterDeath, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogGridMonsterLoot, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogGridExperience, Log, All);

namespace
{
	constexpr uint32 MON8LootSeedSalt = 0x4D4F4E38u;

	int32 BuildMON8LootSeed(const AGridMonsterActor* Monster)
	{
		uint32 Seed = MON8LootSeedSalt;
		if (Monster)
		{
			Seed = HashCombine(Seed, GetTypeHash(Monster->ResolvePersistenceId()));
			Seed = HashCombine(Seed, GetTypeHash(Monster->MonsterDefinition ? Monster->MonsterDefinition->MonsterId : NAME_None));
		}
		return static_cast<int32>(Seed);
	}

	FVector GetMON8LootLocalOffset(int32 LootIndex)
	{
		static const FVector Offsets[] = { FVector(0.0f, 0.0f, 0.0f), FVector(20.0f, 0.0f, 0.0f), FVector(-20.0f, 0.0f, 0.0f), FVector(0.0f, 20.0f, 0.0f),
			FVector(0.0f, -20.0f, 0.0f), FVector(20.0f, 20.0f, 0.0f), FVector(-20.0f, 20.0f, 0.0f), FVector(20.0f, -20.0f, 0.0f),
			FVector(-20.0f, -20.0f, 0.0f) };
		return Offsets[LootIndex % UE_ARRAY_COUNT(Offsets)];
	}

	UGridPartyInventoryComponent* FindMON152PartyInventory(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<AGrimrockPartyPawn> It(World); It; ++It)
		{
			AGrimrockPartyPawn* PartyPawn = *It;
			if (IsValid(PartyPawn) && IsValid(PartyPawn->PartyInventoryComponent))
			{
				return PartyPawn->PartyInventoryComponent;
			}
		}
		return nullptr;
	}
}

UGridMonsterDeathComponent::UGridMonsterDeathComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UGridMonsterDeathComponent::InitializeDeathComponent(AGridLevelRuntimeActor* InRuntimeActor)
{
	if (!IsValid(OwnerMonster))
	{
		OwnerMonster = Cast<AGridMonsterActor>(GetOwner());
	}
	if (!IsValid(OwnerMonster))
	{
		return false;
	}

	RuntimeActor = IsValid(InRuntimeActor) ? InRuntimeActor : nullptr;
	if (!RuntimeActor)
	{
		if (const UGridMonsterMovementComponent* Movement = OwnerMonster->FindComponentByClass<UGridMonsterMovementComponent>())
		{
			RuntimeActor = Movement->RuntimeActor;
		}
	}
	if (!RuntimeActor)
	{
		RuntimeActor = FindRuntimeActor();
	}
	return true;
}

bool UGridMonsterDeathComponent::CommitDeath()
{
	if (bDeathCommitted)
	{
		return false;
	}
	if (!InitializeDeathComponent(RuntimeActor))
	{
		return false;
	}

	if (OwnerMonster->IdleVariationComponent)
	{
		OwnerMonster->IdleVariationComponent->StopIdleVariations();
	}

	// Commit the guard before calling any external gameplay hook.
	bDeathCommitted = true;
	DeathCell = OwnerMonster->CurrentCell;
	const FGuid EncounterSpawnId = OwnerMonster->HasMonsterSpawnIdentity() ? OwnerMonster->SpawnObjectId : FGuid();

	if (UGridMonsterCombatComponent* Combat = OwnerMonster->FindComponentByClass<UGridMonsterCombatComponent>())
	{
		Combat->CancelAttackPresentation();
	}
	if (OwnerMonster->AudioComponent)
	{
		OwnerMonster->AudioComponent->PlayDeath();
	}
	if (OwnerMonster->VFXComponent)
	{
		OwnerMonster->VFXComponent->PlayDeathVFX();
	}

	bool bOccupancyReleased = false;
	if (UGridMonsterMovementComponent* Movement = OwnerMonster->FindComponentByClass<UGridMonsterMovementComponent>())
	{
		Movement->CancelCurrentAction();
		Movement->HandleOwnerDeath();
		bOccupancyReleased = true;
	}
	else
	{
		if (UGridMonsterOccupancySubsystem* Occupancy = GetWorld() ? GetWorld()->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr)
		{
			Occupancy->UnregisterMonster(OwnerMonster);
			bOccupancyReleased = true;
		}
		UE_LOG(
			LogGridMonsterDeath, Warning, TEXT("[GridMonsterDeath] Monster=%s Reason=MissingMonsterMovement; continuing death."), *GetNameSafe(OwnerMonster));
	}

	if (OwnerMonster->CollisionComponent)
	{
		OwnerMonster->CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	GenerateAndPlaceLoot();

	const int32 RequestedExperienceReward = OwnerMonster->MonsterDefinition ? FMath::Max(0, OwnerMonster->MonsterDefinition->ExperienceReward) : 0;
	int32 AppliedExperienceReward = 0;
	if (RequestedExperienceReward > 0)
	{
		if (UGridPartyInventoryComponent* PartyInventory = FindMON152PartyInventory(GetWorld()))
		{
			AppliedExperienceReward = FRPGExperienceRewardService::AwardToActiveParty(PartyInventory, RequestedExperienceReward);
		}
		else
		{
			UE_LOG(LogGridExperience, Warning, TEXT("[GridExperience] Monster=%s PersistenceId=%s Reward=%d Applied=0 Reason=MissingPartyInventory"),
				*GetNameSafe(OwnerMonster), *OwnerMonster->ResolvePersistenceId().ToString(), RequestedExperienceReward);
		}
	}

	if (RequestedExperienceReward > 0)
	{
		UE_LOG(LogGridExperience, Log, TEXT("[GridExperience] Monster=%s PersistenceId=%s Reward=%d Applied=%d"), *GetNameSafe(OwnerMonster),
			*OwnerMonster->ResolvePersistenceId().ToString(), RequestedExperienceReward, AppliedExperienceReward);
	}

	bool bLinksExecuted = false;
	if (RuntimeActor && EncounterSpawnId.IsValid())
	{
		++LinkExecutionAttemptCount;
		bLinksExecuted = RuntimeActor->ExecuteLinksFromRuntimeObject(EncounterSpawnId, EGridObjectEvent::MonsterDied);
		UE_LOG(LogGridMonsterDeath, Log, TEXT("[GridMonsterDeath] Links Monster=%s SourceId=%s Event=MonsterDied Executed=%s"), *GetNameSafe(OwnerMonster),
			*OwnerMonster->SpawnObjectId.ToString(), bLinksExecuted ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogGridMonsterDeath, Log, TEXT("[GridMonsterDeath] Links Monster=%s SourceId=%s Event=MonsterDied Executed=false Reason=%s"),
			*GetNameSafe(OwnerMonster), *OwnerMonster->SpawnObjectId.ToString(), RuntimeActor ? TEXT("InvalidSpawnObjectId") : TEXT("MissingRuntimeActor"));
	}

	++LogicalDeathEventCount;
	OwnerMonster->OnMonsterDied.Broadcast(OwnerMonster, DeathCell);
	if (RuntimeActor && EncounterSpawnId.IsValid())
	{
		RuntimeActor->NotifyMonsterEncounterDeath(EncounterSpawnId);
	}
	UE_LOG(LogGridMonsterDeath, Log, TEXT("[GridMonsterDeath] Broadcast Monster=%s DeathCell=(%d,%d)"), *GetNameSafe(OwnerMonster), DeathCell.X, DeathCell.Y);

	StartDeathPresentation();

	UE_LOG(LogGridMonsterDeath, Log, TEXT("[GridMonsterDeath] Commit Monster=%s Cell=(%d,%d) SpawnObjectId=%s OccupancyReleased=%s"),
		*GetNameSafe(OwnerMonster), DeathCell.X, DeathCell.Y, *OwnerMonster->SpawnObjectId.ToString(), bOccupancyReleased ? TEXT("true") : TEXT("false"));
	return true;
}

void UGridMonsterDeathComponent::RestoreCommittedDeathState(FIntPoint InDeathCell)
{
	if (!InitializeDeathComponent(RuntimeActor))
	{
		return;
	}

	if (OwnerMonster->IdleVariationComponent)
	{
		OwnerMonster->IdleVariationComponent->StopIdleVariations();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathPresentationTimerHandle);
	}

	ResetDeathDissolvePresentation(true, false);
	ResetDeathRagdollPresentation(false);

	if (UGridMonsterCombatComponent* Combat = OwnerMonster->FindComponentByClass<UGridMonsterCombatComponent>())
	{
		Combat->CancelAttackPresentation();
	}
	if (OwnerMonster->AudioComponent)
	{
		OwnerMonster->AudioComponent->StopAllMonsterAudio();
	}
	if (OwnerMonster->VFXComponent)
	{
		OwnerMonster->VFXComponent->StopAllMonsterVFX();
	}

	bool bReleasedByMovement = false;
	if (UGridMonsterMovementComponent* Movement = OwnerMonster->FindComponentByClass<UGridMonsterMovementComponent>())
	{
		Movement->CancelCurrentAction();
		Movement->HandleOwnerDeath();
		bReleasedByMovement = true;
	}

	if (!bReleasedByMovement)
	{
		if (UGridMonsterOccupancySubsystem* Occupancy = GetWorld() ? GetWorld()->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr)
		{
			Occupancy->UnregisterMonster(OwnerMonster);
		}
	}

	bDeathCommitted = true;
	bLootGenerated = true;
	bDeathPresentationActive = false;
	DeathCell = InDeathCell;

	OwnerMonster->CurrentCell = InDeathCell;
	OwnerMonster->CurrentHealth = 0;
	OwnerMonster->MonsterState = EGridMonsterState::Dead;
	OwnerMonster->ResetAnimationSignals();
	OwnerMonster->SetActorEnableCollision(false);
	OwnerMonster->SetActorHiddenInGame(false);
	if (OwnerMonster->CollisionComponent)
	{
		OwnerMonster->CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (OwnerMonster->SkeletalMeshComponent)
	{
		OwnerMonster->SkeletalMeshComponent->SetVisibility(false, true);
	}
}

void UGridMonsterDeathComponent::RestoreLivingState()
{
	if (!InitializeDeathComponent(RuntimeActor))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathPresentationTimerHandle);
	}

	ResetDeathDissolvePresentation(true, true);
	ResetDeathRagdollPresentation(true);

	bDeathCommitted = false;
	bLootGenerated = false;
	bDeathPresentationActive = false;
	DeathCell = FIntPoint::ZeroValue;
	GeneratedLoot.Reset();
	PlacedLootCount = 0;
	FailedLootCount = 0;
	LogicalDeathEventCount = 0;
	LinkExecutionAttemptCount = 0;
}

void UGridMonsterDeathComponent::GenerateAndPlaceLoot()
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

	const TArray<FGridMonsterLootEntry>& LootTable = OwnerMonster->MonsterDefinition->LootTable;
	const TArray<FGridMonsterLootRollResult> Results = FGridMonsterLootResolver::ResolveLoot(LootTable, BuildMON8LootSeed(OwnerMonster));
	int32 DroppedLootCount = 0;

	for (const FGridMonsterLootRollResult& Result : Results)
	{
		const float DropChance = LootTable.IsValidIndex(Result.EntryIndex) ? LootTable[Result.EntryIndex].DropChance : 0.0f;
		UE_LOG(LogGridMonsterLoot, Log, TEXT("[GridMonsterLoot] Roll Monster=%s Entry=%d Item=%s Chance=%.3f Roll=%.3f Dropped=%s Quantity=%d"),
			*GetNameSafe(OwnerMonster), Result.EntryIndex, *Result.ItemDefinitionId.ToString(), DropChance, Result.DropRoll,
			Result.bDropped ? TEXT("true") : TEXT("false"), Result.Quantity);

		if (!Result.bDropped)
		{
			UE_LOG(LogGridMonsterLoot, Log, TEXT("[GridMonsterLoot] NoDrop Monster=%s Entry=%d Item=%s Chance=%.3f Roll=%.3f"), *GetNameSafe(OwnerMonster),
				Result.EntryIndex, *Result.ItemDefinitionId.ToString(), DropChance, Result.DropRoll);
			continue;
		}

		++DroppedLootCount;
		if (!LootTable.IsValidIndex(Result.EntryIndex))
		{
			++FailedLootCount;
			continue;
		}

		const FGridMonsterLootEntry& Entry = LootTable[Result.EntryIndex];
		UGridItemDefinitionAsset* ItemDefinition = Entry.ItemDefinitionAsset;

		FGridItemInstance ItemInstance;
		ItemInstance.RuntimeObjectId = FGuid::NewGuid();
		ItemInstance.ItemDefinitionId = Result.ItemDefinitionId;
		ItemInstance.DisplayName = ItemDefinition ? ItemDefinition->DisplayName : FText::FromName(Result.ItemDefinitionId);
		ItemInstance.Quantity = Result.Quantity;
		ItemInstance.Weight = ItemDefinition ? ItemDefinition->Weight : 0.0f;
		ItemInstance.OwnerType = EGridItemOwnerType::World;
		ItemInstance.OwnerGuid = FGuid();
		ItemInstance.OwnerCharacterIndex = INDEX_NONE;
		ItemInstance.EquipmentSlot = EGridEquipmentSlot::None;
		ItemInstance.bLightsEnabled = ItemDefinition && ItemDefinition->bDefaultLightEnabled;

		const FVector LocalOffset = GetMON8LootLocalOffset(PlacedLootCount);
		if (RuntimeActor)
		{
			ItemInstance.LastWorldTransform =
				FTransform(FRotator::ZeroRotator, RuntimeActor->GetCellCenterWorld(DeathCell.X, DeathCell.Y, 12.0f) + LocalOffset, FVector::OneVector);
		}

		const bool bPlaced =
			RuntimeActor && RuntimeActor->TryDropItemInstanceAtCell(ItemInstance, ItemDefinition, DeathCell.X, DeathCell.Y, EGridEdge::None, LocalOffset);
		if (!bPlaced)
		{
			++FailedLootCount;
			UE_LOG(LogGridMonsterLoot, Warning, TEXT("[GridMonsterLoot] PlacementFailed Monster=%s Item=%s Cell=(%d,%d) Reason=%s"), *GetNameSafe(OwnerMonster),
				*Result.ItemDefinitionId.ToString(), DeathCell.X, DeathCell.Y,
				RuntimeActor ? TEXT("UnresolvedDefinitionOrInvalidCell") : TEXT("MissingRuntimeActor"));
			continue;
		}

		GeneratedLoot.Add(ItemInstance);
		++PlacedLootCount;
		UE_LOG(LogGridMonsterLoot, Log, TEXT("[GridMonsterLoot] Placed Monster=%s Item=%s Quantity=%d Cell=(%d,%d) RuntimeId=%s"), *GetNameSafe(OwnerMonster),
			*Result.ItemDefinitionId.ToString(), Result.Quantity, DeathCell.X, DeathCell.Y, *ItemInstance.RuntimeObjectId.ToString());
	}

	UE_LOG(LogGridMonsterLoot, Log, TEXT("[GridMonsterLoot] Summary Monster=%s Evaluated=%d Dropped=%d Placed=%d Failed=%d"), *GetNameSafe(OwnerMonster),
		Results.Num(), DroppedLootCount, PlacedLootCount, FailedLootCount);
}

void UGridMonsterDeathComponent::StartDeathPresentation()
{
	if (!OwnerMonster || !OwnerMonster->MonsterDefinition)
	{
		return;
	}

	bDeathObstacleDetected = false;
	LastDeathObstacleImpactPoint = FVector::ZeroVector;

	// An obstacle branch enters physics immediately so an animated fall cannot tunnel through nearby geometry.
	if (TryStartObstacleAwareDeathRagdoll())
	{
		return;
	}

	UAnimMontage* Montage = OwnerMonster->MonsterDefinition->DeathMontage.LoadSynchronous();
	UAnimInstance* AnimInstance = OwnerMonster->SkeletalMeshComponent ? OwnerMonster->SkeletalMeshComponent->GetAnimInstance() : nullptr;
	if (!Montage || !AnimInstance || AnimInstance->Montage_Play(Montage) <= 0.0f)
	{
		bDeathPresentationActive = false;
		return;
	}

	bDeathPresentationActive = true;
	ScheduleDeathPresentationCompletion();
}

void UGridMonsterDeathComponent::ScheduleDeathPresentationCompletion()
{
	if (!OwnerMonster || !OwnerMonster->MonsterDefinition)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(DeathPresentationTimerHandle, this, &UGridMonsterDeathComponent::NotifyDeathPresentationComplete,
			FMath::Max(0.01f, OwnerMonster->MonsterDefinition->DeathExpectedDuration), false);
	}
}

FVector UGridMonsterDeathComponent::ResolveDeathFallWorldDirection() const
{
	if (!OwnerMonster || !OwnerMonster->MonsterDefinition)
	{
		return FVector::ZeroVector;
	}

	FVector LocalDirection = OwnerMonster->MonsterDefinition->DeathFallLocalDirection;
	LocalDirection.Z = 0.0f;
	if (LocalDirection.ContainsNaN() || LocalDirection.IsNearlyZero())
	{
		LocalDirection = FVector(-1.0f, 0.0f, 0.0f);
	}
	LocalDirection.Normalize();

	FVector WorldDirection = OwnerMonster->GetActorTransform().TransformVectorNoScale(LocalDirection);
	WorldDirection.Z = 0.0f;
	if (WorldDirection.IsNearlyZero())
	{
		return -OwnerMonster->GetActorForwardVector().GetSafeNormal2D();
	}
	return WorldDirection.GetSafeNormal2D();
}

bool UGridMonsterDeathComponent::ProbeDeathObstacle(FHitResult& OutHit) const
{
	OutHit = FHitResult();
	if (!OwnerMonster || !OwnerMonster->MonsterDefinition || !OwnerMonster->MonsterDefinition->bEnableObstacleAwareDeath)
	{
		return false;
	}

	UWorld* World = GetWorld();
	const FVector FallDirection = ResolveDeathFallWorldDirection();
	if (!World || FallDirection.IsNearlyZero())
	{
		return false;
	}

	const UGridMonsterDefinitionAsset* Definition = OwnerMonster->MonsterDefinition;
	const float Distance = FMath::Max(1.0f, Definition->DeathObstacleProbeDistance);
	const float Radius = FMath::Max(1.0f, Definition->DeathObstacleProbeRadius);
	const float HalfHeight = FMath::Max(Radius, Definition->DeathObstacleProbeHalfHeight);
	const FVector Start = OwnerMonster->GetActorLocation() + FVector(0.0f, 0.0f, HalfHeight + 5.0f);
	const FVector End = Start + FallDirection * Distance;

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQuery.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(OwnerMonster);

	TArray<FHitResult> Hits;
	if (!World->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjectQuery, FCollisionShape::MakeCapsule(Radius, HalfHeight), QueryParams))
	{
		return false;
	}

	const FHitResult* NearestPhysicalHit = nullptr;
	for (const FHitResult& Hit : Hits)
	{
		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (!HitComponent)
		{
			continue;
		}

		const ECollisionEnabled::Type CollisionEnabled = HitComponent->GetCollisionEnabled();
		const bool bPhysicallyBlocking = CollisionEnabled == ECollisionEnabled::QueryAndPhysics || CollisionEnabled == ECollisionEnabled::PhysicsOnly;
		if (!bPhysicallyBlocking)
		{
			continue;
		}

		if (!NearestPhysicalHit || Hit.Distance < NearestPhysicalHit->Distance)
		{
			NearestPhysicalHit = &Hit;
		}
	}

	if (NearestPhysicalHit)
	{
		OutHit = *NearestPhysicalHit;
		return true;
	}
	return false;
}

bool UGridMonsterDeathComponent::TryStartObstacleAwareDeathRagdoll()
{
	if (!OwnerMonster || !OwnerMonster->MonsterDefinition || !OwnerMonster->MonsterDefinition->bEnableObstacleAwareDeath)
	{
		return false;
	}

	FHitResult ObstacleHit;
	if (!ProbeDeathObstacle(ObstacleHit))
	{
		return false;
	}

	bDeathObstacleDetected = true;
	LastDeathObstacleImpactPoint = ObstacleHit.ImpactPoint;

	USkeletalMeshComponent* Mesh = OwnerMonster->SkeletalMeshComponent;
	if (!Mesh || !Mesh->GetPhysicsAsset())
	{
		UE_LOG(LogGridMonsterDeath, Warning,
			TEXT("[GridMonsterDeath] ObstacleAware fallback Monster=%s Obstacle=%s Reason=MissingPhysicsAsset"),
			*GetNameSafe(OwnerMonster), *GetNameSafe(ObstacleHit.GetActor()));
		return false;
	}

	if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.05f);
	}

	Mesh->SetCollisionObjectType(ECC_PhysicsBody);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(true);

	if (!Mesh->IsAnySimulatingPhysics())
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UE_LOG(LogGridMonsterDeath, Warning,
			TEXT("[GridMonsterDeath] ObstacleAware fallback Monster=%s Obstacle=%s Reason=PhysicsSimulationRejected"),
			*GetNameSafe(OwnerMonster), *GetNameSafe(ObstacleHit.GetActor()));
		return false;
	}

	Mesh->WakeAllRigidBodies();

	const UGridMonsterDefinitionAsset* Definition = OwnerMonster->MonsterDefinition;
	const FVector FallDirection = ResolveDeathFallWorldDirection();
	const FVector LinearVelocity =
		FallDirection * FMath::Max(0.0f, Definition->DeathRagdollBackwardSpeed) + FVector::DownVector * FMath::Max(0.0f, Definition->DeathRagdollDownwardSpeed);
	Mesh->SetAllPhysicsLinearVelocity(LinearVelocity, false);

	const FVector AngularAxis = FVector::CrossProduct(FVector::UpVector, FallDirection).GetSafeNormal();
	if (!AngularAxis.IsNearlyZero() && Definition->DeathRagdollAngularSpeedDegrees > 0.0f)
	{
		Mesh->SetAllPhysicsAngularVelocityInDegrees(AngularAxis * Definition->DeathRagdollAngularSpeedDegrees, false);
	}

	bDeathRagdollActive = true;
	bDeathPresentationActive = true;
	ScheduleDeathPresentationCompletion();

	UE_LOG(LogGridMonsterDeath, Log,
		TEXT("[GridMonsterDeath] ObstacleAware ragdoll Monster=%s Obstacle=%s Component=%s Impact=%s FallDirection=%s LinearVelocity=%s"),
		*GetNameSafe(OwnerMonster), *GetNameSafe(ObstacleHit.GetActor()), *GetNameSafe(ObstacleHit.GetComponent()),
		*ObstacleHit.ImpactPoint.ToCompactString(), *FallDirection.ToCompactString(), *LinearVelocity.ToCompactString());
	return true;
}

void UGridMonsterDeathComponent::ResetDeathRagdollPresentation(bool bRestoreVisualPose)
{
	bDeathObstacleDetected = false;
	bDeathRagdollActive = false;
	LastDeathObstacleImpactPoint = FVector::ZeroVector;

	if (!OwnerMonster || !OwnerMonster->SkeletalMeshComponent)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = OwnerMonster->SkeletalMeshComponent;
	if (Mesh->IsAnySimulatingPhysics())
	{
		Mesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector, false);
		Mesh->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector, false);
		Mesh->SetSimulatePhysics(false);
	}

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetGenerateOverlapEvents(false);

	if (bRestoreVisualPose && OwnerMonster->SceneRoot)
	{
		if (Mesh->GetAttachParent() != OwnerMonster->SceneRoot)
		{
			Mesh->AttachToComponent(OwnerMonster->SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
		}
		OwnerMonster->ApplyDefinitionVisuals();
	}
}

void UGridMonsterDeathComponent::NotifyDeathPresentationComplete()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathPresentationTimerHandle);
	}
	bDeathPresentationActive = false;
}

void UGridMonsterDeathComponent::DebugKillMonster()
{
#if !UE_BUILD_SHIPPING
	if (!InitializeDeathComponent(RuntimeActor))
	{
		return;
	}
	OwnerMonster->MarkDead();
#endif
}

AGridLevelRuntimeActor* UGridMonsterDeathComponent::FindRuntimeActor() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AGridLevelRuntimeActor> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}
