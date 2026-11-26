#include "SGridItemWorkshopViewport.h"

#include "AdvancedPreviewScene.h"
#include "Components/StaticMeshComponent.h"
#include "EditorViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Inspection/GridItemPreviewActor.h"

namespace
{
	class FGridWorkshopBlackScene : public FAdvancedPreviewScene
	{
	public:
		FGridWorkshopBlackScene()
			: FAdvancedPreviewScene(FPreviewScene::ConstructionValues(), 0.0f)
		{
			SetFloorVisibility(false, true);
			SetEnvironmentVisibility(false, true);
		}
		virtual FLinearColor GetBackgroundColor() const override { return FLinearColor::Black; }
	};
}

// The scene owner is constructed before, and destroyed after, the viewport
// client base. The base must never outlive the preview world it references.
class FGridWorkshopSceneOwner
{
protected:
	explicit FGridWorkshopSceneOwner(TSharedRef<FAdvancedPreviewScene> InScene)
		: SceneOwner(MoveTemp(InScene))
	{
	}

	TSharedRef<FAdvancedPreviewScene> SceneOwner;
};

class FGridItemWorkshopViewportClient : private FGridWorkshopSceneOwner, public FEditorViewportClient
{
public:
	FGridItemWorkshopViewportClient(TSharedRef<FAdvancedPreviewScene> InScene, const TSharedRef<SEditorViewport>& InViewport)
		: FGridWorkshopSceneOwner(InScene)
		, FEditorViewportClient(nullptr, &InScene.Get(), InViewport)
	{
		SetViewportType(LVT_Perspective);
		SetViewMode(VMI_Lit);
		DrawHelper.bDrawGrid = false;
		EngineShowFlags.SetGrid(false);
		EngineShowFlags.SetSelectionOutline(false);
		ExposureSettings.bFixed = true;
		ExposureSettings.FixedEV100 = 0;
		SetRealtime(true);
		SetViewRotation(FRotator(-12.0, 35.0, 0.0));
		ToggleOrbitCamera(true);
	}

	virtual FLinearColor GetBackgroundColor() const override { return FLinearColor::Black; }

	virtual void Tick(float DeltaSeconds) override
	{
		FEditorViewportClient::Tick(DeltaSeconds);
		if (!GIntraFrameDebuggingGameThread && IsRealtime())
		{
			if (UWorld* World = SceneOwner->GetWorld())
			{
				World->Tick(LEVELTICK_All, DeltaSeconds);
			}
		}
	}
};

void SGridItemWorkshopViewport::Construct(const FArguments&)
{
	Scene = MakeShared<FGridWorkshopBlackScene>();
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

SGridItemWorkshopViewport::~SGridItemWorkshopViewport()
{
	Clear();
	if (AGridItemPreviewActor* Actor = Preview.Get())
	{
		Actor->Destroy();
	}
	Preview.Reset();
	if (WorkshopClient.IsValid())
	{
		WorkshopClient->Viewport = nullptr;
	}
}

TSharedRef<FEditorViewportClient> SGridItemWorkshopViewport::MakeEditorViewportClient()
{
	WorkshopClient = MakeShared<FGridItemWorkshopViewportClient>(Scene.ToSharedRef(), SharedThis(this));
	return WorkshopClient.ToSharedRef();
}

EGridItemPreviewResult SGridItemWorkshopViewport::Display(const FGridItemInspectionView& View, bool bRecenter)
{
	if (!Preview.IsValid())
	{
		UWorld* World = Scene.IsValid() ? Scene->GetWorld() : nullptr;
		if (!World)
		{
			return EGridItemPreviewResult::InvalidHost;
		}
		FActorSpawnParameters Params;
		Params.ObjectFlags = RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Preview = World->SpawnActor<AGridItemPreviewActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}
	AGridItemPreviewActor* Actor = Preview.Get();
	if (!Actor)
	{
		return EGridItemPreviewResult::InvalidHost;
	}
	UStaticMeshComponent* Mesh = Actor->GetPreviewMesh();
	const UStaticMesh* PreviousMesh = Mesh->GetStaticMesh();
	const EGridItemPreviewResult Result = Actor->ApplyPreview(View);
	if (Result == EGridItemPreviewResult::Ready && (bRecenter || PreviousMesh != Mesh->GetStaticMesh()))
	{
		Frame();
	}
	if (WorkshopClient.IsValid())
	{
		WorkshopClient->Invalidate();
	}
	return Result;
}

void SGridItemWorkshopViewport::Frame(const FRotator& Rotation)
{
	UStaticMeshComponent* Mesh = GetMesh();
	if (!Mesh || !Mesh->GetStaticMesh() || !WorkshopClient.IsValid())
	{
		return;
	}
	Mesh->UpdateBounds();
	const FBoxSphereBounds Bounds = Mesh->Bounds;
	const double Radius = FMath::Max(1.0, Bounds.SphereRadius);
	// Keep the camera outside the mesh even before the first Slate layout pass.
	WorkshopClient->ToggleOrbitCamera(false);
	WorkshopClient->SetViewRotation(Rotation);
	WorkshopClient->SetViewLocation(Bounds.Origin - Rotation.Vector() * Radius * 3.5);
	WorkshopClient->SetLookAtLocation(Bounds.Origin, false);
	WorkshopClient->ToggleOrbitCamera(true);
	WorkshopClient->Invalidate();
}

void SGridItemWorkshopViewport::Clear()
{
	if (AGridItemPreviewActor* Actor = Preview.Get())
	{
		Actor->ClearPreview();
	}
	if (WorkshopClient.IsValid())
	{
		WorkshopClient->Invalidate();
	}
}

UStaticMeshComponent* SGridItemWorkshopViewport::GetMesh() const
{
	const AGridItemPreviewActor* Actor = Preview.Get();
	return Actor ? Actor->GetPreviewMesh() : nullptr;
}
