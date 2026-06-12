#include "Runtime/GridThrownItemActor.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "TimerManager.h"

AGridThrownItemActor::AGridThrownItemActor ()
{
    CollisionComponent = CreateDefaultSubobject<USphereComponent> (TEXT ("ProjectileCollision"));
    CollisionComponent->InitSphereRadius (12.0f);
    CollisionComponent->SetCollisionEnabled (ECollisionEnabled::QueryAndPhysics);
    CollisionComponent->SetCollisionObjectType (ECC_WorldDynamic);
    CollisionComponent->SetCollisionResponseToAllChannels (ECR_Ignore);
    CollisionComponent->SetCollisionResponseToChannel (ECC_WorldStatic, ECR_Block);
    CollisionComponent->SetCollisionResponseToChannel (ECC_WorldDynamic, ECR_Block);
    CollisionComponent->SetCollisionResponseToChannel (ECC_PhysicsBody, ECR_Block);
    CollisionComponent->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
    CollisionComponent->SetNotifyRigidBodyCollision (true);
    SetRootComponent (CollisionComponent);

    if (SceneRoot)
    {
        SceneRoot->SetupAttachment (CollisionComponent);
    }
    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
        MeshComponent->SetSimulatePhysics (false);
        MeshComponent->SetEnableGravity (false);
    }

    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent> (TEXT ("ProjectileMovement"));
    ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
    ProjectileMovementComponent->bRotationFollowsVelocity = true;
    ProjectileMovementComponent->bShouldBounce = false;
    ProjectileMovementComponent->ProjectileGravityScale = 1.0f;
    ProjectileMovementComponent->InitialSpeed = 0.0f;
    ProjectileMovementComponent->MaxSpeed = 0.0f;
}

void AGridThrownItemActor::BeginPlay ()
{
    Super::BeginPlay ();
    if (CollisionComponent)
    {
        CollisionComponent->OnComponentHit.AddDynamic (this, &AGridThrownItemActor::HandleProjectileImpact);
    }
    if (ProjectileMovementComponent)
    {
        ProjectileMovementComponent->OnProjectileStop.AddDynamic (this, &AGridThrownItemActor::HandleProjectileStopped);
    }
}

void AGridThrownItemActor::InitializeThrownItem (
    AGridLevelRuntimeActor* InRuntimeActor,
    const FGridItemInstance& InItemInstance,
    UGridItemDefinitionAsset* InDefinition,
    const FVector& LaunchVelocity,
    int32 InSourceCellX,
    int32 InSourceCellY)
{
    RuntimeActor = InRuntimeActor;
    ThrownItemInstance = InItemInstance;
    SourceCellX = InSourceCellX;
    SourceCellY = InSourceCellY;
    ImpactDropOffset = InDefinition ? FMath::Max (0.0f, InDefinition->ThrowImpactDropOffset) : 12.0f;

    InitializeFromItemDefinition (InDefinition, InItemInstance.RuntimeObjectId);
    ConfigureAsAttachedItem ();
    SetItemLightsEnabled (InItemInstance.bLightsEnabled);

    if (ProjectileMovementComponent)
    {
        ProjectileMovementComponent->Velocity = LaunchVelocity;
        ProjectileMovementComponent->InitialSpeed = LaunchVelocity.Size ();
        ProjectileMovementComponent->MaxSpeed = LaunchVelocity.Size ();
        ProjectileMovementComponent->Activate (true);
    }

    const float LifeSeconds = InDefinition ? FMath::Max (0.01f, InDefinition->ThrowLifeSeconds) : 5.0f;
    GetWorldTimerManager ().SetTimer (
        ExpirationTimerHandle,
        this,
        &AGridThrownItemActor::HandleProjectileExpired,
        LifeSeconds,
        false);
}

void AGridThrownItemActor::HandleProjectileImpact (
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    (void)HitComponent;
    (void)OtherActor;
    (void)OtherComp;
    (void)NormalImpulse;
    ConvertToWorldPickupAtImpact (Hit);
}

void AGridThrownItemActor::HandleProjectileStopped (const FHitResult& Hit)
{
    ConvertToWorldPickupAtImpact (Hit);
}

void AGridThrownItemActor::HandleProjectileExpired ()
{
    FHitResult ExpirationHit;
    ExpirationHit.ImpactPoint = GetActorLocation ();
    ExpirationHit.Location = GetActorLocation ();
    ExpirationHit.ImpactNormal = FVector::UpVector;
    ConvertToWorldPickupAtImpact (ExpirationHit);
}

void AGridThrownItemActor::ConvertToWorldPickupAtImpact (const FHitResult& Hit)
{
    if (bConversionAttempted)
    {
        return;
    }
    bConversionAttempted = true;
    GetWorldTimerManager ().ClearTimer (ExpirationTimerHandle);

    if (ProjectileMovementComponent)
    {
        ProjectileMovementComponent->StopMovementImmediately ();
        ProjectileMovementComponent->Deactivate ();
    }
    if (CollisionComponent)
    {
        CollisionComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    }

    int32 ImpactCellX = INDEX_NONE;
    int32 ImpactCellY = INDEX_NONE;
    FVector ImpactLocalOffset = FVector::ZeroVector;
    const FVector ResolvePoint = Hit.ImpactPoint + Hit.ImpactNormal.GetSafeNormal () * ImpactDropOffset;
    bool bDropped = RuntimeActor &&
        RuntimeActor->TryResolveWorldCellFromImpactPoint (
            ResolvePoint,
            ImpactCellX,
            ImpactCellY,
            ImpactLocalOffset) &&
        RuntimeActor->TryDropItemInstanceAtCell (
            ThrownItemInstance,
            ImpactCellX,
            ImpactCellY,
            EGridEdge::None,
            ImpactLocalOffset);

    if (!bDropped && RuntimeActor && RuntimeActor->IsWalkableCell (SourceCellX, SourceCellY))
    {
        bDropped = RuntimeActor->TryDropItemInstanceAtCell (
            ThrownItemInstance,
            SourceCellX,
            SourceCellY,
            EGridEdge::None,
            FVector::ZeroVector);
    }

    if (!bDropped)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridInventory Throw Impact Failed Item=%s RuntimeId=%s SourceCell=(%d,%d) Reason=NoValidDropCell"),
            *ThrownItemInstance.ItemDefinitionId.ToString (),
            *ThrownItemInstance.RuntimeObjectId.ToString (),
            SourceCellX,
            SourceCellY);
    }

    Destroy ();
}
