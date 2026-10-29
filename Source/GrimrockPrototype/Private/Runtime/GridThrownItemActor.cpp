#include "Runtime/GridThrownItemActor.h"

#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Runtime/Combat/GridProjectileGeometry.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogGridThrownItem);

AGridThrownItemActor::AGridThrownItemActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileCollision"));
	CollisionComponent->InitSphereRadius(12.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	SetRootComponent(CollisionComponent);

	if (SceneRoot)
	{
		SceneRoot->SetupAttachment(CollisionComponent);
	}
	if (MeshComponent)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetSimulatePhysics(false);
		MeshComponent->SetEnableGravity(false);
	}

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	// Explicit swept movement prevents fast thrown items from tunnelling through blocking geometry.
	ProjectileMovementComponent->bSweepCollision = true;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;
	ProjectileMovementComponent->ProjectileGravityScale = 1.0f;
	ProjectileMovementComponent->InitialSpeed = 0.0f;
	ProjectileMovementComponent->MaxSpeed = 0.0f;
}

void AGridThrownItemActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bConversionAttempted)
	{
		SetActorTickEnabled(false);
		return;
	}

	// While visible, the shaft must follow the animated bone rather than
	// remaining at the original world impact point. Natural fall/save is 13.6C.
	if (bEmbeddedInMonster)
	{
		const AGridMonsterActor* Monster = EmbeddedMonster.Get();
		if (!IsValid(Monster) || !IsValid(Monster->SkeletalMeshComponent) ||
			!Monster->SkeletalMeshComponent->IsVisible())
		{
			ReleaseEmbeddedArrowToPickup();
		}
		else
		{
			KeepEmbeddedArrowOnAnimatedBone();
		}
		return;
	}

	if (MeshComponent && !AppliedThrowVisualAngularVelocityDegreesPerSecond.IsNearlyZero())
	{
		const FVector Angular = AppliedThrowVisualAngularVelocityDegreesPerSecond * DeltaSeconds;
		MeshComponent->AddLocalRotation(FRotator(Angular.X, Angular.Y, Angular.Z));
	}

	const FVector CurrentLocation = GetActorLocation();
	// Remember actual transit cells on both hit and miss trajectories. If
	// collision happens on the boundary, never arbitrarily return to source.
	if (IsValid(RuntimeActor))
	{
		int32 CellX = INDEX_NONE;
		int32 CellY = INDEX_NONE;
		FVector LocalOffset = FVector::ZeroVector;
		if (RuntimeActor->TryResolveWorldCellFromImpactPoint(CurrentLocation, CellX, CellY, LocalOffset))
		{
			LastWalkableFlightCell = FIntPoint(CellX, CellY);
			LastWalkableFlightOffset = LocalOffset;
		}
	}

	if (bRangedMissDeflected && !bRangedMissGravityStarted &&
		FVector::DotProduct(CurrentLocation - RangedMissTargetLocation, RangedMissForward) >= 25.0f)
	{
		// After clearing the target plane, allow the missed arrow to fall under
		// Chaos / projectile gravity, or terminate sooner on a real wall.
		bRangedMissGravityStarted = true;
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->ProjectileGravityScale = 1.0f;
		}
		UE_LOG(LogGridThrownItem, Log,
			TEXT("[GridProjectile] MissGravity Item=%s RuntimeId=%s Location=%s"),
			*ThrownItemInstance.ItemDefinitionId.ToString(), *ThrownItemInstance.RuntimeObjectId.ToString(),
			*CurrentLocation.ToCompactString());
	}
	if (!bStopsAtCombatPresentationTarget)
	{
		return;
	}
	const FVector Segment = CurrentLocation - PreviousPresentationLocation;
	const float SegmentLengthSquared = Segment.SizeSquared();
	FVector ClosestPoint = CurrentLocation;
	if (SegmentLengthSquared > KINDA_SMALL_NUMBER)
	{
		const float SegmentAlpha =
			FMath::Clamp(FVector::DotProduct(CombatPresentationTargetLocation - PreviousPresentationLocation, Segment) / SegmentLengthSquared, 0.0f, 1.0f);
		ClosestPoint = PreviousPresentationLocation + Segment * SegmentAlpha;
	}

	const bool bReachedTarget = FVector::DistSquared(ClosestPoint, CombatPresentationTargetLocation) <= FMath::Square(CombatPresentationTargetAcceptanceRadius);
	const bool bPassedTarget = SegmentLengthSquared > KINDA_SMALL_NUMBER &&
		FVector::DotProduct(CombatPresentationTargetLocation - PreviousPresentationLocation, CombatPresentationTargetLocation - CurrentLocation) <= 0.0f;
	if (bReachedTarget || bPassedTarget)
	{
		FHitResult TargetHit;
		TargetHit.ImpactPoint = CombatPresentationTargetLocation;
		TargetHit.Location = CombatPresentationTargetLocation;
		TargetHit.ImpactNormal = (-Segment).GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
		if (!TryEmbedInMonster())
		{
			ConvertToWorldPickupAtImpact(TargetHit);
		}
		return;
	}

	PreviousPresentationLocation = CurrentLocation;
}

