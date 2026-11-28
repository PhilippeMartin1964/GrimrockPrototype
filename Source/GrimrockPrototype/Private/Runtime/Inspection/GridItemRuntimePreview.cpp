#include "Runtime/Inspection/GridItemRuntimePreview.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureCube.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "PreviewScene.h"
#include "Runtime/Inspection/GridItemPreviewActor.h"

#if !UE_BUILD_SHIPPING
#include "TextureResource.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogGridItemPreview, Log, All);

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
		.SetLightRotation(FRotator(-40.0, 35.0, 0.0)).SetLightBrightness(12.0f).SetSkyBrightness(0.0f));
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
	// A small on-demand preview must not depend on the dungeon's Lumen
	// history or an asynchronously captured sky.
	FPostProcessSettings& StudioSettings = CaptureComponent->PostProcessSettings;
	StudioSettings.bOverride_DynamicGlobalIlluminationMethod = true;
	StudioSettings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;
	StudioSettings.bOverride_ReflectionMethod = true;
	StudioSettings.ReflectionMethod = EReflectionMethod::None;
	StudioSettings.AmbientCubemap = Actor->GetStudioCubemap();
	StudioSettings.bOverride_AmbientCubemapIntensity = true;
	StudioSettings.AmbientCubemapIntensity = 1.0f;
	StudioSettings.bOverride_AmbientCubemapTint = true;
	StudioSettings.AmbientCubemapTint = FLinearColor::White;
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
#if !UE_BUILD_SHIPPING
	bFirstCaptureMeasured = false;
#endif
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
	CaptureComponent->CaptureScene();
	bCaptureRequested = true;
#if !UE_BUILD_SHIPPING
	// One readback per presentation, never per drag or frame. Distinguish
	// capture darkness from Slate opacity and identify the loaded binary.
	if (!bFirstCaptureMeasured)
	{
		bFirstCaptureMeasured = true;
		FTextureRenderTargetResource* Resource = CaptureComponent->TextureTarget->GameThread_GetRenderTargetResource();
		TArray<FColor> Pixels;
		if (Resource && Resource->ReadPixels(Pixels) && !Pixels.IsEmpty())
		{
			int32 NonBlack = 0;
			int32 Bright = 0;
			int32 Peak = 0;
			int32 MinAlpha = 255;
			int32 MaxAlpha = 0;
			int32 MinLitAlpha = 255;
			for (const FColor& Pixel : Pixels)
			{
				const int32 Value = FMath::Max3(int32(Pixel.R), int32(Pixel.G), int32(Pixel.B));
				Peak = FMath::Max(Peak, Value);
				MinAlpha = FMath::Min(MinAlpha, int32(Pixel.A));
				MaxAlpha = FMath::Max(MaxAlpha, int32(Pixel.A));
				if (Value > 4)
				{
					++NonBlack;
					MinLitAlpha = FMath::Min(MinLitAlpha, int32(Pixel.A));
				}
				if (Value >= 64) ++Bright;
			}
			UE_LOG(LogGridItemPreview, Log, TEXT("[ItemExamination] StudioCaptureV2 Pixels=%d RGBAbove4=%d RGBAbove63=%d Peak=%d Alpha=%d..%d LitMinAlpha=%d"),
				Pixels.Num(), NonBlack, Bright, Peak, MinAlpha, MaxAlpha, NonBlack ? MinLitAlpha : -1);
		}
		else
		{
			UE_LOG(LogGridItemPreview, Warning, TEXT("[ItemExamination] StudioCaptureV2 GPU readback unavailable"));
		}
	}
#endif
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

#if WITH_DEV_AUTOMATION_TESTS
const USceneCaptureComponent2D* FGridItemRuntimePreview::GetCaptureForTest() const
{
	return Camera.Get();
}
#endif
