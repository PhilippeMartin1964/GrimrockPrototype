#include "Runtime/GridPitTrapdoorActor.h"

#include "Components/StaticMeshComponent.h"

AGridPitTrapdoorActor::AGridPitTrapdoorActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGridPitTrapdoorActor::InitializeMechanismVisuals(
	const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype, const FTransform& WorldTransform)
{
	ObjectId = ObjectData.ObjectId;
	ObjectType = ObjectData.Type;
	CellX = ObjectData.CellX;
	CellY = ObjectData.CellY;
	Edge = ObjectData.Edge;
	SetActorTransform(WorldTransform);

	if (!Archetype)
	{
		return;
	}

	// The open pit geometry is permanent. PreviewMesh remains the editor fallback.
	SetFixedMesh(Archetype->FixedMesh ? Archetype->FixedMesh.Get() : Archetype->PreviewMesh.Get(),
		Archetype->FixedMaterial ? Archetype->FixedMaterial.Get() : Archetype->PreviewMaterial.Get());

	// PIT03 does not invent a lid mesh. Authoring may assign MovingMesh later.
	SetMovingMesh(Archetype->MovingMesh.Get(), Archetype->MovingMaterial.Get());

	if (FixedMeshComponent)
	{
		FixedMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AGridPitTrapdoorActor::InitializeGridObject(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform)
{
	(void)Mesh;
	(void)Material;
	// Avoid duplicating the open-pit PreviewMesh on the inherited generic MeshComponent.
	AGridRuntimeObjectActor::InitializeGridObject(ObjectData, nullptr, nullptr, WorldTransform);
}

void AGridPitTrapdoorActor::SetPitOpenVisualState(bool bOpen, bool bPlayAudio)
{
	const bool bChanged = bIsOpen != bOpen;
	bIsOpen = bOpen;

	if (MovingMeshComponent)
	{
		const bool bHasCoverMesh = MovingMeshComponent->GetStaticMesh() != nullptr;
		MovingMeshComponent->SetVisibility(!bIsOpen && bHasCoverMesh, true);
		MovingMeshComponent->SetCollisionEnabled(!bIsOpen && bHasCoverMesh ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}

	if (bChanged && bPlayAudio)
	{
		PlayObjectAudioEvent(bIsOpen ? FName(TEXT("Open")) : FName(TEXT("Close")));
	}
}
