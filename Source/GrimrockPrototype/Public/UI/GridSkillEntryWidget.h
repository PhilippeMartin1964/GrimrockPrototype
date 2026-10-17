#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "GridSkillEntryWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

UENUM(BlueprintType)
enum class EGridSkillEntryVisualState : uint8
{
	Normal,
	Hovered,
	Selected
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridSkillSelectionRequestedSignature, FName, SkillId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridSkillIncreaseRequestedSignature, FName, SkillId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridSkillDecreaseRequestedSignature, FName, SkillId);

/**
 * Presentation-only row for one canonical Skill.
 *
 * The widget owns no Skill progression state. It receives one immutable
 * FGridSkillEntryView produced by FGridSkillsPageService and mirrors only
 * fields that already exist in the read model.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridSkillEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Entry")
	FGridSkillEntryView Entry;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Entry")
	bool bInitialized = false;

	/** UI-only selected state controlled by UGridSkillsWidget. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Entry|Interaction")
	bool bSelected = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Entry|Interaction")
	EGridSkillEntryVisualState VisualState = EGridSkillEntryVisualState::Normal;

	/** Presentation icon resolved from URPGSkillPresentationAsset; never gameplay authority. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Entry|Presentation")
	TObjectPtr<UTexture2D> SkillIcon = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Skills|Entry|Events")
	FGridSkillSelectionRequestedSignature OnSelectSkillRequested;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Skills|Entry|Events")
	FGridSkillIncreaseRequestedSignature OnIncreaseSkillRequested;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Skills|Entry|Events")
	FGridSkillDecreaseRequestedSignature OnDecreaseSkillRequested;

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Entry")
	bool InitializeSkillEntry(const FGridSkillEntryView& InEntry);

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Entry")
	void ClearSkillEntry();

	/** Sets the presentation icon without changing Skill identity or progression state. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Entry|Presentation")
	void SetSkillIcon(UTexture2D* InIcon);

	/** Applies selection state supplied by the parent browser. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Entry|Interaction")
	void SetSelected(bool bInSelected);

	/** Emits a presentation-only selection request for this row. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Entry|Interaction")
	bool RequestSkillSelection();

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Entry|Interaction")
	static EGridSkillEntryVisualState ResolveVisualState(bool bInSelected, bool bInHovered);

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Entry")
	static EGridSkillTrainingState ResolveTrainingState(const FGridSkillEntryView& InEntry);

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Entry")
	void RefreshEntryVisual();

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Entry")
	static FText ResolveDisplayName(const FGridSkillEntryView& InEntry);

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Entry")
	static FText ResolveAttributeLabel(const FGridSkillEntryView& InEntry);

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Entry")
	static FText ResolveRankLabel(const FGridSkillEntryView& InEntry);

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Entry")
	static FText ResolveTrainingLabel(const FGridSkillEntryView& InEntry);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	void RefreshInteractionVisual();

	UFUNCTION()
	void HandleIncreaseSkillClicked();

	UFUNCTION()
	void HandleDecreaseSkillClicked();

	static bool IsValidEntry(const FGridSkillEntryView& InEntry);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SkillName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SkillAttribute;

	/** Designer-owned backgrounds; C++ only selects the semantic state. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_AffinityRecommended;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_AffinityUseful;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_AffinityNeutral;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_AffinityDiscouraged;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SkillRank;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SkillTrainingPolicy;

	/** UI-SKILLS02.3 Designer materialization is deferred to UI-SKILLS02.5. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_SkillIcon;

	/** Designer-owned appearance; C++ only toggles semantic selected visibility. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_SkillSelection;

	/** Designer-owned appearance; hidden whenever selection takes precedence. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_SkillHover;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_IncreaseSkill;

	/** RPG-SKILL01.3B Designer materialization; optional until WBP_RPGSkillEntry is updated manually. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_DecreaseSkill;

	bool bHovered = false;
};
