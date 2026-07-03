#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrimrockDesignSurfaceWidget.generated.h"

class UCanvasPanel;
class UScaleBox;
class USizeBox;

UCLASS (Abstract)
class GRIMROCKPROTOTYPE_API UGrimrockDesignSurfaceWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UFUNCTION (BlueprintCallable, Category = "UI|Design Surface")
    void ApplyDesignSurfaceViewportLimit ();

protected:
    virtual void NativeConstruct () override;

    virtual void NativeTick (
        const FGeometry& MyGeometry,
        float InDeltaTime) override;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Design Surface")
    TObjectPtr<UCanvasPanel> CanvasPanel_Root;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Design Surface")
    TObjectPtr<UScaleBox> ScaleBox_DesignRoot;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Design Surface")
    TObjectPtr<USizeBox> SizeBox_DesignSurface;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "UI|Design Surface", meta = (ClampMin = "1.0"))
    float DesignWidth = 1920.0f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "UI|Design Surface", meta = (ClampMin = "1.0"))
    float DesignHeight = 1080.0f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "UI|Design Surface", meta = (ClampMin = "0.0"))
    float SafeMarginPx = 48.0f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "UI|Design Surface")
    bool bLimitToDesignSize = true;

private:
    UCanvasPanel* ResolveCanvasPanelRoot () const;
    UScaleBox* ResolveScaleBoxDesignRoot () const;
    USizeBox* ResolveSizeBoxDesignSurface () const;

    UPROPERTY (Transient)
    FVector2D LastAppliedViewportPx = FVector2D::ZeroVector;

    UPROPERTY (Transient)
    float LastAppliedViewportScale = 0.0f;
};
