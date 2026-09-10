#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridTypes.h"
#include "GridEditorPreviewObjectActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UGridMonsterDefinitionAsset;
class UGridWorldObjectDefinitionAsset;
struct FGridMonsterSpawnInstance;
struct FGridWorldObjectInstanceConfig;

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

	/** Initializes a standalone single-mesh preview from native typed placement identity. */
	void InitializePreviewObject(FGuid InObjectId, EGridLevelObjectType InObjectType, UStaticMesh* Mesh);

	/** Renders StaticPart + resolved MovingPart[0..1] using the same sparse instance overrides as runtime. */
	void InitializePreviewObjectFromDefinition(
		FGuid InObjectId,
		EGridLevelObjectType InObjectType,
		const UGridWorldObjectDefinitionAsset* Definition,
		const FGridWorldObjectInstanceConfig* InstanceConfig = nullptr);

	/** Typed monster preview entry point. */
	void InitializeMonsterPreviewObject(const FGridMonsterSpawnInstance& SpawnData, UGridMonsterDefinitionAsset* MonsterDefinition);

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