void AGridThrownItemActor::BeginPlay()
{
	Super::BeginPlay();
	if (CollisionComponent)
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &AGridThrownItemActor::HandleProjectileImpact);
	}
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->OnProjectileStop.AddDynamic(this, &AGridThrownItemActor::HandleProjectileStopped);
	}
}

void AGridThrownItemActor::InitializeThrownItem(AGridLevelRuntimeActor* InRuntimeActor, const FGridItemInstance& InItemInstance,
	UGridItemDefinitionAsset* InDefinition, const FVector& LaunchVelocity, int32 InSourceCellX, int32 InSourceCellY)
{
	RuntimeActor = InRuntimeActor;
	ThrownItemInstance = InItemInstance;
	SourceCellX = InSourceCellX;
	SourceCellY = InSourceCellY;
	ImpactDropOffset = InDefinition ? FMath::Max(0.0f, InDefinition->ThrowImpactDropOffset) : 12.0f;

	InitializeFromItemDefinition(InDefinition, InItemInstance.RuntimeObjectId);
	ConfigureAsAttachedItem();
	SetItemLightsEnabled(InItemInstance.bLightsEnabled);

	InitialFlightWorldLocation = GetActorLocation();
	const FRotator VisualRotation = InDefinition ? InDefinition->ThrowVisualRelativeRotation : FRotator(-90.0f, 0.0f, 0.0f);
	FVector VisualScale = InDefinition ? InDefinition->ThrowVisualRelativeScale : FVector(1.5f);
	if (VisualScale.ContainsNaN())
	{
		VisualScale = FVector(1.5f);
	}
	VisualScale.X = FMath::Clamp(FMath::Abs(VisualScale.X), 0.01f, 100.0f);
	VisualScale.Y = FMath::Clamp(FMath::Abs(VisualScale.Y), 0.01f, 100.0f);
	VisualScale.Z = FMath::Clamp(FMath::Abs(VisualScale.Z), 0.01f, 100.0f);
	AppliedThrowVisualAngularVelocityDegreesPerSecond = FVector::ZeroVector;
	const EGridThrowVisualMode VisualMode = InDefinition ? InDefinition->ThrowVisualMode : EGridThrowVisualMode::Tumble;
	switch (VisualMode)
	{
		case EGridThrowVisualMode::Stable:
			break;

		case EGridThrowVisualMode::Spin:
			AppliedThrowVisualAngularVelocityDegreesPerSecond =
				FVector(0.0f, 0.0f, InDefinition ? FMath::Max(0.0f, InDefinition->ThrowVisualSpinDegreesPerSecond) : 1080.0f);
			break;

		case EGridThrowVisualMode::Tumble:
		default:
		{
			FVector TumbleAxis = InDefinition ? InDefinition->ThrowVisualTumbleAxis : FVector(0.65f, 0.35f, 1.0f);
			if (TumbleAxis.ContainsNaN() || TumbleAxis.IsNearlyZero())
			{
				TumbleAxis = FVector(0.65f, 0.35f, 1.0f);
			}
			TumbleAxis.Normalize();
			const float TumbleSpeed = InDefinition ? FMath::Max(0.0f, InDefinition->ThrowVisualTumbleDegreesPerSecond) : 180.0f;
			AppliedThrowVisualAngularVelocityDegreesPerSecond = TumbleAxis * TumbleSpeed;
			break;
		}
	}

	if (MeshComponent)
	{
		MeshComponent->SetRelativeTransform(FTransform(VisualRotation, FVector::ZeroVector, VisualScale));
		MeshComponent->SetVisibility(true, true);
		MeshComponent->SetHiddenInGame(false, true);
		MeshComponent->UpdateBounds();
	}
	if (VisualMode == EGridThrowVisualMode::Stable)
	{
		AlignMeshForwardToFlight();
	}
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	UE_LOG(LogGridThrownItem, Log, TEXT("GridThrownItem Visual Item=%s RuntimeId=%s Mesh=%s Rotation=%s Scale=%s Mode=%s AngularVelocity=%s"),
		*InItemInstance.ItemDefinitionId.ToString(), *InItemInstance.RuntimeObjectId.ToString(),
		MeshComponent ? *GetNameSafe(MeshComponent->GetStaticMesh()) : TEXT("None"), *VisualRotation.ToCompactString(), *VisualScale.ToCompactString(),
		*UEnum::GetValueAsString(VisualMode), *AppliedThrowVisualAngularVelocityDegreesPerSecond.ToCompactString());

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Velocity = LaunchVelocity;
		ProjectileMovementComponent->InitialSpeed = LaunchVelocity.Size();
		ProjectileMovementComponent->MaxSpeed = LaunchVelocity.Size();
		ProjectileMovementComponent->Activate(true);
	}

	const float LifeSeconds = InDefinition ? FMath::Max(0.01f, InDefinition->ThrowLifeSeconds) : 5.0f;
	GetWorldTimerManager().SetTimer(ExpirationTimerHandle, this, &AGridThrownItemActor::HandleProjectileExpired, LifeSeconds, false);
}

