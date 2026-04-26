#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridTypes.h"
#include "GridEditorPreviewObjectActor.generated.h"

class UStaticMeshComponent;

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridEditorPreviewObjectActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridEditorPreviewObjectActor ();

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY (BlueprintReadOnly, Category = "Grid")
    FGuid ObjectId;

    UPROPERTY (BlueprintReadOnly, Category = "Grid")
    EGridLevelObjectType ObjectType = EGridLevelObjectType::None;

    UFUNCTION (BlueprintCallable, Category = "Preview")
    void InitializePreviewObject (
        const FGridLevelObjectData& ObjectData,
        UStaticMesh* Mesh,
        UMaterialInterface* NormalMaterial,
        UMaterialInterface* HighlightMaterial);

    UFUNCTION (BlueprintCallable, Category = "Preview")
    void SetHighlighted (bool bHighlighted);

private:
    UPROPERTY (Transient)
    TObjectPtr<UMaterialInterface> CachedNormalMaterial;

    UPROPERTY (Transient)
    TObjectPtr<UMaterialInterface> CachedHighlightMaterial;
};