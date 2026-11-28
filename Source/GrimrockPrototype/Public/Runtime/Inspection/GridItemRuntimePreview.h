#pragma once

#include "CoreMinimal.h"
#include "Runtime/Inspection/GridItemInspectionTypes.h"
#include "Runtime/Inspection/GridItemInspectionOrbit.h"

class FPreviewScene;
class UDirectionalLightComponent;
class AGridItemPreviewActor;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UWorld;

/** Non-UMG runtime preview session. Owns a private GamePreview scene, never
 * inserts a pickup, light or capture into the player's dungeon world.
 * Game thread only. The eventual widget must close it on world teardown.
 */
class GRIMROCKPROTOTYPE_API FGridItemRuntimePreview
{
public:
	FGridItemRuntimePreview();
	~FGridItemRuntimePreview();
	FGridItemRuntimePreview(const FGridItemRuntimePreview&) = delete;
	FGridItemRuntimePreview& operator=(const FGridItemRuntimePreview&) = delete;

	bool Open(UWorld* PlayerWorld);
	void Close();
	EGridItemPreviewResult Present(const FGridItemInspectionView& View);
	void Clear();
	bool Capture();
	void Drag(const FVector2D& Delta);
	void Zoom(double WheelDelta);
	void Pan(const FVector2D& Delta, double ProjectionAxisPixels);
	void Frame(const FRotator& Rotation = FRotator(-12.0, 35.0, 0.0));

	bool IsOpen() const;
	UWorld* GetPreviewWorld() const;
	AGridItemPreviewActor* GetPreviewActor() const;
	UTextureRenderTarget2D* GetRenderTarget() const;
	const FGridItemInspectionOrbit& GetOrbit() const { return Orbit; }
#if WITH_DEV_AUTOMATION_TESTS
	const USceneCaptureComponent2D* GetCaptureForTest() const;
#endif

private:
	void UpdateCamera();

	TUniquePtr<FPreviewScene> Scene;
	TWeakObjectPtr<UWorld> OwnerWorld;
	TWeakObjectPtr<AGridItemPreviewActor> Actor;
	TWeakObjectPtr<USceneCaptureComponent2D> Camera;
	TWeakObjectPtr<UDirectionalLightComponent> FillLight;
#if !UE_BUILD_SHIPPING
	bool bFirstCaptureMeasured = false;
#endif
	FGridItemInspectionOrbit Orbit;
	bool bCaptureRequested = false;
};
