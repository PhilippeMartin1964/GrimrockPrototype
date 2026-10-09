#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "GridSkillsWidget.generated.h"

class AGrimrockPartyPawn;
class UButton;
class UGridPartyInventoryComponent;
class UGridRPGNotificationWidget;
class UGridSkillEntryWidget;
class URPGSkillAsset;
class UGridTalentBranchWidget;
class UPanelWidget;
class UGridTalentDetailWidget;
class UTextBlock;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGridSkillsWidgetRefreshedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTalentSelectionChangedSignature, FName, TalentNodeId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridProgressionNotificationSignature, const FRPGProgressionNotificationView&, Notification);

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

	/** UI-RPG06.2B: reusable row class used to render the authoritative Skills projection. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|UI|Presentation")
	TSubclassOf<UGridSkillEntryWidget> SkillEntryWidgetClass;

	/** UI-RPG06.3A: mandatory Designer container for the canonical Skills projection. */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "RPG|Skills|UI|Presentation")
	TObjectPtr<UPanelWidget> Panel_SkillEntries;

	/** Mandatory empty-state label for a missing/empty canonical Skills projection. */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "RPG|Skills|UI|Presentation")
	TObjectPtr<UTextBlock> Text_EmptySkills;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Skills|UI|Events")
	FGridSkillsWidgetRefreshedSignature OnSkillsRefreshed;

	/** UI-only selection. NAME_None means that no conceptual Talent node is selected. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName SelectedTalentNodeId = NAME_None;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Talents|UI|Events")
	FGridTalentSelectionChangedSignature OnTalentSelectionChanged;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Progression|Notification")
	FRPGProgressionNotificationView LastProgressionNotification;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Progression|Notification|Events")
	FGridProgressionNotificationSignature OnProgressionNotification;

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI")
	void InitializeSkillsWidget(AGrimrockPartyPawn* InPartyPawn);

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI")
	void RefreshSkills();

	/** Rebuilds the Designer Skills list from View.Skills. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI|Presentation")
	void RebuildSkillEntryWidgets();

	/** Starts a fresh undo boundary every time the standalone Skills window opens. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Allocation")
	void BeginSkillAllocationSession();

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Allocation")
	bool CommitSkillRankIncrease(FName SkillId, FText& OutFeedback);

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Allocation")
	bool CommitSkillRankDecrease(FName SkillId, FText& OutFeedback);

	// Read-only accessors retained for diagnostics and Blueprint presentation helpers.
	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	int32 GetSkillEntryCount() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	bool GetSkillEntry(int32 EntryIndex, FGridSkillEntryView& OutEntry) const;

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

	/** Selects a conceptual Talent node for presentation only; never commits progression. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|UI")
	bool SelectTalentNode(FName TalentNodeId);

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|UI")
	void ClearTalentSelection();

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|UI")
	bool GetSelectedTalentNode(FGridTalentNodeView& OutNode) const;

	/** Called only after UI confirmation. Supports simple and variant ChoiceIds; transaction service remains sole authority. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Acquire")
	bool CommitConfirmedTalentChoice(FName ChoiceId, FText& OutFeedback);

	UFUNCTION(BlueprintCallable, Category = "RPG|Progression|Notification")
	void PublishProgressionNotification(const FRPGProgressionNotificationView& Notification);

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

	UFUNCTION()
	void HandleTalentNodeClicked(FName TalentNodeId);

	UFUNCTION()
	void HandleTalentAcquireConfirmed(FName ChoiceId);

	UFUNCTION()
	void HandleSkillIncreaseRequested(FName SkillId);

	UFUNCTION()
	void HandleSkillDecreaseRequested(FName SkillId);

	const URPGSkillAsset* ResolveCanonicalSkillDefinition(FName SkillId) const;
	FString MakeSkillAllocationSessionKey(const FGuid& CharacterId, FName SkillId) const;
	int32 GetSessionPurchasedSkillRankCount(const FGuid& CharacterId, FName SkillId) const;
	int32 GetSessionSkillRankFloor(const FGuid& CharacterId, FName SkillId) const;
	void RecordSessionSkillPurchase(const FGuid& CharacterId, FName SkillId, int32 PreviousRank);
	void ConsumeSessionSkillPurchase(const FGuid& CharacterId, FName SkillId);

	const FGridTalentNodeView* FindTalentNode(FName TalentNodeId) const;
	void ClearView();
	void BindDesignerShell();
	void UnbindDesignerShell();
	void ApplyDesignerPresentation();
	void ApplyTalentDetailPresentation();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CharacterName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ClassLevel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_TalentPoints;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillPoints;

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

	/** UI-RPG04.2A: optional until WBP_RPGTalentDetail is materialized in Designer. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UGridTalentDetailWidget> Detail_Talent;

	/** UI-RPG05 : optionnel jusqu’à matérialisation de WBP_RPGNotification. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UGridRPGNotificationWidget> Notification_Progression;

	/** UI-session-only history. Never persisted and never gameplay authority. */
	TMap<FString, int32> SessionPurchasedSkillRanks;
	TMap<FString, int32> SessionSkillRankFloors;

	bool bRefreshInProgress = false;
};