FVector AGridThrownItemActor::ResolveMeshForwardAxis(EGridProjectileForwardAxis Axis)
{
	return FGridProjectileGeometry::MeshTipAxis(Axis);
}

void AGridThrownItemActor::AlignMeshForwardToFlight()
{
	const UGridItemDefinitionAsset* Definition = GetItemDefinitionAsset();
	if (Definition && MeshComponent)
	{
		MeshComponent->SetRelativeRotation(FGridProjectileGeometry::MeshToFlightRotation(Definition->ProjectileMeshForwardAxis));
	}
}

void AGridThrownItemActor::ConfigureLaunchSourceActor(AActor* SourceActor)
{
	LaunchSourceActor = SourceActor;
	if (IsValid(SourceActor) && CollisionComponent)
	{
		CollisionComponent->IgnoreActorWhenMoving(SourceActor, true);
	}
}

void AGridThrownItemActor::ConfigureAsRangedAmmunition()
{
	AlignMeshForwardToFlight();
	bRangedAmmunitionProjectile = true;
	// Fly aligned to velocity, never with the shuriken's spin or tumble.
	AppliedThrowVisualAngularVelocityDegreesPerSecond = FVector::ZeroVector;
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
	}
}

void AGridThrownItemActor::ConfigureCombatPresentationTarget(bool bStopAtTarget, const FVector& TargetWorldLocation, float AcceptanceRadius,
	FIntPoint PreferredTargetCell)
{
	bStopsAtCombatPresentationTarget = bStopAtTarget && !TargetWorldLocation.ContainsNaN();
	CombatPresentationTargetLocation = TargetWorldLocation;
	CombatPresentationTargetAcceptanceRadius = FMath::Clamp(AcceptanceRadius, 1.0f, 100.0f);
	PreferredImpactCell = bRangedAmmunitionProjectile && bStopsAtCombatPresentationTarget
		? PreferredTargetCell : FIntPoint(INDEX_NONE, INDEX_NONE);
	PreviousPresentationLocation = GetActorLocation();
	SetActorTickEnabled(!bConversionAttempted);
}


void AGridThrownItemActor::ConfigureRangedHitEmbedding(AGridMonsterActor* HitMonster)
{
	PendingHitMonster = bRangedAmmunitionProjectile && IsValid(HitMonster) ? HitMonster : nullptr;
}

