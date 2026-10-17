#include "Runtime/GridLevelRuntimeActor.h"

#include "Blueprint/UserWidget.h"
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
	// EVENTHUD01.8: accepted attacks are projected centrally from the authoritative
	// combat log. This legacy facade only preserves rejected player feedback.
	HideCombatFeedback();
	if (Feedback.PrimaryText.IsEmpty() || Feedback.bAccepted)
	{
		return;
	}

	FGridHudEventEntry Entry;
	Entry.Category = EGridHudEventCategory::Combat;
	Entry.Semantic = EGridHudEventSemantic::Interaction;
	Entry.Visibility = EGridEventVisibility::Always;
	Entry.EventCode = FName(TEXT("Combat.Attack.Rejected"));
	Entry.Message = Feedback.PrimaryText;
	Entry.SourceId = GridEventFeedPublisher::SourceIdFromGuid(Feedback.AttackRequest.AttackerCharacterId);
	if (!Feedback.DetailText.IsEmpty())
	{
		Entry.Details.Add(Feedback.DetailText);
	}
	GridEventFeedPublisher::PublishEvent(this, Entry, true);
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
