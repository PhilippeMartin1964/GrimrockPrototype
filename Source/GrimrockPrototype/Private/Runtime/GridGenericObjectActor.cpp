#include "Runtime/GridGenericObjectActor.h"

#include "Core/GridObjectArchetypeAsset.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"

AGridGenericObjectActor::AGridGenericObjectActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    PointLightComponent = CreateDefaultSubobject<UPointLightComponent> (TEXT ("PointLight"));
    PointLightComponent->SetupAttachment (SceneRoot);
    PointLightComponent->SetVisibility (false);
    PointLightComponent->SetIntensity (0.f);
    PointLightComponent->SetAttenuationRadius (250.f);
}

void AGridGenericObjectActor::InitializeGenericObject (const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype,
    UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform)
{
    SourceArchetype = Archetype;
    InitializeGridObject (ObjectData, Mesh, Material, WorldTransform);
    ApplyArchetypeOptions (Archetype);
}

bool AGridGenericObjectActor::HasReadableText () const
{
    return !RuntimeReadableText.IsEmpty ();
}

FText AGridGenericObjectActor::GetReadableText () const
{
    return RuntimeReadableText;
}

void AGridGenericObjectActor::MarkAsRead ()
{
    bRuntimeHasBeenRead = true;
}

void AGridGenericObjectActor::ApplyArchetypeOptions (const UGridObjectArchetypeAsset* Archetype)
{
    RuntimeReadableText = FText::GetEmpty ();
    bRuntimeReadableOnlyOnce = false;
    bRuntimeHasBeenRead = false;

    if (!Archetype)
    {
        if (PointLightComponent)
        {
            PointLightComponent->SetVisibility (false);
            PointLightComponent->SetIntensity (0.f);
        }
        return;
    }

    if (MeshComponent)
    {
        const ECollisionEnabled::Type CollisionMode = Archetype->bBlocksMovement
            ? ECollisionEnabled::QueryAndPhysics
            : ECollisionEnabled::NoCollision;
        MeshComponent->SetCollisionEnabled (CollisionMode);
    }

    if (Archetype->IsReadable ())
    {
        RuntimeReadableText = Archetype->ReadableText;
        bRuntimeReadableOnlyOnce = Archetype->bShowReadableOnlyOnce;
    }

    if (!PointLightComponent)
    {
        return;
    }

    const bool bEnableLight = Archetype->IsLightSource ();
    PointLightComponent->SetVisibility (bEnableLight);

    if (bEnableLight)
    {
        PointLightComponent->SetLightColor (Archetype->LightColor);
        PointLightComponent->SetIntensity (Archetype->LightIntensity);
        PointLightComponent->SetAttenuationRadius (Archetype->LightRadius);
    }
    else
    {
        PointLightComponent->SetIntensity (0.f);
    }
}
