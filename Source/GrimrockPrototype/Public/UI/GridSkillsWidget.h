#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "GridSkillsWidget.generated.h"

class AGrimrockPartyPawn;
class UBorder;
class UButton;
class UGridPartyInventoryComponent;
class UGridRPGNotificationWidget;
class UGridSkillDetailWidget;
class UGridSkillEntryWidget;
class URPGClassVisualAsset;
class URPGSkillAsset;
class URPGSkillPresentationAsset;
class UGridTalentBranchWidget;
class UGridTalentNodeWidget;
class UGridTalentVariantFlyoutWidget;
class UPanelWidget;
class UGridTalentDetailWidget;
class UImage;
class UTextBlock;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGridSkillsWidgetRefreshedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridSkillSelectionChangedSignature, FName, SkillId);
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

	/** UI-only ordering. Never persisted and never mutates canonical Skill data. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Skills|UI|Sort")
	EGridSkillSortColumn SkillSortColumn = EGridSkillSortColumn::Name;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Skills|UI|Sort")
	EGridSkillSortDirection SkillSortDirection = EGridSkillSortDirection::Ascending;

	/** UI-RPG06.2B: reusable row class used to render the authoritative Skills projection. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|UI|Presentation")
	TSubclassOf<UGridSkillEntryWidget> SkillEntryWidgetClass;

	/** Optional UI-SKILLS02 presentation catalog; gameplay-only detail remains valid when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|UI|Presentation")
	TObjectPtr<URPGSkillPresentationAsset> SkillPresentationAsset;

	/** UI-RPG06.3A: mandatory Designer container for the canonical Skills projection. */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "RPG|Skills|UI|Presentation")
	TObjectPtr<UPanelWidget> Panel_SkillEntries;

	/** Canonical title-bar close button. The pawn owns the close transition/input mode. */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "RPG|Skills|UI")
	TObjectPtr<UButton> Button_CloseSkills;

	/** Mandatory empty-state label for a missing/empty canonical Skills projection. */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "RPG|Skills|UI|Presentation")
	TObjectPtr<UTextBlock> Text_EmptySkills;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Skills|UI|Events")
	FGridSkillsWidgetRefreshedSignature OnSkillsRefreshed;

	/** UI-session-only Skill selection. It is never saved and never gameplay authority. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Skills|UI")
	FName SelectedSkillId = NAME_None;

	/** Presentation-ready projection for SelectedSkillId; rebuilt from authoritative state. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Skills|UI")
	FGridSkillDetailView SelectedSkillDetail;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Skills|UI|Events")
	FGridSkillSelectionChangedSignature OnSkillSelectionChanged;

	/** UI-only conceptual selection. NAME_None means that no Talent node is selected. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName SelectedTalentNodeId = NAME_None;

	/** Concrete variant consulted before acquisition. UI-only and never persisted. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName SelectedTalentChoiceId = NAME_None;

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

	/** Applies presentation-only Skill ordering and rebuilds the list. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI|Sort")
	void SetSkillSort(EGridSkillSortColumn Column, EGridSkillSortDirection Direction);

	/** Deterministic presentation sort used by the widget and automation tests. */
	static void SortSkillEntries(
		TArray<FGridSkillEntryView>& InOutEntries,
		EGridSkillSortColumn Column,
		EGridSkillSortDirection Direction);

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

	/** Selects one Skill for consultation only; no rank mutation occurs. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI")
	bool SelectSkill(FName SkillId);

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI")
	void ClearSkillSelection();

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	bool GetSelectedSkillDetail(FGridSkillDetailView& OutDetail) const;

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

	/** Selects one concrete variant for consultation only; locked variants remain inspectable. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|UI")
	bool SelectTalentVariant(FName TalentNodeId, FName ChoiceId);

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
	virtual FReply NativeOnPreviewMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

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
	void HandleTalentVariantFlyoutChosen(FName ChoiceId);

	UFUNCTION()
	void HandleSkillSelectionRequested(FName SkillId);

	UFUNCTION()
	void HandleSkillIncreaseRequested(FName SkillId);

	UFUNCTION()
	void HandleSkillDecreaseRequested(FName SkillId);

	UFUNCTION()
	void HandleSortNameClicked();

	UFUNCTION()
	void HandleSortAttributeClicked();

	UFUNCTION()
	void HandleSortAffinityClicked();

	UFUNCTION()
	void HandleSortRankClicked();

	void ToggleSkillSortColumn(EGridSkillSortColumn Column);
	void RefreshSkillSortIndicators();
	void RefreshSkillSummaryPresentation();

	const URPGSkillAsset* ResolveCanonicalSkillDefinition(FName SkillId) const;
	FString MakeSkillAllocationSessionKey(const FGuid& CharacterId, FName SkillId) const;
	int32 GetSessionPurchasedSkillRankCount(const FGuid& CharacterId, FName SkillId) const;
	int32 GetSessionSkillRankFloor(const FGuid& CharacterId, FName SkillId) const;
	void RecordSessionSkillPurchase(const FGuid& CharacterId, FName SkillId, int32 PreviousRank);
	void ConsumeSessionSkillPurchase(const FGuid& CharacterId, FName SkillId);
	void RefreshSelectedSkillDetail(const TArray<const URPGSkillAsset*>& SkillDefinitions);
	void RefreshSkillEntrySelection();
	void ApplySkillDetailPresentation();

	const FGridTalentNodeView* FindTalentNode(FName TalentNodeId) const;
	UGridTalentNodeWidget* FindTalentNodeWidget(const FGridTalentNodeView& NodeView) const;
	bool BuildSelectedTalentDetailView(FGridTalentNodeView& OutDetailNode) const;
	bool OpenTalentVariantFlyout(FName TalentNodeId);
	void CloseTalentVariantFlyout();
	void ClearView();
	void BindDesignerShell();
	void UnbindDesignerShell();
	void ApplyDesignerPresentation();
	const URPGClassVisualAsset* ResolveCurrentClassVisual() const;
	void ApplyClassVisualIdentity();
	void RefreshTabSelection();
	void ApplyTalentDetailPresentation();

	/** Class-specific header banner sourced from URPGClassVisualAsset::Banner. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_Header;

	/** Class-specific flag sourced from URPGClassVisualAsset::Flag. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_ClassFlag;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CharacterName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ClassLevel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_TalentPoints;

	/** UI-SKILLS-SUMMARY01.1 migration-only legacy combined summary; removed after WBP split. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillPoints;

	/** UI-SKILLS-SUMMARY01: dynamic value only; static label belongs to ST_UI in Designer. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AvailableSkillPoints;

	/** UI-SKILLS-SUMMARY01: dynamic value only; static label belongs to ST_UI in Designer. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CurrentSkillRankCap;

	/** UI-SKILLS-SORT01: materialized column sort controls in WBP_GridSkills. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_SortName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_SortAttribute;

	/** UI-SKILLS-AFFINITY01.2D: materialized Affinité sort header. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_SortAffinity;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_SortRank;

	/** Mechanical sort glyphs only; static column labels reference ST_UI directly in Designer. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SortNameIndicator;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SortAttributeIndicator;

	/** Mechanical ▲ / ▼ indicator for the materialized Affinité sort header. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SortAffinityIndicator;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SortRankIndicator;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_SkillsTab;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_TalentsTab;

	/** Presentation-only selection frame shown while the Skills page is active. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_SkillsTabSelectionFrame;

	/** Presentation-only selection frame shown while the Talents page is active. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_TalentsTabSelectionFrame;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher_SkillsTalents;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridTalentBranchWidget> Branch_Left;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridTalentBranchWidget> Branch_Center;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridTalentBranchWidget> Branch_Right;

	/** Materialized by UI-SKILLS02.5; the Skill browser requires its detail panel. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridSkillDetailWidget> Detail_Skill;

	/** UI-RPG04.2A: optional until WBP_RPGTalentDetail is materialized in Designer. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UGridTalentDetailWidget> Detail_Talent;

	/** UI-RPG05 : optionnel jusqu’à matérialisation de WBP_RPGNotification. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UGridRPGNotificationWidget> Notification_Progression;

	/** Viewport popup owned by Skills. Z=9000 intentionally stays below WBP_GridMouseCursor (Z=9999). */
	UPROPERTY(Transient)
	TObjectPtr<UGridTalentVariantFlyoutWidget> ActiveVariantFlyout;

	FName ActiveVariantFlyoutNodeId = NAME_None;

	/** UI-session-only history. Never persisted and never gameplay authority. */
	TMap<FString, int32> SessionPurchasedSkillRanks;
	TMap<FString, int32> SessionSkillRankFloors;

	bool bRefreshInProgress = false;
};
