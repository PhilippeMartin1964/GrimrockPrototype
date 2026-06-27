#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GrimrockGameInstance.h"
#include "GrimrockLoadGameSlotWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FGrimrockLoadGameSlotSelectedSignature,
    const FString&, SlotName,
    int32, UserIndex);

/**
 * One selectable save-slot row used by WBP_LoadGameMenu.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGrimrockLoadGameSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Load Game")
    void InitializeSaveSlot(const FGrimrockSaveSlotInfo& InSaveSlotInfo);

    UFUNCTION(BlueprintPure, Category = "Load Game")
    FGrimrockSaveSlotInfo GetSaveSlotInfo() const;

    UPROPERTY(BlueprintAssignable, Category = "Load Game")
    FGrimrockLoadGameSlotSelectedSignature OnSaveSlotSelected;

protected:
    virtual void NativeConstruct() override;

private:
    void BindSlotButton();
    void RefreshSlotText();

    UFUNCTION()
    void HandleLoadSlotClicked();

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Load Game", meta = (AllowPrivateAccess = "true"))
    FGrimrockSaveSlotInfo SaveSlotInfo;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_LoadSlot;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_DisplayName;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_SlotName;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Status;
};