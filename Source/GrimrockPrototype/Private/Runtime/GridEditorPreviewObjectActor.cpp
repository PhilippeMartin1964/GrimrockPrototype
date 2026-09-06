#include "Runtime/GridEditorPreviewObjectActor.h"

#include "Animation/AnimInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridObjectArchetypeAsset.h"
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

	MovingPart0MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MovingPart0"));
	MovingPart0MeshComponent->SetupAttachment(SceneRoot);
	MovingPart0MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MovingPart0MeshComponent->SetGenerateOverlapEvents(false);
	MovingPart0MeshComponent->SetMobility(EComponentMobility::Movable);
	MovingPart0MeshComponent->SetVisibility(false, true);

	MovingPart1MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MovingPart1"));
	MovingPart1MeshComponent->SetupAttachment(SceneRoot);
	MovingPart1MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MovingPart1MeshComponent->SetGenerateOverlapEvents(false);
	MovingPart1MeshComponent->SetMobility(EComponentMobility::Movable);
	MovingPart1MeshComponent->SetVisibility(false, true);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMeshComponent->SetupAttachment(SceneRoot);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetGenerateOverlapEvents(false);
	SkeletalMeshComponent->SetMobility(EComponentMobility::Movable);
	SkeletalMeshComponent->SetVisibility(false, true);
}

void AGridEditorPreviewObjectActor::ResetStaticPreviewComponents()
{
	for (UStaticMeshComponent* Component : {MeshComponent, MovingPart0MeshComponent, MovingPart1MeshComponent})
	{
		if (!Component)
		{
			continue;
		}
		Component->SetStaticMesh(nullptr);
		Component->SetRelativeTransform(FTransform::Identity);
		Component->SetVisibility(false, true);
		Component->SetRenderCustomDepth(false);
		Component->SetCustomDepthStencilValue(0);
		Component->MarkRenderStateDirty();
	}
}

void AGridEditorPreviewObjectActor::InitializePreviewObject(const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh)
{
	ObjectId = ObjectData.ObjectId;
	ObjectType = ObjectData.Type;

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

void AGridEditorPreviewObjectActor::InitializePreviewObjectFromArchetype(
	const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype)
{
	if (!Archetype || !Archetype->HasAnyVisualPart())
	{
		InitializePreviewObject(ObjectData, nullptr);
		return;
	}

	ObjectId = ObjectData.ObjectId;
	ObjectType = ObjectData.Type;
	ResetStaticPreviewComponents();

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetSkeletalMesh(nullptr);
		SkeletalMeshComponent->SetAnimInstanceClass(nullptr);
		SkeletalMeshComponent->SetVisibility(false, true);
	}

	const auto ConfigurePart = [](UStaticMeshComponent* Component, UStaticMesh* Mesh, const FTransform& LocalTransform)
	{
		if (!Component)
		{
			return;
		}
		Component->SetStaticMesh(Mesh);
		Component->SetRelativeTransform(LocalTransform);
		Component->SetVisibility(Mesh != nullptr, true);
	};

	ConfigurePart(MeshComponent, Archetype->StaticPart.Mesh.Get(), Archetype->StaticPart.LocalTransform);
	ConfigurePart(MovingPart0MeshComponent, Archetype->MovingParts.Part0.Mesh.Get(), Archetype->MovingParts.Part0.LocalTransform);
	ConfigurePart(MovingPart1MeshComponent, Archetype->MovingParts.Part1.Mesh.Get(), Archetype->MovingParts.Part1.LocalTransform);

	bIsHovered = false;
	bIsSelected = false;
	RefreshStencilState();
}

void AGridEditorPreviewObjectActor::InitializeMonsterPreviewObject(
	const FGridLevelObjectData& ObjectData, UGridMonsterDefinitionAsset* MonsterDefinition)
{
	ObjectId = ObjectData.ObjectId;
	ObjectType = ObjectData.Type;

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
	ApplyStencil(MovingPart0MeshComponent);
	ApplyStencil(MovingPart1MeshComponent);
	ApplyStencil(SkeletalMeshComponent);
}
