#include "SGridItemWorkshopViewport.h"

#include "AdvancedPreviewScene.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "EditorViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Widgets/SNullWidget.h"
#include "Runtime/Inspection/GridItemPreviewActor.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

namespace
{
	class FGridWorkshopBlackScene : public FAdvancedPreviewScene
	{
	public:
		FGridWorkshopBlackScene()
			: FAdvancedPreviewScene(FPreviewScene::ConstructionValues(), 0.0f)
		{
			// At construction only the engine's stock scene geometry exists.
			// Keep explicit identities; later stage meshes must never be hidden.
			for (UActorComponent* Component : Components)
			{
				if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
				{
					StockBackdropMeshes.Add(Mesh);
				}
			}
			EnforceBlackBackdrop();
			StudioIntensity = DirectionalLight->Intensity;
			StudioSkyIntensity = SkyLight->Intensity;
			StudioColor = DirectionalLight->GetLightColor();
		}
		virtual FLinearColor GetBackgroundColor() const override { return FLinearColor::Black; }

		void EnforceBlackBackdrop()
		{
			// Asset Viewer refreshes may restore the shared profile's panorama/floor.
			// Direct visibility only: keep the cubemap lighting and the user's profile.
			SetFloorVisibility(false, true);
			SetEnvironmentVisibility(false, true);
			for (const TWeakObjectPtr<UStaticMeshComponent>& Entry : StockBackdropMeshes)
			{
				if (UStaticMeshComponent* Mesh = Entry.Get())
				{
					Mesh->SetVisibility(false);
					Mesh->SetHiddenInGame(true);
				}
			}
		}

#if WITH_DEV_AUTOMATION_TESTS
		const TArray<TWeakObjectPtr<UStaticMeshComponent>>& GetStockBackdropMeshesForTest() const
		{
			return StockBackdropMeshes;
		}
#endif

		void ApplyLighting(EGridWorkshopLighting Lighting)
		{
			// Fixed, identical authoring illumination. Never edit the shared
			// Asset Viewer profile or a production light.
			const bool bStudio = Lighting == EGridWorkshopLighting::Studio;
			const bool bTorch = Lighting == EGridWorkshopLighting::Torch;
			DirectionalLight->SetIntensity(StudioIntensity * (bStudio ? 1.0f : bTorch ? 0.65f : 0.18f));
			DirectionalLight->SetLightColor(bStudio ? StudioColor : bTorch ? FLinearColor(1.0f, 0.55f, 0.22f) : FLinearColor(0.65f, 0.76f, 1.0f), false);
			SkyLight->SetIntensity(StudioSkyIntensity * (bStudio ? 1.0f : bTorch ? 0.18f : 0.12f));
		}

	private:
		TArray<TWeakObjectPtr<UStaticMeshComponent>> StockBackdropMeshes;
		float StudioIntensity = 0.0f;
		float StudioSkyIntensity = 0.0f;
		FLinearColor StudioColor = FLinearColor::White;
	};

