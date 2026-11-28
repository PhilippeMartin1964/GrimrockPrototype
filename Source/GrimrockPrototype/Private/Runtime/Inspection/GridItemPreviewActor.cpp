#include "Runtime/Inspection/GridItemPreviewActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureCube.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/Inspection/GridItemMeshPresentation.h"

AGridItemPreviewActor::AGridItemPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);

	// Native runtime asset reference, not an editor-only preview profile.
	// Retain on the class for cooking; never change the item's materials.
	static ConstructorHelpers::FObjectFinder<UTextureCube> Studio(
		TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap"));
	StudioCubemap = Studio.Object;

	USceneComponent* ObservationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ObservationRoot"));
	SetRootComponent(ObservationRoot);
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	MeshComponent->SetupAttachment(ObservationRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCanEverAffectNavigation(false);
	MeshComponent->SetSimulatePhysics(false);
	MeshComponent->SetEnableGravity(false);

	OrnamentComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewOrnament"));
	OrnamentComponent->SetupAttachment(MeshComponent);
	OrnamentComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OrnamentComponent->SetGenerateOverlapEvents(false);
	OrnamentComponent->SetCanEverAffectNavigation(false);
	OrnamentComponent->SetSimulatePhysics(false);
	OrnamentComponent->SetEnableGravity(false);

	EffectsComponent = CreateDefaultSubobject<UGridItemVisualEffectsComponent>(TEXT("PreviewEffects"));
	EffectsComponent->SetupAttachment(MeshComponent);
}

EGridItemPreviewResult AGridItemPreviewActor::ApplyPreview(const FGridItemInspectionView& View)
{
	ClearPreview();
	const UWorld* World = GetWorld();
	if (!World || (World->WorldType != EWorldType::EditorPreview && World->WorldType != EWorldType::GamePreview))
	{
		return EGridItemPreviewResult::InvalidHost;
	}
	if (!View.IsValid())
	{
		return EGridItemPreviewResult::InvalidSelection;
	}
	UStaticMesh* Mesh = View.Item.WorldMesh.LoadSynchronous();
	if (!Mesh)
	{
		return EGridItemPreviewResult::MissingMesh;
	}
	UStaticMesh* Ornament = nullptr;
	if (!View.Item.OrnamentMesh.IsNull())
	{
		Ornament = View.Item.OrnamentMesh.LoadSynchronous();
		if (!Ornament)
		{
			return EGridItemPreviewResult::MissingOrnament;
		}
	}
	FGridItemMeshPresentation::SetMesh(MeshComponent, Mesh, View.Item.WeaponFinish, FinishMaterials);
	// Keep the authored scale/offset relative to the independent observation root.
	MeshComponent->SetRelativeTransform(View.Item.WorldVisualTransform);
	if (Ornament)
	{
		OrnamentComponent->SetStaticMesh(Ornament);
		OrnamentComponent->SetRelativeTransform(View.Item.OrnamentTransform);
	}
	EffectsComponent->SetItemLightState(View.bPhysicalInstance && View.bItemLightsEnabled);
	EffectsComponent->ApplyConfig(View.Item.VisualEffects);
	return EGridItemPreviewResult::Ready;
}

FBoxSphereBounds AGridItemPreviewActor::GetPresentationBounds() const
{
	MeshComponent->UpdateBounds();
	if (OrnamentComponent->GetStaticMesh())
	{
		OrnamentComponent->UpdateBounds();
		return MeshComponent->Bounds + OrnamentComponent->Bounds;
	}
	return MeshComponent->Bounds;
}

void AGridItemPreviewActor::ClearPreview()
{
	if (IsValid(OrnamentComponent.Get()))
	{
		OrnamentComponent->SetStaticMesh(nullptr);
		OrnamentComponent->SetRelativeTransform(FTransform::Identity);
	}
	if (IsValid(EffectsComponent.Get()))
	{
		EffectsComponent->ApplyConfig(FGridItemVisualEffectsConfig());
		EffectsComponent->SetItemLightState(false);
	}
	if (IsValid(MeshComponent.Get()))
	{
		FGridItemMeshPresentation::SetMesh(MeshComponent, nullptr, FGridWeaponFinish(), FinishMaterials);
		MeshComponent->SetRelativeTransform(FTransform::Identity);
	}
	else
	{
		FinishMaterials.Reset();
	}
}

void AGridItemPreviewActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPreview();
	Super::EndPlay(EndPlayReason);
}

void AGridItemPreviewActor::Destroyed()
{
	ClearPreview();
	Super::Destroyed();
}
