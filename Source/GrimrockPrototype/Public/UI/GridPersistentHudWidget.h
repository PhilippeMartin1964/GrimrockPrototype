#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "GridPersistentHudWidget.generated.h"

class AGrimrockPartyPawn;
class UButton;
class UGridCombatHudActionWidget;
class UGridNavigationTooltipWidget;
class UGridPartyInventoryComponent;
class UGridPartyPortraitHudWidget;
class UGridRPGNotificationWidget;
class UHorizontalBox;
class UImage;
class UPanelWidget;
class UVerticalBox;

/**
 * UI-GLOBALHUD01 persistent gameplay chrome.
 * Owns global navigation and the general action bar independently from combat presentation.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridPersistentHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Persistent")
	TObjectPtr<AGrimrockPartyPawn> PartyPawn;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Persistent")
	TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

	/** Required visual class for persistent action-bar slots. The combat HUD no longer provides a fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Persistent|Actions")
	TSubclassOf<UGridCombatHudActionWidget> ActionWidgetClass;

	/** UI-PARTYHUD01.2 visual class used to materialize one entry in the persistent left-side party rail. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Persistent|Party")
	TSubclassOf<UGridPartyPortraitHudWidget> PartyPortraitWidgetClass;

	/** Compact custom tooltip class shared by ESC/I/K/G/M/J/H navigation buttons. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TSubclassOf<UGridNavigationTooltipWidget> NavigationTooltipWidgetClass;

	/** Used only until the first action widget can report its authored desired width. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Persistent|Actions", meta = (ClampMin = "1.0"))
	float FallbackActionSlotWidth = 50.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Persistent|Actions")
	int32 VisibleActionSlotCount = 12;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Persistent|Actions")
	TArray<TObjectPtr<UGridCombatHudActionWidget>> ActionWidgets;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Persistent|Actions")
	TObjectPtr<UHorizontalBox> ActionBarRow;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UPanelWidget> Panel_GlobalNavigation;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Actions")
	TObjectPtr<UPanelWidget> Panel_ActionBar;

	/** UI-PARTYHUD01.2 Designer materialization point. Must be a vertical stack in WBP_GridPersistentHud. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Party")
	TObjectPtr<UVerticalBox> Panel_PartyPortraits;

	/** Runtime-only presenters, ordered exactly like UGridPartyInventoryComponent::PartyPortraitOrder. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Persistent|Party")
	TArray<TObjectPtr<UGridPartyPortraitHudWidget>> PartyPortraitWidgets;

	/** RPG-LEVELUX01.2 Designer materialization point for global non-modal progression feedback. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Progression")
	TObjectPtr<UGridRPGNotificationWidget> Notification_Progression;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UButton> Button_NavEscape;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UButton> Button_NavInventory;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UButton> Button_NavSkills;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UButton> Button_NavCrafting;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UButton> Button_NavMap;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UButton> Button_NavJournal;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UButton> Button_NavHelp;

	/** Optional golden selection frames, authored with the same brush/pattern as WBP_InventoryBag filter frames. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UImage> Image_NavEscapeSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UImage> Image_NavInventorySelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UImage> Image_NavSkillsSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UImage> Image_NavCraftingSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UImage> Image_NavMapSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UImage> Image_NavJournalSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UImage> Image_NavHelpSelectionFrame;

	UFUNCTION(BlueprintCallable, Category = "HUD|Persistent")
	void InitializePersistentHud(AGrimrockPartyPawn* InPartyPawn);

	UFUNCTION(BlueprintCallable, Category = "HUD|Persistent")
	void RefreshFromSources();

	/** Event-driven refresh of the persistent party rail. No per-frame RPG polling. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Persistent|Party")
	void RefreshPartyPortraits();

	/** Returns false until the optional Designer notification surface is materialized. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Persistent|Progression")
	bool ShowProgressionNotification(const FRPGProgressionNotificationView& Notification);

	/** Pure layout rule: fixed-width adjacent slots, with any remainder left empty at the right edge. */
	static int32 CalculateVisibleActionSlotCount(float ViewportWidth, float NavigationWidth, float ActionSlotWidth);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	float LastSynchronizedViewportWidth = -1.0f;
	float LastSynchronizedNavigationWidth = -1.0f;
	float LastSynchronizedActionSlotWidth = -1.0f;
	int32 LastSynchronizedCharacterIndex = INDEX_NONE;
	int32 LastSynchronizedUsedSlotCount = INDEX_NONE;

	int32 ResolveActionBarCharacterIndex() const;
	float ResolveActionSlotWidth() const;
	void SynchronizeActionBarToViewport();
	void BindNavigationButtons();
	void UnbindNavigationButtons();
	void ApplyNavigationTooltips();
	void RefreshNavigationSelection();
	void EnsureActionWidgets();
	void RefreshActionWidgets();
	void BindInventoryEvents();
	void UnbindInventoryEvents();

	UFUNCTION()
	void HandlePartyInventoryChanged(int32 CharacterIndex);

	UFUNCTION()
	void HandleNavEscapeClicked();

	UFUNCTION()
	void HandleNavInventoryClicked();

	UFUNCTION()
	void HandleNavSkillsClicked();

	UFUNCTION()
	void HandleNavCraftingClicked();

	UFUNCTION()
	void HandleNavMapClicked();

	UFUNCTION()
	void HandleNavJournalClicked();

	UFUNCTION()
	void HandleNavHelpClicked();
};