bool AGridThrownItemActor::ConfigureRangedMissDeflection(const FVector& TargetCenter, const FVector& TargetBoxExtent)
{
	if (!bRangedAmmunitionProjectile || bConversionAttempted || bEmbeddedInMonster ||
		bRangedMissDeflected || !IsValid(ProjectileMovementComponent))
	{
		return false;
	}

	const FVector CurrentLocation = GetActorLocation();
	const int32 SideSign = (GetTypeHash(ThrownItemInstance.RuntimeObjectId) & 1u) == 0u ? 1 : -1;
	FVector DeflectedDirection;
	FVector AimPoint;
	const float SphereRadius = CollisionComponent ? CollisionComponent->GetScaledSphereRadius() : 12.0f;
	if (!FGridProjectileGeometry::CalculateMissDeflection(CurrentLocation, TargetCenter, TargetBoxExtent,
		SphereRadius, 12.0f, SideSign, DeflectedDirection, AimPoint))
	{
		return false;
	}
	FVector Forward = TargetCenter - CurrentLocation;
	Forward.Z = 0.0f;
	if (!Forward.Normalize())
	{
		return false;
	}
	const float ExistingSpeed = ProjectileMovementComponent->Velocity.Size();
	if (ExistingSpeed <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	bRangedMissDeflected = true;
	RangedMissSideSign = SideSign;
	RangedMissTargetLocation = TargetCenter;
	RangedMissForward = Forward;
	bRangedMissGravityStarted = false;
	bStopsAtCombatPresentationTarget = false;
	PendingHitMonster = nullptr;
	// Change heading, not position or speed. ProjectileMovement sweeps
	// normally, and the root/mesh axes continue to follow the velocity.
	ProjectileMovementComponent->Velocity = DeflectedDirection * ExistingSpeed;
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
	SetActorRotation(DeflectedDirection.Rotation());
	SetActorTickEnabled(true);
	UE_LOG(LogGridThrownItem, Log,
		TEXT("[GridProjectile] MissDeflect Item=%s RuntimeId=%s Side=%s Aim=%s Source=%s Velocity=%s"),
		*ThrownItemInstance.ItemDefinitionId.ToString(), *ThrownItemInstance.RuntimeObjectId.ToString(),
		SideSign > 0 ? TEXT("Right") : TEXT("Left"), *AimPoint.ToCompactString(),
		*CurrentLocation.ToCompactString(), *ProjectileMovementComponent->Velocity.ToCompactString());
	return true;
}

FName AGridThrownItemActor::FindNearestAnimatedBone(USkeletalMeshComponent* Skeletal, const FVector& WorldTip)
{
	if (!IsValid(Skeletal) || !Skeletal->GetSkeletalMeshAsset())
	{
		return NAME_None;
	}

	// FindClosestBone may choose index 0 (root): for animated deaths, the
	// root can remain at the standing location while the body falls.
	// Prefer an actual pose bone when one exists; this also supports rigs
	// without physics assets, and avoids hard-coded skeleton-specific names.
	const int32 NumBones = Skeletal->GetNumBones();
	if (NumBones <= 1)
	{
		return NumBones == 1 ? Skeletal->GetBoneName(0) : NAME_None;
	}

	FName BestBone = NAME_None;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (int32 Index = 1; Index < NumBones; ++Index)
	{
		const FName Bone = Skeletal->GetBoneName(Index);
		const FVector BoneWorld = Skeletal->GetBoneLocation(Bone);
		if (Bone.IsNone() || BoneWorld.ContainsNaN())
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared(WorldTip, BoneWorld);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestBone = Bone;
			BestDistanceSquared = DistanceSquared;
		}
	}
	return BestBone.IsNone() ? Skeletal->GetBoneName(0) : BestBone;
}

