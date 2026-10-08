#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGAttributePointService.h"
#include "UI/GridInventoryWidget.h"
#include "GridCharacterSheetWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * Independent left-side character/equipment window.
 *
 * RPG-ATTR01 adds only presentation/session state here. Character.Attributes
 * remains the durable authority and FRPGAttributePointService owns mutations.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridCharacterSheetWidget : public UGridInventoryWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|UI")
	TObjectPtr<UButton> Button_CloseCharacterSheet;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UTextBlock> Text_AttributePoints;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_DecreaseStrength;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_IncreaseStrength;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_DecreaseDexterity;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_IncreaseDexterity;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_DecreaseConstitution;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_IncreaseConstitution;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_DecreaseIntelligence;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_IncreaseIntelligence;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_DecreaseWisdom;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_IncreaseWisdom;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_DecreaseCharisma;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Attributes")
	TObjectPtr<UButton> Button_IncreaseCharisma;

	/** Reopening the Character Sheet commits the previous undo boundary. */
	UFUNCTION(BlueprintCallable, Category = "RPG|Attributes")
	void BeginAttributeAllocationSession();

	UFUNCTION(BlueprintCallable, Category = "RPG|Attributes")
	void RefreshAttributeAllocationPresentation();

	virtual void RefreshInventory() override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	TMap<FString, int32> SessionAttributeFloors;

	void BindAttributeButtons();
	void UnbindAttributeButtons();
	void CaptureSelectedCharacterSessionFloors();
	FString MakeSessionKey(const FGuid& CharacterId, ERPGAttributePointTarget Target) const;
	int32 GetSessionFloor(const FGridCharacterInventoryState& Character, ERPGAttributePointTarget Target) const;
	void SetAttributeButtonState(
		UButton* DecreaseButton,
		UButton* IncreaseButton,
		const FGridCharacterInventoryState& Character,
		ERPGAttributePointTarget Target);
	void CommitAttributeIncrease(ERPGAttributePointTarget Target, const FText& DisplayName);
	void CommitAttributeDecrease(ERPGAttributePointTarget Target, const FText& DisplayName);
	void ShowAttributeFeedback(const struct FRPGProgressionNotificationView& Notification);

	UFUNCTION()
	void HandleDecreaseStrengthClicked();
	UFUNCTION()
	void HandleIncreaseStrengthClicked();
	UFUNCTION()
	void HandleDecreaseDexterityClicked();
	UFUNCTION()
	void HandleIncreaseDexterityClicked();
	UFUNCTION()
	void HandleDecreaseConstitutionClicked();
	UFUNCTION()
	void HandleIncreaseConstitutionClicked();
	UFUNCTION()
	void HandleDecreaseIntelligenceClicked();
	UFUNCTION()
	void HandleIncreaseIntelligenceClicked();
	UFUNCTION()
	void HandleDecreaseWisdomClicked();
	UFUNCTION()
	void HandleIncreaseWisdomClicked();
	UFUNCTION()
	void HandleDecreaseCharismaClicked();
	UFUNCTION()
	void HandleIncreaseCharismaClicked();
};
