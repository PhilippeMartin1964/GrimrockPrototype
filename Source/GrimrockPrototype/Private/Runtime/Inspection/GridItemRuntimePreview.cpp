#include "Runtime/Inspection/GridItemRuntimePreview.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkyLightComponent.h"
#include "Engine/TextureCube.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "PreviewScene.h"
#include "Runtime/Inspection/GridItemPreviewActor.h"

FGridItemRuntimePreview::FGridItemRuntimePreview() = default;

FGridItemRuntimePreview::~FGridItemRuntimePreview()
{
	Close();
}

bool FGridItemRuntimePreview::Open(UWorld* PlayerWorld)
{
	Close();
	if (!IsValid(PlayerWorld) ||
		(PlayerWorld->WorldType != EWorldType::Game && PlayerWorld->WorldType != EWorldType::PIE))
	{
		return false;
	}

	// FPreviewScene is in Engine, not AdvancedPreviewScene/UnrealEd.
	// The isolated GamePreview scene has no gameplay map or audible output.
	Scene = MakeUnique<FPreviewScene>(FPreviewScene::ConstructionValues()
		.SetEditor(false).SetCreatePhysicsScene(false).ShouldSimulatePhysics(false)
		.AllowAudioPlayback(false).SetTransactional(false).SetForceMipsResident(false)
		.SetLightRotation(FRotator(-40.0, 35.0, 0.0)).SetLightBrightness(12.0f).SetSkyBrightness(1.0f));
	UWorld* PreviewWorld = Scene->GetWorld();
	if (!PreviewWorld || PreviewWorld == PlayerWorld || PreviewWorld->WorldType != EWorldType::GamePreview)
	{
		Close();
		return false;
	}
	OwnerWorld = PlayerWorld;

	FActorSpawnParameters Params;
	Params.ObjectFlags = RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Actor = PreviewWorld->SpawnActor<AGridItemPreviewActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!Actor.IsValid())
	{
		Close();
		return false;
	}

	// An empty black world cannot supply reflections for metallic materials.
	// Use a specified cubemap for illumination only, never a sky mesh.
	if (UTextureCube* Studio = Actor->GetStudioCubemap())
	{
		Scene->SkyLight->SourceType = SLS_SpecifiedCubemap;
		Scene->SkyLight->bLowerHemisphereIsBlack = false;
		Scene->SkyLight->SetCubemap(Studio);
		bStudioCaptureDirty = true;
	}
	Scene->DirectionalLight->SetMobility(EComponentMobility::Movable);
	// A second directional source improves legibility without a visible sky dome.
	UDirectionalLightComponent* Fill = NewObject<UDirectionalLightComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	Fill->SetMobility(EComponentMobility::Movable);
	Fill->SetIntensity(6.0f);
	Scene->AddComponent(Fill, FTransform(FRotator(-25.0, -135.0, 0.0)), false);
	FillLight = Fill;

	USceneCaptureComponent2D* CaptureComponent = NewObject<USceneCaptureComponent2D>(GetTransientPackage(), NAME_None, RF_Transient);
	CaptureComponent->SetMobility(EComponentMobility::Movable);
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->CaptureSource = SCS_FinalColorLDR;
	CaptureComponent->PostProcessBlendWeight = 1.0f;
	CaptureComponent->PostProcessSettings.bOverride_AutoExposureMethod = true;
	CaptureComponent->PostProcessSettings.AutoExposureMethod = AEM_Manual;
	CaptureComponent->PostProcessSettings.bOverride_AutoExposureBias = true;
	CaptureComponent->PostProcessSettings.AutoExposureBias = 1.0f;
	CaptureComponent->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	CaptureComponent->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
	Scene->AddComponent(CaptureComponent, FTransform::Identity, false);
	Camera = CaptureComponent;
	return true;
}

void FGridItemRuntimePreview::Close()
{
	Clear();
	if (AGridItemPreviewActor* PreviewActor = Actor.Get())
	{
		PreviewActor->Destroy();
	}
	Actor.Reset();
	if (USceneCaptureComponent2D* CaptureComponent = Camera.Get())
	{
		CaptureComponent->TextureTarget = nullptr;
		if (Scene)
		{
			Scene->RemoveComponent(CaptureComponent);
		}
	}
	Camera.Reset();
	FillLight.Reset();
	bStudioCaptureDirty = false;
	OwnerWorld.Reset();
	Scene.Reset();
}

