#include "Runtime/GridGenericObjectActor.h"

#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

AGridGenericObjectActor::AGridGenericObjectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	PointLightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLightComponent->SetupAttachment(SceneRoot);
	PointLightComponent->SetVisibility(false);
	PointLightComponent->SetIntensity(0.f);
	PointLightComponent->SetAttenuationRadius(250.f);
}


void AGridGenericObjectActor::InitializeRuntimeGenericObject(
	const FGridRuntimeWorldObjectData& ObjectData, const UGridWorldObjectDefinitionAsset* Definition, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	(void)Mesh;
	SourceWorldObjectDefinition = Definition;

	// WORLDOBJ-MIG03.4: generic world-object presentation is defined only by StaticPart.
	UStaticMesh* ResolvedMesh = Definition && Definition->StaticPart.IsDefined() ? Definition->StaticPart.Mesh.Get() : nullptr;
	InitializeRuntimeWorldObject(ObjectData, ResolvedMesh, WorldTransform);
	if (MeshComponent)
	{
		MeshComponent->SetRelativeTransform(Definition ? Definition->StaticPart.LocalTransform : FTransform::Identity);
		MeshComponent->SetVisibility(ResolvedMesh != nullptr, true);
	}

	ApplyDefinitionOptions(Definition);
	if (!ObjectData.OverrideReadableText.IsEmpty())
	{
		RuntimeReadableText = ObjectData.OverrideReadableText;
	}
}

bool AGridGenericObjectActor::HasReadableText() const
{
	return !RuntimeReadableText.IsEmpty();
}

FText AGridGenericObjectActor::GetReadableText() const
{
	return RuntimeReadableText;
}

void AGridGenericObjectActor::MarkAsRead()
{
	bRuntimeHasBeenRead = true;
}

void AGridGenericObjectActor::ApplyDefinitionOptions(const UGridWorldObjectDefinitionAsset* Definition)
{
	RuntimeReadableText = FText::GetEmpty();
	bRuntimeReadableOnlyOnce = false;
	bRuntimeHasBeenRead = false;

	if (!Definition)
	{
		if (PointLightComponent)
		{
			PointLightComponent->SetVisibility(false);
			PointLightComponent->SetIntensity(0.f);
		}
		return;
	}

	if (MeshComponent)
	{
		const bool bIsReadable = Definition->IsReadable();
		const ECollisionEnabled::Type CollisionMode =
			Definition->bBlocksMovement || bIsReadable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision;
		MeshComponent->SetCollisionEnabled(CollisionMode);

		if (bIsReadable)
		{
			MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
	}

	if (Definition->IsReadable())
	{
		RuntimeReadableText = Definition->ReadableText;
		bRuntimeReadableOnlyOnce = Definition->bShowReadableOnlyOnce;
	}

	if (!PointLightComponent)
	{
		return;
	}

	const bool bEnableLight = Definition->IsLightSource();
	PointLightComponent->SetVisibility(bEnableLight);

	if (bEnableLight)
	{
		PointLightComponent->SetLightColor(Definition->LightColor);
		PointLightComponent->SetIntensity(Definition->LightIntensity);
		PointLightComponent->SetAttenuationRadius(Definition->LightRadius);
	}
	else
	{
		PointLightComponent->SetIntensity(0.f);
	}
}

bool AGridGenericObjectActor::CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
	if (!InstigatorPawn || !HitComponent)
	{
		return false;
	}

	if (HitComponent != MeshComponent)
	{
		return false;
	}

	if (!HasReadableText())
	{
		return false;
	}

	if (bRuntimeReadableOnlyOnce && bRuntimeHasBeenRead)
	{
		return false;
	}

	const AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	if (Edge != EGridEdge::None && (!PartyPawn || !RuntimeActor || !RuntimeActor->CanPartyInteractWithEdgeObject(CellX, CellY, Edge, PartyPawn)))
	{
		return false;
	}

	return true;
}

void AGridGenericObjectActor::Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
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

	AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	if (RuntimeActor && (Edge == EGridEdge::None || RuntimeActor->CanPartyInteractWithEdgeObject(CellX, CellY, Edge, PartyPawn)))
	{
		RuntimeActor->TryInteractAtEdge(CellX, CellY, Edge, PartyPawn);
	}
}

EGridInteractionCursor AGridGenericObjectActor::GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent == MeshComponent && HasReadableText() && !(bRuntimeReadableOnlyOnce && bRuntimeHasBeenRead))
	{
		return EGridInteractionCursor::Read;
	}

	return EGridInteractionCursor::Default;
}

FText AGridGenericObjectActor::GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent == MeshComponent && HasReadableText())
	{
		return FText::FromString(TEXT("Read"));
	}

	return FText::GetEmpty();
}
