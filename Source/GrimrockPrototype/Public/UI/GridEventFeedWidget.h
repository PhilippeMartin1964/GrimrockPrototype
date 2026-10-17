#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GridEventFeedWidget.generated.h"

class SBox;
class SScrollBox;
class UBorder;
class UScrollBox;
class USizeBox;

/**
 * EVENTHUD01.2 presentation shell.
 *
 * This step intentionally renders no event rows and no category controls.
 * EVENTHUD01.3 adds filter/resize interaction; later milestones bind producers.
 * A native Slate fallback keeps the shell usable before WBP_GridEventFeed is
 * authored in Unreal Editor.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridEventFeedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGridEventFeedWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "HUD|Event Feed")
	void SetPanelHeight(float InHeight);

	UFUNCTION(BlueprintPure, Category = "HUD|Event Feed")
	float GetPanelHeight() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Event Feed")
	float GetPanelWidth() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Event Feed")
	float GetMinPanelHeight() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Event Feed")
	float GetMaxPanelHeight() const;

	static float ClampPanelHeight(float InHeight, float InMinHeight, float InMaxHeight);

	virtual void SynchronizeProperties() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

public:
	/** Fixed width for EVENTHUD01 V1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Event Feed|Layout", meta = (ClampMin = "1.0"))
	float PanelWidth = 560.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Event Feed|Layout", meta = (ClampMin = "1.0"))
	float MinPanelHeight = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Event Feed|Layout", meta = (ClampMin = "1.0"))
	float MaxPanelHeight = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Event Feed|Layout", meta = (ClampMin = "1.0"))
	float InitialPanelHeight = 320.0f;

	/** Simple native colour only; semantic colour coding belongs to EVENTHUD01.6. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Event Feed|Style")
	FLinearColor ShellBackgroundColor = FLinearColor(0.015f, 0.02f, 0.025f, 0.82f);

	/** Optional WBP_GridEventFeed bindings. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<USizeBox> SizeBox_EventFeed;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<UBorder> Border_EventFeed;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<UScrollBox> ScrollBox_Entries;

private:
	void SynchronizeShell();

	UPROPERTY(Transient)
	float CurrentPanelHeight = 320.0f;

	TSharedPtr<SBox> NativeSizeBox;
	TSharedPtr<SScrollBox> NativeScrollBox;
};