void AGridThrownItemActor::KeepEmbeddedArrowOnAnimatedBone()
{
	AGridMonsterActor* Monster = EmbeddedMonster.Get();
	USkeletalMeshComponent* Skeletal = IsValid(Monster) ? Monster->SkeletalMeshComponent.Get() : nullptr;
	if (!IsValid(Skeletal))
	{
		return;
	}
	// A skeletal asset may be refreshed during runtime. Preserve the last
	// world position and re-anchor to the component if the bone disappears.
	if (!EmbeddedBone.IsNone() && Skeletal->GetBoneIndex(EmbeddedBone) == INDEX_NONE)
	{
		const FTransform WorldTransform = GetActorTransform();
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		EmbeddedBone = NAME_None;
		AttachToComponent(Skeletal, FAttachmentTransformRules::KeepWorldTransform);
		EmbeddedLocalBoneTransform = WorldTransform.GetRelativeTransform(Skeletal->GetComponentTransform());
		UE_LOG(LogGridThrownItem, Warning, TEXT("[GridProjectile] EmbeddedBoneMissing Item=%s Monster=%s Fallback=MeshComponent"),
			*ThrownItemInstance.ItemDefinitionId.ToString(), *GetNameSafe(Monster));
	}
	const FTransform Anchor = EmbeddedBone.IsNone()
		? Skeletal->GetComponentTransform() : Skeletal->GetSocketTransform(EmbeddedBone, RTS_World);
	const FTransform ExpectedWorld = EmbeddedLocalBoneTransform * Anchor;
	if (!GetActorTransform().Equals(ExpectedWorld, 0.1f))
	{
		const float Correction = FVector::Dist(GetActorLocation(), ExpectedWorld.GetLocation());
		SetActorTransform(ExpectedWorld, false, nullptr, ETeleportType::TeleportPhysics);
		if (Correction > 5.0f)
		{
			UE_LOG(LogGridThrownItem, Verbose, TEXT("[GridProjectile] EmbeddedFollowCorrection Item=%s Monster=%s Bone=%s DistanceCm=%.1f"),
				*ThrownItemInstance.ItemDefinitionId.ToString(), *GetNameSafe(Monster), *EmbeddedBone.ToString(), Correction);
		}
	}
}

bool AGridThrownItemActor::TryEmbedInMonster()
{
	if (!bRangedAmmunitionProjectile || bConversionAttempted || bEmbeddedInMonster)
	{
		return false;
	}
	AGridMonsterActor* Monster = PendingHitMonster.Get();
	if (!IsValid(Monster) || !IsValid(Monster->SkeletalMeshComponent) ||
		!Monster->SkeletalMeshComponent->IsVisible())
	{
		return false;
	}

	USkeletalMeshComponent* Skeletal = Monster->SkeletalMeshComponent;
	const FVector FlightDirection = (ProjectileMovementComponent && !ProjectileMovementComponent->Velocity.IsNearlyZero()
		? ProjectileMovementComponent->Velocity : GetActorForwardVector()).GetSafeNormal();
	if (FlightDirection.IsNearlyZero())
	{
		return false;
	}

	// Estimate the near face from the monster's gameplay collision box.
	// The visual tip sinks 8 cm inside, without relying on a physics asset.
	const FVector Center = IsValid(Monster->CollisionComponent)
		? Monster->CollisionComponent->Bounds.Origin : Monster->GetActorLocation();
	const FVector Extent = IsValid(Monster->CollisionComponent)
		? Monster->CollisionComponent->Bounds.BoxExtent : FVector(35.0f);
	const float NearFace = FMath::Abs(FlightDirection.X) * Extent.X +
		FMath::Abs(FlightDirection.Y) * Extent.Y + FMath::Abs(FlightDirection.Z) * Extent.Z;
	const uint32 Seed = GetTypeHash(ThrownItemInstance.RuntimeObjectId);
	const float HorizontalFactor = (static_cast<float>(Seed % 101u) / 100.0f) * 2.0f - 1.0f;
	const float VerticalFactor = (static_cast<float>((Seed / 101u) % 101u) / 100.0f) * 2.0f - 1.0f;
	const FVector SideAxis = FVector::CrossProduct(FVector::UpVector, FlightDirection).GetSafeNormal();
	const FVector UpAxis = FVector::CrossProduct(FlightDirection, SideAxis).GetSafeNormal();
	const FVector TipLocation = Center - FlightDirection * FMath::Max(0.0f, NearFace - 8.0f) +
		SideAxis * HorizontalFactor * Extent.GetMin() * 0.20f +
		UpAxis * VerticalFactor * Extent.Z * 0.18f;

	const FName Bone = FindNearestAnimatedBone(Skeletal, TipLocation);
	SetActorLocationAndRotation(TipLocation, FlightDirection.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (!AttachToComponent(Skeletal, FAttachmentTransformRules::KeepWorldTransform, Bone))
	{
		return false;
	}

	EmbeddedBone = Bone;
	EmbeddedLocalBoneTransform = GetActorTransform().GetRelativeTransform(
		Bone.IsNone() ? Skeletal->GetComponentTransform() : Skeletal->GetSocketTransform(Bone, RTS_World));
	// The monster's visual pose must update before the embedded projectile.
	AddTickPrerequisiteComponent(Skeletal);
	bEmbeddedInMonster = true;
	EmbeddedMonster = Monster;
	PendingHitMonster = nullptr;
	bStopsAtCombatPresentationTarget = false;
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->Deactivate();
	}
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	GetWorldTimerManager().ClearTimer(ExpirationTimerHandle);
	AppliedThrowVisualAngularVelocityDegreesPerSecond = FVector::ZeroVector;
	SetActorTickEnabled(true);
	KeepEmbeddedArrowOnAnimatedBone();
	UE_LOG(LogGridThrownItem, Log, TEXT("[GridProjectile] Embedded Item=%s RuntimeId=%s Monster=%s Bone=%s Tip=%s"),
		*ThrownItemInstance.ItemDefinitionId.ToString(), *ThrownItemInstance.RuntimeObjectId.ToString(),
		*GetNameSafe(Monster), *Bone.ToString(), *TipLocation.ToCompactString());
	return true;
}

