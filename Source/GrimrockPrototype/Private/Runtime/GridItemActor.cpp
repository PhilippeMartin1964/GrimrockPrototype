#include "Runtime/GridItemActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridReceptacleActor.h"

AGridItemActor::AGridItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SparkleMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorldSparkleMesh"));
	SparkleMeshComponent->SetupAttachment(MeshComponent);
	SparkleMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SparkleMeshComponent->SetSimulatePhysics(false);
	SparkleMeshComponent->SetEnableGravity(false);
	SparkleMeshComponent->SetCastShadow(false);
	SparkleMeshComponent->SetVisibility(false, true);
}

void AGridItemActor::InitializeItem(FName InArchetypeId, const TArray<FName>& InItemTags, UStaticMesh* Mesh, UMaterialInterface* Material)
{
	ArchetypeId = InArchetypeId;
	ItemTags = InItemTags;
	if (ItemDefinitionId.IsNone())
	{
		ItemDefinitionId = InArchetypeId;
	}
	if (!RuntimeObjectId.IsValid())
	{
		RuntimeObjectId = FGuid::NewGuid();
	}

	if (MeshComponent)
	{
		if (Mesh)
		{
			MeshComponent->SetStaticMesh(Mesh);
		}
		if (Material)
		{
			MeshComponent->SetMaterial(0, Material);
		}
	}
	RefreshWorldSparklePresentation();
}

void AGridItemActor::OnPlacedInWorld()
{
	SetItemLightsEnabled(true);
}

void AGridItemActor::OnRemovedFromWorld()
{
	SetItemLightsEnabled(false);
	SetWorldSparkleEnabled(false);
}

void AGridItemActor::SetItemLightsEnabled(bool bEnabled)
{
	TArray<UGridLightEmitterComponent*> LightEmitters;
	GetComponents<UGridLightEmitterComponent>(LightEmitters);

	for (UGridLightEmitterComponent* LightEmitter : LightEmitters)
	{
		if (LightEmitter)
		{
			LightEmitter->SetLightEnabled(bEnabled);
		}
	}
}

void AGridItemActor::InitializeFromItemDefinition(UGridItemDefinitionAsset* InDefinition, const FGuid& InRuntimeObjectId)
{
	if (!InDefinition)
	{
		return;
	}

	ItemDefinitionAsset = InDefinition;
	ItemDefinitionId = InDefinition->ItemDefinitionId;
	if (ArchetypeId.IsNone())
	{
		ArchetypeId = ItemDefinitionId;
	}
	ItemTags = InDefinition->ItemTags;
	SetRuntimeObjectId(InRuntimeObjectId);
	if (!RuntimeObjectId.IsValid())
	{
		RuntimeObjectId = FGuid::NewGuid();
	}

	if (MeshComponent)
	{
		if (UStaticMesh* WorldMesh = InDefinition->WorldMesh.LoadSynchronous())
		{
			MeshComponent->SetStaticMesh(WorldMesh);
		}
	}
	RefreshWorldSparklePresentation();
}

void AGridItemActor::InitializeFromItemDefinitionId(FName InItemDefinitionId, const FGuid& InRuntimeObjectId)
{
	if (InItemDefinitionId.IsNone())
	{
		return;
	}

	ItemDefinitionAsset = nullptr;
	ItemDefinitionId = InItemDefinitionId;
	if (ArchetypeId.IsNone())
	{
		ArchetypeId = InItemDefinitionId;
	}
	SetRuntimeObjectId(InRuntimeObjectId);
	if (!RuntimeObjectId.IsValid())
	{
		RuntimeObjectId = FGuid::NewGuid();
	}
}

