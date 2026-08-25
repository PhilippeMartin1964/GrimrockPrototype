#include "Runtime/GridEditorPreviewObjectActor.h"

#include "Animation/AnimInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

AGridEditorPreviewObjectActor::AGridEditorPreviewObjectActor()
{
	PrimaryActorTick.bCanEverTick = false;
#if WITH_EDITORONLY_DATA
	bIsEditorOnlyActor = true;
#endif
	SetActorHiddenInGame(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetMobility(EComponentMobility::Movable);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMeshComponent->SetupAttachment(SceneRoot);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetGenerateOverlapEvents(false);
	SkeletalMeshComponent->SetMobility(EComponentMobility::Movable);
	SkeletalMeshComponent->SetVisibility(false, true);
}

void AGridEditorPreviewObjectActor::InitializePreviewObject(const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* NormalMaterial)
{
	ObjectId = ObjectData.ObjectId;
	ObjectType = ObjectData.Type;

	CachedNormalMaterial = NormalMaterial;

	if (!MeshComponent)
	{
		return;
	}

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetSkeletalMesh(nullptr);
		SkeletalMeshComponent->SetAnimInstanceClass(nullptr);
		SkeletalMeshComponent->SetVisibility(false, true);
	}

	MeshComponent->SetVisibility(true, true);
	MeshComponent->SetStaticMesh(Mesh);

	if (CachedNormalMaterial)
	{
		MeshComponent->SetMaterial(0, CachedNormalMaterial);
	}

	MeshComponent->SetRenderCustomDepth(false);
	MeshComponent->SetCustomDepthStencilValue(0);
	MeshComponent->MarkRenderStateDirty();

	bIsHovered = false;
	bIsSelected = false;
	RefreshStencilState();
}

void AGridEditorPreviewObjectActor::InitializeMonsterPreviewObject(const FGridLevelObjectData& ObjectData, UGridMonsterDefinitionAsset* MonsterDefinition)
{
	ObjectId = ObjectData.ObjectId;
	ObjectType = ObjectData.Type;
	CachedNormalMaterial = nullptr;

	if (MeshComponent)
	{
		MeshComponent->SetStaticMesh(nullptr);
		MeshComponent->SetVisibility(false, true);
	}

	if (!SkeletalMeshComponent || !IsValid(MonsterDefinition))
	{
		return;
	}

	SkeletalMeshComponent->SetSkeletalMesh(MonsterDefinition->SkeletalMesh.LoadSynchronous());
	SkeletalMeshComponent->SetRelativeLocation(MonsterDefinition->VisualOffset);
	SkeletalMeshComponent->SetRelativeRotation(MonsterDefinition->VisualRotationOffset);
	SkeletalMeshComponent->SetRelativeScale3D(MonsterDefinition->VisualScale);
	if (MonsterDefinition->AnimationClass)
	{
		SkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		SkeletalMeshComponent->SetAnimInstanceClass(MonsterDefinition->AnimationClass.Get());
	}
	else
	{
		SkeletalMeshComponent->SetAnimInstanceClass(nullptr);
	}
	SkeletalMeshComponent->SetVisibility(true, true);

	bIsHovered = false;
	bIsSelected = false;
	RefreshStencilState();
}

void AGridEditorPreviewObjectActor::SetHovered(bool bHovered)
{
	bIsHovered = bHovered;
	RefreshStencilState();
}

void AGridEditorPreviewObjectActor::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
	RefreshStencilState();
}

void AGridEditorPreviewObjectActor::RefreshStencilState()
{
	int32 StencilValue = 0;

	if (bIsSelected)
	{
		StencilValue = 2;
	}
	else if (bIsHovered)
	{
		StencilValue = 1;
	}

	const auto ApplyStencil = [StencilValue](UPrimitiveComponent* Component)
	{
		if (!Component)
		{
			return;
		}
		Component->SetRenderCustomDepth(StencilValue != 0);
		Component->SetCustomDepthStencilValue(StencilValue);
		Component->MarkRenderStateDirty();
	};

	ApplyStencil(MeshComponent);
	ApplyStencil(SkeletalMeshComponent);
}
