#include "Runtime/GridItemActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridLightEmitterComponent.h"

AGridItemActor::AGridItemActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("Mesh"));
    MeshComponent->SetupAttachment (SceneRoot);
    MeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);

    ItemLightComponent = CreateDefaultSubobject<UPointLightComponent> (TEXT ("ItemLight"));
    ItemLightComponent->SetupAttachment (SceneRoot);
    ItemLightComponent->SetVisibility (false);
    ItemLightComponent->SetIntensity (0.f);
    ItemLightComponent->SetAttenuationRadius (250.f);
}

void AGridItemActor::InitializeItem (FName InArchetypeId, const TArray<FName>& InItemTags, UStaticMesh* Mesh, UMaterialInterface* Material)
{
    ArchetypeId = InArchetypeId;
    ItemTags = InItemTags;

    if (MeshComponent)
    {
        MeshComponent->SetStaticMesh (Mesh);
        if (Material)
        {
            MeshComponent->SetMaterial (0, Material);
        }
    }
}

void AGridItemActor::InitializeItemFromArchetype (const UGridObjectArchetypeAsset* ItemArchetype)
{
    if (!ItemArchetype)
    {
        InitializeItem (NAME_None, TArray<FName> (), nullptr, nullptr);
        ApplyItemLightFromArchetype (nullptr);
        return;
    }

    UStaticMesh* ItemMesh = ItemArchetype->MovingMesh ? ItemArchetype->MovingMesh.Get () : ItemArchetype->PreviewMesh.Get ();
    if (!ItemMesh)
    {
        ItemMesh = ItemArchetype->FixedMesh.Get ();
    }

    UMaterialInterface* ItemMaterial = ItemArchetype->MovingMaterial ? ItemArchetype->MovingMaterial.Get () : ItemArchetype->PreviewMaterial.Get ();
    if (!ItemMaterial)
    {
        ItemMaterial = ItemArchetype->FixedMaterial.Get ();
    }

    InitializeItem (ItemArchetype->ArchetypeId, ItemArchetype->ItemTags, ItemMesh, ItemMaterial);
    ApplyItemLightFromArchetype (ItemArchetype);
}

void AGridItemActor::ApplyItemLightFromArchetype (const UGridObjectArchetypeAsset* ItemArchetype)
{
    bHasArchetypeLightConfig = true;
    bRuntimeIsLightSource = ItemArchetype && ItemArchetype->bIsLightSource;

    TArray<UGridLightEmitterComponent*> LightEmitters;
    GetComponents<UGridLightEmitterComponent> (LightEmitters);

    for (UGridLightEmitterComponent* LightEmitter : LightEmitters)
    {
        if (!LightEmitter)
        {
            continue;
        }

        if (ItemArchetype)
        {
            LightEmitter->LightColor = ItemArchetype->LightColor;
            LightEmitter->LightIntensity = ItemArchetype->LightIntensity;
            LightEmitter->LightRadius = ItemArchetype->LightRadius;
            LightEmitter->BaseLightColor = ItemArchetype->LightColor;
            LightEmitter->BaseLightIntensity = ItemArchetype->LightIntensity;
            LightEmitter->BaseAttenuationRadius = ItemArchetype->LightRadius;
            LightEmitter->bEnableLightFlicker = ItemArchetype->bUseLightFlicker;
            LightEmitter->bEnableLightColorFlicker = ItemArchetype->bUseLightFlicker;
            LightEmitter->bEnableLightPositionFlicker = ItemArchetype->bUseLightFlicker;
        }

        LightEmitter->SetLightEnabled (bRuntimeIsLightSource);
    }

    const bool bUseNativeLightFallback = bRuntimeIsLightSource && LightEmitters.Num () == 0;
    if (ItemLightComponent)
    {
        ItemLightComponent->SetVisibility (bUseNativeLightFallback);
        if (bUseNativeLightFallback && ItemArchetype)
        {
            ItemLightComponent->SetLightColor (ItemArchetype->LightColor);
            ItemLightComponent->SetIntensity (ItemArchetype->LightIntensity);
            ItemLightComponent->SetAttenuationRadius (ItemArchetype->LightRadius);
        }
        else
        {
            ItemLightComponent->SetIntensity (0.f);
        }
    }
}

void AGridItemActor::SetRuntimeLightEnabled (bool bEnabled)
{
    TArray<UGridLightEmitterComponent*> LightEmitters;
    GetComponents<UGridLightEmitterComponent> (LightEmitters);

    for (UGridLightEmitterComponent* LightEmitter : LightEmitters)
    {
        if (LightEmitter)
        {
            LightEmitter->SetLightEnabled (bEnabled);
        }
    }

    if (ItemLightComponent)
    {
        const bool bUseNativeLight = bEnabled && bHasArchetypeLightConfig && LightEmitters.Num () == 0;
        ItemLightComponent->SetVisibility (bUseNativeLight);
    }
}

void AGridItemActor::OnPlacedInWorld ()
{
    SetRuntimeLightEnabled (bHasArchetypeLightConfig ? bRuntimeIsLightSource : true);
}

void AGridItemActor::OnRemovedFromWorld ()
{
    SetRuntimeLightEnabled (false);
}

bool AGridItemActor::HasItemTag (FName Tag) const
{
    return !Tag.IsNone () && ItemTags.Contains (Tag);
}
