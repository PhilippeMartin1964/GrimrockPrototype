#include "Runtime/GridLevelRuntimeActor.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/GridEventFeedPublisher.h"
#include "UI/ReadableMessageWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridLevelRuntime, Log, All);
void AGridLevelRuntimeActor::ShowReadableMessage(const FText& MessageText)
{
	if (MessageText.IsEmpty())
	{
		return;
	}

	// EVENTHUD01.5 compatibility façade: readable world information is no
	// longer modal and no longer owns a viewport widget.
	HideReadableMessage();
	GridEventFeedPublisher::PublishDungeonEvent(
		this,
		FName(TEXT("Dungeon.Information.Readable")),
		MessageText,
		EGridHudEventSemantic::Information,
		EGridEventVisibility::Always,
		true,
		NAME_None,
		NSLOCTEXT("GridEventFeed", "InformationHeader", "Information"));
}

bool AGridLevelRuntimeActor::HasActiveReadableMessage() const
{
	// EVENTHUD01.5: world readable content is non-blocking Event Feed content.
	return false;
}

bool AGridLevelRuntimeActor::DismissReadableMessage()
{
	const bool bHadLegacyPopup = ActiveReadableMessageWidget && ActiveReadableMessageWidget->IsInViewport();
	HideReadableMessage();
	return bHadLegacyPopup;
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
	(void)DurationSeconds;
	if (MessageText.IsEmpty())
	{
		return;
	}

	// EVENTHUD01.5 compatibility façade: short non-blocking interaction errors
	// now use the persistent feed instead of a transient popup.
	HideInteractionFeedback();
	GridEventFeedPublisher::PublishDungeonEvent(
		this,
		FName(TEXT("Dungeon.Interaction.Feedback")),
		MessageText,
		EGridHudEventSemantic::Interaction,
		EGridEventVisibility::Always,
		true);
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
		UE_LOG(LogGridLevelRuntime, Verbose, TEXT("ShowCombatFeedback skipped: missing world or widget class."));
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