	// Use the existing native-scale dungeon meshes; no packages are authored.
	UStaticMeshComponent* ShowStageMesh(FAdvancedPreviewScene& Scene, TWeakObjectPtr<UStaticMeshComponent>& Component, const TCHAR* AssetPath, bool bShow)
	{
		if (!bShow)
		{
			if (UStaticMeshComponent* Existing = Component.Get())
			{
				Scene.RemoveComponent(Existing);
				Component.Reset();
			}
			return nullptr;
		}
		if (!Component.IsValid())
		{
			UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, AssetPath);
			if (!Asset)
			{
				return nullptr;
			}
			UStaticMeshComponent* NewMesh = NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
			NewMesh->SetStaticMesh(Asset);
			NewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			NewMesh->SetGenerateOverlapEvents(false);
			NewMesh->SetCanEverAffectNavigation(false);
			NewMesh->SetMobility(EComponentMobility::Movable);
			Scene.AddComponent(NewMesh, FTransform::Identity, false);
			Component = NewMesh;
		}
		return Component.Get();
	}
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

	virtual void Draw(FViewport* InViewport, FCanvas* Canvas) override
	{
		// Reassert the local backdrop after any profile refresh, before rendering.
		static_cast<FGridWorkshopBlackScene&>(*SceneOwner).EnforceBlackBackdrop();
		FEditorViewportClient::Draw(InViewport, Canvas);
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

EGridItemPreviewResult SGridItemWorkshopViewport::Display(const FGridItemInspectionView& View, bool bRecenter, EGridWorkshopContext Context)
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
	const EGridItemPreviewResult Result = Actor->ApplyPreview(FGridWorkshopContext::Project(View, Context));
	if (Result == EGridItemPreviewResult::Ready)
	{
		const FBoxSphereBounds ItemBounds = Actor->GetPresentationBounds();
		UpdateStage(Context, &ItemBounds);
		Orbit.SetBounds(ItemBounds, bRecenter || PreviousMesh != Mesh->GetStaticMesh());
		ApplyOrbit();
	}
	else
	{
		Orbit.Clear();
		UpdateStage(Context, nullptr);
		if (WorkshopClient.IsValid())
		{
			WorkshopClient->Invalidate();
		}
	}
	return Result;
}

void SGridItemWorkshopViewport::SetLighting(EGridWorkshopLighting Lighting)
{
	static_cast<FGridWorkshopBlackScene&>(*Scene).ApplyLighting(Lighting);
	WorkshopClient->Invalidate();
}

void SGridItemWorkshopViewport::UpdateStage(EGridWorkshopContext Context, const FBoxSphereBounds* ItemBounds)
{
	bStageError = false;
	const bool bFloor = ItemBounds && FGridWorkshopContext::HasFloor(Context);
	const bool bWall = ItemBounds && Context == EGridWorkshopContext::Decor;
	UStaticMeshComponent* Floor = ShowStageMesh(*Scene, StageFloor,
		TEXT("/Game/GrimrockPrototype/Meshes/Floor/SM_Floor_Stone_01.SM_Floor_Stone_01"), bFloor);
	UStaticMeshComponent* Wall = ShowStageMesh(*Scene, StageWall,
		TEXT("/Game/GrimrockPrototype/Meshes/Wall/SM_Wall_Stone_05.SM_Wall_Stone_05"), bWall);
	bStageError = (bFloor && !Floor) || (bWall && !Wall);
	if (!ItemBounds || !bFloor)
	{
		return;
	}
	const FVector Center = ItemBounds->Origin;
	const FVector Extent = ItemBounds->BoxExtent;
	const double GroundZ = Center.Z - Extent.Z;
	if (Floor)
	{
		const FBoxSphereBounds Bounds = Floor->GetStaticMesh()->GetBounds();
		Floor->SetWorldLocation(FVector(Center.X - Bounds.Origin.X, Center.Y - Bounds.Origin.Y,
			GroundZ - Bounds.Origin.Z - Bounds.BoxExtent.Z));
	}
	if (Wall)
	{
		// Technical backdrop behind the item; keep actual scale and turn
		// only to align the thinner horizontal axis with world X.
		const FBoxSphereBounds Bounds = Wall->GetStaticMesh()->GetBounds();
		const FRotator Rotation(0.0, Bounds.BoxExtent.X <= Bounds.BoxExtent.Y ? 0.0 : 90.0, 0.0);
		Wall->SetWorldRotation(Rotation);
		Wall->SetWorldLocation(FVector::ZeroVector);
		Wall->UpdateBounds();
		const FBoxSphereBounds WorldBounds = Wall->Bounds;
		Wall->SetWorldLocation(FVector(Center.X + Extent.X + 100.0 + WorldBounds.BoxExtent.X,
			Center.Y, GroundZ + WorldBounds.BoxExtent.Z) - WorldBounds.Origin);
	}
}

void SGridItemWorkshopViewport::Frame(const FRotator& Rotation)
{
	UStaticMeshComponent* Mesh = GetMesh();
	if (!Mesh || !Mesh->GetStaticMesh() || !WorkshopClient.IsValid())
	{
		return;
	}
	Orbit.SetBounds(Preview->GetPresentationBounds(), true, Rotation);
	ApplyOrbit(true);
}

