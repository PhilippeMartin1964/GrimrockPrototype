#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "GridSkillsWidget.generated.h"

class AGrimrockPartyPawn;
class UButton;
class UGridPartyInventoryComponent;
class UPanelWidget;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGridSkillsWidgetRefreshedSignature);

/**
 * Read-only presentation bridge for WBP_GridSkills.
 * Character selection remains authoritative in UGridPartyInventoryComponent.
 *
 * UI-RPG03 designer mode is activated automatically when the Blueprint contains
 * Panel_GridSkillsDesignerRoot. Until then, the MON20 native renderer remains a
 * compatibility fallback.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridSkillsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	FGridSkillsPageView View;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Skills|UI|Events")
	FGridSkillsWidgetRefreshedSignature OnSkillsRefreshed;

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI")
	void InitializeSkillsWidget(AGrimrockPartyPawn* InPartyPawn);

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI")
	void RefreshSkills();

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	int32 GetSkillEntryCount() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	bool GetSkillEntry(int32 EntryIndex, FGridSkillEntryView& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	int32 GetTalentEntryCount() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	bool GetTalentEntry(int32 EntryIndex, FGridTalentEntryView& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	bool IsUsingDesignerPresentation() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|UI")
	bool GetCurrentClassPresentation(FRPGClassPresentationDefinition& OutPresentation) const;

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|UI")
	bool GetPresentedTalentBranch(
		int32 VisualIndex,
		FGridTalentBranchView& OutBranch,
		FRPGTalentBranchPresentationDefinition& OutPresentation) const;

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI")
	void ShowSkillsTab();

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI")
	void ShowTalentsTab();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePartyInventoryChanged(int32 CharacterIndex);

	UFUNCTION()
	void HandleSkillsTabClicked();

	UFUNCTION()
	void HandleTalentsTabClicked();

	void ClearView();
	void BindDesignerShell();
	void UnbindDesignerShell();
	void ApplyDesignerPresentation();
	void RebuildPresentation();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Panel_GridSkillsDesignerRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CharacterName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ClassLevel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TalentPoints;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_SkillsTab;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TalentsTab;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> Switcher_SkillsTalents;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_BranchLeft;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_BranchCenter;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_BranchRight;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> NativeScrollBox;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> NativeContentBox;

	bool bRefreshInProgress = false;
};