void AGridItemActor::InitializeReadableContent(
	UGridReadableContentAsset* InReadableContentAsset, FName InReadableContentId, const FText& InReadTitleOverride, const FText& InReadTextOverride)
{
	ReadableContentAsset = InReadableContentAsset;
	ReadableContentId = InReadableContentId;
	ReadTitleOverride = InReadTitleOverride;
	ReadTextOverride = InReadTextOverride;

	if (ReadableContentAsset && ReadableContentId.IsNone())
	{
		ReadableContentId = ReadableContentAsset->ReadableContentId;
	}
}

bool AGridItemActor::AreItemLightsEnabled() const
{
	TArray<UGridLightEmitterComponent*> LightEmitters;
	GetComponents<UGridLightEmitterComponent>(LightEmitters);

	for (const UGridLightEmitterComponent* LightEmitter : LightEmitters)
	{
		if (LightEmitter)
		{
			return LightEmitter->IsLightEnabled();
		}
	}

	return false;
}

void AGridItemActor::SetRuntimeObjectId(FGuid InRuntimeObjectId)
{
	if (InRuntimeObjectId.IsValid())
	{
		RuntimeObjectId = InRuntimeObjectId;
	}
}

FGuid AGridItemActor::GetRuntimeObjectId() const
{
	return RuntimeObjectId;
}

void AGridItemActor::ConfigureAsWorldPickup()
{
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	MeshComponent->SetEnableGravity(true);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->WakeRigidBody();
	SetWorldSparkleEnabled(true);
}

void AGridItemActor::ConfigureAsAttachedItem()
{
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetSimulatePhysics(false);
	MeshComponent->SetEnableGravity(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetWorldSparkleEnabled(false);
}

void AGridItemActor::SetWorldSparkleEnabled(bool bEnabled)
{
	const bool bCanEnable = bEnabled && ItemDefinitionAsset && ItemDefinitionAsset->bEnableWorldSparkle &&
		!ItemDefinitionAsset->WorldSparkleMaterial.IsNull() && MeshComponent && MeshComponent->GetStaticMesh() != nullptr;

	bWorldSparkleActive = bCanEnable;
	RefreshWorldSparklePresentation();
}

float AGridItemActor::BuildWorldSparklePhase() const
{
	uint32 Hash = RuntimeObjectId.IsValid() ? GetTypeHash(RuntimeObjectId) : GetTypeHash(GetItemDefinitionId());
	Hash = HashCombine(Hash, GetTypeHash(ArchetypeId));
	return static_cast<float>(Hash % 10000u) / 10000.0f;
}

void AGridItemActor::RefreshWorldSparklePresentation()
{
	if (!SparkleMeshComponent)
	{
		return;
	}

	SparkleMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SparkleMeshComponent->SetSimulatePhysics(false);
	SparkleMeshComponent->SetEnableGravity(false);
	SparkleMeshComponent->SetCastShadow(false);
	SparkleMeshComponent->SetRelativeTransform(FTransform::Identity);

	UStaticMesh* SourceMesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
	SparkleMeshComponent->SetStaticMesh(SourceMesh);

	if (!bWorldSparkleActive || !ItemDefinitionAsset || !SourceMesh)
	{
		SparkleMeshComponent->SetVisibility(false, true);
		WorldSparkleMaterialInstance = nullptr;
		return;
	}

	UMaterialInterface* SparkleMaterial = ItemDefinitionAsset->WorldSparkleMaterial.LoadSynchronous();
	if (!SparkleMaterial)
	{
		bWorldSparkleActive = false;
		SparkleMeshComponent->SetVisibility(false, true);
		WorldSparkleMaterialInstance = nullptr;
		return;
	}

	WorldSparkleMaterialInstance = UMaterialInstanceDynamic::Create(SparkleMaterial, this);
	if (!WorldSparkleMaterialInstance)
	{
		bWorldSparkleActive = false;
		SparkleMeshComponent->SetVisibility(false, true);
		return;
	}

	WorldSparkleMaterialInstance->SetVectorParameterValue(TEXT("SparkleColor"), ItemDefinitionAsset->WorldSparkleColor);
	WorldSparkleMaterialInstance->SetScalarParameterValue(TEXT("SparkleIntensity"), FMath::Max(0.0f, ItemDefinitionAsset->WorldSparkleIntensity));
	WorldSparkleMaterialInstance->SetScalarParameterValue(TEXT("SparkleSpeed"), FMath::Max(0.0f, ItemDefinitionAsset->WorldSparkleSpeed));
	WorldSparkleMaterialInstance->SetScalarParameterValue(TEXT("SparkleVariation"), FMath::Clamp(ItemDefinitionAsset->WorldSparkleVariation, 0.0f, 1.0f));
	WorldSparkleMaterialInstance->SetScalarParameterValue(TEXT("SparklePhase"), BuildWorldSparklePhase());

	const int32 MaterialCount = FMath::Max(1, SparkleMeshComponent->GetNumMaterials());
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		SparkleMeshComponent->SetMaterial(MaterialIndex, WorldSparkleMaterialInstance);
	}

	SparkleMeshComponent->SetVisibility(true, true);
}