void AGridThrownItemActor::ReleaseEmbeddedArrowToPickup()
{
	if (!bEmbeddedInMonster || bConversionAttempted)
	{
		return;
	}
	const FVector LastWorldTip = GetActorLocation();
	if (AGridMonsterActor* Monster = EmbeddedMonster.Get())
	{
		if (IsValid(Monster->SkeletalMeshComponent))
		{
			RemoveTickPrerequisiteComponent(Monster->SkeletalMeshComponent);
		}
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	bEmbeddedInMonster = false;
	EmbeddedMonster = nullptr;
	EmbeddedBone = NAME_None;
	EmbeddedLocalBoneTransform = FTransform::Identity;
	FHitResult ReleaseHit;
	ReleaseHit.Location = LastWorldTip;
	ReleaseHit.ImpactPoint = LastWorldTip;
	ReleaseHit.ImpactNormal = FVector::UpVector;
	UE_LOG(LogGridThrownItem, Log, TEXT("[GridProjectile] EmbeddedRelease Item=%s RuntimeId=%s Tip=%s Reason=MonsterMeshHidden"),
		*ThrownItemInstance.ItemDefinitionId.ToString(), *ThrownItemInstance.RuntimeObjectId.ToString(),
		*LastWorldTip.ToCompactString());
	ConvertToWorldPickupAtImpact(ReleaseHit);
}

void AGridThrownItemActor::HandleProjectileImpact(
	UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	(void)HitComponent;
	(void)OtherComp;
	(void)NormalImpulse;
	UE_LOG(LogGridThrownItem, Log, TEXT("[GridProjectile] Hit Item=%s Other=%s Point=%s TravelCm=%.1f"),
		*ThrownItemInstance.ItemDefinitionId.ToString(), *GetNameSafe(OtherActor), *Hit.ImpactPoint.ToCompactString(),
		FVector::Dist(InitialFlightWorldLocation, Hit.ImpactPoint));
	if (!bEmbeddedInMonster)
	{
		ConvertToWorldPickupAtImpact(Hit);
	}
}

void AGridThrownItemActor::HandleProjectileStopped(const FHitResult& Hit)
{
	if (!bEmbeddedInMonster)
	{
		ConvertToWorldPickupAtImpact(Hit);
	}
}

void AGridThrownItemActor::HandleProjectileExpired()
{
	if (bEmbeddedInMonster)
	{
		return;
	}
	FHitResult ExpirationHit;
	ExpirationHit.ImpactPoint = GetActorLocation();
	ExpirationHit.Location = GetActorLocation();
	ExpirationHit.ImpactNormal = FVector::UpVector;
	ConvertToWorldPickupAtImpact(ExpirationHit);
}

void AGridThrownItemActor::ConvertToWorldPickupAtImpact(const FHitResult& Hit)
{
	if (bConversionAttempted)
	{
		return;
	}
	bConversionAttempted = true;
	const bool bNearIntendedTarget = bStopsAtCombatPresentationTarget &&
		FVector::DistSquared(Hit.ImpactPoint, CombatPresentationTargetLocation) <= FMath::Square(150.0f);
	bStopsAtCombatPresentationTarget = false;
	SetActorTickEnabled(false);
	GetWorldTimerManager().ClearTimer(ExpirationTimerHandle);

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->Deactivate();
	}
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// The projectile already carries the canonical item definition. Reuse it
	// at conversion instead of asking runtime registries to rediscover an
	// item that may exist only in a transient level or inventory.
	UGridItemDefinitionAsset* ImpactItemDefinition = GetItemDefinitionAsset();
	int32 ImpactCellX = INDEX_NONE;
	int32 ImpactCellY = INDEX_NONE;
	FVector ImpactLocalOffset = FVector::ZeroVector;
	const FVector ResolvePoint = Hit.ImpactPoint + Hit.ImpactNormal.GetSafeNormal() * ImpactDropOffset;
	bool bDropped = false;
	// On a hit, the arrow is recoverable in the monster's cell, including
	// after death. Do not teleport a projectile stopped by an earlier wall.
	if (bRangedAmmunitionProjectile && bNearIntendedTarget && RuntimeActor &&
		RuntimeActor->IsWalkableCell(PreferredImpactCell.X, PreferredImpactCell.Y))
	{
		bDropped = RuntimeActor->TryDropItemInstanceAtCell(
			ThrownItemInstance, ImpactItemDefinition, PreferredImpactCell.X, PreferredImpactCell.Y, EGridEdge::None, FVector::ZeroVector);
	}
	if (!bDropped)
	{
		bDropped = RuntimeActor && RuntimeActor->TryResolveWorldCellFromImpactPoint(ResolvePoint, ImpactCellX, ImpactCellY, ImpactLocalOffset) &&
			RuntimeActor->TryDropItemInstanceAtCell(ThrownItemInstance, ImpactItemDefinition, ImpactCellX, ImpactCellY, EGridEdge::None, ImpactLocalOffset);
	}

	// Prefer the last actual traversed floor cell to teleporting back to the party.
	if (!bDropped && RuntimeActor && RuntimeActor->IsWalkableCell(LastWalkableFlightCell.X, LastWalkableFlightCell.Y))
	{
		bDropped = RuntimeActor->TryDropItemInstanceAtCell(ThrownItemInstance, ImpactItemDefinition,
			LastWalkableFlightCell.X, LastWalkableFlightCell.Y, EGridEdge::None, LastWalkableFlightOffset);
	}
	if (!bDropped && RuntimeActor && RuntimeActor->IsWalkableCell(SourceCellX, SourceCellY))
	{
		UE_LOG(LogGridThrownItem, Warning, TEXT("[GridProjectile] SourceFallback Item=%s Point=%s Source=(%d,%d) Last=(%d,%d)"),
			*ThrownItemInstance.ItemDefinitionId.ToString(), *Hit.ImpactPoint.ToCompactString(),
			SourceCellX, SourceCellY, LastWalkableFlightCell.X, LastWalkableFlightCell.Y);
		bDropped = RuntimeActor->TryDropItemInstanceAtCell(ThrownItemInstance, ImpactItemDefinition,
			SourceCellX, SourceCellY, EGridEdge::None, FVector::ZeroVector);
	}

	if (!bDropped)
	{
		UE_LOG(LogGridThrownItem, Warning, TEXT("GridInventory Throw Impact Failed Item=%s RuntimeId=%s SourceCell=(%d,%d) Reason=NoValidDropCell"),
			*ThrownItemInstance.ItemDefinitionId.ToString(), *ThrownItemInstance.RuntimeObjectId.ToString(), SourceCellX, SourceCellY);
	}

	UE_LOG(LogGridThrownItem, Log, TEXT("[GridProjectile] Landed Item=%s Success=%d Impact=%s Distance=%.1f"),
		*ThrownItemInstance.ItemDefinitionId.ToString(), bDropped ? 1 : 0,
		*Hit.ImpactPoint.ToCompactString(), FVector::Dist(InitialFlightWorldLocation, Hit.ImpactPoint));
	Destroy();
}
