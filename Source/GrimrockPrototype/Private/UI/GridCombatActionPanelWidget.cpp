#include "UI/GridCombatActionPanelWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

namespace
{
	FText FormatCurrentAndMaximum(int32 CurrentValue, int32 MaximumValue)
	{
		return FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::Max(0, CurrentValue), FMath::Max(0, MaximumValue)));
	}

	FText FormatActionPoints(int32 CurrentValue, int32 MaximumValue)
	{
		return FText::FromString(FString::Printf(TEXT("PA %d / %d"), FMath::Max(0, CurrentValue), FMath::Max(0, MaximumValue)));
	}

	FText BuildStatusToolTip(const TArray<FGridStatusEffectPresentationView>& StatusEffects)
	{
		TArray<FText> ToolTips;
		for (const FGridStatusEffectPresentationView& Status : StatusEffects)
		{
			if (!Status.ToolTipText.IsEmpty())
			{
				ToolTips.Add(Status.ToolTipText);
			}
		}
		return FText::Join(FText::FromString(TEXT("\n\n")), ToolTips);
	}
}

void UGridCombatActionPanelWidget::SetView(const FGridCombatHudPartyMemberView& InView)
{
	View = InView;
	RefreshBoundWidgets();
}

FText UGridCombatActionPanelWidget::GetActionStateText() const
{
	switch (View.TurnState)
	{
		case EGridCombatantTurnState::Active:
			return FText::FromString(TEXT("ACTIF"));
		case EGridCombatantTurnState::Completed:
			return FText::GetEmpty();
		case EGridCombatantTurnState::Incapacitated:
			return FText::FromString(TEXT("INCAPACITÉ"));
		case EGridCombatantTurnState::Defeated:
			return FText::FromString(TEXT("VAINCU"));
		case EGridCombatantTurnState::Waiting:
		default:
			return FText::GetEmpty();
	}
}

void UGridCombatActionPanelWidget::RefreshBoundWidgets()
{
	SetVisibility(View.bPresent ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (Image_Portrait)
	{
		if (View.Portrait.IsNull())
		{
			Image_Portrait->SetBrushFromTexture(nullptr);
			Image_Portrait->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Image_Portrait->SetBrushFromSoftTexture(View.Portrait, false);
			Image_Portrait->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	if (Text_Name)
	{
		Text_Name->SetText(View.DisplayName);
	}
	if (Text_Health)
	{
		Text_Health->SetText(FormatCurrentAndMaximum(View.CurrentHealth, View.MaximumHealth));
	}
	if (Text_Mana)
	{
		Text_Mana->SetText(FormatCurrentAndMaximum(View.CurrentMana, View.MaximumMana));
	}
	if (Text_ActionPoints)
	{
		Text_ActionPoints->SetText(FormatActionPoints(View.RemainingActionPoints, View.MaximumActionPoints));
	}

	if (Text_ActionState)
	{
		const FText ActionStateText = GetActionStateText();
		Text_ActionState->SetText(ActionStateText);
		Text_ActionState->SetVisibility(ActionStateText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (Border_ActionState)
	{
		FLinearColor StateColor = WaitingColor;
		switch (View.TurnState)
		{
			case EGridCombatantTurnState::Active:
				StateColor = ReadyColor;
				break;
			case EGridCombatantTurnState::Completed:
				StateColor = AlreadyActedColor;
				break;
			case EGridCombatantTurnState::Incapacitated:
				StateColor = IncapacitatedColor;
				break;
			case EGridCombatantTurnState::Defeated:
				StateColor = DefeatedColor;
				break;
			case EGridCombatantTurnState::Waiting:
			default:
				break;
		}
		Border_ActionState->SetBrushColor(StateColor);
	}
	if (Panel_DisabledOverlay)
	{
		Panel_DisabledOverlay->SetVisibility(View.bCanAct ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (Text_StatusEffects)
	{
		Text_StatusEffects->SetText(View.StatusSummary);
		Text_StatusEffects->SetToolTipText(BuildStatusToolTip(View.StatusEffects));
		Text_StatusEffects->SetVisibility(View.StatusSummary.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (Text_StatusFeedback)
	{
		Text_StatusFeedback->SetText(View.LatestStatusFeedback);
		Text_StatusFeedback->SetVisibility(View.LatestStatusFeedback.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	SetIsEnabled(View.bCanAct);
	SetRenderOpacity(View.bCanAct ? 1.0f : DisabledOpacity);
}