FName AGridItemActor::GetItemArchetypeId() const
{
	return ArchetypeId;
}

UGridItemDefinitionAsset* AGridItemActor::GetItemDefinitionAsset() const
{
	return ItemDefinitionAsset;
}

FName AGridItemActor::GetItemDefinitionId() const
{
	if (ItemDefinitionAsset && !ItemDefinitionAsset->ItemDefinitionId.IsNone())
	{
		return ItemDefinitionAsset->ItemDefinitionId;
	}
	return ItemDefinitionId;
}

bool AGridItemActor::HasItemTag(FName Tag) const
{
	return !Tag.IsNone() && ItemTags.Contains(Tag);
}

void AGridItemActor::SetRuntimeCell(int32 InCellX, int32 InCellY)
{
	RuntimeCellX = InCellX;
	RuntimeCellY = InCellY;
}

bool AGridItemActor::CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
	if (!InstigatorPawn || !HitComponent || HitComponent != MeshComponent || (ArchetypeId.IsNone() && GetItemDefinitionId().IsNone()))
	{
		return false;
	}
	if (const AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor>(GetOwner()))
	{
		return OwnerReceptacle->CanInteract_Implementation(const_cast<APawn*>(InstigatorPawn), MeshComponent);
	}

	const AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	const AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	return PartyPawn && RuntimeActor && RuntimeActor->CanPartyPickupItemActor(this, PartyPawn);
}

void AGridItemActor::Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
{
	if (!CanInteract_Implementation(InstigatorPawn, HitComponent))
	{
		return;
	}

	AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	if (!PartyPawn)
	{
		return;
	}

	if (AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor>(GetOwner()))
	{
		OwnerReceptacle->Interact_Implementation(PartyPawn, MeshComponent);
		return;
	}

	AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	if (RuntimeActor)
	{
		RuntimeActor->TryPickupItemActor(this, PartyPawn);
	}
}

void AGridItemActor::InteractWithHit_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult)
{
	(void)HitResult;
	Interact_Implementation(InstigatorPawn, HitComponent);
}

EGridInteractionCursor AGridItemActor::GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent != MeshComponent || (ArchetypeId.IsNone() && GetItemDefinitionId().IsNone()))
	{
		return EGridInteractionCursor::Default;
	}
	if (const AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor>(GetOwner()))
	{
		return OwnerReceptacle->GetInteractionCursor_Implementation(MeshComponent);
	}
	return EGridInteractionCursor::Take;
}

FText AGridItemActor::GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent != MeshComponent || (ArchetypeId.IsNone() && GetItemDefinitionId().IsNone()))
	{
		return FText::GetEmpty();
	}
	if (const AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor>(GetOwner()))
	{
		return OwnerReceptacle->GetInteractionText_Implementation(MeshComponent);
	}
	return FText::FromString(TEXT("Take"));
}
