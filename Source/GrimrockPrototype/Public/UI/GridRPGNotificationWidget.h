#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "GridRPGNotificationWidget.generated.h"

class UBorder;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FGridRPGNotificationShownSignature,
	const FRPGProgressionNotificationView&,
	Notification);

/** Widget de présentation réutilisable pour les notifications de progression RPG. */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridRPGNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Progression|Notification")
	FRPGProgressionNotificationView CurrentNotification;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Progression|Notification|Events")
	FGridRPGNotificationShownSignature OnNotificationShown;

	UFUNCTION(BlueprintCallable, Category = "RPG|Progression|Notification")
	void ShowNotification(const FRPGProgressionNotificationView& Notification);

	UFUNCTION(BlueprintCallable, Category = "RPG|Progression|Notification")
	void DismissNotification();

	UFUNCTION(BlueprintPure, Category = "RPG|Progression|Notification")
	bool HasNotification() const { return CurrentNotification.IsValid(); }

protected:
	virtual void NativeDestruct() override;

private:
	void ApplyPresentation();
	FLinearColor ResolveAccentColor() const;

	FTimerHandle AutoDismissTimerHandle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_NotificationAccent;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_NotificationTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_NotificationMessage;
};
