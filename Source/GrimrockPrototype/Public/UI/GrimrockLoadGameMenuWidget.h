#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrimrockLoadGameMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UGrimrockLoadGameSlotWidget;

/**
 * Load-game screen that lists existing party save slots and opens the runtime map.
 *
 * WBP_LoadGameMenu should derive from this class and provide a VerticalBox named
 * VerticalBox_SaveSlots plus an optional Button_Back and Text_EmptyState.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGrimrockLoadGameMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Load Game")
    void RefreshSaveSlots();

protected:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Load Game")
    void OnLoadSlotRequestFailed(const FString& SlotName, int32 UserIndex);

private:
    void BindButtons();
    void SetEmptyStateVisible(bool bIsVisible);

    UFUNCTION()
    void HandleBackClicked();

    UFUNCTION()
    void HandleSaveSlotSelected(const FString& SlotName, int32 UserIndex);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Load Game", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UGrimrockLoadGameSlotWidget> SaveSlotEntryWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Load Game", meta = (AllowPrivateAccess = "true"))
    FName RuntimeLevelName = TEXT("L_GrimrockEditor");

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> VerticalBox_SaveSlots;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Back;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_EmptyState;
};