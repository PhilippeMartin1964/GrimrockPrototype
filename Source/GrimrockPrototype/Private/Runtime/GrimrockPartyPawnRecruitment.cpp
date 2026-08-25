#include "Runtime/GrimrockPartyPawn.h"

#include "GameFramework/PlayerController.h"
#include "RPG/RPGStoryCompanionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/RPGStoryCompanionRecruitmentWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridRecruitmentRuntime, Log, All);

bool AGrimrockPartyPawn::ShowStoryCompanionRecruitmentWidget(URPGStoryCompanionAsset* CompanionDefinition)
{
	if (!IsValid(PartyInventoryComponent))
	{
		UE_LOG(LogGridRecruitmentRuntime, Warning, TEXT("[GridRecruitmentRuntime] Show Failed Pawn=%s Reason=NoPartyInventoryComponent"), *GetName());
		return false;
	}

	if (!IsValid(CompanionDefinition) || !CompanionDefinition->IsValidDefinition())
	{
		UE_LOG(LogGridRecruitmentRuntime, Warning, TEXT("[GridRecruitmentRuntime] Show Failed Pawn=%s Companion=%s Reason=InvalidDefinition"), *GetName(),
			*GetNameSafe(CompanionDefinition));
		return false;
	}

	if (bCharacterCreationModalActive)
	{
		UE_LOG(LogGridRecruitmentRuntime, Verbose, TEXT("[GridRecruitmentRuntime] Show Rejected Pawn=%s Companion=%s Reason=CharacterCreationModalActive"),
			*GetName(), *CompanionDefinition->CompanionId.ToString());
		return false;
	}

	if (IsStoryCompanionRecruitmentModalActive())
	{
		UE_LOG(LogGridRecruitmentRuntime, Verbose, TEXT("[GridRecruitmentRuntime] Show Rejected Pawn=%s Companion=%s Reason=RecruitmentModalAlreadyActive"),
			*GetName(), *CompanionDefinition->CompanionId.ToString());
		return false;
	}

	if (StoryCompanionRecruitmentWidgetInstance)
	{
		StoryCompanionRecruitmentWidgetInstance = nullptr;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogGridRecruitmentRuntime, Warning, TEXT("[GridRecruitmentRuntime] Show Failed Pawn=%s Companion=%s Reason=NoPlayerController"), *GetName(),
			*CompanionDefinition->CompanionId.ToString());
		return false;
	}

	TSubclassOf<URPGStoryCompanionRecruitmentWidget> EffectiveWidgetClass = StoryCompanionRecruitmentWidgetClass;
	if (!EffectiveWidgetClass)
	{
		EffectiveWidgetClass = URPGStoryCompanionRecruitmentWidget::StaticClass();
	}

	URPGStoryCompanionRecruitmentWidget* Widget = CreateWidget<URPGStoryCompanionRecruitmentWidget>(PlayerController, EffectiveWidgetClass);
	if (!Widget)
	{
		UE_LOG(LogGridRecruitmentRuntime, Warning, TEXT("[GridRecruitmentRuntime] Show Failed Pawn=%s Companion=%s Reason=CreateWidgetFailed"), *GetName(),
			*CompanionDefinition->CompanionId.ToString());
		return false;
	}

	if (!Widget->InitializeRecruitmentWidget(PartyInventoryComponent, CompanionDefinition))
	{
		UE_LOG(LogGridRecruitmentRuntime, Warning, TEXT("[GridRecruitmentRuntime] Show Failed Pawn=%s Companion=%s Reason=InitializeWidgetFailed"), *GetName(),
			*CompanionDefinition->CompanionId.ToString());
		return false;
	}

	ClearBufferedCommand();
	Widget->OnClosed().AddUObject(this, &AGrimrockPartyPawn::HandleStoryCompanionRecruitmentClosed);

	StoryCompanionRecruitmentWidgetInstance = Widget;
	Widget->AddToViewport(FMath::Max(0, StoryCompanionRecruitmentZOrder));
	Widget->SetVisibility(ESlateVisibility::Visible);

	UE_LOG(LogGridRecruitmentRuntime, Log, TEXT("[GridRecruitmentRuntime] Shown Pawn=%s Companion=%s WidgetClass=%s ZOrder=%d"), *GetName(),
		*CompanionDefinition->CompanionId.ToString(), *GetNameSafe(EffectiveWidgetClass.Get()), FMath::Max(0, StoryCompanionRecruitmentZOrder));
	return true;
}

void AGrimrockPartyPawn::CloseStoryCompanionRecruitmentWidget()
{
	URPGStoryCompanionRecruitmentWidget* Widget = StoryCompanionRecruitmentWidgetInstance.Get();
	if (!IsValid(Widget))
	{
		StoryCompanionRecruitmentWidgetInstance = nullptr;
		return;
	}

	Widget->CloseRecruitment();
	if (StoryCompanionRecruitmentWidgetInstance == Widget)
	{
		StoryCompanionRecruitmentWidgetInstance = nullptr;
	}
}

bool AGrimrockPartyPawn::IsStoryCompanionRecruitmentModalActive() const
{
	return IsValid(StoryCompanionRecruitmentWidgetInstance) && StoryCompanionRecruitmentWidgetInstance->IsInViewport();
}

URPGStoryCompanionRecruitmentWidget* AGrimrockPartyPawn::GetStoryCompanionRecruitmentWidget() const
{
	return IsValid(StoryCompanionRecruitmentWidgetInstance) ? StoryCompanionRecruitmentWidgetInstance.Get() : nullptr;
}

void AGrimrockPartyPawn::HandleStoryCompanionRecruitmentClosed(URPGStoryCompanionRecruitmentWidget* ClosedWidget)
{
	if (StoryCompanionRecruitmentWidgetInstance == ClosedWidget)
	{
		StoryCompanionRecruitmentWidgetInstance = nullptr;
	}

	UE_LOG(LogGridRecruitmentRuntime, Verbose, TEXT("[GridRecruitmentRuntime] Closed Pawn=%s Widget=%s"), *GetName(), *GetNameSafe(ClosedWidget));
}
