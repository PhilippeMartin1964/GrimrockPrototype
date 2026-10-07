#include "UI/GridRPGNotificationWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UGridRPGNotificationWidget::ShowNotification(const FRPGProgressionNotificationView& Notification)
{
	if (!Notification.IsValid())
	{
		DismissNotification();
		return;
	}

	CurrentNotification = Notification;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ApplyPresentation();
	OnNotificationShown.Broadcast(CurrentNotification);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoDismissTimerHandle);
		if (CurrentNotification.DurationSeconds > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				AutoDismissTimerHandle,
				this,
				&UGridRPGNotificationWidget::DismissNotification,
				CurrentNotification.DurationSeconds,
				false);
		}
	}
}

void UGridRPGNotificationWidget::DismissNotification()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoDismissTimerHandle);
	}
	CurrentNotification = FRPGProgressionNotificationView();
	SetVisibility(ESlateVisibility::Collapsed);
	ApplyPresentation();
}

void UGridRPGNotificationWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoDismissTimerHandle);
	}
	Super::NativeDestruct();
}

void UGridRPGNotificationWidget::ApplyPresentation()
{
	if (Text_NotificationTitle)
	{
		Text_NotificationTitle->SetText(CurrentNotification.Title);
	}
	if (Text_NotificationMessage)
	{
		Text_NotificationMessage->SetText(CurrentNotification.Message);
	}
	if (Border_NotificationAccent)
	{
		Border_NotificationAccent->SetBrushColor(ResolveAccentColor());
	}
}

FLinearColor UGridRPGNotificationWidget::ResolveAccentColor() const
{
	switch (CurrentNotification.Severity)
	{
		case ERPGProgressionNotificationSeverity::Success:
			return FLinearColor(0.32f, 0.58f, 0.24f, 1.0f);
		case ERPGProgressionNotificationSeverity::Warning:
			return FLinearColor(0.78f, 0.56f, 0.16f, 1.0f);
		case ERPGProgressionNotificationSeverity::Error:
			return FLinearColor(0.72f, 0.18f, 0.14f, 1.0f);
		case ERPGProgressionNotificationSeverity::Info:
		default:
			return FLinearColor(0.32f, 0.46f, 0.70f, 1.0f);
	}
}
