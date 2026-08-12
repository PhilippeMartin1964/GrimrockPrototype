#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridTypes.h"
#include "GridEditorPreviewObjectActor.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class UGridMonsterDefinitionAsset;

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

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USkeletalMeshComponent* SkeletalMeshComponent;

    UPROPERTY (BlueprintReadOnly, Category = "Grid")
    FGuid ObjectId;

    UPROPERTY (BlueprintReadOnly, Category = "Grid")
    EGridLevelObjectType ObjectType = EGridLevelObjectType::None;

    UFUNCTION (BlueprintCallable, Category = "Preview")
    void InitializePreviewObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* NormalMaterial);

    void InitializeMonsterPreviewObject (
        const FGridLevelObjectData& ObjectData,
        UGridMonsterDefinitionAsset* MonsterDefinition);

    UFUNCTION (BlueprintCallable, Category = "Preview")
    void SetHovered (bool bHovered);

    UFUNCTION (BlueprintCallable, Category = "Preview")
    void SetSelected (bool bSelected);

private:
    void RefreshStencilState ();

    bool bIsHovered = false;
    bool bIsSelected = false;
private:
    UPROPERTY (Transient)
    TObjectPtr<UMaterialInterface> CachedNormalMaterial;
};
