#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "GridSkillsWidget.generated.h"

class AGrimrockPartyPawn;
class UButton;
class UGridPartyInventoryComponent;
class UGridTalentBranchWidget;
class UTextBlock;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGridSkillsWidgetRefreshedSignature);

/**
 * Read-only presentation bridge for WBP_GridSkills.
 * Character selection remains authoritative in UGridPartyInventoryComponent.
 *
 * UI-RPG03.3 removes the MON20 native widget renderer: WBP_GridSkills is now
 * the single presentation authority for the Skills / Talents surface.
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

	// Retained for the future Skills page renderer. These are read-only projections only.
	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	int32 GetSkillEntryCount() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	bool GetSkillEntry(int32 EntryIndex, FGridSkillEntryView& OutEntry) const;

	// Transitional flat Talent accessors retained until the flat Talent projection is retired separately.
	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	int32 GetTalentEntryCount() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	bool GetTalentEntry(int32 EntryIndex, FGridTalentEntryView& OutEntry) const;

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

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CharacterName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ClassLevel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_TalentPoints;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_SkillsTab;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_TalentsTab;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher_SkillsTalents;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridTalentBranchWidget> Branch_Left;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridTalentBranchWidget> Branch_Center;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridTalentBranchWidget> Branch_Right;

	bool bRefreshInProgress = false;
};
