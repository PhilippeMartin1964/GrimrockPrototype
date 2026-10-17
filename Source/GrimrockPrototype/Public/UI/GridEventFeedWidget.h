#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "Runtime/GridEventFeedTypes.h"
#include "GridEventFeedWidget.generated.h"

struct FSlateBrush;
class SBox;
class SScrollBox;
class SVerticalBox;
class UBorder;
class UButton;
class UGridEventFeedSubsystem;
class UFont;
class UScrollBox;
class USizeBox;
class UTextBlock;

/**
 * EVENTHUD01.3 Event Feed interaction shell.
 *
 * Owns presentation-only filtering, generic text projection, follow-newest
 * scroll behaviour and vertical resizing. Gameplay authority and the bounded
 * event history remain in UGridEventFeedSubsystem.
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

	UFUNCTION(BlueprintCallable, Category = "HUD|Event Feed|Filters")
	void SetCategoryEnabled(EGridHudEventCategory Category, bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "HUD|Event Feed|Filters")
	void ToggleCategory(EGridHudEventCategory Category);

	UFUNCTION(BlueprintPure, Category = "HUD|Event Feed|Filters")
	bool IsCategoryEnabled(EGridHudEventCategory Category) const;

	/** Testable projection of the authoritative history. Never mutates SourceEntries. */
	static void FilterEntries(
		const TArray<FGridHudEventEntry>& SourceEntries,
		bool bInShowEvents,
		bool bInShowCombat,
		bool bInShowDiscussion,
		TArray<FGridHudEventEntry>& OutEntries);

	static float ClampPanelHeight(float InHeight, float InMinHeight, float InMaxHeight);
	static bool IsScrollAtEnd(float CurrentOffset, float EndOffset, float Tolerance = 8.0f);

	/** Public deterministic resize primitives used by both native and authored handles. */
	void BeginResizeAtScreenY(float ScreenY, float ViewportScale);
	void UpdateResizeAtScreenY(float ScreenY);
	void EndResize();

	/** Converts platform-pixel mouse movement into UMG viewport logical units. */
	static float ScreenDeltaToViewportDelta(float ScreenDelta, float ViewportScale);

	/** Solid fill brush used by both native Slate and optional UMG shell paths. */
	static const FSlateBrush* GetSolidBackgroundBrush();

	virtual void SynchronizeProperties() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
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
	FLinearColor ShellBackgroundColor = FLinearColor(0.015f, 0.02f, 0.025f, 0.92f);

	/** Explicit EVENTHUD font settings. Defaults to project Alegreya Sans when available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Event Feed|Style")
	FSlateFontInfo EntryFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Event Feed|Style")
	FSlateFontInfo FilterFont;

	/** EVENTHUD01.3 three independent filters, enabled by default. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Event Feed|Filters")
	bool bShowEvents = true;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Event Feed|Filters")
	bool bShowCombat = true;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Event Feed|Filters")
	bool bShowDiscussion = true;

	/** Optional WBP_GridEventFeed bindings. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<USizeBox> SizeBox_EventFeed;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<UBorder> Border_EventFeed;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<UScrollBox> ScrollBox_Entries;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<UButton> Button_ResizeHandle;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<UButton> Button_FilterEvents;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<UButton> Button_FilterCombat;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<UButton> Button_FilterDiscussion;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<UTextBlock> Text_FilterEvents;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<UTextBlock> Text_FilterCombat;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<UTextBlock> Text_FilterDiscussion;

private:
	void SynchronizeShell();
	void BindControls();
	void UnbindControls();
	void BindEventFeed();
	void UnbindEventFeed();
	void RebuildEntryPresentation(bool bForceScrollToEnd = false);
	void ScrollPresentationToEnd();
	float GetPresentationScrollOffset() const;
	float GetPresentationScrollEndOffset() const;
	FText BuildEntryText(const FGridHudEventEntry& Entry) const;
	FText BuildFilterText(EGridHudEventCategory Category) const;
	void HandleNativeUserScrolled(float CurrentOffset);
	void BeginResizeFromCursor();

	UFUNCTION()
	void HandleFilterEventsClicked();

	UFUNCTION()
	void HandleFilterCombatClicked();

	UFUNCTION()
	void HandleFilterDiscussionClicked();

	UFUNCTION()
	void HandleResizePressed();

	UFUNCTION()
	void HandleResizeReleased();

	UFUNCTION()
	void HandleBlueprintUserScrolled(float CurrentOffset);

	UFUNCTION()
	void HandleEventPublished(const FGridHudEventEntry& Entry);

	UPROPERTY(Transient)
	float CurrentPanelHeight = 320.0f;

	TWeakObjectPtr<UGridEventFeedSubsystem> EventFeedSubsystem;
	bool bResizeDragging = false;
	float ResizeStartScreenY = 0.0f;
	float ResizeStartHeight = 320.0f;
	float ResizeViewportScale = 1.0f;
	bool bFollowNewestEntries = true;

	TSharedPtr<SBox> NativeSizeBox;
	TSharedPtr<SScrollBox> NativeScrollBox;
	TSharedPtr<SVerticalBox> NativeEntryList;
};
