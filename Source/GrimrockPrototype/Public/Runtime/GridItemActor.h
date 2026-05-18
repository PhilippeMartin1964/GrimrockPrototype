#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridItemActor.generated.h"

class UStaticMeshComponent;
class UGridObjectArchetypeAsset;
class UPointLightComponent;

UCLASS (Blueprintable)
class GRIMROCKPROTOTYPE_API AGridItemActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridItemActor ();

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UPointLightComponent> ItemLightComponent;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName ArchetypeId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    TArray<FName> ItemTags;

    UFUNCTION (BlueprintCallable, Category = "Item")
    virtual void InitializeItem (FName InArchetypeId, const TArray<FName>& InItemTags, UStaticMesh* Mesh, UMaterialInterface* Material);

    UFUNCTION (BlueprintCallable, Category = "Item")
    virtual void InitializeItemFromArchetype (const UGridObjectArchetypeAsset* ItemArchetype);

    UFUNCTION (BlueprintCallable, Category = "Item")
    virtual void OnPlacedInWorld ();

    UFUNCTION (BlueprintCallable, Category = "Item")
    virtual void OnRemovedFromWorld ();

    UFUNCTION (BlueprintCallable, Category = "Item")
    bool HasItemTag (FName Tag) const;

protected:
    bool bHasArchetypeLightConfig = false;
    bool bRuntimeIsLightSource = false;

    void ApplyItemLightFromArchetype (const UGridObjectArchetypeAsset* ItemArchetype);
    void SetRuntimeLightEnabled (bool bEnabled);
};
