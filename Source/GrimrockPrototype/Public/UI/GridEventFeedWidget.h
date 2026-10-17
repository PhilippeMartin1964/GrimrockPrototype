#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "Runtime/GridEventFeedTypes.h"
#include "GridEventFeedWidget.generated.h"

class UButton;
class UGridEventFeedSubsystem;
class UFont;
class UScrollBox;
class USizeBox;
class UTextBlock;

/** EVENTHUD01.6 editable semantic palette. Colour is supplementary; text remains authoritative. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridEventFeedSemanticPalette
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Default = FLinearColor(0.88f, 0.86f, 0.80f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Information = FLinearColor(0.88f, 0.86f, 0.80f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Interaction = FLinearColor(1.00f, 0.72f, 0.25f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Locked = FLinearColor(0.78f, 0.50f, 0.24f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Mechanism = FLinearColor(0.82f, 0.44f, 0.22f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Secret = FLinearColor(1.00f, 0.82f, 0.24f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Danger = FLinearColor(1.00f, 0.28f, 0.22f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor PhysicalDamage = FLinearColor(0.92f, 0.38f, 0.24f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Fire = FLinearColor(1.00f, 0.32f, 0.10f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Ice = FLinearColor(0.48f, 0.84f, 1.00f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Lightning = FLinearColor(0.58f, 0.52f, 1.00f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Poison = FLinearColor(0.46f, 0.86f, 0.34f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Arcane = FLinearColor(0.72f, 0.42f, 1.00f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Healing = FLinearColor(0.52f, 1.00f, 0.58f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Buff = FLinearColor(0.48f, 0.90f, 0.52f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Debuff = FLinearColor(1.00f, 0.42f, 0.36f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Dialogue = FLinearColor(0.82f, 0.86f, 0.96f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Holy = FLinearColor(1.00f, 0.92f, 0.62f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed|Semantic Colors")
	FLinearColor Necrotic = FLinearColor(0.76f, 0.42f, 0.78f, 1.0f);
};

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

	/** EVENTHUD01.6 deterministic semantic-to-colour mapping. */
	static FLinearColor ResolveSemanticColor(
		EGridHudEventSemantic Semantic,
		const FGridEventFeedSemanticPalette& Palette);

	UFUNCTION(BlueprintPure, Category = "HUD|Event Feed|Style")
	FLinearColor GetSemanticColor(EGridHudEventSemantic Semantic) const;

	/** Public deterministic resize primitives used by the authored UMG handle. */
	void BeginResizeAtScreenY(float ScreenY, float ViewportScale);
	void UpdateResizeAtScreenY(float ScreenY);
	void EndResize();

	/** Converts platform-pixel mouse movement into UMG viewport logical units. */
	static float ScreenDeltaToViewportDelta(float ScreenDelta, float ViewportScale);

	virtual void SynchronizeProperties() override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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

	/** Explicit EVENTHUD font settings. Defaults to project Alegreya Sans when available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Event Feed|Style")
	FSlateFontInfo EntryFont;

	/** Editable in WBP_GridEventFeed Class Defaults; no semantic icons are used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Event Feed|Style")
	FGridEventFeedSemanticPalette SemanticPalette;

	/** EVENTHUD01.3 three independent filters, enabled by default. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Event Feed|Filters")
	bool bShowEvents = true;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Event Feed|Filters")
	bool bShowCombat = true;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Event Feed|Filters")
	bool bShowDiscussion = true;

	/** Authored WBP_GridEventFeed bindings. Visual style belongs to UMG. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Event Feed")
	TObjectPtr<USizeBox> SizeBox_EventFeed;

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
};
