#include "Runtime/GridLevelRuntimeActor.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "UI/ReadableMessageWidget.h"
void AGridLevelRuntimeActor::ShowReadableMessage(const FText& MessageText)
{
	if (MessageText.IsEmpty())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || !ReadableMessageWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowReadableMessage failed: missing world or widget class."));
		return;
	}
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowReadableMessage failed: missing player controller."));
		return;
	}
	if (!ActiveReadableMessageWidget)
	{
		ActiveReadableMessageWidget = CreateWidget<UReadableMessageWidget>(PlayerController, ReadableMessageWidgetClass);

		if (!ActiveReadableMessageWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("ShowReadableMessage failed: widget creation failed."));
			return;
		}
		ActiveReadableMessageWidget->AddToViewport(50);
	}
	ActiveReadableMessageWidget->SetReadableText(MessageText);
	GetWorldTimerManager().ClearTimer(ReadableMessageTimerHandle);
	if (bReadableMessageAutoHide)
	{
		GetWorldTimerManager().SetTimer(ReadableMessageTimerHandle, this, &AGridLevelRuntimeActor::HideReadableMessage, ReadableMessageDuration, false);
	}
	if (!ActiveReadableMessageWidget->IsInViewport())
	{
		ActiveReadableMessageWidget->AddToViewport(50);
	}
}

bool AGridLevelRuntimeActor::HasActiveReadableMessage() const
{
	return ActiveReadableMessageWidget && ActiveReadableMessageWidget->IsInViewport();
}

bool AGridLevelRuntimeActor::DismissReadableMessage()
{
	if (!HasActiveReadableMessage())
	{
		return false;
	}

	HideReadableMessage();
	return true;
}

void AGridLevelRuntimeActor::HideReadableMessage()
{
	GetWorldTimerManager().ClearTimer(ReadableMessageTimerHandle);

	if (ActiveReadableMessageWidget)
	{
		ActiveReadableMessageWidget->RemoveFromParent();
		ActiveReadableMessageWidget = nullptr;
	}
}

void AGridLevelRuntimeActor::ShowInteractionFeedback(const FText& MessageText, float DurationSeconds)
{
	if (MessageText.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	const TSubclassOf<UReadableMessageWidget> WidgetClass = InteractionFeedbackWidgetClass ? InteractionFeedbackWidgetClass : ReadableMessageWidgetClass;
	if (!World || !WidgetClass)
	{
		UE_LOG(LogTemp, Verbose, TEXT("ShowInteractionFeedback skipped: missing world or widget class."));
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	if (!ActiveInteractionFeedbackWidget)
	{
		ActiveInteractionFeedbackWidget = CreateWidget<UReadableMessageWidget>(PlayerController, WidgetClass);
		if (!ActiveInteractionFeedbackWidget)
		{
			return;
		}
	}

	ActiveInteractionFeedbackWidget->SetReadableText(MessageText);
	if (!ActiveInteractionFeedbackWidget->IsInViewport())
	{
		ActiveInteractionFeedbackWidget->AddToViewport(60);
	}

	GetWorldTimerManager().ClearTimer(InteractionFeedbackTimerHandle);
	GetWorldTimerManager().SetTimer(
		InteractionFeedbackTimerHandle, this, &AGridLevelRuntimeActor::HideInteractionFeedback, FMath::Max(0.1f, DurationSeconds), false);
}

void AGridLevelRuntimeActor::HideInteractionFeedback()
{
	GetWorldTimerManager().ClearTimer(InteractionFeedbackTimerHandle);

	if (ActiveInteractionFeedbackWidget)
	{
		ActiveInteractionFeedbackWidget->RemoveFromParent();
		ActiveInteractionFeedbackWidget = nullptr;
	}
}

void AGridLevelRuntimeActor::ShowCombatFeedback(const FGridPlayerAttackFeedbackRequest& Feedback)
{
	if (Feedback.PrimaryText.IsEmpty())
	{
		return;
	}
	UWorld* World = GetWorld();
	const TSubclassOf<UReadableMessageWidget> WidgetClass = CombatFeedbackWidgetClass ? CombatFeedbackWidgetClass
		: InteractionFeedbackWidgetClass                                              ? InteractionFeedbackWidgetClass
																					  : ReadableMessageWidgetClass;
	if (!World || !WidgetClass)
	{
		UE_LOG(LogTemp, Verbose, TEXT("ShowCombatFeedback skipped: missing world or widget class."));
		return;
	}
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}
	if (!ActiveCombatFeedbackWidget)
	{
		ActiveCombatFeedbackWidget = CreateWidget<UReadableMessageWidget>(PlayerController, WidgetClass);
		if (!ActiveCombatFeedbackWidget)
		{
			return;
		}
	}
	const FText DisplayText = Feedback.DetailText.IsEmpty()
		? Feedback.PrimaryText
		: FText::Format(NSLOCTEXT("GridPlayerAttackPresentation", "FeedbackWithDetail", "{0}\n{1}"), Feedback.PrimaryText, Feedback.DetailText);
	ActiveCombatFeedbackWidget->SetReadableText(DisplayText);
	if (!ActiveCombatFeedbackWidget->IsInViewport())
	{
		ActiveCombatFeedbackWidget->AddToViewport(65);
	}
	GetWorldTimerManager().ClearTimer(CombatFeedbackTimerHandle);
	GetWorldTimerManager().SetTimer(
		CombatFeedbackTimerHandle, this, &AGridLevelRuntimeActor::HideCombatFeedback, FMath::Clamp(Feedback.DurationSeconds, 0.1f, 10.0f), false);
}

void AGridLevelRuntimeActor::HideCombatFeedback()
{
	GetWorldTimerManager().ClearTimer(CombatFeedbackTimerHandle);
	if (ActiveCombatFeedbackWidget)
	{
		ActiveCombatFeedbackWidget->RemoveFromParent();
		ActiveCombatFeedbackWidget = nullptr;
	}
}
