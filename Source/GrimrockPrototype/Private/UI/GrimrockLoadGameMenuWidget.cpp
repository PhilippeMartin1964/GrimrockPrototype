#include "UI/GrimrockLoadGameMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GrimrockGameInstance.h"
#include "UI/GrimrockLoadGameSlotWidget.h"

void UGrimrockLoadGameMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindButtons();
    RefreshSaveSlots();
}

void UGrimrockLoadGameMenuWidget::RefreshSaveSlots()
{
    if (!VerticalBox_SaveSlots)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadGameMenu Refresh Failed Widget=%s Reason=NoVerticalBox_SaveSlots"), *GetName());
        SetEmptyStateVisible(true);
        return;
    }

    VerticalBox_SaveSlots->ClearChildren();

    if (!SaveSlotEntryWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadGameMenu Refresh Failed Widget=%s Reason=NoSaveSlotEntryWidgetClass"), *GetName());
        SetEmptyStateVisible(true);
        return;
    }

    UGrimrockGameInstance* GrimrockGameInstance = GetWorld()
        ? GetWorld()->GetGameInstance<UGrimrockGameInstance>()
        : nullptr;

    if (!GrimrockGameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadGameMenu Refresh Failed Widget=%s Reason=NoGrimrockGameInstance"), *GetName());
        SetEmptyStateVisible(true);
        return;
    }

    const TArray<FGrimrockSaveSlotInfo> ExistingSlots = GrimrockGameInstance->GetExistingPartySaveSlotInfos();
    SetEmptyStateVisible(ExistingSlots.Num() == 0);

    for (const FGrimrockSaveSlotInfo& SlotInfo : ExistingSlots)
    {
        UGrimrockLoadGameSlotWidget* SlotWidget = CreateWidget<UGrimrockLoadGameSlotWidget>(
            GetOwningPlayer(),
            SaveSlotEntryWidgetClass);

        if (!SlotWidget)
        {
            UE_LOG(LogTemp, Warning, TEXT("LoadGameMenu Slot Create Failed Slot=%s"), *SlotInfo.SlotName);
            continue;
        }

        SlotWidget->InitializeSaveSlot(SlotInfo);
        SlotWidget->OnSaveSlotSelected.RemoveDynamic(this, &UGrimrockLoadGameMenuWidget::HandleSaveSlotSelected);
        SlotWidget->OnSaveSlotSelected.AddDynamic(this, &UGrimrockLoadGameMenuWidget::HandleSaveSlotSelected);
        VerticalBox_SaveSlots->AddChildToVerticalBox(SlotWidget);
    }

    UE_LOG(LogTemp, Log, TEXT("LoadGameMenu Refreshed Widget=%s ExistingSlots=%d"), *GetName(), ExistingSlots.Num());
}

void UGrimrockLoadGameMenuWidget::BindButtons()
{
    if (!Button_Back)
    {
        return;
    }

    Button_Back->OnClicked.RemoveDynamic(this, &UGrimrockLoadGameMenuWidget::HandleBackClicked);
    Button_Back->OnClicked.AddDynamic(this, &UGrimrockLoadGameMenuWidget::HandleBackClicked);
}

void UGrimrockLoadGameMenuWidget::SetEmptyStateVisible(bool bIsVisible)
{
    if (Text_EmptyState)
    {
        Text_EmptyState->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UGrimrockLoadGameMenuWidget::HandleBackClicked()
{
    RemoveFromParent();
}

void UGrimrockLoadGameMenuWidget::HandleSaveSlotSelected(const FString& SlotName, int32 UserIndex)
{
    UGrimrockGameInstance* GrimrockGameInstance = GetWorld()
        ? GetWorld()->GetGameInstance<UGrimrockGameInstance>()
        : nullptr;

    if (!GrimrockGameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadGameMenu Load Failed Slot=%s UserIndex=%d Reason=NoGrimrockGameInstance"), *SlotName, UserIndex);
        OnLoadSlotRequestFailed(SlotName, UserIndex);
        return;
    }

    if (!GrimrockGameInstance->RequestLoadPartySaveSlot(SlotName, UserIndex))
    {
        OnLoadSlotRequestFailed(SlotName, UserIndex);
        RefreshSaveSlots();
        return;
    }

    if (RuntimeLevelName.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadGameMenu Load Failed Slot=%s UserIndex=%d Reason=NoRuntimeLevelName"), *SlotName, UserIndex);
        OnLoadSlotRequestFailed(SlotName, UserIndex);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("LoadGameMenu OpenRuntimeLevel Slot=%s UserIndex=%d Level=%s"), *SlotName, UserIndex, *RuntimeLevelName.ToString());
    UGameplayStatics::OpenLevel(this, RuntimeLevelName);
}