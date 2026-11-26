#include "SGridItemWorkshopViewport.h"

#include "AdvancedPreviewScene.h"
#include "Components/StaticMeshComponent.h"
#include "EditorViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Widgets/SNullWidget.h"
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
		ToggleOrbitCamera(false);
		OverrideNearClipPlane(static_cast<float>(FGridItemWorkshopOrbit::NearClipCm));
	}

	virtual FLinearColor GetBackgroundColor() const override { return FLinearColor::Black; }

	void SetInspectionFov(double Degrees)
	{
		ViewFOV = static_cast<float>(Degrees);
	}

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
	if (AGridItemPreviewActor* Actor = Preview.Get())
	{
		Actor->Destroy();
	}
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
	if (Result == EGridItemPreviewResult::Ready)
	{
		Mesh->UpdateBounds();
		Orbit.SetBounds(Mesh->Bounds, bRecenter || PreviousMesh != Mesh->GetStaticMesh());
		ApplyOrbit();
	}
	else
	{
		Orbit.Clear();
		if (WorkshopClient.IsValid())
		{
			WorkshopClient->Invalidate();
		}
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
	Orbit.SetBounds(Mesh->Bounds, true, Rotation);
	ApplyOrbit();
}

UStaticMeshComponent* SGridItemWorkshopViewport::GetMesh() const
{
	const AGridItemPreviewActor* Actor = Preview.Get();
	return Actor ? Actor->GetPreviewMesh() : nullptr;
}

void SGridItemWorkshopViewport::ApplyOrbit()
{
	if (!WorkshopClient.IsValid() || !Orbit.bReady)
	{
		return;
	}
	WorkshopClient->SetViewLocation(Orbit.CameraLocation());
	WorkshopClient->SetViewRotation(Orbit.ViewRotation());
	WorkshopClient->SetLookAtLocation(Orbit.AimLocation(), false);
	WorkshopClient->SetInspectionFov(Orbit.ViewFov());
	WorkshopClient->Invalidate();
}

void SGridItemWorkshopViewport::OrbitDrag(const FVector2D& Delta)
{
	Orbit.Drag(Delta);
	ApplyOrbit();
}

void SGridItemWorkshopViewport::OrbitZoom(double WheelDelta)
{
	Orbit.Zoom(WheelDelta);
	ApplyOrbit();
}

void SGridItemWorkshopViewport::OrbitPan(const FVector2D& Delta, double ProjectionAxisPixels)
{
	Orbit.Pan(Delta, ProjectionAxisPixels);
	ApplyOrbit();
}

void SGridItemWorkshopOrbitInput::Construct(const FArguments& InArgs)
{
	PreviewViewport = InArgs._PreviewViewport;
	ChildSlot[SNullWidget::NullWidget];
}

FReply SGridItemWorkshopOrbitInput::OnMouseButtonDown(const FGeometry&, const FPointerEvent& Event)
{
	const TSharedPtr<SGridItemWorkshopViewport> Preview = PreviewViewport.Pin();
	const FKey Button = Event.GetEffectingButton();
	if (!HasMouseCapture() && Preview.IsValid() && Preview->HasOrbitTarget() &&
		(Button == EKeys::LeftMouseButton || Button == EKeys::RightMouseButton || Button == EKeys::MiddleMouseButton))
	{
		ActiveDragButton = Button;
		return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse)
			.CaptureMouse(SharedThis(this)).UseHighPrecisionMouseMovement(SharedThis(this));
	}
	return FReply::Handled();
}

FReply SGridItemWorkshopOrbitInput::OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == ActiveDragButton && HasMouseCapture())
	{
		ActiveDragButton = FKey();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Handled();
}

FReply SGridItemWorkshopOrbitInput::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (HasMouseCapture())
	{
		if (!ActiveDragButton.IsValid() || !Event.IsMouseButtonDown(ActiveDragButton))
		{
			ActiveDragButton = FKey();
			return FReply::Handled().ReleaseMouseCapture();
		}
		if (const TSharedPtr<SGridItemWorkshopViewport> Preview = PreviewViewport.Pin())
		{
			if (ActiveDragButton == EKeys::LeftMouseButton)
			{
				Preview->OrbitDrag(Event.GetCursorDelta());
			}
			else
			{
				const FVector2D Size = Geometry.GetAbsoluteSize();
				Preview->OrbitPan(Event.GetCursorDelta(), FMath::Max(Size.X, Size.Y));
			}
		}
	}
	return FReply::Handled();
}

void SGridItemWorkshopOrbitInput::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	ActiveDragButton = FKey();
	SCompoundWidget::OnMouseCaptureLost(CaptureLostEvent);
}

FReply SGridItemWorkshopOrbitInput::OnMouseWheel(const FGeometry&, const FPointerEvent& Event)
{
	if (const TSharedPtr<SGridItemWorkshopViewport> Preview = PreviewViewport.Pin())
	{
		Preview->OrbitZoom(Event.GetWheelDelta());
	}
	return FReply::Handled();
}

FReply SGridItemWorkshopOrbitInput::OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
	return OnMouseButtonDown(Geometry, Event);
}

FReply SGridItemWorkshopOrbitInput::OnKeyDown(const FGeometry&, const FKeyEvent& Event)
{
	// Let the editor keep its normal Tab navigation; do not forward camera keys.
	if (Event.GetKey() == EKeys::Tab)
	{
		return FReply::Unhandled();
	}
	if (Event.GetKey() == EKeys::Escape && HasMouseCapture())
	{
		ActiveDragButton = FKey();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Handled();
}
