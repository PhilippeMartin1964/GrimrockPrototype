#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridPersistentHudWidget.generated.h"

class AGrimrockPartyPawn;
class UButton;
class UGridCombatHudActionWidget;
class UGridPartyInventoryComponent;
class UHorizontalBox;
class UImage;
class UPanelWidget;

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

	/** Optional override. Falls back to the action widget class configured on the combat HUD backend. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Persistent|Actions")
	TSubclassOf<UGridCombatHudActionWidget> ActionWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Persistent|Actions")
	TArray<TObjectPtr<UGridCombatHudActionWidget>> ActionWidgets;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Persistent|Actions")
	TObjectPtr<UHorizontalBox> ActionBarRow;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	TObjectPtr<UPanelWidget> Panel_GlobalNavigation;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Actions")
	TObjectPtr<UPanelWidget> Panel_ActionBar;

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

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BindNavigationButtons();
	void UnbindNavigationButtons();
	void RefreshNavigationSelection();
	void EnsureActionWidgets();
	void RefreshActionWidgets();

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
