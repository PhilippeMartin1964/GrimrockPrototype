#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RPGStoryCompanionRecruitmentWidget.generated.h"

class UButton;
class UGridPartyInventoryComponent;
class UImage;
class UPanelWidget;
class URPGStoryCompanionAsset;
class UTextBlock;
class UTexture2D;
class FReply;
class SButton;
class STextBlock;
class SWidget;

UENUM(BlueprintType)
enum class ERPGStoryCompanionRecruitmentState : uint8
{
	Uninitialized,
	Ready,
	Recruited,
	AlreadyActive,
	PartyFull,
	Declined,
	Invalid,
	Failed
};

USTRUCT(BlueprintType)
struct FRPGStoryCompanionRecruitmentView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	FName CompanionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	FGuid CharacterId;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	FText ShortDescription;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	FText RaceName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	FText ClassName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	TSoftObjectPtr<UTexture2D> FullBody;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	TSoftObjectPtr<UTexture2D> ClassIcon;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	FText RecruitmentConditionText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	ERPGStoryCompanionRecruitmentState State = ERPGStoryCompanionRecruitmentState::Uninitialized;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	FText StatusText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	bool bCanRecruit = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	bool bCandidateAlreadyRegistered = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	bool bDetailsVisible = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FRPGStoryCompanionRecruitmentWidgetAcceptedNativeSignature, class URPGStoryCompanionRecruitmentWidget*);
DECLARE_MULTICAST_DELEGATE_OneParam(FRPGStoryCompanionRecruitmentWidgetDeclinedNativeSignature, class URPGStoryCompanionRecruitmentWidget*);
DECLARE_MULTICAST_DELEGATE_OneParam(FRPGStoryCompanionRecruitmentWidgetClosedNativeSignature, class URPGStoryCompanionRecruitmentWidget*);

/**
 * MON20.4 story-companion recruitment modal.
 *
 * The widget is presentation/orchestration only. It never mutates
 * PartyInventoryState directly: registration remains authoritative in MON20.3
 * and active-party activation remains authoritative in MON20.2.
 *
 * A native Slate fallback keeps the whole transaction testable without a WBP.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API URPGStoryCompanionRecruitmentWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Recruitment")
	FRPGStoryCompanionRecruitmentView View;

	UFUNCTION(BlueprintCallable, Category = "RPG|Recruitment")
	bool InitializeRecruitmentWidget(UGridPartyInventoryComponent* InInventoryComponent, URPGStoryCompanionAsset* InCompanionDefinition);

	/** Returns true only when this call commits a new active-party recruit. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Recruitment")
	bool TryRecruit();

	UFUNCTION(BlueprintCallable, Category = "RPG|Recruitment")
	void DeclineRecruitment();

	UFUNCTION(BlueprintCallable, Category = "RPG|Recruitment")
	void ToggleDetails();

	UFUNCTION(BlueprintCallable, Category = "RPG|Recruitment")
	void CloseRecruitment();

	UFUNCTION(BlueprintPure, Category = "RPG|Recruitment")
	URPGStoryCompanionAsset* GetCompanionDefinition() const;

	FRPGStoryCompanionRecruitmentWidgetAcceptedNativeSignature& OnAccepted();
	FRPGStoryCompanionRecruitmentWidgetDeclinedNativeSignature& OnDeclined();
	FRPGStoryCompanionRecruitmentWidgetClosedNativeSignature& OnClosed();

	virtual void SynchronizeProperties() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "RPG|Recruitment")
	void BP_OnRecruitmentViewRefreshed();

private:
	UPROPERTY(Transient)
	TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<URPGStoryCompanionAsset> CompanionDefinition;

	bool bInputGuardApplied = false;
	bool bPreviousInventoryUiOpen = false;
	bool bGamePausedByModal = false;
	bool bCloseBroadcast = false;

	FRPGStoryCompanionRecruitmentWidgetAcceptedNativeSignature AcceptedDelegate;
	FRPGStoryCompanionRecruitmentWidgetDeclinedNativeSignature DeclinedDelegate;
	FRPGStoryCompanionRecruitmentWidgetClosedNativeSignature ClosedDelegate;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Recruit;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Decline;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ShowDetails;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Identity;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Details;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ShowDetailsAction;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Portrait;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_FullBody;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_ClassIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Panel_Details;

	TSharedPtr<STextBlock> NativeNameText;
	TSharedPtr<STextBlock> NativeIdentityText;
	TSharedPtr<STextBlock> NativeDescriptionText;
	TSharedPtr<STextBlock> NativeStatusText;
	TSharedPtr<STextBlock> NativeDetailsText;
	TSharedPtr<SButton> NativeRecruitButton;
	TSharedPtr<SButton> NativeDetailsButton;

	void BindButtons();
	void RefreshView();
	void RefreshPresentation();
	void RefreshBoundWidgets();
	void RefreshNativeSlate();
	void ApplyInputGuard();
	void RestoreInputGuard();
	void CloseModal();

	FText BuildIdentityLine() const;
	FText BuildDetailsText() const;
	FText BuildRegistrationFailureText(const FString& Error) const;
	FText BuildRecruitmentFailureText(int32 RejectReasonValue, const FString& Error) const;

	UFUNCTION()
	void HandleRecruitClicked();

	UFUNCTION()
	void HandleDeclineClicked();

	UFUNCTION()
	void HandleShowDetailsClicked();

	FReply HandleNativeRecruitClicked();
	FReply HandleNativeDeclineClicked();
	FReply HandleNativeDetailsClicked();
};
