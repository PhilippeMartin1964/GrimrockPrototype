#include "Runtime/Inspection/GridItemRuntimePreview.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureCube.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "PreviewScene.h"
#include "Runtime/Inspection/GridItemPreviewActor.h"

namespace
{
	bool IsPreparingRenderAssets(UStaticMeshComponent& Component)
	{
		const UStaticMesh* Mesh = Component.GetStaticMesh();
		if (!Mesh)
		{
			return false;
		}
		// UE skips creating a scene proxy while the mesh is compiling and can
		// defer it while the component's graphics pipelines are being prepared.
		if (Mesh->IsCompiling() || Component.IsPSOPrecaching())
		{
			return true;
		}
#if WITH_EDITOR
		for (int32 Index = 0; Index < Component.GetNumMaterials(); ++Index)
		{
			const UMaterialInterface* Material = Component.GetMaterial(Index);
			if (Material && Material->IsCompiling())
			{
				return true;
			}
		}
		TArray<UTexture*> Textures;
		Component.GetUsedTextures(Textures, EMaterialQualityLevel::Num);
		for (const UTexture* Texture : Textures)
		{
			if (Texture && Texture->IsCompiling())
			{
				return true;
			}
		}
#endif
		return false;
	}
}

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

	// Match the Asset Viewer light path in an independent Engine preview world.
	Scene = MakeUnique<FPreviewScene>(FPreviewScene::ConstructionValues()
			.SetEditor(false)
			.SetCreatePhysicsScene(false)
			.ShouldSimulatePhysics(false)
			.AllowAudioPlayback(false)
			.SetTransactional(false)
			.SetForceMipsResident(false)
			.SetLightBrightness(4.0f)
			.SetSkyBrightness(6.0f));
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
	Scene->DirectionalLight->SetLightSourceAngle(12.0f);
	Scene->SetSkyCubemap(Actor->GetStudioCubemap());

	UDirectionalLightComponent* Fill = NewObject<UDirectionalLightComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	Fill->SetMobility(EComponentMobility::Movable);
	Fill->SetIntensity(2.0f);
	Fill->SetLightSourceAngle(30.0f);
	Fill->SetCastShadows(false);
	Scene->AddComponent(Fill, FTransform::Identity, false);
	FillLight = Fill;

	USceneCaptureComponent2D* CaptureComponent = NewObject<USceneCaptureComponent2D>(GetTransientPackage(), NAME_None, RF_Transient);
	CaptureComponent->SetMobility(EComponentMobility::Movable);
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->bOverride_CustomNearClippingPlane = true;
	CaptureComponent->CustomNearClippingPlane = FGridItemInspectionOrbit::NearClipCm;
	CaptureComponent->CaptureSource = SCS_FinalToneCurveHDR;
	CaptureComponent->ShowFlags.SetTemporalAA(false);
	FPostProcessSettings& StudioSettings = CaptureComponent->PostProcessSettings;
	StudioSettings.bOverride_DynamicGlobalIlluminationMethod = true;
	StudioSettings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;
	StudioSettings.bOverride_ReflectionMethod = true;
	StudioSettings.ReflectionMethod = EReflectionMethod::None;
	StudioSettings.bOverride_BloomIntensity = true;
	StudioSettings.BloomIntensity = 0.0f;
	StudioSettings.bOverride_VignetteIntensity = true;
	StudioSettings.VignetteIntensity = 0.0f;
	StudioSettings.bOverride_LocalExposureHighlightContrastScale = true;
	StudioSettings.LocalExposureHighlightContrastScale = 1.0f;
	StudioSettings.bOverride_LocalExposureShadowContrastScale = true;
	StudioSettings.LocalExposureShadowContrastScale = 1.0f;
	StudioSettings.bOverride_AutoExposureMethod = true;
	StudioSettings.AutoExposureMethod = AEM_Manual;
	StudioSettings.bOverride_AutoExposureBias = true;
	StudioSettings.AutoExposureBias = 1.0f;
	StudioSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	StudioSettings.AutoExposureApplyPhysicalCameraExposure = false;
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
		Frame();
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
		// Keep the two soft sources in the camera's hemisphere as it orbits.
		const FQuat View = Orbit.Orientation;
		Scene->DirectionalLight->SetWorldRotation((View * FRotator(-30.0, 35.0, 0.0).Quaternion()).Rotator());
		FillLight->SetWorldRotation((View * FRotator(10.0, -30.0, 0.0).Quaternion()).Rotator());
	}
}

EGridItemCaptureResult FGridItemRuntimePreview::Capture()
{
	if (!IsOpen() || !Orbit.bReady || !FApp::CanEverRender())
	{
		return EGridItemCaptureResult::Unavailable;
	}
	USkyLightComponent* SkyLight = Scene->SkyLight;
	if (SkyLight->Cubemap && !SkyLight->GetProcessedSkyTexture())
	{
#if WITH_EDITOR
		if (SkyLight->Cubemap->IsCompiling())
		{
			return EGridItemCaptureResult::WaitingForAssets;
		}
#endif
		// This preview world has no editor viewport to process its HDR environment.
		Scene->UpdateCaptureContents();
	}
	if (IsPreparingRenderAssets(*Actor->GetPreviewMesh()) || IsPreparingRenderAssets(*Actor->GetPreviewOrnament()))
	{
		return EGridItemCaptureResult::WaitingForAssets;
	}
	USceneCaptureComponent2D* CaptureComponent = Camera.Get();
	if (!CaptureComponent->TextureTarget)
	{
		UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(CaptureComponent, NAME_None, RF_Transient);
		Target->ClearColor = FLinearColor::Black;
		// Tone-curve output is linear; store in an sRGB texture for Slate.
		// Do not force a linear 8-bit surface or apply a second display gamma.
		Target->RenderTargetFormat = RTF_RGBA8_SRGB;
		Target->bForceLinearGamma = false;
		Target->InitAutoFormat(1024, 1024);
		Target->UpdateResourceImmediate(true);
		CaptureComponent->TextureTarget = Target;
	}
	CaptureComponent->CaptureScene();
	bCaptureRequested = true;
	return EGridItemCaptureResult::Ready;
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

void FGridItemRuntimePreview::Frame(TOptional<FRotator> Rotation)
{
	if (IsOpen() && Actor->GetPreviewMesh()->GetStaticMesh())
	{
		const FBoxSphereBounds Bounds = Actor->GetPresentationBounds();
		// Favour the broader side of a thin item in the opening three-quarter view.
		const FRotator DefaultRotation(-12.0, Bounds.BoxExtent.X > Bounds.BoxExtent.Y ? 65.0 : 25.0, 0.0);
		Orbit.SetBounds(Bounds, true, Rotation.Get(DefaultRotation), 1.85);
		UpdateCamera();
	}
}

#if WITH_DEV_AUTOMATION_TESTS
const USceneCaptureComponent2D* FGridItemRuntimePreview::GetCaptureForTest() const
{
	return Camera.Get();
}
#endif
