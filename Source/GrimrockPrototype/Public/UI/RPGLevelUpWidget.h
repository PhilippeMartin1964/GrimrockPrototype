#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RPGLevelUpWidget.generated.h"

class UGridPartyInventoryComponent;
class URPGClassAsset;
class FReply;
class SButton;
class SWidget;
class STextBlock;
class SVerticalBox;

USTRUCT(BlueprintType)
struct FRPGLevelUpChoiceView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	FName ChoiceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 PointCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 MinimumLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	bool bCommitted = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	bool bPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	bool bAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	FText StatusText;
};

/**
 * MON20.7 presentation-only vocabulary for the existing MON15 Level Up flow.
 * No gameplay authority lives here: ChoiceId and ChoicePoints remain MON15.
 */
USTRUCT(BlueprintType)
struct FRPGLevelUpPresentationView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up|Presentation")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up|Presentation")
	FText TalentSectionTitle;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up|Presentation")
	FText TalentPointsLabel;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up|Presentation")
	FText EmptyTalentsMessage;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up|Presentation")
	FText SelectionPrompt;
};

USTRUCT(BlueprintType)
struct FRPGLevelUpView
{
	GENERATED_BODY()

	/** Presentation vocabulary only. Gameplay state continues to use Choice*. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	FRPGLevelUpPresentationView Presentation;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 CharacterIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	FText CharacterName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	FText ClassName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 PreviousLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 NewLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 PreviousMaxHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 NewMaxHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 PreviousMaxMana = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 NewMaxMana = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 PreviousPhysicalArmor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 NewPhysicalArmor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 PreviousMagicalArmor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 NewMagicalArmor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 GrantedChoicePoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 SpentChoicePoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	int32 RemainingChoicePoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	bool bHasSelectableChoices = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	bool bCanConfirm = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	FText ValidationMessage;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	TArray<FRPGLevelUpChoiceView> Choices;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FRPGLevelUpWidgetClosedNativeSignature, class URPGLevelUpWidget*);

/**
 * MON15.5 modal. The native Slate fallback is fully usable without a WBP;
 * Blueprint subclasses can instead project the public View.
 * MON20.7.3 adds Talent vocabulary only; transaction semantics remain MON15.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API URPGLevelUpWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Level Up")
	FRPGLevelUpView View;

	UFUNCTION(BlueprintCallable, Category = "RPG|Level Up")
	bool InitializeLevelUpWidget(UGridPartyInventoryComponent* InInventoryComponent, int32 InCharacterIndex, int32 InPreviousLevel, int32 InNewLevel);

	UFUNCTION(BlueprintCallable, Category = "RPG|Level Up")
	bool StageOrUnstageChoice(FName ChoiceId);

	UFUNCTION(BlueprintCallable, Category = "RPG|Level Up")
	bool ConfirmSelection();

	UFUNCTION(BlueprintCallable, Category = "RPG|Level Up")
	void CancelSelection();

	UFUNCTION(BlueprintPure, Category = "RPG|Level Up")
	TArray<FName> GetPendingChoiceIds() const;

	FRPGLevelUpWidgetClosedNativeSignature& OnClosed();

	virtual void SynchronizeProperties() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "RPG|Level Up")
	void BP_OnLevelUpViewRefreshed();

private:
	UPROPERTY(Transient)
	TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	int32 CharacterIndex = INDEX_NONE;

	UPROPERTY(Transient)
	int32 PreviousLevel = 1;

	UPROPERTY(Transient)
	int32 NewLevel = 1;

	UPROPERTY(Transient)
	TArray<FName> PendingChoiceIds;

	bool bInputGuardApplied = false;
	bool bPreviousInventoryUiOpen = false;
	bool bGamePausedByModal = false;
	FRPGLevelUpWidgetClosedNativeSignature ClosedDelegate;

	TSharedPtr<STextBlock> NativeTitleText;
	TSharedPtr<STextBlock> NativeCharacterText;
	TSharedPtr<STextBlock> NativeStatsText;
	TSharedPtr<STextBlock> NativePointsText;
	TSharedPtr<STextBlock> NativeTalentSectionText;
	TSharedPtr<STextBlock> NativeValidationText;
	TSharedPtr<SVerticalBox> NativeChoicesBox;
	TSharedPtr<SButton> NativeConfirmButton;

	URPGClassAsset* ResolveClassDefinition() const;
	bool BuildCombinedSelection(TSet<FName>& OutSelectedChoiceIds, TArray<FName>* OutCommittedChoiceIds = nullptr) const;
	bool CanRemovePendingChoice(FName ChoiceId) const;
	void RefreshView();
	void RefreshNativeSlate();
	void ApplyInputGuard();
	void RestoreInputGuard();
	void CloseModal();
	FText BuildCharacterLine() const;
	FText BuildStatsLine() const;
	FText BuildPointsLine() const;
	FText GetChoiceStatusText(int32 AvailabilityReasonValue) const;

	FReply HandleNativeChoiceClicked(FName ChoiceId);
	FReply HandleNativeConfirmClicked();
	FReply HandleNativeCancelClicked();
};
