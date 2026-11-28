#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "Runtime/Inspection/GridItemRuntimePreview.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridItemExaminationWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UVerticalBox;
class UWidget;
class UGridItemTooltipWeaponLineWidget;
class UGridItemSubsystem;

/** Player-facing snapshot. No character comparison, no mutations or synthetic levels. */
struct GRIMROCKPROTOTYPE_API FGridItemExaminationData
{
	FGridItemInspectionView View;
	TArray<FGridWeaponTooltipFact> Facts;

	static bool Build(const UGridItemSubsystem* Registry, const FGridItemInstance& Instance,
		FGridItemExaminationData& Out);
};

DECLARE_MULTICAST_DELEGATE(FGridItemExaminationCloseRequested);

/** The Designer owns all layout and styling. The inventory owner must
 * revalidate the live slot before presentation and own modal input policy.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridItemExaminationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	bool PresentItem(const UGridItemSubsystem* Registry, const FGridItemInstance& Instance);
	void ClearExamination();
	bool HasDesignerContract() const;
	FGridItemExaminationCloseRequested OnCloseRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& Event) override;

	UPROPERTY(EditDefaultsOnly, Category="Inspection|Designer")
	TSubclassOf<UGridItemTooltipWeaponLineWidget> StatRowWidgetClass;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UImage> Image_RenderTarget;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UImage> Image_FallbackIcon;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UTextBlock> Text_PreviewUnavailable;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UTextBlock> Text_ItemName;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UTextBlock> Text_ItemLevel;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UWidget> Border_LevelBadge;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UTextBlock> Text_Description;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UTextBlock> Text_Weight;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UTextBlock> Text_Quantity;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UVerticalBox> VerticalBox_Stats;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UWidget> Section_Stats;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UButton> Button_Close;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UButton> Button_Front;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UButton> Button_Profile;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UButton> Button_Back;
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inspection|Designer")
	TObjectPtr<UButton> Button_Reset;

private:
	UFUNCTION()
	void RequestClose();
	UFUNCTION()
	void FrameFront();
	UFUNCTION()
	void FrameProfile();
	UFUNCTION()
	void FrameBack();
	UFUNCTION()
	void FrameReset();

	void RefreshCapture();
	bool IsOverPreview(const FVector2D& ScreenPosition) const;
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	FGridItemRuntimePreview Preview;
	TWeakObjectPtr<UWorld> PresentationWorld;
	FDelegateHandle WorldCleanupHandle;
	FKey DragButton;
	bool bPresented = false;
};