bool FGridItemRuntimePreview::IsOpen() const
{
	return Scene && OwnerWorld.IsValid() && Actor.IsValid() && Camera.IsValid();
}

UWorld* FGridItemRuntimePreview::GetPreviewWorld() const
{
	return Scene ? Scene->GetWorld() : nullptr;
}

AGridItemPreviewActor* FGridItemRuntimePreview::GetPreviewActor() const
{
	return Actor.Get();
}

EGridItemPreviewResult FGridItemRuntimePreview::Present(const FGridItemInspectionView& View)
{
	Clear();
	if (!IsOpen())
	{
		return EGridItemPreviewResult::InvalidHost;
	}
	const EGridItemPreviewResult Result = Actor->ApplyPreview(View);
	if (Result == EGridItemPreviewResult::Ready)
	{
		Orbit.SetBounds(Actor->GetPresentationBounds(), true);
		UpdateCamera();
	}
	return Result;
}

void FGridItemRuntimePreview::Clear()
{
	bCaptureRequested = false;
	Orbit.Clear();
	if (AGridItemPreviewActor* PreviewActor = Actor.Get())
	{
		PreviewActor->ClearPreview();
	}
}

void FGridItemRuntimePreview::UpdateCamera()
{
	bCaptureRequested = false;
	if (IsOpen() && Orbit.bReady)
	{
		Camera->SetWorldLocationAndRotation(Orbit.CameraLocation(), Orbit.ViewRotation());
		Camera->FOVAngle = static_cast<float>(Orbit.ViewFov());
		// Follow the view, not the mesh: both broad faces remain lit while orbiting.
		const FQuat View = Orbit.Orientation;
		Scene->DirectionalLight->SetWorldRotation((View * FRotator(-30.0, 35.0, 0.0).Quaternion()).Rotator());
		if (UDirectionalLightComponent* Fill = FillLight.Get())
		{
			Fill->SetWorldRotation((View * FRotator(20.0, -45.0, 0.0).Quaternion()).Rotator());
		}
	}
}

bool FGridItemRuntimePreview::Capture()
{
	if (!IsOpen() || !Orbit.bReady || !FApp::CanEverRender())
	{
		return false;
	}
	USceneCaptureComponent2D* CaptureComponent = Camera.Get();
	if (!CaptureComponent->TextureTarget)
	{
		UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(CaptureComponent, NAME_None, RF_Transient);
		Target->ClearColor = FLinearColor::Black;
		Target->RenderTargetFormat = RTF_RGBA8;
		Target->InitAutoFormat(1024, 1024);
		CaptureComponent->TextureTarget = Target;
	}
	if (bStudioCaptureDirty)
	{
		// GamePreview is not ticked by the dungeon world. Process the
		// specified cubemap before requesting the first image.
		Scene->UpdateCaptureContents();
		bStudioCaptureDirty = false;
	}
	CaptureComponent->CaptureScene();
	bCaptureRequested = true;
	return true;
}

UTextureRenderTarget2D* FGridItemRuntimePreview::GetRenderTarget() const
{
	// Never expose the previous item's image after an invalid selection or camera change.
	return IsOpen() && Orbit.bReady && bCaptureRequested ? Camera->TextureTarget.Get() : nullptr;
}

void FGridItemRuntimePreview::Drag(const FVector2D& Delta)
{
	Orbit.Drag(Delta);
	UpdateCamera();
}

void FGridItemRuntimePreview::Zoom(double WheelDelta)
{
	Orbit.Zoom(WheelDelta);
	UpdateCamera();
}

void FGridItemRuntimePreview::Pan(const FVector2D& Delta, double ProjectionAxisPixels)
{
	Orbit.Pan(Delta, ProjectionAxisPixels);
	UpdateCamera();
}

void FGridItemRuntimePreview::Frame(const FRotator& Rotation)
{
	if (IsOpen() && Orbit.bReady)
	{
		Orbit.SetBounds(Actor->GetPresentationBounds(), true, Rotation);
		UpdateCamera();
	}
}
