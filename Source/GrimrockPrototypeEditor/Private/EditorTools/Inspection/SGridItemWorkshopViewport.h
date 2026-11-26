#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "Runtime/Inspection/GridItemInspectionTypes.h"

class FAdvancedPreviewScene;
class AGridItemPreviewActor;
class FGridItemWorkshopViewportClient;
class UStaticMeshComponent;

class SGridItemWorkshopViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SGridItemWorkshopViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SGridItemWorkshopViewport() override;

	EGridItemPreviewResult Display(const FGridItemInspectionView& View, bool bRecenter);
	void Frame(const FRotator& Rotation = FRotator(-12.0, 35.0, 0.0));
	void Clear();
	UStaticMeshComponent* GetMesh() const;

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override { return nullptr; }

private:
	TSharedPtr<FAdvancedPreviewScene> Scene;
	TSharedPtr<FGridItemWorkshopViewportClient> WorkshopClient;
	TWeakObjectPtr<AGridItemPreviewActor> Preview;
};
