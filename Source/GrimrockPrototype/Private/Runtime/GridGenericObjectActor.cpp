#include "Runtime/GridGenericObjectActor.h"

#include "Core/GridObjectArchetypeAsset.h"
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

void AGridGenericObjectActor::InitializeGenericObject(const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype, UStaticMesh* Mesh,
	const FTransform& WorldTransform)
{
	SourceArchetype = Archetype;
	InitializeGridObject(ObjectData, Mesh, WorldTransform);
	ApplyArchetypeOptions(Archetype);
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

void AGridGenericObjectActor::ApplyArchetypeOptions(const UGridObjectArchetypeAsset* Archetype)
{
	RuntimeReadableText = FText::GetEmpty();
	bRuntimeReadableOnlyOnce = false;
	bRuntimeHasBeenRead = false;

	if (!Archetype)
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
		const bool bIsReadable = Archetype->IsReadable();
		const ECollisionEnabled::Type CollisionMode =
			Archetype->bBlocksMovement || bIsReadable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision;
		MeshComponent->SetCollisionEnabled(CollisionMode);

		if (bIsReadable)
		{
			MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
	}

	if (Archetype->IsReadable())
	{
		RuntimeReadableText = Archetype->ReadableText;
		bRuntimeReadableOnlyOnce = Archetype->bShowReadableOnlyOnce;
	}

	if (!PointLightComponent)
	{
		return;
	}

	const bool bEnableLight = Archetype->IsLightSource();
	PointLightComponent->SetVisibility(bEnableLight);

	if (bEnableLight)
	{
		PointLightComponent->SetLightColor(Archetype->LightColor);
		PointLightComponent->SetIntensity(Archetype->LightIntensity);
		PointLightComponent->SetAttenuationRadius(Archetype->LightRadius);
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