UStaticMeshComponent* SGridItemWorkshopViewport::GetMesh() const
{
	const AGridItemPreviewActor* Actor = Preview.Get();
	return Actor ? Actor->GetPreviewMesh() : nullptr;
}

void SGridItemWorkshopViewport::SetLinkedViewport(const TSharedPtr<SGridItemWorkshopViewport>& Other)
{
	LinkedViewport = Other;
}

void SGridItemWorkshopViewport::MatchFraming(const SGridItemWorkshopViewport& Other)
{
	if (Orbit.bReady && Other.Orbit.bReady)
	{
		Orbit.MatchFraming(Other.Orbit);
		ApplyOrbit();
	}
}

void SGridItemWorkshopViewport::ApplyOrbit(bool bPropagate)
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
	if (bPropagate)
	{
		if (const TSharedPtr<SGridItemWorkshopViewport> Other = LinkedViewport.Pin())
		{
			Other->MatchFraming(*this);
		}
	}
}

void SGridItemWorkshopViewport::OrbitDrag(const FVector2D& Delta)
{
	Orbit.Drag(Delta);
	ApplyOrbit(true);
}

void SGridItemWorkshopViewport::OrbitZoom(double WheelDelta)
{
	Orbit.Zoom(WheelDelta);
	ApplyOrbit(true);
}

void SGridItemWorkshopViewport::OrbitPan(const FVector2D& Delta, double ProjectionAxisPixels)
{
	Orbit.Pan(Delta, ProjectionAxisPixels);
	ApplyOrbit(true);
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

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridWorkshopBlackBackdropTest, "Grimrock.Editor.Items.Inspection.BlackBackdrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorkshopBlackBackdropTest::RunTest(const FString&)
{
	FGridWorkshopBlackScene Scene;
	const auto& Stock = Scene.GetStockBackdropMeshesForTest();
	if (!TestTrue(TEXT("Stock scene components explicitly captured"), Stock.Num() > 0))
	{
		return false;
	}
	// Test the scene's own components, not a global UObject/world search.
	// Directly set the raw visibility flag; the test does not require a GPU
	// or a profile capable of showing both engine meshes under NullRHI.
	for (const TWeakObjectPtr<UStaticMeshComponent>& Entry : Stock)
	{
		UStaticMeshComponent* Mesh = Entry.Get();
		if (!TestTrue(TEXT("Stock component exists"), Mesh != nullptr))
		{
			return false;
		}
		TestFalse(TEXT("Initially hidden"), Mesh->GetVisibleFlag());
		Mesh->SetVisibility(true);
		Mesh->SetHiddenInGame(false);
		TestTrue(TEXT("Visibility flag deliberately restored"), Mesh->GetVisibleFlag());
	}

	// A later authored stage component must remain visible.
	UStaticMeshComponent* Stage = NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	Stage->SetVisibility(true);
	Scene.AddComponent(Stage, FTransform::Identity, false);
	const float SkyIntensity = Scene.SkyLight->Intensity;
	const float DirectionalIntensity = Scene.DirectionalLight->Intensity;
	Scene.EnforceBlackBackdrop();
	for (const TWeakObjectPtr<UStaticMeshComponent>& Entry : Stock)
	{
		TestFalse(TEXT("Stock geometry hidden again"), Entry->GetVisibleFlag());
	}
	TestTrue(TEXT("Authored stage visibility preserved"), Stage->GetVisibleFlag());
	TestTrue(TEXT("Clear color stays black"), Scene.GetBackgroundColor().Equals(FLinearColor::Black));
	TestEqual(TEXT("Sky illumination unchanged"), Scene.SkyLight->Intensity, SkyIntensity);
	TestEqual(TEXT("Directional illumination unchanged"), Scene.DirectionalLight->Intensity, DirectionalIntensity);
	Scene.RemoveComponent(Stage);
	return !HasAnyErrors();
}
#endif
