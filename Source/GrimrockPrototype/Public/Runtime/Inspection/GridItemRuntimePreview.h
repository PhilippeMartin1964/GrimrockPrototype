#pragma once

#include "CoreMinimal.h"
#include "Runtime/Inspection/GridItemInspectionTypes.h"
#include "Runtime/Inspection/GridItemInspectionOrbit.h"

class FPreviewScene;
class AGridItemPreviewActor;
class UDirectionalLightComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UWorld;

enum class EGridItemCaptureResult : uint8
{
	Unavailable,
	WaitingForAssets,
	Ready
};

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
	EGridItemCaptureResult Capture();
	void Drag(const FVector2D& Delta);
	void Zoom(double WheelDelta);
	void Pan(const FVector2D& Delta, double ProjectionAxisPixels);
	void Frame(TOptional<FRotator> Rotation = {});

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
	FGridItemInspectionOrbit Orbit;
	bool bCaptureRequested = false;
};
