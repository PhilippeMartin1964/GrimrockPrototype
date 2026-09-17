#include "Runtime/GridEditorPreviewObjectActor.h"

#include "Animation/AnimInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridWorldObjectInstanceVisual.h"
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

void AGridEditorPreviewObjectActor::ResetStaticPreviewComponents()
{
	if (MeshComponent)
	{
		MeshComponent->SetStaticMesh(nullptr);
		MeshComponent->SetRelativeTransform(FTransform::Identity);
		MeshComponent->SetVisibility(false, true);
		MeshComponent->SetRenderCustomDepth(false);
		MeshComponent->SetCustomDepthStencilValue(0);
		MeshComponent->MarkRenderStateDirty();
	}
	for (UStaticMeshComponent* Component : MovingPartMeshComponents)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	MovingPartMeshComponents.Reset();
}

void AGridEditorPreviewObjectActor::InitializePreviewObject(FGuid InObjectId, EGridLevelObjectType InObjectType, UStaticMesh* Mesh)
{
	ObjectId = InObjectId;
	ObjectType = InObjectType;

	ResetStaticPreviewComponents();

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetSkeletalMesh(nullptr);
		SkeletalMeshComponent->SetAnimInstanceClass(nullptr);
		SkeletalMeshComponent->SetVisibility(false, true);
	}

	if (MeshComponent)
	{
		MeshComponent->SetStaticMesh(Mesh);
		MeshComponent->SetVisibility(Mesh != nullptr, true);
	}

	bIsHovered = false;
	bIsSelected = false;
	RefreshStencilState();
}

void AGridEditorPreviewObjectActor::InitializePreviewObjectFromDefinition(
	FGuid InObjectId,
	EGridLevelObjectType InObjectType,
	const UGridWorldObjectDefinitionAsset* Definition,
	const FGridWorldObjectInstanceConfig* InstanceConfig)
{
	if (!Definition || !Definition->HasAnyVisualPart())
	{
		InitializePreviewObject(InObjectId, InObjectType, nullptr);
		return;
	}

	ObjectId = InObjectId;
	ObjectType = InObjectType;
	ResetStaticPreviewComponents();

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetSkeletalMesh(nullptr);
		SkeletalMeshComponent->SetAnimInstanceClass(nullptr);
		SkeletalMeshComponent->SetVisibility(false, true);
	}

	const TArray<FGridWorldObjectMovingPartInstanceOverride> EmptyOverrides;
	const TArray<FGridWorldObjectMovingPartInstanceOverride>& Overrides =
		InstanceConfig ? InstanceConfig->MovingPartOverrides : EmptyOverrides;
	MeshComponent->SetStaticMesh(Definition->StaticPart.Mesh.Get());
	MeshComponent->SetRelativeTransform(Definition->StaticPart.LocalTransform);
	MeshComponent->SetVisibility(Definition->StaticPart.Mesh != nullptr, true);
	for (int32 PartIndex = 0; PartIndex < Definition->MovingParts.Num(); ++PartIndex)
	{
		const FGridWorldObjectMovingPart ResolvedPart = GridWorldObjectInstanceVisual::ResolveMovingPart(
			Definition->MovingParts[PartIndex], Overrides, PartIndex);
		UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("MovingPart%d"), PartIndex));
		Component->SetupAttachment(SceneRoot);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetStaticMesh(ResolvedPart.Mesh.Get());
		Component->SetRelativeTransform(ResolvedPart.LocalTransform);
		Component->SetVisibility(ResolvedPart.Mesh != nullptr, true);
		AddInstanceComponent(Component);
		Component->RegisterComponent();
		MovingPartMeshComponents.Add(Component);
	}

	bIsHovered = false;
	bIsSelected = false;
	RefreshStencilState();
}

void AGridEditorPreviewObjectActor::InitializeMonsterPreviewObject(
	const FGridMonsterSpawnInstance& SpawnData, UGridMonsterDefinitionAsset* MonsterDefinition)
{
	ObjectId = SpawnData.SpawnId;
	ObjectType = EGridLevelObjectType::MonsterSpawn;

	ResetStaticPreviewComponents();

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
	for (UStaticMeshComponent* Component : MovingPartMeshComponents)
	{
		ApplyStencil(Component);
	}
	ApplyStencil(SkeletalMeshComponent);
}
