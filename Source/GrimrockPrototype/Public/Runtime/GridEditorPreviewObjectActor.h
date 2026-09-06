#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridTypes.h"
#include "GridEditorPreviewObjectActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UGridMonsterDefinitionAsset;
class UGridObjectArchetypeAsset;

UCLASS()
class GRIMROCKPROTOTYPE_API AGridEditorPreviewObjectActor : public AActor
{
	GENERATED_BODY()

public:
	AGridEditorPreviewObjectActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneRoot;

	/** StaticPart for the target visual composition. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MovingPart0MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MovingPart1MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* SkeletalMeshComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	FGuid ObjectId;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	EGridLevelObjectType ObjectType = EGridLevelObjectType::None;

	/** Initializes a standalone single-mesh preview when explicitly requested by a caller. */
	UFUNCTION(BlueprintCallable, Category = "Preview")
	void InitializePreviewObject(const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh);

	/** MIG03 target entry point: renders StaticPart + MovingPart[0..1] from the same definition used by runtime. */
	void InitializePreviewObjectFromArchetype(const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype);

	void InitializeMonsterPreviewObject(const FGridLevelObjectData& ObjectData, UGridMonsterDefinitionAsset* MonsterDefinition);

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetHovered(bool bHovered);

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetSelected(bool bSelected);

private:
	void ResetStaticPreviewComponents();
	void RefreshStencilState();

	bool bIsHovered = false;
	bool bIsSelected = false;
};
